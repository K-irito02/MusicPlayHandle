#include "audioengine.h"
#include "../core/logger.h"
#include <QFileInfo>
#include <QUrl>
#include <QStandardPaths>
#include <QDir>
#include <QRandomGenerator>
#include <QApplication>
#include <QMediaMetaData>
#include <QtMath>
#include <QRegularExpression>
#include <QFile> // Added for QFile::exists

// 静态成员初始化
AudioEngine* AudioEngine::m_instance = nullptr;
QMutex AudioEngine::m_instanceMutex;
QStringList AudioEngine::m_supportedFormats = {
    "mp3", "wav", "flac", "aac", "ogg", "wma", "m4a", "opus"
};

AudioEngine::AudioEngine(QObject* parent)
    : QObject(parent),
    m_player(nullptr),
    m_currentVolume(50.0),
    m_audioOutput(nullptr),
    m_audioWorker(nullptr),
    m_state(AudioTypes::AudioState::Stopped),
    m_position(0),
    m_duration(0),
    m_volume(70),
    m_muted(false),
    m_currentIndex(-1),
    m_playMode(AudioTypes::PlayMode::Loop),
    m_equalizerEnabled(false),
    m_balance(0.0),
    m_speed(1.0),
    m_maxHistorySize(100),
    m_positionTimer(nullptr),
    m_bufferTimer(nullptr)
{
    // 初始化均衡器频段（10个频段）
    m_equalizerBands.resize(10);
    m_equalizerBands.fill(0.0);
    
    m_audioWorker = new AudioWorkerThread(this);
    m_audioWorker->startThread();
    qDebug() << "[AudioEngine] Audio worker thread started";
    
    initializeAudio();
    
    LOG_INFO("AudioEngine", "音频引擎初始化完成");
}

AudioEngine::~AudioEngine()
{
    LOG_INFO("AudioEngine", "音频引擎开始清理");
    cleanupAudio();
}

AudioEngine* AudioEngine::instance()
{
    QMutexLocker locker(&m_instanceMutex);
    if (!m_instance) {
        m_instance = new AudioEngine();
        qDebug() << "[排查] AudioEngine::instance() 创建单例:" << m_instance;
    } else {
        qDebug() << "[排查] AudioEngine::instance() 返回已存在单例:" << m_instance;
    }
    return m_instance;
}

void AudioEngine::cleanup()
{
    QMutexLocker locker(&m_instanceMutex);
    if (m_instance) {
        delete m_instance;
        m_instance = nullptr;
    }
}

void AudioEngine::initializeAudio()
{
    try {
        // 先创建音频输出组件
        if(!m_audioOutput) {
            m_audioOutput = new QAudioOutput(this);
            qDebug() << "[音频引擎] 音频输出组件初始化完成";
        }
        
        // 创建媒体播放器
        if(!m_player) {
            m_player = new QMediaPlayer(this);
            qDebug() << "[音频引擎] 媒体播放器初始化完成";
            
            // 验证播放器是否创建成功
            if(!m_player) {
                throw std::runtime_error("QMediaPlayer创建失败");
            }
        }
        
        // 设置音频输出
        m_player->setAudioOutput(m_audioOutput);
        qDebug() << "[音频引擎] 音频输出已设置到媒体播放器";
        
        // 初始化音量
        m_audioOutput->setVolume(m_volume / 100.0);
        m_audioOutput->setMuted(m_muted);
        qDebug() << "[音频引擎] 音频输出参数已配置 音量:" << (m_volume / 100.0) << " 静音:" << m_muted;
        
        qDebug() << "[音频引擎] 媒体播放器信号将在connectSignals中统一连接";
        
        // 创建定时器
        m_positionTimer = new QTimer(this);
        m_positionTimer->setInterval(100); // 100ms更新一次
        m_bufferTimer = new QTimer(this);
        m_bufferTimer->setInterval(500); // 500ms更新一次
        
        // 连接信号
        connectSignals();
        
        LOG_INFO("AudioEngine", "音频系统初始化成功");
    } catch (const std::exception& e) {
        LOG_ERROR("AudioEngine", QString("音频系统初始化失败: %1").arg(e.what()));
        m_state = AudioTypes::AudioState::Error;
        emit errorOccurred("音频系统初始化失败");
    }
}

void AudioEngine::cleanupAudio()
{
    if (m_player) {
        stop();
        disconnectSignals();
        
        m_player->deleteLater();
        m_player = nullptr;
        
        if (m_audioOutput) {
            m_audioOutput->deleteLater();
            m_audioOutput = nullptr;
        }
        
        if (m_positionTimer) {
            m_positionTimer->stop();
            m_positionTimer->deleteLater();
            m_positionTimer = nullptr;
        }
        
        if (m_bufferTimer) {
            m_bufferTimer->stop();
            m_bufferTimer->deleteLater();
            m_bufferTimer = nullptr;
        }
        
        if (m_audioWorker) {
            m_audioWorker->stopThread();
            m_audioWorker->deleteLater();
            m_audioWorker = nullptr;
            qDebug() << "[AudioEngine] Audio worker thread stopped and cleaned up";
        }
    }
}

void AudioEngine::connectSignals()
{
    if (!m_player) return;
    
    // 媒体播放器信号
    connect(m_player, &QMediaPlayer::positionChanged, this, &AudioEngine::onPositionChanged);
    connect(m_player, &QMediaPlayer::durationChanged, this, &AudioEngine::onDurationChanged);
    connect(m_player, &QMediaPlayer::playbackStateChanged, this, &AudioEngine::handlePlaybackStateChanged);
    connect(m_player, &QMediaPlayer::mediaStatusChanged, this, &AudioEngine::onMediaStatusChanged);
    connect(m_player, &QMediaPlayer::errorOccurred, this, &AudioEngine::onErrorOccurred);
    connect(m_player, &QMediaPlayer::bufferProgressChanged, this, &AudioEngine::onBufferProgressChanged);
    
    // 定时器信号
    connect(m_positionTimer, &QTimer::timeout, this, &AudioEngine::updatePlaybackPosition);
    
    // 音频输出信号
    if (m_audioOutput) {
        connect(m_audioOutput, &QAudioOutput::volumeChanged, this, [this](float volume) {
            m_volume = static_cast<int>(volume * 100);
            emit volumeChanged(m_volume);
        });
        connect(m_audioOutput, &QAudioOutput::mutedChanged, this, [this](bool muted) {
            m_muted = muted;
            emit mutedChanged(muted);
        });
    }
}

void AudioEngine::disconnectSignals()
{
    if (m_player) {
        disconnect(m_player, nullptr, this, nullptr);
    }
    if (m_audioOutput) {
        disconnect(m_audioOutput, nullptr, this, nullptr);
    }
    if (m_positionTimer) {
        disconnect(m_positionTimer, nullptr, this, nullptr);
    }
    if (m_bufferTimer) {
        disconnect(m_bufferTimer, nullptr, this, nullptr);
    }
}

void AudioEngine::play()
{
    qDebug() << "[排查] AudioEngine::play() 被调用";
    qDebug() << "[排查] 当前播放列表大小:" << m_playlist.size();
    qDebug() << "[排查] 当前索引:" << m_currentIndex;
    
    // 预检查（不加锁）
    if (m_currentIndex < 0 || m_currentIndex >= m_playlist.size()) {
        qWarning() << "[排查] 当前索引越界，无法播放";
        return;
    }
    
    if (!m_player) {
        logError("播放器未初始化");
        qDebug() << "[AudioEngine::play] 错误：播放器未初始化";
        return;
    }
    
    const Song& song = m_playlist.at(m_currentIndex);
    qDebug() << "[排查] AudioEngine::play() Song.isValid():" << song.isValid();
    qDebug() << "[排查] AudioEngine::play() Song.filePath():" << song.filePath();
    qDebug() << "[排查] AudioEngine::play() 文件是否存在:" << QFile::exists(song.filePath());
    
    qDebug() << "[AudioEngine::play] 开始执行播放方法";
    
    try {
        QMutexLocker locker(&m_mutex);
        // 如果是暂停或停止状态，直接恢复播放
        if (m_state == AudioTypes::AudioState::Paused || m_state == AudioTypes::AudioState::Stopped) {
            qDebug() << "[AudioEngine::play] 恢复暂停/停止的播放";
            m_player->play();
            m_state = AudioTypes::AudioState::Playing;
            emit stateChanged(m_state);
            if (m_positionTimer) m_positionTimer->start();
            logPlaybackEvent("恢复播放", currentSong().title());
            return;
        }
    }
    catch (const std::exception& e) {
        QString errorMsg = QString("播放失败: %1").arg(e.what());
        logError(errorMsg);
        qDebug() << "[AudioEngine::play] 捕获到异常:" << e.what();
        QMutexLocker locker(&m_mutex);
        m_state = AudioTypes::AudioState::Error;
        emit stateChanged(AudioTypes::AudioState::Error);
        emit errorOccurred(errorMsg);
        return;
    }
    // 处理新歌曲播放（不持有锁，避免界面卡顿）
    const Song& song2 = m_playlist[m_currentIndex];
    qDebug() << "[AudioEngine::play] 开始播放新歌曲: " << song2.title() << " - " << song2.artist();
    qDebug() << "[AudioEngine::play] 文件路径: " << song2.filePath();
    if (m_player->audioOutput() != m_audioOutput) {
        qDebug() << "[AudioEngine::play] 重新设置音频输出";
        m_player->setAudioOutput(m_audioOutput);
    }
    {
        QMutexLocker locker(&m_mutex);
        m_state = AudioTypes::AudioState::Loading;
    }
    emit stateChanged(AudioTypes::AudioState::Loading);
    qDebug() << "[媒体加载] 开始加载媒体文件";
    loadMedia(song2.filePath());
    qDebug() << "[媒体加载] 媒体加载函数调用完成";
    QMediaPlayer::MediaStatus mediaStatus = m_player->mediaStatus();
    qDebug() << "[媒体加载] 当前媒体状态:" << mediaStatus;
    if (mediaStatus == QMediaPlayer::LoadedMedia) {
        qDebug() << "[媒体加载] 媒体已加载，开始播放";
        QMutexLocker locker(&m_mutex);
        m_state = AudioTypes::AudioState::Playing;
        emit stateChanged(AudioTypes::AudioState::Playing);
        m_player->play();
    } else {
        qDebug() << "[媒体加载] 媒体正在加载中，等待加载完成";
    }
    logPlaybackEvent("开始播放", song2.title());
    updateCurrentSong();
    addToHistory(song2);
    if (m_positionTimer) {
        m_positionTimer->start();
        qDebug() << "[AudioEngine::play] 位置更新定时器已启动";
    }
    qDebug() << "[AudioEngine::play] 播放方法执行完成，当前播放状态:" << m_player->playbackState();
}

void AudioEngine::pause()
{
    QMutexLocker locker(&m_mutex);
    
    qDebug() << "[AudioEngine::pause] 开始执行暂停方法";
    
    // 检查播放器是否初始化
    if (!m_player) {
        logError("播放器未初始化");
        qDebug() << "[AudioEngine::pause] 错误：播放器未初始化";
        return;
    }
    
    try {
        // 直接暂停播放
        m_player->pause();
        m_state = AudioTypes::AudioState::Paused;
        emit stateChanged(m_state);
        
        // 停止位置更新定时器
        if (m_positionTimer) {
            m_positionTimer->stop();
        }
        
        // 记录暂停事件
        logPlaybackEvent("暂停播放", currentSong().title());
        
        qDebug() << "[AudioEngine::pause] 已直接调用QMediaPlayer::pause()";
    } catch (const std::exception& e) {
        logError(QString("暂停失败: %1").arg(e.what()));
        qDebug() << "[AudioEngine::pause] 捕获到异常:" << e.what();
    }
}

void AudioEngine::stop()
{
    QMutexLocker locker(&m_mutex);
    
    qDebug() << "[AudioEngine::stop] 开始执行停止方法";
    
    // 检查播放器是否初始化
    if (!m_player) {
        logError("播放器未初始化");
        qDebug() << "[AudioEngine::stop] 错误：播放器未初始化";
        return;
    }
    
    try {
        // 停止播放
        m_audioWorker->stopAudio();
        m_state = AudioTypes::AudioState::Stopped;
        emit stateChanged(m_state);
        
        // 停止位置更新定时器
        if (m_positionTimer) {
            m_positionTimer->stop();
        }
        
        // 重置位置和时长
        m_position = 0;
        m_duration = 0;
        
        // 记录停止事件
        logPlaybackEvent("停止播放", currentSong().title());
        
        // 发送位置和时长变化信号
        emit positionChanged(0);
        emit durationChanged(0);
        
        qDebug() << "[AudioEngine::stop] 已委托工作线程停止播放";
    } catch (const std::exception& e) {
        logError(QString("停止失败: %1").arg(e.what()));
        qDebug() << "[AudioEngine::stop] 捕获到异常:" << e.what();
    }
}

void AudioEngine::seek(qint64 position)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_audioWorker) return;
    
    try {
        m_audioWorker->seekAudio(position);
        m_position = position;
        
        logPlaybackEvent("跳转位置", QString("位置: %1ms").arg(position));
        emit positionChanged(position);
        qDebug() << "[AudioEngine::seek] 已委托工作线程设置位置为:" << position;
    } catch (const std::exception& e) {
        logError(QString("跳转失败: %1").arg(e.what()));
    }
}

void AudioEngine::setPosition(qint64 position)
{
    seek(position);
}

void AudioEngine::setVolume(int volume)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_audioOutput) return;
    
    volume = qBound(0, volume, 100);
    
    try {
        m_audioWorker->setVolume(volume);
        m_volume = volume;
        
        logPlaybackEvent("音量调节", QString("音量: %1%").arg(volume));
        emit volumeChanged(volume);
        qDebug() << "[AudioEngine::setVolume] 已委托工作线程设置音量为:" << volume;
    } catch (const std::exception& e) {
        logError(QString("音量调节失败: %1").arg(e.what()));
    }
}

void AudioEngine::setMuted(bool muted)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_audioOutput) return;
    
    try {
        m_audioOutput->setMuted(muted);
        m_muted = muted;
        
        logPlaybackEvent("静音设置", muted ? "静音" : "取消静音");
        emit mutedChanged(muted);
    } catch (const std::exception& e) {
        logError(QString("静音设置失败: %1").arg(e.what()));
    }
}

int AudioEngine::volume() const
{
    QMutexLocker locker(&m_mutex);
    return m_volume;
}

bool AudioEngine::isMuted() const
{
    QMutexLocker locker(&m_mutex);
    return m_muted;
}

void AudioEngine::setPlaylist(const QList<Song>& songs)
{
    qDebug() << "[AudioEngine::setPlaylist] 开始设置播放列表，歌曲数量:" << songs.size();
    
    // 验证歌曲文件
    QList<Song> validSongs;
    for (const Song& song : songs) {
        if (isFormatSupported(song.filePath())) {
            validSongs.append(song);
        } else {
            logError(QString("不支持的音频格式: %1").arg(song.filePath()));
        }
    }
    
    qDebug() << "[AudioEngine::setPlaylist] 有效歌曲数量:" << validSongs.size();
    
    // 在临界区内更新数据
    {
        QMutexLocker locker(&m_mutex);
        m_playlist = validSongs;
        m_currentIndex = validSongs.isEmpty() ? -1 : 0;
        qDebug() << "[AudioEngine::setPlaylist] 设置当前索引:" << m_currentIndex;
    }
    
    logPlaybackEvent("设置播放列表", QString("歌曲数量: %1").arg(validSongs.size()));
    
    qDebug() << "[AudioEngine::setPlaylist] 发出播放列表变化信号";
    emit playlistChanged(m_playlist);
    
    if (m_currentIndex >= 0) {
        qDebug() << "[AudioEngine::setPlaylist] 发出当前索引变化信号:" << m_currentIndex;
        emit currentIndexChanged(m_currentIndex);
        qDebug() << "[AudioEngine::setPlaylist] 发出当前歌曲变化信号";
        emit currentSongChanged(m_playlist[m_currentIndex]);
    }
    
    qDebug() << "[AudioEngine::setPlaylist] 播放列表设置完成";
}

void AudioEngine::setCurrentSong(const Song& song)
{
    QMutexLocker locker(&m_mutex);
    
    int index = -1;
    for (int i = 0; i < m_playlist.size(); ++i) {
        if (m_playlist[i].id() == song.id()) {
            index = i;
            break;
        }
    }
    
    if (index >= 0) {
        setCurrentIndex(index);
    } else {
        logError("歌曲不在当前播放列表中");
    }
}

void AudioEngine::setCurrentIndex(int index)
{
    qDebug() << "[AudioEngine::setCurrentIndex] 设置当前播放索引:" << index;
    QMutexLocker locker(&m_mutex);
    
    // 检查索引是否有效
    if (index < 0 || index >= m_playlist.size()) {
        QString errorMsg = QString("无效的播放索引: %1, 播放列表大小: %2").arg(index).arg(m_playlist.size());
        qDebug() << "[AudioEngine::setCurrentIndex] " << errorMsg;
        logError(errorMsg);
        return;
    }
    
    // 如果索引没有变化，不执行任何操作
    if (m_currentIndex == index) {
        qDebug() << "[AudioEngine::setCurrentIndex] 索引未变化，保持当前索引:" << index;
        return;
    }
    
    try {
        // 记录当前播放状态
        bool wasPlaying = (m_state == AudioTypes::AudioState::Playing);
        qDebug() << "[AudioEngine::setCurrentIndex] 当前播放状态:" << (wasPlaying ? "播放中" : "已停止");
        
        // 如果正在播放，先停止
        if (wasPlaying) {
            qDebug() << "[AudioEngine::setCurrentIndex] 停止当前播放以切换歌曲";
            stop();
        }
        
        // 保存旧索引用于日志
        int oldIndex = m_currentIndex;
        
        // 更新当前索引
        m_currentIndex = index;
        
        // 获取当前歌曲信息
        const Song& currentSong = m_playlist[index];
        QString songInfo = QString("%1 - %2").arg(currentSong.title()).arg(currentSong.artist());
        
        // 记录切换事件
        QString eventDetails = QString("从索引 %1 切换到索引 %2, 歌曲: %3").arg(oldIndex).arg(index).arg(songInfo);
        qDebug() << "[AudioEngine::setCurrentIndex] " << eventDetails;
        logPlaybackEvent("切换歌曲", eventDetails);
        
        // 发送信号通知UI更新
        qDebug() << "[AudioEngine::setCurrentIndex] 发送currentIndexChanged和currentSongChanged信号";
        emit currentIndexChanged(index);
        emit currentSongChanged(currentSong);
        
        // 总是自动播放新歌曲
        qDebug() << "[AudioEngine::setCurrentIndex] 自动播放新选择的歌曲";
        play();
    } catch (const std::exception& e) {
        QString errorMsg = QString("设置当前索引时发生异常: %1").arg(e.what());
        qDebug() << "[AudioEngine::setCurrentIndex] " << errorMsg;
        logError(errorMsg);
    } catch (...) {
        QString errorMsg = "设置当前索引时发生未知异常";
        qDebug() << "[AudioEngine::setCurrentIndex] " << errorMsg;
        logError(errorMsg);
    }
}

void AudioEngine::playNext()
{
    qDebug() << "[AudioEngine::playNext] 播放下一首歌曲";
    QMutexLocker locker(&m_mutex);
    
    // 检查播放列表是否为空
    if (m_playlist.isEmpty()) {
        qDebug() << "[AudioEngine::playNext] 播放列表为空，无法播放下一首";
        return;
    }
    
    try {
        // 获取下一首歌曲的索引
        int currentIndex = m_currentIndex;
        int nextIndex = getNextIndex();
        
        qDebug() << "[AudioEngine::playNext] 当前索引:" << currentIndex 
                 << ", 下一首索引:" << nextIndex 
                 << ", 播放模式:" << static_cast<int>(m_playMode);
        
        // 设置当前索引并播放
        if (nextIndex >= 0 && nextIndex < m_playlist.size()) {
            setCurrentIndex(nextIndex);
            qDebug() << "[AudioEngine::playNext] 成功切换到下一首歌曲:" 
                     << (m_currentIndex < m_playlist.size() ? m_playlist[m_currentIndex].title() : "未知");
        } else {
            qDebug() << "[AudioEngine::playNext] 无效的下一首索引:" << nextIndex;
        }
    } catch (const std::exception& e) {
        qDebug() << "[AudioEngine::playNext] 播放下一首时发生异常:" << e.what();
        logError(QString("播放下一首时发生异常: %1").arg(e.what()));
    } catch (...) {
        qDebug() << "[AudioEngine::playNext] 播放下一首时发生未知异常";
        logError("播放下一首时发生未知异常");
    }
}

void AudioEngine::playPrevious()
{
    qDebug() << "[AudioEngine::playPrevious] 播放上一首歌曲";
    QMutexLocker locker(&m_mutex);
    
    // 检查播放列表是否为空
    if (m_playlist.isEmpty()) {
        qDebug() << "[AudioEngine::playPrevious] 播放列表为空，无法播放上一首";
        return;
    }
    
    try {
        // 获取上一首歌曲的索引
        int currentIndex = m_currentIndex;
        int previousIndex = getPreviousIndex();
        
        qDebug() << "[AudioEngine::playPrevious] 当前索引:" << currentIndex 
                 << ", 上一首索引:" << previousIndex 
                 << ", 播放模式:" << static_cast<int>(m_playMode);
        
        // 设置当前索引并播放
        if (previousIndex >= 0 && previousIndex < m_playlist.size()) {
            setCurrentIndex(previousIndex);
            qDebug() << "[AudioEngine::playPrevious] 成功切换到上一首歌曲:" 
                     << (m_currentIndex < m_playlist.size() ? m_playlist[m_currentIndex].title() : "未知");
        } else {
            qDebug() << "[AudioEngine::playPrevious] 无效的上一首索引:" << previousIndex;
        }
    } catch (const std::exception& e) {
        qDebug() << "[AudioEngine::playPrevious] 播放上一首时发生异常:" << e.what();
        logError(QString("播放上一首时发生异常: %1").arg(e.what()));
    } catch (...) {
        qDebug() << "[AudioEngine::playPrevious] 播放上一首时发生未知异常";
        logError("播放上一首时发生未知异常");
    }
}

void AudioEngine::setPlayMode(AudioTypes::PlayMode mode)
{
    QMutexLocker locker(&m_mutex);
    
    if (m_playMode == mode) return;
    
    m_playMode = mode;

    
    QString modeStr;
    switch (mode) {
        case AudioTypes::PlayMode::Loop: modeStr = "列表循环"; break;
        case AudioTypes::PlayMode::RepeatOne: modeStr = "单曲循环"; break;
        case AudioTypes::PlayMode::Random: modeStr = "随机播放"; break;
        default: modeStr = "列表循环"; break;
    }
    
    logPlaybackEvent("播放模式", modeStr);
    emit playModeChanged(mode);
    qDebug() << "[AudioEngine::setPlayMode] 已委托工作线程设置播放模式为:" << static_cast<int>(mode);
}

// playMode、state、position、duration、currentIndex、playlist、playHistory等getter不再加QMutexLocker，直接返回成员变量。
// 在注释中说明：这些getter假定只在主线程读，音频线程写时通过信号同步。
AudioTypes::PlayMode AudioEngine::playMode() const
{
    return m_playMode;
}

void AudioEngine::setEqualizerEnabled(bool enabled)
{
    QMutexLocker locker(&m_mutex);
    
    if (m_equalizerEnabled == enabled) return;
    
    m_equalizerEnabled = enabled;
    applyAudioEffects();
    
    logPlaybackEvent("均衡器", enabled ? "启用" : "禁用");
    emit equalizerChanged(enabled, m_equalizerBands);
}

void AudioEngine::setEqualizerBands(const QVector<double>& bands)
{
    QMutexLocker locker(&m_mutex);
    
    if (bands.size() != 10) {
        logError("均衡器频段数量必须为10");
        return;
    }
    
    m_equalizerBands = bands;
    if (m_equalizerEnabled) {
        applyAudioEffects();
    }
    
    logPlaybackEvent("均衡器频段", "更新");
    emit equalizerChanged(m_equalizerEnabled, m_equalizerBands);
}

void AudioEngine::setBalance(double balance)
{
    QMutexLocker locker(&m_mutex);
    
    balance = qBound(-1.0, balance, 1.0);
    
    if (qAbs(m_balance - balance) < 0.01) return;
    
    m_balance = balance;
    updateBalance();
    
    logPlaybackEvent("声道平衡", QString("平衡: %1").arg(balance));
    emit balanceChanged(balance);
}

void AudioEngine::setSpeed(double speed)
{
    QMutexLocker locker(&m_mutex);
    
    speed = qBound(0.25, speed, 4.0);
    
    if (qAbs(m_speed - speed) < 0.01) return;
    
    m_speed = speed;
    updateSpeed();
    
    logPlaybackEvent("播放速度", QString("速度: %1x").arg(speed));
    emit speedChanged(speed);
}

// playMode、state、position、duration、currentIndex、playlist、playHistory等getter不再加QMutexLocker，直接返回成员变量。
// 在注释中说明：这些getter假定只在主线程读，音频线程写时通过信号同步。
AudioTypes::AudioState AudioEngine::state() const
{
    return m_state;
}

// playMode、state、position、duration、currentIndex、playlist、playHistory等getter不再加QMutexLocker，直接返回成员变量。
// 在注释中说明：这些getter假定只在主线程读，音频线程写时通过信号同步。
qint64 AudioEngine::position() const
{
    return m_position;
}

// playMode、state、position、duration、currentIndex、playlist、playHistory等getter不再加QMutexLocker，直接返回成员变量。
// 在注释中说明：这些getter假定只在主线程读，音频线程写时通过信号同步。
qint64 AudioEngine::duration() const
{
    return m_duration;
}

// playMode、state、position、duration、currentIndex、playlist、playHistory等getter不再加QMutexLocker，直接返回成员变量。
// 在注释中说明：这些getter假定只在主线程读，音频线程写时通过信号同步。
Song AudioEngine::currentSong() const
{
    qDebug() << "[AudioEngine::currentSong] 开始获取当前歌曲";
    qDebug() << "[AudioEngine::currentSong] 当前索引:" << m_currentIndex << ", 播放列表大小:" << m_playlist.size();
    
    // 避免使用互斥锁，因为可能导致死锁
    // QMutexLocker locker(&m_mutex);
    
    if (m_currentIndex >= 0 && m_currentIndex < m_playlist.size()) {
        const Song& song = m_playlist[m_currentIndex];
        qDebug() << "[AudioEngine::currentSong] 返回歌曲:" << song.title() << "by" << song.artist();
        return song;
    }
    
    qDebug() << "[AudioEngine::currentSong] 返回空歌曲对象";
    return Song();
}

// playMode、state、position、duration、currentIndex、playlist、playHistory等getter不再加QMutexLocker，直接返回成员变量。
// 在注释中说明：这些getter假定只在主线程读，音频线程写时通过信号同步。
int AudioEngine::currentIndex() const
{
    return m_currentIndex;
}

// playMode、state、position、duration、currentIndex、playlist、playHistory等getter不再加QMutexLocker，直接返回成员变量。
// 在注释中说明：这些getter假定只在主线程读，音频线程写时通过信号同步。
QList<Song> AudioEngine::playlist() const
{
    return m_playlist;
}

bool AudioEngine::isFormatSupported(const QString& filePath) const
{
    return checkAudioFormat(filePath);
}

QStringList AudioEngine::supportedFormats()
{
    return m_supportedFormats;
}

void AudioEngine::addToHistory(const Song& song)
{
    QMutexLocker locker(&m_mutex);
    
    // 移除重复项
    m_playHistory.removeOne(song);
    
    // 添加到开头
    m_playHistory.prepend(song);
    
    // 限制历史记录数量
    while (m_playHistory.size() > m_maxHistorySize) {
        m_playHistory.removeLast();
    }
    
    logPlaybackEvent("添加到历史", song.title());
}

// playMode、state、position、duration、currentIndex、playlist、playHistory等getter不再加QMutexLocker，直接返回成员变量。
// 在注释中说明：这些getter假定只在主线程读，音频线程写时通过信号同步。
QList<Song> AudioEngine::playHistory() const
{
    return m_playHistory;
}

void AudioEngine::clearHistory()
{
    QMutexLocker locker(&m_mutex);
    m_playHistory.clear();
    logPlaybackEvent("清空历史", "");
}

// 私有槽函数实现
void AudioEngine::onPositionChanged(qint64 position)
{
    QMutexLocker locker(&m_mutex);
    m_position = position;
    emit positionChanged(position);
}

void AudioEngine::onDurationChanged(qint64 duration)
{
    QMutexLocker locker(&m_mutex);
    m_duration = duration;
    emit durationChanged(duration);
}

void AudioEngine::onStateChanged(QMediaPlayer::PlaybackState state)
{
    QMutexLocker locker(&m_mutex);
    
    AudioTypes::AudioState newState = convertMediaState(state);
    if (m_state != newState) {
        m_state = newState;
        emit stateChanged(newState);
        
        if (newState == AudioTypes::AudioState::Stopped) {
            handlePlaybackFinished();
        }
    }
}

void AudioEngine::onMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    emit mediaStatusChanged(status);
    
    if (status == QMediaPlayer::LoadedMedia) {
        logPlaybackEvent("媒体加载完成", currentSong().title());
        
        // 使用锁保护状态检查和修改
        {
            QMutexLocker locker(&m_mutex);
            // 如果当前状态是Loading，说明是在等待媒体加载完成后播放
            if (m_state == AudioTypes::AudioState::Loading) {
                qDebug() << "[媒体状态] 媒体加载完成，开始播放";
                m_state = AudioTypes::AudioState::Playing;
            } else {
                qDebug() << "[媒体状态] 媒体加载完成，但当前状态不是Loading:" << static_cast<int>(m_state);
                return; // 不是Loading状态，不需要自动播放
            }
        }
        
        emit stateChanged(AudioTypes::AudioState::Playing);
        m_player->play();
        
    } else if (status == QMediaPlayer::EndOfMedia) {
        logPlaybackEvent("播放完成", currentSong().title());
        handlePlaybackFinished();
    }
}

void AudioEngine::handlePlaybackStateChanged(QMediaPlayer::PlaybackState state)
{
    qDebug() << "[播放状态] QMediaPlayer状态变化:" << state;
    
    // 转换并更新AudioEngine状态
    AudioTypes::AudioState newState = convertMediaState(state);
    if (m_state != newState) {
        m_state = newState;
        qDebug() << "[播放状态] AudioEngine状态更新为:" << static_cast<int>(newState);
        emit stateChanged(newState);
    }
    
    // 发出原始播放状态信号
    emit playbackStateChanged(static_cast<int>(state));
    
    // 处理播放完成
    if (state == QMediaPlayer::StoppedState && m_player && m_player->mediaStatus() == QMediaPlayer::EndOfMedia) {
        qDebug() << "[播放状态] 检测到播放完成，调用handlePlaybackFinished";
        handlePlaybackFinished();
    }
}

void AudioEngine::onErrorOccurred(QMediaPlayer::Error error)
{
    qDebug() << "[AudioEngine::onErrorOccurred] 捕获到媒体播放器错误:" << static_cast<int>(error);
    
    // 如果没有错误，直接返回
    if (error == QMediaPlayer::NoError) {
        qDebug() << "[AudioEngine::onErrorOccurred] 无错误，返回";
        return;
    }
    
    // 获取当前播放的歌曲信息（如果有）
    QString songInfo = "未知歌曲";
    if (m_currentIndex >= 0 && m_currentIndex < m_playlist.size()) {
        const Song& song = m_playlist[m_currentIndex];
        songInfo = QString("%1 - %2 (%3)").arg(song.title()).arg(song.artist()).arg(song.filePath());
    }
    
    // 根据错误类型生成详细错误信息
    QString errorString;
    QString detailedError;
    
    switch (error) {
        case QMediaPlayer::ResourceError:
            errorString = "资源错误";
            detailedError = "无法访问或加载媒体资源，请检查文件路径是否正确、文件是否存在且可读";
            break;
        case QMediaPlayer::FormatError:
            errorString = "格式错误";
            detailedError = "不支持的媒体格式或文件损坏，请检查文件完整性";
            break;
        case QMediaPlayer::NetworkError:
            errorString = "网络错误";
            detailedError = "网络连接问题导致媒体无法加载";
            break;
        case QMediaPlayer::AccessDeniedError:
            errorString = "访问被拒绝";
            detailedError = "没有权限访问媒体文件，请检查文件权限";
            break;

        default:
            errorString = "未知错误";
            detailedError = QString("未分类错误，错误代码: %1").arg(static_cast<int>(error));
            break;
    }
    
    // 记录详细错误信息
    QString fullErrorMessage = QString("%1: %2 - 歌曲: %3").arg(errorString).arg(detailedError).arg(songInfo);
    qDebug() << "[AudioEngine::onErrorOccurred] 错误详情:" << fullErrorMessage;
    
    // 记录错误并更新状态
    logError(fullErrorMessage);
    m_state = AudioTypes::AudioState::Error;
    
    // 发送信号
    emit stateChanged(AudioTypes::AudioState::Error);
    emit errorOccurred(errorString);
    
    // 如果是播放中发生错误，尝试停止播放器
    if (m_player && m_player->playbackState() != QMediaPlayer::StoppedState) {
        qDebug() << "[AudioEngine::onErrorOccurred] 尝试停止播放器";
        try {
            m_player->stop();
        } catch (...) {
            qDebug() << "[AudioEngine::onErrorOccurred] 停止播放器时发生异常";
        }
    }
}

void AudioEngine::onBufferProgressChanged(int progress)
{
    emit bufferProgressChanged(progress);
    
    // 根据缓冲进度更新缓冲状态
    AudioTypes::BufferStatus status;
    if (progress == 0) {
        status = AudioTypes::BufferStatus::Empty;
    } else if (progress < 100) {
        status = AudioTypes::BufferStatus::Buffering;
    } else {
        status = AudioTypes::BufferStatus::Buffered;
    }
    
    emit bufferStatusChanged(status);
}

void AudioEngine::onPlaybackFinished()
{
    handlePlaybackFinished();
}

void AudioEngine::updatePlaybackPosition()
{
    if (m_player && m_state == AudioTypes::AudioState::Playing) {
        qint64 currentPos = m_player->position();
        if (currentPos != m_position) {
            m_position = currentPos;
            emit positionChanged(currentPos);
        }
    }
}

// 私有方法实现
void AudioEngine::loadMedia(const QString& filePath)
{
    qDebug() << "[AudioEngine::loadMedia] 开始加载媒体文件:" << filePath;
    
    // 检查文件是否存在
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        qDebug() << "[AudioEngine::loadMedia] 错误：文件不存在";
        throw std::runtime_error("文件不存在");
    }
    
    // 检查文件是否可读
    if (!fileInfo.isReadable()) {
        qDebug() << "[AudioEngine::loadMedia] 错误：文件无法读取，可能是权限问题";
        throw std::runtime_error("文件无法读取，可能是权限问题");
    }
    
    // 检查文件大小
    if (fileInfo.size() == 0) {
        qDebug() << "[AudioEngine::loadMedia] 错误：文件大小为0";
        throw std::runtime_error("文件大小为0");
    }
    
    // 检查音频格式支持
    QString extension = fileInfo.suffix().toLower();
    qDebug() << "[AudioEngine::loadMedia] 文件扩展名:" << extension;
    
    if (!checkAudioFormat(filePath)) {
        qDebug() << "[AudioEngine::loadMedia] 错误：不支持的音频格式:" << extension;
        throw std::runtime_error(QString("不支持的音频格式: %1").arg(extension).toStdString());
    }
    
    // 设置媒体源
    QUrl mediaUrl = QUrl::fromLocalFile(filePath);
    qDebug() << "[AudioEngine::loadMedia] 媒体URL:" << mediaUrl.toString();
    
    // 检查URL是否有效
    if (!mediaUrl.isValid()) {
        qDebug() << "[AudioEngine::loadMedia] 错误：无效的媒体URL";
        throw std::runtime_error("无效的媒体URL");
    }
    
    // 设置媒体源
    m_player->setSource(mediaUrl);
    qDebug() << "[AudioEngine::loadMedia] 媒体源设置完成";
    
    // 记录加载事件
    logPlaybackEvent("加载媒体", filePath);
    
    qDebug() << "[AudioEngine::loadMedia] 媒体加载完成，等待播放器状态更新";
}

void AudioEngine::updateCurrentSong()
{
    qDebug() << "[AudioEngine::updateCurrentSong] 开始更新当前歌曲信息";
    qDebug() << "[AudioEngine::updateCurrentSong] 当前索引:" << m_currentIndex << ", 播放列表大小:" << m_playlist.size();
    
    if (m_currentIndex >= 0 && m_currentIndex < m_playlist.size()) {
        const Song& currentSong = m_playlist[m_currentIndex];
        qDebug() << "[AudioEngine::updateCurrentSong] 发送currentSongChanged信号";
        qDebug() << "  歌曲ID:" << currentSong.id();
        qDebug() << "  歌曲标题:" << currentSong.title();
        qDebug() << "  歌曲艺术家:" << currentSong.artist();
        
        emit currentSongChanged(currentSong);
        qDebug() << "[AudioEngine::updateCurrentSong] currentSongChanged信号发送完成";
        
        qDebug() << "[AudioEngine::updateCurrentSong] 发送currentIndexChanged信号";
        emit currentIndexChanged(m_currentIndex);
        qDebug() << "[AudioEngine::updateCurrentSong] currentIndexChanged信号发送完成";
    } else {
        qDebug() << "[AudioEngine::updateCurrentSong] 警告：索引无效，无法更新当前歌曲";
    }
    
    qDebug() << "[AudioEngine::updateCurrentSong] 更新当前歌曲信息完成";
}

void AudioEngine::handlePlaybackFinished()
{
    qDebug() << "[AudioEngine::handlePlaybackFinished] 处理播放完成事件";
    
    // 检查播放列表是否为空
    if (m_playlist.isEmpty()) {
        qDebug() << "[AudioEngine::handlePlaybackFinished] 播放列表为空，不执行任何操作";
        return;
    }
    
    // 记录当前播放模式和索引
    qDebug() << "[AudioEngine::handlePlaybackFinished] 当前播放模式:" << static_cast<int>(m_playMode) 
             << ", 当前索引:" << m_currentIndex 
             << ", 播放列表大小:" << m_playlist.size();
    
    try {
        // 根据播放模式执行相应操作
        switch (m_playMode) {
            case AudioTypes::PlayMode::Loop:
                qDebug() << "[AudioEngine::handlePlaybackFinished] 循环模式，播放下一首";
                playNext();
                break;
            case AudioTypes::PlayMode::RepeatOne:
                qDebug() << "[AudioEngine::handlePlaybackFinished] 单曲循环模式，重新播放当前歌曲";
                play();
                break;
            case AudioTypes::PlayMode::Random:
                qDebug() << "[AudioEngine::handlePlaybackFinished] 随机模式，播放下一首";
                playNext();
                break;
            default:
                qDebug() << "[AudioEngine::handlePlaybackFinished] 未知播放模式:" << static_cast<int>(m_playMode) << "，默认播放下一首";
                playNext();
                break;
        }
    } catch (const std::exception& e) {
        qDebug() << "[AudioEngine::handlePlaybackFinished] 处理播放完成事件时发生异常:" << e.what();
        logError(QString("处理播放完成事件时发生异常: %1").arg(e.what()));
    } catch (...) {
        qDebug() << "[AudioEngine::handlePlaybackFinished] 处理播放完成事件时发生未知异常";
        logError("处理播放完成事件时发生未知异常");
    }
}

void AudioEngine::shufflePlaylist()
{
    if (m_playlist.size() <= 1) return;
    
    // 使用Fisher-Yates洗牌算法
    for (int i = m_playlist.size() - 1; i > 0; --i) {
        int j = QRandomGenerator::global()->bounded(i + 1);
        m_playlist.swapItemsAt(i, j);
    }
    
    // 重置当前索引
    m_currentIndex = 0;
    
    logPlaybackEvent("随机播放列表", "");
    emit playlistChanged(m_playlist);
    emit currentIndexChanged(m_currentIndex);
}

int AudioEngine::getNextIndex()
{
    qDebug() << "[AudioEngine::getNextIndex] 计算下一首歌曲索引";
    
    // 检查播放列表是否为空
    if (m_playlist.isEmpty()) {
        qDebug() << "[AudioEngine::getNextIndex] 播放列表为空，返回-1";
        return -1;
    }
    
    // 检查当前索引是否有效
    if (m_currentIndex < 0 || m_currentIndex >= m_playlist.size()) {
        qDebug() << "[AudioEngine::getNextIndex] 当前索引无效:" << m_currentIndex 
                 << ", 播放列表大小:" << m_playlist.size() << ", 返回0";
        return 0; // 如果当前索引无效，返回第一首歌曲
    }
    
    int nextIndex = -1;
    
    // 根据播放模式计算下一首歌曲索引
    switch (m_playMode) {
        case AudioTypes::PlayMode::Loop:
            nextIndex = (m_currentIndex + 1) % m_playlist.size();
            qDebug() << "[AudioEngine::getNextIndex] 循环模式，下一首索引:" << nextIndex;
            break;
            
        case AudioTypes::PlayMode::RepeatOne:
            nextIndex = m_currentIndex;
            qDebug() << "[AudioEngine::getNextIndex] 单曲循环模式，保持当前索引:" << nextIndex;
            break;
            
        case AudioTypes::PlayMode::Random:
            // 确保随机模式不会连续播放同一首歌
            if (m_playlist.size() > 1) {
                do {
                    nextIndex = QRandomGenerator::global()->bounded(m_playlist.size());
                } while (nextIndex == m_currentIndex);
            } else {
                nextIndex = 0; // 只有一首歌时返回0
            }
            qDebug() << "[AudioEngine::getNextIndex] 随机模式，随机选择索引:" << nextIndex;
            break;
            
        default:
            nextIndex = (m_currentIndex + 1) % m_playlist.size();
            qDebug() << "[AudioEngine::getNextIndex] 默认模式，下一首索引:" << nextIndex;
            break;
    }
    
    return nextIndex;
}

int AudioEngine::getPreviousIndex()
{
    qDebug() << "[AudioEngine::getPreviousIndex] 计算上一首歌曲索引";
    
    // 检查播放列表是否为空
    if (m_playlist.isEmpty()) {
        qDebug() << "[AudioEngine::getPreviousIndex] 播放列表为空，返回-1";
        return -1;
    }
    
    // 检查当前索引是否有效
    if (m_currentIndex < 0 || m_currentIndex >= m_playlist.size()) {
        qDebug() << "[AudioEngine::getPreviousIndex] 当前索引无效:" << m_currentIndex 
                 << ", 播放列表大小:" << m_playlist.size() << ", 返回0";
        return 0; // 如果当前索引无效，返回第一首歌曲
    }
    
    int previousIndex = -1;
    
    // 根据播放模式计算上一首歌曲索引
    switch (m_playMode) {
        case AudioTypes::PlayMode::Loop:
            previousIndex = (m_currentIndex - 1 + m_playlist.size()) % m_playlist.size();
            qDebug() << "[AudioEngine::getPreviousIndex] 循环模式，上一首索引:" << previousIndex;
            break;
            
        case AudioTypes::PlayMode::RepeatOne:
            previousIndex = m_currentIndex;
            qDebug() << "[AudioEngine::getPreviousIndex] 单曲循环模式，保持当前索引:" << previousIndex;
            break;
            
        case AudioTypes::PlayMode::Random:
            // 确保随机模式不会连续播放同一首歌
            if (m_playlist.size() > 1) {
                do {
                    previousIndex = QRandomGenerator::global()->bounded(m_playlist.size());
                } while (previousIndex == m_currentIndex);
            } else {
                previousIndex = 0; // 只有一首歌时返回0
            }
            qDebug() << "[AudioEngine::getPreviousIndex] 随机模式，随机选择索引:" << previousIndex;
            break;
            
        default:
            previousIndex = (m_currentIndex - 1 + m_playlist.size()) % m_playlist.size();
            qDebug() << "[AudioEngine::getPreviousIndex] 默认模式，上一首索引:" << previousIndex;
            break;
    }
    
    return previousIndex;
}

void AudioEngine::applyAudioEffects()
{
    // 这里可以实现音频效果处理
    // 由于Qt6的QMediaPlayer不直接支持实时音频效果，
    // 实际实现可能需要使用更底层的音频处理库
    logPlaybackEvent("应用音效", "");
}

void AudioEngine::updateBalance()
{
    // 实现声道平衡
    // 这也需要底层音频处理支持
    logPlaybackEvent("更新声道平衡", QString("平衡: %1").arg(m_balance));
}

void AudioEngine::updateSpeed()
{
    if (m_player) {
        m_player->setPlaybackRate(m_speed);
        logPlaybackEvent("更新播放速度", QString("速度: %1x").arg(m_speed));
    }
}

void AudioEngine::logPlaybackEvent(const QString& event, const QString& details)
{
    QString message = details.isEmpty() ? event : QString("%1: %2").arg(event, details);
    qDebug() << "[AudioEngine]" << message;
}

void AudioEngine::logError(const QString& error)
{
    qWarning() << "[AudioEngine] Error:" << error;
}

bool AudioEngine::checkAudioFormat(const QString& filePath) const
{
    qDebug() << "[AudioEngine::checkAudioFormat] 检查文件格式:" << filePath;
    
    // 获取文件扩展名
    QString extension = getFileExtension(filePath);
    
    // 检查扩展名是否为空
    if (extension.isEmpty()) {
        qDebug() << "[AudioEngine::checkAudioFormat] 错误：文件没有扩展名";
        return false;
    }
    
    // 检查扩展名是否在支持列表中
    bool isSupported = m_supportedFormats.contains(extension, Qt::CaseInsensitive);
    
    qDebug() << "[AudioEngine::checkAudioFormat] 文件扩展名:" << extension 
             << ", 是否支持:" << (isSupported ? "是" : "否");
    
    return isSupported;
}

QString AudioEngine::getFileExtension(const QString& filePath) const
{
    QString extension = QFileInfo(filePath).suffix().toLower();
    qDebug() << "[AudioEngine::getFileExtension] 文件:" << filePath 
             << ", 扩展名:" << extension;
    return extension;
}

AudioTypes::AudioState AudioEngine::convertMediaState(QMediaPlayer::PlaybackState state) const
{
    switch (state) {
        case QMediaPlayer::StoppedState:
            return AudioTypes::AudioState::Stopped;
        case QMediaPlayer::PlayingState:
            return AudioTypes::AudioState::Playing;
        case QMediaPlayer::PausedState:
            return AudioTypes::AudioState::Paused;
        default:
            return AudioTypes::AudioState::Stopped;
    }
}

// AudioUtils 实现
QString AudioUtils::formatTime(qint64 milliseconds)
{
    int seconds = static_cast<int>(milliseconds / 1000);
    int minutes = seconds / 60;
    int hours = minutes / 60;
    
    seconds %= 60;
    minutes %= 60;
    
    if (hours > 0) {
        return QString("%1:%2:%3")
            .arg(hours, 2, 10, QChar('0'))
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'));
    } else {
        return QString("%1:%2")
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'));
    }
}

QString AudioUtils::formatDuration(qint64 duration)
{
    return formatTime(duration);
}

int AudioUtils::linearToDb(int linear)
{
    if (linear <= 0) return -100;
    return static_cast<int>(20 * log10(linear / 100.0));
}

int AudioUtils::dbToLinear(int db)
{
    if (db <= -100) return 0;
    return static_cast<int>(100 * pow(10, db / 20.0));
}

bool AudioUtils::isAudioFile(const QString& filePath)
{
    QString extension = QFileInfo(filePath).suffix().toLower();
    return AudioEngine::supportedFormats().contains(extension);
}

QString AudioUtils::getAudioFormat(const QString& filePath)
{
    return QFileInfo(filePath).suffix().toLower();
}

qint64 AudioUtils::getAudioDuration(const QString& filePath)
{
    // 这里可以实现音频文件时长获取
    // 实际实现可能需要使用专门的音频库
    Q_UNUSED(filePath)
    return 0;
}

QVector<double> AudioUtils::calculateSpectrum(const QByteArray& audioData)
{
    // 实现频谱分析
    // 这需要FFT算法实现
    Q_UNUSED(audioData)
    return QVector<double>();
}

double AudioUtils::calculateRMS(const QByteArray& audioData)
{
    // 实现RMS计算
    Q_UNUSED(audioData)
    return 0.0;
}

double AudioUtils::calculatePeak(const QByteArray& audioData)
{
    // 实现峰值计算
    Q_UNUSED(audioData)
    return 0.0;
}
