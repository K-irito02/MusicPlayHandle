#include "tagdao.h"
#include "databasemanager.h"
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariant>
#include <QDebug>

TagDao::TagDao(QObject* parent)
    : BaseDao(parent)
{
}

int TagDao::addTag(const Tag& tag)
{
    const QString sql = R"(
        INSERT INTO tags (name, color, description, is_system)
        VALUES (?, ?, ?, ?)
    )";
    
    QSqlQuery query = prepareQuery(sql);
    query.addBindValue(tag.name());
    query.addBindValue(tag.color());
    query.addBindValue(tag.description());
    query.addBindValue(tag.isSystem() ? 1 : 0);
    
    if (query.exec()) {
        return query.lastInsertId().toInt();
    } else {
        logError("addTag", query.lastError().text());
        return -1;
    }
}

Tag TagDao::getTagById(int id)
{
    const QString sql = "SELECT * FROM tags WHERE id = ?";
    QSqlQuery query = prepareQuery(sql);
    query.addBindValue(id);
    
    if (query.exec() && query.next()) {
        return createTagFromQuery(query);
    }
    
    return Tag(); // 返回空对象
}

Tag TagDao::getTagByName(const QString& name)
{
    const QString sql = "SELECT * FROM tags WHERE name = ?";
    QSqlQuery query = prepareQuery(sql);
    query.addBindValue(name);
    
    if (query.exec() && query.next()) {
        return createTagFromQuery(query);
    }
    
    return Tag(); // 返回空对象
}

QList<Tag> TagDao::getAllTags()
{
    QList<Tag> tags;
    
    qDebug() << "TagDao::getAllTags - 开始查询所有标签";
    const QString sql = "SELECT * FROM tags ORDER BY is_system DESC, name";
    QSqlQuery query = executeQuery(sql);
    
    // 检查查询是否有效
    if (!query.isValid() && !query.isActive()) {
        logError("getAllTags", "查询执行失败，无法获取标签列表");
        qDebug() << "TagDao::getAllTags - 查询失败，返回空列表";
        return tags;
    }
    
    qDebug() << "TagDao::getAllTags - 查询成功，开始处理结果";
     
    while (query.next()) {
        tags.append(createTagFromQuery(query));
    }
    
    qDebug() << QString("TagDao::getAllTags - 查询完成，共获取 %1 个标签").arg(tags.size());
    return tags;
}

QList<Tag> TagDao::getSystemTags()
{
    QList<Tag> tags;
    const QString sql = "SELECT * FROM tags WHERE is_system = 1 ORDER BY name";
    QSqlQuery query = executeQuery(sql);
    
    // 检查查询是否有效
    if (!query.isValid() && !query.isActive()) {
        logError("getSystemTags", "查询执行失败，无法获取系统标签列表");
        return tags;
    }
    
    while (query.next()) {
        tags.append(createTagFromQuery(query));
    }
    
    return tags;
}

QList<Tag> TagDao::getUserTags()
{
    QList<Tag> tags;
    const QString sql = "SELECT * FROM tags WHERE is_system = 0 ORDER BY name";
    QSqlQuery query = executeQuery(sql);
    
    // 检查查询是否有效
    if (!query.isValid() && !query.isActive()) {
        logError("getUserTags", "查询执行失败，无法获取用户标签列表");
        return tags;
    }
    
    while (query.next()) {
        tags.append(createTagFromQuery(query));
    }
    
    return tags;
}

QList<Tag> TagDao::searchTags(const QString& keyword)
{
    QList<Tag> tags;
    const QString sql = R"(
        SELECT * FROM tags 
        WHERE name LIKE ? OR description LIKE ? 
        ORDER BY is_system DESC, name
    )";
    
    QSqlQuery query = prepareQuery(sql);
    QString searchPattern = "%" + keyword + "%";
    query.addBindValue(searchPattern);
    query.addBindValue(searchPattern);
    
    if (query.exec()) {
        while (query.next()) {
            tags.append(createTagFromQuery(query));
        }
    } else {
        logError("searchTags", query.lastError().text());
    }
    
    // 额外检查：如果查询对象无效，记录错误
    if (!query.isValid() && !query.isActive() && tags.isEmpty()) {
        logError("searchTags", "查询执行失败，无法搜索标签");
    }
    
    return tags;
}

bool TagDao::updateTag(const Tag& tag)
{
    const QString sql = R"(
        UPDATE tags SET 
            name = ?, color = ?, description = ?, updated_at = CURRENT_TIMESTAMP
        WHERE id = ? AND is_system = 0
    )";
    
    QSqlQuery query = prepareQuery(sql);
    query.addBindValue(tag.name());
    query.addBindValue(tag.color());
    query.addBindValue(tag.description());
    query.addBindValue(tag.id());
    
    if (query.exec()) {
        if (query.numRowsAffected() == 0) {
            logError("updateTag", "无法更新系统标签或标签不存在");
            return false;
        }
        return true;
    } else {
        logError("updateTag", query.lastError().text());
        return false;
    }
}

bool TagDao::deleteTag(int id)
{
    // 检查是否为系统标签
    if (isSystemTag(id)) {
        logError("deleteTag", "不能删除系统标签");
        return false;
    }
    
    const QString sql = "DELETE FROM tags WHERE id = ? AND is_system = 0";
    QSqlQuery query = prepareQuery(sql);
    query.addBindValue(id);
    
    if (query.exec()) {
        return query.numRowsAffected() > 0;
    } else {
        logError("deleteTag", query.lastError().text());
        return false;
    }
}

bool TagDao::tagExists(const QString& name)
{
    const QString sql = "SELECT COUNT(*) FROM tags WHERE name = ?";
    QSqlQuery query = prepareQuery(sql);
    query.addBindValue(name);
    
    if (query.exec() && query.next()) {
        return query.value(0).toInt() > 0;
    }
    
    return false;
}

bool TagDao::isSystemTag(int id)
{
    const QString sql = "SELECT is_system FROM tags WHERE id = ?";
    QSqlQuery query = prepareQuery(sql);
    query.addBindValue(id);
    
    if (query.exec() && query.next()) {
        return query.value(0).toInt() == 1;
    }
    
    return false;
}

int TagDao::getTagCount()
{
    const QString sql = "SELECT COUNT(*) FROM tags";
    QSqlQuery query = executeQuery(sql);
    
    if (query.next()) {
        return query.value(0).toInt();
    }
    
    return 0;
}

int TagDao::getUserTagCount()
{
    const QString sql = "SELECT COUNT(*) FROM tags WHERE is_system = 0";
    QSqlQuery query = executeQuery(sql);
    
    if (query.next()) {
        return query.value(0).toInt();
    }
    
    return 0;
}

Tag TagDao::createTagFromQuery(const QSqlQuery& query)
{
    Tag tag;
    tag.setId(query.value("id").toInt());
    tag.setName(query.value("name").toString());
    tag.setColor(query.value("color").toString());
    tag.setDescription(query.value("description").toString());
    tag.setIsSystem(query.value("is_system").toInt() == 1);
    tag.setCreatedAt(query.value("created_at").toDateTime());
    tag.setUpdatedAt(query.value("updated_at").toDateTime());
    
    return tag;
}