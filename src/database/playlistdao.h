#ifndef PLAYLISTDAO_H
#define PLAYLISTDAO_H

#include "basedao.h"
#include "../models/playlist.h"
#include "../models/song.h"
#include <QList>
#include <QString>
#include <QDateTime>

/**
 * @brief 播放列表数据访问对象
 * 
 * 负责播放列表相关的数据库操作，包括播放列表的增删改查、
 * 播放列表与歌曲的关联管理等功能。
 */
class PlaylistDao : public BaseDao
{
    Q_OBJECT
    
public:
    explicit PlaylistDao(QObject* parent = nullptr);
    ~PlaylistDao() override;
    
    // 播放列表基本操作
    /**
     * @brief 添加播放列表
     * @param playlist 播放列表对象
     * @return 成功返回播放列表ID，失败返回-1
     */
    int addPlaylist(const Playlist& playlist);
    
    /**
     * @brief 更新播放列表
     * @param playlist 播放列表对象
     * @return 操作是否成功
     */
    bool updatePlaylist(const Playlist& playlist);
    
    /**
     * @brief 删除播放列表
     * @param id 播放列表ID
     * @return 操作是否成功
     */
    bool deletePlaylist(int id);
    
    /**
     * @brief 根据ID获取播放列表
     * @param id 播放列表ID
     * @return 播放列表对象
     */
    Playlist getPlaylistById(int id) const;
    
    /**
     * @brief 根据名称获取播放列表
     * @param name 播放列表名称
     * @return 播放列表对象
     */
    Playlist getPlaylistByName(const QString& name) const;
    
    /**
     * @brief 获取所有播放列表
     * @return 播放列表列表
     */
    QList<Playlist> getAllPlaylists() const;
    
    /**
     * @brief 检查播放列表是否存在
     * @param name 播放列表名称
     * @return 是否存在
     */
    bool playlistExists(const QString& name) const;
    
    /**
     * @brief 检查播放列表是否存在
     * @param id 播放列表ID
     * @return 是否存在
     */
    bool playlistExists(int id) const;
    
    // 播放列表-歌曲关联操作
    /**
     * @brief 添加歌曲到播放列表
     * @param playlistId 播放列表ID
     * @param songId 歌曲ID
     * @param sortOrder 排序序号
     * @return 操作是否成功
     */
    bool addSongToPlaylist(int playlistId, int songId, int sortOrder = -1);
    
    /**
     * @brief 从播放列表移除歌曲
     * @param playlistId 播放列表ID
     * @param songId 歌曲ID
     * @return 操作是否成功
     */
    bool removeSongFromPlaylist(int playlistId, int songId);
    
    /**
     * @brief 获取播放列表中的所有歌曲
     * @param playlistId 播放列表ID
     * @return 歌曲列表
     */
    QList<Song> getPlaylistSongs(int playlistId) const;
    
    /**
     * @brief 获取播放列表中的歌曲数量
     * @param playlistId 播放列表ID
     * @return 歌曲数量
     */
    int getPlaylistSongCount(int playlistId) const;
    
    /**
     * @brief 清空播放列表
     * @param playlistId 播放列表ID
     * @return 操作是否成功
     */
    bool clearPlaylist(int playlistId);
    
    /**
     * @brief 更新播放列表统计信息
     * @param playlistId 播放列表ID
     * @return 操作是否成功
     */
    bool updatePlaylistStatistics(int playlistId);
    
    /**
     * @brief 获取最近播放的播放列表
     * @param count 数量限制
     * @return 播放列表列表
     */
    QList<Playlist> getRecentPlaylists(int count = 10) const;
    
    /**
     * @brief 获取收藏的播放列表
     * @return 播放列表列表
     */
    QList<Playlist> getFavoritePlaylists() const;
    
private:
    /**
     * @brief 从查询结果创建播放列表对象
     * @param query 查询对象
     * @return 播放列表对象
     */
    Playlist createPlaylistFromQuery(const QSqlQuery& query) const;
    
    /**
     * @brief 获取下一个排序序号
     * @param playlistId 播放列表ID
     * @return 排序序号
     */
    int getNextSortOrder(int playlistId) const;
    
    /**
     * @brief 重新排序播放列表中的歌曲
     * @param playlistId 播放列表ID
     * @return 操作是否成功
     */
    bool reorderPlaylistSongs(int playlistId);
};

#endif // PLAYLISTDAO_H