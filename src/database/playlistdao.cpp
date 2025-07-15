#include "playlistdao.h"
#include "songdao.h"
#include "databasemanager.h"
#include "../core/constants.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDateTime>
#include <QDebug>

PlaylistDao::PlaylistDao(QObject* parent)
    : BaseDao(parent)
{
    qDebug() << "PlaylistDao 构造函数";
}

PlaylistDao::~PlaylistDao()
{
    qDebug() << "PlaylistDao 析构函数";
}

int PlaylistDao::addPlaylist(const Playlist& playlist)
{
    if (!dbManager() || !dbManager()->isInitialized()) {
        logError("PlaylistDao::addPlaylist", "数据库未连接");
        return -1;
    }
    
    if (playlist.name().trimmed().isEmpty()) {
        logError("PlaylistDao::addPlaylist", "播放列表名称不能为空");
        return -1;
    }
    
    // 检查播放列表名称是否已存在
    if (playlistExists(playlist.name())) {
        logError("PlaylistDao::addPlaylist", 
            QString("播放列表名称已存在: %1").arg(playlist.name()));
        return -1;
    }
    
    try {
        QSqlQuery query(dbManager()->database());
        QString sql = QString(
            "INSERT INTO %1 (name, description, created_at, modified_at, "
            "song_count, total_duration, play_count, color, icon_path, "
            "is_smart_playlist, smart_criteria, is_system_playlist, "
            "is_favorite, sort_order) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"
        ).arg(Constants::Database::TABLE_PLAYLISTS);
        
        query.prepare(sql);
        
        QDateTime now = QDateTime::currentDateTime();
        query.addBindValue(playlist.name());
        query.addBindValue(playlist.description());
        query.addBindValue(now);
        query.addBindValue(now);
        query.addBindValue(0); // song_count
        query.addBindValue(0); // total_duration
        query.addBindValue(0); // play_count
        query.addBindValue(playlist.color());
        query.addBindValue(playlist.iconPath());
        query.addBindValue(playlist.isSmartPlaylist());
        query.addBindValue(playlist.smartCriteria());
        query.addBindValue(playlist.isSystemPlaylist());
        query.addBindValue(playlist.isFavorite());
        query.addBindValue(playlist.sortOrder());
        
        if (!query.exec()) {
            logError("PlaylistDao::addPlaylist", 
                QString("添加播放列表失败: %1").arg(query.lastError().text()));
            return -1;
        }
        
        int playlistId = query.lastInsertId().toInt();
        // 成功添加播放列表: playlist.name(), ID: playlistId
        
        return playlistId;
        
    } catch (const std::exception& e) {
        logError("PlaylistDao::addPlaylist", 
            QString("添加播放列表时发生异常: %1").arg(e.what()));
        return -1;
    }
}

bool PlaylistDao::updatePlaylist(const Playlist& playlist)
{
    if (!dbManager() || !dbManager()->isInitialized()) {
        logError("PlaylistDao::updatePlaylist", "数据库未连接");
        return false;
    }
    
    if (playlist.id() <= 0) {
        logError("PlaylistDao::updatePlaylist", "无效的播放列表ID");
        return false;
    }
    
    if (playlist.name().trimmed().isEmpty()) {
        logError("PlaylistDao::updatePlaylist", "播放列表名称不能为空");
        return false;
    }
    
    try {
        QSqlQuery query(dbManager()->database());
        QString sql = QString(
            "UPDATE %1 SET name=?, description=?, modified_at=?, "
            "color=?, icon_path=?, is_smart_playlist=?, smart_criteria=?, "
            "is_system_playlist=?, is_favorite=?, sort_order=? "
            "WHERE id=?"
        ).arg(Constants::Database::TABLE_PLAYLISTS);
        
        query.prepare(sql);
        
        query.addBindValue(playlist.name());
        query.addBindValue(playlist.description());
        query.addBindValue(QDateTime::currentDateTime());
        query.addBindValue(playlist.color());
        query.addBindValue(playlist.iconPath());
        query.addBindValue(playlist.isSmartPlaylist());
        query.addBindValue(playlist.smartCriteria());
        query.addBindValue(playlist.isSystemPlaylist());
        query.addBindValue(playlist.isFavorite());
        query.addBindValue(playlist.sortOrder());
        query.addBindValue(playlist.id());
        
        if (!query.exec()) {
            logError("PlaylistDao::updatePlaylist", 
                QString("更新播放列表失败: %1").arg(query.lastError().text()));
            return false;
        }
        
        if (query.numRowsAffected() == 0) {
            // 播放列表不存在: ID=playlist.id()
            return false;
        }
        
        // 成功更新播放列表: playlist.name()
        
        return true;
        
    } catch (const std::exception& e) {
        logError("PlaylistDao::updatePlaylist", 
            QString("更新播放列表时发生异常: %1").arg(e.what()));
        return false;
    }
}

bool PlaylistDao::deletePlaylist(int id)
{
    if (!dbManager() || !dbManager()->isInitialized()) {
        logError("PlaylistDao::deletePlaylist", "数据库未连接");
        return false;
    }
    
    if (id <= 0) {
        logError("PlaylistDao::deletePlaylist", "无效的播放列表ID");
        return false;
    }
    
    try {
        // 首先删除播放列表-歌曲关联
        QSqlQuery deleteAssocQuery(dbManager()->database());
        QString deleteAssocSql = QString(
            "DELETE FROM %1 WHERE playlist_id = ?"
        ).arg(Constants::Database::TABLE_PLAYLIST_SONGS);
        
        deleteAssocQuery.prepare(deleteAssocSql);
        deleteAssocQuery.addBindValue(id);
        
        if (!deleteAssocQuery.exec()) {
            logError("PlaylistDao::deletePlaylist", 
                QString("删除播放列表关联失败: %1").arg(deleteAssocQuery.lastError().text()));
            return false;
        }
        
        // 然后删除播放列表
        QSqlQuery deletePlaylistQuery(dbManager()->database());
        QString deletePlaylistSql = QString(
            "DELETE FROM %1 WHERE id = ?"
        ).arg(Constants::Database::TABLE_PLAYLISTS);
        
        deletePlaylistQuery.prepare(deletePlaylistSql);
        deletePlaylistQuery.addBindValue(id);
        
        if (!deletePlaylistQuery.exec()) {
            logError("PlaylistDao::deletePlaylist", 
                QString("删除播放列表失败: %1").arg(deletePlaylistQuery.lastError().text()));
            return false;
        }
        
        if (deletePlaylistQuery.numRowsAffected() == 0) {
            // 播放列表不存在: ID=id
            return false;
        }
        
        // 成功删除播放列表: ID=id
        
        return true;
        
    } catch (const std::exception& e) {
        logError("PlaylistDao::deletePlaylist", 
            QString("删除播放列表时发生异常: %1").arg(e.what()));
        return false;
    }
}

Playlist PlaylistDao::getPlaylistById(int id) const
{
    Playlist playlist;
    
    if (!dbManager() || !dbManager()->isInitialized()) {
        logError("PlaylistDao::getPlaylistById", "数据库未连接");
        return playlist;
    }
    
    if (id <= 0) {
        logError("PlaylistDao::getPlaylistById", "无效的播放列表ID");
        return playlist;
    }
    
    try {
        QSqlQuery query(dbManager()->database());
        QString sql = QString(
            "SELECT * FROM %1 WHERE id = ?"
        ).arg(Constants::Database::TABLE_PLAYLISTS);
        
        query.prepare(sql);
        query.addBindValue(id);
        
        if (!query.exec()) {
            logError("PlaylistDao::getPlaylistById", 
                QString("查询播放列表失败: %1").arg(query.lastError().text()));
            return playlist;
        }
        
        if (query.next()) {
            playlist = createPlaylistFromQuery(query);
        }
        
        return playlist;
        
    } catch (const std::exception& e) {
        logError("PlaylistDao::getPlaylistById", 
            QString("查询播放列表时发生异常: %1").arg(e.what()));
        return playlist;
    }
}

Playlist PlaylistDao::getPlaylistByName(const QString& name) const
{
    Playlist playlist;
    
    if (!dbManager() || !dbManager()->isInitialized()) {
        logError("PlaylistDao::getPlaylistByName", "数据库未连接");
        return playlist;
    }
    
    if (name.trimmed().isEmpty()) {
        logError("PlaylistDao::getPlaylistByName", "播放列表名称不能为空");
        return playlist;
    }
    
    try {
        QSqlQuery query(dbManager()->database());
        QString sql = QString(
            "SELECT * FROM %1 WHERE name = ?"
        ).arg(Constants::Database::TABLE_PLAYLISTS);
        
        query.prepare(sql);
        query.addBindValue(name.trimmed());
        
        if (!query.exec()) {
            logError("PlaylistDao::getPlaylistByName", 
                QString("查询播放列表失败: %1").arg(query.lastError().text()));
            return playlist;
        }
        
        if (query.next()) {
            playlist = createPlaylistFromQuery(query);
        }
        
        return playlist;
        
    } catch (const std::exception& e) {
        logError("PlaylistDao::getPlaylistByName", 
            QString("查询播放列表时发生异常: %1").arg(e.what()));
        return playlist;
    }
}

QList<Playlist> PlaylistDao::getAllPlaylists() const
{
    QList<Playlist> playlists;
    
    if (!dbManager() || !dbManager()->isInitialized()) {
        logError("PlaylistDao::getAllPlaylists", "数据库未连接");
        return playlists;
    }
    
    try {
        QSqlQuery query(dbManager()->database());
        QString sql = QString(
            "SELECT * FROM %1 ORDER BY sort_order ASC, created_at DESC"
        ).arg(Constants::Database::TABLE_PLAYLISTS);
        
        if (!query.exec()) {
            logError("PlaylistDao::getAllPlaylists", 
                QString("查询所有播放列表失败: %1").arg(query.lastError().text()));
            return playlists;
        }
        
        while (query.next()) {
            Playlist playlist = createPlaylistFromQuery(query);
            if (playlist.isValid()) {
                playlists.append(playlist);
            }
        }
        
        // Logger::instance().logInfo("PlaylistDao::getAllPlaylists", 
        //     QString("成功查询到 %1 个播放列表").arg(playlists.size()));
        
        return playlists;
        
    } catch (const std::exception& e) {
        logError("PlaylistDao::getAllPlaylists", 
            QString("查询所有播放列表时发生异常: %1").arg(e.what()));
        return playlists;
    }
}

bool PlaylistDao::playlistExists(const QString& name) const
{
    if (!dbManager() || !dbManager()->isInitialized()) {
        return false;
    }
    
    if (name.trimmed().isEmpty()) {
        return false;
    }
    
    try {
        QSqlQuery query(dbManager()->database());
        QString sql = QString(
            "SELECT COUNT(*) FROM %1 WHERE name = ?"
        ).arg(Constants::Database::TABLE_PLAYLISTS);
        
        query.prepare(sql);
        query.addBindValue(name.trimmed());
        
        if (!query.exec()) {
            return false;
        }
        
        if (query.next()) {
            return query.value(0).toInt() > 0;
        }
        
        return false;
        
    } catch (const std::exception& e) {
        return false;
    }
}

bool PlaylistDao::playlistExists(int id) const
{
    if (!dbManager() || !dbManager()->isInitialized()) {
        return false;
    }
    
    if (id <= 0) {
        return false;
    }
    
    try {
        QSqlQuery query(dbManager()->database());
        QString sql = QString(
            "SELECT COUNT(*) FROM %1 WHERE id = ?"
        ).arg(Constants::Database::TABLE_PLAYLISTS);
        
        query.prepare(sql);
        query.addBindValue(id);
        
        if (!query.exec()) {
            return false;
        }
        
        if (query.next()) {
            return query.value(0).toInt() > 0;
        }
        
        return false;
        
    } catch (const std::exception& e) {
        return false;
    }
}

bool PlaylistDao::addSongToPlaylist(int playlistId, int songId, int sortOrder)
{
    if (!dbManager() || !dbManager()->isInitialized()) {
        logError("PlaylistDao::addSongToPlaylist", "数据库未连接");
        return false;
    }
    
    if (playlistId <= 0 || songId <= 0) {
        logError("PlaylistDao::addSongToPlaylist", "无效的播放列表ID或歌曲ID");
        return false;
    }
    
    try {
        // 如果没有指定排序序号，获取下一个序号
        if (sortOrder < 0) {
            sortOrder = getNextSortOrder(playlistId);
        }
        
        QSqlQuery query(dbManager()->database());
        QString sql = QString(
            "INSERT OR REPLACE INTO %1 (playlist_id, song_id, sort_order, added_at) "
            "VALUES (?, ?, ?, ?)"
        ).arg(Constants::Database::TABLE_PLAYLIST_SONGS);
        
        query.prepare(sql);
        query.addBindValue(playlistId);
        query.addBindValue(songId);
        query.addBindValue(sortOrder);
        query.addBindValue(QDateTime::currentDateTime());
        
        if (!query.exec()) {
            logError("PlaylistDao::addSongToPlaylist",
                QString("添加歌曲到播放列表失败: %1").arg(query.lastError().text()));
            return false;
        }
        
        // Logger::instance().logInfo("PlaylistDao::addSongToPlaylist",
        //     QString("成功添加歌曲 %1 到播放列表 %2").arg(songId).arg(playlistId));
        
        return true;
        
    } catch (const std::exception& e) {
        logError("PlaylistDao::addSongToPlaylist",
            QString("添加歌曲到播放列表时发生异常: %1").arg(e.what()));
        return false;
    }
}

bool PlaylistDao::removeSongFromPlaylist(int playlistId, int songId)
{
    if (!dbManager() || !dbManager()->isInitialized()) {
        logError("PlaylistDao::removeSongFromPlaylist", "数据库未连接");
        return false;
    }
    
    if (playlistId <= 0 || songId <= 0) {
        logError("PlaylistDao::removeSongFromPlaylist", "无效的播放列表ID或歌曲ID");
        return false;
    }
    
    try {
        QSqlQuery query(dbManager()->database());
        QString sql = QString(
            "DELETE FROM %1 WHERE playlist_id = ? AND song_id = ?"
        ).arg(Constants::Database::TABLE_PLAYLIST_SONGS);
        
        query.prepare(sql);
        query.addBindValue(playlistId);
        query.addBindValue(songId);
        
        if (!query.exec()) {
            logError("PlaylistDao::removeSongFromPlaylist",
                QString("从播放列表移除歌曲失败: %1").arg(query.lastError().text()));
            return false;
        }
        
        if (query.numRowsAffected() == 0) {
            // Logger::instance().logWarning("PlaylistDao::removeSongFromPlaylist",
            //     QString("歌曲 %1 不在播放列表 %2 中").arg(songId).arg(playlistId));
            return false;
        }
        
        // 重新排序播放列表中的歌曲
        reorderPlaylistSongs(playlistId);
        
        // 更新播放列表统计信息
        updatePlaylistStatistics(playlistId);
        
        // Logger::instance().logInfo("PlaylistDao::removeSongFromPlaylist",
        //     QString("成功从播放列表 %1 移除歌曲 %2").arg(playlistId).arg(songId));
        
        return true;
        
    } catch (const std::exception& e) {
        logError("PlaylistDao::removeSongFromPlaylist",
            QString("从播放列表移除歌曲时发生异常: %1").arg(e.what()));
        return false;
    }
}

QList<Song> PlaylistDao::getPlaylistSongs(int playlistId) const
{
    QList<Song> songs;
    
    if (!dbManager() || !dbManager()->isInitialized()) {
        logError("PlaylistDao::getPlaylistSongs", "数据库未连接");
        return songs;
    }
    
    if (playlistId <= 0) {
        logError("PlaylistDao::getPlaylistSongs", "无效的播放列表ID");
        return songs;
    }
    
    try {
        SongDao songDao;
        QSqlQuery query(dbManager()->database());
        QString sql = QString(
            "SELECT s.* FROM %1 s "
            "INNER JOIN %2 ps ON s.id = ps.song_id "
            "WHERE ps.playlist_id = ? "
            "ORDER BY ps.sort_order ASC"
        ).arg(Constants::Database::TABLE_SONGS)
         .arg(Constants::Database::TABLE_PLAYLIST_SONGS);
        
        query.prepare(sql);
        query.addBindValue(playlistId);
        
        if (!query.exec()) {
            logError("PlaylistDao::getPlaylistSongs",
                QString("查询播放列表歌曲失败: %1").arg(query.lastError().text()));
            return songs;
        }
        
        while (query.next()) {
            Song song = songDao.createSongFromQuery(query);
            if (song.isValid()) {
                songs.append(song);
            }
        }
        
        return songs;
        
    } catch (const std::exception& e) {
        logError("PlaylistDao::getPlaylistSongs",
            QString("查询播放列表歌曲时发生异常: %1").arg(e.what()));
        return songs;
    }
}

int PlaylistDao::getPlaylistSongCount(int playlistId) const
{
    if (!dbManager() || !dbManager()->isInitialized()) {
        return 0;
    }
    
    if (playlistId <= 0) {
        return 0;
    }
    
    try {
        QSqlQuery query(dbManager()->database());
        QString sql = QString(
            "SELECT COUNT(*) FROM %1 WHERE playlist_id = ?"
        ).arg(Constants::Database::TABLE_PLAYLIST_SONGS);
        
        query.prepare(sql);
        query.addBindValue(playlistId);
        
        if (!query.exec()) {
            return 0;
        }
        
        if (query.next()) {
            return query.value(0).toInt();
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        return 0;
    }
}

bool PlaylistDao::clearPlaylist(int playlistId)
{
    if (!dbManager() || !dbManager()->isInitialized()) {
        logError("PlaylistDao::clearPlaylist", "数据库未连接");
        return false;
    }
    
    if (playlistId <= 0) {
        logError("PlaylistDao::clearPlaylist", "无效的播放列表ID");
        return false;
    }
    
    try {
        QSqlQuery query(dbManager()->database());
        QString sql = QString(
            "DELETE FROM %1 WHERE playlist_id = ?"
        ).arg(Constants::Database::TABLE_PLAYLIST_SONGS);
        
        query.prepare(sql);
        query.addBindValue(playlistId);
        
        if (!query.exec()) {
            logError("PlaylistDao::clearPlaylist",
                QString("清空播放列表失败: %1").arg(query.lastError().text()));
            return false;
        }
        
        // 更新播放列表统计信息
        updatePlaylistStatistics(playlistId);
        
        // Logger::instance().logInfo("PlaylistDao::clearPlaylist",
        //     QString("成功清空播放列表 %1").arg(playlistId));
        
        return true;
        
    } catch (const std::exception& e) {
        logError("PlaylistDao::clearPlaylist",
            QString("清空播放列表时发生异常: %1").arg(e.what()));
        return false;
    }
}

bool PlaylistDao::updatePlaylistStatistics(int playlistId)
{
    if (!dbManager() || !dbManager()->isInitialized()) {
        return false;
    }
    
    if (playlistId <= 0) {
        return false;
    }
    
    try {
        // 计算歌曲数量和总时长
        QSqlQuery statsQuery(dbManager()->database());
        QString statsSql = QString(
            "SELECT COUNT(*), COALESCE(SUM(s.duration), 0) "
            "FROM %1 ps "
            "INNER JOIN %2 s ON ps.song_id = s.id "
            "WHERE ps.playlist_id = ?"
        ).arg(Constants::Database::TABLE_PLAYLIST_SONGS)
         .arg(Constants::Database::TABLE_SONGS);
        
        statsQuery.prepare(statsSql);
        statsQuery.addBindValue(playlistId);
        
        if (!statsQuery.exec() || !statsQuery.next()) {
            return false;
        }
        
        int songCount = statsQuery.value(0).toInt();
        qint64 totalDuration = statsQuery.value(1).toLongLong();
        
        // 更新播放列表统计信息
        QSqlQuery updateQuery(dbManager()->database());
        QString updateSql = QString(
            "UPDATE %1 SET song_count = ?, total_duration = ?, modified_at = ? "
            "WHERE id = ?"
        ).arg(Constants::Database::TABLE_PLAYLISTS);
        
        updateQuery.prepare(updateSql);
        updateQuery.addBindValue(songCount);
        updateQuery.addBindValue(totalDuration);
        updateQuery.addBindValue(QDateTime::currentDateTime());
        updateQuery.addBindValue(playlistId);
        
        return updateQuery.exec();
        
    } catch (const std::exception& e) {
        return false;
    }
}

QList<Playlist> PlaylistDao::getRecentPlaylists(int count) const
{
    QList<Playlist> playlists;
    
    if (!dbManager() || !dbManager()->isInitialized()) {
        return playlists;
    }
    
    try {
        QSqlQuery query(dbManager()->database());
        QString sql = QString(
            "SELECT * FROM %1 "
            "WHERE last_played_at IS NOT NULL "
            "ORDER BY last_played_at DESC "
            "LIMIT ?"
        ).arg(Constants::Database::TABLE_PLAYLISTS);
        
        query.prepare(sql);
        query.addBindValue(count);
        
        if (!query.exec()) {
            return playlists;
        }
        
        while (query.next()) {
            Playlist playlist = createPlaylistFromQuery(query);
            if (playlist.isValid()) {
                playlists.append(playlist);
            }
        }
        
        return playlists;
        
    } catch (const std::exception& e) {
        return playlists;
    }
}

QList<Playlist> PlaylistDao::getFavoritePlaylists() const
{
    QList<Playlist> playlists;
    
    if (!dbManager() || !dbManager()->isInitialized()) {
        return playlists;
    }
    
    try {
        QSqlQuery query(dbManager()->database());
        QString sql = QString(
            "SELECT * FROM %1 "
            "WHERE is_favorite = 1 "
            "ORDER BY sort_order ASC, created_at DESC"
        ).arg(Constants::Database::TABLE_PLAYLISTS);
        
        if (!query.exec()) {
            return playlists;
        }
        
        while (query.next()) {
            Playlist playlist = createPlaylistFromQuery(query);
            if (playlist.isValid()) {
                playlists.append(playlist);
            }
        }
        
        return playlists;
        
    } catch (const std::exception& e) {
        return playlists;
    }
}

Playlist PlaylistDao::createPlaylistFromQuery(const QSqlQuery& query) const
{
    Playlist playlist;
    
    try {
        playlist.setId(query.value("id").toInt());
        playlist.setName(query.value("name").toString());
        playlist.setDescription(query.value("description").toString());
        playlist.setCreatedAt(query.value("created_at").toDateTime());
        playlist.setModifiedAt(query.value("modified_at").toDateTime());
        playlist.setLastPlayedAt(query.value("last_played_at").toDateTime());
        playlist.setSongCount(query.value("song_count").toInt());
        playlist.setTotalDuration(query.value("total_duration").toLongLong());
        playlist.setPlayCount(query.value("play_count").toInt());
        playlist.setColor(QColor(query.value("color").toString()));
        playlist.setIconPath(query.value("icon_path").toString());
        playlist.setIsSmartPlaylist(query.value("is_smart_playlist").toBool());
        playlist.setSmartCriteria(query.value("smart_criteria").toString());
        playlist.setIsSystemPlaylist(query.value("is_system_playlist").toBool());
        playlist.setIsFavorite(query.value("is_favorite").toBool());
        playlist.setSortOrder(query.value("sort_order").toInt());
        
    } catch (const std::exception& e) {
        logError("PlaylistDao::createPlaylistFromQuery", 
            QString("从查询结果创建播放列表对象时发生异常: %1").arg(e.what()));
    }
    
    return playlist;
}

int PlaylistDao::getNextSortOrder(int playlistId) const
{
    if (!dbManager() || !dbManager()->isInitialized()) {
        return 0;
    }
    
    try {
        QSqlQuery query(dbManager()->database());
        QString sql = QString(
            "SELECT COALESCE(MAX(sort_order), -1) + 1 FROM %1 WHERE playlist_id = ?"
        ).arg(Constants::Database::TABLE_PLAYLIST_SONGS);
        
        query.prepare(sql);
        query.addBindValue(playlistId);
        
        if (!query.exec() || !query.next()) {
            return 0;
        }
        
        return query.value(0).toInt();
        
    } catch (const std::exception& e) {
        return 0;
    }
}

bool PlaylistDao::reorderPlaylistSongs(int playlistId)
{
    if (!dbManager() || !dbManager()->isInitialized()) {
        return false;
    }
    
    if (playlistId <= 0) {
        return false;
    }
    
    try {
        // 获取当前播放列表中的所有歌曲ID，按当前排序
        QSqlQuery selectQuery(dbManager()->database());
        QString selectSql = QString(
            "SELECT id, song_id FROM %1 "
            "WHERE playlist_id = ? "
            "ORDER BY sort_order ASC"
        ).arg(Constants::Database::TABLE_PLAYLIST_SONGS);
        
        selectQuery.prepare(selectSql);
        selectQuery.addBindValue(playlistId);
        
        if (!selectQuery.exec()) {
            return false;
        }
        
        // 重新分配排序序号
        int newSortOrder = 0;
        while (selectQuery.next()) {
            int id = selectQuery.value("id").toInt();
            
            QSqlQuery updateQuery(dbManager()->database());
            QString updateSql = QString(
                "UPDATE %1 SET sort_order = ? WHERE id = ?"
            ).arg(Constants::Database::TABLE_PLAYLIST_SONGS);
            
            updateQuery.prepare(updateSql);
            updateQuery.addBindValue(newSortOrder++);
            updateQuery.addBindValue(id);
            
            if (!updateQuery.exec()) {
                return false;
            }
        }
        
        return true;
        
    } catch (const std::exception& e) {
        return false;
    }
}