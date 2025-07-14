#include "databasemanager.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDir>
#include <QStandardPaths>
#include <QMutexLocker>
#include <QDebug>

// 静态成员初始化
DatabaseManager* DatabaseManager::m_instance = nullptr;
QMutex DatabaseManager::m_mutex;
const QString DatabaseManager::DEFAULT_CONNECTION_NAME = "default";

DatabaseManager* DatabaseManager::instance()
{
    if (m_instance == nullptr) {
        QMutexLocker locker(&m_mutex);
        if (m_instance == nullptr) {
            m_instance = new DatabaseManager();
        }
    }
    return m_instance;
}

DatabaseManager::DatabaseManager(QObject* parent)
    : QObject(parent)
    , m_initialized(false)
    , m_inTransaction(false)
{
}

DatabaseManager::~DatabaseManager()
{
    closeDatabase();
}

bool DatabaseManager::initialize(const QString& dbPath)
{
    QMutexLocker locker(&m_dbMutex);
    
    if (m_initialized) {
        return true;
    }
    
    m_databasePath = dbPath;
    
    // 确保数据库目录存在
    QFileInfo fileInfo(dbPath);
    QDir dir = fileInfo.dir();
    if (!dir.exists()) {
        if (!dir.mkpath(dir.absolutePath())) {
            logError("Failed to create database directory: " + dir.absolutePath());
            return false;
        }
    }
    
    // 创建数据库连接
    m_database = QSqlDatabase::addDatabase("QSQLITE", DEFAULT_CONNECTION_NAME);
    m_database.setDatabaseName(dbPath);
    
    if (!m_database.open()) {
        logError("Failed to open database: " + m_database.lastError().text());
        return false;
    }
    
    // 设置数据库选项
    QSqlQuery query(m_database);
    query.exec("PRAGMA foreign_keys = ON");
    query.exec("PRAGMA journal_mode = WAL");
    query.exec("PRAGMA synchronous = NORMAL");
    
    // TODO: 创建表结构
    if (!createTables()) {
        logError("Failed to create database tables");
        return false;
    }
    
    // TODO: 创建索引
    if (!createIndexes()) {
        logError("Failed to create database indexes");
        return false;
    }
    
    // TODO: 创建触发器
    if (!createTriggers()) {
        logError("Failed to create database triggers");
        return false;
    }
    
    // TODO: 插入初始数据
    if (!insertInitialData()) {
        logError("Failed to insert initial data");
        return false;
    }
    
    m_initialized = true;
    emit databaseConnected();
    
    qDebug() << "Database initialized successfully:" << dbPath;
    return true;
}

QSqlDatabase DatabaseManager::database()
{
    return m_database;
}

QSqlDatabase DatabaseManager::database(const QString& connectionName)
{
    return QSqlDatabase::database(connectionName);
}

bool DatabaseManager::beginTransaction()
{
    QMutexLocker locker(&m_dbMutex);
    
    if (m_inTransaction) {
        return false;
    }
    
    if (m_database.transaction()) {
        m_inTransaction = true;
        emit transactionStarted();
        return true;
    }
    
    logError("Failed to begin transaction: " + m_database.lastError().text());
    return false;
}

bool DatabaseManager::commitTransaction()
{
    QMutexLocker locker(&m_dbMutex);
    
    if (!m_inTransaction) {
        return false;
    }
    
    if (m_database.commit()) {
        m_inTransaction = false;
        emit transactionCommitted();
        return true;
    }
    
    logError("Failed to commit transaction: " + m_database.lastError().text());
    return false;
}

bool DatabaseManager::rollbackTransaction()
{
    QMutexLocker locker(&m_dbMutex);
    
    if (!m_inTransaction) {
        return false;
    }
    
    if (m_database.rollback()) {
        m_inTransaction = false;
        emit transactionRolledBack();
        return true;
    }
    
    logError("Failed to rollback transaction: " + m_database.lastError().text());
    return false;
}

bool DatabaseManager::isValid() const
{
    return m_initialized && m_database.isValid() && m_database.isOpen();
}

QString DatabaseManager::lastError() const
{
    return m_lastError;
}

void DatabaseManager::closeDatabase()
{
    QMutexLocker locker(&m_dbMutex);
    
    if (m_database.isOpen()) {
        m_database.close();
    }
    
    m_initialized = false;
    m_inTransaction = false;
    
    qDebug() << "Database closed";
}

bool DatabaseManager::createTables()
{
    QMutexLocker locker(&m_mutex);
    
    if (!isValid()) {
        qCritical() << "Database not connected";
        return false;
    }
    
    // 创建歌曲表
    if (!createSongsTable()) {
        qCritical() << "Failed to create songs table";
        return false;
    }
    
    // 创建标签表
    if (!createTagsTable()) {
        qCritical() << "Failed to create tags table";
        return false;
    }
    
    // 创建错误日志表
    if (!createErrorLogTable()) {
        qCritical() << "Failed to create error log table";
        return false;
    }
    
    // 创建系统日志表
    if (!createSystemLogTable()) {
        qCritical() << "Failed to create system log table";
        return false;
    }
    
    return true;
}

bool DatabaseManager::createErrorLogTable()
{
    QSqlQuery query(m_database);
    
    QString createSql = R"(
        CREATE TABLE IF NOT EXISTS error_logs (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp INTEGER NOT NULL,
            level TEXT NOT NULL,
            category TEXT NOT NULL,
            message TEXT NOT NULL,
            file_path TEXT,
            line_number INTEGER,
            function_name TEXT,
            thread_id TEXT,
            user_id TEXT,
            session_id TEXT,
            error_code INTEGER,
            stack_trace TEXT,
            system_info TEXT,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
    )";
    
    if (!query.exec(createSql)) {
        qCritical() << "Failed to create error_logs table:" << query.lastError().text();
        return false;
    }
    
    // 创建索引（分别执行每条语句）
    QStringList indexSqls = {
        "CREATE INDEX IF NOT EXISTS idx_error_logs_timestamp ON error_logs(timestamp)",
        "CREATE INDEX IF NOT EXISTS idx_error_logs_level ON error_logs(level)",
        "CREATE INDEX IF NOT EXISTS idx_error_logs_category ON error_logs(category)",
        "CREATE INDEX IF NOT EXISTS idx_error_logs_thread_id ON error_logs(thread_id)"
    };
    
    for (const QString& indexSql : indexSqls) {
        if (!query.exec(indexSql)) {
            qCritical() << "Failed to create error_logs index:" << indexSql << query.lastError().text();
            return false;
        }
    }
    
    return true;
}

bool DatabaseManager::createSystemLogTable()
{
    QSqlQuery query(m_database);
    
    QString createSql = R"(
        CREATE TABLE IF NOT EXISTS system_logs (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp INTEGER NOT NULL,
            level TEXT NOT NULL,
            category TEXT NOT NULL,
            message TEXT NOT NULL,
            component TEXT,
            operation TEXT,
            duration INTEGER,
            memory_usage INTEGER,
            cpu_usage REAL,
            thread_id TEXT,
            session_id TEXT,
            metadata TEXT,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
    )";
    
    if (!query.exec(createSql)) {
        qCritical() << "Failed to create system_logs table:" << query.lastError().text();
        return false;
    }
    
    // 创建索引（分别执行每条语句）
    QStringList indexSqls = {
        "CREATE INDEX IF NOT EXISTS idx_system_logs_timestamp ON system_logs(timestamp)",
        "CREATE INDEX IF NOT EXISTS idx_system_logs_level ON system_logs(level)",
        "CREATE INDEX IF NOT EXISTS idx_system_logs_category ON system_logs(category)",
        "CREATE INDEX IF NOT EXISTS idx_system_logs_component ON system_logs(component)",
        "CREATE INDEX IF NOT EXISTS idx_system_logs_thread_id ON system_logs(thread_id)"
    };
    
    for (const QString& indexSql : indexSqls) {
        if (!query.exec(indexSql)) {
            qCritical() << "Failed to create system_logs index:" << indexSql << query.lastError().text();
            return false;
        }
    }
    
    return true;
}

bool DatabaseManager::createIndexes()
{
    // TODO: 实现索引创建逻辑
    return true;
}

bool DatabaseManager::createTriggers()
{
    // TODO: 实现触发器创建逻辑
    return true;
}

bool DatabaseManager::insertInitialData()
{
    QSqlQuery query(m_database);
    
    // 检查是否已经有系统标签
    query.prepare("SELECT COUNT(*) FROM tags WHERE is_system = 1");
    if (!query.exec()) {
        qCritical() << "Failed to check system tags:" << query.lastError().text();
        return false;
    }
    
    if (query.next() && query.value(0).toInt() > 0) {
        // 系统标签已存在，无需重复插入
        return true;
    }
    
    // 插入系统默认标签
    QStringList systemTagsData = {
        "('我的歌曲', '所有歌曲的默认标签', 1, 1, 0, 0)",
        "('我的收藏', '收藏的歌曲', 1, 1, 0, 1)",
        "('最近播放', '最近播放的歌曲', 1, 1, 0, 2)"
    };
    
    QString insertSql = "INSERT INTO tags (name, description, tag_type, is_system, is_deletable, sort_order) VALUES ";
    insertSql += systemTagsData.join(", ");
    
    if (!query.exec(insertSql)) {
        qCritical() << "Failed to insert system tags:" << query.lastError().text();
        return false;
    }
    
    qInfo() << "System tags initialized successfully";
    return true;
}

void DatabaseManager::logError(const QString& error)
{
    m_lastError = error;
    qWarning() << "DatabaseManager Error:" << error;
    emit databaseError(error);
}

// SQL语句常量定义 - 待完善
const QString DatabaseManager::SQLStatements::CREATE_SONGS_TABLE = 
    "CREATE TABLE IF NOT EXISTS songs (...)";

const QString DatabaseManager::SQLStatements::CREATE_TAGS_TABLE = 
    "CREATE TABLE IF NOT EXISTS tags (...)";

// 其他SQL语句常量将在后续实现中添加

bool DatabaseManager::createSongsTable()
{
    QSqlQuery query(m_database);
    
    QString createSql = R"(
        CREATE TABLE IF NOT EXISTS songs (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            file_path TEXT NOT NULL UNIQUE,
            file_name TEXT NOT NULL,
            title TEXT,
            artist TEXT,
            album TEXT,
            duration INTEGER DEFAULT 0,
            file_size INTEGER DEFAULT 0,
            bit_rate INTEGER DEFAULT 0,
            sample_rate INTEGER DEFAULT 0,
            channels INTEGER DEFAULT 2,
            file_format TEXT,
            cover_path TEXT,
            has_lyrics BOOLEAN DEFAULT 0,
            lyrics_path TEXT,
            play_count INTEGER DEFAULT 0,
            last_played_time INTEGER,
            date_added INTEGER NOT NULL,
            date_modified INTEGER,
            is_favorite BOOLEAN DEFAULT 0,
            is_available BOOLEAN DEFAULT 1,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
    )";
    
    if (!query.exec(createSql)) {
        qCritical() << "Failed to create songs table:" << query.lastError().text();
        return false;
    }
    
    // 创建索引（分别执行每条语句）
    QStringList indexSqls = {
        "CREATE INDEX IF NOT EXISTS idx_songs_file_path ON songs(file_path)",
        "CREATE INDEX IF NOT EXISTS idx_songs_title ON songs(title)",
        "CREATE INDEX IF NOT EXISTS idx_songs_artist ON songs(artist)",
        "CREATE INDEX IF NOT EXISTS idx_songs_album ON songs(album)",
        "CREATE INDEX IF NOT EXISTS idx_songs_date_added ON songs(date_added)",
        "CREATE INDEX IF NOT EXISTS idx_songs_play_count ON songs(play_count)",
        "CREATE INDEX IF NOT EXISTS idx_songs_last_played ON songs(last_played_time)",
        "CREATE INDEX IF NOT EXISTS idx_songs_is_favorite ON songs(is_favorite)"
    };
    
    for (const QString& indexSql : indexSqls) {
        if (!query.exec(indexSql)) {
            qCritical() << "Failed to create songs index:" << indexSql << query.lastError().text();
            return false;
        }
    }
    
    return true;
}

bool DatabaseManager::createTagsTable()
{
    QSqlQuery query(m_database);
    
    QString createSql = R"(
        CREATE TABLE IF NOT EXISTS tags (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL UNIQUE,
            description TEXT,
            cover_path TEXT,
            color TEXT DEFAULT '#3498db',
            tag_type INTEGER DEFAULT 0,
            is_system BOOLEAN DEFAULT 0,
            is_deletable BOOLEAN DEFAULT 1,
            sort_order INTEGER DEFAULT 0,
            song_count INTEGER DEFAULT 0,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
    )";
    
    if (!query.exec(createSql)) {
        qCritical() << "Failed to create tags table:" << query.lastError().text();
        return false;
    }
    
    // 创建索引（分别执行每条语句）
    QStringList indexSqls = {
        "CREATE INDEX IF NOT EXISTS idx_tags_name ON tags(name)",
        "CREATE INDEX IF NOT EXISTS idx_tags_type ON tags(tag_type)",
        "CREATE INDEX IF NOT EXISTS idx_tags_sort_order ON tags(sort_order)"
    };
    
    for (const QString& indexSql : indexSqls) {
        if (!query.exec(indexSql)) {
            qCritical() << "Failed to create tags index:" << indexSql << query.lastError().text();
            return false;
        }
    }
    
    return true;
}