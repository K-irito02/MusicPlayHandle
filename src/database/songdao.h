#ifndef SONGDAO_H
#define SONGDAO_H

#include <QObject>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariantList>
#include <QStringList>
#include <QDateTime>
#include <QList>
#include "song.h"

class DatabaseManager;

/**
 * @brief 歌曲数据访问对象类
 * 
 * 负责处理歌曲相关的数据库操作，包括增删改查等CRUD操作
 */
class SongDao : public QObject
{
    Q_OBJECT
    
public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit SongDao(QObject* parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~SongDao();
    
    /**
     * @brief 插入歌曲
     * @param song 歌曲对象
     * @return 插入成功返回歌曲ID，失败返回-1
     */
    int insertSong(const Song& song);
    
    /**
     * @brief 批量插入歌曲
     * @param songs 歌曲列表
     * @return 插入成功的歌曲数量
     */
    int insertSongs(const QList<Song>& songs);
    
    /**
     * @brief 更新歌曲
     * @param song 歌曲对象
     * @return 更新是否成功
     */
    bool updateSong(const Song& song);
    
    /**
     * @brief 删除歌曲
     * @param songId 歌曲ID
     * @return 删除是否成功
     */
    bool deleteSong(int songId);
    
    /**
     * @brief 根据文件路径删除歌曲
     * @param filePath 文件路径
     * @return 删除是否成功
     */
    bool deleteSongByPath(const QString& filePath);
    
    /**
     * @brief 批量删除歌曲
     * @param songIds 歌曲ID列表
     * @return 删除成功的歌曲数量
     */
    int deleteSongs(const QList<int>& songIds);
    
    /**
     * @brief 根据ID获取歌曲
     * @param songId 歌曲ID
     * @return 歌曲对象，如果不存在返回无效对象
     */
    Song getSong(int songId);
    
    /**
     * @brief 根据文件路径获取歌曲
     * @param filePath 文件路径
     * @return 歌曲对象，如果不存在返回无效对象
     */
    Song getSongByPath(const QString& filePath);
    
    /**
     * @brief 获取所有歌曲
     * @return 歌曲列表
     */
    QList<Song> getAllSongs();
    
    /**
     * @brief 根据标签获取歌曲
     * @param tagId 标签ID
     * @return 歌曲列表
     */
    QList<Song> getSongsByTag(int tagId);
    
    /**
     * @brief 根据艺术家获取歌曲
     * @param artist 艺术家名称
     * @return 歌曲列表
     */
    QList<Song> getSongsByArtist(const QString& artist);
    
    /**
     * @brief 根据专辑获取歌曲
     * @param album 专辑名称
     * @return 歌曲列表
     */
    QList<Song> getSongsByAlbum(const QString& album);
    
    /**
     * @brief 获取收藏歌曲
     * @return 收藏歌曲列表
     */
    QList<Song> getFavoriteSongs();
    
    /**
     * @brief 搜索歌曲
     * @param keyword 搜索关键词
     * @param searchFields 搜索字段（标题、艺术家、专辑等）
     * @return 搜索结果列表
     */
    QList<Song> searchSongs(const QString& keyword, 
                           const QStringList& searchFields = QStringList());
    
    /**
     * @brief 分页获取歌曲
     * @param offset 偏移量
     * @param limit 限制数量
     * @param orderBy 排序字段
     * @param ascending 是否升序
     * @return 歌曲列表
     */
    QList<Song> getSongsPaginated(int offset, int limit, 
                                 const QString& orderBy = "title",
                                 bool ascending = true);
    
    /**
     * @brief 获取歌曲总数
     * @return 歌曲总数
     */
    int getSongCount();
    
    /**
     * @brief 获取标签下的歌曲数量
     * @param tagId 标签ID
     * @return 歌曲数量
     */
    int getSongCountByTag(int tagId);
    
    /**
     * @brief 检查歌曲是否存在
     * @param songId 歌曲ID
     * @return 是否存在
     */
    bool songExists(int songId);
    
    /**
     * @brief 检查文件路径是否已存在
     * @param filePath 文件路径
     * @return 是否存在
     */
    bool pathExists(const QString& filePath);
    
    /**
     * @brief 更新歌曲播放统计
     * @param songId 歌曲ID
     * @return 更新是否成功
     */
    bool updatePlayCount(int songId);
    
    /**
     * @brief 设置歌曲收藏状态
     * @param songId 歌曲ID
     * @param isFavorite 是否收藏
     * @return 更新是否成功
     */
    bool setFavorite(int songId, bool isFavorite);
    
    /**
     * @brief 更新歌曲可用性状态
     * @param songId 歌曲ID
     * @param isAvailable 是否可用
     * @return 更新是否成功
     */
    bool updateAvailability(int songId, bool isAvailable);
    
    /**
     * @brief 获取最近播放的歌曲
     * @param limit 限制数量
     * @return 最近播放的歌曲列表
     */
    QList<Song> getRecentlyPlayed(int limit = 50);
    
    /**
     * @brief 获取最近添加的歌曲
     * @param limit 限制数量
     * @return 最近添加的歌曲列表
     */
    QList<Song> getRecentlyAdded(int limit = 50);
    
    /**
     * @brief 获取播放次数最多的歌曲
     * @param limit 限制数量
     * @return 播放次数最多的歌曲列表
     */
    QList<Song> getMostPlayed(int limit = 50);
    
    /**
     * @brief 获取所有艺术家
     * @return 艺术家列表
     */
    QStringList getAllArtists();
    
    /**
     * @brief 获取所有专辑
     * @return 专辑列表
     */
    QStringList getAllAlbums();
    
    /**
     * @brief 获取文件格式统计
     * @return 格式统计映射（格式->数量）
     */
    QMap<QString, int> getFormatStatistics();
    
    /**
     * @brief 清理不可用的歌曲
     * @return 清理的歌曲数量
     */
    int cleanupUnavailableSongs();
    
    /**
     * @brief 获取最后一次错误信息
     * @return 错误信息
     */
    QString lastError() const;
    
    /**
     * @brief 将歌曲添加到标签
     * @param songId 歌曲ID
     * @param tagId 标签ID
     * @return 是否成功
     */
    bool addSongToTag(int songId, int tagId);
    /**
     * @brief 从标签移除歌曲
     * @param songId 歌曲ID
     * @param tagId 标签ID
     * @return 是否成功
     */
    bool removeSongFromTag(int songId, int tagId);
    /**
     * @brief 检查歌曲是否属于标签
     * @param songId 歌曲ID
     * @param tagId 标签ID
     * @return 是否属于
     */
    bool songHasTag(int songId, int tagId);
    
signals:
    /**
     * @brief 歌曲插入成功信号
     * @param song 插入的歌曲
     */
    void songInserted(const Song& song);
    
    /**
     * @brief 歌曲更新成功信号
     * @param song 更新的歌曲
     */
    void songUpdated(const Song& song);
    
    /**
     * @brief 歌曲删除成功信号
     * @param songId 删除的歌曲ID
     */
    void songDeleted(int songId);
    
    /**
     * @brief 数据库操作错误信号
     * @param error 错误信息
     */
    void databaseError(const QString& error);
    
private:
    /**
     * @brief 从查询结果创建Song对象
     * @param query 查询结果
     * @return Song对象
     */
    Song createSongFromQuery(const QSqlQuery& query);
    
    /**
     * @brief 构建搜索条件
     * @param keyword 搜索关键词
     * @param searchFields 搜索字段
     * @return SQL WHERE子句
     */
    QString buildSearchCondition(const QString& keyword, 
                                const QStringList& searchFields);
    
    /**
     * @brief 执行查询并返回歌曲列表
     * @param query 查询对象
     * @return 歌曲列表
     */
    QList<Song> executeQueryAndGetSongs(QSqlQuery& query);
    
    /**
     * @brief 记录错误信息
     * @param error 错误信息
     */
    void logError(const QString& error);
    
    /**
     * @brief 记录SQL错误
     * @param query 查询对象
     * @param operation 操作描述
     */
    void logSqlError(const QSqlQuery& query, const QString& operation);
    
    DatabaseManager* m_dbManager;   ///< 数据库管理器
    QString m_lastError;            ///< 最后一次错误信息
    
    // SQL语句常量
    struct SQLStatements {
        static const QString INSERT_SONG;
        static const QString UPDATE_SONG;
        static const QString DELETE_SONG;
        static const QString SELECT_SONG_BY_ID;
        static const QString SELECT_SONG_BY_PATH;
        static const QString SELECT_ALL_SONGS;
        static const QString SELECT_SONGS_BY_TAG;
        static const QString SELECT_SONGS_BY_ARTIST;
        static const QString SELECT_SONGS_BY_ALBUM;
        static const QString SELECT_FAVORITE_SONGS;
        static const QString SELECT_RECENT_PLAYED;
        static const QString SELECT_RECENT_ADDED;
        static const QString SELECT_MOST_PLAYED;
        static const QString COUNT_SONGS;
        static const QString COUNT_SONGS_BY_TAG;
        static const QString UPDATE_PLAY_COUNT;
        static const QString UPDATE_FAVORITE;
        static const QString UPDATE_AVAILABILITY;
        static const QString SELECT_ALL_ARTISTS;
        static const QString SELECT_ALL_ALBUMS;
        static const QString SELECT_FORMAT_STATISTICS;
        static const QString DELETE_UNAVAILABLE_SONGS;
    };
};

#endif // SONGDAO_H 