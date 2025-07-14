#include "tag.h"
#include <QDebug>
#include <QJsonObject>
#include <QVariantMap>

// 系统标签常量定义
const QString Tag::DEFAULT_TAG_NAME = "默认标签";
const QString Tag::MY_MUSIC_TAG_NAME = "我的歌曲";
const QString Tag::FAVORITE_TAG_NAME = "收藏";

Tag::Tag()
    : m_id(0)
    , m_color("#3498db")
    , m_tagType(UserTag)
    , m_isSystem(false)
    , m_isDeletable(true)
    , m_sortOrder(0)
    , m_songCount(0)
    , m_createdAt(QDateTime::currentDateTime())
    , m_updatedAt(QDateTime::currentDateTime())
{
}

Tag::Tag(const QString& name, const QString& description, TagType type)
    : m_id(0)
    , m_name(name)
    , m_description(description)
    , m_color("#3498db")
    , m_tagType(type)
    , m_isSystem(type == SystemTag)
    , m_isDeletable(type != SystemTag)
    , m_sortOrder(0)
    , m_songCount(0)
    , m_createdAt(QDateTime::currentDateTime())
    , m_updatedAt(QDateTime::currentDateTime())
{
}

// 其他方法的实现将在后续开发中完成
// TODO: 实现完整的Tag类方法

bool Tag::isValid() const
{
    return !m_name.isEmpty();
}

QString Tag::displayName() const
{
    return m_name;
}

void Tag::updateModifiedTime()
{
    m_updatedAt = QDateTime::currentDateTime();
}

// 拷贝构造函数
Tag::Tag(const Tag& other)
    : m_id(other.m_id)
    , m_name(other.m_name)
    , m_description(other.m_description)
    , m_coverPath(other.m_coverPath)
    , m_color(other.m_color)
    , m_tagType(other.m_tagType)
    , m_isSystem(other.m_isSystem)
    , m_isDeletable(other.m_isDeletable)
    , m_sortOrder(other.m_sortOrder)
    , m_songCount(other.m_songCount)
    , m_createdAt(other.m_createdAt)
    , m_updatedAt(other.m_updatedAt)
{
}

// 赋值操作符
Tag& Tag::operator=(const Tag& other)
{
    if (this != &other) {
        m_id = other.m_id;
        m_name = other.m_name;
        m_description = other.m_description;
        m_coverPath = other.m_coverPath;
        m_color = other.m_color;
        m_tagType = other.m_tagType;
        m_isSystem = other.m_isSystem;
        m_isDeletable = other.m_isDeletable;
        m_sortOrder = other.m_sortOrder;
        m_songCount = other.m_songCount;
        m_createdAt = other.m_createdAt;
        m_updatedAt = other.m_updatedAt;
    }
    return *this;
}

// 相等操作符
bool Tag::operator==(const Tag& other) const
{
    return m_id == other.m_id &&
           m_name == other.m_name &&
           m_description == other.m_description &&
           m_coverPath == other.m_coverPath &&
           m_color == other.m_color &&
           m_tagType == other.m_tagType &&
           m_isSystem == other.m_isSystem &&
           m_isDeletable == other.m_isDeletable &&
           m_sortOrder == other.m_sortOrder &&
           m_songCount == other.m_songCount &&
           m_createdAt == other.m_createdAt &&
           m_updatedAt == other.m_updatedAt;
}

// 不等操作符
bool Tag::operator!=(const Tag& other) const
{
    return !(*this == other);
}

QString Tag::tagTypeString() const
{
    return m_tagType == SystemTag ? "系统标签" : "用户标签";
}

void Tag::addSongCount(int count)
{
    m_songCount += count;
    updateModifiedTime();
}

void Tag::removeSongCount(int count)
{
    m_songCount = qMax(0, m_songCount - count);
    updateModifiedTime();
}

void Tag::resetSongCount(int count)
{
    m_songCount = count;
    updateModifiedTime();
}

bool Tag::canDelete() const
{
    return m_isDeletable && !m_isSystem;
}

bool Tag::canEdit() const
{
    return !m_isSystem || m_isDeletable;
}

Tag Tag::createSystemTag(const QString& name, const QString& description, int sortOrder, bool isDeletable)
{
    Tag tag(name, description, SystemTag);
    tag.setIsSystem(true);
    tag.setIsDeletable(isDeletable);
    tag.setSortOrder(sortOrder);
    return tag;
}

Tag Tag::createUserTag(const QString& name, const QString& description, const QString& color)
{
    Tag tag(name, description, UserTag);
    tag.setColor(color);
    tag.setIsSystem(false);
    tag.setIsDeletable(true);
    return tag;
}

QJsonObject Tag::toJson() const
{
    QJsonObject json;
    json["id"] = m_id;
    json["name"] = m_name;
    json["description"] = m_description;
    json["coverPath"] = m_coverPath;
    json["color"] = m_color;
    json["tagType"] = static_cast<int>(m_tagType);
    json["isSystem"] = m_isSystem;
    json["isDeletable"] = m_isDeletable;
    json["sortOrder"] = m_sortOrder;
    json["songCount"] = m_songCount;
    json["createdAt"] = m_createdAt.toString(Qt::ISODate);
    json["updatedAt"] = m_updatedAt.toString(Qt::ISODate);
    return json;
}

Tag Tag::fromJson(const QJsonObject& json)
{
    Tag tag;
    tag.setId(json["id"].toInt());
    tag.setName(json["name"].toString());
    tag.setDescription(json["description"].toString());
    tag.setCoverPath(json["coverPath"].toString());
    tag.setColor(json["color"].toString());
    tag.setTagType(static_cast<TagType>(json["tagType"].toInt()));
    tag.setIsSystem(json["isSystem"].toBool());
    tag.setIsDeletable(json["isDeletable"].toBool());
    tag.setSortOrder(json["sortOrder"].toInt());
    tag.setSongCount(json["songCount"].toInt());
    tag.setCreatedAt(QDateTime::fromString(json["createdAt"].toString(), Qt::ISODate));
    tag.setUpdatedAt(QDateTime::fromString(json["updatedAt"].toString(), Qt::ISODate));
    return tag;
}

QVariantMap Tag::toVariantMap() const
{
    QVariantMap map;
    map["id"] = m_id;
    map["name"] = m_name;
    map["description"] = m_description;
    map["coverPath"] = m_coverPath;
    map["color"] = m_color;
    map["tagType"] = static_cast<int>(m_tagType);
    map["isSystem"] = m_isSystem;
    map["isDeletable"] = m_isDeletable;
    map["sortOrder"] = m_sortOrder;
    map["songCount"] = m_songCount;
    map["createdAt"] = m_createdAt;
    map["updatedAt"] = m_updatedAt;
    return map;
}

Tag Tag::fromVariantMap(const QVariantMap& map)
{
    Tag tag;
    tag.setId(map["id"].toInt());
    tag.setName(map["name"].toString());
    tag.setDescription(map["description"].toString());
    tag.setCoverPath(map["coverPath"].toString());
    tag.setColor(map["color"].toString());
    tag.setTagType(static_cast<TagType>(map["tagType"].toInt()));
    tag.setIsSystem(map["isSystem"].toBool());
    tag.setIsDeletable(map["isDeletable"].toBool());
    tag.setSortOrder(map["sortOrder"].toInt());
    tag.setSongCount(map["songCount"].toInt());
    tag.setCreatedAt(map["createdAt"].toDateTime());
    tag.setUpdatedAt(map["updatedAt"].toDateTime());
    return tag;
}

void Tag::clear()
{
    m_id = 0;
    m_name.clear();
    m_description.clear();
    m_coverPath.clear();
    m_color = "#3498db";
    m_tagType = UserTag;
    m_isSystem = false;
    m_isDeletable = true;
    m_sortOrder = 0;
    m_songCount = 0;
    m_createdAt = QDateTime::currentDateTime();
    m_updatedAt = QDateTime::currentDateTime();
}

bool Tag::isEmpty() const
{
    return m_name.isEmpty();
}

QString Tag::toString() const
{
    return QString("Tag(id=%1, name=%2, type=%3, system=%4, deletable=%5, songs=%6)")
           .arg(m_id)
           .arg(m_name)
           .arg(tagTypeString())
           .arg(m_isSystem ? "true" : "false")
           .arg(m_isDeletable ? "true" : "false")
           .arg(m_songCount);
}