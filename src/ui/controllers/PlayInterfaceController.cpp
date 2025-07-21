#include "PlayInterfaceController.h"
#include "../../audio/audioengine.h"

// 定义静态常量
const int PlayInterfaceController::UPDATE_INTERVAL = 100;  // ms

PlayInterfaceController::PlayInterfaceController(PlayInterface* interface, QObject* parent)
    : QObject(parent)
    // 界面相关
    , m_interface(interface)
    , m_isProgressBarDragging(false)
    // 核心组件
    , m_audioEngine(nullptr)
    , m_tagManager(nullptr)
    , m_playlistManager(nullptr)
    , m_databaseManager(nullptr)
    , m_logger(nullptr)
    , m_updateTimer(new QTimer(this))
    // 初始化标志
    , m_initialized(false)
    // 当前状态
    , m_currentSong()
    , m_isPlaying(false)
    , m_isPaused(false)
    , m_isMuted(false)
    , m_currentTime(0)
    , m_totalTime(0)
    , m_volume(50)
    , m_balance(0)
    // 显示相关
    , m_displayMode(DisplayMode::Cover)
    , m_visualizationType(VisualizationType::None)
    // 均衡器
    , m_equalizerPreset("Default")
{
    // 设置定时器
    m_updateTimer->setInterval(UPDATE_INTERVAL);
    
    // 初始化均衡器值
    m_equalizerValues.resize(5, 0);  // 5个频段，初始值都为0
    
    // 初始化其他成员
    initialize();
}

PlayInterfaceController::~PlayInterfaceController()
{
    shutdown();
}

// AudioEngine设置方法
void PlayInterfaceController::setAudioEngine(AudioEngine* audioEngine)
{
    // 断开之前的连接
    if (m_audioEngine) {
        disconnect(m_audioEngine, nullptr, this, nullptr);
    }
    
    m_audioEngine = audioEngine;
    
    // 如果已经初始化，重新设置连接
    if (m_initialized && m_audioEngine) {
        // 连接AudioEngine状态变化信号
        connect(m_audioEngine, &AudioEngine::stateChanged, 
                this, &PlayInterfaceController::onPlaybackStateChanged);
        connect(m_audioEngine, &AudioEngine::positionChanged, 
                this, &PlayInterfaceController::onPositionChanged);
        connect(m_audioEngine, &AudioEngine::durationChanged, 
                this, &PlayInterfaceController::onDurationChanged);
        connect(m_audioEngine, &AudioEngine::volumeChanged, 
                this, &PlayInterfaceController::onVolumeChanged);
        connect(m_audioEngine, &AudioEngine::mutedChanged, 
                this, &PlayInterfaceController::onMutedChanged);
        connect(m_audioEngine, &AudioEngine::currentSongChanged, 
                this, &PlayInterfaceController::onCurrentSongChanged);
        
        
        // 同步当前状态
        if (m_interface) {
            m_interface->setPlaybackState(m_audioEngine->state() == AudioTypes::AudioState::Playing);
            m_interface->setCurrentTime(m_audioEngine->position());
            m_interface->setTotalTime(m_audioEngine->duration());
            m_interface->setVolumeSliderValue(m_audioEngine->volume());
            m_interface->setMuted(m_audioEngine->isMuted());
        }
    }
}

AudioEngine* PlayInterfaceController::getAudioEngine() const
{
    return m_audioEngine;
}

void PlayInterfaceController::initialize()
{
    if (m_initialized) {
        return;
    }
    
    try {
        // 设置连接
        setupConnections();
        
        // 启动定时器
        if (m_updateTimer) {
            m_updateTimer->start(UPDATE_INTERVAL);
        }
        
        m_initialized = true;
        
    } catch (const std::exception& e) {
        logError(QString("PlayInterfaceController initialization failed: %1").arg(e.what()));
    } catch (...) {
        logError("PlayInterfaceController initialization failed with unknown error");
    }
}

void PlayInterfaceController::shutdown()
{
    // 停止定时器
    if (m_updateTimer) {
        m_updateTimer->stop();
    }
    
    // 断开所有连接
    if (m_interface) {
        disconnect(m_interface, nullptr, this, nullptr);
    }
    if (m_audioEngine) {
        disconnect(m_audioEngine, nullptr, this, nullptr);
    }
    
    // 清理资源
    m_initialized = false;
}

bool PlayInterfaceController::isInitialized() const
{
    return m_initialized;
}

Song PlayInterfaceController::getCurrentSong() const
{
    return m_currentSong;
}

void PlayInterfaceController::setCurrentSong(const Song& song)
{
    m_currentSong = song;
    
    // 更新界面信息
    if (m_interface) {
        m_interface->setSongTitle(song.title());
        m_interface->setSongArtist(song.artist());
        m_interface->setSongAlbum(song.album());
    }
    
    // 加载歌曲信息
    loadSongInfo(song);
}

// 与主界面同步方法实现
void PlayInterfaceController::syncWithMainWindow(qint64 position, qint64 duration, int volume, bool muted)
{
    try {
        // 同步播放进度
        syncProgressBar(position, duration);
        
        // 同步音量控制
        syncVolumeControls(volume, muted);
        
        logDebug(QString("Synced with main window: pos=%1/%2, vol=%3, muted=%4")
                .arg(position).arg(duration).arg(volume).arg(muted));
                
    } catch (const std::exception& e) {
        handleError(QString("Error syncing with main window: %1").arg(e.what()));
    } catch (...) {
        handleError("Unknown error occurred while syncing with main window");
    }
}

void PlayInterfaceController::syncProgressBar(qint64 position, qint64 duration)
{
    try {
        if (!m_interface) {
            return;
        }
        
        // 更新内部状态
        m_currentTime = position;
        m_totalTime = duration;
        
        // 只有在进度条未被拖动时才更新界面
        if (!m_isProgressBarDragging) {
            m_interface->setProgressBarPosition(position);
            m_interface->setProgressBarDuration(duration);
            m_interface->updateProgressDisplay();
        }
        
    } catch (const std::exception& e) {
        handleError(QString("Error syncing progress bar: %1").arg(e.what()));
    } catch (...) {
        handleError("Unknown error occurred while syncing progress bar");
    }
}

void PlayInterfaceController::syncVolumeControls(int volume, bool muted)
{
    try {
        if (!m_interface) {
            return;
        }
        
        // 更新内部状态
        m_volume = volume;
        m_isMuted = muted;
        
        // 更新界面音量控制
        m_interface->updateVolumeControls();
        m_interface->setVolumeSliderValue(volume);
        m_interface->updateVolumeLabel(volume);
        m_interface->updateMuteButtonIcon();
        
    } catch (const std::exception& e) {
        handleError(QString("Error syncing volume controls: %1").arg(e.what()));
    } catch (...) {
        handleError("Unknown error occurred while syncing volume controls");
    }
}

void PlayInterfaceController::setDisplayMode(DisplayMode mode)
{
    if (m_displayMode != mode) {
        m_displayMode = mode;
        emit displayModeChanged(mode);
    }
}

DisplayMode PlayInterfaceController::getDisplayMode() const
{
    return m_displayMode;
}

void PlayInterfaceController::setVisualizationType(VisualizationType type)
{
    if (m_visualizationType != type) {
        m_visualizationType = type;
        emit visualizationTypeChanged(type);
    }
}

VisualizationType PlayInterfaceController::getVisualizationType() const
{
    return m_visualizationType;
}

void PlayInterfaceController::loadSongInfo(const Song& song)
{
    // 加载歌曲信息
    logInfo(QString("Loading song info for: %1").arg(song.title()));
}

// 槽函数实现
void PlayInterfaceController::onUpdateTimer()
{
    try {
        // 检查初始化状态
        if (!m_initialized || !m_interface) {
            return;
        }
        
        // 更新播放时间显示
        if (m_isPlaying) {
            updateTimeDisplay();
            updatePlaybackControls();
            updateVolumeDisplay();
            updateBalanceDisplay();
        }
        
    } catch (const std::exception& e) {
        handleError(QString("Error in update timer: %1").arg(e.what()));
    } catch (...) {
        handleError("Unknown error occurred in update timer");
    }
}



void PlayInterfaceController::onProgressSliderPressed()
{
    // 进度条按下时暂停更新，避免跳动
    m_isProgressBarDragging = true;
}

void PlayInterfaceController::onProgressSliderReleased()
{
    // 进度条释放时恢复更新
    m_isProgressBarDragging = false;
}

// 私有方法实现
void PlayInterfaceController::setupConnections()
{
    try {
        // 连接界面信号
        if (m_interface) {
            // 播放控制信号
            connect(m_interface, &PlayInterface::playPauseClicked, 
                    this, &PlayInterfaceController::onPlayPauseClicked);
            connect(m_interface, &PlayInterface::playModeClicked, 
                    this, &PlayInterfaceController::onPlayModeClicked);
            connect(m_interface, &PlayInterface::nextClicked, 
                    this, &PlayInterfaceController::onNextClicked);
            connect(m_interface, &PlayInterface::previousClicked, 
                    this, &PlayInterfaceController::onPreviousClicked);
            
            // 音频控制信号
            connect(m_interface, &PlayInterface::volumeChanged, 
                    this, &PlayInterfaceController::onVolumeSliderChanged);
            connect(m_interface, &PlayInterface::balanceChanged, 
                    this, &PlayInterfaceController::onBalanceSliderChanged);
            connect(m_interface, &PlayInterface::positionChanged, 
                    this, &PlayInterfaceController::onPositionSliderChanged);
            
            // 新增的进度条和音量控制信号
            connect(m_interface, &PlayInterface::seekRequested, 
                    this, &PlayInterfaceController::seekRequested);
            connect(m_interface, &PlayInterface::volumeSliderChanged, 
                    this, &PlayInterfaceController::volumeChanged);
            connect(m_interface, &PlayInterface::seekRequested,
                    this, &PlayInterfaceController::seekRequested);
            connect(m_interface, &PlayInterface::progressSliderPressed,
                    [this]() { onProgressSliderPressed(); });
            connect(m_interface, &PlayInterface::progressSliderReleased,
                    [this]() { onProgressSliderReleased(); });
            
            // 均衡器信号
            connect(m_interface, &PlayInterface::equalizerChanged, 
                    this, &PlayInterfaceController::onEqualizerSliderChanged);
            
            logDebug("Connected interface signals");
        } else {
            logError("Interface is null, cannot connect signals");
            return;
        }
        
        // 连接定时器信号（避免重复连接）
        if (m_updateTimer) {
            // 断开之前的连接（如果有的话）
            disconnect(m_updateTimer, &QTimer::timeout, this, &PlayInterfaceController::onUpdateTimer);
            connect(m_updateTimer, &QTimer::timeout, 
                    this, &PlayInterfaceController::onUpdateTimer);
            logDebug("Connected update timer");
        } else {
            logError("Update timer is null");
        }
        
        // 连接AudioEngine信号
        if (m_audioEngine) {
            // 连接AudioEngine状态变化信号
            connect(m_audioEngine, &AudioEngine::stateChanged, 
                    this, &PlayInterfaceController::onPlaybackStateChanged);
            connect(m_audioEngine, &AudioEngine::positionChanged, 
                    this, &PlayInterfaceController::onPositionChanged);
            connect(m_audioEngine, &AudioEngine::durationChanged, 
                    this, &PlayInterfaceController::onDurationChanged);
            connect(m_audioEngine, &AudioEngine::volumeChanged, 
                    this, &PlayInterfaceController::onVolumeChanged);
            connect(m_audioEngine, &AudioEngine::mutedChanged, 
                    this, &PlayInterfaceController::onMutedChanged);
            connect(m_audioEngine, &AudioEngine::currentSongChanged, 
                    this, &PlayInterfaceController::onCurrentSongChanged);
            
            logDebug("AudioEngine信号连接完成");
        }
        
    } catch (const std::exception& e) {
        handleError(QString("Error setting up connections: %1").arg(e.what()));
    } catch (...) {
        handleError("Unknown error occurred while setting up connections");
    }
}



void PlayInterfaceController::handleError(const QString& error)
{
    logError(error);
    emit errorOccurred(error);
}

void PlayInterfaceController::logInfo(const QString& message)
{
    qInfo() << "PlayInterfaceController:" << message;
}

void PlayInterfaceController::logError(const QString& error)
{
    qCritical() << "PlayInterfaceController Error:" << error;
}

void PlayInterfaceController::logDebug(const QString& message)
{
    qDebug() << "PlayInterfaceController:" << message;
}

// 修正后的槽函数实现
void PlayInterfaceController::onPlaybackStateChanged(AudioTypes::AudioState state)
{
    // 更新播放控件状态
    if (m_interface) {
        m_interface->setPlaybackState(state == AudioTypes::AudioState::Playing);
    }
}

void PlayInterfaceController::onCurrentSongChanged(const Song& song)
{
    setCurrentSong(song);
}

void PlayInterfaceController::onPositionChanged(qint64 position)
{
    m_currentTime = position;
    if (m_interface && !m_isProgressBarDragging) {
        m_interface->setCurrentTime(position);
    }
}

void PlayInterfaceController::onDurationChanged(qint64 duration)
{
    m_totalTime = duration;
    if (m_interface) {
        m_interface->setTotalTime(duration);
    }
}

void PlayInterfaceController::onVolumeChanged(int volume)
{
    m_volume = volume;
    if (m_interface) {
        m_interface->setVolumeSliderValue(volume);
    }
}

void PlayInterfaceController::onMutedChanged(bool muted)
{
    if (m_interface) {
        m_interface->setMuted(muted);
    }
}



void PlayInterfaceController::onPlayPauseClicked()
{
    if (m_audioEngine) {
        if (m_audioEngine->state() == AudioTypes::AudioState::Playing) {
            m_audioEngine->pause();
        } else {
            Song currentSong = getCurrentSong();
            if (currentSong.isValid()) {
                emit playRequested(currentSong);
            } else {
                m_audioEngine->play();
            }
        }
    } else {
        // 如果没有AudioEngine，发送信号让主控制器处理
        emit playRequested(getCurrentSong());
    }
}

void PlayInterfaceController::onPlayModeClicked()
{
    // 发出信号让主控制器处理播放模式切换
    emit playModeChangeRequested();
}

void PlayInterfaceController::onNextClicked()
{
    if (m_audioEngine) {
        m_audioEngine->playNext();
    } else {
        emit nextRequested();
    }
}

void PlayInterfaceController::onPreviousClicked()
{
    if (m_audioEngine) {
        m_audioEngine->playPrevious();
    } else {
        emit previousRequested();
    }
}

void PlayInterfaceController::onVolumeSliderChanged(int value)
{
    m_volume = value;
    if (m_audioEngine) {
        m_audioEngine->setVolume(value);
    } else {
        emit volumeChanged(value);
    }
}

void PlayInterfaceController::onBalanceSliderChanged(int value)
{
    m_balance = value;
    emit balanceChanged(value);
}

void PlayInterfaceController::onPositionSliderChanged(int value)
{
    qint64 position = static_cast<qint64>(value);
    if (m_audioEngine) {
        m_audioEngine->seek(position);
    } else {
        emit seekRequested(position);
    }
}

void PlayInterfaceController::onMuteButtonClicked()
{
    if (m_audioEngine) {
        m_audioEngine->toggleMute();
    } else {
        emit muteToggled(!m_isMuted);
    }
}

void PlayInterfaceController::onDisplayModeClicked()
{
    // 切换显示模式
    DisplayMode newMode = (m_displayMode == DisplayMode::Lyrics) ? DisplayMode::Cover : DisplayMode::Lyrics;
    setDisplayMode(newMode);
}

void PlayInterfaceController::onVisualizationTypeClicked()
{
    int currentType = static_cast<int>(m_visualizationType);
    int nextType = (currentType + 1) % 6;
    setVisualizationType(static_cast<VisualizationType>(nextType));
}

void PlayInterfaceController::onEqualizerSliderChanged(const QVector<int>& values)
{
    // 处理所有频段的值
    for (int i = 0; i < values.size() && i < 10; ++i) {
        emit equalizerChanged(static_cast<EqualizerBand>(i), values[i]);
    }
}



// ==================== 可视化绘制方法实现 ====================



// ==================== 辅助方法实现 ====================

QString PlayInterfaceController::formatTime(qint64 milliseconds) const
{
    qint64 seconds = milliseconds / 1000;
    qint64 minutes = seconds / 60;
    qint64 hours = minutes / 60;
    
    seconds %= 60;
    minutes %= 60;
    
    if (hours > 0) {
        return QString("%1:%2:%3")
                .arg(hours)
                .arg(minutes, 2, 10, QChar('0'))
                .arg(seconds, 2, 10, QChar('0'));
    } else {
        return QString("%1:%2")
                .arg(minutes)
                .arg(seconds, 2, 10, QChar('0'));
    }
}

void PlayInterfaceController::updateTimeDisplay()
{
    try {
        if (!m_interface) {
            return;
        }
        
        // 更新时间显示
        QString currentTimeStr = formatTime(m_currentTime);
        QString totalTimeStr = formatTime(m_totalTime);
        QString timeText = QString("%1 / %2").arg(currentTimeStr, totalTimeStr);
        
        // 更新进度条
        if (m_totalTime > 0) {
            int progress = static_cast<int>((m_currentTime * 100) / m_totalTime);
            // 这里应该调用界面更新方法，暂时用日志记录
            logDebug(QString("Time display updated: %1, progress: %2%").arg(timeText).arg(progress));
        }
        
    } catch (const std::exception& e) {
        handleError(QString("Error updating time display: %1").arg(e.what()));
    } catch (...) {
        handleError("Unknown error occurred while updating time display");
    }
}

void PlayInterfaceController::updatePlaybackControls()
{
    try {
        if (!m_interface) {
            return;
        }
        
        // 更新播放控制按钮状态
        logDebug(QString("Playback controls updated: playing=%1, paused=%2")
                .arg(m_isPlaying)
                .arg(m_isPaused));
        
    } catch (const std::exception& e) {
        handleError(QString("Error updating playback controls: %1").arg(e.what()));
    } catch (...) {
        handleError("Unknown error occurred while updating playback controls");
    }
}

void PlayInterfaceController::updateVolumeDisplay()
{
    try {
        if (!m_interface) {
            return;
        }
        
        // 更新音量显示
        logDebug(QString("Volume display updated: %1, muted=%2")
                .arg(m_volume)
                .arg(m_isMuted));
        
    } catch (const std::exception& e) {
        handleError(QString("Error updating volume display: %1").arg(e.what()));
    } catch (...) {
        handleError("Unknown error occurred while updating volume display");
    }
}

void PlayInterfaceController::updateBalanceDisplay()
{
    try {
        if (!m_interface) {
            return;
        }
        
        // 更新平衡显示
        logDebug(QString("Balance display updated: %1").arg(m_balance));
        
    } catch (const std::exception& e) {
        handleError(QString("Error updating balance display: %1").arg(e.what()));
    } catch (...) {
        handleError("Unknown error occurred while updating balance display");
    }
}

void PlayInterfaceController::updatePlayModeButton(int playMode)
{
    if (!m_interface) {
        return;
    }
    
    QString text, iconPath, tooltip;
    
    // 根据播放模式设置按钮属性
    switch (playMode) {
        case 0: // 列表循环
            text = "列表循环";
            iconPath = ":/new/prefix1/images/listCycle.png";
            tooltip = "当前模式：列表循环";
            break;
        case 1: // 随机播放
            text = "随机播放";
            iconPath = ":/new/prefix1/images/shufflePlay.png";
            tooltip = "当前模式：随机播放";
            break;
        case 2: // 单曲循环
            text = "单曲循环";
            iconPath = ":/new/prefix1/images/singleCycle.png";
            tooltip = "当前模式：单曲循环";
            break;
        default:
            text = "列表循环";
            iconPath = ":/new/prefix1/images/listCycle.png";
            tooltip = "当前模式：列表循环";
            break;
    }
    
    m_interface->updatePlayModeButton("", iconPath, tooltip); // 不显示文字，只显示图标
}