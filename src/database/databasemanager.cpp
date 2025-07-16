#include "databasemanager.h"
#include <QDir>
#include <QStandardPaths>
#include <QMutexLocker>
#include <QDebug>
#include <QSqlRecord>
#include <QVariant>

// 静态成员初始化
DatabaseManager* DatabaseManager::m_instance = nullptr;
QMutex DatabaseManager::m_mutex;
const QString DatabaseManager::CONNECTION_NAME = "MusicPlayerDB";

DatabaseManager* DatabaseManager::instance()
{
    QMutexLocker locker(&m_mutex);
    if (!m_instance) {
        m_instance = new DatabaseManager();
    }
    return m_instance;
}

DatabaseManager::DatabaseManager(QObject* parent)
    : QObject(parent)
    , m_initialized(false)
{
    qDebug() << "DatabaseManager 构造函数";
}

DatabaseManager::~DatabaseManager()
{
    closeDatabase();
    qDebug() << "DatabaseManager 析构函数";
}

bool DatabaseManager::initialize(const QString& dbPath)
{
    qDebug() << "DatabaseManager::initialize() - 开始初始化，路径:" << dbPath;
    
    if (m_initialized) {
        qDebug() << "数据库已经初始化";
        return true;
    }
    
    // 确保数据库目录存在
    QFileInfo fileInfo(dbPath);
    QDir dir = fileInfo.absoluteDir();
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            logError("无法创建数据库目录: " + dir.absolutePath());
            return false;
        }
        qDebug() << "创建数据库目录:" << dir.absolutePath();
    }
    
    // 创建数据库连接
    m_database = QSqlDatabase::addDatabase("QSQLITE", CONNECTION_NAME);
    m_database.setDatabaseName(dbPath);
    
    qDebug() << "尝试打开数据库:" << dbPath;
    if (!m_database.open()) {
        logError("无法打开数据库: " + m_database.lastError().text());
        return false;
    }
    
    qDebug() << "数据库连接成功";
    
    // 创建表结构
    if (!createTables()) {
        logError("创建数据库表失败");
        closeDatabase();
        return false;
    }
    
    // 插入初始数据
    if (!insertInitialData()) {
        logError("插入初始数据失败");
        closeDatabase();
        return false;
    }

    // 检查并修复系统标签
    qDebug() << "开始检查系统标签";
    if (!checkAndFixSystemTags()) {
        logError("检查系统标签失败");
        // 不返回false，因为这不是致命错误
    } else {
        qDebug() << "系统标签检查完成";
    }

    m_initialized = true;
    qDebug() << "数据库初始化完成";
    return true;
}

bool DatabaseManager::isValid() const
{
    return m_initialized && m_database.isOpen() && m_database.isValid();
}

QSqlDatabase DatabaseManager::database() const
{
    return QSqlDatabase::database(CONNECTION_NAME);
}

QSqlQuery DatabaseManager::executeQuery(const QString& queryStr)
{
    QSqlQuery query(database());
    if (!query.exec(queryStr)) {
        logError("查询执行失败: " + query.lastError().text() + " SQL: " + queryStr);
        // 返回一个无效的查询对象
        return QSqlQuery();
    }
    return query;
}

bool DatabaseManager::executeUpdate(const QString& queryStr)
{
    QSqlQuery query(database());
    if (!query.exec(queryStr)) {
        logError("更新操作失败: " + query.lastError().text() + " SQL: " + queryStr);
        return false;
    }
    return true;
}

void DatabaseManager::closeDatabase()
{
    if (m_database.isOpen()) {
        m_database.close();
        qDebug() << "数据库连接已关闭";
    }
    QSqlDatabase::removeDatabase(CONNECTION_NAME);
    m_initialized = false;
}

bool DatabaseManager::createTables()
{
    qDebug() << "开始创建数据库表";
    
    if (!createSongsTable()) {
        return false;
    }
    
    if (!createTagsTable()) {
        return false;
    }
    
    if (!createSongTagsTable()) {
        return false;
    }
    
    if (!createLogsTable()) {
        return false;
    }
    
    qDebug() << "所有数据库表创建完成";
    return true;
}

bool DatabaseManager::createSongsTable()
{
    const QString createSongsSQL = R"(
        CREATE TABLE IF NOT EXISTS songs (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            title TEXT NOT NULL,
            artist TEXT,
            album TEXT,
            file_path TEXT NOT NULL UNIQUE,
            duration INTEGER DEFAULT 0,
            file_size INTEGER DEFAULT 0,
            date_added DATETIME DEFAULT CURRENT_TIMESTAMP,
            last_played DATETIME,
            play_count INTEGER DEFAULT 0,
            rating INTEGER DEFAULT 0 CHECK (rating >= 0 AND rating <= 5),
            tags TEXT,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    )";
    
    if (!executeUpdate(createSongsSQL)) {
        logError("创建songs表失败");
        return false;
    }
    
    // 创建索引
    const QStringList indexes = {
        "CREATE INDEX IF NOT EXISTS idx_songs_title ON songs(title)",
        "CREATE INDEX IF NOT EXISTS idx_songs_artist ON songs(artist)",
        "CREATE INDEX IF NOT EXISTS idx_songs_file_path ON songs(file_path)",
        "CREATE INDEX IF NOT EXISTS idx_songs_date_added ON songs(date_added)"
    };
    
    for (const QString& indexSQL : indexes) {
        if (!executeUpdate(indexSQL)) {
            logError("创建songs表索引失败: " + indexSQL);
            return false;
        }
    }
    
    qDebug() << "songs表创建成功";
    return true;
}

bool DatabaseManager::createTagsTable()
{
    const QString createTagsSQL = R"(
        CREATE TABLE IF NOT EXISTS tags (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL UNIQUE,
            color TEXT DEFAULT '#3498db',
            description TEXT,
            is_system INTEGER DEFAULT 0,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    )";
    
    if (!executeUpdate(createTagsSQL)) {
        logError("创建tags表失败");
        return false;
    }
    
    // 创建索引
    const QString indexSQL = "CREATE INDEX IF NOT EXISTS idx_tags_name ON tags(name)";
    if (!executeUpdate(indexSQL)) {
        logError("创建tags表索引失败");
        return false;
    }
    
    qDebug() << "tags表创建成功";
    return true;
}

bool DatabaseManager::createSongTagsTable()
{
    const QString createSongTagsSQL = R"(
        CREATE TABLE IF NOT EXISTS song_tags (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            song_id INTEGER NOT NULL,
            tag_id INTEGER NOT NULL,
            added_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (song_id) REFERENCES songs(id) ON DELETE CASCADE,
            FOREIGN KEY (tag_id) REFERENCES tags(id) ON DELETE CASCADE,
            UNIQUE(song_id, tag_id)
        )
    )";
    
    if (!executeUpdate(createSongTagsSQL)) {
        logError("创建song_tags表失败");
        return false;
    }
    
    // 创建索引以提高查询性能
    const QStringList indexes = {
        "CREATE INDEX IF NOT EXISTS idx_song_tags_song_id ON song_tags(song_id)",
        "CREATE INDEX IF NOT EXISTS idx_song_tags_tag_id ON song_tags(tag_id)",
        "CREATE INDEX IF NOT EXISTS idx_song_tags_added_at ON song_tags(added_at)"
    };
    
    for (const QString& indexSQL : indexes) {
        if (!executeUpdate(indexSQL)) {
            logError("创建song_tags表索引失败: " + indexSQL);
            return false;
        }
    }
    
    qDebug() << "song_tags表创建成功";
    return true;
}

bool DatabaseManager::createLogsTable()
{
    const QString createLogsSQL = R"(
        CREATE TABLE IF NOT EXISTS logs (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            level TEXT NOT NULL,
            message TEXT NOT NULL,
            category TEXT,
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    )";
    
    if (!executeUpdate(createLogsSQL)) {
        logError("创建logs表失败");
        return false;
    }
    
    // 创建索引
    const QStringList indexes = {
        "CREATE INDEX IF NOT EXISTS idx_logs_level ON logs(level)",
        "CREATE INDEX IF NOT EXISTS idx_logs_timestamp ON logs(timestamp)"
    };
    
    for (const QString& indexSQL : indexes) {
        if (!executeUpdate(indexSQL)) {
            logError("创建logs表索引失败: " + indexSQL);
            return false;
        }
    }
    
    qDebug() << "logs表创建成功";
    return true;
}

bool DatabaseManager::insertInitialData()
{
    qDebug() << "开始插入初始数据";
    
    // 先清理多余的标签，只保留三个系统标签
    if (!cleanupExtraTags()) {
        logError("清理多余标签失败");
        return false;
    }
    
    // 检查并创建必需的系统标签
    const QStringList systemTags = {
        "('我的歌曲', '#4CAF50', '所有歌曲的默认标签', 1)",
        "('最近播放', '#2196F3', '最近播放的歌曲', 1)",
        "('我的收藏', '#FF9800', '收藏的歌曲', 1)"
    };
    
    for (const QString& tagData : systemTags) {
        // 提取标签名称进行检查
        QString tagName = tagData.split("'")[1];
        
        // 检查标签是否已存在
        QSqlQuery checkQuery = executeQuery(QString("SELECT COUNT(*) FROM tags WHERE name = '%1'").arg(tagName));
        if (checkQuery.next() && checkQuery.value(0).toInt() > 0) {
            qDebug() << "系统标签已存在，跳过:" << tagName;
            continue;
        }
        
        QString insertSQL = "INSERT INTO tags (name, color, description, is_system) VALUES " + tagData;
        if (!executeUpdate(insertSQL)) {
            logError("插入系统标签失败: " + tagData);
            return false;
        }
        qDebug() << "插入系统标签成功:" << tagName;
    }
    
    qDebug() << "初始数据插入完成";
    return true;
}

bool DatabaseManager::cleanupExtraTags()
{
    qDebug() << "开始清理多余标签";
    
    // 定义需要保留的系统标签
    const QStringList requiredSystemTags = {"我的歌曲", "最近播放", "我的收藏"};
    
    // 构建删除SQL，删除不在必需列表中的所有标签
    QString deleteSQL = "DELETE FROM tags WHERE name NOT IN ('我的歌曲', '最近播放', '我的收藏')";
    
    if (!executeUpdate(deleteSQL)) {
        logError("清理多余标签失败");
        return false;
    }
    
    // 查询剩余标签数量
    QSqlQuery countQuery = executeQuery("SELECT COUNT(*) FROM tags");
    if (countQuery.next()) {
        int remainingCount = countQuery.value(0).toInt();
        qDebug() << "清理完成，剩余标签数量:" << remainingCount;
    }
    
    return true;
}

bool DatabaseManager::checkAndFixSystemTags()
{
    qDebug() << "检查并修复系统标签";
    
    // 检查必需的系统标签是否存在
    const QStringList requiredSystemTags = {"我的歌曲", "最近播放", "我的收藏"};
    
    for (const QString& tagName : requiredSystemTags) {
        QSqlQuery checkQuery = executeQuery(QString("SELECT COUNT(*) FROM tags WHERE name = '%1' AND is_system = 1").arg(tagName));
        
        if (!checkQuery.isValid() || !checkQuery.next() || checkQuery.value(0).toInt() == 0) {
            qDebug() << "系统标签缺失，正在添加:" << tagName;
            
            // 根据标签名称设置颜色和描述
            QString color, description;
            if (tagName == "我的歌曲") {
                color = "#4CAF50";
                description = "所有歌曲的默认标签";
            } else if (tagName == "最近播放") {
                color = "#2196F3";
                description = "最近播放的歌曲";
            } else if (tagName == "我的收藏") {
                color = "#FF9800";
                description = "收藏的歌曲";
            }
            
            QString insertSQL = QString("INSERT INTO tags (name, color, description, is_system) VALUES ('%1', '%2', '%3', 1)")
                               .arg(tagName, color, description);
            
            if (!executeUpdate(insertSQL)) {
                logError("添加系统标签失败: " + tagName);
                return false;
            }
            
            qDebug() << "系统标签添加成功:" << tagName;
        }
    }
    
    qDebug() << "系统标签检查完成";
    return true;
}

void DatabaseManager::logError(const QString& error)
{
    m_lastError = error;
    qCritical() << "DatabaseManager Error:" << error;
    
    // 简单的错误记录，不使用弹窗避免阻塞
    if (m_initialized && database().isOpen()) {
        QString errorMessage = error;
        QString logSQL = QString("INSERT INTO logs (level, message, category) VALUES ('ERROR', '%1', 'Database')")
                        .arg(errorMessage.replace("'", "''"));
        QSqlQuery query(database());
        query.exec(logSQL);
    }
}