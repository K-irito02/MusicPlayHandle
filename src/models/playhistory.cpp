#include "playhistory.h"
#include <QDebug>

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

// 其他方法的实现将在后续开发中完成
// TODO: 实现完整的PlayHistory类方法

bool PlayHistory::isValid() const
{
    return m_songId > 0 && m_playedAt.isValid();
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