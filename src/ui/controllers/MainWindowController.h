#ifndef MAINWINDOWCONTROLLER_H
#define MAINWINDOWCONTROLLER_H

#include <QObject>
#include <QMainWindow>
#include <QListWidget>
#include <QListWidgetItem>
#include <QList>
#include <QToolBar>
#include <QAction>
#include <QSplitter>
#include <QFrame>
#include <QLabel>
#include <QSlider>
#include <QPushButton>
#include <QProgressBar>
#include <QStatusBar>
#include <QTimer>
#include <QMap>
#include <QMenu>
#include <QContextMenuEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QColorDialog>
#include <QPixmap>
#include <QIcon>
#include <QKeyEvent>
#include <QCloseEvent>
#include <QSettings>
#include <QApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QMutex>
#include <QThread>
#include <QFuture>
#include <QtConcurrent>
#include <memory>

// 前向声明
class MainWindow;
class AudioEngine;
class TagManager;
class PlaylistManager;
class ComponentIntegration;
class AddSongDialogController;
class PlayInterfaceController;
class ManageTagDialogController;

#include "../../models/song.h"
#include "../../models/tag.h"
#include "../../models/playlist.h"
#include "../../core/logger.h"
#include "../../threading/mainthreadmanager.h"
#include "../../audio/audiotypes.h"

// 主窗口状态枚举
enum class MainWindowState {
    Initializing,
    Ready,
    Playing,
    Paused,
    Loading,
    Error
};

// 视图模式枚举
enum class ViewMode {
    TagView,
    PlaylistView,
    AlbumView,
    ArtistView
};

// 排序模式枚举
enum class SortMode {
    Title,
    Artist,
    Album,
    Duration,
    DateAdded,
    PlayCount
};

// 主窗口控制器
class MainWindowController : public QObject {
    Q_OBJECT
    
public:
    explicit MainWindowController(MainWindow* mainWindow, QObject* parent = nullptr);
    ~MainWindowController();
    
    // 初始化和清理
    bool initialize();
    void shutdown();
    bool isInitialized() const;
    
    // 主窗口状态
    void setState(MainWindowState state);
    MainWindowState getState() const;
    
    // 视图模式
    void setViewMode(ViewMode mode);
    ViewMode getViewMode() const;
    
    // 标签管理
    void refreshTagList();
    void selectTag(int tagId);
    void selectTag(const QString& tagName);
    Tag getSelectedTag() const;
    QList<Tag> getSelectedTags() const;
    
    // 歌曲管理
    void refreshSongList();
    void selectSong(int songId);
    void selectSong(const Song& song);
    Song getSelectedSong() const;
    QList<Song> getSelectedSongs() const;
    
    // 播放控制
    void playSelectedSong();
    void pausePlayback();
    void stopPlayback();
    void nextSong();
    void previousSong();
    void seekToPosition(qint64 position);
    void setVolume(int volume);
    void toggleMute();
    
    // 播放模式
    void setPlayMode(AudioTypes::PlayMode mode);
    AudioTypes::PlayMode getPlayMode() const;
    void togglePlayMode();
    
    // 排序和过滤
    void setSortMode(SortMode mode);
    SortMode getSortMode() const;
    void toggleSortOrder();
    void setFilterText(const QString& text);
    QString getFilterText() const;
    
    // 界面布局
    void saveLayout();
    void restoreLayout();
    void resetLayout();
    
    // 设置管理
    void loadSettings();
    void saveSettings();
    void applySettings();
    
    // 快捷键
    void setupShortcuts();
    void handleGlobalShortcut(const QString& shortcut);
    
    // 拖放支持
    void enableDragDrop(bool enabled);
    bool isDragDropEnabled() const;
    
    // 上下文菜单
    void showTagContextMenu(const QPoint& position);
    void showSongContextMenu(const QPoint& position);
    void showPlaylistContextMenu(const QPoint& position);
    
    // 搜索功能
    void startSearch(const QString& query);
    void clearSearch();
    void nextSearchResult();
    void previousSearchResult();
    
    // 状态显示
    void updateStatusBar(const QString& message, int timeout = 0);
    void updateProgressBar(int value, int maximum = 100);
    void updatePlaybackInfo(const Song& song);
    void updateVolumeDisplay(int volume);
    
    // 错误处理
    void handleError(const QString& error);
    void showErrorDialog(const QString& title, const QString& message);
    void showWarningDialog(const QString& title, const QString& message);
    void showInfoDialog(const QString& title, const QString& message);
    
    Song getCurrentSong() const;
    int getCurrentVolume() const;
    qint64 getCurrentPosition() const;
    qint64 getCurrentDuration() const;
    void setCurrentVolume(int volume);
    void refreshWindowTitle();
    
    void refreshTagListPublic();
    
public slots:
    // 主窗口事件
    void onMainWindowShow();
    void onMainWindowClose();
    void onMainWindowResize();
    void onMainWindowMove();
    
    // 工具栏动作
    void onActionAddMusic();
    void onActionCreateTag();
    void onActionManageTag();
    void onActionPlayInterface();
    void onActionSettings();
    void onActionAbout();
    void onActionExit();
    
    // 标签列表事件
    void onTagListItemClicked(QListWidgetItem* item);
    void onTagListItemDoubleClicked(QListWidgetItem* item);
    void onTagListContextMenuRequested(const QPoint& position);
    void onTagListSelectionChanged();
    
    // 歌曲列表事件
    void onSongListItemClicked(QListWidgetItem* item);
    void onSongListItemDoubleClicked(QListWidgetItem* item);
    void onSongListContextMenuRequested(const QPoint& position);
    void onSongListSelectionChanged();
    
    // 播放控制事件
    void onPlayButtonClicked();
    void onPauseButtonClicked();

    void onNextButtonClicked();
    void onPreviousButtonClicked();
    void onVolumeSliderChanged(int value);
    void onProgressSliderChanged(int value);
    void onMuteButtonClicked();
    
    // 歌曲列表控制按钮事件
    void onPlayAllButtonClicked();             // 开始播放按钮点击
    void onPlayModeButtonClicked();            // 播放模式按钮点击
    void onSelectAllButtonClicked();           // 全选按钮点击
    void onClearSelectionButtonClicked();      // 取消全选按钮点击
    void onDeleteSelectedButtonClicked();      // 删除按钮点击
    
    // 音频引擎事件
    void onAudioStateChanged(AudioTypes::AudioState state);
    void onCurrentSongChanged(const Song& song);
    void onPositionChanged(qint64 position);
    void onDurationChanged(qint64 duration);
    void onVolumeChanged(int volume);
    void onMutedChanged(bool muted);
    void onPlayModeChanged(AudioTypes::PlayMode mode);
    void onAudioError(const QString& error);
    
    // 标签管理器事件
    void onTagCreated(const Tag& tag);
    void onTagUpdated(const Tag& tag);
    void onTagDeleted(int tagId, const QString& name);
    void onSongAddedToTag(int songId, int tagId);
    void onSongRemovedFromTag(int songId, int tagId);
    
    // 播放列表管理器事件
    void onPlaylistCreated(const Playlist& playlist);
    void onPlaylistUpdated(const Playlist& playlist);
    void onPlaylistDeleted(int playlistId, const QString& name);
    void onPlaybackStarted(const Song& song);
    void onPlaybackPaused();
    void onPlaybackStopped();
    
    // 拖放事件
    void onDragEnterEvent(QDragEnterEvent* event);
    void onDropEvent(QDropEvent* event);
    
    void addSongs(const QStringList& filePaths);
    void addSongs(const QStringList& filePaths, const QMap<QString, QStringList>& fileTagAssignments);
    void addTag(const QString& name, const QString& imagePath);
    void editTag(const QString& oldName, const QString& newName, const QString& imagePath);
    void editTagFromMainWindow(const QString& tagName);
    void deleteTag(const QString& name);
    
    // 歌曲操作方法
    void showAddToTagDialog(int songId, const QString& songTitle);
    void removeFromCurrentTag(int songId, const QString& songTitle);
    void showEditSongDialog(int songId, const QString& songTitle);
    void showInFileExplorer(int songId, const QString& songTitle);
    void deleteSongFromDatabase(int songId, const QString& songTitle);
    
    // 新增的歌曲列表控制方法
    void togglePlayPause();                    // 切换播放/暂停状态
    void cyclePlayMode();                      // 循环切换播放模式
    void selectAllSongs();                     // 全选当前列表歌曲
    void clearSongSelection();                 // 取消选中状态
    void deleteSelectedSongs();                // 删除选中的歌曲
    
    // 播放列表操作方法
    void showCreatePlaylistDialog();
    void importPlaylistFromFile();
    void refreshPlaylistView();
    
signals:
    // 状态变化信号
    void stateChanged(MainWindowState state);
    void viewModeChanged(ViewMode mode);
    void sortModeChanged(SortMode mode);
    
    // 选择变化信号
    void tagSelectionChanged(const Tag& tag);
    void songSelectionChanged(const Song& song);
    void multipleTagsSelected(const QList<Tag>& tags);
    void multipleSongsSelected(const QList<Song>& songs);
    
    // 用户操作信号
    void playRequested(const Song& song);
    void pauseRequested();

    void nextRequested();
    void previousRequested();
    void seekRequested(qint64 position);
    void volumeChangeRequested(int volume);
    void muteToggleRequested();
    
    // 界面操作信号
    void addSongRequested();
    void createTagRequested();
    void manageTagRequested();
    void playInterfaceRequested();
    void settingsRequested();
    
    // 搜索信号
    void searchRequested(const QString& query);
    void searchCleared();
    
    // 错误信号
    void errorOccurred(const QString& error);
    
private:
    // 主窗口引用
    MainWindow* m_mainWindow;
    
    // 核心组件
    AudioEngine* m_audioEngine;
    TagManager* m_tagManager;
    PlaylistManager* m_playlistManager;
    ComponentIntegration* m_componentIntegration;
    MainThreadManager* m_mainThreadManager;
    
    // 子控制器
    std::unique_ptr<AddSongDialogController> m_addSongController;
    std::unique_ptr<PlayInterfaceController> m_playInterfaceController;
    std::unique_ptr<ManageTagDialogController> m_manageTagController;
    
    // UI组件引用
    QListWidget* m_tagListWidget;
    QListWidget* m_songListWidget;
    QToolBar* m_toolBar;
    QSplitter* m_splitter;
    QFrame* m_tagFrame;
    QFrame* m_songFrame;
    QFrame* m_playbackFrame;
    QLabel* m_currentSongLabel;
    QLabel* m_currentTimeLabel;     // 当前播放时间标签
    QLabel* m_totalTimeLabel;       // 总时长标签
    // QLabel* m_volumeLabel;          // 音量显示标签已删除
    QSlider* m_progressSlider;
    QSlider* m_volumeSlider;
    QPushButton* m_playButton;
    QPushButton* m_pauseButton;

    QPushButton* m_nextButton;
    QPushButton* m_previousButton;
    QPushButton* m_muteButton;
    QProgressBar* m_progressBar;
    QStatusBar* m_statusBar;
    QPushButton* m_playModeButton; // 播放模式切换按钮
    
    // 状态管理
    MainWindowState m_state;
    ViewMode m_viewMode;
    SortMode m_sortMode;
    bool m_sortAscending;
    bool m_initialized;
    
    // 选择状态
    Tag m_selectedTag;
    Song m_selectedSong;
    QList<Tag> m_selectedTags;
    QList<Song> m_selectedSongs;
    
    // 搜索状态
    QString m_searchQuery;
    QList<Song> m_searchResults;
    int m_currentSearchIndex;
    
    // 设置
    QSettings* m_settings;
    QHash<QString, QVariant> m_defaultSettings;
    
    // 定时器
    QTimer* m_updateTimer;
    QTimer* m_statusTimer;
    
    // 线程安全
    QMutex m_mutex;
    
    // 拖放支持
    bool m_dragDropEnabled;
    
    // 内部方法
    void setupUI();
    void setupConnections();
    void setupActions();
    void setupMenus();
    void setupToolBar();
    void setupStatusBar();
    void setupShortcutsInternal();
    
    // UI更新
    void updateTagList();
    void updateSongList();
    void updatePlaybackControls();
    void updateVolumeControls();
    void updateProgressControls();
    void updateCurrentSongInfo();
    void updatePlayModeButton();
    void updateToolBarActions();
    void updateMenuActions();
    
    // 列表管理
    void populateTagList();
    void populateSongList();
    void clearTagList();
    void clearSongList();
    void filterSongList(const QString& filter);
    void sortSongList(SortMode mode, bool ascending);
    
    // 选择管理
    void updateTagSelection();
    void updateSongSelection();
    void clearSelections();
    
    // 事件处理
    void handleTagSelectionChange();
    void handleSongSelectionChange();
    void handlePlaybackStateChange();
    void handleAudioEngineError(const QString& error);
    
    // 上下文菜单
    void createTagContextMenu();
    void createSongContextMenu();
    void createPlaylistContextMenu();
    
    // 拖放处理
    void handleFileDrop(const QStringList& filePaths);
    void handleTagDrop(const QList<Tag>& tags);
    void handleSongDrop(const QList<Song>& songs);
    
    // 搜索实现
    void performSearch();
    void highlightSearchResults();
    void clearSearchHighlight();
    
    // 设置管理
    void loadDefaultSettings();
    void validateSettings();
    void applySettingsToUI();
    
    // 工具方法
    void updateUIState();
    void updateWindowTitle();
    void updateStatusMessage();
    void refreshUI();
    
    // 错误处理
    void logError(const QString& error);
    void logInfo(const QString& message);
    void logDebug(const QString& message);
    void logWarning(const QString& message);
    
    // 资源管理
    void loadIcons();
    void loadStyleSheets();
    void loadTranslations();
    
    // 格式化方法
    QString formatTime(qint64 milliseconds) const;
    QString formatDuration(qint64 duration) const;
    QString formatFileSize(qint64 size) const;
    QString formatPlayCount(int count) const;
    
    // 验证方法
    bool validateSongFile(const QString& filePath) const;
    bool validateTagName(const QString& name) const;
    bool validatePlaylistName(const QString& name) const;
    
    // 播放列表文件解析方法
    QStringList parseM3UPlaylist(const QString& filePath) const;
    QStringList parsePLSPlaylist(const QString& filePath) const;
    QStringList parseXSPFPlaylist(const QString& filePath) const;
    
    // 常量
    static const int UPDATE_INTERVAL = 100; // 100ms
    static const int STATUS_TIMEOUT = 5000; // 5秒
    static const int PROGRESS_UPDATE_INTERVAL = 50; // 50ms
    static const int MAX_RECENT_FILES = 10;
    static const int MAX_SEARCH_RESULTS = 100;
};

#endif // MAINWINDOWCONTROLLER_H