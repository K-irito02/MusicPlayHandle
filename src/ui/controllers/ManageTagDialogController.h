#ifndef MANAGETAGDIALOGCONTROLLER_H
#define MANAGETAGDIALOGCONTROLLER_H

#include <QObject>
#include <QStringList>
#include <QList>
#include <QMap>
#include <QHash>
#include <QTimer>
#include <QMutex>
#include <QSettings>
#include <QMessageBox>
#include <QInputDialog>
#include <QColorDialog>
#include <QFileDialog>
#include <QStandardPaths>
#include <QStack>

// 前向声明
class ManageTagDialog;
class TagManager;
class PlaylistManager;
class DatabaseManager;
class Logger;

#include "../../models/song.h"
#include "../../models/tag.h"
#include "../../models/playlist.h"
#include "../../core/logger.h"

// 操作类型枚举
enum class OperationType {
    CreateTag,
    DeleteTag,
    RenameTag,
    MoveSong,
    CopySong,
    EditTagProperties,
    BulkMove,
    BulkCopy
};

// 标签操作结构
struct TagDialogOperation {
    OperationType type;
    QString tagName;
    QString newTagName;
    QString oldTagName;
    QStringList songIds;
    QStringList fromTags;
    QStringList toTags;
    QMap<QString, QVariant> properties;
    qint64 timestamp;
    bool isReversible;
    
    TagDialogOperation() : type(OperationType::CreateTag), timestamp(0), isReversible(true) {}
};

// 歌曲传输信息
struct SongTransferInfo {
    QString songId;
    QString songTitle;
    QString artistName;
    QString albumName;
    QString filePath;
    qint64 duration;
    QStringList currentTags;
    QStringList targetTags;
    bool isSelected;
    
    SongTransferInfo() : duration(0), isSelected(false) {}
};

// 标签统计信息
struct TagDialogStatistics {
    QString tagName;
    int songCount;
    int playCount;
    qint64 totalDuration;
    QDateTime lastModified;
    QDateTime createdDate;
    QString color;
    QString iconPath;
    
    TagDialogStatistics() : songCount(0), playCount(0), totalDuration(0) {}
};

struct OperationHistory {
    QStack<TagDialogOperation> undoStack;
    QStack<TagDialogOperation> redoStack;
};

class ManageTagDialogController : public QObject
{
    Q_OBJECT

public:
    explicit ManageTagDialogController(ManageTagDialog* dialog, QObject* parent = nullptr);
    ~ManageTagDialogController();

    // 初始化和清理
    bool initialize();
    void shutdown();
    bool isInitialized() const;

    // 数据加载
    void loadTags();
    void loadSongs();
    QList<TagDialogStatistics> loadTagStatistics();
    void refreshData();
    
    // 标签操作
    void createTag(const QString& name, const QString& color = "", const QString& iconPath = "");
    void deleteTag(const QString& name);
    void renameTag(const QString& oldName, const QString& newName);
    void editTagProperties(const QString& name, const QMap<QString, QVariant>& properties);
    void duplicateTag(const QString& sourceName, const QString& targetName);
    
    // 歌曲传输
    void moveSongs(const QStringList& songIds, const QString& fromTag, const QString& toTag);
    void copySongs(const QStringList& songIds, const QString& fromTag, const QString& toTag);
    void moveSongsBulk(const QStringList& songIds, const QStringList& fromTags, const QStringList& toTags);
    void copySongsBulk(const QStringList& songIds, const QStringList& fromTags, const QStringList& toTags);
    
    // 选择操作
    void selectAllSongs();
    void selectNoneSongs();
    void selectSongsByTag(const QString& tagName);
    void selectSongsByArtist(const QString& artistName);
    void selectSongsByAlbum(const QString& albumName);
    void invertSelection();
    
    // 过滤和搜索
    void filterSongsByTag(const QString& tagName);
    void filterSongsByText(const QString& text);
    void clearFilter();
    void searchSongs(const QString& query);
    
    // 排序
    void sortSongsByTitle();
    void sortSongsByArtist();
    void sortSongsByAlbum();
    void sortSongsByDuration();
    void sortSongsByDateAdded();
    
    // 操作历史
    void undoLastOperation();
    void redoLastOperation();
    void clearOperationHistory();
    bool canUndo() const;
    bool canRedo() const;
    
    // 批量操作
    void batchCreateTags(const QStringList& tagNames);
    void batchDeleteTags(const QStringList& tagNames);
    void batchMoveSongs(const QStringList& songIds, const QString& targetTag);
    void batchCopySongs(const QStringList& songIds, const QString& targetTag);
    
    // 导入导出
    void exportTagConfiguration(const QString& filePath);
    void importTagConfiguration(const QString& filePath);
    void exportSongList(const QString& filePath, const QString& format);
    
    // 统计信息
    QList<TagDialogStatistics> getTagStatistics() const;
    QStringList getMostUsedTags() const;
    QStringList getLeastUsedTags() const;
    QStringList getEmptyTags() const;
    
    // 验证
    bool validateTagName(const QString& name) const;
    bool isTagNameDuplicate(const QString& name) const;
    bool canDeleteTag(const QString& name) const;
    bool canRenameTag(const QString& name) const;

signals:
    // 标签操作信号
    void tagCreated(const QString& name, const TagDialogStatistics& stats);
    void tagDeleted(const QString& name);
    void tagRenamed(const QString& oldName, const QString& newName);
    void tagPropertiesChanged(const QString& name, const QMap<QString, QVariant>& properties);
    
    // 歌曲传输信号
    void songsMoved(const QStringList& songIds, const QString& fromTag, const QString& toTag);
    void songsCopied(const QStringList& songIds, const QString& fromTag, const QString& toTag);
    void songsSelected(const QStringList& songIds);
    void songsFiltered(const QStringList& songIds);
    
    // 操作历史信号
    void operationExecuted(const TagDialogOperation& operation);
    void operationUndone(const TagDialogOperation& operation);
    void operationRedone(const TagDialogOperation& operation);
    
    // 数据更新信号
    void dataLoaded();
    void dataRefreshed();
    void statisticsUpdated(const QList<TagDialogStatistics>& stats);
    
    // 进度信号
    void progressUpdated(int value, const QString& message);
    void operationStarted(const QString& operation);
    void operationCompleted(const QString& operation, bool success);
    
    // 错误信号
    void errorOccurred(const QString& error);
    void warningOccurred(const QString& warning);
    void dialogAccepted();
    void dialogRejected();
    void uiRefreshed();

public slots:
    // 标签选择槽
    void onTag1SelectionChanged(const QString& tagName);
    void onTag2SelectionChanged(const QString& tagName);
    void onSongSelectionChanged(const QStringList& songIds);
    
    // 操作按钮槽
    void onCreateTagClicked();
    void onDeleteTagClicked();
    void onRenameTagClicked();
    void onEditTagPropertiesClicked();
    void onMoveTransferClicked();
    void onCopyTransferClicked();
    void onUndoClicked();
    void onRedoClicked();
    
    // 选择操作槽
    void onSelectAllClicked();
    void onSelectNoneClicked();
    void onInvertSelectionClicked();
    
    // 过滤搜索槽
    void onFilterChanged(const QString& text);
    void onSearchChanged(const QString& query);
    void onClearFilterClicked();
    
    // 排序槽
    void onSortByTitleClicked();
    void onSortByArtistClicked();
    void onSortByAlbumClicked();
    void onSortByDurationClicked();
    void onSortByDateAddedClicked();
    
    // 对话框槽
    void onAcceptRequested();
    void onRejectRequested();
    void onApplyRequested();
    void onResetRequested();

    // 歌曲传输（由 ManageTagDialog 调用）
    void transferSongs(const QString& fromTag, const QString& toTag, bool copy);
    void updateStatistics();

private slots:
    void onDataUpdateTimer();
    void onOperationTimer();
    void onStatisticsUpdateTimer();

private:
    ManageTagDialog* m_dialog;
    TagManager* m_tagManager;
    PlaylistManager* m_playlistManager;
    DatabaseManager* m_databaseManager;
    Logger* m_logger;
    
    // 数据存储
    QList<Tag> m_tags;
    QList<Song> m_songs;
    QList<TagDialogStatistics> m_tagStatistics;
    QList<SongTransferInfo> m_songTransferInfo;
    
    // 选择状态
    QString m_selectedTag1;
    QString m_selectedTag2;
    QStringList m_selectedSongs;
    QStringList m_filteredSongs;
    
    // 操作历史
    OperationHistory m_operationHistory;
    int m_operationIndex;
    int m_maxHistorySize;
    
    // 过滤和搜索
    QString m_filterText;
    QString m_searchQuery;
    bool m_filterActive;
    
    // 排序
    enum class SortField { Title, Artist, Album, Duration, DateAdded };
    SortField m_sortField;
    bool m_sortAscending;
    
    // 状态
    bool m_initialized;
    bool m_dataLoaded;
    bool m_processing;
    bool m_hasUnsavedChanges;
    
    // 定时器
    QTimer* m_dataUpdateTimer;
    QTimer* m_operationTimer;
    QTimer* m_statisticsUpdateTimer;
    
    // 线程安全
    QMutex m_mutex;
    
    // 设置
    QSettings* m_settings;
    
    // 内部方法
    void setupConnections();
    void loadSettings();
    void saveSettings();
    
    // 数据处理
    void loadTagsFromDatabase();
    void loadSongsFromDatabase();
    void loadStatisticsFromDatabase();
    void saveTagToDatabase(const Tag& tag);
    void deleteTagFromDatabase(const QString& tagName);
    void updateSongTagsInDatabase(const QString& songId, const QStringList& tags);
    
    // 操作执行
    void executeOperation(const TagDialogOperation& operation);
    void undoOperation(const TagDialogOperation& operation);
    void redoOperation(const TagDialogOperation& operation);
    void addOperationToHistory(const TagDialogOperation& operation);
    
    // 过滤和排序
    void applyFilter();
    void applySorting();
    void updateFilteredSongs();
    void updateSortedSongs();
    
    // UI更新
    void updateTagLists();
    void updateSongList();
    void updateButtonStates();
    void updateStatusBar();
    void updateProgressBar();
    void updateStatisticsDisplay();
    
    // 选择处理
    void updateSelectionState();
    void clearSelections();
    void selectSongsInternal(const QStringList& songIds);
    
    // 验证
    bool validateOperationInternal(const TagDialogOperation& operation);
    bool validateTagOperationInternal(const QString& tagName, OperationType type);
    bool validateSongTransferInternal(const QStringList& songIds, const QString& fromTag, const QString& toTag);
    
    // 工具方法
    QString formatDuration(qint64 duration) const;
    QString formatFileSize(qint64 size) const;
    QString formatDateTime(const QDateTime& dateTime) const;
    QColor getTagColor(const QString& tagName) const;
    QPixmap getTagIcon(const QString& tagName) const;
    
    // 错误处理
    void handleError(const QString& error);
    void handleWarning(const QString& warning);
    void logInfo(const QString& message);
    void logError(const QString& error);
    void logDebug(const QString& message);
    void logWarning(const QString& warning);
    void refreshUI();
    void cleanupOperationHistory();
    
    // 常量
    static const int MAX_HISTORY_SIZE = 100;
    static const int DATA_UPDATE_INTERVAL = 1000; // ms
    static const int OPERATION_TIMEOUT = 30000; // ms
    static const int STATISTICS_UPDATE_INTERVAL = 5000; // ms
    static const int MAX_TAG_NAME_LENGTH = 50;
    static const int MAX_BATCH_SIZE = 1000;
    void applyOperations();
    void cancelOperations();
    void resetOperations();
    void undoOperation();
    void redoOperation();
    bool undoCreateTag(const TagDialogOperation& operation);
    bool undoDeleteTag(const TagDialogOperation& operation);
    bool undoRenameTag(const TagDialogOperation& operation);
    bool undoTransferSongs(const TagDialogOperation& operation);
    bool redoCreateTag(const TagDialogOperation& operation);
    bool redoDeleteTag(const TagDialogOperation& operation);
    bool redoRenameTag(const TagDialogOperation& operation);
    bool redoTransferSongs(const TagDialogOperation& operation);
};

#endif // MANAGETAGDIALOGCONTROLLER_H 