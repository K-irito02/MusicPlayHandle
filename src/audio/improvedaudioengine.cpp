#include "improvedaudioengine.h"
#include "../core/logger.h"
#include "../database/playhistorydao.h"
#include "../models/song.h"
#include <QDebug>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QRandomGenerator>
#include <QStandardPaths>

// 静态成员初始化
QStringList ImprovedAudioEngine::m_supportedFormats = {
    "mp3", "wav", "flac", "ogg", "m4a", "aac", "wma"
};

ImprovedAudioEngine::ImprovedAudioEngine(const AudioEngineConfig& config, QObject* parent)
    : QObject(parent)
    , m_config(config)
    , m_isInitialized(false)
    , m_player(nullptr)
    , m_audioOutput(nullptr)
    , m_audioWorker(nullptr)
    , m_ffmpegDecoder(nullptr)
    , m_state(AudioTypes::AudioState::Stopped)
    , m_position(0)
    , m_duration(0)
    , m_volume(50)
    , m_muted(false)
    , m_userPaused(false)
    , m_currentIndex(-1)
    , m_playMode(AudioTypes::PlayMode::Loop)
    , m_equalizerEnabled(false)
    , m_balance(0.0)
    , m_speed(1.0)
    , m_vuEnabled(false)
    , m_vuLevels(2, 0.0)
    , m_vuTimer(new QTimer(this))
    , m_playHistoryDao(nullptr)
    , m_positionTimer(new QTimer(this))
    , m_observerSyncTimer(new QTimer(this))
    , m_resourceManager(ResourceManager::instance())
    , m_errorCount(0)
{
    try {
        if (initialize()) {
            m_isInitialized = true;
            qDebug() << "ImprovedAudioEngine: 初始化成功";
        } else {
            qCritical() << "ImprovedAudioEngine: 初始化失败";
        }
    } catch (const std::exception& e) {
        qCritical() << "ImprovedAudioEngine: 初始化异常:" << e.what();
    }
}

ImprovedAudioEngine::~ImprovedAudioEngine()
{
    cleanup();
    qDebug() << "ImprovedAudioEngine: 已销毁";
}

bool ImprovedAudioEngine::initialize()
{
    // 初始化性能管理器
    if (m_config.enablePerformanceMonitoring) {
        m_performanceManager = std::make_unique<PerformanceManager>(this);
        if (m_config.enableAdaptiveDecoding) {
            m_adaptiveController = std::make_unique<AdaptiveDecodeController>(m_performanceManager.get(), this);
        }
    }
    
    // 初始化音频组件
    if (!initializeAudioComponents()) {
        return false;
    }
    
    // 设置连接
    if (!setupConnections()) {
        return false;
    }
    
    // 获取资源锁
    if (m_config.enableResourceLocking) {
        if (!acquireAudioLock()) {
            qWarning() << "ImprovedAudioEngine: 无法获取音频资源锁";
        }
    }
    
    return true;
}

void ImprovedAudioEngine::cleanup()
{
    // 停止播放
    stop();
    
    // 释放资源锁
    releaseAudioLock();
    
    // 断开连接
    disconnectConnections();
    
    // 清理音频组件
    cleanupAudioComponents();
    
    m_isInitialized = false;
}

bool ImprovedAudioEngine::initializeAudioComponents()
{
    // 创建QMediaPlayer
    m_player = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_player->setAudioOutput(m_audioOutput);
    
    // 创建FFmpeg解码器
    if (!initializeFFmpegDecoder()) {
        qWarning() << "ImprovedAudioEngine: FFmpeg解码器初始化失败";
    }
    
    // 创建音频工作线程
    m_audioWorker = new AudioWorkerThread(this);
    
    // 设置初始音量
    setVolume(m_volume);
    
    return true;
}

void ImprovedAudioEngine::cleanupAudioComponents()
{
    if (m_audioWorker) {
        m_audioWorker->stopThread();
        m_audioWorker->deleteLater();
        m_audioWorker = nullptr;
    }
    
    cleanupFFmpegDecoder();
    
    if (m_player) {
        m_player->deleteLater();
        m_player = nullptr;
    }
    
    if (m_audioOutput) {
        m_audioOutput->deleteLater();
        m_audioOutput = nullptr;
    }
}

bool ImprovedAudioEngine::initializeFFmpegDecoder()
{
    try {
        m_ffmpegDecoder = new FFmpegDecoder(this);
        return true;
    } catch (const std::exception& e) {
        qWarning() << "ImprovedAudioEngine: FFmpeg解码器创建失败:" << e.what();
        return false;
    }
}

void ImprovedAudioEngine::cleanupFFmpegDecoder()
{
    if (m_ffmpegDecoder) {
        m_ffmpegDecoder->deleteLater();
        m_ffmpegDecoder = nullptr;
    }
}

bool ImprovedAudioEngine::setupConnections()
{
    // QMediaPlayer连接
    connect(m_player, &QMediaPlayer::playbackStateChanged,
            this, &ImprovedAudioEngine::onMediaPlayerStateChanged);
    connect(m_player, &QMediaPlayer::positionChanged,
            this, &ImprovedAudioEngine::onMediaPlayerPositionChanged);
    connect(m_player, &QMediaPlayer::durationChanged,
            this, &ImprovedAudioEngine::onMediaPlayerDurationChanged);
    connect(m_player, &QMediaPlayer::mediaStatusChanged,
            this, &ImprovedAudioEngine::onMediaPlayerMediaStatusChanged);
    connect(m_player, &QMediaPlayer::errorOccurred,
            this, &ImprovedAudioEngine::onMediaPlayerErrorOccurred);
    
    // FFmpeg解码器连接
    if (m_ffmpegDecoder) {
        connect(m_ffmpegDecoder, &FFmpegDecoder::audioDataReady,
                this, &ImprovedAudioEngine::onFFmpegAudioDataReady);
        connect(m_ffmpegDecoder, &FFmpegDecoder::positionChanged,
                this, &ImprovedAudioEngine::onFFmpegPositionChanged);
        connect(m_ffmpegDecoder, &FFmpegDecoder::durationChanged,
                this, &ImprovedAudioEngine::onFFmpegDurationChanged);
        connect(m_ffmpegDecoder, &FFmpegDecoder::decodingFinished,
                this, &ImprovedAudioEngine::onFFmpegDecodingFinished);
        connect(m_ffmpegDecoder, &FFmpegDecoder::errorOccurred,
                this, &ImprovedAudioEngine::onFFmpegErrorOccurred);
    }
    
    // 定时器连接
    connect(m_positionTimer, &QTimer::timeout, this, &ImprovedAudioEngine::updatePosition);
    connect(m_vuTimer, &QTimer::timeout, this, &ImprovedAudioEngine::updateVULevels);
    connect(m_observerSyncTimer, &QTimer::timeout, this, &ImprovedAudioEngine::syncObservers);
    
    // 性能管理器连接
    if (m_performanceManager) {
        connect(m_performanceManager.get(), &PerformanceManager::performanceUpdated,
                this, &ImprovedAudioEngine::onPerformanceUpdated);
    }
    
    return true;
}

void ImprovedAudioEngine::disconnectConnections()
{
    if (m_player) {
        disconnect(m_player, nullptr, this, nullptr);
    }
    
    if (m_ffmpegDecoder) {
        disconnect(m_ffmpegDecoder, nullptr, this, nullptr);
    }
    
    disconnect(m_positionTimer, nullptr, this, nullptr);
    disconnect(m_vuTimer, nullptr, this, nullptr);
    disconnect(m_observerSyncTimer, nullptr, this, nullptr);
}

// 基本播放控制实现
bool ImprovedAudioEngine::play()
{
    if (!m_isInitialized || m_playlist.isEmpty() || m_currentIndex < 0) {
        return false;
    }
    
    if (m_state == AudioTypes::AudioState::Paused) {
        m_player->play();
        m_userPaused = false;
        return true;
    }
    
    const Song& currentSong = m_playlist[m_currentIndex];
    
    if (m_config.engineType == AudioTypes::AudioEngineType::QMediaPlayer) {
        return playWithQMediaPlayer(currentSong);
    } else {
        return playWithFFmpeg(currentSong);
    }
}

bool ImprovedAudioEngine::pause()
{
    if (!m_isInitialized) {
        return false;
    }
    
    m_player->pause();
    m_userPaused = true;
    return true;
}

bool ImprovedAudioEngine::stop()
{
    if (!m_isInitialized) {
        return false;
    }
    
    m_player->stop();
    m_userPaused = false;
    m_position = 0;
    return true;
}

bool ImprovedAudioEngine::seek(qint64 position)
{
    if (!m_isInitialized) {
        return false;
    }
    
    m_player->setPosition(position);
    return true;
}

// 音量控制实现
bool ImprovedAudioEngine::setVolume(int volume)
{
    if (!m_isInitialized) {
        return false;
    }
    
    m_volume = qBound(0, volume, 100);
    if (m_audioOutput) {
        m_audioOutput->setVolume(m_volume / 100.0);
    }
    
    publishVolumeChanged();
    return true;
}

bool ImprovedAudioEngine::setMuted(bool muted)
{
    if (!m_isInitialized) {
        return false;
    }
    
    m_muted = muted;
    if (m_audioOutput) {
        m_audioOutput->setMuted(muted);
    }
    
    publishVolumeChanged();
    return true;
}

bool ImprovedAudioEngine::toggleMute()
{
    return setMuted(!m_muted);
}

int ImprovedAudioEngine::volume() const
{
    return m_volume;
}

bool ImprovedAudioEngine::isMuted() const
{
    return m_muted;
}

// 播放列表管理实现
bool ImprovedAudioEngine::setPlaylist(const QList<Song>& songs)
{
    QMutexLocker locker(&m_playlistMutex);
    m_playlist = songs;
    m_currentIndex = songs.isEmpty() ? -1 : 0;
    publishPlaylistChanged();
    return true;
}

bool ImprovedAudioEngine::setCurrentSong(const Song& song)
{
    QMutexLocker locker(&m_playlistMutex);
    
    // 在播放列表中查找歌曲
    for (int i = 0; i < m_playlist.size(); ++i) {
        if (m_playlist[i].filePath() == song.filePath()) {
            m_currentIndex = i;
            publishSongChanged();
            return true;
        }
    }
    
    // 如果没找到，添加到播放列表
    m_playlist.append(song);
    m_currentIndex = m_playlist.size() - 1;
    publishPlaylistChanged();
    publishSongChanged();
    return true;
}

bool ImprovedAudioEngine::setCurrentIndex(int index)
{
    QMutexLocker locker(&m_playlistMutex);
    
    if (index < 0 || index >= m_playlist.size()) {
        return false;
    }
    
    m_currentIndex = index;
    publishSongChanged();
    return true;
}

bool ImprovedAudioEngine::playNext()
{
    QMutexLocker locker(&m_playlistMutex);
    
    if (m_playlist.isEmpty()) {
        return false;
    }
    
    int nextIndex = getNextIndex();
    if (nextIndex != m_currentIndex) {
        m_currentIndex = nextIndex;
        locker.unlock();
        return play();
    }
    
    return false;
}

bool ImprovedAudioEngine::playPrevious()
{
    QMutexLocker locker(&m_playlistMutex);
    
    if (m_playlist.isEmpty()) {
        return false;
    }
    
    int prevIndex = getPreviousIndex();
    if (prevIndex != m_currentIndex) {
        m_currentIndex = prevIndex;
        locker.unlock();
        return play();
    }
    
    return false;
}

// 播放模式实现
bool ImprovedAudioEngine::setPlayMode(AudioTypes::PlayMode mode)
{
    m_playMode = mode;
    emit playModeChanged(mode);
    return true;
}

AudioTypes::PlayMode ImprovedAudioEngine::playMode() const
{
    return m_playMode;
}

// 状态获取实现
AudioTypes::AudioState ImprovedAudioEngine::state() const
{
    return m_state;
}

qint64 ImprovedAudioEngine::position() const
{
    return m_position;
}

qint64 ImprovedAudioEngine::duration() const
{
    return m_duration;
}

Song ImprovedAudioEngine::currentSong() const
{
    QMutexLocker locker(&m_playlistMutex);
    if (m_currentIndex >= 0 && m_currentIndex < m_playlist.size()) {
        return m_playlist[m_currentIndex];
    }
    return Song();
}

int ImprovedAudioEngine::currentIndex() const
{
    return m_currentIndex;
}

QList<Song> ImprovedAudioEngine::playlist() const
{
    QMutexLocker locker(&m_playlistMutex);
    return m_playlist;
}

// 有效性检查实现
bool ImprovedAudioEngine::isValid() const
{
    return m_isInitialized && m_player != nullptr;
}

bool ImprovedAudioEngine::isInitialized() const
{
    return m_isInitialized;
}

// 音频格式支持实现
bool ImprovedAudioEngine::isFormatSupported(const QString& filePath) const
{
    QString extension = getFileExtension(filePath).toLower();
    return m_supportedFormats.contains(extension);
}

QStringList ImprovedAudioEngine::supportedFormats()
{
    return m_supportedFormats;
}

// 私有辅助方法实现
bool ImprovedAudioEngine::playWithQMediaPlayer(const Song& song)
{
    if (!loadMedia(song.filePath())) {
        return false;
    }
    
    m_player->play();
    addToHistory(song);
    return true;
}

bool ImprovedAudioEngine::playWithFFmpeg(const Song& song)
{
    if (!m_ffmpegDecoder) {
        return false;
    }
    
    // 使用FFmpeg解码器播放
    // 这里需要实现FFmpeg播放逻辑
    addToHistory(song);
    return true;
}

bool ImprovedAudioEngine::loadMedia(const QString& filePath)
{
    if (!QFileInfo::exists(filePath)) {
        handleError("文件不存在: " + filePath);
        return false;
    }
    
    if (!isFormatSupported(filePath)) {
        handleError("不支持的音频格式: " + filePath);
        return false;
    }
    
    m_player->setSource(QUrl::fromLocalFile(filePath));
    return true;
}

int ImprovedAudioEngine::getNextIndex()
{
    if (m_playlist.isEmpty()) {
        return -1;
    }
    
    switch (m_playMode) {
        case AudioTypes::PlayMode::Loop:
            return (m_currentIndex + 1 < m_playlist.size()) ? m_currentIndex + 1 : 0;
        case AudioTypes::PlayMode::Random:
            return QRandomGenerator::global()->bounded(m_playlist.size());
        case AudioTypes::PlayMode::RepeatOne:
            return m_currentIndex;
        default:
            return m_currentIndex;
    }
}

int ImprovedAudioEngine::getPreviousIndex()
{
    if (m_playlist.isEmpty()) {
        return -1;
    }
    
    switch (m_playMode) {
        case AudioTypes::PlayMode::Loop:
            return (m_currentIndex - 1 + m_playlist.size()) % m_playlist.size();
        case AudioTypes::PlayMode::Random:
            return QRandomGenerator::global()->bounded(m_playlist.size());
        case AudioTypes::PlayMode::RepeatOne:
            return m_currentIndex;
        default:
            return m_currentIndex;
    }
}

QString ImprovedAudioEngine::getFileExtension(const QString& filePath) const
{
    QFileInfo fileInfo(filePath);
    return fileInfo.suffix();
}

// 观察者模式事件发布实现
void ImprovedAudioEngine::publishStateChanged()
{
    AudioEvents::StateChanged event;
    event.state = static_cast<AudioEvents::StateChanged::State>(m_state);
    event.position = m_position;
    event.duration = m_duration;
    
    AudioStateSubject::notifyObservers(event);
    emit stateChanged(m_state);
}

void ImprovedAudioEngine::publishVolumeChanged()
{
    AudioEvents::VolumeChanged event;
    event.volume = m_volume;
    event.muted = m_muted;
    event.balance = m_balance;
    
    AudioVolumeSubject::notifyObservers(event);
    emit volumeChanged(m_volume);
    emit mutedChanged(m_muted);
}

void ImprovedAudioEngine::publishSongChanged()
{
    if (m_currentIndex >= 0 && m_currentIndex < m_playlist.size()) {
        const Song& song = m_playlist[m_currentIndex];
        
        AudioEvents::SongChanged event;
        event.title = song.title();
        event.artist = song.artist();
        event.album = song.album();
        event.filePath = song.filePath();
        event.duration = song.duration();
        event.index = m_currentIndex;
        
        AudioSongSubject::notifyObservers(event);
        emit currentSongChanged(song);
        emit currentIndexChanged(m_currentIndex);
    }
}

void ImprovedAudioEngine::publishPlaylistChanged()
{
    AudioEvents::PlaylistChanged event;
    for (const Song& song : m_playlist) {
        event.songs.append(song.filePath());
    }
    event.currentIndex = m_currentIndex;
    event.playMode = static_cast<AudioEvents::PlaylistChanged::PlayMode>(m_playMode);
    
    AudioPlaylistSubject::notifyObservers(event);
    emit playlistChanged(m_playlist);
}

// 错误处理实现
void ImprovedAudioEngine::handleError(const QString& error)
{
    m_errorCount.fetchAndAddOrdered(1);
    logError(error);
    emit errorOccurred(error);
    
    if (m_errorCount.loadAcquire() > MAX_ERROR_COUNT) {
        qCritical() << "ImprovedAudioEngine: 错误次数过多，停止播放";
        stop();
    }
}

void ImprovedAudioEngine::logError(const QString& error)
{
    qWarning() << "ImprovedAudioEngine Error:" << error;
}

void ImprovedAudioEngine::logInfo(const QString& message)
{
    qDebug() << "ImprovedAudioEngine:" << message;
}

// 资源管理实现
bool ImprovedAudioEngine::acquireAudioLock()
{
    try {
        m_resourceLock = m_resourceManager.createScopedLock(
            m_config.lockId, m_config.ownerName);
        return m_resourceLock != nullptr;
    } catch (const std::exception& e) {
        qWarning() << "ImprovedAudioEngine: 获取资源锁失败:" << e.what();
        return false;
    }
}

void ImprovedAudioEngine::releaseAudioLock()
{
    m_resourceLock.reset();
}

bool ImprovedAudioEngine::isResourceLocked() const
{
    return m_resourceLock != nullptr;
}

QString ImprovedAudioEngine::getResourceLockOwner() const
{
    return m_config.ownerName;
}

// 播放历史实现
bool ImprovedAudioEngine::addToHistory(const Song& song)
{
    m_playHistory.prepend(song);
    
    // 限制历史记录大小
    while (m_playHistory.size() > m_config.maxHistorySize) {
        m_playHistory.removeLast();
    }
    
    return true;
}

QList<Song> ImprovedAudioEngine::playHistory() const
{
    return m_playHistory;
}

bool ImprovedAudioEngine::clearHistory()
{
    m_playHistory.clear();
    return true;
}

// 性能监控实现
PerformanceManager* ImprovedAudioEngine::getPerformanceManager() const
{
    return m_performanceManager.get();
}

ResourceManager& ImprovedAudioEngine::getResourceManager() const
{
    return m_resourceManager;
}

// 配置管理实现
ImprovedAudioEngine::AudioEngineConfig ImprovedAudioEngine::getConfig() const
{
    return m_config;
}

bool ImprovedAudioEngine::updateConfig(const AudioEngineConfig& newConfig)
{
    m_config = newConfig;
    // 这里可以添加配置更新后的处理逻辑
    return true;
}

// 槽函数实现
void ImprovedAudioEngine::onMediaPlayerStateChanged(QMediaPlayer::PlaybackState state)
{
    m_state = convertMediaState(state);
    publishStateChanged();
}

void ImprovedAudioEngine::onMediaPlayerPositionChanged(qint64 position)
{
    m_position = position;
    emit positionChanged(position);
}

void ImprovedAudioEngine::onMediaPlayerDurationChanged(qint64 duration)
{
    m_duration = duration;
    emit durationChanged(duration);
}

void ImprovedAudioEngine::onMediaPlayerMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    emit mediaStatusChanged(status);
}

void ImprovedAudioEngine::onMediaPlayerErrorOccurred(QMediaPlayer::Error error)
{
    QString errorString = m_player->errorString();
    handleError("QMediaPlayer错误: " + errorString);
}

void ImprovedAudioEngine::onFFmpegAudioDataReady(const QVector<double>& levels)
{
    if (m_vuEnabled) {
        m_vuLevels = levels;
        emit vuLevelsChanged(levels);
    }
}

void ImprovedAudioEngine::onFFmpegPositionChanged(qint64 position)
{
    m_position = position;
    emit positionChanged(position);
}

void ImprovedAudioEngine::onFFmpegDurationChanged(qint64 duration)
{
    m_duration = duration;
    emit durationChanged(duration);
}

void ImprovedAudioEngine::onFFmpegDecodingFinished()
{
    handlePlaybackFinished();
}

void ImprovedAudioEngine::onFFmpegErrorOccurred(const QString& error)
{
    handleError("FFmpeg错误: " + error);
}

void ImprovedAudioEngine::onPerformanceUpdated(double cpuUsage, qint64 memoryUsage, double responseTime)
{
    Q_UNUSED(memoryUsage)
    // 性能监控处理
    if (cpuUsage > m_config.targetCpuUsage) {
        emit performanceWarning(QString("CPU使用率过高: %1%").arg(cpuUsage));
    }
    
    if (responseTime > m_config.maxResponseTime) {
        emit performanceWarning(QString("响应时间过长: %1ms").arg(responseTime));
    }
}

void ImprovedAudioEngine::onDecodeIntervalChanged(int newInterval, int oldInterval)
{
    logInfo(QString("解码间隔调整: %1ms -> %2ms").arg(oldInterval).arg(newInterval));
}

void ImprovedAudioEngine::onResourceLockConflict(const QString& lockId, const QString& requester, const QString& currentOwner)
{
    emit resourceLockFailed(QString("资源锁冲突: %1 (请求者: %2, 当前持有者: %3)")
                           .arg(lockId).arg(requester).arg(currentOwner));
}

void ImprovedAudioEngine::updatePosition()
{
    if (m_player && m_state == AudioTypes::AudioState::Playing) {
        m_position = m_player->position();
        emit positionChanged(m_position);
    }
}

void ImprovedAudioEngine::updateVULevels()
{
    if (m_vuEnabled) {
        // 更新VU表数据
        emit vuLevelsChanged(m_vuLevels);
    }
}

void ImprovedAudioEngine::syncObservers()
{
    // 同步观察者状态
    publishStateChanged();
    publishVolumeChanged();
    publishSongChanged();
}

void ImprovedAudioEngine::handlePlaybackFinished()
{
    if (m_playMode == AudioTypes::PlayMode::RepeatOne) {
        stop();
    } else {
        playNext();
    }
}

AudioTypes::AudioState ImprovedAudioEngine::convertMediaState(QMediaPlayer::PlaybackState state) const
{
    switch (state) {
        case QMediaPlayer::PlayingState:
            return AudioTypes::AudioState::Playing;
        case QMediaPlayer::PausedState:
            return AudioTypes::AudioState::Paused;
        case QMediaPlayer::StoppedState:
            return AudioTypes::AudioState::Stopped;
        default:
            return AudioTypes::AudioState::Stopped;
    }
}

// 其他未实现的方法的空实现
bool ImprovedAudioEngine::setEqualizerEnabled(bool enabled) { m_equalizerEnabled = enabled; return true; }
bool ImprovedAudioEngine::setEqualizerBands(const QVector<double>& bands) { m_equalizerBands = bands; return true; }
bool ImprovedAudioEngine::setBalance(double balance) { m_balance = balance; return true; }
double ImprovedAudioEngine::getBalance() const { return m_balance; }
bool ImprovedAudioEngine::setSpeed(double speed) { m_speed = speed; return true; }
bool ImprovedAudioEngine::setAudioEngineType(AudioTypes::AudioEngineType type) { m_config.engineType = type; return true; }
AudioTypes::AudioEngineType ImprovedAudioEngine::getAudioEngineType() const { return m_config.engineType; }
QString ImprovedAudioEngine::getAudioEngineTypeString() const { return "QMediaPlayer"; }
bool ImprovedAudioEngine::setVUEnabled(bool enabled) { m_vuEnabled = enabled; return true; }
bool ImprovedAudioEngine::isVUEnabled() const { return m_vuEnabled; }
QVector<double> ImprovedAudioEngine::getVULevels() const { return m_vuLevels; }

// AudioEngineFactory 实现
std::unique_ptr<ImprovedAudioEngine> AudioEngineFactory::createEngine(
    const ImprovedAudioEngine::AudioEngineConfig& config)
{
    return std::make_unique<ImprovedAudioEngine>(config);
}

std::unique_ptr<ImprovedAudioEngine> AudioEngineFactory::createDefaultEngine(
    const QString& ownerName)
{
    auto config = getDefaultConfig();
    config.ownerName = ownerName;
    return createEngine(config);
}

std::unique_ptr<ImprovedAudioEngine> AudioEngineFactory::createPerformanceOptimizedEngine(
    const QString& ownerName)
{
    auto config = getPerformanceConfig();
    config.ownerName = ownerName;
    return createEngine(config);
}

std::unique_ptr<ImprovedAudioEngine> AudioEngineFactory::createPowerSaverEngine(
    const QString& ownerName)
{
    auto config = getPowerSaverConfig();
    config.ownerName = ownerName;
    return createEngine(config);
}

ImprovedAudioEngine::AudioEngineConfig AudioEngineFactory::getDefaultConfig()
{
    ImprovedAudioEngine::AudioEngineConfig config;
    config.engineType = AudioTypes::AudioEngineType::QMediaPlayer;
    config.enablePerformanceMonitoring = true;
    config.enableResourceLocking = true;
    config.enableAdaptiveDecoding = true;
    return config;
}

ImprovedAudioEngine::AudioEngineConfig AudioEngineFactory::getPerformanceConfig()
{
    auto config = getDefaultConfig();
    config.targetCpuUsage = 20.0;
    config.maxResponseTime = 10.0;
    config.performanceProfile = PerformanceManager::PerformanceProfile::Performance;
    return config;
}

ImprovedAudioEngine::AudioEngineConfig AudioEngineFactory::getPowerSaverConfig()
{
    auto config = getDefaultConfig();
    config.targetCpuUsage = 50.0;
    config.maxResponseTime = 30.0;
    config.performanceProfile = PerformanceManager::PerformanceProfile::PowerSaver;
    config.enableAdaptiveDecoding = false;
    return config;
}