#include "playlist.h"
#include <QJsonDocument>
#include <QRegularExpression>

Playlist::Playlist()
{
    initializeDefaults();
}

Playlist::Playlist(const QString& name, const QString& description)
{
    initializeDefaults();
    setName(name);
    setDescription(description);
}

Playlist::Playlist(const Playlist& other)
{
    *this = other;
}

Playlist& Playlist::operator=(const Playlist& other)
{
    if (this != &other) {
        m_id = other.m_id;
        m_name = other.m_name;
        m_description = other.m_description;
        m_createdAt = other.m_createdAt;
        m_modifiedAt = other.m_modifiedAt;
        m_lastPlayedAt = other.m_lastPlayedAt;
        m_songCount = other.m_songCount;
        m_totalDuration = other.m_totalDuration;
        m_playCount = other.m_playCount;
        m_color = other.m_color;
        m_iconPath = other.m_iconPath;
        m_isSmartPlaylist = other.m_isSmartPlaylist;
        m_smartCriteria = other.m_smartCriteria;
        m_isSystemPlaylist = other.m_isSystemPlaylist;
        m_isFavorite = other.m_isFavorite;
        m_sortOrder = other.m_sortOrder;
    }
    return *this;
}

Playlist::~Playlist()
{
    // 默认析构函数
}

void Playlist::initializeDefaults()
{
    m_id = -1;
    m_name = QString();
    m_description = QString();
    m_createdAt = QDateTime::currentDateTime();
    m_modifiedAt = QDateTime::currentDateTime();
    m_lastPlayedAt = QDateTime();
    m_songCount = 0;
    m_totalDuration = 0;
    m_playCount = 0;
    m_color = QColor(100, 150, 255); // 默认蓝色
    m_iconPath = QString();
    m_isSmartPlaylist = false;
    m_smartCriteria = QString();
    m_isSystemPlaylist = false;
    m_isFavorite = false;
    m_sortOrder = 0;
}

// 基本属性
int Playlist::id() const
{
    return m_id;
}

void Playlist::setId(int id)
{
    m_id = id;
}

QString Playlist::name() const
{
    return m_name;
}

void Playlist::setName(const QString& name)
{
    if (isValidName(name)) {
        m_name = name;
        updateModifiedTime();
    }
}

QString Playlist::description() const
{
    return m_description;
}

void Playlist::setDescription(const QString& description)
{
    m_description = description;
    updateModifiedTime();
}

// 时间戳
QDateTime Playlist::createdAt() const
{
    return m_createdAt;
}

void Playlist::setCreatedAt(const QDateTime& createdAt)
{
    m_createdAt = createdAt;
}

QDateTime Playlist::modifiedAt() const
{
    return m_modifiedAt;
}

void Playlist::setModifiedAt(const QDateTime& modifiedAt)
{
    m_modifiedAt = modifiedAt;
}

QDateTime Playlist::lastPlayedAt() const
{
    return m_lastPlayedAt;
}

void Playlist::setLastPlayedAt(const QDateTime& lastPlayedAt)
{
    m_lastPlayedAt = lastPlayedAt;
}

// 统计信息
int Playlist::songCount() const
{
    return m_songCount;
}

void Playlist::setSongCount(int count)
{
    m_songCount = qMax(0, count);
}

qint64 Playlist::totalDuration() const
{
    return m_totalDuration;
}

void Playlist::setTotalDuration(qint64 duration)
{
    m_totalDuration = qMax(0LL, duration);
}

int Playlist::playCount() const
{
    return m_playCount;
}

void Playlist::setPlayCount(int count)
{
    m_playCount = qMax(0, count);
}

void Playlist::incrementPlayCount()
{
    m_playCount++;
    updateLastPlayedTime();
}

// 外观设置
QColor Playlist::color() const
{
    return m_color;
}

void Playlist::setColor(const QColor& color)
{
    if (color.isValid()) {
        m_color = color;
        updateModifiedTime();
    }
}

QString Playlist::iconPath() const
{
    return m_iconPath;
}

void Playlist::setIconPath(const QString& iconPath)
{
    m_iconPath = iconPath;
    updateModifiedTime();
}

// 智能播放列表
bool Playlist::isSmartPlaylist() const
{
    return m_isSmartPlaylist;
}

void Playlist::setIsSmartPlaylist(bool isSmart)
{
    m_isSmartPlaylist = isSmart;
    updateModifiedTime();
}

QString Playlist::smartCriteria() const
{
    return m_smartCriteria;
}

void Playlist::setSmartCriteria(const QString& criteria)
{
    m_smartCriteria = criteria;
    updateModifiedTime();
}

// 系统属性
bool Playlist::isSystemPlaylist() const
{
    return m_isSystemPlaylist;
}

void Playlist::setIsSystemPlaylist(bool isSystem)
{
    m_isSystemPlaylist = isSystem;
}

bool Playlist::isFavorite() const
{
    return m_isFavorite;
}

void Playlist::setIsFavorite(bool favorite)
{
    m_isFavorite = favorite;
}

// 排序和布局
int Playlist::sortOrder() const
{
    return m_sortOrder;
}

void Playlist::setSortOrder(int order)
{
    m_sortOrder = order;
}

// 验证
bool Playlist::isValid() const
{
    return isValidId(m_id) && isValidName(m_name);
}

// 比较操作
bool Playlist::operator==(const Playlist& other) const
{
    return m_id == other.m_id;
}

bool Playlist::operator!=(const Playlist& other) const
{
    return !(*this == other);
}

bool Playlist::operator<(const Playlist& other) const
{
    // 按名称排序
    return m_name < other.m_name;
}

// JSON序列化
QJsonObject Playlist::toJson() const
{
    QJsonObject json;
    json["id"] = m_id;
    json["name"] = m_name;
    json["description"] = m_description;
    json["createdAt"] = m_createdAt.toString(Qt::ISODate);
    json["modifiedAt"] = m_modifiedAt.toString(Qt::ISODate);
    json["lastPlayedAt"] = m_lastPlayedAt.toString(Qt::ISODate);
    json["songCount"] = m_songCount;
    json["totalDuration"] = static_cast<double>(m_totalDuration);
    json["playCount"] = m_playCount;
    json["color"] = m_color.name();
    json["iconPath"] = m_iconPath;
    json["isSmartPlaylist"] = m_isSmartPlaylist;
    json["smartCriteria"] = m_smartCriteria;
    json["isSystemPlaylist"] = m_isSystemPlaylist;
    json["isFavorite"] = m_isFavorite;
    json["sortOrder"] = m_sortOrder;
    return json;
}

Playlist Playlist::fromJson(const QJsonObject& json)
{
    Playlist playlist;
    playlist.setId(json["id"].toInt());
    playlist.setName(json["name"].toString());
    playlist.setDescription(json["description"].toString());
    playlist.setCreatedAt(QDateTime::fromString(json["createdAt"].toString(), Qt::ISODate));
    playlist.setModifiedAt(QDateTime::fromString(json["modifiedAt"].toString(), Qt::ISODate));
    playlist.setLastPlayedAt(QDateTime::fromString(json["lastPlayedAt"].toString(), Qt::ISODate));
    playlist.setSongCount(json["songCount"].toInt());
    playlist.setTotalDuration(static_cast<qint64>(json["totalDuration"].toDouble()));
    playlist.setPlayCount(json["playCount"].toInt());
    playlist.setColor(QColor(json["color"].toString()));
    playlist.setIconPath(json["iconPath"].toString());
    playlist.setIsSmartPlaylist(json["isSmartPlaylist"].toBool());
    playlist.setSmartCriteria(json["smartCriteria"].toString());
    playlist.setIsSystemPlaylist(json["isSystemPlaylist"].toBool());
    playlist.setIsFavorite(json["isFavorite"].toBool());
    playlist.setSortOrder(json["sortOrder"].toInt());
    return playlist;
}

// 调试输出
QString Playlist::toString() const
{
    return QString("Playlist(id=%1, name='%2', songCount=%3, duration=%4)")
        .arg(m_id)
        .arg(m_name)
        .arg(m_songCount)
        .arg(formatDuration());
}

// 清空数据
void Playlist::clear()
{
    initializeDefaults();
}

// 克隆
Playlist Playlist::clone() const
{
    Playlist cloned = *this;
    cloned.setId(-1); // 克隆时重置ID
    return cloned;
}

// 属性更新
void Playlist::updateModifiedTime()
{
    m_modifiedAt = QDateTime::currentDateTime();
}

void Playlist::updateLastPlayedTime()
{
    m_lastPlayedAt = QDateTime::currentDateTime();
}

// 格式化方法
QString Playlist::formatDuration() const
{
    qint64 seconds = m_totalDuration / 1000;
    int hours = seconds / 3600;
    int minutes = (seconds % 3600) / 60;
    int secs = seconds % 60;
    
    if (hours > 0) {
        return QString("%1:%2:%3")
            .arg(hours, 2, 10, QChar('0'))
            .arg(minutes, 2, 10, QChar('0'))
            .arg(secs, 2, 10, QChar('0'));
    } else {
        return QString("%1:%2")
            .arg(minutes, 2, 10, QChar('0'))
            .arg(secs, 2, 10, QChar('0'));
    }
}

QString Playlist::formatCreatedTime() const
{
    return m_createdAt.toString("yyyy-MM-dd hh:mm:ss");
}

QString Playlist::formatModifiedTime() const
{
    return m_modifiedAt.toString("yyyy-MM-dd hh:mm:ss");
}

QString Playlist::formatLastPlayedTime() const
{
    if (m_lastPlayedAt.isValid()) {
        return m_lastPlayedAt.toString("yyyy-MM-dd hh:mm:ss");
    }
    return QObject::tr("从未播放");
}

// 验证方法
bool Playlist::isValidName(const QString& name) const
{
    if (name.isEmpty() || name.length() > 255) {
        return false;
    }
    
    // 检查是否包含非法字符
    QRegularExpression invalidChars("[<>:\"/\\\\|?*]");
    return !invalidChars.match(name).hasMatch();
}

bool Playlist::isValidId(int id) const
{
    return id >= 0;
} 