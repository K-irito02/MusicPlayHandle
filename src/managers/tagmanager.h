#ifndef TAGMANAGER_H
#define TAGMANAGER_H

#include <QObject>
#include <QMutex>
#include <QHash>
#include <QList>
#include <QSet>
#include <QPixmap>
#include <QUrl>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <QDateTime>

// 前向声明
class TagDAO;
class SongDao;
class Logger;

#include "../models/song.h"
#include "../models/tag.h"
#include "../database/tagdao.h"
#include "../database/songdao.h"

// 标签操作类型
enum class TagOperation {
    Create,
    Update,
    Delete,
    AddSong,
    RemoveSong,
    MoveSong,
    CopySong
};

// 标签操作结果
struct TagOperationResult {
    bool success;
    QString message;
    QVariant data;
    
    TagOperationResult(bool s = false, const QString& msg = QString(), const QVariant& d = QVariant())
        : success(s), message(msg), data(d) {}
};

// 标签统计信息
struct TagStatistics {
    int totalTags;
    int totalSongs;
    int averageSongsPerTag;
    int maxSongsInTag;
    int minSongsInTag;
    QString mostPopularTag;
    QString leastPopularTag;
    QMap<QString, int> tagSongCounts;
    
    TagStatistics() : totalTags(0), totalSongs(0), averageSongsPerTag(0), 
                     maxSongsInTag(0), minSongsInTag(0) {}
};

// 标签过滤器
class TagFilter {
public:
    TagFilter();
    
    // 过滤条件
    void setNameFilter(const QString& name);
    void setCreatedDateRange(const QDateTime& start, const QDateTime& end);
    void setMinSongCount(int count);
    void setMaxSongCount(int count);
    void setColorFilter(const QColor& color);
    void setSystemTagsOnly(bool systemOnly);
    void setUserTagsOnly(bool userOnly);
    
    // 应用过滤
    QList<Tag> applyFilter(const QList<Tag>& tags) const;
    
    // 清除过滤
    void clearFilter();
    
    // 获取过滤条件
    QString nameFilter() const { return m_nameFilter; }
    QDateTime startDate() const { return m_startDate; }
    QDateTime endDate() const { return m_endDate; }
    int minSongCount() const { return m_minSongCount; }
    int maxSongCount() const { return m_maxSongCount; }
    QColor colorFilter() const { return m_colorFilter; }
    bool systemTagsOnly() const { return m_systemTagsOnly; }
    bool userTagsOnly() const { return m_userTagsOnly; }
    
private:
    QString m_nameFilter;
    QDateTime m_startDate;
    QDateTime m_endDate;
    int m_minSongCount;
    int m_maxSongCount;
    QColor m_colorFilter;
    bool m_systemTagsOnly;
    bool m_userTagsOnly;
    
    bool matchesFilter(const Tag& tag) const;
};

// 标签管理器
class TagManager : public QObject {
    Q_OBJECT
    
public:
    explicit TagManager(QObject* parent = nullptr);
    ~TagManager();
    
    // 单例模式
    static TagManager* instance();
    static void cleanup();
    
    // 初始化
    bool initialize();
    void shutdown();
    
    // 标签CRUD操作
    TagOperationResult createTag(const QString& name, const QString& description = QString(), 
                                const QColor& color = QColor(), const QPixmap& icon = QPixmap());
    TagOperationResult updateTag(int tagId, const QString& name, const QString& description = QString(),
                                const QColor& color = QColor(), const QPixmap& icon = QPixmap());
    TagOperationResult deleteTag(int tagId, bool deleteSongs = false);
    
    // 标签查询
    Tag getTag(int tagId) const;
    Tag getTagByName(const QString& name) const;
    QList<Tag> getAllTags() const;
    QList<Tag> getSystemTags() const;
    QList<Tag> getUserTags() const;
    QList<Tag> getTagsWithFilter(const TagFilter& filter) const;
    
    // 默认标签管理
    Tag getDefaultTag() const;
    Tag getAllSongsTag() const;
    bool isSystemTag(int tagId) const;
    bool isSystemTag(const QString& name) const;
    
    // 歌曲-标签关联
    TagOperationResult addSongToTag(int songId, int tagId);
    TagOperationResult removeSongFromTag(int songId, int tagId);
    TagOperationResult moveSongToTag(int songId, int fromTagId, int toTagId);
    TagOperationResult copySongToTag(int songId, int fromTagId, int toTagId);
    
    // 批量操作
    TagOperationResult addSongsToTag(const QList<int>& songIds, int tagId);
    TagOperationResult removeSongsFromTag(const QList<int>& songIds, int tagId);
    TagOperationResult moveSongsToTag(const QList<int>& songIds, int fromTagId, int toTagId);
    TagOperationResult copySongsToTag(const QList<int>& songIds, int fromTagId, int toTagId);
    
    // 标签下的歌曲
    QList<Song> getSongsInTag(int tagId) const;
    QList<Song> getSongsInTag(const QString& tagName) const;
    int getSongCountInTag(int tagId) const;
    int getSongCountInTag(const QString& tagName) const;
    
    // 歌曲所属标签
    QList<Tag> getTagsForSong(int songId) const;
    bool isSongInTag(int songId, int tagId) const;
    bool isSongInTag(int songId, const QString& tagName) const;
    
    // 标签验证
    bool tagExists(const QString& name) const;
    bool tagExists(int tagId) const;
    bool canDeleteTag(int tagId) const;
    bool canUpdateTag(int tagId) const;
    
    // 标签排序
    enum class SortBy {
        Name,
        CreatedDate,
        ModifiedDate,
        SongCount,
        Color
    };
    
    enum class SortOrder {
        Ascending,
        Descending
    };
    
    QList<Tag> getSortedTags(SortBy sortBy, SortOrder order = SortOrder::Ascending) const;
    
    // 标签搜索
    QList<Tag> searchTags(const QString& keyword) const;
    QList<Tag> searchTagsByColor(const QColor& color) const;
    QList<Tag> searchTagsBySongCount(int minCount, int maxCount) const;
    
    // 标签统计
    TagStatistics getTagStatistics() const;
    int getTotalTagCount() const;
    int getTotalSongCount() const;
    QMap<QString, int> getTagSongCounts() const;
    
    // 标签导入导出
    bool exportTagsToJson(const QString& filePath) const;
    bool importTagsFromJson(const QString& filePath);
    QJsonObject exportTagToJson(int tagId) const;
    Tag importTagFromJson(const QJsonObject& json);
    
    // 标签图标管理
    bool setTagIcon(int tagId, const QPixmap& icon);
    QPixmap getTagIcon(int tagId) const;
    bool hasTagIcon(int tagId) const;
    bool removeTagIcon(int tagId);
    
    // 标签颜色管理
    bool setTagColor(int tagId, const QColor& color);
    QColor getTagColor(int tagId) const;
    QList<QColor> getUsedColors() const;
    QColor suggestColor() const;
    
    // 缓存管理
    void refreshCache();
    void clearCache();
    void enableCache(bool enabled);
    bool isCacheEnabled() const;
    
    // 撤销/重做支持
    void enableUndoRedo(bool enabled);
    bool canUndo() const;
    bool canRedo() const;
    void undo();
    void redo();
    void clearUndoRedoStack();
    
signals:
    // 标签变化信号
    void tagCreated(const Tag& tag);
    void tagUpdated(const Tag& tag);
    void tagDeleted(int tagId, const QString& name);
    
    // 歌曲-标签关联信号
    void songAddedToTag(int songId, int tagId);
    void songRemovedFromTag(int songId, int tagId);
    void songMovedToTag(int songId, int fromTagId, int toTagId);
    void songCopiedToTag(int songId, int fromTagId, int toTagId);
    
    // 批量操作信号
    void batchOperationStarted(TagOperation operation);
    void batchOperationProgress(int current, int total);
    void batchOperationFinished(TagOperation operation, bool success);
    
    // 错误信号
    void errorOccurred(const QString& error);
    
    // 统计信号
    void statisticsUpdated(const TagStatistics& stats);
    
private slots:
    void onTagChanged(int tagId);
    void onSongChanged(int songId);
    void updateStatistics();
    void cleanupOrphanedTags();
    
private:
    // 单例实例
    static TagManager* m_instance;
    static QMutex m_instanceMutex;
    
    // 数据访问对象
    TagDAO* m_tagDao;
    SongDao* m_songDao;
    
    // 缓存
    mutable QHash<int, Tag> m_tagCache;
    mutable QHash<QString, Tag> m_tagNameCache;
    mutable QHash<int, QList<Song>> m_tagSongsCache;
    mutable QHash<int, QList<Tag>> m_songTagsCache;
    mutable QMutex m_cacheMutex;
    bool m_cacheEnabled;
    
    // 系统标签
    int m_defaultTagId;
    int m_allSongsTagId;
    QSet<int> m_systemTagIds;
    
    // 统计信息
    TagStatistics m_statistics;
    QTimer* m_statisticsTimer;
    
    // 撤销/重做
    struct UndoRedoCommand {
        TagOperation operation;
        QVariant data;
        QString description;
    };
    
    QList<UndoRedoCommand> m_undoStack;
    QList<UndoRedoCommand> m_redoStack;
    bool m_undoRedoEnabled;
    int m_maxUndoRedoSize;
    
    // 线程安全
    mutable QMutex m_mutex;
    
    // 初始化方法
    bool initializeSystemTags();
    bool loadTagsFromDatabase();
    
    // 缓存管理
    void addTagToCache(const Tag& tag);
    void removeTagFromCache(int tagId);
    void updateTagInCache(const Tag& tag);
    void addSongTagsToCache(int songId, const QList<Tag>& tags);
    void removeSongTagsFromCache(int songId);
    void addTagSongsToCache(int tagId, const QList<Song>& songs);
    void removeTagSongsFromCache(int tagId);
    
    // 验证方法
    bool validateTagName(const QString& name) const;
    bool validateTagColor(const QColor& color) const;
    bool validateSongId(int songId) const;
    bool validateTagId(int tagId) const;
    
    // 内部操作
    TagOperationResult createTagInternal(const Tag& tag);
    TagOperationResult updateTagInternal(const Tag& tag);
    TagOperationResult deleteTagInternal(int tagId, bool deleteSongs);
    
    // 撤销/重做实现
    void addUndoCommand(const UndoRedoCommand& command);
    void executeUndoCommand(const UndoRedoCommand& command);
    void executeRedoCommand(const UndoRedoCommand& command);
    
    // 系统标签创建
    Tag createSystemTag(const QString& name, const QString& description, 
                       const QColor& color, bool isDeletable = false);
    
    // 统计计算
    void calculateStatistics();
    
    // 日志记录
    void logTagOperation(TagOperation operation, const QString& details);
    void logError(const QString& error);
    void logInfo(const QString& message);
    
    // 工具方法
    QColor generateRandomColor() const;
    QString generateTagDescription(const QString& name) const;
    bool isValidTagName(const QString& name) const;
    QString sanitizeTagName(const QString& name) const;
};

#endif // TAGMANAGER_H
