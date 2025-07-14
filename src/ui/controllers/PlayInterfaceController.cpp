#include "playinterfacecontroller.h"
#include "../dialogs/playinterface.h"
#include <QDebug>

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
    // 更新定时器
}

void PlayInterfaceController::onVisualizationTimer()
{
    // 可视化更新定时器
}

void PlayInterfaceController::onLyricUpdateTimer()
{
    // 歌词更新定时器
}

void PlayInterfaceController::onAnimationTimer()
{
    // 动画定时器
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
    // 设置信号连接
    if (m_interface) {
        // 连接界面信号
        connect(m_interface, &PlayInterface::playPauseClicked, this, &PlayInterfaceController::playRequested);
        connect(m_interface, &PlayInterface::stopClicked, this, &PlayInterfaceController::stopRequested);
        connect(m_interface, &PlayInterface::nextClicked, this, &PlayInterfaceController::nextRequested);
        connect(m_interface, &PlayInterface::previousClicked, this, &PlayInterfaceController::previousRequested);
        connect(m_interface, &PlayInterface::volumeChanged, this, &PlayInterfaceController::volumeChanged);
        connect(m_interface, &PlayInterface::balanceChanged, this, &PlayInterfaceController::balanceChanged);
        connect(m_interface, &PlayInterface::positionChanged, this, &PlayInterfaceController::seekRequested);
        connect(m_interface, &PlayInterface::muteToggled, this, &PlayInterfaceController::muteToggled);
    }
}

void PlayInterfaceController::setupVisualization()
{
    // 设置可视化
    logInfo("Setting up visualization");
}

void PlayInterfaceController::setupAnimations()
{
    // 设置动画
    logInfo("Setting up animations");
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
    Q_UNUSED(data)
    // TODO: 实现音频数据处理
}

void PlayInterfaceController::onSpectrumDataAvailable(const QVector<float>& spectrum)
{
    Q_UNUSED(spectrum)
    // TODO: 实现频谱显示
}

void PlayInterfaceController::onLevelDataAvailable(float left, float right)
{
    Q_UNUSED(left)
    Q_UNUSED(right)
    // TODO: 实现电平显示
}

void PlayInterfaceController::onPlayPauseClicked()
{
    logInfo("播放/暂停按钮点击");
    emit playRequested();
}

void PlayInterfaceController::onStopClicked()
{
    logInfo("停止按钮点击");
    emit stopRequested();
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

void PlayInterfaceController::onEqualizerSliderChanged(int value)
{
    logInfo(QString("均衡器滑块改变: %1").arg(value));
    // 假设第一个频段
    emit equalizerChanged(EqualizerBand::Band32Hz, value);
}

void PlayInterfaceController::onLyricClicked(qint64 position)
{
    logInfo(QString("歌词点击: %1").arg(position));
    emit seekRequested(position);
}