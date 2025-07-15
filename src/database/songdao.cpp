#include "songdao.h"
#include "databasemanager.h"
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariant>
#include <QDebug>

SongDao::SongDao(QObject* parent)
    : BaseDao(parent)
{
}

int SongDao::addSong(const Song& song)
{
    const QString sql = R"(
        INSERT INTO songs (title, artist, album, file_path, duration, file_size, tags, rating)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?)
    )";
    
    QSqlQuery query = prepareQuery(sql);
    query.addBindValue(song.title());
    query.addBindValue(song.artist());
    query.addBindValue(song.album());
    query.addBindValue(song.filePath());
    query.addBindValue(song.duration());
    query.addBindValue(song.fileSize());
    query.addBindValue(song.tags().join(","));
    query.addBindValue(song.rating());
    
    if (query.exec()) {
        return query.lastInsertId().toInt();
    } else {
        logError("addSong", query.lastError().text());
        return -1;
    }
}

Song SongDao::getSongById(int id)
{
    const QString sql = "SELECT * FROM songs WHERE id = ?";
    QSqlQuery query = prepareQuery(sql);
    query.addBindValue(id);
    
    if (query.exec() && query.next()) {
        return createSongFromQuery(query);
    }
    
    return Song(); // 返回空对象
}

Song SongDao::getSongByPath(const QString& filePath)
{
    const QString sql = "SELECT * FROM songs WHERE file_path = ?";
    QSqlQuery query = prepareQuery(sql);
    query.addBindValue(filePath);
    
    if (query.exec() && query.next()) {
        return createSongFromQuery(query);
    }
    
    return Song(); // 返回空对象
}

QList<Song> SongDao::getAllSongs()
{
    QList<Song> songs;
    const QString sql = "SELECT * FROM songs ORDER BY title";
    QSqlQuery query = executeQuery(sql);
    
    while (query.next()) {
        songs.append(createSongFromQuery(query));
    }
    
    return songs;
}

QList<Song> SongDao::searchByTitle(const QString& title)
{
    QList<Song> songs;
    const QString sql = "SELECT * FROM songs WHERE title LIKE ? ORDER BY title";
    QSqlQuery query = prepareQuery(sql);
    query.addBindValue("%" + title + "%");
    
    if (query.exec()) {
        while (query.next()) {
            songs.append(createSongFromQuery(query));
        }
    } else {
        logError("searchByTitle", query.lastError().text());
    }
    
    return songs;
}

QList<Song> SongDao::searchByArtist(const QString& artist)
{
    QList<Song> songs;
    const QString sql = "SELECT * FROM songs WHERE artist LIKE ? ORDER BY title";
    QSqlQuery query = prepareQuery(sql);
    query.addBindValue("%" + artist + "%");
    
    if (query.exec()) {
        while (query.next()) {
            songs.append(createSongFromQuery(query));
        }
    } else {
        logError("searchByArtist", query.lastError().text());
    }
    
    return songs;
}

QList<Song> SongDao::searchByTag(const QString& tag)
{
    QList<Song> songs;
    const QString sql = "SELECT * FROM songs WHERE tags LIKE ? ORDER BY title";
    QSqlQuery query = prepareQuery(sql);
    query.addBindValue("%" + tag + "%");
    
    if (query.exec()) {
        while (query.next()) {
            songs.append(createSongFromQuery(query));
        }
    } else {
        logError("searchByTag", query.lastError().text());
    }
    
    return songs;
}

bool SongDao::updateSong(const Song& song)
{
    const QString sql = R"(
        UPDATE songs SET 
            title = ?, artist = ?, album = ?, duration = ?, 
            file_size = ?, tags = ?, rating = ?, updated_at = CURRENT_TIMESTAMP
        WHERE id = ?
    )";
    
    QSqlQuery query = prepareQuery(sql);
    query.addBindValue(song.title());
    query.addBindValue(song.artist());
    query.addBindValue(song.album());
    query.addBindValue(song.duration());
    query.addBindValue(song.fileSize());
    query.addBindValue(song.tags().join(","));
    query.addBindValue(song.rating());
    query.addBindValue(song.id());
    
    if (query.exec()) {
        return query.numRowsAffected() > 0;
    } else {
        logError("updateSong", query.lastError().text());
        return false;
    }
}

bool SongDao::deleteSong(int id)
{
    const QString sql = "DELETE FROM songs WHERE id = ?";
    QSqlQuery query = prepareQuery(sql);
    query.addBindValue(id);
    
    if (query.exec()) {
        return query.numRowsAffected() > 0;
    } else {
        logError("deleteSong", query.lastError().text());
        return false;
    }
}

bool SongDao::incrementPlayCount(int id)
{
    const QString sql = R"(
        UPDATE songs SET 
            play_count = play_count + 1, 
            last_played = CURRENT_TIMESTAMP,
            updated_at = CURRENT_TIMESTAMP
        WHERE id = ?
    )";
    
    QSqlQuery query = prepareQuery(sql);
    query.addBindValue(id);
    
    if (query.exec()) {
        return query.numRowsAffected() > 0;
    } else {
        logError("incrementPlayCount", query.lastError().text());
        return false;
    }
}

bool SongDao::updateLastPlayed(int id, const QDateTime& lastPlayed)
{
    const QString sql = R"(
        UPDATE songs SET 
            last_played = ?, 
            updated_at = CURRENT_TIMESTAMP
        WHERE id = ?
    )";
    
    QSqlQuery query = prepareQuery(sql);
    query.addBindValue(lastPlayed);
    query.addBindValue(id);
    
    if (query.exec()) {
        return query.numRowsAffected() > 0;
    } else {
        logError("updateLastPlayed", query.lastError().text());
        return false;
    }
}

bool SongDao::updateRating(int id, int rating)
{
    if (rating < 0 || rating > 5) {
        logError("updateRating", "评分必须在0-5之间");
        return false;
    }
    
    const QString sql = R"(
        UPDATE songs SET 
            rating = ?, 
            updated_at = CURRENT_TIMESTAMP
        WHERE id = ?
    )";
    
    QSqlQuery query = prepareQuery(sql);
    query.addBindValue(rating);
    query.addBindValue(id);
    
    if (query.exec()) {
        return query.numRowsAffected() > 0;
    } else {
        logError("updateRating", query.lastError().text());
        return false;
    }
}

bool SongDao::songExists(const QString& filePath)
{
    const QString sql = "SELECT COUNT(*) FROM songs WHERE file_path = ?";
    QSqlQuery query = prepareQuery(sql);
    query.addBindValue(filePath);
    
    if (query.exec() && query.next()) {
        return query.value(0).toInt() > 0;
    }
    
    return false;
}

int SongDao::getSongCount()
{
    const QString sql = "SELECT COUNT(*) FROM songs";
    QSqlQuery query = executeQuery(sql);
    
    if (query.next()) {
        return query.value(0).toInt();
    }
    
    return 0;
}

Song SongDao::createSongFromQuery(const QSqlQuery& query)
{
    Song song;
    song.setId(query.value("id").toInt());
    song.setTitle(query.value("title").toString());
    song.setArtist(query.value("artist").toString());
    song.setAlbum(query.value("album").toString());
    song.setFilePath(query.value("file_path").toString());
    song.setDuration(query.value("duration").toInt());
    song.setFileSize(query.value("file_size").toLongLong());
    song.setDateAdded(query.value("date_added").toDateTime());
    song.setLastPlayedTime(query.value("last_played").toDateTime());
    song.setPlayCount(query.value("play_count").toInt());
    song.setRating(query.value("rating").toInt());
    
    // 解析标签字符串
    QString tagsStr = query.value("tags").toString();
    if (!tagsStr.isEmpty()) {
        song.setTags(tagsStr.split(",", Qt::SkipEmptyParts));
    }
    
    return song;
}

QList<Song> SongDao::getSongsByTag(int tagId)
{
    QList<Song> songs;
    const QString sql = "SELECT s.* FROM songs s "
                       "INNER JOIN song_tags st ON s.id = st.song_id "
                       "WHERE st.tag_id = ?";
    
    QSqlQuery query = prepareQuery(sql);
    query.addBindValue(tagId);
    
    if (query.exec()) {
        while (query.next()) {
            songs.append(createSongFromQuery(query));
        }
    }
    
    return songs;
}

bool SongDao::removeSongFromTag(int songId, int tagId)
{
    const QString sql = "DELETE FROM song_tags WHERE song_id = ? AND tag_id = ?";
    QSqlQuery query = prepareQuery(sql);
    query.addBindValue(songId);
    query.addBindValue(tagId);
    
    return query.exec();
}

bool SongDao::addSongToTag(int songId, int tagId)
{
    const QString sql = "INSERT OR IGNORE INTO song_tags (song_id, tag_id) VALUES (?, ?)";
    QSqlQuery query = prepareQuery(sql);
    query.addBindValue(songId);
    query.addBindValue(tagId);
    
    return query.exec();
}

bool SongDao::songHasTag(int songId, int tagId)
{
    const QString sql = "SELECT COUNT(*) FROM song_tags WHERE song_id = ? AND tag_id = ?";
    QSqlQuery query = prepareQuery(sql);
    query.addBindValue(songId);
    query.addBindValue(tagId);
    
    if (query.exec() && query.next()) {
        return query.value(0).toInt() > 0;
    }
    
    return false;
}

int SongDao::insertSongs(const QList<Song>& songs)
{
    int insertedCount = 0;
    
    for (const Song& song : songs) {
        int id = addSong(song);
        if (id > 0) {
            insertedCount++;
        }
    }
    
    return insertedCount;
}