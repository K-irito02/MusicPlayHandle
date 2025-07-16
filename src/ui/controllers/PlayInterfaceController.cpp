#include "playinterfacecontroller.h"
#include "../dialogs/playinterface.h"
#include <QDebug>
#include <QDateTime>
#include <QMutexLocker>
#include <QRandomGenerator>

// 静态常量定义
const int PlayInterfaceController::SPECTRUM_BANDS;

PlayInterfaceController::PlayInterfaceController(PlayInterface* interface, QObject* parent)
    : QObject(parent)
    , m_interface(interface)
    , m_audioEngine(nullptr)
    , m_tagManager(nullptr)
    , m_playlistManager(nullptr)
    , m_databaseManager(nullptr)
    , m_logger(nullptr)
    , m_displayMode(DisplayMode::Cover)
    , m_visualizationType(VisualizationType::Spectrum)
    , m_initialized(false)
    , m_isPlaying(false)
    , m_isPaused(false)
    , m_isMuted(false)
    , m_currentTime(0)
    , m_totalTime(0)
    , m_volume(50)
    , m_balance(0)
    , m_visualizationEnabled(true)
    , m_equalizerPreset("Default")
    , m_currentLyricIndex(0)
    , m_lyricDocument(nullptr)
    , m_waveformScene(nullptr)
    , m_spectrumScene(nullptr)
    , m_coverScene(nullptr)
    , m_lyricScene(nullptr)
    , m_coverItem(nullptr)
    , m_lyricItem(nullptr)
    , m_spinAnimation(nullptr)
    , m_pulseAnimation(nullptr)
    , m_fadeAnimation(nullptr)
    , m_animationGroup(nullptr)
    , m_updateTimer(nullptr)
    , m_visualizationTimer(nullptr)
    , m_lyricUpdateTimer(nullptr)
    , m_animationTimer(nullptr)
    , m_settings(nullptr)
{
    // 初始化定时器
    m_updateTimer = new QTimer(this);
    m_visualizationTimer = new QTimer(this);
    m_lyricUpdateTimer = new QTimer(this);
    m_animationTimer = new QTimer(this);
    
    // 初始化设置
    m_settings = new QSettings(this);
    
    // 初始化均衡器值
    m_equalizerValues.resize(EQUALIZER_BANDS);
    m_equalizerValues.fill(0);
    
    // 连接定时器
    connect(m_updateTimer, &QTimer::timeout, this, &PlayInterfaceController::onUpdateTimer);
    connect(m_visualizationTimer, &QTimer::timeout, this, &PlayInterfaceController::onVisualizationTimer);
    connect(m_lyricUpdateTimer, &QTimer::timeout, this, &PlayInterfaceController::onLyricUpdateTimer);
    connect(m_animationTimer, &QTimer::timeout, this, &PlayInterfaceController::onAnimationTimer);
}

PlayInterfaceController::~PlayInterfaceController()
{
    shutdown();
}

bool PlayInterfaceController::initialize()
{
    if (m_initialized) {
        return true;
    }
    
    try {
        // 设置连接
        setupConnections();
        
        // 设置可视化
        setupVisualization();
        
        // 设置动画
        setupAnimations();
        
        // 加载设置
        loadSettings();
        
        // 启动定时器
        m_updateTimer->start(UPDATE_INTERVAL);
        m_visualizationTimer->start(VISUALIZATION_UPDATE_INTERVAL);
        m_lyricUpdateTimer->start(LYRIC_UPDATE_INTERVAL);
        
        m_initialized = true;
        logInfo("PlayInterfaceController initialized successfully");
        
        return true;
    } catch (const std::exception& e) {
        logError(QString("PlayInterfaceController initialization failed: %1").arg(e.what()));
        return false;
    }
}

void PlayInterfaceController::shutdown()
{
    if (!m_initialized) {
        return;
    }
    
    // 保存设置
    saveSettings();
    
    // 停止定时器
    if (m_updateTimer) m_updateTimer->stop();
    if (m_visualizationTimer) m_visualizationTimer->stop();
    if (m_lyricUpdateTimer) m_lyricUpdateTimer->stop();
    if (m_animationTimer) m_animationTimer->stop();
    
    // 停止动画
    if (m_animationGroup) {
        m_animationGroup->stop();
    }
    
    m_initialized = false;
    logInfo("PlayInterfaceController shut down");
}

bool PlayInterfaceController::isInitialized() const
{
    return m_initialized;
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

Song PlayInterfaceController::getCurrentSong() const
{
    return m_currentSong;
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

void PlayInterfaceController::loadSongLyrics(const Song& song)
{
    // 加载歌词
    logInfo(QString("Loading lyrics for: %1").arg(song.title()));
}

void PlayInterfaceController::loadSongCover(const Song& song)
{
    // 加载封面
    logInfo(QString("Loading cover for: %1").arg(song.title()));
}

// 槽函数实现
void PlayInterfaceController::onUpdateTimer()
{
    try {
        // 更新播放时间显示
        if (m_interface && m_isPlaying) {
            updateTimeDisplay();
            updatePlaybackControls();
            updateVolumeDisplay();
            updateBalanceDisplay();
        }
        
        // 每秒记录一次调试信息
        static int updateCount = 0;
        if (++updateCount % 20 == 0) { // 50ms * 20 = 1秒
            logDebug(QString("Update timer: playing=%1, time=%2/%3")
                    .arg(m_isPlaying)
                    .arg(m_currentTime)
                    .arg(m_totalTime));
        }
        
    } catch (const std::exception& e) {
        handleError(QString("Error in update timer: %1").arg(e.what()));
    } catch (...) {
        handleError("Unknown error occurred in update timer");
    }
}

void PlayInterfaceController::onVisualizationTimer()
{
    try {
        if (!m_visualizationEnabled || !m_interface) {
            return;
        }
        
        // 更新可视化显示
        updateVisualizationDisplay();
        
        // 根据当前可视化类型绘制
        switch (m_visualizationType) {
            case VisualizationType::Waveform:
                if (!m_visualizationData.waveform.isEmpty()) {
                    drawWaveform(m_visualizationData.waveform);
                }
                break;
            case VisualizationType::Spectrum:
                if (!m_visualizationData.spectrum.isEmpty()) {
                    drawSpectrum(m_visualizationData.spectrum);
                }
                break;
            case VisualizationType::VUMeter:
                drawVUMeter(m_visualizationData.leftLevel, m_visualizationData.rightLevel);
                break;
            case VisualizationType::Oscilloscope:
                if (!m_visualizationData.waveform.isEmpty()) {
                    drawOscilloscope(m_visualizationData.waveform);
                }
                break;
            case VisualizationType::Bars:
                if (!m_visualizationData.spectrum.isEmpty()) {
                    drawBars(m_visualizationData.spectrum);
                }
                break;
            case VisualizationType::Particles:
                if (!m_visualizationData.spectrum.isEmpty()) {
                    drawParticles(m_visualizationData.spectrum);
                }
                break;
        }
        
        // 每秒记录一次调试信息
        static int vizCount = 0;
        if (++vizCount % 60 == 0) { // 16ms * 60 ≈ 1秒
            logDebug(QString("Visualization timer: type=%1, enabled=%2")
                    .arg(static_cast<int>(m_visualizationType))
                    .arg(m_visualizationEnabled));
        }
        
    } catch (const std::exception& e) {
        handleError(QString("Error in visualization timer: %1").arg(e.what()));
    } catch (...) {
        handleError("Unknown error occurred in visualization timer");
    }
}

void PlayInterfaceController::onLyricUpdateTimer()
{
    try {
        if (!m_interface || m_lyrics.isEmpty()) {
            return;
        }
        
        // 更新歌词高亮
        updateLyricHighlight(m_currentTime);
        
        // 更新歌词显示
        updateLyricDisplay();
        
        // 自动滚动到当前歌词
        if (m_currentLyricIndex >= 0 && m_currentLyricIndex < m_lyrics.size()) {
            scrollToCurrentLyric();
        }
        
        // 每秒记录一次调试信息
        static int lyricCount = 0;
        if (++lyricCount % 10 == 0) { // 100ms * 10 = 1秒
            logDebug(QString("Lyric timer: index=%1/%2, time=%3")
                    .arg(m_currentLyricIndex)
                    .arg(m_lyrics.size())
                    .arg(m_currentTime));
        }
        
    } catch (const std::exception& e) {
        handleError(QString("Error in lyric update timer: %1").arg(e.what()));
    } catch (...) {
        handleError("Unknown error occurred in lyric update timer");
    }
}

void PlayInterfaceController::onAnimationTimer()
{
    try {
        if (!m_interface) {
            return;
        }
        
        // 更新封面动画
        if (m_isPlaying && m_spinAnimation && m_spinAnimation->state() != QAbstractAnimation::Running) {
            startSpinAnimation();
        } else if (!m_isPlaying && m_spinAnimation && m_spinAnimation->state() == QAbstractAnimation::Running) {
            stopSpinAnimation();
        }
        
        // 更新脉冲动画
        if (m_isPlaying && m_pulseAnimation && m_pulseAnimation->state() != QAbstractAnimation::Running) {
            startPulseAnimation();
        } else if (!m_isPlaying && m_pulseAnimation && m_pulseAnimation->state() == QAbstractAnimation::Running) {
            stopPulseAnimation();
        }
        
        // 峰值电平衰减
        const float decayRate = 0.95f; // VU_METER_DECAY
        m_visualizationData.peakLeft *= decayRate;
        m_visualizationData.peakRight *= decayRate;
        
        // 确保峰值不低于当前电平
        m_visualizationData.peakLeft = qMax(m_visualizationData.peakLeft, m_visualizationData.leftLevel);
        m_visualizationData.peakRight = qMax(m_visualizationData.peakRight, m_visualizationData.rightLevel);
        
        // 每秒记录一次调试信息
        static int animCount = 0;
        if (++animCount % 60 == 0) { // 16ms * 60 ≈ 1秒
            logDebug(QString("Animation timer: playing=%1, spin=%2, pulse=%3")
                    .arg(m_isPlaying)
                    .arg(m_spinAnimation ? m_spinAnimation->state() : -1)
                    .arg(m_pulseAnimation ? m_pulseAnimation->state() : -1));
        }
        
    } catch (const std::exception& e) {
        handleError(QString("Error in animation timer: %1").arg(e.what()));
    } catch (...) {
        handleError("Unknown error occurred in animation timer");
    }
}

void PlayInterfaceController::onCoverLoadFinished()
{
    // 封面加载完成
}

void PlayInterfaceController::onLyricLoadFinished()
{
    // 歌词加载完成
}

// 私有方法实现
void PlayInterfaceController::setupConnections()
{
    try {
        logInfo("Setting up signal connections");
        
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
            connect(m_interface, &PlayInterface::muteToggled, 
                    this, &PlayInterfaceController::onMuteButtonClicked);
            
            // 显示模式和可视化信号
            connect(m_interface, &PlayInterface::displayModeChanged, 
                    this, &PlayInterfaceController::onDisplayModeClicked);
            // 注释掉 connect(m_interface, &PlayInterface::visualizationTypeChanged, 
            //         this, &PlayInterfaceController::onVisualizationTypeClicked);
            
            // 均衡器信号
            connect(m_interface, &PlayInterface::equalizerChanged, 
                    this, &PlayInterfaceController::onEqualizerSliderChanged);
            
            // 歌词信号
            // 注释掉 connect(m_interface, &PlayInterface::lyricToggled, 
            //         this, &PlayInterfaceController::onLyricClicked);
            
            logDebug("Connected interface signals");
        }
        
        // 连接定时器信号
        if (m_updateTimer) {
            connect(m_updateTimer, &QTimer::timeout, 
                    this, &PlayInterfaceController::onUpdateTimer);
            logDebug("Connected update timer");
        }
        
        if (m_visualizationTimer) {
            connect(m_visualizationTimer, &QTimer::timeout, 
                    this, &PlayInterfaceController::onVisualizationTimer);
            logDebug("Connected visualization timer");
        }
        
        if (m_lyricUpdateTimer) {
            connect(m_lyricUpdateTimer, &QTimer::timeout, 
                    this, &PlayInterfaceController::onLyricUpdateTimer);
            logDebug("Connected lyric update timer");
        }
        
        if (m_animationTimer) {
            connect(m_animationTimer, &QTimer::timeout, 
                    this, &PlayInterfaceController::onAnimationTimer);
            logDebug("Connected animation timer");
        }
        
        // 连接音频引擎信号（如果可用）
        if (m_audioEngine) {
            // 这些连接需要根据实际的AudioEngine接口来实现
            // connect(m_audioEngine, &AudioEngine::stateChanged, 
            //         this, &PlayInterfaceController::onPlaybackStateChanged);
            // connect(m_audioEngine, &AudioEngine::positionChanged, 
            //         this, &PlayInterfaceController::onPositionChanged);
            // connect(m_audioEngine, &AudioEngine::durationChanged, 
            //         this, &PlayInterfaceController::onDurationChanged);
            // connect(m_audioEngine, &AudioEngine::volumeChanged, 
            //         this, &PlayInterfaceController::onVolumeChanged);
            // connect(m_audioEngine, &AudioEngine::mutedChanged, 
            //         this, &PlayInterfaceController::onMutedChanged);
            // connect(m_audioEngine, &AudioEngine::audioDataAvailable, 
            //         this, &PlayInterfaceController::onAudioDataAvailable);
            // connect(m_audioEngine, &AudioEngine::spectrumDataAvailable, 
            //         this, &PlayInterfaceController::onSpectrumDataAvailable);
            // connect(m_audioEngine, &AudioEngine::levelDataAvailable, 
            //         this, &PlayInterfaceController::onLevelDataAvailable);
            
            logDebug("Audio engine connections prepared (commented out)");
        }
        
        logInfo("Signal connections setup completed successfully");
        
    } catch (const std::exception& e) {
        handleError(QString("Error setting up connections: %1").arg(e.what()));
    } catch (...) {
        handleError("Unknown error occurred while setting up connections");
    }
}

void PlayInterfaceController::setupVisualization()
{
    try {
        logInfo("Setting up visualization");
        
        if (!m_interface) {
            logError("Interface is null, cannot setup visualization");
            return;
        }
        
        // 创建可视化场景
        if (!m_waveformScene) {
            m_waveformScene = new QGraphicsScene(this);
            m_waveformScene->setSceneRect(0, 0, 400, 200);
            m_waveformScene->setBackgroundBrush(QBrush(QColor(20, 20, 20)));
            logDebug("Created waveform scene");
        }
        
        if (!m_spectrumScene) {
            m_spectrumScene = new QGraphicsScene(this);
            m_spectrumScene->setSceneRect(0, 0, 400, 200);
            m_spectrumScene->setBackgroundBrush(QBrush(QColor(20, 20, 20)));
            logDebug("Created spectrum scene");
        }
        
        if (!m_coverScene) {
            m_coverScene = new QGraphicsScene(this);
            m_coverScene->setSceneRect(0, 0, 300, 300);
            m_coverScene->setBackgroundBrush(QBrush(QColor(40, 40, 40)));
            logDebug("Created cover scene");
        }
        
        if (!m_lyricScene) {
            m_lyricScene = new QGraphicsScene(this);
            m_lyricScene->setSceneRect(0, 0, 400, 300);
            m_lyricScene->setBackgroundBrush(QBrush(QColor(30, 30, 30)));
            logDebug("Created lyric scene");
        }
        
        // 初始化可视化数据
        m_visualizationData.waveform.resize(WAVEFORM_SAMPLES);
        m_visualizationData.spectrum.resize(SPECTRUM_BANDS);
        m_visualizationData.leftLevel = 0.0f;
        m_visualizationData.rightLevel = 0.0f;
        m_visualizationData.peakLeft = 0.0f;
        m_visualizationData.peakRight = 0.0f;
        m_visualizationData.timestamp = 0;
        
        // 初始化均衡器值
        m_equalizerValues.resize(EQUALIZER_BANDS);
        for (int i = 0; i < EQUALIZER_BANDS; ++i) {
            m_equalizerValues[i] = 0; // 默认值
        }
        
        logInfo("Visualization setup completed successfully");
        
    } catch (const std::exception& e) {
        handleError(QString("Error setting up visualization: %1").arg(e.what()));
    } catch (...) {
        handleError("Unknown error occurred while setting up visualization");
    }
}

void PlayInterfaceController::setupAnimations()
{
    try {
        logInfo("Setting up animations");
        
        if (!m_interface) {
            logError("Interface is null, cannot setup animations");
            return;
        }
        
        // 创建旋转动画（用于封面旋转）
        if (!m_spinAnimation) {
            m_spinAnimation = new QPropertyAnimation(this);
            m_spinAnimation->setDuration(10000); // 10秒一圈
            m_spinAnimation->setLoopCount(-1); // 无限循环
            m_spinAnimation->setEasingCurve(QEasingCurve::Linear);
            logDebug("Created spin animation");
        }
        
        // 创建脉冲动画（用于播放状态指示）
        if (!m_pulseAnimation) {
            m_pulseAnimation = new QPropertyAnimation(this);
            m_pulseAnimation->setDuration(1000); // 1秒
            m_pulseAnimation->setLoopCount(-1); // 无限循环
            m_pulseAnimation->setEasingCurve(QEasingCurve::InOutSine);
            logDebug("Created pulse animation");
        }
        
        // 创建淡入淡出动画
        if (!m_fadeAnimation) {
            m_fadeAnimation = new QPropertyAnimation(this);
            m_fadeAnimation->setDuration(500); // 0.5秒
            m_fadeAnimation->setEasingCurve(QEasingCurve::InOutQuad);
            logDebug("Created fade animation");
        }
        
        // 创建动画组（用于组合动画）
        if (!m_animationGroup) {
            m_animationGroup = new QParallelAnimationGroup(this);
            logDebug("Created animation group");
        }
        
        // 连接动画信号
        if (m_spinAnimation) {
            connect(m_spinAnimation, &QPropertyAnimation::finished,
                    this, [this]() {
                logDebug("Spin animation finished");
            });
        }
        
        if (m_pulseAnimation) {
            connect(m_pulseAnimation, &QPropertyAnimation::finished,
                    this, [this]() {
                logDebug("Pulse animation finished");
            });
        }
        
        if (m_fadeAnimation) {
            connect(m_fadeAnimation, &QPropertyAnimation::finished,
                    this, [this]() {
                logDebug("Fade animation finished");
            });
        }
        
        logInfo("Animations setup completed successfully");
        
    } catch (const std::exception& e) {
        handleError(QString("Error setting up animations: %1").arg(e.what()));
    } catch (...) {
        handleError("Unknown error occurred while setting up animations");
    }
}

void PlayInterfaceController::loadSettings()
{
    if (m_settings) {
        m_volume = m_settings->value("Volume", 50).toInt();
        m_balance = m_settings->value("Balance", 0).toInt();
        m_visualizationEnabled = m_settings->value("VisualizationEnabled", true).toBool();
        m_equalizerPreset = m_settings->value("EqualizerPreset", "Default").toString();
    }
}

void PlayInterfaceController::saveSettings()
{
    if (m_settings) {
        m_settings->setValue("Volume", m_volume);
        m_settings->setValue("Balance", m_balance);
        m_settings->setValue("VisualizationEnabled", m_visualizationEnabled);
        m_settings->setValue("EqualizerPreset", m_equalizerPreset);
        m_settings->sync();
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
    logInfo(QString("播放状态改变: %1").arg(static_cast<int>(state)));
    // 更新播放控件状态
    if (m_interface) {
        m_interface->setPlaybackState(state == AudioTypes::AudioState::Playing);
    }
}

void PlayInterfaceController::onCurrentSongChanged(const Song& song)
{
    logInfo(QString("当前歌曲改变: %1").arg(song.title()));
    setCurrentSong(song);
}

void PlayInterfaceController::onPositionChanged(qint64 position)
{
    logInfo(QString("播放位置改变: %1").arg(position));
    if (m_interface) {
        m_interface->setCurrentTime(position);
    }
}

void PlayInterfaceController::onDurationChanged(qint64 duration)
{
    logInfo(QString("歌曲时长改变: %1").arg(duration));
    if (m_interface) {
        m_interface->setTotalTime(duration);
    }
}

void PlayInterfaceController::onVolumeChanged(int volume)
{
    logInfo(QString("音量改变: %1").arg(volume));
    m_volume = volume;
    if (m_interface) {
        m_interface->setVolume(volume);
    }
}

void PlayInterfaceController::onMutedChanged(bool muted)
{
    logInfo(QString("静音状态改变: %1").arg(muted ? "是" : "否"));
    if (m_interface) {
        m_interface->setMuted(muted);
    }
}

void PlayInterfaceController::onAudioDataAvailable(const VisualizationData& data)
{
    try {
        // 检查初始化状态
        if (!m_initialized || !m_visualizationEnabled) {
            return;
        }

        // 线程安全保护
        QMutexLocker locker(&m_mutex);
        
        // 数据有效性检查
        if (data.waveform.isEmpty() && data.spectrum.isEmpty()) {
            logDebug("Received empty visualization data");
            return;
        }

        // 更新可视化数据
        m_visualizationData = data;
        
        // 更新波形历史数据（保持最近的数据）
        if (!data.waveform.isEmpty()) {
            m_waveformHistory = {data.waveform};
            if (m_waveformHistory.size() > WAVEFORM_SAMPLES) {
                m_waveformHistory.resize(WAVEFORM_SAMPLES);
            }
        }
        
        // 更新频谱历史数据
        if (!data.spectrum.isEmpty()) {
            m_spectrumHistory = {data.spectrum};
            if (m_spectrumHistory.size() > SPECTRUM_BANDS) {
                m_spectrumHistory.resize(SPECTRUM_BANDS);
            }
        }
        
        // 根据当前可视化类型更新显示
        switch (m_visualizationType) {
            case VisualizationType::Waveform:
                if (!data.waveform.isEmpty()) {
                    drawWaveform(data.waveform);
                }
                break;
            case VisualizationType::Spectrum:
                if (!data.spectrum.isEmpty()) {
                    drawSpectrum(data.spectrum);
                }
                break;
            case VisualizationType::VUMeter:
                drawVUMeter(data.leftLevel, data.rightLevel);
                break;
            case VisualizationType::Oscilloscope:
                if (!data.waveform.isEmpty()) {
                    drawOscilloscope(data.waveform);
                }
                break;
            case VisualizationType::Bars:
                if (!data.spectrum.isEmpty()) {
                    drawBars(data.spectrum);
                }
                break;
            case VisualizationType::Particles:
                if (!data.spectrum.isEmpty()) {
                    drawParticles(data.spectrum);
                }
                break;
        }
        
        logDebug(QString("Processed audio data: waveform=%1 samples, spectrum=%2 bands, levels=L:%3 R:%4")
                .arg(data.waveform.size())
                .arg(data.spectrum.size())
                .arg(data.leftLevel, 0, 'f', 3)
                .arg(data.rightLevel, 0, 'f', 3));
                
    } catch (const std::exception& e) {
        handleError(QString("Error processing audio data: %1").arg(e.what()));
    } catch (...) {
        handleError("Unknown error occurred while processing audio data");
    }
}

void PlayInterfaceController::onSpectrumDataAvailable(const QVector<float>& spectrum)
{
    try {
        // 检查初始化状态和可视化启用状态
        if (!m_initialized || !m_visualizationEnabled) {
            return;
        }
        
        // 数据有效性检查
        if (spectrum.isEmpty()) {
            logDebug("Received empty spectrum data");
            return;
        }
        
        // 线程安全保护
        QMutexLocker locker(&m_mutex);
        
        // 更新频谱历史数据
        m_spectrumHistory = {spectrum};
        
        // 限制频谱数据大小
        if (m_spectrumHistory.size() > SPECTRUM_BANDS) {
            m_spectrumHistory.resize(SPECTRUM_BANDS);
        }
        
        // 更新可视化数据结构中的频谱数据
        if (!m_spectrumHistory.isEmpty()) {
            m_visualizationData.spectrum = m_spectrumHistory.first();
        }
        m_visualizationData.timestamp = QDateTime::currentMSecsSinceEpoch();
        
        // 根据当前可视化类型更新显示
        switch (m_visualizationType) {
            case VisualizationType::Spectrum:
                if (!m_spectrumHistory.isEmpty()) {
                    drawSpectrum(m_spectrumHistory.first());
                }
                break;
            case VisualizationType::Bars:
                if (!m_spectrumHistory.isEmpty()) {
                    drawBars(m_spectrumHistory.first());
                }
                break;
            case VisualizationType::Particles:
                if (!m_spectrumHistory.isEmpty()) {
                    drawParticles(m_spectrumHistory.first());
                }
                break;
            default:
                // 对于其他可视化类型，仅更新数据但不重绘
                break;
        }
        
        // 计算频谱统计信息
        float avgLevel = 0.0f;
        float peakLevel = 0.0f;
        for (const auto& spectrum : m_spectrumHistory) {
            for (float value : spectrum) {
                avgLevel += value;
                peakLevel = qMax(peakLevel, value);
            }
        }
        avgLevel /= m_spectrumHistory.size();
        
        logDebug(QString("Processed spectrum data: %1 bands, avg=%2, peak=%3")
                .arg(m_spectrumHistory.size())
                .arg(avgLevel, 0, 'f', 3)
                .arg(peakLevel, 0, 'f', 3));
                
    } catch (const std::exception& e) {
        handleError(QString("Error processing spectrum data: %1").arg(e.what()));
    } catch (...) {
        handleError("Unknown error occurred while processing spectrum data");
    }
}

void PlayInterfaceController::onLevelDataAvailable(float left, float right)
{
    try {
        // 检查初始化状态和可视化启用状态
        if (!m_initialized || !m_visualizationEnabled) {
            return;
        }
        
        // 数据有效性检查
        if (left < 0.0f || right < 0.0f || left > 1.0f || right > 1.0f) {
            logDebug(QString("Invalid level data: L=%1, R=%2").arg(left).arg(right));
            return;
        }
        
        // 线程安全保护
        QMutexLocker locker(&m_mutex);
        
        // 更新当前电平值
        m_visualizationData.leftLevel = left;
        m_visualizationData.rightLevel = right;
        
        // 更新峰值电平（带衰减）
        static float leftPeak = 0.0f;
        static float rightPeak = 0.0f;
        static qint64 lastPeakUpdate = 0;
        
        qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
        
        // 峰值保持和衰减逻辑
        if (left > leftPeak) {
            leftPeak = left;
            lastPeakUpdate = currentTime;
        } else if (currentTime - lastPeakUpdate > PEAK_HOLD_TIME) {
            leftPeak *= VU_METER_DECAY;
        }
        
        if (right > rightPeak) {
            rightPeak = right;
            lastPeakUpdate = currentTime;
        } else if (currentTime - lastPeakUpdate > PEAK_HOLD_TIME) {
            rightPeak *= VU_METER_DECAY;
        }
        
        // 更新峰值数据
        m_visualizationData.peakLeft = leftPeak;
        m_visualizationData.peakRight = rightPeak;
        m_visualizationData.timestamp = currentTime;
        
        // 根据当前可视化类型更新显示
        if (m_visualizationType == VisualizationType::VUMeter) {
            drawVUMeter(left, right);
        }
        
        // 检查音频削波（过载）
        static const float CLIPPING_THRESHOLD = 0.95f;
        if (left > CLIPPING_THRESHOLD || right > CLIPPING_THRESHOLD) {
            logDebug(QString("Audio clipping detected: L=%1, R=%2")
                    .arg(left, 0, 'f', 3)
                    .arg(right, 0, 'f', 3));
        }
        
        // 详细调试信息（仅在需要时启用）
        static int debugCounter = 0;
        if (++debugCounter % 100 == 0) { // 每100次更新输出一次调试信息
            logDebug(QString("Level data: L=%1(peak:%2), R=%3(peak:%4)")
                    .arg(left, 0, 'f', 3)
                    .arg(leftPeak, 0, 'f', 3)
                    .arg(right, 0, 'f', 3)
                    .arg(rightPeak, 0, 'f', 3));
        }
        
    } catch (const std::exception& e) {
        handleError(QString("Error processing level data: %1").arg(e.what()));
    } catch (...) {
        handleError("Unknown error occurred while processing level data");
    }
}

void PlayInterfaceController::onPlayPauseClicked()
{
    logInfo("播放/暂停按钮点击");
    emit playRequested();
}

void PlayInterfaceController::onPlayModeClicked()
{
    logInfo("播放模式按钮点击");
    // 发出信号让主控制器处理播放模式切换
    emit playModeChangeRequested();
}

void PlayInterfaceController::onNextClicked()
{
    logInfo("下一首按钮点击");
    emit nextRequested();
}

void PlayInterfaceController::onPreviousClicked()
{
    logInfo("上一首按钮点击");
    emit previousRequested();
}

void PlayInterfaceController::onVolumeSliderChanged(int value)
{
    logInfo(QString("音量滑块改变: %1").arg(value));
    m_volume = value;
    emit volumeChanged(value);
}

void PlayInterfaceController::onBalanceSliderChanged(int value)
{
    logInfo(QString("平衡滑块改变: %1").arg(value));
    m_balance = value;
    emit balanceChanged(value);
}

void PlayInterfaceController::onPositionSliderChanged(int value)
{
    logInfo(QString("位置滑块改变: %1").arg(value));
    qint64 position = static_cast<qint64>(value);
    emit seekRequested(position);
}

void PlayInterfaceController::onMuteButtonClicked()
{
    logInfo("静音按钮点击");
    emit muteToggled(m_interface ? m_interface->isMuted() : false);
}

void PlayInterfaceController::onDisplayModeClicked()
{
    logInfo("显示模式按钮点击");
    // 切换显示模式
    DisplayMode newMode = (m_displayMode == DisplayMode::Lyrics) ? DisplayMode::Cover : DisplayMode::Lyrics;
    setDisplayMode(newMode);
}

void PlayInterfaceController::onVisualizationTypeClicked()
{
    logInfo("可视化类型按钮点击");
    int currentType = static_cast<int>(m_visualizationType);
    int nextType = (currentType + 1) % 6;
    setVisualizationType(static_cast<VisualizationType>(nextType));
}

void PlayInterfaceController::onEqualizerSliderChanged(const QVector<int>& values)
{
    logInfo(QString("均衡器滑块改变: %1个值").arg(values.size()));
    // 处理所有频段的值
    for (int i = 0; i < values.size() && i < 10; ++i) {
        emit equalizerChanged(static_cast<EqualizerBand>(i), values[i]);
    }
}

void PlayInterfaceController::onLyricClicked(qint64 position)
{
    logInfo(QString("歌词点击: %1").arg(position));
    emit seekRequested(position);
}

// ==================== 可视化绘制方法实现 ====================

void PlayInterfaceController::drawWaveform(const QVector<float>& data)
{
    try {
        if (!m_waveformScene || data.isEmpty()) {
            return;
        }
        
        // 清除之前的绘制内容
        m_waveformScene->clear();
        
        // 获取场景尺寸
        QRectF sceneRect = m_waveformScene->sceneRect();
        if (sceneRect.isEmpty()) {
            sceneRect = QRectF(0, 0, 400, 200); // 默认尺寸
            m_waveformScene->setSceneRect(sceneRect);
        }
        
        float width = sceneRect.width();
        float height = sceneRect.height();
        float centerY = height / 2.0f;
        
        // 计算采样步长
        int sampleCount = qMin(data.size(), static_cast<int>(width));
        float stepX = width / static_cast<float>(sampleCount);
        
        // 创建波形路径
        QPainterPath waveformPath;
        bool firstPoint = true;
        
        for (int i = 0; i < sampleCount; ++i) {
            float x = i * stepX;
            int dataIndex = (i * data.size()) / sampleCount;
            float amplitude = qBound(-1.0f, data[dataIndex], 1.0f);
            float y = centerY - (amplitude * centerY * 0.8f); // 0.8f 为缩放因子
            
            if (firstPoint) {
                waveformPath.moveTo(x, y);
                firstPoint = false;
            } else {
                waveformPath.lineTo(x, y);
            }
        }
        
        // 设置画笔和画刷
        QPen pen(QColor(0, 150, 255), 2); // 蓝色波形线
        pen.setCapStyle(Qt::RoundCap);
        pen.setJoinStyle(Qt::RoundJoin);
        
        // 添加到场景
        m_waveformScene->addPath(waveformPath, pen);
        
        // 添加中心线
        QPen centerLinePen(QColor(100, 100, 100), 1, Qt::DashLine);
        m_waveformScene->addLine(0, centerY, width, centerY, centerLinePen);
        
        logDebug(QString("Drew waveform: %1 samples, scene size: %2x%3")
                .arg(sampleCount)
                .arg(static_cast<int>(width))
                .arg(static_cast<int>(height)));
                
    } catch (const std::exception& e) {
        handleError(QString("Error drawing waveform: %1").arg(e.what()));
    } catch (...) {
        handleError("Unknown error occurred while drawing waveform");
    }
}

void PlayInterfaceController::drawSpectrum(const QVector<float>& data)
{
    try {
        if (!m_spectrumScene || data.isEmpty()) {
            return;
        }
        
        // 清除之前的绘制内容
        m_spectrumScene->clear();
        
        // 获取场景尺寸
        QRectF sceneRect = m_spectrumScene->sceneRect();
        if (sceneRect.isEmpty()) {
            sceneRect = QRectF(0, 0, 400, 200); // 默认尺寸
            m_spectrumScene->setSceneRect(sceneRect);
        }
        
        float width = sceneRect.width();
        float height = sceneRect.height();
        
        // 计算频谱条的数量和宽度
        int barCount = qMin(data.size(), SPECTRUM_BANDS);
        float barWidth = width / static_cast<float>(barCount);
        float barSpacing = barWidth * 0.1f; // 10% 间距
        float actualBarWidth = barWidth - barSpacing;
        
        // 绘制频谱条
        for (int i = 0; i < barCount; ++i) {
            float amplitude = qBound(0.0f, data[i], 1.0f);
            float barHeight = amplitude * height * 0.9f; // 0.9f 为缩放因子
            
            float x = i * barWidth + barSpacing / 2.0f;
            float y = height - barHeight;
            
            // 根据频率获取颜色
            QColor barColor = getSpectrumColor(i);
            
            // 创建渐变效果
            QLinearGradient gradient(0, y, 0, height);
            gradient.setColorAt(0, barColor.lighter(150));
            gradient.setColorAt(1, barColor.darker(120));
            
            QBrush brush(gradient);
            QPen pen(barColor.darker(150), 1);
            
            // 添加频谱条
            QRectF barRect(x, y, actualBarWidth, barHeight);
            m_spectrumScene->addRect(barRect, pen, brush);
        }
        
        logDebug(QString("Drew spectrum: %1 bars, scene size: %2x%3")
                .arg(barCount)
                .arg(static_cast<int>(width))
                .arg(static_cast<int>(height)));
                
    } catch (const std::exception& e) {
        handleError(QString("Error drawing spectrum: %1").arg(e.what()));
    } catch (...) {
        handleError("Unknown error occurred while drawing spectrum");
    }
}

void PlayInterfaceController::drawVUMeter(float leftLevel, float rightLevel)
{
    try {
        if (!m_spectrumScene) { // 复用频谱场景
            return;
        }
        
        // 清除之前的绘制内容
        m_spectrumScene->clear();
        
        // 获取场景尺寸
        QRectF sceneRect = m_spectrumScene->sceneRect();
        if (sceneRect.isEmpty()) {
            sceneRect = QRectF(0, 0, 400, 200); // 默认尺寸
            m_spectrumScene->setSceneRect(sceneRect);
        }
        
        float width = sceneRect.width();
        float height = sceneRect.height();
        
        // VU表参数
        float meterWidth = width * 0.4f; // 每个表的宽度
        float meterHeight = height * 0.8f;
        float meterSpacing = width * 0.1f;
        
        // 左声道VU表
        float leftX = meterSpacing;
        float leftY = (height - meterHeight) / 2.0f;
        drawSingleVUMeter(leftX, leftY, meterWidth, meterHeight, leftLevel, "L");
        
        // 右声道VU表
        float rightX = leftX + meterWidth + meterSpacing;
        float rightY = leftY;
        drawSingleVUMeter(rightX, rightY, meterWidth, meterHeight, rightLevel, "R");
        
        logDebug(QString("Drew VU meters: L=%1, R=%2")
                .arg(leftLevel, 0, 'f', 3)
                .arg(rightLevel, 0, 'f', 3));
                
    } catch (const std::exception& e) {
        handleError(QString("Error drawing VU meter: %1").arg(e.what()));
    } catch (...) {
        handleError("Unknown error occurred while drawing VU meter");
    }
}

void PlayInterfaceController::drawOscilloscope(const QVector<float>& data)
{
    try {
        if (!m_waveformScene || data.isEmpty()) {
            return;
        }
        
        // 清除之前的绘制内容
        m_waveformScene->clear();
        
        // 获取场景尺寸
        QRectF sceneRect = m_waveformScene->sceneRect();
        if (sceneRect.isEmpty()) {
            sceneRect = QRectF(0, 0, 400, 200); // 默认尺寸
            m_waveformScene->setSceneRect(sceneRect);
        }
        
        float width = sceneRect.width();
        float height = sceneRect.height();
        float centerY = height / 2.0f;
        
        // 绘制网格
        QPen gridPen(QColor(50, 50, 50), 1, Qt::DotLine);
        
        // 水平网格线
        for (int i = 1; i < 4; ++i) {
            float y = (height / 4.0f) * i;
            m_waveformScene->addLine(0, y, width, y, gridPen);
        }
        
        // 垂直网格线
        for (int i = 1; i < 8; ++i) {
            float x = (width / 8.0f) * i;
            m_waveformScene->addLine(x, 0, x, height, gridPen);
        }
        
        // 绘制示波器波形
        int sampleCount = qMin(data.size(), static_cast<int>(width));
        float stepX = width / static_cast<float>(sampleCount);
        
        QPainterPath oscilloscopePath;
        bool firstPoint = true;
        
        for (int i = 0; i < sampleCount; ++i) {
            float x = i * stepX;
            int dataIndex = (i * data.size()) / sampleCount;
            float amplitude = qBound(-1.0f, data[dataIndex], 1.0f);
            float y = centerY - (amplitude * centerY * 0.9f);
            
            if (firstPoint) {
                oscilloscopePath.moveTo(x, y);
                firstPoint = false;
            } else {
                oscilloscopePath.lineTo(x, y);
            }
        }
        
        // 设置示波器波形样式
        QPen oscilloscopePen(QColor(0, 255, 0), 2); // 绿色波形
        oscilloscopePen.setCapStyle(Qt::RoundCap);
        
        m_waveformScene->addPath(oscilloscopePath, oscilloscopePen);
        
        logDebug(QString("Drew oscilloscope: %1 samples")
                .arg(sampleCount));
                
    } catch (const std::exception& e) {
        handleError(QString("Error drawing oscilloscope: %1").arg(e.what()));
    } catch (...) {
        handleError("Unknown error occurred while drawing oscilloscope");
    }
}

void PlayInterfaceController::drawBars(const QVector<float>& data)
{
    try {
        if (!m_spectrumScene || data.isEmpty()) {
            return;
        }
        
        // 清除之前的绘制内容
        m_spectrumScene->clear();
        
        // 获取场景尺寸
        QRectF sceneRect = m_spectrumScene->sceneRect();
        if (sceneRect.isEmpty()) {
            sceneRect = QRectF(0, 0, 400, 200); // 默认尺寸
            m_spectrumScene->setSceneRect(sceneRect);
        }
        
        float width = sceneRect.width();
        float height = sceneRect.height();
        
        // 计算条形图参数
        int barCount = qMin(data.size(), SPECTRUM_BANDS);
        float barWidth = width / static_cast<float>(barCount);
        float barSpacing = barWidth * 0.05f; // 5% 间距
        float actualBarWidth = barWidth - barSpacing;
        
        // 绘制3D效果的条形图
        for (int i = 0; i < barCount; ++i) {
            float amplitude = qBound(0.0f, data[i], 1.0f);
            float barHeight = amplitude * height * 0.95f;
            
            float x = i * barWidth + barSpacing / 2.0f;
            float y = height - barHeight;
            
            // 根据高度获取颜色（彩虹效果）
            QColor barColor;
            if (amplitude < 0.3f) {
                barColor = QColor(0, 255 * amplitude / 0.3f, 255); // 蓝到青
            } else if (amplitude < 0.6f) {
                float t = (amplitude - 0.3f) / 0.3f;
                barColor = QColor(255 * t, 255, 255 * (1 - t)); // 青到黄
            } else {
                float t = (amplitude - 0.6f) / 0.4f;
                barColor = QColor(255, 255 * (1 - t), 0); // 黄到红
            }
            
            // 创建3D渐变效果
            QLinearGradient gradient(x, y, x + actualBarWidth, y);
            gradient.setColorAt(0, barColor.lighter(130));
            gradient.setColorAt(0.5, barColor);
            gradient.setColorAt(1, barColor.darker(130));
            
            QBrush brush(gradient);
            QPen pen(barColor.darker(150), 1);
            
            // 主条形
            QRectF barRect(x, y, actualBarWidth, barHeight);
            m_spectrumScene->addRect(barRect, pen, brush);
            
            // 顶部高光效果
            if (barHeight > 5) {
                QPen highlightPen(barColor.lighter(200), 2);
                m_spectrumScene->addLine(x + 1, y + 1, x + actualBarWidth - 1, y + 1, highlightPen);
            }
        }
        
        logDebug(QString("Drew bars: %1 bars, scene size: %2x%3")
                .arg(barCount)
                .arg(static_cast<int>(width))
                .arg(static_cast<int>(height)));
                
    } catch (const std::exception& e) {
        handleError(QString("Error drawing bars: %1").arg(e.what()));
    } catch (...) {
        handleError("Unknown error occurred while drawing bars");
    }
}

void PlayInterfaceController::drawParticles(const QVector<float>& data)
{
    try {
        if (!m_spectrumScene || data.isEmpty()) {
            return;
        }
        
        // 清除之前的绘制内容
        m_spectrumScene->clear();
        
        // 获取场景尺寸
        QRectF sceneRect = m_spectrumScene->sceneRect();
        if (sceneRect.isEmpty()) {
            sceneRect = QRectF(0, 0, 400, 200); // 默认尺寸
            m_spectrumScene->setSceneRect(sceneRect);
        }
        
        float width = sceneRect.width();
        float height = sceneRect.height();
        
        // 粒子系统参数
        int particleCount = qMin(data.size() * 3, 200); // 限制粒子数量
        
        for (int i = 0; i < particleCount; ++i) {
            int dataIndex = i % data.size();
            float amplitude = qBound(0.0f, data[dataIndex], 1.0f);
            
            if (amplitude < 0.1f) continue; // 跳过低幅度
            
            // 随机粒子位置
            float x = (static_cast<float>(dataIndex) / data.size()) * width;
            x += (QRandomGenerator::global()->bounded(20) - 10); // 添加随机偏移
            
            float baseY = height - (amplitude * height * 0.8f);
            float y = baseY + (QRandomGenerator::global()->bounded(40) - 20); // 添加随机偏移
            
            // 确保粒子在场景内
            x = qBound(0.0f, x, width);
            y = qBound(0.0f, y, height);
            
            // 粒子大小基于幅度
            float particleSize = 2 + amplitude * 8;
            
            // 粒子颜色基于频率和幅度
            QColor particleColor = getSpectrumColor(dataIndex);
            particleColor.setAlphaF(amplitude * 0.8f + 0.2f);
            
            // 创建粒子（圆形）
            QBrush brush(particleColor);
            QPen pen(particleColor.lighter(150), 1);
            
            QRectF particleRect(x - particleSize/2, y - particleSize/2, particleSize, particleSize);
            m_spectrumScene->addEllipse(particleRect, pen, brush);
            
            // 添加粒子轨迹效果
            if (amplitude > 0.5f) {
                QPen trailPen(particleColor, 1);
                trailPen.setStyle(Qt::DashLine);
                m_spectrumScene->addLine(x, y, x, height, trailPen);
            }
        }
        
        logDebug(QString("Drew particles: %1 particles, scene size: %2x%3")
                .arg(particleCount)
                .arg(static_cast<int>(width))
                .arg(static_cast<int>(height)));
                
    } catch (const std::exception& e) {
        handleError(QString("Error drawing particles: %1").arg(e.what()));
    } catch (...) {
        handleError("Unknown error occurred while drawing particles");
    }
}

// ==================== 辅助方法实现 ====================

void PlayInterfaceController::drawSingleVUMeter(float x, float y, float width, float height, float level, const QString& label)
{
    try {
        if (!m_spectrumScene) {
            return;
        }
        
        // VU表背景
        QBrush backgroundBrush(QColor(20, 20, 20));
        QPen backgroundPen(QColor(100, 100, 100), 1);
        QRectF backgroundRect(x, y, width, height);
        m_spectrumScene->addRect(backgroundRect, backgroundPen, backgroundBrush);
        
        // VU表刻度
        QPen scalePen(QColor(150, 150, 150), 1);
        QFont scaleFont("Arial", 8);
        
        // 绘制刻度线和标签
        for (int i = 0; i <= 10; ++i) {
            float scaleY = y + height - (i * height / 10.0f);
            float scaleX1 = x + width - 10;
            float scaleX2 = x + width - 5;
            
            m_spectrumScene->addLine(scaleX1, scaleY, scaleX2, scaleY, scalePen);
            
            // 添加刻度标签
            if (i % 2 == 0) {
                QString scaleText = QString::number(i * 10);
                QGraphicsTextItem* textItem = m_spectrumScene->addText(scaleText, scaleFont);
                textItem->setPos(scaleX2 + 2, scaleY - 6);
                textItem->setDefaultTextColor(QColor(150, 150, 150));
            }
        }
        
        // VU表电平条
        float levelHeight = level * height;
        float levelY = y + height - levelHeight;
        
        QColor levelColor = getVUMeterColor(level);
        
        // 创建电平条渐变
        QLinearGradient levelGradient(x, levelY, x, y + height);
        levelGradient.setColorAt(0, levelColor.lighter(120));
        levelGradient.setColorAt(1, levelColor.darker(120));
        
        QBrush levelBrush(levelGradient);
        QPen levelPen(levelColor.darker(150), 1);
        
        QRectF levelRect(x + 2, levelY, width - 15, levelHeight);
        m_spectrumScene->addRect(levelRect, levelPen, levelBrush);
        
        // 峰值指示器
        float peakLevel = (label == "L") ? m_visualizationData.peakLeft : m_visualizationData.peakRight;
        if (peakLevel > 0.01f) {
            float peakY = y + height - (peakLevel * height);
            QPen peakPen(QColor(255, 255, 0), 2); // 黄色峰值线
            m_spectrumScene->addLine(x + 2, peakY, x + width - 15, peakY, peakPen);
        }
        
        // 声道标签
        QFont labelFont("Arial", 12, QFont::Bold);
        QGraphicsTextItem* labelItem = m_spectrumScene->addText(label, labelFont);
        labelItem->setPos(x + width/2 - 6, y - 20);
        labelItem->setDefaultTextColor(QColor(200, 200, 200));
        
    } catch (const std::exception& e) {
        handleError(QString("Error drawing single VU meter: %1").arg(e.what()));
    }
}

QColor PlayInterfaceController::getSpectrumColor(int index) const
{
    try {
        // 基于频率索引生成彩虹色谱
        float hue = (static_cast<float>(index) / SPECTRUM_BANDS) * 360.0f;
        
        // 确保色调在有效范围内
        while (hue >= 360.0f) hue -= 360.0f;
        while (hue < 0.0f) hue += 360.0f;
        
        // 创建HSV颜色，高饱和度和亮度
        QColor color = QColor::fromHsvF(hue / 360.0f, 0.8f, 0.9f);
        
        return color;
        
    } catch (const std::exception& e) {
        const_cast<PlayInterfaceController*>(this)->logError(QString("Error getting spectrum color: %1").arg(e.what()));
        return QColor(100, 150, 255); // 默认蓝色
    } catch (...) {
        const_cast<PlayInterfaceController*>(this)->logError("Unknown error occurred while getting spectrum color");
        return QColor(100, 150, 255); // 默认蓝色
    }
}

QColor PlayInterfaceController::getVUMeterColor(float level) const
{
    try {
        // 根据电平值返回相应颜色
        level = qBound(0.0f, level, 1.0f);
        
        if (level < 0.6f) {
            // 绿色区域 (0-60%)
            float greenIntensity = 100 + (level / 0.6f) * 155; // 100-255
            return QColor(0, static_cast<int>(greenIntensity), 0);
        } else if (level < 0.8f) {
            // 黄色区域 (60-80%)
            float t = (level - 0.6f) / 0.2f; // 0-1
            int red = static_cast<int>(255 * t);
            return QColor(red, 255, 0);
        } else if (level < 0.95f) {
            // 橙色区域 (80-95%)
            return QColor(255, 165, 0);
        } else {
            // 红色区域 (95-100%)
            return QColor(255, 0, 0);
        }
        
    } catch (const std::exception& e) {
        const_cast<PlayInterfaceController*>(this)->logError(QString("Error getting VU meter color: %1").arg(e.what()));
        return QColor(100, 255, 100); // 默认绿色
    } catch (...) {
        const_cast<PlayInterfaceController*>(this)->logError("Unknown error occurred while getting VU meter color");
        return QColor(100, 255, 100); // 默认绿色
    }
}

void PlayInterfaceController::updateVisualizationHistory(const QVector<float>& newData)
{
    try {
        if (newData.isEmpty()) {
            return;
        }
        
        QMutexLocker locker(&m_visualizationMutex);
        
        // 更新波形历史数据
        if (!m_visualizationData.waveform.isEmpty()) {
            m_waveformHistory.append(m_visualizationData.waveform);
            
            // 限制历史数据大小
            const int maxHistorySize = 100;
            while (m_waveformHistory.size() > maxHistorySize) {
                m_waveformHistory.removeFirst();
            }
        }
        
        // 更新频谱历史数据
        if (!m_visualizationData.spectrum.isEmpty()) {
            m_spectrumHistory.append(m_visualizationData.spectrum);
            
            // 限制历史数据大小
            const int maxHistorySize = 50;
            while (m_spectrumHistory.size() > maxHistorySize) {
                m_spectrumHistory.removeFirst();
            }
        }
        
        logDebug(QString("Updated visualization history: waveform=%1, spectrum=%2")
                .arg(m_waveformHistory.size())
                .arg(m_spectrumHistory.size()));
                
    } catch (const std::exception& e) {
        handleError(QString("Error updating visualization history: %1").arg(e.what()));
    } catch (...) {
        handleError("Unknown error occurred while updating visualization history");
    }
}

void PlayInterfaceController::calculateSpectrumStatistics(const QVector<float>& spectrum)
{
    try {
        if (spectrum.isEmpty()) {
            return;
        }
        
        QMutexLocker locker(&m_visualizationMutex);
        
        // 计算频谱统计信息
        float sum = 0.0f;
        float maxValue = 0.0f;
        float minValue = 1.0f;
        
        for (float value : spectrum) {
            sum += value;
            maxValue = qMax(maxValue, value);
            minValue = qMin(minValue, value);
        }
        
        float average = sum / spectrum.size();
        
        // 计算RMS (均方根)
        float rmsSum = 0.0f;
        for (float value : spectrum) {
            float diff = value - average;
            rmsSum += diff * diff;
        }
        float rms = qSqrt(rmsSum / spectrum.size());
        
        // 查找主要频率峰值
        int peakIndex = 0;
        for (int i = 1; i < spectrum.size(); ++i) {
            if (spectrum[i] > spectrum[peakIndex]) {
                peakIndex = i;
            }
        }
        
        // 记录统计信息（每秒一次，避免日志过多）
        static QDateTime lastLogTime;
        QDateTime currentTime = QDateTime::currentDateTime();
        if (lastLogTime.isNull() || lastLogTime.msecsTo(currentTime) >= 1000) {
            logDebug(QString("Spectrum stats - Avg: %1, Max: %2, Min: %3, RMS: %4, Peak@%5")
                    .arg(average, 0, 'f', 3)
                    .arg(maxValue, 0, 'f', 3)
                    .arg(minValue, 0, 'f', 3)
                    .arg(rms, 0, 'f', 3)
                    .arg(peakIndex));
            lastLogTime = currentTime;
        }
        
    } catch (const std::exception& e) {
        handleError(QString("Error calculating spectrum statistics: %1").arg(e.what()));
    } catch (...) {
        handleError("Unknown error occurred while calculating spectrum statistics");
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

void PlayInterfaceController::updateVisualizationDisplay()
{
    try {
        if (!m_interface || !m_visualizationEnabled) {
            return;
        }
        
        // 更新可视化显示区域
        logDebug(QString("Visualization display updated: type=%1")
                .arg(static_cast<int>(m_visualizationType)));
        
    } catch (const std::exception& e) {
        handleError(QString("Error updating visualization display: %1").arg(e.what()));
    } catch (...) {
        handleError("Unknown error occurred while updating visualization display");
    }
}

void PlayInterfaceController::updateLyricHighlight(qint64 currentTime)
{
    try {
        if (m_lyrics.isEmpty()) {
            return;
        }
        
        // 查找当前时间对应的歌词行
        int newIndex = -1;
        for (int i = 0; i < m_lyrics.size(); ++i) {
            if (currentTime >= m_lyrics[i].timestamp) {
                newIndex = i;
            } else {
                break;
            }
        }
        
        // 更新高亮状态
        if (newIndex != m_currentLyricIndex) {
            // 清除之前的高亮
            if (m_currentLyricIndex >= 0 && m_currentLyricIndex < m_lyrics.size()) {
                m_lyrics[m_currentLyricIndex].isHighlighted = false;
            }
            
            // 设置新的高亮
            m_currentLyricIndex = newIndex;
            if (m_currentLyricIndex >= 0 && m_currentLyricIndex < m_lyrics.size()) {
                m_lyrics[m_currentLyricIndex].isHighlighted = true;
                logDebug(QString("Lyric highlight updated: index=%1, text=%2")
                        .arg(m_currentLyricIndex)
                        .arg(m_lyrics[m_currentLyricIndex].text));
            }
        }
        
    } catch (const std::exception& e) {
        handleError(QString("Error updating lyric highlight: %1").arg(e.what()));
    } catch (...) {
        handleError("Unknown error occurred while updating lyric highlight");
    }
}

void PlayInterfaceController::updateLyricDisplay()
{
    try {
        if (!m_interface || !m_lyricScene) {
            return;
        }
        
        // 更新歌词显示
        logDebug(QString("Lyric display updated: current=%1/%2")
                .arg(m_currentLyricIndex)
                .arg(m_lyrics.size()));
        
    } catch (const std::exception& e) {
        handleError(QString("Error updating lyric display: %1").arg(e.what()));
    } catch (...) {
        handleError("Unknown error occurred while updating lyric display");
    }
}

void PlayInterfaceController::scrollToCurrentLyric()
{
    try {
        if (!m_interface || m_currentLyricIndex < 0 || m_currentLyricIndex >= m_lyrics.size()) {
            return;
        }
        
        // 滚动到当前歌词
        logDebug(QString("Scrolled to lyric: index=%1").arg(m_currentLyricIndex));
        
    } catch (const std::exception& e) {
        handleError(QString("Error scrolling to current lyric: %1").arg(e.what()));
    } catch (...) {
        handleError("Unknown error occurred while scrolling to current lyric");
    }
}

void PlayInterfaceController::startSpinAnimation()
{
    try {
        if (!m_spinAnimation || !m_coverItem) {
            return;
        }
        
        if (m_spinAnimation->state() != QAbstractAnimation::Running) {
            m_spinAnimation->start();
            logDebug("Spin animation started");
        }
        
    } catch (const std::exception& e) {
        handleError(QString("Error starting spin animation: %1").arg(e.what()));
    } catch (...) {
        handleError("Unknown error occurred while starting spin animation");
    }
}

void PlayInterfaceController::stopSpinAnimation()
{
    try {
        if (!m_spinAnimation) {
            return;
        }
        
        if (m_spinAnimation->state() == QAbstractAnimation::Running) {
            m_spinAnimation->stop();
            logDebug("Spin animation stopped");
        }
        
    } catch (const std::exception& e) {
        handleError(QString("Error stopping spin animation: %1").arg(e.what()));
    } catch (...) {
        handleError("Unknown error occurred while stopping spin animation");
    }
}

void PlayInterfaceController::startPulseAnimation()
{
    try {
        if (!m_pulseAnimation) {
            return;
        }
        
        if (m_pulseAnimation->state() != QAbstractAnimation::Running) {
            m_pulseAnimation->start();
            logDebug("Pulse animation started");
        }
        
    } catch (const std::exception& e) {
        handleError(QString("Error starting pulse animation: %1").arg(e.what()));
    } catch (...) {
        handleError("Unknown error occurred while starting pulse animation");
    }
}

void PlayInterfaceController::stopPulseAnimation()
{
    try {
        if (!m_pulseAnimation) {
            return;
        }
        
        if (m_pulseAnimation->state() == QAbstractAnimation::Running) {
            m_pulseAnimation->stop();
            logDebug("Pulse animation stopped");
        }
        
    } catch (const std::exception& e) {
        handleError(QString("Error stopping pulse animation: %1").arg(e.what()));
    } catch (...) {
        handleError("Unknown error occurred while stopping pulse animation");
    }
}

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