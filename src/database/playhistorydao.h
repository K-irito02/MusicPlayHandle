#ifndef PLAYHISTORYDAO_H
#define PLAYHISTORYDAO_H

#include <QObject>
#include <QSqlQuery>
#include <QSqlError>
#include <QList>
#include <QDateTime>
#include <QMutex>

#include "../models/playhistory.h"
#include "../models/song.h"
#include "basedao.h"

/**
 * @brief 播放历史数据访问对象
 * 
 * 负责播放历史记录的增删改查操作
 * 支持最近播放列表的获取和管理
 */
class PlayHistoryDao : public BaseDao
{
    Q_OBJECT

public:
    explicit PlayHistoryDao(QObject* parent = nullptr);
    ~PlayHistoryDao();

    /**
     * @brief 添加播放记录
     * @param songId 歌曲ID
     * @param playedAt 播放时间（可选，默认为当前时间）
     * @return 是否成功
     */
    bool addPlayRecord(int songId, const QDateTime& playedAt = QDateTime::currentDateTime());

    /**
     * @brief 获取最近播放的歌曲列表
     * @param limit 限制数量（默认100）
     * @return 最近播放的歌曲列表
     */
    QList<Song> getRecentPlayedSongs(int limit = 100);

    /**
     * @brief 获取指定歌曲的播放历史
     * @param songId 歌曲ID
     * @param limit 限制数量
     * @return 播放历史列表
     */
    QList<PlayHistory> getSongPlayHistory(int songId, int limit = 50);

    /**
     * @brief 获取所有播放历史记录
     * @param limit 限制数量
     * @return 播放历史列表
     */
    QList<PlayHistory> getAllPlayHistory(int limit = 1000);

    /**
     * @brief 删除指定歌曲的播放历史
     * @param songId 歌曲ID
     * @return 是否成功
     */
    bool deleteSongPlayHistory(int songId);

    /**
     * @brief 删除指定时间之前的播放历史
     * @param beforeTime 时间点
     * @return 是否成功
     */
    bool deletePlayHistoryBefore(const QDateTime& beforeTime);

    /**
     * @brief 清空所有播放历史
     * @return 是否成功
     */
    bool clearAllPlayHistory();

    /**
     * @brief 获取播放历史统计信息
     * @return 统计信息
     */
    struct PlayHistoryStats {
        int totalRecords;
        int uniqueSongs;
        QDateTime firstPlayTime;
        QDateTime lastPlayTime;
        QString mostPlayedSong;
        int mostPlayedCount;
    };
    PlayHistoryStats getPlayHistoryStats();

    /**
     * @brief 检查歌曲是否有播放历史
     * @param songId 歌曲ID
     * @return 是否有播放历史
     */
    bool hasPlayHistory(int songId);

    /**
     * @brief 获取歌曲的播放次数
     * @param songId 歌曲ID
     * @return 播放次数
     */
    int getSongPlayCount(int songId);

    /**
     * @brief 获取最近播放时间
     * @param songId 歌曲ID
     * @return 最近播放时间
     */
    QDateTime getLastPlayTime(int songId);

    /**
     * @brief 批量添加播放记录
     * @param songIds 歌曲ID列表
     * @return 成功添加的记录数
     */
    int batchAddPlayRecords(const QList<int>& songIds);

    /**
     * @brief 获取播放历史记录数量
     * @return 记录数量
     */
    int getPlayHistoryCount();

    /**
     * @brief 获取唯一歌曲数量
     * @return 唯一歌曲数量
     */
    int getUniqueSongCount();

private:
    mutable QMutex m_mutex;

    /**
     * @brief 从查询结果创建PlayHistory对象
     * @param query 查询结果
     * @return PlayHistory对象
     */
    PlayHistory createPlayHistoryFromQuery(const QSqlQuery& query);

    /**
     * @brief 从查询结果创建Song对象
     * @param query 查询结果
     * @return Song对象
     */
    Song createSongFromQuery(const QSqlQuery& query);

    /**
     * @brief 清理重复记录，保留最新的
     * @param songId 歌曲ID
     * @return 是否成功
     */
    bool cleanupDuplicateRecords(int songId);

    /**
     * @brief 限制播放历史记录数量
     * @param maxRecords 最大记录数
     * @return 是否成功
     */
    bool limitPlayHistoryRecords(int maxRecords = 1000);
};

#endif // PLAYHISTORYDAO_H 