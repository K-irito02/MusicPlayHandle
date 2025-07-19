#include "playhistory.h"
#include <QDebug>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariantMap>

PlayHistory::PlayHistory()
    : m_id(0)
    , m_songId(0)
    , m_playedAt(QDateTime::currentDateTime())
    , m_playDuration(0)
    , m_playPosition(0)
    , m_isCompleted(false)
{
}

PlayHistory::PlayHistory(int songId, const QDateTime& playedAt, qint64 playDuration, 
                         qint64 playPosition, bool isCompleted)
    : m_id(0)
    , m_songId(songId)
    , m_playedAt(playedAt)
    , m_playDuration(playDuration)
    , m_playPosition(playPosition)
    , m_isCompleted(isCompleted)
{
}

PlayHistory::PlayHistory(const PlayHistory& other)
    : m_id(other.m_id)
    , m_songId(other.m_songId)
    , m_playedAt(other.m_playedAt)
    , m_playDuration(other.m_playDuration)
    , m_playPosition(other.m_playPosition)
    , m_isCompleted(other.m_isCompleted)
{
}

PlayHistory& PlayHistory::operator=(const PlayHistory& other)
{
    if (this != &other) {
        m_id = other.m_id;
        m_songId = other.m_songId;
        m_playedAt = other.m_playedAt;
        m_playDuration = other.m_playDuration;
        m_playPosition = other.m_playPosition;
        m_isCompleted = other.m_isCompleted;
    }
    return *this;
}

bool PlayHistory::operator==(const PlayHistory& other) const
{
    return m_id == other.m_id &&
           m_songId == other.m_songId &&
           m_playedAt == other.m_playedAt &&
           m_playDuration == other.m_playDuration &&
           m_playPosition == other.m_playPosition &&
           m_isCompleted == other.m_isCompleted;
}

bool PlayHistory::operator!=(const PlayHistory& other) const
{
    return !(*this == other);
}

bool PlayHistory::isValid() const
{
    return m_songId > 0 && m_playedAt.isValid();
}

QString PlayHistory::formattedPlayedAt() const
{
    if (!m_playedAt.isValid()) {
        return "未知时间";
    }
    
    // 格式："年/月-日/时-分-秒"（如：2023/12-25/15-30-45）
    return m_playedAt.toString("yyyy/MM-dd/hh-mm-ss");
}

QString PlayHistory::formattedPlayDuration() const
{
    return formatTime(m_playDuration);
}

QString PlayHistory::formattedPlayPosition() const
{
    return formatTime(m_playPosition);
}

int PlayHistory::completionPercentage(qint64 songDuration) const
{
    if (songDuration <= 0) {
        return 0;
    }
    
    int percentage = static_cast<int>((m_playPosition * 100) / songDuration);
    return qBound(0, percentage, 100);
}

bool PlayHistory::isValidPlay(qint64 thresholdMs) const
{
    return m_playDuration >= thresholdMs;
}

PlayHistory PlayHistory::fromJson(const QJsonObject& json)
{
    PlayHistory history;
    history.setId(json["id"].toInt());
    history.setSongId(json["songId"].toInt());
    history.setPlayedAt(QDateTime::fromString(json["playedAt"].toString(), Qt::ISODate));
    history.setPlayDuration(json["playDuration"].toVariant().toLongLong());
    history.setPlayPosition(json["playPosition"].toVariant().toLongLong());
    history.setIsCompleted(json["isCompleted"].toBool());
    return history;
}

QJsonObject PlayHistory::toJson() const
{
    QJsonObject json;
    json["id"] = m_id;
    json["songId"] = m_songId;
    json["playedAt"] = m_playedAt.toString(Qt::ISODate);
    json["playDuration"] = QJsonValue::fromVariant(m_playDuration);
    json["playPosition"] = QJsonValue::fromVariant(m_playPosition);
    json["isCompleted"] = m_isCompleted;
    return json;
}

PlayHistory PlayHistory::fromVariantMap(const QVariantMap& map)
{
    PlayHistory history;
    history.setId(map["id"].toInt());
    history.setSongId(map["songId"].toInt());
    history.setPlayedAt(map["playedAt"].toDateTime());
    history.setPlayDuration(map["playDuration"].toLongLong());
    history.setPlayPosition(map["playPosition"].toLongLong());
    history.setIsCompleted(map["isCompleted"].toBool());
    return history;
}

QVariantMap PlayHistory::toVariantMap() const
{
    QVariantMap map;
    map["id"] = m_id;
    map["songId"] = m_songId;
    map["playedAt"] = m_playedAt;
    map["playDuration"] = m_playDuration;
    map["playPosition"] = m_playPosition;
    map["isCompleted"] = m_isCompleted;
    return map;
}

void PlayHistory::clear()
{
    m_id = 0;
    m_songId = 0;
    m_playedAt = QDateTime();
    m_playDuration = 0;
    m_playPosition = 0;
    m_isCompleted = false;
}

bool PlayHistory::isEmpty() const
{
    return m_id == 0 && m_songId == 0;
}

QString PlayHistory::toString() const
{
    return QString("PlayHistory(id=%1, songId=%2, playedAt=%3, duration=%4, position=%5, completed=%6)")
           .arg(m_id)
           .arg(m_songId)
           .arg(formattedPlayedAt())
           .arg(formattedPlayDuration())
           .arg(formattedPlayPosition())
           .arg(m_isCompleted ? "true" : "false");
}

QString PlayHistory::formatTime(qint64 milliseconds) const
{
    if (milliseconds <= 0) {
        return "00:00";
    }
    
    int totalSeconds = milliseconds / 1000;
    int minutes = totalSeconds / 60;
    int seconds = totalSeconds % 60;
    
    return QString("%1:%2")
           .arg(minutes)
           .arg(seconds, 2, 10, QChar('0'));
} 