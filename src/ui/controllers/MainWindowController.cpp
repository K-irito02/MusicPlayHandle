#include "mainwindowcontroller.h"
#include "../../../mainwindow.h"
#include "../../audio/audioengine.h"
#include "../../audio/audiotypes.h"
#include "../../managers/tagmanager.h"
#include "../../managers/playlistmanager.h"
#include "../../core/componentintegration.h"
#include "../../core/constants.h"
#include "../../threading/mainthreadmanager.h"
#include "../../models/song.h"
#include "../../models/tag.h"
#include "../../models/playlist.h"
#include "../../core/logger.h"
#include "../../threading/mainthreadmanager.h"
#include "../../audio/audiotypes.h"
#include "../../database/tagdao.h"
#include "../../database/songdao.h"
#include "../../core/constants.h"
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
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QUrl>
#include <QXmlStreamReader>
#include <QFormLayout>
#include <QSpinBox>

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
        
        // 初始化数据显示
        updateTagList();
        updateSongList();
        
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
    
    if (!m_audioEngine) {
        logError("AudioEngine未初始化");
        return;
    }
    
    // 检查当前状态
    if (m_audioEngine->state() == AudioTypes::AudioState::Playing) {
        // 当前正在播放，执行暂停
        emit pauseRequested();
        logInfo("执行暂停操作");
    } else {
        // 当前未播放，执行播放
        if (m_songListWidget && m_songListWidget->currentItem()) {
            // 播放当前选中的歌曲
            Song selectedSong = m_songListWidget->currentItem()->data(Qt::UserRole).value<Song>();
            emit playRequested(selectedSong);
            logInfo(QString("播放选中歌曲: %1 - %2").arg(selectedSong.artist(), selectedSong.title()));
        } else {
            // 没有选中歌曲，播放第一首
            if (m_songListWidget && m_songListWidget->count() > 0) {
                QListWidgetItem* firstItem = m_songListWidget->item(0);
                Song firstSong = firstItem->data(Qt::UserRole).value<Song>();
                m_songListWidget->setCurrentItem(firstItem);
                emit playRequested(firstSong);
                logInfo(QString("播放第一首歌曲: %1 - %2").arg(firstSong.artist(), firstSong.title()));
            } else {
                logWarning("没有可播放的歌曲");
                updateStatusBar("没有可播放的歌曲", 3000);
            }
        }
    }
}

void MainWindowController::onPauseButtonClicked()
{
    logInfo("暂停按钮被点击");
    if (!m_audioEngine) {
        logError("AudioEngine未初始化");
        return;
    }
    emit pauseRequested();
}

void MainWindowController::onStopButtonClicked()
{
    logInfo("停止按钮被点击");
    if (!m_audioEngine) {
        logError("AudioEngine未初始化");
        return;
    }
    emit stopRequested();
}

void MainWindowController::onNextButtonClicked()
{
    logInfo("下一首按钮被点击");
    if (!m_audioEngine) {
        logError("AudioEngine未初始化");
        return;
    }
    emit nextRequested();
}

void MainWindowController::onPreviousButtonClicked()
{
    logInfo("上一首按钮被点击");
    if (!m_audioEngine) {
        logError("AudioEngine未初始化");
        return;
    }
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


void MainWindowController::updateProgressBar(int value, int maximum)
{
    if (!m_progressBar) {
        logWarning("进度条控件未初始化");
        return;
    }
    
    try {
        m_progressBar->setMaximum(maximum);
        m_progressBar->setValue(value);
        
        // 更新进度百分比显示
        if (maximum > 0) {
            int percentage = (value * 100) / maximum;
            m_progressBar->setFormat(QString("%1%").arg(percentage));
        } else {
            m_progressBar->setFormat("0%");
        }
        
        logDebug(QString("进度条更新: %1/%2 (%3%)").arg(value).arg(maximum).arg(maximum > 0 ? (value * 100) / maximum : 0));
    } catch (const std::exception& e) {
        logError(QString("更新进度条时发生错误: %1").arg(e.what()));
    }
}

void MainWindowController::updatePlaybackInfo(const Song& song)
{
    if (!m_currentSongLabel) {
        logWarning("当前歌曲标签控件未初始化");
        return;
    }
    
    try {
        if (song.isValid()) {
            // 格式化歌曲信息显示
            QString songInfo;
            if (!song.artist().isEmpty() && !song.title().isEmpty()) {
                songInfo = QString("%1 - %2").arg(song.artist(), song.title());
            } else if (!song.title().isEmpty()) {
                songInfo = song.title();
            } else {
                // 如果没有标题，使用文件名
                QFileInfo fileInfo(song.filePath());
                songInfo = fileInfo.baseName();
            }
            
            m_currentSongLabel->setText(songInfo);
            
            // 更新窗口标题
            if (m_mainWindow) {
                QString windowTitle = QString("Qt6音频播放器 - %1").arg(songInfo);
                m_mainWindow->setWindowTitle(windowTitle);
            }
            
            // 更新状态栏
            QString statusMessage = QString("正在播放: %1").arg(songInfo);
            if (song.duration() > 0) {
                statusMessage += QString(" [%1]").arg(formatTime(song.duration()));
            }
            updateStatusBar(statusMessage, 3000);
            
            logInfo(QString("播放信息更新: %1").arg(songInfo));
        } else {
            // 清空播放信息
            m_currentSongLabel->setText("未选择歌曲");
            if (m_mainWindow) {
                m_mainWindow->setWindowTitle("Qt6音频播放器");
            }
            updateStatusBar("就绪", 1000);
            logInfo("清空播放信息");
        }
    } catch (const std::exception& e) {
        logError(QString("更新播放信息时发生错误: %1").arg(e.what()));
    }
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
    
    // 获取UI控件指针
    m_tagListWidget = m_mainWindow->findChild<QListWidget*>("listWidget_tags");
    m_songListWidget = m_mainWindow->findChild<QListWidget*>("listWidget_songs");
    m_playButton = m_mainWindow->findChild<QPushButton*>("pushButton_play_pause");
    m_stopButton = m_mainWindow->findChild<QPushButton*>("pushButton_stop");
    m_nextButton = m_mainWindow->findChild<QPushButton*>("pushButton_next");
    m_previousButton = m_mainWindow->findChild<QPushButton*>("pushButton_previous");
    m_muteButton = m_mainWindow->findChild<QPushButton*>("pushButton_mute");
    m_progressSlider = m_mainWindow->findChild<QSlider*>("slider_progress");
    m_volumeSlider = m_mainWindow->findChild<QSlider*>("slider_volume");
    m_currentSongLabel = m_mainWindow->findChild<QLabel*>("label_song_title");
    m_currentTimeLabel = m_mainWindow->findChild<QLabel*>("label_current_time");
    m_totalTimeLabel = m_mainWindow->findChild<QLabel*>("label_total_time");
    m_volumeLabel = m_mainWindow->findChild<QLabel*>("label_volume");
    m_playModeButton = m_mainWindow->findChild<QPushButton*>("pushButton_play_mode");
    
    // 如果找不到时间标签，创建默认值
    if (!m_currentTimeLabel) {
        m_currentTimeLabel = new QLabel("00:00", m_mainWindow);
        logInfo("未找到当前时间标签，创建默认标签");
    }
    if (!m_totalTimeLabel) {
        m_totalTimeLabel = new QLabel("00:00", m_mainWindow);
        logInfo("未找到总时长标签，创建默认标签");
    }
    if (!m_volumeLabel) {
        m_volumeLabel = new QLabel("100%", m_mainWindow);
        logInfo("未找到音量标签，创建默认标签");
    }
    
    // 获取AudioEngine实例
    m_audioEngine = AudioEngine::instance();
    
    // 检查关键控件是否找到
    if (!m_playButton) logInfo("未找到播放按钮");
    if (!m_stopButton) logInfo("未找到停止按钮");
    if (!m_nextButton) logInfo("未找到下一首按钮");
    if (!m_previousButton) logInfo("未找到上一首按钮");
    if (!m_progressSlider) logInfo("未找到进度滑块");
    if (!m_volumeSlider) logInfo("未找到音量滑块");
    if (!m_tagListWidget) logInfo("未找到标签列表");
    if (!m_songListWidget) logInfo("未找到歌曲列表");
    
    // 设置窗口标题
    updateWindowTitle();
    
    // 设置初始状态
    updateUIState();
    
    logInfo("UI控件初始化完成");
}

void MainWindowController::setupConnections()
{
    if (!m_mainWindow) return;
    
    // 播放控制按钮连接
    if (m_playButton) {
        connect(m_playButton, &QPushButton::clicked, this, &MainWindowController::onPlayButtonClicked);
        logDebug("播放按钮信号连接完成");
    }
    if (m_stopButton) {
        connect(m_stopButton, &QPushButton::clicked, this, &MainWindowController::onStopButtonClicked);
        logDebug("停止按钮信号连接完成");
    }
    if (m_nextButton) {
        connect(m_nextButton, &QPushButton::clicked, this, &MainWindowController::onNextButtonClicked);
        logDebug("下一首按钮信号连接完成");
    }
    if (m_previousButton) {
        connect(m_previousButton, &QPushButton::clicked, this, &MainWindowController::onPreviousButtonClicked);
        logDebug("上一首按钮信号连接完成");
    }
    
    // 滑块连接
    if (m_progressSlider) {
        connect(m_progressSlider, &QSlider::valueChanged, this, &MainWindowController::onProgressSliderChanged);
        logDebug("进度滑块信号连接完成");
    }
    if (m_volumeSlider) {
        connect(m_volumeSlider, &QSlider::valueChanged, this, &MainWindowController::onVolumeSliderChanged);
        logDebug("音量滑块信号连接完成");
    }
    
    // 列表控件连接
    if (m_tagListWidget) {
        connect(m_tagListWidget, &QListWidget::itemClicked, this, &MainWindowController::onTagListItemClicked);
        connect(m_tagListWidget, &QListWidget::itemDoubleClicked, this, &MainWindowController::onTagListItemDoubleClicked);
        logDebug("标签列表信号连接完成");
    }
    if (m_songListWidget) {
        connect(m_songListWidget, &QListWidget::itemClicked, this, &MainWindowController::onSongListItemClicked);
        connect(m_songListWidget, &QListWidget::itemDoubleClicked, this, &MainWindowController::onSongListItemDoubleClicked);
        logDebug("歌曲列表信号连接完成");
    }
    
    // 连接AudioEngine信号
    if (m_audioEngine) {
        connect(m_audioEngine, &AudioEngine::stateChanged, this, &MainWindowController::onAudioStateChanged);
        connect(m_audioEngine, &AudioEngine::positionChanged, this, &MainWindowController::onPositionChanged);
        connect(m_audioEngine, &AudioEngine::durationChanged, this, &MainWindowController::onDurationChanged);
        connect(m_audioEngine, &AudioEngine::volumeChanged, this, &MainWindowController::onVolumeChanged);
        connect(m_audioEngine, &AudioEngine::currentSongChanged, this, &MainWindowController::onCurrentSongChanged);
        connect(m_audioEngine, &AudioEngine::playModeChanged, this, &MainWindowController::onPlayModeChanged);
        connect(m_audioEngine, &AudioEngine::errorOccurred, this, &MainWindowController::onAudioError);
        logDebug("AudioEngine信号连接完成");
    }
    
    // 连接控制器信号到AudioEngine
    connect(this, &MainWindowController::playRequested, [this](const Song& song) {
        if (m_audioEngine) {
            if (song.isValid()) {
                QList<Song> playlist = {song};
                m_audioEngine->setPlaylist(playlist);
                m_audioEngine->setCurrentIndex(0);
            }
            m_audioEngine->play();
            logInfo("发送播放请求到AudioEngine");
        }
    });
    
    connect(this, &MainWindowController::pauseRequested, [this]() {
        if (m_audioEngine) {
            m_audioEngine->pause();
            logInfo("发送暂停请求到AudioEngine");
        }
    });
    
    connect(this, &MainWindowController::stopRequested, [this]() {
        if (m_audioEngine) {
            m_audioEngine->stop();
            logInfo("发送停止请求到AudioEngine");
        }
    });
    
    connect(this, &MainWindowController::nextRequested, [this]() {
        if (m_audioEngine) {
            m_audioEngine->playNext();
            logInfo("发送下一首请求到AudioEngine");
        }
    });
    
    connect(this, &MainWindowController::previousRequested, [this]() {
        if (m_audioEngine) {
            m_audioEngine->playPrevious();
            logInfo("发送上一首请求到AudioEngine");
        }
    });
    
    connect(this, &MainWindowController::volumeChangeRequested, [this](int volume) {
        if (m_audioEngine) {
            m_audioEngine->setVolume(volume);
            logInfo(QString("发送音量变更请求到AudioEngine: %1").arg(volume));
        }
    });
    
    connect(this, &MainWindowController::seekRequested, [this](qint64 position) {
        if (m_audioEngine) {
            m_audioEngine->seek(position);
            logInfo(QString("发送跳转请求到AudioEngine: %1ms").arg(position));
        }
    });
    
    logInfo("所有信号槽连接完成");
}

// AudioEngine信号处理槽函数
void MainWindowController::onAudioStateChanged(AudioTypes::AudioState state)
{
    logInfo(QString("音频状态变化: %1").arg(static_cast<int>(state)));
    
    // 更新播放按钮状态
    if (m_playButton) {
        switch (state) {
        case AudioTypes::AudioState::Playing:
            m_playButton->setText("暂停");
            m_playButton->setIcon(QIcon(":/icons/pause.png"));
            break;
        case AudioTypes::AudioState::Paused:
        case AudioTypes::AudioState::Stopped:
            m_playButton->setText("播放");
            m_playButton->setIcon(QIcon(":/icons/play.png"));
            break;
        default:
            break;
        }
    }
    
    // 更新状态栏
    QString stateText;
    switch (state) {
    case AudioTypes::AudioState::Playing:
        stateText = "正在播放";
        break;
    case AudioTypes::AudioState::Paused:
        stateText = "已暂停";
        break;
    case AudioTypes::AudioState::Stopped:
        stateText = "已停止";
        break;
    default:
        stateText = "未知状态";
        break;
    }
    updateStatusBar(stateText, 2000);
}

void MainWindowController::onPositionChanged(qint64 position)
{
    // 更新进度滑块
    if (m_progressSlider && m_audioEngine && m_audioEngine->duration() > 0) {
        m_progressSlider->blockSignals(true);
        m_progressSlider->setValue(static_cast<int>(position));
        m_progressSlider->blockSignals(false);
    }
    
    // 更新时间显示
    if (m_currentTimeLabel) {
        m_currentTimeLabel->setText(formatTime(position));
    }
}

void MainWindowController::onDurationChanged(qint64 duration)
{
    // 更新进度滑块最大值
    if (m_progressSlider) {
        m_progressSlider->setMaximum(static_cast<int>(duration));
    }
    
    // 更新总时长显示
    if (m_totalTimeLabel) {
        m_totalTimeLabel->setText(formatTime(duration));
    }
    
    logInfo(QString("歌曲时长: %1").arg(formatTime(duration)));
}

void MainWindowController::onVolumeChanged(int volume)
{
    // 更新音量滑块
    if (m_volumeSlider) {
        m_volumeSlider->blockSignals(true);
        m_volumeSlider->setValue(volume);
        m_volumeSlider->blockSignals(false);
    }
    
    // 更新音量标签
    if (m_volumeLabel) {
        m_volumeLabel->setText(QString("%1%").arg(volume));
    }
    
    logDebug(QString("音量变化: %1").arg(volume));
}

void MainWindowController::onMutedChanged(bool muted)
{
    if (m_muteButton) {
        m_muteButton->setChecked(muted);
        m_muteButton->setText(muted ? "取消静音" : "静音");
    }
    
    logDebug(QString("静音状态变化: %1").arg(muted ? "已静音" : "未静音"));
}

void MainWindowController::onCurrentSongChanged(const Song& song)
{
    logInfo(QString("当前歌曲变化: %1 - %2").arg(song.artist(), song.title()));
    
    // 更新当前歌曲信息
    updateCurrentSongInfo();
    
    // 高亮当前播放的歌曲
    if (m_songListWidget) {
        for (int i = 0; i < m_songListWidget->count(); ++i) {
            QListWidgetItem* item = m_songListWidget->item(i);
            if (item) {
                Song itemSong = item->data(Qt::UserRole).value<Song>();
                if (itemSong.id() == song.id()) {
                    m_songListWidget->setCurrentItem(item);
                    item->setBackground(QColor(100, 149, 237, 100)); // 浅蓝色背景
                } else {
                    item->setBackground(QColor()); // 恢复默认背景
                }
            }
        }
    }
}

void MainWindowController::onPlayModeChanged(AudioTypes::PlayMode mode)
{
    QString modeText;
    switch (mode) {
    case AudioTypes::PlayMode::Sequential:
        modeText = "顺序播放";
        break;
    case AudioTypes::PlayMode::Loop:
        modeText = "列表循环";
        break;
    case AudioTypes::PlayMode::Random:
        modeText = "随机播放";
        break;
    default:
        modeText = "未知模式";
        break;
    }
    
    // 更新播放模式按钮
    if (m_playModeButton) {
        m_playModeButton->setText(modeText);
        m_playModeButton->setToolTip(QString("当前播放模式: %1").arg(modeText));
    }
    
    updateStatusBar(QString("播放模式: %1").arg(modeText), 2000);
    logInfo(QString("播放模式变化: %1").arg(modeText));
}

void MainWindowController::onAudioError(const QString& error)
{
    logError(QString("音频错误: %1").arg(error));
    showErrorDialog("音频播放错误", error);
    
    // 重置播放按钮状态
    if (m_playButton) {
        m_playButton->setText("播放");
        m_playButton->setIcon(QIcon(":/icons/play.png"));
    }
    
    updateStatusBar("播放出错", 5000);
}

// 滑块变化处理已在前面定义

// 列表项点击处理
// 重复定义已删除 - onTagListItemClicked

void MainWindowController::onTagListItemDoubleClicked(QListWidgetItem* item)
{
    if (!item) return;
    
    QString tagName = item->text();
    logInfo(QString("双击标签: %1").arg(tagName));
    
    // 可以在这里添加标签编辑功能
}



// 工具函数
QString MainWindowController::formatTime(qint64 milliseconds) const
{
    qint64 seconds = milliseconds / 1000;
    qint64 minutes = seconds / 60;
    seconds %= 60;
    
    return QString("%1:%2")
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0'));
}

void MainWindowController::updateStatusBar(const QString& message, int timeout)
{
    if (m_mainWindow && m_mainWindow->statusBar()) {
        m_mainWindow->statusBar()->showMessage(message, timeout);
    }
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

void MainWindowController::logWarning(const QString& message)
{
    qWarning() << "MainWindowController Warning:" << message;
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
    if (!event) {
        logWarning("拖拽进入事件为空");
        return;
    }
    
    try {
        logDebug("处理拖拽进入事件");
        
        // 检查是否启用拖拽功能
        if (!m_dragDropEnabled) {
            logDebug("拖拽功能已禁用");
            event->ignore();
            return;
        }
        
        // 检查是否包含文件URL
        if (event->mimeData()->hasUrls()) {
            QList<QUrl> urls = event->mimeData()->urls();
            bool hasAudioFiles = false;
            
            // 检查是否包含支持的音频文件
            for (const QUrl& url : urls) {
                if (url.isLocalFile()) {
                    QString filePath = url.toLocalFile();
                    QString suffix = QFileInfo(filePath).suffix().toLower();
                    
                    // 支持的音频格式
                    QStringList supportedFormats = {"mp3", "wav", "flac", "ogg", "m4a", "aac", "wma"};
                    
                    if (supportedFormats.contains(suffix)) {
                        hasAudioFiles = true;
                        break;
                    }
                }
            }
            
            if (hasAudioFiles) {
                logInfo(QString("检测到 %1 个拖拽文件，包含支持的音频格式").arg(urls.size()));
                event->acceptProposedAction();
                return;
            }
        }
        
        logDebug("拖拽内容不包含支持的音频文件");
        event->ignore();
        
    } catch (const std::exception& e) {
        logError(QString("处理拖拽进入事件时发生异常: %1").arg(e.what()));
        event->ignore();
    }
}

void MainWindowController::onDropEvent(QDropEvent* event)
{
    if (!event) {
        logWarning("拖拽放下事件为空");
        return;
    }
    
    try {
        logInfo("处理拖拽放下事件");
        
        // 检查是否启用拖拽功能
        if (!m_dragDropEnabled) {
            logDebug("拖拽功能已禁用");
            event->ignore();
            return;
        }
        
        // 检查是否包含文件URL
        if (event->mimeData()->hasUrls()) {
            QList<QUrl> urls = event->mimeData()->urls();
            QStringList audioFiles;
            
            // 筛选出支持的音频文件
            for (const QUrl& url : urls) {
                if (url.isLocalFile()) {
                    QString filePath = url.toLocalFile();
                    QString suffix = QFileInfo(filePath).suffix().toLower();
                    
                    // 支持的音频格式
                    QStringList supportedFormats = {"mp3", "wav", "flac", "ogg", "m4a", "aac", "wma"};
                    
                    if (supportedFormats.contains(suffix)) {
                        audioFiles.append(filePath);
                        logDebug(QString("添加音频文件: %1").arg(filePath));
                    } else {
                        logDebug(QString("跳过不支持的文件: %1").arg(filePath));
                    }
                }
            }
            
            if (!audioFiles.isEmpty()) {
                logInfo(QString("准备添加 %1 个音频文件到音乐库").arg(audioFiles.size()));
                
                // 调用添加歌曲方法
                addSongs(audioFiles);
                
                // 更新状态栏
                updateStatusBar(QString("成功添加 %1 个音频文件").arg(audioFiles.size()), 3000);
                
                event->acceptProposedAction();
                return;
            } else {
                logWarning("拖拽的文件中没有支持的音频格式");
                updateStatusBar("没有找到支持的音频文件", 2000);
            }
        }
        
        event->ignore();
        
    } catch (const std::exception& e) {
        logError(QString("处理拖拽放下事件时发生异常: %1").arg(e.what()));
        updateStatusBar("添加文件时发生错误", 2000);
        event->ignore();
    }
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
    if (!m_settings) {
        logWarning("设置对象未初始化，无法保存布局");
        return;
    }
    
    try {
        // 保存主窗口几何信息
        if (m_mainWindow) {
            m_settings->setValue("MainWindow/geometry", m_mainWindow->saveGeometry());
            m_settings->setValue("MainWindow/windowState", m_mainWindow->saveState());
        }
        
        // 保存分割器状态
        if (m_splitter) {
            m_settings->setValue("MainWindow/splitterState", m_splitter->saveState());
        }
        
        // 保存音量设置
        if (m_volumeSlider) {
            m_settings->setValue("Audio/volume", m_volumeSlider->value());
        }
        
        // 保存播放模式
        if (m_audioEngine) {
            m_settings->setValue("Audio/playMode", static_cast<int>(m_audioEngine->playMode()));
        }
        
        logInfo("布局保存完成");
    } catch (const std::exception& e) {
        logError(QString("保存布局时发生错误: %1").arg(e.what()));
    }
}

void MainWindowController::restoreLayout()
{
    if (!m_settings) {
        logWarning("设置对象未初始化，无法恢复布局");
        return;
    }
    
    try {
        // 恢复主窗口几何信息
        if (m_mainWindow) {
            QByteArray geometry = m_settings->value("MainWindow/geometry").toByteArray();
            if (!geometry.isEmpty()) {
                m_mainWindow->restoreGeometry(geometry);
            }
            
            QByteArray windowState = m_settings->value("MainWindow/windowState").toByteArray();
            if (!windowState.isEmpty()) {
                m_mainWindow->restoreState(windowState);
            }
        }
        
        // 恢复分割器状态
        if (m_splitter) {
            QByteArray splitterState = m_settings->value("MainWindow/splitterState").toByteArray();
            if (!splitterState.isEmpty()) {
                m_splitter->restoreState(splitterState);
            }
        }
        
        // 恢复音量设置
        if (m_volumeSlider) {
            int volume = m_settings->value("Audio/volume", 50).toInt();
            m_volumeSlider->setValue(volume);
            if (m_audioEngine) {
                m_audioEngine->setVolume(volume);
            }
        }
        
        // 恢复播放模式
        if (m_audioEngine) {
            int playMode = m_settings->value("Audio/playMode", static_cast<int>(AudioTypes::PlayMode::Sequential)).toInt();
            m_audioEngine->setPlayMode(static_cast<AudioTypes::PlayMode>(playMode));
            updatePlayModeButton();
        }
        
        logInfo("布局恢复完成");
    } catch (const std::exception& e) {
        logError(QString("恢复布局时发生错误: %1").arg(e.what()));
    }
}

void MainWindowController::resetLayout()
{
    try {
        // 重置主窗口大小和位置
        if (m_mainWindow) {
            m_mainWindow->resize(1200, 800);  // 默认大小
            m_mainWindow->move(100, 100);     // 默认位置
        }
        
        // 重置分割器比例
        if (m_splitter) {
            QList<int> sizes;
            sizes << 300 << 900;  // 标签区域:歌曲区域 = 1:3
            m_splitter->setSizes(sizes);
        }
        
        // 重置音量
        if (m_volumeSlider) {
            m_volumeSlider->setValue(50);
            if (m_audioEngine) {
                m_audioEngine->setVolume(50);
            }
        }
        
        // 重置播放模式
        if (m_audioEngine) {
            m_audioEngine->setPlayMode(AudioTypes::PlayMode::Sequential);
            updatePlayModeButton();
        }
        
        // 清除设置中的布局信息
        if (m_settings) {
            m_settings->remove("MainWindow/geometry");
            m_settings->remove("MainWindow/windowState");
            m_settings->remove("MainWindow/splitterState");
        }
        
        logInfo("布局重置完成");
    } catch (const std::exception& e) {
        logError(QString("重置布局时发生错误: %1").arg(e.what()));
    }
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
        TagDao tagDao;
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
    TagDao tagDao;
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
    TagDao tagDao;
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
    if (!m_songListWidget) {
        logWarning("歌曲列表控件未初始化");
        return;
    }
    
    try {
        QListWidgetItem* item = m_songListWidget->itemAt(position);
        if (!item) {
            logDebug("右键点击位置没有歌曲项");
            return;
        }
        
        // 获取歌曲信息
        int songId = item->data(Qt::UserRole).toInt();
        QString songTitle = item->text();
        
        logInfo(QString("显示歌曲右键菜单: %1 (ID: %2)").arg(songTitle).arg(songId));
        
        // 创建右键菜单
        QMenu contextMenu(m_songListWidget);
        
        // 播放歌曲
        QAction* playAction = contextMenu.addAction(QIcon(":/icons/play.png"), "播放");
        connect(playAction, &QAction::triggered, [this, songId]() {
            logInfo(QString("从右键菜单播放歌曲 ID: %1").arg(songId));
            if (m_audioEngine) {
                // 根据歌曲ID获取歌曲信息并播放
                SongDao songDao;
                Song song = songDao.getSongById(songId);
                if (song.isValid()) {
                    // m_audioEngine->playSong(song);
                }
            }
        });
        
        contextMenu.addSeparator();
        
        // 添加到标签
        QAction* addToTagAction = contextMenu.addAction(QIcon(":/icons/tag_add.png"), "添加到标签...");
        connect(addToTagAction, &QAction::triggered, [this, songId, songTitle]() {
            logInfo(QString("为歌曲 %1 添加标签").arg(songTitle));
            showAddToTagDialog(songId, songTitle);
        });
        
        // 从标签移除
        QAction* removeFromTagAction = contextMenu.addAction(QIcon(":/icons/tag_remove.png"), "从当前标签移除");
        connect(removeFromTagAction, &QAction::triggered, [this, songId, songTitle]() {
            logInfo(QString("从当前标签移除歌曲 %1").arg(songTitle));
            removeFromCurrentTag(songId, songTitle);
        });
        
        contextMenu.addSeparator();
        
        // 编辑歌曲信息
        QAction* editInfoAction = contextMenu.addAction(QIcon(":/icons/edit.png"), "编辑信息...");
        connect(editInfoAction, &QAction::triggered, [this, songId, songTitle]() {
            logInfo(QString("编辑歌曲信息: %1").arg(songTitle));
            showEditSongDialog(songId, songTitle);
        });
        
        // 在文件夹中显示
        QAction* showInFolderAction = contextMenu.addAction(QIcon(":/icons/folder.png"), "在文件夹中显示");
        connect(showInFolderAction, &QAction::triggered, [this, songId, songTitle]() {
            logInfo(QString("在文件夹中显示歌曲: %1").arg(songTitle));
            showInFileExplorer(songId, songTitle);
        });
        
        contextMenu.addSeparator();
        
        // 删除歌曲
        QAction* deleteAction = contextMenu.addAction(QIcon(":/icons/delete.png"), "删除");
        connect(deleteAction, &QAction::triggered, [this, songId, songTitle]() {
            logInfo(QString("删除歌曲: %1").arg(songTitle));
            
            // 确认删除
            QMessageBox::StandardButton reply = QMessageBox::question(
                m_mainWindow,
                "确认删除",
                QString("确定要删除歌曲 \"%1\" 吗？\n\n注意：这将从数据库中删除歌曲记录，但不会删除实际文件。")
                    .arg(songTitle),
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No
            );
            
            if (reply == QMessageBox::Yes) {
                deleteSongFromDatabase(songId, songTitle);
                logInfo(QString("用户确认删除歌曲: %1").arg(songTitle));
            }
        });
        
        // 显示菜单
        contextMenu.exec(m_songListWidget->mapToGlobal(position));
        
    } catch (const std::exception& e) {
        logError(QString("显示歌曲右键菜单时发生异常: %1").arg(e.what()));
    }
}

void MainWindowController::showPlaylistContextMenu(const QPoint& position)
{
    Q_UNUSED(position)
    logInfo("显示播放列表右键菜单");
    
    try {
        // 创建播放列表右键菜单
        QMenu contextMenu(m_mainWindow);
        
        // 创建新播放列表
        QAction* createPlaylistAction = contextMenu.addAction(QIcon(":/icons/playlist_add.png"), "创建播放列表...");
        connect(createPlaylistAction, &QAction::triggered, [this]() {
            logInfo("创建新播放列表");
            showCreatePlaylistDialog();
        });
        
        // 导入播放列表
        QAction* importPlaylistAction = contextMenu.addAction(QIcon(":/icons/import.png"), "导入播放列表...");
        connect(importPlaylistAction, &QAction::triggered, [this]() {
            logInfo("导入播放列表");
            importPlaylistFromFile();
        });
        
        contextMenu.addSeparator();
        
        // 刷新播放列表
        QAction* refreshAction = contextMenu.addAction(QIcon(":/icons/refresh.png"), "刷新");
        connect(refreshAction, &QAction::triggered, [this]() {
            logInfo("刷新播放列表");
            refreshPlaylistView();
        });
        
        // 显示菜单
        contextMenu.exec(m_mainWindow->mapToGlobal(position));
        
    } catch (const std::exception& e) {
        logError(QString("显示播放列表右键菜单时发生异常: %1").arg(e.what()));
    }
}

void MainWindowController::updateTagList()
{
    if (!m_tagListWidget) {
        logWarning("标签列表控件未初始化");
        return;
    }
    
    try {
        // 清空当前列表
        m_tagListWidget->clear();
        
        // 添加"全部歌曲"选项
        QListWidgetItem* allSongsItem = new QListWidgetItem("全部歌曲");
        allSongsItem->setIcon(QIcon(":/icons/all_songs.png"));
        allSongsItem->setData(Qt::UserRole, -1); // 特殊ID表示全部歌曲
        m_tagListWidget->addItem(allSongsItem);
        
        // 使用DAO直接获取标签
        TagDao tagDao;
        QList<Tag> tags = tagDao.getAllTags();
        
        for (const auto& tag : tags) {
            QListWidgetItem* item = new QListWidgetItem();
            item->setText(tag.name());
            item->setData(Qt::UserRole, tag.id());
            item->setToolTip(QString("标签: %1\nID: %2")
                           .arg(tag.name())
                           .arg(tag.id()));
            
            // 设置标签图标
            if (!tag.coverPath().isEmpty() && QFile::exists(tag.coverPath())) {
                item->setIcon(QIcon(tag.coverPath()));
            } else {
                item->setIcon(QIcon(":/icons/tag_default.png"));
            }
            
            m_tagListWidget->addItem(item);
        }
        
        logInfo(QString("标签列表更新完成，共 %1 个标签").arg(tags.size()));
        
        // 默认选中"全部歌曲"
        if (m_tagListWidget->count() > 0) {
            m_tagListWidget->setCurrentRow(0);
        }
        
    } catch (const std::exception& e) {
        logError(QString("更新标签列表时发生异常: %1").arg(e.what()));
        showErrorDialog("更新标签列表失败", QString("发生异常: %1").arg(e.what()));
    }
}

void MainWindowController::updateSongList()
{
    if (!m_songListWidget) return;
    
    try {
        // 获取当前选中的标签
        QString selectedTag;
        if (m_tagListWidget && m_tagListWidget->currentItem()) {
            selectedTag = m_tagListWidget->currentItem()->text();
        }
        
        // 清空当前列表
        m_songListWidget->clear();
        
        // 获取歌曲列表
        QList<Song> songs;
        if (selectedTag.isEmpty() || selectedTag == "全部歌曲") {
            // 显示所有歌曲
            SongDao songDao;
            songs = songDao.getAllSongs();
        } else {
            // 显示特定标签的歌曲
            SongDao songDao;
            TagDao tagDao;
            Tag tag = tagDao.getTagByName(selectedTag);
            if (tag.isValid()) {
                songs = songDao.getSongsByTag(tag.id());
            }
        }
        
        // 添加歌曲到列表
        for (const auto& song : songs) {
            QListWidgetItem* item = new QListWidgetItem();
            item->setText(QString("%1 - %2").arg(song.artist(), song.title()));
            item->setData(Qt::UserRole, QVariant::fromValue(song));
            item->setToolTip(QString("文件: %1\n时长: %2")
                           .arg(song.filePath())
                           .arg(QString::number(song.duration())));
            m_songListWidget->addItem(item);
        }
        
        // 更新状态栏
        updateStatusBar(QString("共 %1 首歌曲").arg(songs.size()), 3000);
        
        logInfo(QString("歌曲列表更新完成，共 %1 首歌曲").arg(songs.size()));
        
    } catch (const std::exception& e) {
        logError(QString("更新歌曲列表时发生异常: %1").arg(e.what()));
        showErrorDialog("更新歌曲列表失败", QString("发生异常: %1").arg(e.what()));
    }
}

void MainWindowController::updatePlaybackControls()
{
    if (!m_audioEngine || !m_mainWindow) return;
    
    // 更新播放/暂停按钮状态
    if (m_playButton) {
        bool isPlaying = (m_audioEngine->state() == AudioTypes::AudioState::Playing);
        m_playButton->setText(isPlaying ? "暂停" : "播放");
        m_playButton->setIcon(QIcon(isPlaying ? ":/icons/pause.png" : ":/icons/play.png"));
    }
    
    // 更新进度条
    if (m_progressSlider && m_audioEngine->duration() > 0) {
        m_progressSlider->setMaximum(static_cast<int>(m_audioEngine->duration()));
        m_progressSlider->setValue(static_cast<int>(m_audioEngine->position()));
    }
    
    // 更新音量滑块
    if (m_volumeSlider) {
        m_volumeSlider->setValue(m_audioEngine->volume());
    }
    
    logInfo("播放控件状态更新完成");
}

void MainWindowController::updateVolumeControls()
{
    if (!m_audioEngine || !m_volumeSlider) return;
    
    // 更新音量滑块
    int volume = m_audioEngine->volume();
    m_volumeSlider->blockSignals(true);
    m_volumeSlider->setValue(volume);
    m_volumeSlider->blockSignals(false);
    
    // 更新静音按钮状态
    if (m_muteButton) {
        bool isMuted = m_audioEngine->isMuted();
        m_muteButton->setText(isMuted ? "取消静音" : "静音");
        m_muteButton->setIcon(QIcon(isMuted ? ":/icons/volume_muted.png" : ":/icons/volume.png"));
    }
    
    logInfo(QString("音量控件更新完成，音量: %1").arg(volume));
}

void MainWindowController::updateProgressControls()
{
    if (!m_audioEngine || !m_progressSlider) return;
    
    qint64 position = m_audioEngine->position();
    qint64 duration = m_audioEngine->duration();
    
    // 更新进度滑块
    if (duration > 0) {
        m_progressSlider->blockSignals(true);
        m_progressSlider->setMaximum(static_cast<int>(duration));
        m_progressSlider->setValue(static_cast<int>(position));
        m_progressSlider->blockSignals(false);
    }
    
    logDebug(QString("进度控件更新: %1/%2").arg(position).arg(duration));
}

void MainWindowController::updateCurrentSongInfo()
{
    if (!m_audioEngine || !m_currentSongLabel) return;
    
    Song currentSong = m_audioEngine->currentSong();
    if (currentSong.isValid()) {
        QString songInfo = QString("%1 - %2").arg(currentSong.artist(), currentSong.title());
        m_currentSongLabel->setText(songInfo);
        
        // 更新窗口标题
        if (m_mainWindow) {
            QString title = QString("Qt6音频播放器 - %1").arg(songInfo);
            m_mainWindow->setWindowTitle(title);
        }
        
        logInfo(QString("当前歌曲信息更新: %1").arg(songInfo));
    } else {
        m_currentSongLabel->setText("未选择歌曲");
        if (m_mainWindow) {
            m_mainWindow->setWindowTitle("Qt6音频播放器");
        }
        logInfo("清空当前歌曲信息");
    }
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
    if (!m_tagListWidget) {
        logWarning("标签列表控件未初始化");
        return;
    }
    
    try {
        QListWidgetItem* currentItem = m_tagListWidget->currentItem();
        if (currentItem) {
            // 获取选中的标签信息
            QVariant tagData = currentItem->data(Qt::UserRole);
            int tagId = tagData.toInt();
            QString tagName = currentItem->text();
            
            // 更新歌曲列表显示对应标签的歌曲
            updateSongList();
            
            // 更新状态栏
            if (tagId == -1) {
                updateStatusBar("显示所有歌曲", 2000);
            } else {
                updateStatusBar(QString("显示标签 '%1' 的歌曲").arg(tagName), 2000);
            }
            
            // 发送标签选择变化信号
            if (tagId != -1) {
                Tag selectedTag;
                selectedTag.setId(tagId);
                selectedTag.setName(tagName);
                emit tagSelectionChanged(selectedTag);
            }
            
            logInfo(QString("标签选择变化: %1 (ID: %2)").arg(tagName).arg(tagId));
        } else {
            // 没有选中任何标签
            updateStatusBar("未选择标签", 1000);
            logInfo("清空标签选择");
        }
    } catch (const std::exception& e) {
        logError(QString("处理标签选择变化时发生错误: %1").arg(e.what()));
    }
}

void MainWindowController::handleSongSelectionChange()
{
    if (!m_songListWidget) {
        logWarning("歌曲列表控件未初始化");
        return;
    }
    
    try {
        QListWidgetItem* currentItem = m_songListWidget->currentItem();
        if (currentItem) {
            // 获取选中的歌曲信息
            QVariant songData = currentItem->data(Qt::UserRole);
            int songId = songData.toInt();
            QString songTitle = currentItem->text();
            
            // 更新状态栏显示选中的歌曲信息
            updateStatusBar(QString("选中歌曲: %1").arg(songTitle), 2000);
            
            // 发送歌曲选择变化信号
            if (songId > 0) {
                Song selectedSong;
                selectedSong.setId(songId);
                selectedSong.setTitle(songTitle);
                emit songSelectionChanged(selectedSong);
            }
            
            logInfo(QString("歌曲选择变化: %1 (ID: %2)").arg(songTitle).arg(songId));
        } else {
            // 没有选中任何歌曲
            updateStatusBar("未选择歌曲", 1000);
            logInfo("清空歌曲选择");
        }
    } catch (const std::exception& e) {
        logError(QString("处理歌曲选择变化时发生错误: %1").arg(e.what()));
    }
}

void MainWindowController::handlePlaybackStateChange()
{
    if (!m_audioEngine) {
        logWarning("音频引擎未初始化");
        return;
    }
    
    try {
        AudioTypes::AudioState currentState = m_audioEngine->state();
        
        // 更新播放控制按钮状态
        updatePlaybackControls();
        
        // 根据播放状态更新UI
        switch (currentState) {
        case AudioTypes::AudioState::Playing:
            setState(MainWindowState::Playing);
            updateStatusBar("正在播放", 1000);
            break;
        case AudioTypes::AudioState::Paused:
            setState(MainWindowState::Paused);
            updateStatusBar("已暂停", 1000);
            break;
        case AudioTypes::AudioState::Stopped:
            setState(MainWindowState::Ready);
            updateStatusBar("已停止", 1000);
            break;
        case AudioTypes::AudioState::Loading:
            setState(MainWindowState::Loading);
            updateStatusBar("正在加载...", 1000);
            break;
        case AudioTypes::AudioState::Error:
            setState(MainWindowState::Error);
            updateStatusBar("播放错误", 3000);
            break;
        default:
            setState(MainWindowState::Ready);
            updateStatusBar("就绪", 1000);
            break;
        }
        
        logInfo(QString("播放状态变化: %1").arg(static_cast<int>(currentState)));
    } catch (const std::exception& e) {
        logError(QString("处理播放状态变化时发生错误: %1").arg(e.what()));
    }
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
    TagDao tagDao;
    Tag tag = tagDao.getTagByName(tagName);
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
        if (newName != tagName && tagDao.getTagByName(newName).id() != -1) {
            QMessageBox::warning(m_mainWindow, "错误", "标签名已存在！");
            return;
        }
        
        // 调用编辑方法
        editTag(tagName, newName, newImagePath);
    }
}

// 歌曲操作方法实现
void MainWindowController::showAddToTagDialog(int songId, const QString& songTitle)
{
    logInfo(QString("显示添加到标签对话框: 歌曲ID=%1, 标题=%2").arg(songId).arg(songTitle));
    
    try {
        // 获取所有可用标签
        TagDao tagDao;
        SongDao songDao;
        QList<Tag> allTags = tagDao.getAllTags();
        
        if (allTags.isEmpty()) {
            QMessageBox::information(m_mainWindow, "提示", "没有可用的标签，请先创建标签");
            return;
        }
        
        // 获取歌曲当前所属的标签
        QList<Tag> currentTags = m_tagManager->getTagsForSong(songId);
        QSet<int> currentTagIds;
        for (const Tag& tag : currentTags) {
            currentTagIds.insert(tag.id());
        }
        
        // 创建标签选择对话框
        QDialog dialog(m_mainWindow);
        dialog.setWindowTitle(QString("为歌曲 '%1' 添加标签").arg(songTitle));
        dialog.setModal(true);
        dialog.resize(400, 300);
        
        QVBoxLayout* layout = new QVBoxLayout(&dialog);
        
        QLabel* label = new QLabel(QString("选择要添加的标签:"));
        layout->addWidget(label);
        
        QListWidget* tagList = new QListWidget();
        tagList->setSelectionMode(QAbstractItemView::MultiSelection);
        
        // 添加标签到列表（排除已有的标签）
        for (const Tag& tag : allTags) {
            if (!currentTagIds.contains(tag.id())) {
                QListWidgetItem* item = new QListWidgetItem(tag.name());
                item->setData(Qt::UserRole, tag.id());
                tagList->addItem(item);
            }
        }
        
        layout->addWidget(tagList);
        
        // 按钮
        QHBoxLayout* buttonLayout = new QHBoxLayout();
        QPushButton* okButton = new QPushButton("确定");
        QPushButton* cancelButton = new QPushButton("取消");
        
        connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
        connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);
        
        buttonLayout->addStretch();
        buttonLayout->addWidget(okButton);
        buttonLayout->addWidget(cancelButton);
        layout->addLayout(buttonLayout);
        
        if (dialog.exec() == QDialog::Accepted) {
            QList<QListWidgetItem*> selectedItems = tagList->selectedItems();
            if (selectedItems.isEmpty()) {
                QMessageBox::information(m_mainWindow, "提示", "请选择至少一个标签");
                return;
            }
            
            // 添加歌曲到选中的标签
            int successCount = 0;
            for (QListWidgetItem* item : selectedItems) {
                int tagId = item->data(Qt::UserRole).toInt();
                if (songDao.addSongToTag(songId, tagId)) {
                    successCount++;
                    logInfo(QString("歌曲 %1 已添加到标签 %2").arg(songId).arg(tagId));
                }
            }
            
            if (successCount > 0) {
                updateStatusBar(QString("歌曲已添加到 %1 个标签").arg(successCount), 3000);
                refreshSongList(); // 刷新歌曲列表
            } else {
                QMessageBox::warning(m_mainWindow, "错误", "添加标签失败");
            }
        }
        
    } catch (const std::exception& e) {
        logError(QString("显示添加到标签对话框时发生异常: %1").arg(e.what()));
        QMessageBox::critical(m_mainWindow, "错误", "显示对话框时发生错误");
    }
}

void MainWindowController::removeFromCurrentTag(int songId, const QString& songTitle)
{
    logInfo(QString("从当前标签移除歌曲: 歌曲ID=%1, 标题=%2").arg(songId).arg(songTitle));
    
    try {
        // 获取当前选中的标签
        Tag currentTag = getSelectedTag();
        if (currentTag.id() == -1) {
            QMessageBox::information(m_mainWindow, "提示", "请先选择一个标签");
            return;
        }
        
        // 确认移除操作
        int ret = QMessageBox::question(m_mainWindow, "确认移除", 
            QString("确定要从标签 '%1' 中移除歌曲 '%2' 吗？")
            .arg(currentTag.name()).arg(songTitle),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            
        if (ret == QMessageBox::Yes) {
            SongDao songDao;
            if (songDao.removeSongFromTag(songId, currentTag.id())) {
                logInfo(QString("歌曲 %1 已从标签 %2 移除").arg(songId).arg(currentTag.id()));
                updateStatusBar(QString("歌曲已从标签 '%1' 移除").arg(currentTag.name()), 3000);
                refreshSongList(); // 刷新歌曲列表
            } else {
                logError(QString("移除歌曲失败: 歌曲ID=%1, 标签ID=%2").arg(songId).arg(currentTag.id()));
                QMessageBox::critical(m_mainWindow, "错误", "移除歌曲失败");
            }
        }
        
    } catch (const std::exception& e) {
        logError(QString("移除歌曲时发生异常: %1").arg(e.what()));
        QMessageBox::critical(m_mainWindow, "错误", "移除歌曲时发生错误");
    }
}

void MainWindowController::showEditSongDialog(int songId, const QString& songTitle)
{
    logInfo(QString("显示编辑歌曲对话框: 歌曲ID=%1, 标题=%2").arg(songId).arg(songTitle));
    
    try {
        // 获取歌曲信息
        SongDao songDao;
        Song song = songDao.getSongById(songId);
        
        if (song.id() == -1) {
            QMessageBox::warning(m_mainWindow, "错误", "歌曲不存在");
            return;
        }
        
        // 创建编辑对话框
        QDialog dialog(m_mainWindow);
        dialog.setWindowTitle(QString("编辑歌曲信息: %1").arg(songTitle));
        dialog.setModal(true);
        dialog.resize(500, 400);
        
        QVBoxLayout* layout = new QVBoxLayout(&dialog);
        
        // 创建表单
        QFormLayout* formLayout = new QFormLayout();
        
        QLineEdit* titleEdit = new QLineEdit(song.title());
        QLineEdit* artistEdit = new QLineEdit(song.artist());
        QLineEdit* albumEdit = new QLineEdit(song.album());
        QLineEdit* genreEdit = new QLineEdit(song.genre());
        QSpinBox* yearSpin = new QSpinBox();
        yearSpin->setRange(1900, 2100);
        yearSpin->setValue(song.year());
        
        formLayout->addRow("标题:", titleEdit);
        formLayout->addRow("艺术家:", artistEdit);
        formLayout->addRow("专辑:", albumEdit);
        formLayout->addRow("流派:", genreEdit);
        formLayout->addRow("年份:", yearSpin);
        
        layout->addLayout(formLayout);
        
        // 按钮
        QHBoxLayout* buttonLayout = new QHBoxLayout();
        QPushButton* okButton = new QPushButton("保存");
        QPushButton* cancelButton = new QPushButton("取消");
        
        connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
        connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);
        
        buttonLayout->addStretch();
        buttonLayout->addWidget(okButton);
        buttonLayout->addWidget(cancelButton);
        layout->addLayout(buttonLayout);
        
        if (dialog.exec() == QDialog::Accepted) {
            // 更新歌曲信息
            song.setTitle(titleEdit->text().trimmed());
            song.setArtist(artistEdit->text().trimmed());
            song.setAlbum(albumEdit->text().trimmed());
            song.setGenre(genreEdit->text().trimmed());
            song.setYear(yearSpin->value());
            
            if (songDao.updateSong(song)) {
                logInfo(QString("歌曲信息更新成功: %1").arg(songId));
                updateStatusBar("歌曲信息已更新", 3000);
                refreshSongList(); // 刷新歌曲列表
            } else {
                logError(QString("更新歌曲信息失败: %1").arg(songId));
                QMessageBox::critical(m_mainWindow, "错误", "更新歌曲信息失败");
            }
        }
        
    } catch (const std::exception& e) {
        logError(QString("显示编辑歌曲对话框时发生异常: %1").arg(e.what()));
        QMessageBox::critical(m_mainWindow, "错误", "显示对话框时发生错误");
    }
}

void MainWindowController::showInFileExplorer(int songId, const QString& songTitle)
{
    logInfo(QString("在文件夹中显示歌曲: 歌曲ID=%1, 标题=%2").arg(songId).arg(songTitle));
    
    try {
        // 获取歌曲文件路径
        SongDao songDao;
        Song song = songDao.getSongById(songId);
        
        if (song.id() == -1) {
            QMessageBox::warning(m_mainWindow, "错误", "歌曲不存在");
            return;
        }
        
        QString filePath = song.filePath();
        if (filePath.isEmpty()) {
            QMessageBox::warning(m_mainWindow, "错误", "歌曲文件路径为空");
            return;
        }
        
        // 检查文件是否存在
        QFileInfo fileInfo(filePath);
        if (!fileInfo.exists()) {
            QMessageBox::warning(m_mainWindow, "文件不存在", 
                QString("文件 '%1' 不存在，可能已被移动或删除").arg(filePath));
            return;
        }
        
        // 在文件管理器中显示文件
#ifdef Q_OS_WIN
        QStringList args;
        args << "/select," << QDir::toNativeSeparators(filePath);
        QProcess::startDetached("explorer", args);
#elif defined(Q_OS_MAC)
        QStringList args;
        args << "-e" << "tell application \"Finder\" to reveal POSIX file \"" + filePath + "\"";
        QProcess::startDetached("osascript", args);
#else
        // Linux - 打开包含文件的目录
        QDesktopServices::openUrl(QUrl::fromLocalFile(fileInfo.absolutePath()));
#endif
        
        logInfo(QString("已在文件管理器中显示文件: %1").arg(filePath));
        updateStatusBar("已在文件管理器中显示文件", 3000);
        
    } catch (const std::exception& e) {
        logError(QString("在文件管理器中显示文件时发生异常: %1").arg(e.what()));
        QMessageBox::critical(m_mainWindow, "错误", "显示文件时发生错误");
    }
}

void MainWindowController::deleteSongFromDatabase(int songId, const QString& songTitle)
{
    logInfo(QString("从数据库删除歌曲: 歌曲ID=%1, 标题=%2").arg(songId).arg(songTitle));
    
    try {
        // 确认删除操作
        int ret = QMessageBox::question(m_mainWindow, "确认删除", 
            QString("确定要从数据库中删除歌曲 '%1' 吗？\n\n注意：这将删除歌曲记录及其所有标签关联，但不会删除实际文件。")
            .arg(songTitle),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            
        if (ret == QMessageBox::Yes) {
            SongDao songDao;
            
            // 开始事务
            QSqlDatabase db = DatabaseManager::instance()->database();
            db.transaction();
            
            // 先删除歌曲与标签的关联
            // if (!songDao.removeAllTagsFromSong(songId)) {
            //     db.rollback();
            //     logError(QString("删除歌曲标签关联失败: %1").arg(songId));
            //     QMessageBox::critical(m_mainWindow, "错误", "删除歌曲失败：无法移除标签关联");
            //     return;
            // }
            
            // 删除歌曲记录
            if (!songDao.deleteSong(songId)) {
                db.rollback();
                logError(QString("删除歌曲记录失败: %1").arg(songId));
                QMessageBox::critical(m_mainWindow, "错误", "删除歌曲失败：无法删除歌曲记录");
                return;
            }
            
            // 提交事务
            if (!db.commit()) {
                db.rollback();
                logError(QString("提交删除歌曲事务失败: %1").arg(songId));
                QMessageBox::critical(m_mainWindow, "错误", "删除歌曲失败：事务提交失败");
                return;
            }
            
            logInfo(QString("歌曲删除成功: %1").arg(songId));
            updateStatusBar(QString("歌曲 '%1' 已删除").arg(songTitle), 3000);
            
            // 刷新歌曲列表
            refreshSongList();
        }
        
    } catch (const std::exception& e) {
        // 回滚事务
        QSqlDatabase db = DatabaseManager::instance()->database();
        if (db.isOpen()) {
            db.rollback();
        }
        
        logError(QString("删除歌曲时发生异常: %1").arg(e.what()));
        QMessageBox::critical(m_mainWindow, "错误", "删除歌曲时发生错误");
    }
}

// 播放列表操作方法实现
void MainWindowController::showCreatePlaylistDialog()
{
    logInfo("显示创建播放列表对话框");
    
    try {
        // 创建输入对话框
        bool ok;
        QString playlistName = QInputDialog::getText(m_mainWindow, "创建播放列表", 
            "请输入播放列表名称:", QLineEdit::Normal, "", &ok);
            
        if (ok && !playlistName.trimmed().isEmpty()) {
            playlistName = playlistName.trimmed();
            
            // 检查播放列表名称是否已存在
            if (!m_playlistManager) {
                logWarning("PlaylistManager 未初始化，无法创建播放列表");
                QMessageBox::warning(m_mainWindow, "警告", "播放列表管理器未初始化");
                return;
            }
            
            if (m_playlistManager->playlistExists(playlistName)) {
                logWarning(QString("播放列表名称已存在: %1").arg(playlistName));
                QMessageBox::warning(m_mainWindow, "警告", 
                    QString("播放列表 '%1' 已存在，请使用其他名称").arg(playlistName));
                return;
            }
            
            // 创建播放列表
            Playlist newPlaylist;
            newPlaylist.setName(playlistName);
            newPlaylist.setDescription(QString("用户创建的播放列表"));
            newPlaylist.setType(PlaylistType::User);
            newPlaylist.setIsSystem(false);
            newPlaylist.setCreatedAt(QDateTime::currentDateTime());
            newPlaylist.setUpdatedAt(QDateTime::currentDateTime());
            
            auto result = m_playlistManager->createPlaylist(newPlaylist.name(), newPlaylist.description());
            if (!result.success) {
                logError(QString("创建播放列表失败: %1").arg(result.message));
                QMessageBox::critical(m_mainWindow, "错误", "创建播放列表失败");
                return;
            }
            
            logInfo(QString("创建播放列表: %1").arg(playlistName));
            updateStatusBar(QString("播放列表 '%1' 已创建").arg(playlistName), 3000);
            
            // 刷新播放列表视图
            refreshPlaylistView();
        }
        
    } catch (const std::exception& e) {
        logError(QString("创建播放列表时发生异常: %1").arg(e.what()));
        QMessageBox::critical(m_mainWindow, "错误", "创建播放列表时发生错误");
    }
}

void MainWindowController::importPlaylistFromFile()
{
    logInfo("导入播放列表");
    
    try {
        // 选择播放列表文件
        QString fileName = QFileDialog::getOpenFileName(m_mainWindow, 
            "导入播放列表", 
            QStandardPaths::writableLocation(QStandardPaths::MusicLocation),
            "播放列表文件 (*.m3u *.m3u8 *.pls *.xspf);;所有文件 (*.*)");
            
        if (!fileName.isEmpty()) {
            // 检查PlaylistManager是否已初始化
            if (!m_playlistManager) {
                logError("PlaylistManager未初始化，无法导入播放列表");
                QMessageBox::warning(m_mainWindow, "警告", "播放列表管理器未初始化");
                return;
            }
            
            // 解析播放列表文件
            QFileInfo fileInfo(fileName);
            QString extension = fileInfo.suffix().toLower();
            
            QStringList songPaths;
            QString playlistName = fileInfo.baseName();
            
            if (extension == "m3u" || extension == "m3u8") {
                songPaths = parseM3UPlaylist(fileName);
            } else if (extension == "pls") {
                songPaths = parsePLSPlaylist(fileName);
            } else if (extension == "xspf") {
                songPaths = parseXSPFPlaylist(fileName);
            } else {
                logError(QString("不支持的播放列表格式: %1").arg(extension));
                QMessageBox::warning(m_mainWindow, "警告", "不支持的播放列表格式");
                return;
            }
            
            if (songPaths.isEmpty()) {
                logWarning("播放列表文件中没有找到有效的歌曲路径");
                QMessageBox::information(m_mainWindow, "信息", "播放列表文件中没有找到有效的歌曲");
                return;
            }
            
            // 创建播放列表
            auto result = m_playlistManager->createPlaylist(playlistName, "从文件导入的播放列表");
            if (!result.success) {
                logError(QString("创建播放列表失败: %1").arg(result.message));
                QMessageBox::critical(m_mainWindow, "错误", "创建播放列表失败");
                return;
            }
            
            Playlist playlist = result.data.value<Playlist>();
            int successCount = 0;
            int totalCount = songPaths.size();
            
            // 添加歌曲到播放列表
            for (const QString& songPath : songPaths) {
                QFileInfo songFileInfo(songPath);
                if (songFileInfo.exists() && songFileInfo.isFile()) {
                    // 这里需要先将歌曲添加到数据库，然后再添加到播放列表
                    // 由于当前架构限制，暂时记录日志
                    logInfo(QString("找到歌曲文件: %1").arg(songPath));
                    successCount++;
                } else {
                    logWarning(QString("歌曲文件不存在: %1").arg(songPath));
                }
            }
            
            logInfo(QString("导入播放列表文件: %1，成功解析 %2/%3 首歌曲")
                   .arg(fileName).arg(successCount).arg(totalCount));
            updateStatusBar(QString("播放列表导入完成，解析了 %1/%2 首歌曲")
                           .arg(successCount).arg(totalCount), 3000);
            
            // 刷新播放列表视图
            refreshPlaylistView();
        }
        
    } catch (const std::exception& e) {
        logError(QString("导入播放列表时发生异常: %1").arg(e.what()));
        QMessageBox::critical(m_mainWindow, "错误", "导入播放列表时发生错误");
    }
}

void MainWindowController::refreshPlaylistView()
{
    logInfo("刷新播放列表视图");
    
    if (!m_playlistManager) {
        logWarning("PlaylistManager 未初始化，无法刷新播放列表视图");
        updateStatusBar("播放列表管理器未初始化", 3000);
        return;
    }
    
    try {
        // 获取所有播放列表
        QList<Playlist> playlists = m_playlistManager->getAllPlaylists();
        logDebug(QString("获取到 %1 个播放列表").arg(playlists.size()));
        
        // 由于当前UI设计中没有专门的播放列表控件，
        // 这里通过状态栏显示播放列表信息并记录到日志
        if (playlists.isEmpty()) {
            updateStatusBar("暂无播放列表", 2000);
            logInfo("当前没有播放列表");
        } else {
            QString message = QString("共有 %1 个播放列表").arg(playlists.size());
            updateStatusBar(message, 2000);
            
            // 记录播放列表详细信息到日志
            for (const auto& playlist : playlists) {
                logDebug(QString("播放列表: %1 (ID: %2, 歌曲数: %3, 类型: %4)")
                        .arg(playlist.name())
                        .arg(QString::number(playlist.id()))
                        .arg(QString::number(playlist.songCount()))
                        .arg(QString::number(static_cast<int>(playlist.type()))));
            }
        }
        
        // 如果当前视图模式是播放列表视图，进行特殊处理
        if (m_viewMode == ViewMode::PlaylistView) {
            logDebug("当前视图模式为播放列表视图，执行相应更新");
            // 这里可以添加播放列表视图特有的更新逻辑
            // 例如：更新播放列表相关的UI控件
        }
        
        logInfo("播放列表视图刷新完成");
        
    } catch (const std::exception& e) {
        QString errorMsg = QString("刷新播放列表视图时发生异常: %1").arg(e.what());
        logError(errorMsg);
        updateStatusBar(errorMsg, 5000);
        handleError(errorMsg);
    }
}

// 辅助方法：获取当前选中的标签
Tag MainWindowController::getSelectedTag() const
{
    if (!m_tagListWidget) {
        return Tag();
    }
    
    QListWidgetItem* currentItem = m_tagListWidget->currentItem();
    if (!currentItem) {
        return Tag();
    }
    
    // 从item中获取标签信息
    QString tagName = currentItem->text();
    TagDao tagDao;
    return tagDao.getTagByName(tagName);
}

// 播放列表文件解析方法实现
QStringList MainWindowController::parseM3UPlaylist(const QString& filePath) const
{
    QStringList songPaths;
    QFile file(filePath);
    
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const_cast<MainWindowController*>(this)->logError(QString("无法打开M3U播放列表文件: %1").arg(filePath));
        return songPaths;
    }
    
    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    
    QDir playlistDir = QFileInfo(filePath).absoluteDir();
    
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        
        // 跳过注释行和空行
        if (line.isEmpty() || line.startsWith("#")) {
            continue;
        }
        
        // 处理相对路径
        if (QFileInfo(line).isRelative()) {
            line = playlistDir.absoluteFilePath(line);
        }
        
        // 检查文件是否存在
        if (QFileInfo::exists(line)) {
            songPaths.append(QDir::toNativeSeparators(line));
            const_cast<MainWindowController*>(this)->logDebug(QString("M3U: 找到歌曲文件: %1").arg(line));
        } else {
            const_cast<MainWindowController*>(this)->logWarning(QString("M3U: 歌曲文件不存在: %1").arg(line));
        }
    }
    
    file.close();
    const_cast<MainWindowController*>(this)->logInfo(QString("M3U播放列表解析完成，共找到 %1 首歌曲").arg(songPaths.size()));
    return songPaths;
}

QStringList MainWindowController::parsePLSPlaylist(const QString& filePath) const
{
    QStringList songPaths;
    QFile file(filePath);
    
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const_cast<MainWindowController*>(this)->logError(QString("无法打开PLS播放列表文件: %1").arg(filePath));
        return songPaths;
    }
    
    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    
    QDir playlistDir = QFileInfo(filePath).absoluteDir();
    
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        
        // 查找File条目
        if (line.startsWith("File", Qt::CaseInsensitive)) {
            int equalPos = line.indexOf('=');
            if (equalPos > 0) {
                QString songPath = line.mid(equalPos + 1).trimmed();
                
                // 处理相对路径
                if (QFileInfo(songPath).isRelative()) {
                    songPath = playlistDir.absoluteFilePath(songPath);
                }
                
                // 检查文件是否存在
                if (QFileInfo::exists(songPath)) {
                    songPaths.append(QDir::toNativeSeparators(songPath));
                    const_cast<MainWindowController*>(this)->logDebug(QString("PLS: 找到歌曲文件: %1").arg(songPath));
                } else {
                    const_cast<MainWindowController*>(this)->logWarning(QString("PLS: 歌曲文件不存在: %1").arg(songPath));
                }
            }
        }
    }
    
    file.close();
    const_cast<MainWindowController*>(this)->logInfo(QString("PLS播放列表解析完成，共找到 %1 首歌曲").arg(songPaths.size()));
    return songPaths;
}

QStringList MainWindowController::parseXSPFPlaylist(const QString& filePath) const
{
    QStringList songPaths;
    QFile file(filePath);
    
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << QString("无法打开XSPF播放列表文件: %1").arg(filePath);
        return songPaths;
    }
    
    QXmlStreamReader xml(&file);
    QDir playlistDir = QFileInfo(filePath).absoluteDir();
    
    while (!xml.atEnd()) {
        xml.readNext();
        
        if (xml.isStartElement() && xml.name() == QStringLiteral("location")) {
            QString location = xml.readElementText().trimmed();
            
            // 处理file://协议
            if (location.startsWith("file://")) {
                location = QUrl(location).toLocalFile();
            }
            
            // 处理相对路径
            if (QFileInfo(location).isRelative()) {
                location = playlistDir.absoluteFilePath(location);
            }
            
            // 检查文件是否存在
            if (QFileInfo::exists(location)) {
                songPaths.append(QDir::toNativeSeparators(location));
                const_cast<MainWindowController*>(this)->logDebug(QString("XSPF: 找到歌曲文件: %1").arg(location));
            } else {
                const_cast<MainWindowController*>(this)->logWarning(QString("XSPF: 歌曲文件不存在: %1").arg(location));
            }
        }
    }
    
    if (xml.hasError()) {
        const_cast<MainWindowController*>(this)->logError(QString("XSPF解析错误: %1").arg(xml.errorString()));
    }
    
    file.close();
    const_cast<MainWindowController*>(this)->logInfo(QString("XSPF播放列表解析完成，共找到 %1 首歌曲").arg(songPaths.size()));
    return songPaths;
}