#ifndef PLAYHISTORY_H
#define PLAYHISTORY_H

#include <QString>
#include <QDateTime>
#include <QVariant>
#include <QJsonObject>
#include <QMetaType>

/**
 * @brief 播放历史数据模型类
 * 
 * 记录歌曲的播放历史信息，包括播放时间、播放时长、播放位置等
 */
class PlayHistory
{
public:
    /**
     * @brief 默认构造函数
     */
    PlayHistory();
    
    /**
     * @brief 带参数构造函数
     * @param songId 歌曲ID
     * @param playedAt 播放时间
     * @param playDuration 播放时长
     * @param playPosition 播放位置
     * @param isCompleted 是否完整播放
     */
    PlayHistory(int songId, const QDateTime& playedAt, qint64 playDuration = 0, 
                qint64 playPosition = 0, bool isCompleted = false);
    
    /**
     * @brief 拷贝构造函数
     * @param other 其他播放历史对象
     */
    PlayHistory(const PlayHistory& other);
    
    /**
     * @brief 赋值操作符
     * @param other 其他播放历史对象
     * @return 当前对象引用
     */
    PlayHistory& operator=(const PlayHistory& other);
    
    /**
     * @brief 相等操作符
     * @param other 其他播放历史对象
     * @return 是否相等
     */
    bool operator==(const PlayHistory& other) const;
    
    /**
     * @brief 不等操作符
     * @param other 其他播放历史对象
     * @return 是否不等
     */
    bool operator!=(const PlayHistory& other) const;
    
    // Getters
    int id() const { return m_id; }
    int songId() const { return m_songId; }
    QDateTime playedAt() const { return m_playedAt; }
    qint64 playDuration() const { return m_playDuration; }
    qint64 playPosition() const { return m_playPosition; }
    bool isCompleted() const { return m_isCompleted; }
    
    // Setters
    void setId(int id) { m_id = id; }
    void setSongId(int songId) { m_songId = songId; }
    void setPlayedAt(const QDateTime& playedAt) { m_playedAt = playedAt; }
    void setPlayDuration(qint64 playDuration) { m_playDuration = playDuration; }
    void setPlayPosition(qint64 playPosition) { m_playPosition = playPosition; }
    void setIsCompleted(bool isCompleted) { m_isCompleted = isCompleted; }
    
    /**
     * @brief 检查播放历史是否有效
     * @return 是否有效
     */
    bool isValid() const;
    
    /**
     * @brief 获取格式化的播放时间字符串
     * @return 格式化的播放时间
     */
    QString formattedPlayedAt() const;
    
    /**
     * @brief 获取格式化的播放时长字符串
     * @return 格式化的播放时长
     */
    QString formattedPlayDuration() const;
    
    /**
     * @brief 获取格式化的播放位置字符串
     * @return 格式化的播放位置
     */
    QString formattedPlayPosition() const;
    
    /**
     * @brief 获取播放完成度百分比
     * @param songDuration 歌曲总时长
     * @return 完成度百分比(0-100)
     */
    int completionPercentage(qint64 songDuration) const;
    
    /**
     * @brief 检查是否为有效播放（播放时长超过阈值）
     * @param thresholdMs 阈值毫秒数
     * @return 是否为有效播放
     */
    bool isValidPlay(qint64 thresholdMs = 30000) const; // 默认30秒
    
    /**
     * @brief 从JSON对象创建PlayHistory实例
     * @param json JSON对象
     * @return PlayHistory实例
     */
    static PlayHistory fromJson(const QJsonObject& json);
    
    /**
     * @brief 转换为JSON对象
     * @return JSON对象
     */
    QJsonObject toJson() const;
    
    /**
     * @brief 从QVariantMap创建PlayHistory实例
     * @param map QVariantMap
     * @return PlayHistory实例
     */
    static PlayHistory fromVariantMap(const QVariantMap& map);
    
    /**
     * @brief 转换为QVariantMap
     * @return QVariantMap
     */
    QVariantMap toVariantMap() const;
    
    /**
     * @brief 清空数据
     */
    void clear();
    
    /**
     * @brief 判断是否为空
     * @return 是否为空
     */
    bool isEmpty() const;
    
    /**
     * @brief 获取调试字符串
     * @return 调试字符串
     */
    QString toString() const;
    
private:
    int m_id;                           ///< 播放历史ID
    int m_songId;                       ///< 歌曲ID
    QDateTime m_playedAt;               ///< 播放时间
    qint64 m_playDuration;              ///< 播放时长(毫秒)
    qint64 m_playPosition;              ///< 播放位置(毫秒)
    bool m_isCompleted;                 ///< 是否完整播放
    
    /**
     * @brief 格式化时间
     * @param milliseconds 毫秒
     * @return 格式化的时间字符串
     */
    QString formatTime(qint64 milliseconds) const;
};

// 使PlayHistory可以在QVariant中使用
Q_DECLARE_METATYPE(PlayHistory)

#endif // PLAYHISTORY_H 