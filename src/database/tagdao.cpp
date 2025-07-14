#include "tagdao.h"
#include "databasemanager.h"
#include "../core/logger.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>
#include <QDateTime>

TagDAO::TagDAO(DatabaseManager* dbManager, QObject* parent)
    : QObject(parent), m_dbManager(dbManager)
{
}

TagDAO::~TagDAO()
{
}

bool TagDAO::initialize()
{
    // TODO: 实现初始化逻辑
    return true;
}

void TagDAO::cleanup()
{
    // TODO: 实现清理逻辑
}

int TagDAO::insertTag(const Tag& tag)
{
    QSqlDatabase db = m_dbManager->database();
    if (!db.isOpen()) {
        qWarning() << "数据库未打开";
        return -1;
    }
    QSqlQuery query(db);
    query.prepare(R"(
        INSERT INTO tags (name, description, cover_path, color, tag_type, is_system, is_deletable, sort_order, song_count, created_at, updated_at)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");
    query.addBindValue(tag.name());
    query.addBindValue(tag.description());
    query.addBindValue(tag.coverPath());
    query.addBindValue(tag.color());
    query.addBindValue(static_cast<int>(tag.tagType()));
    query.addBindValue(tag.isSystem());
    query.addBindValue(tag.isDeletable());
    query.addBindValue(tag.sortOrder());
    query.addBindValue(tag.songCount());
    query.addBindValue(tag.createdAt().toSecsSinceEpoch());
    query.addBindValue(tag.updatedAt().toSecsSinceEpoch());
    if (!query.exec()) {
        qWarning() << "插入标签失败:" << query.lastError().text();
        return -1;
    }
    return query.lastInsertId().toInt();
}

int TagDAO::insertTags(const QList<Tag>& tags)
{
    // TODO: 实现批量插入标签逻辑
    Q_UNUSED(tags)
    return 0;
}

bool TagDAO::updateTag(const Tag& tag)
{
    if (!m_dbManager) {
        qWarning() << "数据库管理器为空";
        return false;
    }
    
    QSqlDatabase db = m_dbManager->database();
    if (!db.isOpen()) {
        qWarning() << "数据库未打开";
        return false;
    }
    
    QSqlQuery query(db);
    query.prepare(R"(
        UPDATE tags SET name = ?, description = ?, cover_path = ?, color = ?, 
        tag_type = ?, is_system = ?, is_deletable = ?, sort_order = ?, 
        song_count = ?, updated_at = ? WHERE id = ?
    )");
    
    query.addBindValue(tag.name());
    query.addBindValue(tag.description());
    query.addBindValue(tag.coverPath());
    query.addBindValue(tag.color());
    query.addBindValue(static_cast<int>(tag.tagType()));
    query.addBindValue(tag.isSystem());
    query.addBindValue(tag.isDeletable());
    query.addBindValue(tag.sortOrder());
    query.addBindValue(tag.songCount());
    query.addBindValue(QDateTime::currentDateTime().toSecsSinceEpoch());
    query.addBindValue(tag.id());
    
    if (!query.exec()) {
        qWarning() << "更新标签失败:" << query.lastError().text();
        return false;
    }
    
    return query.numRowsAffected() > 0;
}

bool TagDAO::deleteTag(int tagId)
{
    if (!m_dbManager) {
        qWarning() << "数据库管理器为空";
        return false;
    }
    
    QSqlDatabase db = m_dbManager->database();
    if (!db.isOpen()) {
        qWarning() << "数据库未打开";
        return false;
    }
    
    QSqlQuery query(db);
    query.prepare("DELETE FROM tags WHERE id = ?");
    query.addBindValue(tagId);
    
    if (!query.exec()) {
        qWarning() << "删除标签失败:" << query.lastError().text();
        return false;
    }
    
    return query.numRowsAffected() > 0;
}

bool TagDAO::deleteTagByName(const QString& name)
{
    if (!m_dbManager) {
        qWarning() << "数据库管理器为空";
        return false;
    }
    
    QSqlDatabase db = m_dbManager->database();
    if (!db.isOpen()) {
        qWarning() << "数据库未打开";
        return false;
    }
    
    QSqlQuery query(db);
    query.prepare("DELETE FROM tags WHERE name = ?");
    query.addBindValue(name);
    
    if (!query.exec()) {
        qWarning() << "根据名称删除标签失败:" << query.lastError().text();
        return false;
    }
    
    return query.numRowsAffected() > 0;
}

int TagDAO::deleteTags(const QList<int>& tagIds)
{
    // TODO: 实现批量删除标签逻辑
    Q_UNUSED(tagIds)
    return 0;
}

Tag TagDAO::getTag(int tagId)
{
    if (!m_dbManager) {
        qWarning() << "数据库管理器为空";
        return Tag();
    }
    
    QSqlDatabase db = m_dbManager->database();
    if (!db.isOpen()) {
        qWarning() << "数据库未打开";
        return Tag();
    }
    
    QSqlQuery query(db);
    query.prepare("SELECT id, name, description, cover_path, color, tag_type, is_system, is_deletable, sort_order, song_count, created_at, updated_at FROM tags WHERE id = ?");
    query.addBindValue(tagId);
    
    if (!query.exec()) {
        qWarning() << "查询标签失败:" << query.lastError().text();
        return Tag();
    }
    
    if (query.next()) {
        Tag tag;
        tag.setId(query.value("id").toInt());
        tag.setName(query.value("name").toString());
        tag.setDescription(query.value("description").toString());
        tag.setCoverPath(query.value("cover_path").toString());
        tag.setColor(query.value("color").toString());
        tag.setTagType(static_cast<Tag::TagType>(query.value("tag_type").toInt()));
        tag.setIsSystem(query.value("is_system").toBool());
        tag.setIsDeletable(query.value("is_deletable").toBool());
        tag.setSortOrder(query.value("sort_order").toInt());
        tag.setSongCount(query.value("song_count").toInt());
        tag.setCreatedAt(QDateTime::fromSecsSinceEpoch(query.value("created_at").toLongLong()));
        tag.setUpdatedAt(QDateTime::fromSecsSinceEpoch(query.value("updated_at").toLongLong()));
        return tag;
    }
    
    return Tag();
}

Tag TagDAO::getTagByName(const QString& name)
{
    if (!m_dbManager) {
        qWarning() << "数据库管理器为空";
        return Tag();
    }
    
    QSqlDatabase db = m_dbManager->database();
    if (!db.isOpen()) {
        qWarning() << "数据库未打开";
        return Tag();
    }
    
    QSqlQuery query(db);
    query.prepare("SELECT id, name, description, cover_path, color, tag_type, is_system, is_deletable, sort_order, song_count, created_at, updated_at FROM tags WHERE name = ?");
    query.addBindValue(name);
    
    if (!query.exec()) {
        qWarning() << "根据名称查询标签失败:" << query.lastError().text();
        return Tag();
    }
    
    if (query.next()) {
        Tag tag;
        tag.setId(query.value("id").toInt());
        tag.setName(query.value("name").toString());
        tag.setDescription(query.value("description").toString());
        tag.setCoverPath(query.value("cover_path").toString());
        tag.setColor(query.value("color").toString());
        tag.setTagType(static_cast<Tag::TagType>(query.value("tag_type").toInt()));
        tag.setIsSystem(query.value("is_system").toBool());
        tag.setIsDeletable(query.value("is_deletable").toBool());
        tag.setSortOrder(query.value("sort_order").toInt());
        tag.setSongCount(query.value("song_count").toInt());
        tag.setCreatedAt(QDateTime::fromSecsSinceEpoch(query.value("created_at").toLongLong()));
        tag.setUpdatedAt(QDateTime::fromSecsSinceEpoch(query.value("updated_at").toLongLong()));
        return tag;
    }
    
    return Tag();
}

QList<Tag> TagDAO::getAllTags()
{
    QList<Tag> tags;
    
    if (!m_dbManager) {
        qWarning() << "数据库管理器为空";
        return tags;
    }
    
    QSqlDatabase db = m_dbManager->database();
    if (!db.isOpen()) {
        qWarning() << "数据库未打开";
        return tags;
    }
    
    QSqlQuery query(db);
    query.prepare("SELECT id, name, description, cover_path, color, tag_type, is_system, is_deletable, sort_order, song_count, created_at, updated_at FROM tags ORDER BY sort_order, name");
    
    if (!query.exec()) {
        qWarning() << "查询所有标签失败:" << query.lastError().text();
        return tags;
    }
    
    while (query.next()) {
        Tag tag;
        tag.setId(query.value("id").toInt());
        tag.setName(query.value("name").toString());
        tag.setDescription(query.value("description").toString());
        tag.setCoverPath(query.value("cover_path").toString());
        tag.setColor(query.value("color").toString());
        tag.setTagType(static_cast<Tag::TagType>(query.value("tag_type").toInt()));
        tag.setIsSystem(query.value("is_system").toBool());
        tag.setIsDeletable(query.value("is_deletable").toBool());
        tag.setSortOrder(query.value("sort_order").toInt());
        tag.setSongCount(query.value("song_count").toInt());
        tag.setCreatedAt(QDateTime::fromSecsSinceEpoch(query.value("created_at").toLongLong()));
        tag.setUpdatedAt(QDateTime::fromSecsSinceEpoch(query.value("updated_at").toLongLong()));
        
        tags.append(tag);
    }
    
    return tags;
}

QList<Tag> TagDAO::getSystemTags()
{
    QList<Tag> tags;
    
    if (!m_dbManager) {
        qWarning() << "数据库管理器为空";
        return tags;
    }
    
    QSqlDatabase db = m_dbManager->database();
    if (!db.isOpen()) {
        qWarning() << "数据库未打开";
        return tags;
    }
    
    QSqlQuery query(db);
    query.prepare("SELECT id, name, description, cover_path, color, tag_type, is_system, is_deletable, sort_order, song_count, created_at, updated_at FROM tags WHERE is_system = 1 ORDER BY sort_order, name");
    
    if (!query.exec()) {
        qWarning() << "查询系统标签失败:" << query.lastError().text();
        return tags;
    }
    
    while (query.next()) {
        tags.append(createTagFromQuery(query));
    }
    
    return tags;
}

QList<Tag> TagDAO::getUserTags()
{
    QList<Tag> tags;
    
    if (!m_dbManager) {
        qWarning() << "数据库管理器为空";
        return tags;
    }
    
    QSqlDatabase db = m_dbManager->database();
    if (!db.isOpen()) {
        qWarning() << "数据库未打开";
        return tags;
    }
    
    QSqlQuery query(db);
    query.prepare("SELECT id, name, description, cover_path, color, tag_type, is_system, is_deletable, sort_order, song_count, created_at, updated_at FROM tags WHERE is_system = 0 ORDER BY sort_order, name");
    
    if (!query.exec()) {
        qWarning() << "查询用户标签失败:" << query.lastError().text();
        return tags;
    }
    
    while (query.next()) {
        tags.append(createTagFromQuery(query));
    }
    
    return tags;
}

QList<Tag> TagDAO::getTagsBySong(int songId)
{
    // TODO: 实现根据歌曲ID获取标签逻辑
    Q_UNUSED(songId)
    return QList<Tag>();
}

QList<Tag> TagDAO::searchTags(const QString& keyword)
{
    // TODO: 实现搜索标签逻辑
    Q_UNUSED(keyword)
    return QList<Tag>();
}

int TagDAO::getTagCount()
{
    // TODO: 实现获取标签总数逻辑
    return 0;
}

int TagDAO::getSongCountInTag(int tagId)
{
    // TODO: 实现获取标签中歌曲数量逻辑
    Q_UNUSED(tagId)
    return 0;
}

bool TagDAO::tagExists(int tagId)
{
    // TODO: 实现检查标签是否存在逻辑
    Q_UNUSED(tagId)
    return false;
}

bool TagDAO::nameExists(const QString& name)
{
    if (!m_dbManager) {
        qWarning() << "数据库管理器为空";
        return false;
    }
    
    QSqlDatabase db = m_dbManager->database();
    if (!db.isOpen()) {
        qWarning() << "数据库未打开";
        return false;
    }
    
    QSqlQuery query(db);
    query.prepare("SELECT COUNT(*) FROM tags WHERE name = ?");
    query.addBindValue(name);
    
    if (!query.exec()) {
        qWarning() << "检查标签名称是否存在失败:" << query.lastError().text();
        return false;
    }
    
    if (query.next()) {
        return query.value(0).toInt() > 0;
    }
    
    return false;
}

bool TagDAO::addSongToTag(int songId, int tagId)
{
    // TODO: 实现添加歌曲到标签逻辑
    Q_UNUSED(songId)
    Q_UNUSED(tagId)
    return false;
}

bool TagDAO::removeSongFromTag(int songId, int tagId)
{
    // TODO: 实现从标签中移除歌曲逻辑
    Q_UNUSED(songId)
    Q_UNUSED(tagId)
    return false;
}

int TagDAO::addSongsToTag(const QList<int>& songIds, int tagId)
{
    // TODO: 实现批量添加歌曲到标签逻辑
    Q_UNUSED(songIds)
    Q_UNUSED(tagId)
    return 0;
}

int TagDAO::removeSongsFromTag(const QList<int>& songIds, int tagId)
{
    // TODO: 实现批量从标签中移除歌曲逻辑
    Q_UNUSED(songIds)
    Q_UNUSED(tagId)
    return 0;
}

bool TagDAO::isSongInTag(int songId, int tagId)
{
    // TODO: 实现检查歌曲是否在标签中逻辑
    Q_UNUSED(songId)
    Q_UNUSED(tagId)
    return false;
}

QList<int> TagDAO::getSongIdsInTag(int tagId)
{
    // TODO: 实现获取标签中所有歌曲ID逻辑
    Q_UNUSED(tagId)
    return QList<int>();
}

bool TagDAO::updateTagSongCount(int tagId)
{
    // TODO: 实现更新标签歌曲数量逻辑
    Q_UNUSED(tagId)
    return false;
}

int TagDAO::cleanupOrphanedAssociations()
{
    // TODO: 实现清理孤立关联逻辑
    return 0;
}

QList<Tag> TagDAO::getTagsSortedByName(bool ascending)
{
    // TODO: 实现按名称排序获取标签逻辑
    Q_UNUSED(ascending)
    return QList<Tag>();
}

QList<Tag> TagDAO::getTagsSortedBySongCount(bool ascending)
{
    // TODO: 实现按歌曲数量排序获取标签逻辑
    Q_UNUSED(ascending)
    return QList<Tag>();
}

QList<Tag> TagDAO::getTagsSortedByCreateTime(bool ascending)
{
    // TODO: 实现按创建时间排序获取标签逻辑
    Q_UNUSED(ascending)
    return QList<Tag>();
}

Tag TagDAO::createTagFromQuery(const QSqlQuery& query)
{
    Tag tag;
    tag.setId(query.value("id").toInt());
    tag.setName(query.value("name").toString());
    tag.setDescription(query.value("description").toString());
    tag.setCoverPath(query.value("cover_path").toString());
    tag.setColor(query.value("color").toString());
    tag.setTagType(static_cast<Tag::TagType>(query.value("tag_type").toInt()));
    tag.setIsSystem(query.value("is_system").toBool());
    tag.setIsDeletable(query.value("is_deletable").toBool());
    tag.setSortOrder(query.value("sort_order").toInt());
    tag.setSongCount(query.value("song_count").toInt());
    tag.setCreatedAt(QDateTime::fromSecsSinceEpoch(query.value("created_at").toLongLong()));
    tag.setUpdatedAt(QDateTime::fromSecsSinceEpoch(query.value("updated_at").toLongLong()));
    return tag;
}

QString TagDAO::buildSearchCondition(const QString& keyword, const QStringList& searchFields)
{
    // TODO: 实现构建搜索条件逻辑
    Q_UNUSED(keyword)
    Q_UNUSED(searchFields)
    return QString();
}

bool TagDAO::prepareQuery(QSqlQuery& query, const QString& sql)
{
    // TODO: 实现准备查询语句逻辑
    Q_UNUSED(query)
    Q_UNUSED(sql)
    return false;
}

void TagDAO::logError(const QString& error)
{
    qWarning() << "TagDAO错误:" << error;
    emit databaseError(error);
}

void TagDAO::logSqlError(const QSqlQuery& query, const QString& operation)
{
    QString errorMsg = QString("SQL错误 - 操作: %1, 错误: %2").arg(operation, query.lastError().text());
    logError(errorMsg);
}

// SQL语句常量定义
const QString TagDAO::SQLStatements::INSERT_TAG = "";
const QString TagDAO::SQLStatements::UPDATE_TAG = "";
const QString TagDAO::SQLStatements::DELETE_TAG = "";
const QString TagDAO::SQLStatements::SELECT_TAG_BY_ID = "";
const QString TagDAO::SQLStatements::SELECT_TAG_BY_NAME = "";
const QString TagDAO::SQLStatements::SELECT_ALL_TAGS = "";
const QString TagDAO::SQLStatements::SELECT_SYSTEM_TAGS = "";
const QString TagDAO::SQLStatements::SELECT_USER_TAGS = "";
const QString TagDAO::SQLStatements::SELECT_TAGS_BY_SONG = "";
const QString TagDAO::SQLStatements::SEARCH_TAGS = "";
const QString TagDAO::SQLStatements::COUNT_TAGS = "";
const QString TagDAO::SQLStatements::COUNT_SONGS_IN_TAG = "";
const QString TagDAO::SQLStatements::TAG_EXISTS = "";
const QString TagDAO::SQLStatements::NAME_EXISTS = "";
const QString TagDAO::SQLStatements::INSERT_SONG_TAG = "";
const QString TagDAO::SQLStatements::DELETE_SONG_TAG = "";
const QString TagDAO::SQLStatements::SONG_IN_TAG = "";
const QString TagDAO::SQLStatements::SELECT_SONG_IDS_IN_TAG = "";
const QString TagDAO::SQLStatements::UPDATE_TAG_SONG_COUNT = "";
const QString TagDAO::SQLStatements::CLEANUP_ORPHANED_ASSOCIATIONS = "";
const QString TagDAO::SQLStatements::SELECT_TAGS_SORTED_BY_NAME = "";
const QString TagDAO::SQLStatements::SELECT_TAGS_SORTED_BY_SONG_COUNT = "";
const QString TagDAO::SQLStatements::SELECT_TAGS_SORTED_BY_CREATE_TIME = "";