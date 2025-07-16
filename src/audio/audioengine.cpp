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

// 静态成员初始化
AudioEngine* AudioEngine::m_instance = nullptr;
QMutex AudioEngine::m_instanceMutex;
QStringList AudioEngine::m_supportedFormats = {
    "mp3", "wav", "flac", "aac", "ogg", "wma", "m4a", "opus"
};

AudioEngine::AudioEngine(QObject* parent)
    : QObject(parent)
    , m_player(nullptr)
    , m_audioOutput(nullptr)
    , m_state(AudioTypes::AudioState::Stopped)
    , m_position(0)
    , m_duration(0)
    , m_volume(70)
    , m_muted(false)
    , m_currentIndex(-1)
    , m_playMode(AudioTypes::PlayMode::Loop)
    , m_equalizerEnabled(false)
    , m_balance(0.0)
    , m_speed(1.0)
    , m_maxHistorySize(100)
    , m_positionTimer(nullptr)
    , m_bufferTimer(nullptr)
{
    // 初始化均衡器频段（10个频段）
    m_equalizerBands.resize(10);
    m_equalizerBands.fill(0.0);
    
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
        // 创建媒体播放器
        m_player = new QMediaPlayer(this);
        m_audioOutput = new QAudioOutput(this);
        
        // 设置音频输出
        m_player->setAudioOutput(m_audioOutput);
        
        // 初始化音量
        m_audioOutput->setVolume(m_volume / 100.0);
        m_audioOutput->setMuted(m_muted);
        
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
    }
}

void AudioEngine::connectSignals()
{
    if (!m_player) return;
    
    // 媒体播放器信号
    connect(m_player, &QMediaPlayer::positionChanged, this, &AudioEngine::onPositionChanged);
    connect(m_player, &QMediaPlayer::durationChanged, this, &AudioEngine::onDurationChanged);
    connect(m_player, &QMediaPlayer::playbackStateChanged, this, &AudioEngine::onStateChanged);
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
    QMutexLocker locker(&m_mutex);
    
    if (!m_player) {
        logError("播放器未初始化");
        return;
    }
    
    if (m_currentIndex < 0 || m_currentIndex >= m_playlist.size()) {
        logError("无效的播放索引");
        return;
    }
    
    try {
        if (m_state == AudioTypes::AudioState::Paused) {
            m_player->play();
            logPlaybackEvent("恢复播放", currentSong().title());
        } else {
            const Song& song = m_playlist[m_currentIndex];
            loadMedia(song.filePath());
            m_player->play();
            logPlaybackEvent("开始播放", song.title());
            
            // 发出当前歌曲变化信号
            updateCurrentSong();
            
            // 添加到播放历史
            addToHistory(song);
        }
        
        m_positionTimer->start();
        
    } catch (const std::exception& e) {
        logError(QString("播放失败: %1").arg(e.what()));
        m_state = AudioTypes::AudioState::Error;
        emit errorOccurred("播放失败");
    }
}

void AudioEngine::pause()
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_player) return;
    
    try {
        m_player->pause();
        m_positionTimer->stop();
        logPlaybackEvent("暂停播放", currentSong().title());
    } catch (const std::exception& e) {
        logError(QString("暂停失败: %1").arg(e.what()));
    }
}

void AudioEngine::stop()
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_player) return;
    
    try {
        m_player->stop();
        m_positionTimer->stop();
        m_position = 0;
        m_duration = 0;
        
        logPlaybackEvent("停止播放", currentSong().title());
        
        emit positionChanged(0);
        emit durationChanged(0);
    } catch (const std::exception& e) {
        logError(QString("停止失败: %1").arg(e.what()));
    }
}

void AudioEngine::seek(qint64 position)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_player) return;
    
    try {
        m_player->setPosition(position);
        m_position = position;
        
        logPlaybackEvent("跳转位置", QString("位置: %1ms").arg(position));
        emit positionChanged(position);
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
        m_audioOutput->setVolume(volume / 100.0);
        m_volume = volume;
        
        logPlaybackEvent("音量调节", QString("音量: %1%").arg(volume));
        emit volumeChanged(volume);
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
    QMutexLocker locker(&m_mutex);
    
    // 验证歌曲文件
    QList<Song> validSongs;
    for (const Song& song : songs) {
        if (isFormatSupported(song.filePath())) {
            validSongs.append(song);
        } else {
            logError(QString("不支持的音频格式: %1").arg(song.filePath()));
        }
    }
    
    m_playlist = validSongs;
    m_currentIndex = validSongs.isEmpty() ? -1 : 0;
    
    logPlaybackEvent("设置播放列表", QString("歌曲数量: %1").arg(validSongs.size()));
    
    emit playlistChanged(m_playlist);
    if (m_currentIndex >= 0) {
        emit currentIndexChanged(m_currentIndex);
        emit currentSongChanged(m_playlist[m_currentIndex]);
    }
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
    QMutexLocker locker(&m_mutex);
    
    if (index < 0 || index >= m_playlist.size()) {
        logError("无效的播放索引");
        return;
    }
    
    bool wasPlaying = (m_state == AudioTypes::AudioState::Playing);
    
    if (wasPlaying) {
        stop();
    }
    
    m_currentIndex = index;
    
    logPlaybackEvent("切换歌曲", QString("索引: %1, 歌曲: %2").arg(index).arg(m_playlist[index].title()));
    
    emit currentIndexChanged(index);
    emit currentSongChanged(m_playlist[index]);
    
    if (wasPlaying) {
        play();
    }
}

void AudioEngine::playNext()
{
    QMutexLocker locker(&m_mutex);
    
    if (m_playlist.isEmpty()) return;
    
    int nextIndex = getNextIndex();
    setCurrentIndex(nextIndex);
}

void AudioEngine::playPrevious()
{
    QMutexLocker locker(&m_mutex);
    
    if (m_playlist.isEmpty()) return;
    
    int previousIndex = getPreviousIndex();
    setCurrentIndex(previousIndex);
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
}

AudioTypes::PlayMode AudioEngine::playMode() const
{
    QMutexLocker locker(&m_mutex);
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

AudioTypes::AudioState AudioEngine::state() const
{
    QMutexLocker locker(&m_mutex);
    return m_state;
}

qint64 AudioEngine::position() const
{
    QMutexLocker locker(&m_mutex);
    return m_position;
}

qint64 AudioEngine::duration() const
{
    QMutexLocker locker(&m_mutex);
    return m_duration;
}

Song AudioEngine::currentSong() const
{
    QMutexLocker locker(&m_mutex);
    if (m_currentIndex >= 0 && m_currentIndex < m_playlist.size()) {
        return m_playlist[m_currentIndex];
    }
    return Song();
}

int AudioEngine::currentIndex() const
{
    QMutexLocker locker(&m_mutex);
    return m_currentIndex;
}

QList<Song> AudioEngine::playlist() const
{
    QMutexLocker locker(&m_mutex);
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

QList<Song> AudioEngine::playHistory() const
{
    QMutexLocker locker(&m_mutex);
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
    } else if (status == QMediaPlayer::EndOfMedia) {
        logPlaybackEvent("播放完成", currentSong().title());
        handlePlaybackFinished();
    }
}

void AudioEngine::onErrorOccurred(QMediaPlayer::Error error)
{
    QString errorString;
    switch (error) {
        case QMediaPlayer::NoError:
            return;
        case QMediaPlayer::ResourceError:
            errorString = "资源错误";
            break;
        case QMediaPlayer::FormatError:
            errorString = "格式错误";
            break;
        case QMediaPlayer::NetworkError:
            errorString = "网络错误";
            break;
        case QMediaPlayer::AccessDeniedError:
            errorString = "访问被拒绝";
            break;
        default:
            errorString = "未知错误";
            break;
    }
    
    logError(errorString);
    m_state = AudioTypes::AudioState::Error;
    emit stateChanged(AudioTypes::AudioState::Error);
    emit errorOccurred(errorString);
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
    if (!QFileInfo::exists(filePath)) {
        throw std::runtime_error("文件不存在");
    }
    
    if (!checkAudioFormat(filePath)) {
        throw std::runtime_error("不支持的音频格式");
    }
    
    QUrl mediaUrl = QUrl::fromLocalFile(filePath);
    m_player->setSource(mediaUrl);
    
    logPlaybackEvent("加载媒体", filePath);
}

void AudioEngine::updateCurrentSong()
{
    if (m_currentIndex >= 0 && m_currentIndex < m_playlist.size()) {
        emit currentSongChanged(m_playlist[m_currentIndex]);
        emit currentIndexChanged(m_currentIndex);
    }
}

void AudioEngine::handlePlaybackFinished()
{
    if (m_playlist.isEmpty()) return;
    
    switch (m_playMode) {
        case AudioTypes::PlayMode::Loop:
            playNext();
            break;
        case AudioTypes::PlayMode::RepeatOne:
            play();
            break;
        case AudioTypes::PlayMode::Random:
            playNext();
            break;
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
    if (m_playlist.isEmpty()) return -1;
    
    switch (m_playMode) {
        case AudioTypes::PlayMode::Loop:
            return (m_currentIndex + 1) % m_playlist.size();
        case AudioTypes::PlayMode::RepeatOne:
            return m_currentIndex;
        case AudioTypes::PlayMode::Random:
            return QRandomGenerator::global()->bounded(m_playlist.size());
        default:
            return (m_currentIndex + 1) % m_playlist.size();
    }
    
    return -1;
}

int AudioEngine::getPreviousIndex()
{
    if (m_playlist.isEmpty()) return -1;
    
    switch (m_playMode) {
        case AudioTypes::PlayMode::Loop:
            return (m_currentIndex - 1 + m_playlist.size()) % m_playlist.size();
        case AudioTypes::PlayMode::RepeatOne:
            return m_currentIndex;
        case AudioTypes::PlayMode::Random:
            return QRandomGenerator::global()->bounded(m_playlist.size());
        default:
            return (m_currentIndex - 1 + m_playlist.size()) % m_playlist.size();
    }
    
    return -1;
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
    QString extension = getFileExtension(filePath);
    return m_supportedFormats.contains(extension, Qt::CaseInsensitive);
}

QString AudioEngine::getFileExtension(const QString& filePath) const
{
    return QFileInfo(filePath).suffix().toLower();
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
