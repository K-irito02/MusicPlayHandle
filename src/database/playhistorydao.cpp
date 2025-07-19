#include "playhistorydao.h"
#include "../core/logger.h"
#include <QDebug>
#include <QSqlError>
#include <QDateTime>

PlayHistoryDao::PlayHistoryDao(QObject* parent)
    : BaseDao(parent)
{
}

PlayHistoryDao::~PlayHistoryDao()
{
}

bool PlayHistoryDao::addPlayRecord(int songId, const QDateTime& playedAt)
{
    QMutexLocker locker(&m_mutex);
    
    if (songId <= 0) {
        logError("addPlayRecord", "无效的歌曲ID: " + QString::number(songId));
        return false;
    }
    
    // 先清理该歌曲的重复记录，保留最新的
    cleanupDuplicateRecords(songId);
    
    const QString sql = R"(
        INSERT INTO play_history (song_id, played_at)
        VALUES (?, ?)
    )";
    
    QSqlQuery query = prepareQuery(sql);
    query.addBindValue(songId);
    query.addBindValue(playedAt);
    
    if (query.exec()) {
        // 限制总记录数，避免数据库过大
        limitPlayHistoryRecords(1000);
        logInfo("addPlayRecord", QString("成功添加播放记录: songId=%1, time=%2")
                .arg(songId).arg(playedAt.toString("yyyy/MM-dd/hh-mm-ss")));
        return true;
    } else {
        logError("addPlayRecord", query.lastError().text());
        return false;
    }
}

QList<Song> PlayHistoryDao::getRecentPlayedSongs(int limit)
{
    QMutexLocker locker(&m_mutex);
    
    // 修改SQL查询，确保每首歌只显示最新的播放记录
    const QString sql = R"(
        SELECT s.id, s.title, s.artist, s.album, s.file_path, s.duration,
               s.file_size, s.date_added, s.last_played, s.play_count, s.rating,
               s.tags, s.created_at, s.updated_at,
               ph.played_at
        FROM songs s
        INNER JOIN (
            SELECT song_id, MAX(played_at) as played_at
            FROM play_history
            GROUP BY song_id
        ) ph ON s.id = ph.song_id
        ORDER BY ph.played_at DESC
        LIMIT ?
    )";
    
    QSqlQuery query = prepareQuery(sql);
    query.addBindValue(limit);
    
    QList<Song> songs;
    if (query.exec()) {
        while (query.next()) {
            Song song = createSongFromQuery(query);
            songs.append(song);
        }
        logInfo("getRecentPlayedSongs", QString("获取到 %1 首最近播放歌曲").arg(songs.size()));
    } else {
        logError("getRecentPlayedSongs", query.lastError().text());
    }
    
    return songs;
}

QList<PlayHistory> PlayHistoryDao::getSongPlayHistory(int songId, int limit)
{
    QMutexLocker locker(&m_mutex);
    
    const QString sql = R"(
        SELECT id, song_id, played_at
        FROM play_history
        WHERE song_id = ?
        ORDER BY played_at DESC
        LIMIT ?
    )";
    
    QSqlQuery query = prepareQuery(sql);
    query.addBindValue(songId);
    query.addBindValue(limit);
    
    QList<PlayHistory> history;
    if (query.exec()) {
        while (query.next()) {
            PlayHistory record = createPlayHistoryFromQuery(query);
            history.append(record);
        }
    } else {
        logError("getSongPlayHistory", query.lastError().text());
    }
    
    return history;
}

QList<PlayHistory> PlayHistoryDao::getAllPlayHistory(int limit)
{
    QMutexLocker locker(&m_mutex);
    
    const QString sql = R"(
        SELECT id, song_id, played_at
        FROM play_history
        ORDER BY played_at DESC
        LIMIT ?
    )";
    
    QSqlQuery query = prepareQuery(sql);
    query.addBindValue(limit);
    
    QList<PlayHistory> history;
    if (query.exec()) {
        while (query.next()) {
            PlayHistory record = createPlayHistoryFromQuery(query);
            history.append(record);
        }
    } else {
        logError("getAllPlayHistory", query.lastError().text());
    }
    
    return history;
}

bool PlayHistoryDao::deleteSongPlayHistory(int songId)
{
    QMutexLocker locker(&m_mutex);
    
    const QString sql = "DELETE FROM play_history WHERE song_id = ?";
    QSqlQuery query = prepareQuery(sql);
    query.addBindValue(songId);
    
    if (query.exec()) {
        logInfo("deleteSongPlayHistory", QString("成功删除歌曲 %1 的播放历史").arg(songId));
        return true;
    } else {
        logError("deleteSongPlayHistory", query.lastError().text());
        return false;
    }
}

bool PlayHistoryDao::deletePlayHistoryBefore(const QDateTime& beforeTime)
{
    QMutexLocker locker(&m_mutex);
    
    const QString sql = "DELETE FROM play_history WHERE played_at < ?";
    QSqlQuery query = prepareQuery(sql);
    query.addBindValue(beforeTime);
    
    if (query.exec()) {
        int deletedCount = query.numRowsAffected();
        logInfo("deletePlayHistoryBefore", QString("成功删除 %1 条播放历史记录").arg(deletedCount));
        return true;
    } else {
        logError("deletePlayHistoryBefore", query.lastError().text());
        return false;
    }
}

bool PlayHistoryDao::clearAllPlayHistory()
{
    QMutexLocker locker(&m_mutex);
    
    const QString sql = "DELETE FROM play_history";
    QSqlQuery query = prepareQuery(sql);
    
    if (query.exec()) {
        int deletedCount = query.numRowsAffected();
        logInfo("clearAllPlayHistory", QString("成功清空 %1 条播放历史记录").arg(deletedCount));
        return true;
    } else {
        logError("clearAllPlayHistory", query.lastError().text());
        return false;
    }
}

PlayHistoryDao::PlayHistoryStats PlayHistoryDao::getPlayHistoryStats()
{
    QMutexLocker locker(&m_mutex);
    
    PlayHistoryStats stats = {};
    
    // 获取总记录数
    const QString countSql = "SELECT COUNT(*) FROM play_history";
    QSqlQuery countQuery = prepareQuery(countSql);
    if (countQuery.exec() && countQuery.next()) {
        stats.totalRecords = countQuery.value(0).toInt();
    }
    
    // 获取唯一歌曲数
    const QString uniqueSql = "SELECT COUNT(DISTINCT song_id) FROM play_history";
    QSqlQuery uniqueQuery = prepareQuery(uniqueSql);
    if (uniqueQuery.exec() && uniqueQuery.next()) {
        stats.uniqueSongs = uniqueQuery.value(0).toInt();
    }
    
    // 获取最早和最晚播放时间
    const QString timeSql = "SELECT MIN(played_at), MAX(played_at) FROM play_history";
    QSqlQuery timeQuery = prepareQuery(timeSql);
    if (timeQuery.exec() && timeQuery.next()) {
        stats.firstPlayTime = timeQuery.value(0).toDateTime();
        stats.lastPlayTime = timeQuery.value(1).toDateTime();
    }
    
    // 获取播放次数最多的歌曲
    const QString mostPlayedSql = R"(
        SELECT s.title, COUNT(*) as play_count
        FROM play_history ph
        INNER JOIN songs s ON ph.song_id = s.id
        GROUP BY ph.song_id
        ORDER BY play_count DESC
        LIMIT 1
    )";
    QSqlQuery mostPlayedQuery = prepareQuery(mostPlayedSql);
    if (mostPlayedQuery.exec() && mostPlayedQuery.next()) {
        stats.mostPlayedSong = mostPlayedQuery.value(0).toString();
        stats.mostPlayedCount = mostPlayedQuery.value(1).toInt();
    }
    
    return stats;
}

bool PlayHistoryDao::hasPlayHistory(int songId)
{
    QMutexLocker locker(&m_mutex);
    
    const QString sql = "SELECT COUNT(*) FROM play_history WHERE song_id = ?";
    QSqlQuery query = prepareQuery(sql);
    query.addBindValue(songId);
    
    if (query.exec() && query.next()) {
        return query.value(0).toInt() > 0;
    }
    
    return false;
}

int PlayHistoryDao::getSongPlayCount(int songId)
{
    QMutexLocker locker(&m_mutex);
    
    const QString sql = "SELECT COUNT(*) FROM play_history WHERE song_id = ?";
    QSqlQuery query = prepareQuery(sql);
    query.addBindValue(songId);
    
    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    
    return 0;
}

QDateTime PlayHistoryDao::getLastPlayTime(int songId)
{
    QMutexLocker locker(&m_mutex);
    
    const QString sql = "SELECT MAX(played_at) FROM play_history WHERE song_id = ?";
    QSqlQuery query = prepareQuery(sql);
    query.addBindValue(songId);
    
    if (query.exec() && query.next()) {
        return query.value(0).toDateTime();
    }
    
    return QDateTime();
}

int PlayHistoryDao::batchAddPlayRecords(const QList<int>& songIds)
{
    QMutexLocker locker(&m_mutex);
    
    if (songIds.isEmpty()) {
        return 0;
    }
    
    int successCount = 0;
    QDateTime currentTime = QDateTime::currentDateTime();
    
    for (int songId : songIds) {
        if (addPlayRecord(songId, currentTime)) {
            successCount++;
        }
    }
    
    logInfo("batchAddPlayRecords", QString("批量添加播放记录: 成功 %1/%2")
            .arg(successCount).arg(songIds.size()));
    
    return successCount;
}

int PlayHistoryDao::getPlayHistoryCount()
{
    QMutexLocker locker(&m_mutex);
    
    const QString sql = "SELECT COUNT(*) FROM play_history";
    QSqlQuery query = prepareQuery(sql);
    
    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    
    return 0;
}

int PlayHistoryDao::getUniqueSongCount()
{
    QMutexLocker locker(&m_mutex);
    
    const QString sql = "SELECT COUNT(DISTINCT song_id) FROM play_history";
    QSqlQuery query = prepareQuery(sql);
    
    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    
    return 0;
}

PlayHistory PlayHistoryDao::createPlayHistoryFromQuery(const QSqlQuery& query)
{
    PlayHistory history;
    history.setId(query.value("id").toInt());
    history.setSongId(query.value("song_id").toInt());
    history.setPlayedAt(query.value("played_at").toDateTime());
    return history;
}

Song PlayHistoryDao::createSongFromQuery(const QSqlQuery& query)
{
    Song song;
    song.setId(query.value("id").toInt());
    song.setTitle(query.value("title").toString());
    song.setArtist(query.value("artist").toString());
    song.setAlbum(query.value("album").toString());
    song.setFilePath(query.value("file_path").toString());
    song.setDuration(query.value("duration").toLongLong());
    song.setFileSize(query.value("file_size").toLongLong());
    song.setDateAdded(query.value("date_added").toDateTime());
    song.setLastPlayedTime(query.value("last_played").toDateTime());
    song.setPlayCount(query.value("play_count").toInt());
    song.setRating(query.value("rating").toInt());
    song.setCreatedAt(query.value("created_at").toDateTime());
    song.setUpdatedAt(query.value("updated_at").toDateTime());
    
    // 处理标签字符串
    QString tagsStr = query.value("tags").toString();
    if (!tagsStr.isEmpty()) {
        song.setTags(tagsStr.split(",", Qt::SkipEmptyParts));
    }
    
    return song;
}

bool PlayHistoryDao::cleanupDuplicateRecords(int songId)
{
    // 删除该歌曲的重复记录，只保留最新的
    const QString sql = R"(
        DELETE FROM play_history 
        WHERE song_id = ? AND id NOT IN (
            SELECT id FROM (
                SELECT id FROM play_history 
                WHERE song_id = ? 
                ORDER BY played_at DESC 
                LIMIT 1
            )
        )
    )";
    
    QSqlQuery query = prepareQuery(sql);
    query.addBindValue(songId);
    query.addBindValue(songId);
    
    return query.exec();
}

bool PlayHistoryDao::limitPlayHistoryRecords(int maxRecords)
{
    // 如果记录数超过限制，删除最旧的记录
    const QString sql = R"(
        DELETE FROM play_history 
        WHERE id NOT IN (
            SELECT id FROM (
                SELECT id FROM play_history 
                ORDER BY played_at DESC 
                LIMIT ?
            )
        )
    )";
    
    QSqlQuery query = prepareQuery(sql);
    query.addBindValue(maxRecords);
    
    if (query.exec()) {
        int deletedCount = query.numRowsAffected();
        if (deletedCount > 0) {
            logInfo("limitPlayHistoryRecords", QString("清理了 %1 条旧记录").arg(deletedCount));
        }
        return true;
    } else {
        logError("limitPlayHistoryRecords", query.lastError().text());
        return false;
    }
} 