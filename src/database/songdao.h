#ifndef SONGDAO_H
#define SONGDAO_H

#include "basedao.h"
#include "../models/song.h"
#include <QList>
#include <QDateTime>

/**
 * @brief 歌曲数据访问对象
 * 
 * 提供歌曲相关的数据库操作
 */
class SongDao : public BaseDao
{
    Q_OBJECT

public:
    explicit SongDao(QObject* parent = nullptr);
    
    /**
     * @brief 添加歌曲
     * @param song 歌曲对象
     * @return 添加成功返回歌曲ID，失败返回-1
     */
    int addSong(const Song& song);
    
    /**
     * @brief 根据ID获取歌曲
     * @param id 歌曲ID
     * @return 歌曲对象，如果不存在返回空对象
     */
    Song getSongById(int id);
    
    /**
     * @brief 根据文件路径获取歌曲
     * @param filePath 文件路径
     * @return 歌曲对象，如果不存在返回空对象
     */
    Song getSongByPath(const QString& filePath);
    
    /**
     * @brief 获取所有歌曲
     * @return 歌曲列表
     */
    QList<Song> getAllSongs();
    
    /**
     * @brief 根据标题搜索歌曲
     * @param title 标题关键词
     * @return 匹配的歌曲列表
     */
    QList<Song> searchByTitle(const QString& title);
    
    /**
     * @brief 根据艺术家搜索歌曲
     * @param artist 艺术家关键词
     * @return 匹配的歌曲列表
     */
    QList<Song> searchByArtist(const QString& artist);
    
    /**
     * @brief 根据标签搜索歌曲
     * @param tag 标签名称
     * @return 匹配的歌曲列表
     */
    QList<Song> searchByTag(const QString& tag);
    
    /**
     * @brief 更新歌曲信息
     * @param song 歌曲对象
     * @return 更新是否成功
     */
    bool updateSong(const Song& song);
    
    /**
     * @brief 删除歌曲
     * @param id 歌曲ID
     * @return 删除是否成功
     */
    bool deleteSong(int id);
    
    /**
     * @brief 更新播放次数
     * @param id 歌曲ID
     * @return 更新是否成功
     */
    bool incrementPlayCount(int id);
    
    /**
     * @brief 更新最后播放时间
     * @param id 歌曲ID
     * @param lastPlayed 最后播放时间
     * @return 更新是否成功
     */
    bool updateLastPlayed(int id, const QDateTime& lastPlayed = QDateTime::currentDateTime());
    
    /**
     * @brief 更新歌曲评分
     * @param id 歌曲ID
     * @param rating 评分(0-5)
     * @return 更新是否成功
     */
    bool updateRating(int id, int rating);
    
    /**
     * @brief 检查歌曲是否存在
     * @param filePath 文件路径
     * @return 是否存在
     */
    bool songExists(const QString& filePath);
    
    /**
     * @brief 获取歌曲总数
     * @return 歌曲总数
     */
    int getSongCount();
    
    /**
     * @brief 根据标签ID获取歌曲列表
     * @param tagId 标签ID
     * @return 歌曲列表
     */
    QList<Song> getSongsByTag(int tagId);
    
    /**
     * @brief 从标签中移除歌曲
     * @param songId 歌曲ID
     * @param tagId 标签ID
     * @return 操作是否成功
     */
    bool removeSongFromTag(int songId, int tagId);
    
    /**
     * @brief 将歌曲添加到标签
     * @param songId 歌曲ID
     * @param tagId 标签ID
     * @return 操作是否成功
     */
    bool addSongToTag(int songId, int tagId);
    
    /**
     * @brief 检查歌曲是否属于某个标签
     * @param songId 歌曲ID
     * @param tagId 标签ID
     * @return 是否属于该标签
     */
    bool songHasTag(int songId, int tagId);
    
    /**
     * @brief 批量插入歌曲
     * @param songs 歌曲列表
     * @return 成功插入的歌曲数量
     */
    int insertSongs(const QList<Song>& songs);

    /**
     * @brief 从查询结果创建歌曲对象
     * @param query 查询结果
     * @return 歌曲对象
     */
    Song createSongFromQuery(const QSqlQuery& query);

private:
};

#endif // SONGDAO_H