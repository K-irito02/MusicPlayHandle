#ifndef PLAYLISTMANAGER_H
#define PLAYLISTMANAGER_H

#include <QObject>
#include <QMutex>
#include <QList>
#include <QQueue>
#include <QStack>
#include <QHash>
#include <QSet>
#include <QTimer>
#include <QJsonObject>
#include <QJsonArray>
#include <QRandomGenerator>
#include <QDateTime>

#include "../models/song.h"
#include "../models/playlist.h"
#include "../database/songdao.h"

// 前向声明
class PlaylistDao;

// 播放模式
enum class PlayMode {
    Sequential,     // 顺序播放
    Loop,          // 列表循环
    SingleLoop,    // 单曲循环
    Random,        // 随机播放
    Shuffle        // 洗牌播放
};

// 重复播放模式
enum class RepeatMode {
    NoRepeat,      // 不重复
    RepeatOne,     // 单曲循环
    RepeatAll      // 列表循环
};

// 导出格式
enum class ExportFormat {
    M3U,           // M3U 格式
    PLS,           // PLS 格式
    JSON           // JSON 格式
};

// 播放列表状态
enum class PlaylistState {
    Stopped,
    Playing,
    Paused,
    Loading,
    Error
};

// 播放列表操作类型
enum class PlaylistOperation {
    Create,
    Update,
    Delete,
    AddSong,
    RemoveSong,
    MoveSong,
    ClearSongs,
    Shuffle,
    Sort
};

// 播放列表操作结果
struct PlaylistOperationResult {
    bool success;
    QString message;
    QString errorMessage;
    QVariant data;
    
    PlaylistOperationResult(bool s = false, const QString& msg = QString(), const QVariant& d = QVariant())
        : success(s), message(msg), errorMessage(msg), data(d) {}
};

// 播放统计信息
struct PlayStatistics {
    int totalPlaylists;
    int totalSongs;
    int totalPlayTime;
    int averagePlaylistLength;
    int longestPlaylist;
    int shortestPlaylist;
    QString mostPlayedPlaylist;
    QString recentPlaylist;
    QMap<QString, int> playlistPlayCounts;
    
    PlayStatistics() : totalPlaylists(0), totalSongs(0), totalPlayTime(0),
                      averagePlaylistLength(0), longestPlaylist(0), shortestPlaylist(0) {}
};

// 播放队列项
struct QueueItem {
    Song song;
    int playlistId;
    int originalIndex;
    qint64 timestamp;
    bool fromHistory;
    
    QueueItem() : playlistId(-1), originalIndex(-1), timestamp(0), fromHistory(false) {}
    QueueItem(const Song& s, int pId = -1, int idx = -1)
        : song(s), playlistId(pId), originalIndex(idx), timestamp(QDateTime::currentMSecsSinceEpoch()), fromHistory(false) {}
};

// 播放列表管理器
class PlaylistManager : public QObject {
    Q_OBJECT
    
public:
    explicit PlaylistManager(QObject* parent = nullptr);
    ~PlaylistManager();
    
    // 单例模式
    static PlaylistManager* instance();
    static void cleanup();
    
    // 初始化
    bool initialize();
    void shutdown();
    
    // 播放列表CRUD操作
    PlaylistOperationResult createPlaylist(const QString& name, const QString& description = QString());
    PlaylistOperationResult updatePlaylist(int playlistId, const QString& name, const QString& description = QString());
    PlaylistOperationResult deletePlaylist(int playlistId);
    PlaylistOperationResult duplicatePlaylist(int playlistId, const QString& newName);
    
    // 播放列表查询
    Playlist getPlaylist(int playlistId) const;
    Playlist getPlaylistByName(const QString& name) const;
    QList<Playlist> getAllPlaylists() const;
    QList<Playlist> getRecentPlaylists(int count = 10) const;
    QList<Playlist> getFavoritePlaylists() const;
    
    // 当前播放列表
    void setCurrentPlaylist(int playlistId);
    void setCurrentPlaylist(const Playlist& playlist);
    Playlist getCurrentPlaylist() const;
    int getCurrentPlaylistId() const;
    bool hasCurrentPlaylist() const;
    
    // 播放列表中的歌曲管理
    PlaylistOperationResult addSongToPlaylist(int playlistId, const Song& song);
    PlaylistOperationResult addSongsToPlaylist(int playlistId, const QList<Song>& songs);
    PlaylistOperationResult removeSongFromPlaylist(int playlistId, int songIndex);
    PlaylistOperationResult removeSongsFromPlaylist(int playlistId, const QList<int>& indices);
    PlaylistOperationResult moveSongInPlaylist(int playlistId, int fromIndex, int toIndex);
    PlaylistOperationResult clearPlaylist(int playlistId);
    
    // 播放列表歌曲查询
    QList<Song> getPlaylistSongs(int playlistId) const;
    int getPlaylistSongCount(int playlistId) const;
    Song getPlaylistSong(int playlistId, int index) const;
    int findSongInPlaylist(int playlistId, const Song& song) const;
    bool isPlaylistEmpty(int playlistId) const;
    
    // 播放控制
    void play();
    void pause();
    void stop();
    void next();
    void previous();
    void playAt(int index);
    void playPlaylist(int playlistId);
    void playSong(const Song& song);
    
    // 重复播放模式
    void setRepeatMode(RepeatMode mode);
    RepeatMode getRepeatMode() const;
    
    // 洗牌模式
    void setShuffleMode(bool enabled);
    bool isShuffleMode() const;
    
    // 导入导出
    bool exportPlaylist(int playlistId, const QString& filePath, ExportFormat format);
    bool importPlaylist(const QString& filePath, const QString& playlistName);
    
    // 当前播放列表管理
    void clearCurrentPlaylist();
    bool loadPlaylist(int playlistId);
    int getCurrentSongIndex() const;
    bool setCurrentSongIndex(int index);
    Song getNextSong();
    Song getPreviousSong();
    
    // 播放位置管理
    void setCurrentIndex(int index);
    int getCurrentIndex() const;
    Song getCurrentSong() const;
    bool hasCurrentSong() const;
    
    // 播放模式管理
    void setPlayMode(PlayMode mode);
    PlayMode getPlayMode() const;
    QString getPlayModeString() const;
    
    // 播放状态
    PlaylistState getState() const;
    bool isPlaying() const;
    bool isPaused() const;
    bool isStopped() const;
    
    // 播放队列管理
    void enqueueNext(const Song& song);
    void enqueueNext(const QList<Song>& songs);
    void enqueueAtEnd(const Song& song);
    void enqueueAtEnd(const QList<Song>& songs);
    void clearQueue();
    QList<QueueItem> getQueue() const;
    int getQueueSize() const;
    bool hasQueue() const;
    
    // 播放历史
    void addToHistory(const Song& song);
    QList<Song> getHistory() const;
    void clearHistory();
    void setHistorySize(int size);
    int getHistorySize() const;
    
    // 播放列表排序
    enum class SortBy {
        Title,
        Artist,
        Album,
        Duration,
        DateAdded,
        PlayCount,
        Custom
    };
    
    enum class SortOrder {
        Ascending,
        Descending
    };
    
    PlaylistOperationResult sortPlaylist(int playlistId, SortBy sortBy, SortOrder order = SortOrder::Ascending);
    
    // 播放列表洗牌
    PlaylistOperationResult shufflePlaylist(int playlistId);
    void restoreOriginalOrder(int playlistId);
    
    // 播放列表搜索
    QList<Playlist> searchPlaylists(const QString& keyword) const;
    QList<Song> searchSongsInPlaylist(int playlistId, const QString& keyword) const;
    
    // 智能播放列表
    PlaylistOperationResult createSmartPlaylist(const QString& name, const QString& criteria);
    PlaylistOperationResult updateSmartPlaylist(int playlistId, const QString& criteria);
    QList<Song> getSmartPlaylistSongs(int playlistId) const;
    bool isSmartPlaylist(int playlistId) const;
    
    // 播放列表统计
    PlayStatistics getStatistics() const;
    qint64 getPlaylistDuration(int playlistId) const;
    int getTotalPlaylistCount() const;
    int getTotalSongCount() const;
    
    // 播放列表导入导出
    bool exportPlaylistToM3U(int playlistId, const QString& filePath) const;
    bool importPlaylistFromM3U(const QString& filePath, const QString& playlistName);
    bool exportPlaylistToJson(int playlistId, const QString& filePath) const;
    bool importPlaylistFromJson(const QString& filePath);
    
    // 播放列表备份恢复
    bool backupPlaylists(const QString& backupPath) const;
    bool restorePlaylists(const QString& backupPath);
    
    // 收藏夹管理
    void addToFavorites(int playlistId);
    void removeFromFavorites(int playlistId);
    bool isFavorite(int playlistId) const;
    QList<int> getFavoritePlaylistIds() const;
    
    // 播放列表验证
    bool playlistExists(const QString& name) const;
    bool playlistExists(int playlistId) const;
    bool canDeletePlaylist(int playlistId) const;
    bool canUpdatePlaylist(int playlistId) const;
    
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
    // 播放列表变化信号
    void playlistCreated(const Playlist& playlist);
    void playlistUpdated(const Playlist& playlist);
    void playlistDeleted(int playlistId, const QString& name);
    void currentPlaylistChanged(int playlistId);
    
    // 播放列表内容变化信号
    void songAddedToPlaylist(int playlistId, const Song& song, int index);
    void songRemovedFromPlaylist(int playlistId, int index);
    void songMovedInPlaylist(int playlistId, int fromIndex, int toIndex);
    void playlistCleared(int playlistId);
    void playlistShuffled(int playlistId);
    void playlistSorted(int playlistId, SortBy sortBy, SortOrder order);
    
    // 播放控制信号
    void playbackStarted(const Song& song);
    void playbackPaused();
    void playbackStopped();
    void currentSongChanged(const Song& song);
    void currentIndexChanged(int index);
    void playModeChanged(PlayMode mode);
    void stateChanged(PlaylistState state);
    void repeatModeChanged(RepeatMode mode);
    void shuffleModeChanged(bool enabled);
    
    // 播放队列信号
    void queueChanged();
    void queueCleared();
    void songEnqueued(const Song& song);
    
    // 播放历史信号
    void historyChanged();
    void historyCleared();
    void songAddedToHistory(const Song& song);
    
    // 统计信号
    void statisticsUpdated(const PlayStatistics& stats);
    
    // 错误信号
    void errorOccurred(const QString& error);
    
private slots:
    void onPlaylistChanged(int playlistId);
    void onSongChanged(int songId);
    void updateStatistics();
    void cleanupEmptyPlaylists();
    void handlePlaybackFinished();
    void handleQueueNext();
    
private:
    // 单例实例
    static PlaylistManager* m_instance;
    static QMutex m_instanceMutex;
    
    // 数据访问对象
    PlaylistDao* m_playlistDao;
    SongDao* m_songDao;
    
    // 播放列表数据
    QList<Playlist> m_playlists;
    QHash<int, QList<Song>> m_playlistSongs;
    QHash<int, QList<int>> m_originalOrders; // 洗牌前的原始顺序
    
    // 当前播放状态
    int m_currentPlaylistId;
    Playlist m_currentPlaylist;
    QList<Song> m_currentPlaylistSongs;
    int m_currentIndex;
    int m_currentSongIndex;
    PlayMode m_playMode;
    PlaylistState m_state;
    RepeatMode m_repeatMode;
    bool m_shuffleMode;
    QList<int> m_shuffledIndices;
    int m_shuffleIndex;
    
    // 播放队列
    QQueue<QueueItem> m_playQueue;
    
    // 播放历史
    QList<Song> m_playHistory;
    int m_maxHistorySize;
    
    // 收藏夹
    QSet<int> m_favoritePlaylistIds;
    
    // 随机播放历史
    QList<int> m_randomHistory;
    int m_randomHistorySize;
    
    // 缓存
    mutable QHash<int, Playlist> m_playlistCache;
    mutable QHash<int, QList<Song>> m_songCache;
    mutable QMutex m_cacheMutex;
    bool m_cacheEnabled;
    
    // 统计信息
    PlayStatistics m_statistics;
    QTimer* m_statisticsTimer;
    
    // 撤销/重做
    struct UndoRedoCommand {
        PlaylistOperation operation;
        QVariant data;
        QString description;
    };
    
    QList<UndoRedoCommand> m_undoStack;
    QList<UndoRedoCommand> m_redoStack;
    bool m_undoRedoEnabled;
    int m_maxUndoRedoSize;
    
    // 线程安全
    mutable QMutex m_mutex;
    
    // 内部索引生成
    int m_nextPlaylistId;
    
    // 内部辅助方法
    bool loadPlaylistsFromDatabase();
    bool savePlaylistToDatabase(const Playlist& playlist);
    bool deletePlaylistFromDatabase(int playlistId);
    bool initializeDao();
    void createDefaultPlaylists();
    int getNextSortOrder() const;
    void generateShuffledIndices();
    int getNextSongIndex() const;
    int getPreviousSongIndex() const;
    
    // 导出导入辅助方法
    bool exportToM3U(const Playlist& playlist, const QList<Song>& songs, const QString& filePath);
    bool exportToPLS(const Playlist& playlist, const QList<Song>& songs, const QString& filePath);
    bool exportToJSON(const Playlist& playlist, const QList<Song>& songs, const QString& filePath);
    bool importFromM3U(const QString& filePath, const QString& playlistName);
    bool importFromPLS(const QString& filePath, const QString& playlistName);
    bool importFromJSON(const QString& filePath, const QString& playlistName);
    
    // 播放控制实现
    void playInternal();
    void pauseInternal();
    void stopInternal();
    void nextInternal();
    void previousInternal();
    void playAtInternal(int index);
    
    // 播放模式实现
    int getNextIndex();
    int getPreviousIndex();
    int getRandomIndex();
    int getShuffleIndex();
    
    // 队列管理
    void processQueue();
    QueueItem dequeue();
    void enqueue(const QueueItem& item);
    
    // 缓存管理
    void addPlaylistToCache(const Playlist& playlist);
    void removePlaylistFromCache(int playlistId);
    void updatePlaylistInCache(const Playlist& playlist);
    void addSongsToCache(int playlistId, const QList<Song>& songs);
    void removeSongsFromCache(int playlistId);
    
    // 验证方法
    bool validatePlaylistName(const QString& name) const;
    bool validatePlaylistId(int playlistId) const;
    bool validateSongIndex(int playlistId, int index) const;
    
    // 内部操作
    PlaylistOperationResult createPlaylistInternal(const Playlist& playlist);
    PlaylistOperationResult updatePlaylistInternal(const Playlist& playlist);
    PlaylistOperationResult deletePlaylistInternal(int playlistId);
    
    // 撤销/重做实现
    void addUndoCommand(const UndoRedoCommand& command);
    void executeUndoCommand(const UndoRedoCommand& command);
    void executeRedoCommand(const UndoRedoCommand& command);
    
    // 统计计算
    void calculateStatistics();
    
    // 智能播放列表
    QList<Song> evaluateSmartPlaylistCriteria(const QString& criteria) const;
    bool isValidSmartPlaylistCriteria(const QString& criteria) const;
    
    // 状态更新
    void updateState(PlaylistState newState);
    void updateCurrentSong();
    void updateCurrentIndex();
    
    // 导入导出实现
    QStringList parseM3UFile(const QString& filePath) const;
    bool writeM3UFile(const QString& filePath, const QList<Song>& songs) const;
    QJsonObject playlistToJson(const Playlist& playlist) const;
    Playlist playlistFromJson(const QJsonObject& json) const;
    
    // 日志记录
    void logPlaylistOperation(PlaylistOperation operation, const QString& details);
    void logError(const QString& error);
    void logInfo(const QString& message);
    
    // 工具方法
    QString generatePlaylistName(const QString& baseName) const;
    int generatePlaylistId();
    bool isValidPlaylistName(const QString& name) const;
    QString sanitizePlaylistName(const QString& name) const;
    
    // 播放模式字符串转换
    QString playModeToString(PlayMode mode) const;
    PlayMode stringToPlayMode(const QString& modeString) const;
};

// 播放模式工具类
class PlayModeUtils {
public:
    static QString toString(PlayMode mode);
    static PlayMode fromString(const QString& modeString);
    static QList<PlayMode> getAllModes();
    static QString getDescription(PlayMode mode);
    static QIcon getIcon(PlayMode mode);
};

// 注册元类型
Q_DECLARE_METATYPE(PlayMode)
Q_DECLARE_METATYPE(RepeatMode)
Q_DECLARE_METATYPE(ExportFormat)
Q_DECLARE_METATYPE(PlaylistState)
Q_DECLARE_METATYPE(PlaylistOperation)
Q_DECLARE_METATYPE(PlaylistOperationResult)

#endif // PLAYLISTMANAGER_H
