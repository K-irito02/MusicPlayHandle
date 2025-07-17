#include "tagmanager.h"
#include "../database/tagdao.h"
#include "../database/songdao.h"
#include "../core/logger.h"
#include <QMutexLocker>
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QDateTime>

static TagManager* s_instance = nullptr;
static QMutex s_mutex;

TagManager::TagManager(QObject* parent)
    : QObject(parent)
    , m_tagDao(new TagDao(this))
    , m_songDao(new SongDao(this))
{
    qDebug() << "TagManager: 初始化TagManager实例";
}

TagManager::~TagManager()
{
}

TagManager* TagManager::instance()
{
    QMutexLocker locker(&s_mutex);
    if (!s_instance) {
        s_instance = new TagManager();
    }
    return s_instance;
}

void TagManager::cleanup()
{
    QMutexLocker locker(&s_mutex);
    delete s_instance;
    s_instance = nullptr;
}

bool TagManager::initialize() { return true; }
void TagManager::shutdown() {}

TagOperationResult TagManager::createTag(const QString& name, const QString& description, const QColor& color, const QPixmap& icon)
{
    Q_UNUSED(icon);
    qDebug() << "[TagManager] createTag: 开始创建标签:" << name;
    
    // 检查标签是否已存在
    if (tagExists(name)) {
        qDebug() << "[TagManager] createTag: 标签已存在:" << name;
        return TagOperationResult(false, "标签已存在");
    }
    
    // 创建Tag对象
    Tag tag;
    tag.setName(name);
    tag.setDescription(description);
    tag.setColor(color.name());
    tag.setTagType(Tag::UserTag);  // 用户创建的标签
    tag.setCreatedAt(QDateTime::currentDateTime());
    tag.setUpdatedAt(QDateTime::currentDateTime());
    
    // 保存到数据库
    int tagId = m_tagDao->addTag(tag);
    if (tagId > 0) {
        qDebug() << "[TagManager] createTag: 标签创建成功, ID:" << tagId << ", 名称:" << name;
        return TagOperationResult(true, "标签创建成功");
    } else {
        qDebug() << "[TagManager] createTag: 标签创建失败:" << name;
        return TagOperationResult(false, "数据库保存失败");
    }
}

bool TagManager::tagExists(const QString& name) const
{
    qDebug() << "[TagManager] tagExists: 检查标签是否存在:" << name;
    bool exists = m_tagDao->tagExists(name);
    qDebug() << "[TagManager] tagExists: 标签" << name << "存在状态:" << exists;
    return exists;
}

bool TagManager::tagExists(int tagId) const
{
    Q_UNUSED(tagId)
    return false;
}

void TagManager::onTagChanged(int tagId) { Q_UNUSED(tagId); }
void TagManager::onSongChanged(int songId) { Q_UNUSED(songId); }
void TagManager::updateStatistics() {}
void TagManager::cleanupOrphanedTags() {}

QList<Tag> TagManager::getTagsForSong(int songId) const
{
    Q_UNUSED(songId)
    return QList<Tag>();
}

Tag TagManager::getTagByName(const QString& name) const
{
    qDebug() << "TagManager::getTagByName: 查询标签" << name;
    
    if (name.isEmpty()) {
        qWarning() << "TagManager::getTagByName: 标签名称为空";
        Logger::instance()->error("getTagByName: 标签名称为空", "TagManager");
        return Tag();
    }
    
    try {
        // 检查数据库连接
        if (!QSqlDatabase::database().isOpen()) {
            qCritical() << "TagManager::getTagByName: 数据库连接未打开";
            Logger::instance()->error("getTagByName: 数据库连接未打开", "TagManager");
            return Tag();
        }
        
        // 查询标签
        Tag tag = m_tagDao->getTagByName(name);
        
        if (tag.id() > 0) {
            qDebug() << "TagManager::getTagByName: 找到标签" << name << "ID:" << tag.id();
            Logger::instance()->info(QString("成功查询标签: %1 (ID: %2)").arg(name).arg(tag.id()), "TagManager");
        } else {
            qDebug() << "TagManager::getTagByName: 未找到标签" << name;
            Logger::instance()->warning(QString("未找到标签: %1").arg(name), "TagManager");
        }
        
        return tag;
        
    } catch (const std::exception& e) {
        QString errorMsg = QString("查询标签时发生异常: %1").arg(e.what());
        qCritical() << "TagManager::getTagByName:" << errorMsg;
        Logger::instance()->error(errorMsg, "TagManager");
        return Tag();
    } catch (...) {
        QString errorMsg = "查询标签时发生未知异常";
        qCritical() << "TagManager::getTagByName:" << errorMsg;
        Logger::instance()->error(errorMsg, "TagManager");
        return Tag();
    }
}

TagOperationResult TagManager::addSongToTag(int songId, int tagId)
{
    qDebug() << "TagManager::addSongToTag: 添加歌曲" << songId << "到标签" << tagId;
    
    // 参数验证
    if (songId <= 0) {
        QString errorMsg = QString("无效的歌曲ID: %1").arg(songId);
        qWarning() << "TagManager::addSongToTag:" << errorMsg;
        Logger::instance()->error(errorMsg, "TagManager");
        return TagOperationResult(false, errorMsg);
    }
    
    if (tagId <= 0) {
        QString errorMsg = QString("无效的标签ID: %1").arg(tagId);
        qWarning() << "TagManager::addSongToTag:" << errorMsg;
        Logger::instance()->error(errorMsg, "TagManager");
        return TagOperationResult(false, errorMsg);
    }
    
    try {
        // 检查数据库连接
        if (!QSqlDatabase::database().isOpen()) {
            QString errorMsg = "数据库连接未打开";
            qCritical() << "TagManager::addSongToTag:" << errorMsg;
            Logger::instance()->error(errorMsg, "TagManager");
            return TagOperationResult(false, errorMsg);
        }
        
        // 验证歌曲存在性
        Song song = m_songDao->getSongById(songId);
        if (song.id() <= 0) {
            QString errorMsg = QString("歌曲不存在: ID %1").arg(songId);
            qWarning() << "TagManager::addSongToTag:" << errorMsg;
            Logger::instance()->error(errorMsg, "TagManager");
            return TagOperationResult(false, errorMsg);
        }
        qDebug() << "TagManager::addSongToTag: 验证歌曲存在:" << song.title();
        
        // 验证标签存在性
        Tag tag = m_tagDao->getTagById(tagId);
        if (tag.id() <= 0) {
            QString errorMsg = QString("标签不存在: ID %1").arg(tagId);
            qWarning() << "TagManager::addSongToTag:" << errorMsg;
            Logger::instance()->error(errorMsg, "TagManager");
            return TagOperationResult(false, errorMsg);
        }
        qDebug() << "TagManager::addSongToTag: 验证标签存在:" << tag.name();
        
        // 检查是否已经存在关联
        if (isSongInTag(songId, tagId)) {
            QString warningMsg = QString("歌曲 %1 已经在标签 %2 中").arg(song.title()).arg(tag.name());
            qDebug() << "TagManager::addSongToTag:" << warningMsg;
            Logger::instance()->warning(warningMsg, "TagManager");
            return TagOperationResult(true, warningMsg); // 返回成功，因为目标状态已达成
        }
        
        // 开始事务
        QSqlDatabase db = QSqlDatabase::database();
        if (!db.transaction()) {
            QString errorMsg = QString("无法开始事务: %1").arg(db.lastError().text());
            qCritical() << "TagManager::addSongToTag:" << errorMsg;
            Logger::instance()->error(errorMsg, "TagManager");
            return TagOperationResult(false, errorMsg);
        }
        
        qDebug() << "TagManager::addSongToTag: 开始数据库事务";
        
        // 执行添加操作（这里需要实现具体的数据库操作）
        // 假设有一个song_tag_relations表来存储关联关系
        QSqlQuery query(db);
        query.prepare("INSERT INTO song_tag_relations (song_id, tag_id, created_at) VALUES (?, ?, ?)");
        query.addBindValue(songId);
        query.addBindValue(tagId);
        query.addBindValue(QDateTime::currentDateTime());
        
        if (!query.exec()) {
            QString errorMsg = QString("插入歌曲-标签关联失败: %1").arg(query.lastError().text());
            qCritical() << "TagManager::addSongToTag:" << errorMsg;
            Logger::instance()->error(errorMsg, "TagManager");
            
            // 回滚事务
            if (!db.rollback()) {
                qCritical() << "TagManager::addSongToTag: 事务回滚失败:" << db.lastError().text();
            }
            return TagOperationResult(false, errorMsg);
        }
        
        // 提交事务
        if (!db.commit()) {
            QString errorMsg = QString("提交事务失败: %1").arg(db.lastError().text());
            qCritical() << "TagManager::addSongToTag:" << errorMsg;
            Logger::instance()->error(errorMsg, "TagManager");
            
            // 尝试回滚
            if (!db.rollback()) {
                qCritical() << "TagManager::addSongToTag: 事务回滚失败:" << db.lastError().text();
            }
            return TagOperationResult(false, errorMsg);
        }
        
        qDebug() << "TagManager::addSongToTag: 成功添加歌曲" << song.title() << "到标签" << tag.name();
        Logger::instance()->info(QString("成功添加歌曲 %1 到标签 %2").arg(song.title()).arg(tag.name()), "TagManager");
        
        // 触发信号通知UI更新
        emit songAddedToTag(songId, tagId);
        qDebug() << "TagManager::addSongToTag: 发送songAddedToTag信号";
        
        // 更新统计信息
        updateStatistics();
        
        return TagOperationResult(true, QString("成功添加歌曲到标签"));
        
    } catch (const std::exception& e) {
        QString errorMsg = QString("添加歌曲到标签时发生异常: %1").arg(e.what());
        qCritical() << "TagManager::addSongToTag:" << errorMsg;
        Logger::instance()->error(errorMsg, "TagManager");
        
        // 尝试回滚事务
        QSqlDatabase db = QSqlDatabase::database();
        if (db.isOpen() && !db.rollback()) {
            qCritical() << "TagManager::addSongToTag: 异常处理中事务回滚失败:" << db.lastError().text();
        }
        
        return TagOperationResult(false, errorMsg);
    } catch (...) {
        QString errorMsg = "添加歌曲到标签时发生未知异常";
        qCritical() << "TagManager::addSongToTag:" << errorMsg;
        Logger::instance()->error(errorMsg, "TagManager");
        
        // 尝试回滚事务
        QSqlDatabase db = QSqlDatabase::database();
        if (db.isOpen() && !db.rollback()) {
            qCritical() << "TagManager::addSongToTag: 异常处理中事务回滚失败:" << db.lastError().text();
        }
        
        return TagOperationResult(false, errorMsg);
     }
}

bool TagManager::isSongInTag(int songId, int tagId) const
{
    qDebug() << "TagManager::isSongInTag: 检查歌曲" << songId << "是否在标签" << tagId << "中";
    
    if (songId <= 0 || tagId <= 0) {
        qWarning() << "TagManager::isSongInTag: 无效的参数 songId:" << songId << "tagId:" << tagId;
        return false;
    }
    
    try {
        if (!QSqlDatabase::database().isOpen()) {
            qCritical() << "TagManager::isSongInTag: 数据库连接未打开";
            return false;
        }
        
        QSqlQuery query;
        query.prepare("SELECT COUNT(*) FROM song_tag_relations WHERE song_id = ? AND tag_id = ?");
        query.addBindValue(songId);
        query.addBindValue(tagId);
        
        if (!query.exec()) {
            qCritical() << "TagManager::isSongInTag: 查询失败:" << query.lastError().text();
            return false;
        }
        
        if (query.next()) {
            int count = query.value(0).toInt();
            bool exists = count > 0;
            qDebug() << "TagManager::isSongInTag: 关联" << (exists ? "存在" : "不存在") << "count:" << count;
            return exists;
        }
        
        return false;
        
    } catch (const std::exception& e) {
        qCritical() << "TagManager::isSongInTag: 异常:" << e.what();
        return false;
    } catch (...) {
        qCritical() << "TagManager::isSongInTag: 未知异常";
        return false;
    }
}