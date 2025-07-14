#include "songdao.h"
#include "databasemanager.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>

SongDao::SongDao(QObject* parent)
    : QObject(parent)
    , m_dbManager(DatabaseManager::instance())
{
}

SongDao::~SongDao()
{
}

int SongDao::insertSong(const Song& song)
{
    // TODO: 实现完整的插入逻辑
    Q_UNUSED(song);
    return -1;
}

int SongDao::insertSongs(const QList<Song>& songs)
{
    if (songs.isEmpty()) return 0;
    QSqlDatabase db = m_dbManager->database();
    if (!db.isOpen()) {
        logError("数据库未打开");
        return 0;
    }
    int successCount = 0;
    if (!db.transaction()) {
        logError("开启事务失败: " + db.lastError().text());
        return 0;
    }
    QSqlQuery query(db);
    for (const Song& song : songs) {
        // 跳过已存在的文件路径
        query.prepare("SELECT COUNT(*) FROM songs WHERE file_path = ?");
        query.addBindValue(song.filePath());
        if (!query.exec() || !query.next() || query.value(0).toInt() > 0) {
            continue;
        }
        query.prepare(SQLStatements::INSERT_SONG);
        query.addBindValue(song.filePath());
        query.addBindValue(song.fileName());
        query.addBindValue(song.title());
        query.addBindValue(song.artist());
        query.addBindValue(song.album());
        query.addBindValue(song.duration());
        query.addBindValue(song.fileSize());
        query.addBindValue(song.bitRate());
        query.addBindValue(song.sampleRate());
        query.addBindValue(song.channels());
        query.addBindValue(song.fileFormat());
        query.addBindValue(song.coverPath());
        query.addBindValue(song.hasLyrics());
        query.addBindValue(song.lyricsPath());
        query.addBindValue(song.playCount());
        query.addBindValue(song.lastPlayedTime().toSecsSinceEpoch());
        query.addBindValue(song.dateAdded().toSecsSinceEpoch());
        query.addBindValue(song.dateModified().toSecsSinceEpoch());
        query.addBindValue(song.isFavorite());
        query.addBindValue(song.isAvailable());
        query.addBindValue(song.createdAt().toSecsSinceEpoch());
        query.addBindValue(song.updatedAt().toSecsSinceEpoch());
        if (query.exec()) {
            ++successCount;
            emit songInserted(song);
        } else {
            logSqlError(query, "insertSong");
        }
    }
    if (!db.commit()) {
        logError("提交事务失败: " + db.lastError().text());
        db.rollback();
        return 0;
    }
    return successCount;
}

bool SongDao::updateSong(const Song& song)
{
    // TODO: 实现更新逻辑
    Q_UNUSED(song);
    return false;
}

bool SongDao::deleteSong(int songId)
{
    // TODO: 实现删除逻辑
    Q_UNUSED(songId);
    return false;
}

Song SongDao::getSong(int songId)
{
    // TODO: 实现获取单个歌曲逻辑
    Q_UNUSED(songId);
    return Song();
}

QList<Song> SongDao::getAllSongs()
{
    // TODO: 实现获取所有歌曲逻辑
    return QList<Song>();
}

int SongDao::getSongCount()
{
    // TODO: 实现获取歌曲数量逻辑
    return 0;
}

bool SongDao::songExists(int songId)
{
    // TODO: 实现检查歌曲是否存在逻辑
    Q_UNUSED(songId);
    return false;
}

bool SongDao::addSongToTag(int songId, int tagId)
{
    QSqlQuery query;
    query.prepare("INSERT OR IGNORE INTO song_tag_rel (song_id, tag_id) VALUES (?, ?)");
    query.addBindValue(songId);
    query.addBindValue(tagId);
    if (!query.exec()) {
        logSqlError(query, "addSongToTag");
        return false;
    }
    return true;
}

bool SongDao::removeSongFromTag(int songId, int tagId)
{
    QSqlQuery query;
    query.prepare("DELETE FROM song_tag_rel WHERE song_id = ? AND tag_id = ?");
    query.addBindValue(songId);
    query.addBindValue(tagId);
    if (!query.exec()) {
        logSqlError(query, "removeSongFromTag");
        return false;
    }
    return true;
}

bool SongDao::songHasTag(int songId, int tagId)
{
    QSqlQuery query;
    query.prepare("SELECT COUNT(*) FROM song_tag_rel WHERE song_id = ? AND tag_id = ?");
    query.addBindValue(songId);
    query.addBindValue(tagId);
    if (!query.exec() || !query.next()) {
        logSqlError(query, "songHasTag");
        return false;
    }
    return query.value(0).toInt() > 0;
}

QList<Song> SongDao::getSongsByTag(int tagId)
{
    QList<Song> songs;
    QSqlDatabase db = m_dbManager->database();
    if (!db.isOpen()) {
        logError("数据库未打开");
        return songs;
    }
    QSqlQuery query(db);
    query.prepare("SELECT s.* FROM songs s JOIN song_tag_rel r ON s.id = r.song_id WHERE r.tag_id = ? AND s.is_available = 1 ORDER BY s.title");
    query.addBindValue(tagId);
    if (!query.exec()) {
        logSqlError(query, "getSongsByTag");
        return songs;
    }
    while (query.next()) {
        songs.append(createSongFromQuery(query));
    }
    return songs;
}

QString SongDao::lastError() const
{
    return m_lastError;
}

Song SongDao::createSongFromQuery(const QSqlQuery& query)
{
    // TODO: 实现从查询结果创建Song对象的逻辑
    Q_UNUSED(query);
    return Song();
}

void SongDao::logError(const QString& error)
{
    m_lastError = error;
    qWarning() << "SongDao Error:" << error;
    emit databaseError(error);
}

void SongDao::logSqlError(const QSqlQuery& query, const QString& operation)
{
    QString error = QString("SQL Error in %1: %2").arg(operation, query.lastError().text());
    logError(error);
}

// SQL语句常量定义 - 待完善
const QString SongDao::SQLStatements::INSERT_SONG = 
    "INSERT INTO songs (file_path, file_name, title, artist, album, duration, "
    "file_size, bit_rate, sample_rate, channels, file_format, cover_path, "
    "has_lyrics, lyrics_path, play_count, last_played_time, date_added, "
    "date_modified, is_favorite, is_available, created_at, updated_at) "
    "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

const QString SongDao::SQLStatements::SELECT_ALL_SONGS = 
    "SELECT * FROM songs WHERE is_available = 1 ORDER BY title";

const QString SongDao::SQLStatements::COUNT_SONGS = 
    "SELECT COUNT(*) FROM songs WHERE is_available = 1";

// 其他SQL语句常量将在后续实现中添加 