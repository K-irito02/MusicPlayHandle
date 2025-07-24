#ifndef SONG_H
#define SONG_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QVariant>
#include <QPixmap>
#include <QJsonObject>
#include <QMetaType>
#include <QStringList>

/**
 * @brief 歌曲数据模型类
 * 
 * 表示一首歌曲的所有信息，包括基本信息、元数据、播放统计等
 */
class Song
{
public:
    /**
     * @brief 默认构造函数
     */
    Song();
    
    /**
     * @brief 带参数构造函数
     * @param filePath 文件路径
     * @param title 歌曲标题
     * @param artist 艺术家
     * @param album 专辑
     */
    Song(const QString& filePath, const QString& title = QString(), 
         const QString& artist = QString(), const QString& album = QString());
    
    /**
     * @brief 拷贝构造函数
     * @param other 其他歌曲对象
     */
    Song(const Song& other);
    
    /**
     * @brief 赋值操作符
     * @param other 其他歌曲对象
     * @return 当前对象引用
     */
    Song& operator=(const Song& other);
    
    /**
     * @brief 相等操作符
     * @param other 其他歌曲对象
     * @return 是否相等
     */
    bool operator==(const Song& other) const;
    
    /**
     * @brief 不等操作符
     * @param other 其他歌曲对象
     * @return 是否不等
     */
    bool operator!=(const Song& other) const;
    
    // Getters
    int id() const { return m_id; }
    QString filePath() const { return m_filePath; }
    QString fileName() const { return m_fileName; }
    QString title() const { return m_title; }
    QString artist() const { return m_artist; }
    QString album() const { return m_album; }
    qint64 duration() const { return m_duration; }
    qint64 fileSize() const { return m_fileSize; }
    int bitRate() const { return m_bitRate; }
    int sampleRate() const { return m_sampleRate; }
    int channels() const { return m_channels; }
    QString fileFormat() const { return m_fileFormat; }
    QString coverPath() const { return m_coverPath; }
    bool hasLyrics() const { return m_hasLyrics; }
    QString lyricsPath() const { return m_lyricsPath; }
    int playCount() const { return m_playCount; }
    QDateTime lastPlayedTime() const { return m_lastPlayedTime; }
    QDateTime dateAdded() const { return m_dateAdded; }
    QDateTime dateModified() const { return m_dateModified; }
    bool isFavorite() const { return m_isFavorite; }
    bool isAvailable() const { return m_isAvailable; }
    QDateTime createdAt() const { return m_createdAt; }
    QDateTime updatedAt() const { return m_updatedAt; }
    QStringList tags() const { return m_tags; }
    int rating() const { return m_rating; }
    QString genre() const { return m_genre; }
    void setGenre(const QString& genre) { m_genre = genre; }
    int year() const { return m_year; }
    void setYear(int year) { m_year = year; }
    void setYear(const QString& year) { m_year = year.toInt(); }
    
    // Setters
    void setId(int id) { m_id = id; }
    void setFilePath(const QString& filePath);
    void setFileName(const QString& fileName) { m_fileName = fileName; }
    void setTitle(const QString& title) { m_title = title; }
    void setArtist(const QString& artist) { m_artist = artist; }
    void setAlbum(const QString& album) { m_album = album; }
    void setDuration(qint64 duration) { m_duration = duration; }
    void setFileSize(qint64 fileSize) { m_fileSize = fileSize; }
    void setBitRate(int bitRate) { m_bitRate = bitRate; }
    void setSampleRate(int sampleRate) { m_sampleRate = sampleRate; }
    void setChannels(int channels) { m_channels = channels; }
    void setFileFormat(const QString& fileFormat) { m_fileFormat = fileFormat; }
    void setCoverPath(const QString& coverPath) { m_coverPath = coverPath; }
    void setHasLyrics(bool hasLyrics) { m_hasLyrics = hasLyrics; }
    void setLyricsPath(const QString& lyricsPath) { m_lyricsPath = lyricsPath; }
    void setPlayCount(int playCount) { m_playCount = playCount; }
    void setLastPlayedTime(const QDateTime& lastPlayedTime) { m_lastPlayedTime = lastPlayedTime; }
    void setDateAdded(const QDateTime& dateAdded) { m_dateAdded = dateAdded; }
    void setDateModified(const QDateTime& dateModified) { m_dateModified = dateModified; }
    void setIsFavorite(bool isFavorite) { m_isFavorite = isFavorite; }
    void setIsAvailable(bool isAvailable) { m_isAvailable = isAvailable; }
    void setCreatedAt(const QDateTime& createdAt) { m_createdAt = createdAt; }
    void setUpdatedAt(const QDateTime& updatedAt) { m_updatedAt = updatedAt; }
    void setTags(const QStringList& tags) { m_tags = tags; }
    void setRating(int rating) { m_rating = rating; }
    
    /**
     * @brief 检查歌曲是否有效
     * @return 是否有效
     */
    bool isValid() const;
    
    /**
     * @brief 获取显示名称
     * @return 显示名称
     */
    QString displayName() const;
    
    /**
     * @brief 获取格式化的时长字符串
     * @return 格式化的时长
     */
    QString formattedDuration() const;
    
    /**
     * @brief 获取格式化的文件大小字符串
     * @return 格式化的文件大小
     */
    QString formattedFileSize() const;
    
    /**
     * @brief 更新播放统计
     */
    void updatePlayCount();
    
    /**
     * @brief 设置为收藏
     */
    void setFavorite();
    
    /**
     * @brief 取消收藏
     */
    void unsetFavorite();
    
    /**
     * @brief 切换收藏状态
     */
    void toggleFavorite();
    
    /**
     * @brief 从JSON对象创建Song实例
     * @param json JSON对象
     * @return Song实例
     */
    static Song fromJson(const QJsonObject& json);
    
    /**
     * @brief 转换为JSON对象
     * @return JSON对象
     */
    QJsonObject toJson() const;
    
    /**
     * @brief 从QVariantMap创建Song实例
     * @param map QVariantMap
     * @return Song实例
     */
    static Song fromVariantMap(const QVariantMap& map);
    
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
    
    /**
     * @brief 从文件路径快速构造 Song 对象（自动提取文件名、大小、格式、添加时间等基础信息）
     * @param filePath 文件路径
     * @return Song 实例
     */
    static Song fromFile(const QString& filePath);
    
    // 元数据解析方法
    static void extractBasicMetadata(Song& song, const QString& filePath);
    static void extractAdvancedMetadata(Song& song, const QString& filePath);
    static bool extractFFmpegMetadata(Song& song, const QString& filePath);
    static QPixmap extractCoverArt(const QString& filePath, const QSize& size = QSize(300, 300));
    
    // 元数据获取方法
    static QString getTitleFromMetadata(const QString& filePath);
    static QString getArtistFromMetadata(const QString& filePath);
    static QString getAlbumFromMetadata(const QString& filePath);
    static QString getYearFromMetadata(const QString& filePath);
    static QString getGenreFromMetadata(const QString& filePath);
    
private:
    int m_id;                           ///< 歌曲ID
    QString m_filePath;                 ///< 文件完整路径
    QString m_fileName;                 ///< 文件名
    QString m_title;                    ///< 歌曲标题
    QString m_artist;                   ///< 艺术家
    QString m_album;                    ///< 专辑
    qint64 m_duration;                  ///< 时长(毫秒)
    qint64 m_fileSize;                  ///< 文件大小(字节)
    int m_bitRate;                      ///< 比特率
    int m_sampleRate;                   ///< 采样率
    int m_channels;                     ///< 声道数
    QString m_fileFormat;               ///< 文件格式
    QString m_coverPath;                ///< 封面图片路径
    bool m_hasLyrics;                   ///< 是否有歌词
    QString m_lyricsPath;               ///< 歌词文件路径
    int m_playCount;                    ///< 播放次数
    QDateTime m_lastPlayedTime;         ///< 最后播放时间
    QDateTime m_dateAdded;              ///< 添加时间
    QDateTime m_dateModified;           ///< 文件修改时间
    bool m_isFavorite;                  ///< 是否收藏
    bool m_isAvailable;                 ///< 文件是否可用
    QDateTime m_createdAt;              ///< 创建时间
    QDateTime m_updatedAt;              ///< 更新时间
    QStringList m_tags;                 ///< 标签列表
    int m_rating;                       ///< 评分(0-5)
    QString m_genre;
    int m_year;
    
    /**
     * @brief 从文件路径提取文件名
     * @param filePath 文件路径
     * @return 文件名
     */
    QString extractFileName(const QString& filePath) const;
    

    
    /**
     * @brief 格式化时间
     * @param milliseconds 毫秒
     * @return 格式化的时间字符串
     */
    QString formatTime(qint64 milliseconds) const;
    
    /**
     * @brief 格式化文件大小
     * @param bytes 字节数
     * @return 格式化的大小字符串
     */
    QString formatFileSize(qint64 bytes) const;
};

// 使Song可以在QVariant中使用
Q_DECLARE_METATYPE(Song)

#endif // SONG_H