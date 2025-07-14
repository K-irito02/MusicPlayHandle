#include "mainwindowcontroller.h"
#include "../../../mainwindow.h"
#include "../../audio/audioengine.h"
#include "../../audio/audiotypes.h"
#include "../../managers/tagmanager.h"
#include "../../managers/playlistmanager.h"
#include "../../core/componentintegration.h"
#include "../../threading/mainthreadmanager.h"
#include "../../models/song.h"
#include "../../models/tag.h"
#include "../../models/playlist.h"
#include "addsongdialogcontroller.h"
#include "playinterfacecontroller.h"
#include "managetagdialogcontroller.h"
#include "../widgets/taglistitem.h"
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMessageBox>
#include <QStatusBar>
#include <QSettings>
#include <QTimer>
#include "../../database/songdao.h"
#include "../../database/tagdao.h"
#include <QListWidgetItem>
#include <QPixmap>
#include "../../ui/dialogs/createtagdialog.h"
#include <QMenu>
#include <QMessageBox>
#include <QLineEdit>
#include <QLabel>
#include "../../database/databasemanager.h"
#include <QIcon>

MainWindowController::MainWindowController(MainWindow* mainWindow, QObject* parent)
    : QObject(parent)
    , m_mainWindow(mainWindow)
    , m_audioEngine(nullptr)
    , m_tagManager(nullptr)
    , m_playlistManager(nullptr)
    , m_componentIntegration(nullptr)
    , m_mainThreadManager(nullptr)
    , m_addSongController(nullptr)
    , m_playInterfaceController(nullptr)
    , m_manageTagController(nullptr)
    , m_tagListWidget(nullptr)
    , m_songListWidget(nullptr)
    , m_toolBar(nullptr)
    , m_splitter(nullptr)
    , m_tagFrame(nullptr)
    , m_songFrame(nullptr)
    , m_playbackFrame(nullptr)
    , m_currentSongLabel(nullptr)
    , m_progressSlider(nullptr)
    , m_volumeSlider(nullptr)
    , m_playButton(nullptr)
    , m_pauseButton(nullptr)
    , m_stopButton(nullptr)
    , m_nextButton(nullptr)
    , m_previousButton(nullptr)
    , m_muteButton(nullptr)
    , m_progressBar(nullptr)
    , m_statusBar(nullptr)
    , m_state(MainWindowState::Initializing)
    , m_viewMode(ViewMode::TagView)
    , m_sortMode(SortMode::Title)
    , m_sortAscending(true)
    , m_initialized(false)
    , m_searchQuery("")
    , m_currentSearchIndex(0)
    , m_settings(nullptr)
    , m_updateTimer(nullptr)
    , m_statusTimer(nullptr)
    , m_dragDropEnabled(true)
{
    // 初始化设置
    m_settings = new QSettings(this);
    
    // 初始化定时器
    m_updateTimer = new QTimer(this);
    m_statusTimer = new QTimer(this);
    
    // 连接定时器信号
    connect(m_updateTimer, &QTimer::timeout, this, &MainWindowController::refreshUI);
    connect(m_statusTimer, &QTimer::timeout, this, &MainWindowController::updateStatusMessage);
}

MainWindowController::~MainWindowController()
{
    shutdown();
}

bool MainWindowController::initialize()
{
    if (m_initialized) {
        return true;
    }
    
    logInfo("正在初始化主窗口控制器...");
    
    try {
        // 设置UI
        setupUI();
        
        // 设置连接
        setupConnections();
        
        // 加载设置
        loadSettings();
        
        // 设置状态
        setState(MainWindowState::Ready);
        
        // 初始化时同步播放模式按钮
        updatePlayModeButton();
        
        m_initialized = true;
        logInfo("主窗口控制器初始化完成");
        
        return true;
    } catch (const std::exception& e) {
        logError(QString("主窗口控制器初始化失败: %1").arg(e.what()));
        setState(MainWindowState::Error);
        return false;
    }
}

void MainWindowController::shutdown()
{
    if (!m_initialized) {
        return;
    }
    
    logInfo("正在关闭主窗口控制器...");
    
    // 保存设置
    saveSettings();
    
    // 停止定时器
    if (m_updateTimer) {
        m_updateTimer->stop();
    }
    if (m_statusTimer) {
        m_statusTimer->stop();
    }
    
    m_initialized = false;
    logInfo("主窗口控制器已关闭");
}

bool MainWindowController::isInitialized() const
{
    return m_initialized;
}

void MainWindowController::setState(MainWindowState state)
{
    if (m_state != state) {
        m_state = state;
        emit stateChanged(state);
        updateUIState();
    }
}

MainWindowState MainWindowController::getState() const
{
    return m_state;
}

void MainWindowController::setViewMode(ViewMode mode)
{
    if (m_viewMode != mode) {
        m_viewMode = mode;
        emit viewModeChanged(mode);
        refreshUI();
    }
}

ViewMode MainWindowController::getViewMode() const
{
    return m_viewMode;
}

// 工具栏动作槽函数
void MainWindowController::onActionAddMusic()
{
    logInfo("添加音乐请求");
    emit addSongRequested();
}

void MainWindowController::onActionCreateTag()
{
    logInfo("创建标签请求");
    emit createTagRequested();
}

void MainWindowController::onActionManageTag()
{
    logInfo("管理标签请求");
    emit manageTagRequested();
}

void MainWindowController::onActionPlayInterface()
{
    logInfo("播放界面请求");
    emit playInterfaceRequested();
}

void MainWindowController::onActionSettings()
{
    logInfo("设置请求");
    emit settingsRequested();
}

void MainWindowController::onActionAbout()
{
    showInfoDialog("关于", "Qt6音频播放器 v1.0.0\n基于Qt6和C++11开发");
}

void MainWindowController::onActionExit()
{
    if (m_mainWindow) {
        m_mainWindow->close();
    }
}

// 标签列表事件
void MainWindowController::onTagListItemClicked(QListWidgetItem* item)
{
    if (item) {
        logInfo(QString("标签被点击: %1").arg(item->text()));
        // 这里应该根据标签ID加载歌曲
        updateStatusBar(QString("选择标签: %1").arg(item->text()));
    }
}

void MainWindowController::onTagListItemDoubleClicked(QListWidgetItem* item)
{
    if (item) {
        logInfo(QString("标签被双击: %1").arg(item->text()));
        // 这里应该编辑标签
        updateStatusBar(QString("编辑标签: %1").arg(item->text()));
    }
}

// 歌曲列表事件
void MainWindowController::onSongListItemClicked(QListWidgetItem* item)
{
    if (item) {
        logInfo(QString("歌曲被点击: %1").arg(item->text()));
        updateStatusBar(QString("选择歌曲: %1").arg(item->text()));
    }
}

void MainWindowController::onSongListItemDoubleClicked(QListWidgetItem* item)
{
    if (item) {
        logInfo(QString("歌曲被双击: %1").arg(item->text()));
        updateStatusBar(QString("播放歌曲: %1").arg(item->text()));
        // 这里应该播放歌曲
    }
}

// 播放控制事件
void MainWindowController::onPlayButtonClicked()
{
    logInfo("播放按钮被点击");
    emit playRequested(m_selectedSong);
}

void MainWindowController::onPauseButtonClicked()
{
    logInfo("暂停按钮被点击");
    emit pauseRequested();
}

void MainWindowController::onStopButtonClicked()
{
    logInfo("停止按钮被点击");
    emit stopRequested();
}

void MainWindowController::onNextButtonClicked()
{
    logInfo("下一首按钮被点击");
    emit nextRequested();
}

void MainWindowController::onPreviousButtonClicked()
{
    logInfo("上一首按钮被点击");
    emit previousRequested();
}

void MainWindowController::onVolumeSliderChanged(int value)
{
    logInfo(QString("音量变化: %1").arg(value));
    emit volumeChangeRequested(value);
    updateVolumeDisplay(value);
}

void MainWindowController::onProgressSliderChanged(int value)
{
    logInfo(QString("进度变化: %1").arg(value));
    // 计算实际时间位置
    qint64 position = static_cast<qint64>(value);
    emit seekRequested(position);
}

void MainWindowController::onMuteButtonClicked()
{
    logInfo("静音按钮被点击");
    emit muteToggleRequested();
}

// 状态显示
void MainWindowController::updateStatusBar(const QString& message, int timeout)
{
    if (m_mainWindow && m_mainWindow->statusBar()) {
        m_mainWindow->statusBar()->showMessage(message, timeout);
    }
}

void MainWindowController::updateProgressBar(int value, int maximum)
{
    // 这里应该更新进度条
    Q_UNUSED(value)
    Q_UNUSED(maximum)
}

void MainWindowController::updatePlaybackInfo(const Song& song)
{
    Q_UNUSED(song)
    // 这里应该更新播放信息
}

void MainWindowController::updateVolumeDisplay(int volume)
{
    updateStatusBar(QString("音量: %1%").arg(volume), 1000);
}

// 错误处理
void MainWindowController::handleError(const QString& error)
{
    logError(error);
    emit errorOccurred(error);
}

void MainWindowController::showErrorDialog(const QString& title, const QString& message)
{
    QMessageBox::critical(m_mainWindow, title, message);
}

void MainWindowController::showWarningDialog(const QString& title, const QString& message)
{
    QMessageBox::warning(m_mainWindow, title, message);
}

void MainWindowController::showInfoDialog(const QString& title, const QString& message)
{
    QMessageBox::information(m_mainWindow, title, message);
}

// 私有方法实现
void MainWindowController::setupUI()
{
    if (!m_mainWindow) {
        return;
    }
    
    // 设置窗口标题
    updateWindowTitle();
    
    // 设置初始状态
    updateUIState();
}

void MainWindowController::setupConnections()
{
    if (!m_mainWindow) return;
    // 假设 m_playButton 等已在 setupUI 初始化
    if (m_playButton) connect(m_playButton, &QPushButton::clicked, this, &MainWindowController::onPlayButtonClicked);
    if (m_pauseButton) connect(m_pauseButton, &QPushButton::clicked, this, &MainWindowController::onPauseButtonClicked);
    if (m_stopButton) connect(m_stopButton, &QPushButton::clicked, this, &MainWindowController::onStopButtonClicked);
    if (m_nextButton) connect(m_nextButton, &QPushButton::clicked, this, &MainWindowController::onNextButtonClicked);
    if (m_previousButton) connect(m_previousButton, &QPushButton::clicked, this, &MainWindowController::onPreviousButtonClicked);
    // 播放模式按钮
    if (m_playModeButton) connect(m_playModeButton, &QPushButton::clicked, this, &MainWindowController::togglePlayMode);
    // ... 其他控件连接 ...
}

void MainWindowController::loadSettings()
{
    // 加载默认设置
    loadDefaultSettings();
    
    // 应用设置到UI
    applySettingsToUI();
}

void MainWindowController::saveSettings()
{
    if (m_settings) {
        m_settings->setValue("MainWindow/ViewMode", static_cast<int>(m_viewMode));
        m_settings->setValue("MainWindow/SortMode", static_cast<int>(m_sortMode));
        m_settings->setValue("MainWindow/SortAscending", m_sortAscending);
        m_settings->sync();
    }
}

void MainWindowController::loadDefaultSettings()
{
    // 设置默认值
    m_viewMode = ViewMode::TagView;
    m_sortMode = SortMode::Title;
    m_sortAscending = true;
}

void MainWindowController::applySettingsToUI()
{
    // 应用设置到UI
    updateUIState();
}

void MainWindowController::updateUIState()
{
    // 更新UI状态
    updateWindowTitle();
    updateStatusMessage();
}

void MainWindowController::updateWindowTitle()
{
    if (m_mainWindow) {
        QString title = "Qt6音频播放器 - v1.0.0";
        if (m_state == MainWindowState::Playing) {
            title += " - 播放中";
        } else if (m_state == MainWindowState::Paused) {
            title += " - 暂停";
        }
        m_mainWindow->setWindowTitle(title);
    }
}

void MainWindowController::updateStatusMessage()
{
    QString message;
    switch (m_state) {
        case MainWindowState::Initializing:
            message = "正在初始化...";
            break;
        case MainWindowState::Ready:
            message = "就绪";
            break;
        case MainWindowState::Playing:
            message = "播放中";
            break;
        case MainWindowState::Paused:
            message = "暂停";
            break;
        case MainWindowState::Loading:
            message = "正在加载...";
            break;
        case MainWindowState::Error:
            message = "错误";
            break;
    }
    updateStatusBar(message);
}

void MainWindowController::refreshUI()
{
    updateUIState();
}

void MainWindowController::logError(const QString& error)
{
    qCritical() << "MainWindowController Error:" << error;
}

void MainWindowController::logInfo(const QString& message)
{
    qInfo() << "MainWindowController Info:" << message;
}

void MainWindowController::logDebug(const QString& message)
{
    qDebug() << "MainWindowController Debug:" << message;
}

// 主窗口事件槽函数
void MainWindowController::onMainWindowShow()
{
    logInfo("主窗口显示");
    updateUIState();
}

void MainWindowController::onMainWindowClose()
{
    logInfo("主窗口关闭");
    shutdown();
}

void MainWindowController::onMainWindowResize()
{
    logInfo("主窗口大小调整");
    saveLayout();
}

void MainWindowController::onMainWindowMove()
{
    logInfo("主窗口移动");
    saveLayout();
}

// 标签列表事件槽函数
void MainWindowController::onTagListContextMenuRequested(const QPoint& position)
{
    logInfo("标签列表右键菜单请求");
    showTagContextMenu(position);
}

void MainWindowController::onTagListSelectionChanged()
{
    logInfo("标签列表选择变化");
    handleTagSelectionChange();
}

// 歌曲列表事件槽函数
void MainWindowController::onSongListContextMenuRequested(const QPoint& position)
{
    logInfo("歌曲列表右键菜单请求");
    showSongContextMenu(position);
}

void MainWindowController::onSongListSelectionChanged()
{
    logInfo("歌曲列表选择变化");
    handleSongSelectionChange();
}

// 音频引擎事件槽函数
void MainWindowController::onAudioStateChanged(AudioTypes::AudioState state)
{
    Q_UNUSED(state)
    logInfo("音频状态改变");
    updatePlaybackControls();
    handlePlaybackStateChange();
}

void MainWindowController::onCurrentSongChanged(const Song& song)
{
    logInfo("当前歌曲改变");
    m_selectedSong = song;
    updateCurrentSongInfo();
    updatePlaybackInfo(song);
    emit songSelectionChanged(song);
}

void MainWindowController::onPositionChanged(qint64 position)
{
    Q_UNUSED(position)
    updateProgressControls();
}

void MainWindowController::onDurationChanged(qint64 duration)
{
    Q_UNUSED(duration)
    updateProgressControls();
}

void MainWindowController::onVolumeChanged(int volume)
{
    updateVolumeDisplay(volume);
}

void MainWindowController::onMutedChanged(bool muted)
{
    logInfo(QString("静音状态改变: %1").arg(muted ? "是" : "否"));
    updateVolumeControls();
}

void MainWindowController::onPlayModeChanged(AudioTypes::PlayMode mode)
{
    Q_UNUSED(mode)
    logInfo("播放模式改变");
    updatePlayModeButton();
}

void MainWindowController::onAudioError(const QString& error)
{
    logError("音频错误: " + error);
    handleAudioEngineError(error);
}

// 标签管理事件槽函数
void MainWindowController::onTagCreated(const Tag& tag)
{
    logInfo("标签创建: " + tag.name());
    refreshTagList();
}

void MainWindowController::onTagUpdated(const Tag& tag)
{
    logInfo("标签更新: " + tag.name());
    refreshTagList();
}

void MainWindowController::onTagDeleted(int tagId, const QString& name)
{
    Q_UNUSED(tagId)
    logInfo("标签删除: " + name);
    refreshTagList();
}

void MainWindowController::onSongAddedToTag(int songId, int tagId)
{
    Q_UNUSED(songId)
    Q_UNUSED(tagId)
    logInfo("歌曲添加到标签");
    refreshSongList();
}

void MainWindowController::onSongRemovedFromTag(int songId, int tagId)
{
    Q_UNUSED(songId)
    Q_UNUSED(tagId)
    logInfo("歌曲从标签移除");
    refreshSongList();
}

// 播放列表事件槽函数
void MainWindowController::onPlaylistCreated(const Playlist& playlist)
{
    logInfo("播放列表创建: " + playlist.name());
    // 更新播放列表UI
}

void MainWindowController::onPlaylistUpdated(const Playlist& playlist)
{
    logInfo("播放列表更新: " + playlist.name());
    // 更新播放列表UI
}

void MainWindowController::onPlaylistDeleted(int playlistId, const QString& name)
{
    Q_UNUSED(playlistId)
    logInfo("播放列表删除: " + name);
    // 更新播放列表UI
}

void MainWindowController::onPlaybackStarted(const Song& song)
{
    logInfo("播放开始: " + song.title());
    setState(MainWindowState::Playing);
}

void MainWindowController::onPlaybackPaused()
{
    logInfo("播放暂停");
    setState(MainWindowState::Paused);
}

void MainWindowController::onPlaybackStopped()
{
    logInfo("播放停止");
    setState(MainWindowState::Ready);
}

// 拖放事件槽函数
void MainWindowController::onDragEnterEvent(QDragEnterEvent* event)
{
    Q_UNUSED(event)
    logInfo("拖放进入事件");
    // 处理拖放进入事件
}

void MainWindowController::onDropEvent(QDropEvent* event)
{
    Q_UNUSED(event)
    logInfo("拖放事件");
    // 处理拖放事件
}

// 缺失的私有方法实现
void MainWindowController::refreshTagList()
{
    logInfo("刷新标签列表");
    updateTagList();
}

void MainWindowController::refreshSongList()
{
    logInfo("刷新歌曲列表");
    updateSongList();
}

void MainWindowController::saveLayout()
{
    logInfo("保存布局");
    // 保存窗口布局
}

void MainWindowController::restoreLayout()
{
    logInfo("恢复布局");
    // 恢复窗口布局
}

void MainWindowController::resetLayout()
{
    logInfo("重置布局");
    // 重置窗口布局
}

void MainWindowController::showTagContextMenu(const QPoint& position)
{
    if (!m_tagListWidget) return;
    QListWidgetItem* item = m_tagListWidget->itemAt(position);
    if (!item) return;
    QString tagName = item->text();
    QMenu menu;
    QAction* editAction = menu.addAction("编辑标签");
    QAction* deleteAction = menu.addAction("删除标签");
    QAction* selected = menu.exec(m_tagListWidget->viewport()->mapToGlobal(position));
    if (selected == editAction) {
        // 弹出编辑对话框
        CreateTagDialog dialog(m_mainWindow);
        dialog.setWindowTitle("编辑标签");
        dialog.findChild<QLineEdit*>("lineEditTagName")->setText(tagName);
        // 预填图片（如有）
        TagDAO tagDao(DatabaseManager::instance());
        Tag tag = tagDao.getTagByName(tagName);
        if (!tag.coverPath().isEmpty()) {
            dialog.findChild<QLabel*>("labelImagePreview")->setPixmap(QPixmap(tag.coverPath()).scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            dialog.findChild<CreateTagDialog*>()->setProperty("m_imagePath", tag.coverPath());
        }
        if (dialog.exec() == QDialog::Accepted) {
            QString newName = dialog.getTagName();
            QString imagePath = dialog.getTagImagePath();
            if (!newName.isEmpty()) {
                editTag(tagName, newName, imagePath);
            }
        }
    } else if (selected == deleteAction) {
        if (QMessageBox::question(m_mainWindow, "删除标签", QString("确定要删除标签 '%1' 吗？").arg(tagName)) == QMessageBox::Yes) {
            deleteTag(tagName);
        }
    }
}

void MainWindowController::editTag(const QString& oldName, const QString& newName, const QString& imagePath)
{
    TagDAO tagDao(DatabaseManager::instance());
    Tag tag = tagDao.getTagByName(oldName);
    if (!tag.isValid()) {
        showErrorDialog("编辑失败", "标签不存在");
        return;
    }
    tag.setName(newName);
    tag.setCoverPath(imagePath);
    if (!tagDao.updateTag(tag)) {
        showErrorDialog("编辑失败", "数据库更新失败");
    } else {
        updateStatusBar("标签编辑成功");
        refreshTagList();
    }
}

void MainWindowController::deleteTag(const QString& name)
{
    TagDAO tagDao(DatabaseManager::instance());
    Tag tag = tagDao.getTagByName(name);
    if (!tag.isValid()) {
        showErrorDialog("删除失败", "标签不存在");
        return;
    }
    if (tag.isSystem()) {
        showErrorDialog("删除失败", "系统标签不可删除");
        return;
    }
    if (!tagDao.deleteTag(tag.id())) {
        showErrorDialog("删除失败", "数据库删除失败");
    } else {
        updateStatusBar("标签删除成功");
        refreshTagList();
    }
}

void MainWindowController::showSongContextMenu(const QPoint& position)
{
    Q_UNUSED(position)
    logInfo("显示歌曲右键菜单");
    // 显示歌曲右键菜单
}

void MainWindowController::showPlaylistContextMenu(const QPoint& position)
{
    Q_UNUSED(position)
    logInfo("显示播放列表右键菜单");
    // 显示播放列表右键菜单
}

void MainWindowController::updateTagList()
{
    logInfo("更新标签列表");
    if (!m_tagListWidget) return;
    m_tagListWidget->clear();
    TagDAO tagDao(DatabaseManager::instance());
    QList<Tag> tags = tagDao.getAllTags();
    
    for (const Tag& tag : tags) {
        // 判断是否为系统标签
        QStringList systemTags = {"我的歌曲", "默认标签", "收藏", "最近播放"};
        bool isSystemTag = systemTags.contains(tag.name());
        
        // 创建自定义标签项控件
        TagListItem* tagItem = new TagListItem(tag.name(), tag.coverPath(), !isSystemTag, !isSystemTag);
        
        // 连接信号
        connect(tagItem, &TagListItem::tagClicked, m_mainWindow, &MainWindow::onTagItemClicked);
        connect(tagItem, &TagListItem::tagDoubleClicked, m_mainWindow, &MainWindow::onTagItemDoubleClicked);
        connect(tagItem, &TagListItem::editRequested, m_mainWindow, &MainWindow::onTagEditRequested);
        
        // 创建QListWidgetItem并设置自定义控件
        QListWidgetItem* item = new QListWidgetItem();
        item->setData(Qt::UserRole, tag.name());
        item->setSizeHint(tagItem->sizeHint());
        
        m_tagListWidget->addItem(item);
        m_tagListWidget->setItemWidget(item, tagItem);
    }
}

void MainWindowController::updateSongList()
{
    logInfo("更新歌曲列表");
    // 更新歌曲列表UI
}

void MainWindowController::updatePlaybackControls()
{
    logInfo("更新播放控件");
    // 更新播放控件状态
}

void MainWindowController::updateVolumeControls()
{
    logInfo("更新音量控件");
    // 更新音量控件状态
}

void MainWindowController::updateProgressControls()
{
    logInfo("更新进度控件");
    // 更新进度控件状态
}

void MainWindowController::updateCurrentSongInfo()
{
    logInfo("更新当前歌曲信息");
    // 更新当前歌曲信息显示
}

void MainWindowController::updatePlayModeButton()
{
    if (!m_audioEngine || !m_playModeButton) return;
    AudioTypes::PlayMode mode = m_audioEngine->playMode();
    QString text;
    QIcon icon;
    switch (mode) {
    case AudioTypes::PlayMode::Sequential:
        text = tr("顺序播放");
        icon = QIcon(":/images/playmode_sequential.png");
        break;
    case AudioTypes::PlayMode::RepeatOne:
        text = tr("单曲循环");
        icon = QIcon(":/images/playmode_repeatone.png");
        break;
    case AudioTypes::PlayMode::Random:
        text = tr("随机播放");
        icon = QIcon(":/images/playmode_shuffle.png");
        break;
    default:
        text = tr("未知模式");
        icon = QIcon();
        break;
    }
    m_playModeButton->setText(text);
    m_playModeButton->setIcon(icon);
}

void MainWindowController::handleTagSelectionChange()
{
    logInfo("处理标签选择变化");
    // 处理标签选择变化
}

void MainWindowController::handleSongSelectionChange()
{
    logInfo("处理歌曲选择变化");
    // 处理歌曲选择变化
}

void MainWindowController::handlePlaybackStateChange()
{
    logInfo("处理播放状态变化");
    // 处理播放状态变化
}

void MainWindowController::handleAudioEngineError(const QString& error)
{
    logError("处理音频引擎错误: " + error);
    handleError(error);
}

void MainWindowController::addSongs(const QStringList& filePaths)
{
    logInfo(QString("批量添加音乐: %1 个文件").arg(filePaths.size()));
    if (filePaths.isEmpty()) return;
    QList<Song> songs;
    for (const QString& path : filePaths) {
        Song song = Song::fromFile(path);
        if (song.isValid()) {
            songs.append(song);
        } else {
            logInfo(QString("无效文件: %1").arg(path));
        }
    }
    if (songs.isEmpty()) {
        showErrorDialog("添加失败", "没有有效的音频文件。");
        return;
    }
    SongDao songDao;
    int inserted = songDao.insertSongs(songs);
    if (inserted > 0) {
        updateStatusBar(QString("成功添加 %1 首歌曲。" ).arg(inserted));
        refreshSongList();
    } else {
        showErrorDialog("添加失败", "歌曲添加到数据库失败。");
    }
}

void MainWindowController::addTag(const QString& name, const QString& imagePath)
{
    logInfo(QString("创建标签: %1, 图片: %2").arg(name, imagePath));
    TagManager* tagManager = TagManager::instance();
    if (tagManager->tagExists(name)) {
        showErrorDialog("标签已存在", "该标签名已存在，请更换。");
        return;
    }
    Tag tag;
    tag.setName(name);
    tag.setCoverPath(imagePath);
    tag.setTagType(Tag::UserTag);
    tag.setCreatedAt(QDateTime::currentDateTime());
    tag.setUpdatedAt(QDateTime::currentDateTime());
    auto result = tagManager->createTag(name, QString(), QColor(), QPixmap(imagePath));
    if (result.success) {
        updateStatusBar("标签创建成功");
        refreshTagList();
    } else {
        showErrorDialog("创建失败", result.message);
    }
}

void MainWindowController::togglePlayMode()
{
    if (!m_audioEngine) return;
    AudioTypes::PlayMode mode = m_audioEngine->playMode();
    using AudioTypes::PlayMode;
    switch (mode) {
    case PlayMode::Sequential:
        m_audioEngine->setPlayMode(PlayMode::RepeatOne);
        break;
    case PlayMode::RepeatOne:
        m_audioEngine->setPlayMode(PlayMode::Random);
        break;
    case PlayMode::Random:
    default:
        m_audioEngine->setPlayMode(PlayMode::Sequential);
        break;
    }
    updatePlayModeButton();
}

Song MainWindowController::getCurrentSong() const
{
    return m_audioEngine ? m_audioEngine->currentSong() : Song();
}
int MainWindowController::getCurrentVolume() const
{
    return m_audioEngine ? m_audioEngine->volume() : 0;
}
qint64 MainWindowController::getCurrentPosition() const
{
    return m_audioEngine ? m_audioEngine->position() : 0;
}
qint64 MainWindowController::getCurrentDuration() const
{
    return m_audioEngine ? m_audioEngine->duration() : 0;
}
void MainWindowController::setCurrentVolume(int volume)
{
    if (m_audioEngine) m_audioEngine->setVolume(volume);
}
void MainWindowController::refreshWindowTitle()
{
    updateWindowTitle();
}

void MainWindowController::refreshTagListPublic()
{
    updateTagList();
}

void MainWindowController::editTagFromMainWindow(const QString& tagName)
{
    // 检查是否为系统标签
    if (tagName == "我的歌曲" || tagName == "我的收藏" || tagName == "最近播放") {
        QMessageBox::warning(m_mainWindow, "警告", "系统标签不能编辑！");
        return;
    }
    
    // 获取标签信息
    TagDAO tagDAO(DatabaseManager::instance());
    Tag tag = tagDAO.getTagByName(tagName);
    if (tag.id() == -1) {
        QMessageBox::warning(m_mainWindow, "错误", "标签不存在！");
        return;
    }
    
    // 创建编辑对话框
    CreateTagDialog dialog(m_mainWindow);
    dialog.setWindowTitle("编辑标签");
    dialog.setTagName(tag.name());
    dialog.setImagePath(tag.coverPath());
    
    if (dialog.exec() == QDialog::Accepted) {
        QString newName = dialog.getTagName().trimmed();
        QString newImagePath = dialog.getTagImagePath();
        
        // 验证新标签名
        if (newName.isEmpty()) {
            QMessageBox::warning(m_mainWindow, "错误", "标签名不能为空！");
            return;
        }
        
        // 检查是否与其他标签重名（除了自己）
        if (newName != tagName && tagDAO.getTagByName(newName).id() != -1) {
            QMessageBox::warning(m_mainWindow, "错误", "标签名已存在！");
            return;
        }
        
        // 调用编辑方法
        editTag(tagName, newName, newImagePath);
    }
}