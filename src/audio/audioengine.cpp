#include "audioengine.h"
#include "../core/logger.h"
#include "../core/appconfig.h"
#include "../database/playhistorydao.h"
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
#include <QMediaDevices> // Added for QMediaDevices
#include <QAudioBuffer>
#include <QAudioFormat>
#include <QtAlgorithms>

// 静态成员初始化
AudioEngine* AudioEngine::m_instance = nullptr;
QMutex AudioEngine::m_instanceMutex;
QStringList AudioEngine::m_supportedFormats = {
    "mp3", "wav", "flac", "aac", "ogg", "wma", "m4a", "opus", "mp4", "ape", "aiff"
};

AudioEngine::AudioEngine(QObject* parent)
    : QObject(parent),
    m_player(nullptr),
    m_currentVolume(50.0),
    m_audioOutput(nullptr),
    m_audioWorker(nullptr),
    m_state(AudioTypes::AudioState::Paused), // 初始状态设为暂停而不是停止
    m_position(0),
    m_duration(0),
    m_volume(50), // 修改默认音量为50，与主界面保持一致
    m_muted(false),
    m_userPaused(false), // 初始化用户暂停标志
    m_currentIndex(-1),
    m_playMode(AudioTypes::PlayMode::Loop),
    m_equalizerEnabled(false),
    m_balance(0.0),
    m_speed(1.0),
    m_audioEngineType(AudioTypes::AudioEngineType::QMediaPlayer), // 默认使用QMediaPlayer
    m_maxHistorySize(100),
    m_playHistoryDao(new PlayHistoryDao(this)), // 初始化播放历史DAO
    m_positionTimer(nullptr),
    m_bufferTimer(nullptr),
    m_vuEnabled(true),
    m_vuLevels(2, 0.0), // 左右声道
    m_vuTimer(nullptr),
    m_ffmpegDecoder(nullptr),
    m_realTimeLevels(2, 0.0)
{
    // 初始化均衡器频段（10个频段）
    m_equalizerBands.resize(10);
    m_equalizerBands.fill(0.0);
    
    // 从配置文件加载设置
    AppConfig* config = AppConfig::instance();
    m_volume = config->getValue("audio/volume", 50).toInt();
    m_muted = config->getValue("audio/muted", false).toBool();
    m_balance = config->getValue("audio/balance", 0.0).toDouble();
    m_vuEnabled = config->getValue("audio/vu_enabled", true).toBool();
    
    // 加载音频引擎类型设置
    int engineTypeInt = config->getValue("audio/engine_type", 0).toInt();
    m_audioEngineType = static_cast<AudioTypes::AudioEngineType>(engineTypeInt);
    
    m_audioWorker = new AudioWorkerThread(this);
    m_audioWorker->startThread();
    
    initializeAudio();
    initializeFFmpegDecoder();
    
    // 初始化VU表定时器
    m_vuTimer = new QTimer(this);
    connect(m_vuTimer, &QTimer::timeout, this, &AudioEngine::updateVULevels);
    m_vuTimer->start(50); // 20fps更新频率
    
    // 确保音量设置正确
    if (m_audioOutput) {
        m_audioOutput->setVolume(m_volume / 100.0); // 使用加载的音量
        m_audioOutput->setMuted(m_muted); // 使用加载的静音状态
        
        // 验证音频输出设备
        QAudioDevice device = m_audioOutput->device();
        if (device.description().isEmpty()) {
            // 尝试设置默认音频设备
            QMediaDevices mediaDevices;
            QList<QAudioDevice> audioOutputs = mediaDevices.audioOutputs();
            if (!audioOutputs.isEmpty()) {
                QAudioDevice defaultDevice = audioOutputs.first();
                m_audioOutput->setDevice(defaultDevice);
            }
        }
    }
    
    LOG_INFO("AudioEngine", "音频引擎初始化完成");
}

AudioEngine::~AudioEngine()
{
    LOG_INFO("AudioEngine", "音频引擎开始清理");
    cleanupAudio();
    cleanupFFmpegDecoder();
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
        // 先创建音频输出组件
        if(!m_audioOutput) {
            m_audioOutput = new QAudioOutput(this);
        }
        
        // 创建媒体播放器
        if(!m_player) {
            m_player = new QMediaPlayer(this);
            
            // 验证播放器是否创建成功
            if(!m_player) {
                throw std::runtime_error("QMediaPlayer创建失败");
            }
        }
        
        // 设置音频输出
        m_player->setAudioOutput(m_audioOutput);
        
        // 初始化音量 - 确保音量不为0
        if (m_volume <= 0) {
            m_volume = 50; // 设置默认音量为50%
        }
        
        // 设置音频输出参数
        if (m_audioOutput) {
            m_audioOutput->setVolume(m_volume / 100.0);
            m_audioOutput->setMuted(m_muted);
            
            // 确保使用最佳音频设备
            QMediaDevices mediaDevices;
            QList<QAudioDevice> audioOutputs = mediaDevices.audioOutputs();
            if (!audioOutputs.isEmpty()) {
                // 选择默认音频输出设备（通常是最佳设备）
                QAudioDevice defaultDevice = mediaDevices.defaultAudioOutput();
                if (defaultDevice.isNull() || defaultDevice.description().isEmpty()) {
                    defaultDevice = audioOutputs.first();
                }
                
                m_audioOutput->setDevice(defaultDevice);
            } else {
                qWarning() << "AudioEngine: 未找到可用的音频输出设备";
            }
        }
        
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
            
            // 保存音量设置到配置
            AppConfig* config = AppConfig::instance();
            config->setValue("audio/volume", m_volume);
        });
        connect(m_audioOutput, &QAudioOutput::mutedChanged, this, [this](bool muted) {
            m_muted = muted;
            emit mutedChanged(muted);
            
            // 保存静音状态到配置
            AppConfig* config = AppConfig::instance();
            config->setValue("audio/muted", m_muted);
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
    try {
        QMutexLocker locker(&m_mutex);
        

        
        if (m_playlist.isEmpty()) {
            qWarning() << "AudioEngine: 播放列表为空，无法播放";
            logError("播放列表为空");
            return;
        }
        
        if (m_currentIndex < 0 || m_currentIndex >= m_playlist.size()) {
            qWarning() << "AudioEngine: 播放索引无效:" << m_currentIndex << "播放列表大小:" << m_playlist.size();
            logError("播放索引无效");
            return;
        }
        
        const Song& song = m_playlist.at(m_currentIndex);
        
        if (!song.isValid()) {
            qWarning() << "AudioEngine: 当前歌曲无效";
            logError("当前歌曲无效");
            return;
        }
        
        if (!checkAudioFormat(song.filePath())) {
            qWarning() << "AudioEngine: 不支持的音频格式:" << song.filePath();
            logError(QString("不支持的音频格式: %1").arg(song.filePath()));
            return;
        }
        

        
        // 根据音频引擎类型选择播放方式
        if (m_audioEngineType == AudioTypes::AudioEngineType::FFmpeg) {
            // 使用FFmpeg解码器播放，支持实时音效处理
            playWithFFmpeg(song);
        } else {
            // 使用QMediaPlayer播放，不受音效处理影响
            playWithQMediaPlayer(song);
        }
        
    } catch (const std::exception& e) {
        qCritical() << "AudioEngine: 播放时发生异常:" << e.what();
        logError(QString("播放失败: %1").arg(e.what()));
        m_state = AudioTypes::AudioState::Error;
        emit stateChanged(m_state);
    } catch (...) {
        qCritical() << "AudioEngine: 播放时发生未知异常";
        logError("播放失败: 未知异常");
        m_state = AudioTypes::AudioState::Error;
        emit stateChanged(m_state);
    }
}

void AudioEngine::pause()
{
    try {
        QMutexLocker locker(&m_mutex);
        
        // 优先处理 FFmpeg 解码器暂停
        if (m_ffmpegDecoder && m_ffmpegDecoder->isDecoding()) {
            try {
                m_ffmpegDecoder->stopDecoding();
                
                // FFmpeg解码器暂停成功，设置状态
                m_state = AudioTypes::AudioState::Paused;
                m_userPaused = true;
                emit stateChanged(m_state);
                
                if (m_positionTimer) {
                    m_positionTimer->stop();
                }
                
                logPlaybackEvent("FFmpeg暂停播放", currentSong().title());
                return; // 成功暂停FFmpeg解码器，直接返回
            } catch (const std::exception& e) {
                qCritical() << "AudioEngine: 暂停FFmpeg解码器异常:" << e.what() << "，回退到QMediaPlayer";
            } catch (...) {
                qCritical() << "AudioEngine: 暂停FFmpeg解码器未知异常，回退到QMediaPlayer";
            }
        }
        
        // 回退到QMediaPlayer进行暂停
        if (!m_player) {
            logError("播放器未初始化");
            return;
        }
        
        QMediaPlayer::PlaybackState playerState = m_player->playbackState();
        
        // 改进的暂停逻辑：只要不是已暂停状态，都可以尝试暂停
        if (playerState != QMediaPlayer::PausedState) {
            m_player->pause();
            
            m_state = AudioTypes::AudioState::Paused;
            m_userPaused = true;
            
            emit stateChanged(m_state);
            
            if (m_positionTimer) {
                m_positionTimer->stop();
            }
            
            logPlaybackEvent("QMediaPlayer暂停播放", currentSong().title());
        }
        
    } catch (const std::exception& e) {
        logError(QString("暂停失败: %1").arg(e.what()));
    }
}

void AudioEngine::stop()
{
    QMutexLocker locker(&m_mutex);
    
    try {
        // 优先处理 FFmpeg 解码器停止
        if (m_ffmpegDecoder && m_ffmpegDecoder->isDecoding()) {
            try {
                m_ffmpegDecoder->stopDecoding();
                m_ffmpegDecoder->closeFile();
                
                // FFmpeg解码器停止成功，设置状态
                m_state = AudioTypes::AudioState::Stopped;
                m_userPaused = false;
                emit stateChanged(m_state);
                
                if (m_positionTimer) {
                    m_positionTimer->stop();
                }
                
                logPlaybackEvent("FFmpeg停止播放", currentSong().title());
                
                // 同时通知工作线程停止（如果存在）
                if (m_audioWorker) {
                    m_audioWorker->stopAudio();
                }
                
                return; // 成功停止FFmpeg解码器，直接返回
            } catch (const std::exception& e) {
                qCritical() << "AudioEngine: 停止FFmpeg解码器异常:" << e.what() << "，回退到QMediaPlayer";
            } catch (...) {
                qCritical() << "AudioEngine: 停止FFmpeg解码器未知异常，回退到QMediaPlayer";
            }
        }
        
        // 回退到QMediaPlayer进行停止
        if (!m_player) {
            logError("播放器未初始化");
    
            return;
        }
        
        // 直接停止QMediaPlayer，确保立即释放文件锁
        if (m_player) {
            m_player->stop();
        }
        
        // 同时通知工作线程停止（如果存在）
        if (m_audioWorker) {
            m_audioWorker->stopAudio();
        }
        
        m_state = AudioTypes::AudioState::Paused; // 改为暂停状态而不是停止状态
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
    } catch (const std::exception& e) {
        logError(QString("停止失败: %1").arg(e.what()));
    }
}

void AudioEngine::seek(qint64 position)
{
    try {
        if (position < 0) {
            logError("跳转位置无效");
            return;
        }
        
        // 根据当前音频引擎类型选择跳转方式
        if (m_audioEngineType == AudioTypes::AudioEngineType::FFmpeg) {
            // FFmpeg模式：使用FFmpeg解码器跳转
            if (m_ffmpegDecoder) {
                try {
                    // 检查位置是否有效
                    if (position < 0) {
                        qWarning() << "[AudioEngine::seek] 无效的跳转位置:" << position;
                        return;
                    }
                    
                    // 直接执行跳转，不检查解码状态
                    // FFmpeg解码器的seekTo方法应该能处理各种状态下的跳转
                    m_ffmpegDecoder->seekTo(position);
                    
                    // 不立即更新位置信息，让FFmpeg解码器的positionChanged信号处理
                    // 这样可以避免位置更新冲突和重复播放的问题
                    
                    logPlaybackEvent("FFmpeg跳转位置", QString("位置: %1ms").arg(position));
                    return;
                } catch (const std::exception& e) {
                    qCritical() << "[AudioEngine::seek] FFmpeg跳转异常:" << e.what();
                    logError(QString("FFmpeg跳转异常: %1").arg(e.what()));
                } catch (...) {
                    qCritical() << "[AudioEngine::seek] FFmpeg跳转未知异常";
                    logError("FFmpeg跳转未知异常");
                }
            } else {
                qWarning() << "[AudioEngine::seek] FFmpeg解码器未初始化";
                logError("FFmpeg解码器未初始化");
                return;
            }
        } else {
            // QMediaPlayer模式：使用QMediaPlayer跳转
            if (!m_player) {
                logError("QMediaPlayer为空，无法执行跳转");
                return;
            }
            
            // 检查播放器状态
            QMediaPlayer::PlaybackState playerState = m_player->playbackState();
            
            // 如果播放器处于停止状态，需要先加载媒体
            if (playerState == QMediaPlayer::StoppedState) {
                qWarning() << "[AudioEngine::seek] 播放器处于停止状态，无法跳转";
                return;
            }
            
            // 直接使用主线程的QMediaPlayer执行跳转
            m_player->setPosition(position);
            
            // 不立即更新m_position和发送positionChanged信号
            // 让QMediaPlayer::positionChanged信号来处理位置更新
            // 这样可以避免异步操作导致的位置不一致问题
            
            logPlaybackEvent("QMediaPlayer跳转位置", QString("位置: %1ms").arg(position));
        }
        
    } catch (const std::exception& e) {
        logError(QString("跳转失败: %1").arg(e.what()));
        qCritical() << "[AudioEngine::seek] 跳转异常:" << e.what();
    }
}

void AudioEngine::setPosition(qint64 position)
{
    seek(position);
}

void AudioEngine::setVolume(int volume)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_audioOutput) {
        logError("音频输出未初始化，无法设置音量");
        return;
    }
    
    volume = qBound(0, volume, 100);
    
    try {
        // 直接设置音频输出音量
        float volumeFloat = volume / 100.0;
        m_audioOutput->setVolume(volumeFloat);
        m_volume = volume;
        
        // 保存音量设置到配置
        AppConfig* config = AppConfig::instance();
        config->setValue("audio/volume", m_volume);
        
        logPlaybackEvent("音量调节", QString("音量: %1%").arg(volume));
        emit volumeChanged(volume);
        
        // 同时通过工作线程设置（如果需要）
        if (m_audioWorker) {
            m_audioWorker->setVolume(volume);
        }
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
        
        // 保存静音状态到配置
        AppConfig* config = AppConfig::instance();
        config->setValue("audio/muted", m_muted);
        
        logPlaybackEvent("静音设置", muted ? "静音" : "取消静音");
        emit mutedChanged(muted);
    } catch (const std::exception& e) {
        logError(QString("静音设置失败: %1").arg(e.what()));
    }
}

void AudioEngine::toggleMute()
{
    if (!m_audioOutput) {
        logError("音频输出未初始化，无法切换静音状态");
        return;
    }
    
    QMutexLocker locker(&m_mutex);
    bool newMutedState = !m_muted;
    m_muted = newMutedState;
    m_audioOutput->setMuted(newMutedState);
    
    // 保存静音状态到配置
    AppConfig* config = AppConfig::instance();
    config->setValue("audio/muted", m_muted);
    
    logPlaybackEvent("静音切换", newMutedState ? "静音" : "取消静音");
    emit mutedChanged(newMutedState);
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
    // 验证歌曲文件
    QList<Song> validSongs;
    for (const Song& song : songs) {
        if (isFormatSupported(song.filePath())) {
            validSongs.append(song);
        } else {
            QString extension = QFileInfo(song.filePath()).suffix().toLower();
            logError(QString("不支持的音频格式: %1 (扩展名: %2)").arg(song.filePath()).arg(extension));
        }
    }
    
    // 在临界区内更新数据
    {
        QMutexLocker locker(&m_mutex);
        m_playlist = validSongs;
        // 不自动设置当前索引，避免触发不必要的currentSongChanged信号
        m_currentIndex = -1;  // 重置为-1，等待后续手动设置
    }
    
    logPlaybackEvent("设置播放列表", QString("歌曲数量: %1").arg(validSongs.size()));
    emit playlistChanged(m_playlist);
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
    
    // 检查索引是否有效
    if (index < 0 || index >= m_playlist.size()) {
        QString errorMsg = QString("无效的播放索引: %1, 播放列表大小: %2").arg(index).arg(m_playlist.size());
        logError(errorMsg);
        return;
    }
    
    // 如果索引没有变化，不执行任何操作
    if (m_currentIndex == index) {
        return;
    }
    
    try {
        // 记录当前播放状态
        bool wasPlaying = (m_state == AudioTypes::AudioState::Playing);
        
        // 如果正在播放，先停止当前播放
        if (wasPlaying) {
            if (m_player) {
                m_player->stop();
            }
            m_state = AudioTypes::AudioState::Paused;
            emit stateChanged(m_state);
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
        logPlaybackEvent("切换歌曲", eventDetails);
        
        // 发送信号通知UI更新
        emit currentIndexChanged(index);
        emit currentSongChanged(currentSong);
    } catch (const std::exception& e) {
        QString errorMsg = QString("设置当前索引时发生异常: %1").arg(e.what());
        logError(errorMsg);
    } catch (...) {
        QString errorMsg = "设置当前索引时发生未知异常";
        logError(errorMsg);
    }
}

void AudioEngine::playNext()
{
    QMutexLocker locker(&m_mutex);
    
    // 检查播放列表是否为空
    if (m_playlist.isEmpty()) {
        logError("播放列表为空，无法播放下一首");
        return;
    }
    
    try {
        // 获取下一首歌曲的索引
        int nextIndex = getNextIndex();
        
        // 设置当前索引并播放
        if (nextIndex >= 0 && nextIndex < m_playlist.size()) {
            setCurrentIndex(nextIndex);
            
            // 确保状态为Loading，然后调用play()
            m_state = AudioTypes::AudioState::Loading;
            emit stateChanged(AudioTypes::AudioState::Loading);
            
            // 延迟一点调用play，确保setCurrentIndex的信号处理完成
            QTimer::singleShot(50, this, [this]() {
                play();
            });
        } else {
            logError(QString("无效的下一首索引: %1").arg(nextIndex));
        }
    } catch (const std::exception& e) {
        logError(QString("播放下一首时发生异常: %1").arg(e.what()));
    } catch (...) {
        logError("播放下一首时发生未知异常");
    }
}

void AudioEngine::playPrevious()
{
    QMutexLocker locker(&m_mutex);
    
    // 检查播放列表是否为空
    if (m_playlist.isEmpty()) {
        logError("播放列表为空，无法播放上一首");
        return;
    }
    
    try {
        // 获取上一首歌曲的索引
        int previousIndex = getPreviousIndex();
        
        // 设置当前索引并播放
        if (previousIndex >= 0 && previousIndex < m_playlist.size()) {
            setCurrentIndex(previousIndex);
            
            // 自动播放新选择的歌曲 - 强制播放
            
            // 确保状态为Loading，然后调用play()
            m_state = AudioTypes::AudioState::Loading;
            emit stateChanged(AudioTypes::AudioState::Loading);
            
            // 延迟一点调用play，确保setCurrentIndex的信号处理完成
            QTimer::singleShot(50, this, [this]() {
                play();
            });
        } else {
        }
    } catch (const std::exception& e) {
        logError(QString("播放上一首时发生异常: %1").arg(e.what()));
    } catch (...) {
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
    
            // 优先使用QMediaPlayer的音频输出进行平衡控制
        if (m_audioOutput) {
            try {
                // Qt6中QAudioOutput没有直接的平衡控制，但我们可以通过其他方式实现
                // 这里先记录设置，在音频处理中应用
                
                // 保存设置到配置
                AppConfig* config = AppConfig::instance();
                config->setValue("audio/balance", balance);
                
                // 如果有FFmpeg解码器且正在解码，也设置FFmpeg的平衡
                if (m_ffmpegDecoder && m_ffmpegDecoder->isDecoding()) {
                    try {
                        m_ffmpegDecoder->setBalance(balance);
                    } catch (const std::exception& e) {
                        qCritical() << "AudioEngine: 设置FFmpeg解码器平衡异常:" << e.what();
                    } catch (...) {
                        qCritical() << "AudioEngine: 设置FFmpeg解码器平衡未知异常";
                    }
                }
                
                updateBalance();
                
                logPlaybackEvent("声道平衡", QString("平衡: %1").arg(balance));
                emit balanceChanged(balance);
                
            } catch (const std::exception& e) {
                qCritical() << "AudioEngine: 设置平衡控制异常:" << e.what();
            }
        } else {
            qWarning() << "AudioEngine: 音频输出未初始化，无法设置平衡";
        }
}

double AudioEngine::getBalance() const
{
    return m_balance;
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
    // 避免使用互斥锁，因为可能导致死锁
    // QMutexLocker locker(&m_mutex);
    
    if (m_currentIndex >= 0 && m_currentIndex < m_playlist.size()) {
        const Song& song = m_playlist[m_currentIndex];
        return song;
    }
    
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
    // 获取文件扩展名
    QFileInfo fileInfo(filePath);
    QString extension = fileInfo.suffix().toLower();
    
    // 检查是否在支持的格式列表中
    return m_supportedFormats.contains(extension);
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
    
    // 记录到数据库
    if (m_playHistoryDao && song.isValid()) {
        m_playHistoryDao->addPlayRecord(song.id());
        logPlaybackEvent("添加到数据库历史", song.title());
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

// 删除未使用的onStateChanged方法，使用handlePlaybackStateChanged替代

void AudioEngine::onMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    emit mediaStatusChanged(status);
    
    if (status == QMediaPlayer::LoadedMedia) {
        logPlaybackEvent("媒体加载完成", currentSong().title());
        
        QMutexLocker locker(&m_mutex);
        if (m_state == AudioTypes::AudioState::Loading && !m_userPaused) {
            // 确保音频输出设置正确
            if (m_audioOutput) {
                m_audioOutput->setVolume(m_volume / 100.0);
                m_audioOutput->setMuted(false);
            }
            
            m_state = AudioTypes::AudioState::Playing;
            emit stateChanged(AudioTypes::AudioState::Playing);
            
            m_player->play();
            
            if (m_positionTimer) {
                m_positionTimer->start();
            }
        }
        
    } else if (status == QMediaPlayer::EndOfMedia) {
        logPlaybackEvent("播放完成", currentSong().title());
        handlePlaybackFinished();
    } else if (status == QMediaPlayer::InvalidMedia) {
        logError("无效的媒体文件");
        QMutexLocker locker(&m_mutex);
        m_state = AudioTypes::AudioState::Error;
        emit stateChanged(AudioTypes::AudioState::Error);
    }
}

void AudioEngine::handlePlaybackStateChanged(QMediaPlayer::PlaybackState state)
{
    QMutexLocker locker(&m_mutex);
    
    AudioTypes::AudioState newState = convertMediaState(state);
    
    // 只有当状态真正发生变化时才发送信号
    if (m_state != newState) {
        m_state = newState;
        
        // 根据状态更新用户暂停标志
        if (state == QMediaPlayer::PlayingState) {
            m_userPaused = false;
        } else if (state == QMediaPlayer::PausedState) {
            m_userPaused = true;
        }
        
        // 更新定时器状态
        if (m_positionTimer) {
            if (state == QMediaPlayer::PlayingState) {
                if (!m_positionTimer->isActive()) {
                    m_positionTimer->start();
                }
            } else {
                if (m_positionTimer->isActive()) {
                    m_positionTimer->stop();
                }
            }
        }
        
        emit stateChanged(m_state);
    }
    
    emit playbackStateChanged(static_cast<int>(state));
}

void AudioEngine::onErrorOccurred(QMediaPlayer::Error error)
{
    // 如果没有错误，直接返回
    if (error == QMediaPlayer::NoError) {
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
    
    // 记录错误并更新状态
    logError(fullErrorMessage);
    m_state = AudioTypes::AudioState::Error;
    
    // 发送信号
    emit stateChanged(AudioTypes::AudioState::Error);
    emit errorOccurred(errorString);
    
    // 如果是播放中发生错误，尝试停止播放器
    if (m_player && m_player->playbackState() != QMediaPlayer::StoppedState) {
        try {
            m_player->stop();
        } catch (...) {
            logError("停止播放器时发生异常");
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
    if (m_state == AudioTypes::AudioState::Playing) {
        qint64 currentPos = 0;
        
        // 根据当前音频引擎类型获取位置
        if (m_audioEngineType == AudioTypes::AudioEngineType::FFmpeg) {
            // FFmpeg模式：从FFmpeg解码器获取位置
            if (m_ffmpegDecoder && m_ffmpegDecoder->isDecoding()) {
                currentPos = m_ffmpegDecoder->getCurrentPosition();
            } else {
                // FFmpeg未运行，提供当前位置
                currentPos = m_position;
            }
        } else {
            // QMediaPlayer模式：从QMediaPlayer获取位置
            if (m_player) {
                currentPos = m_player->position();
            }
        }
        
        // 如果位置发生变化，更新并发送信号
        if (currentPos != m_position) {
            m_position = currentPos;
            emit positionChanged(currentPos);
        }
    }
}

// 私有方法实现
void AudioEngine::loadMedia(const QString& filePath)
{
    // 检查文件是否存在
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        throw std::runtime_error("文件不存在");
    }
    
    // 检查文件是否可读
    if (!fileInfo.isReadable()) {
        throw std::runtime_error("文件无法读取，可能是权限问题");
    }
    
    // 检查文件大小
    if (fileInfo.size() == 0) {
        throw std::runtime_error("文件大小为0");
    }
    
    // 检查音频格式支持
    QString extension = fileInfo.suffix().toLower();
    
    if (!checkAudioFormat(filePath)) {
        throw std::runtime_error(QString("不支持的音频格式: %1").arg(extension).toStdString());
    }
    
    // 设置媒体源
    QUrl mediaUrl = QUrl::fromLocalFile(filePath);
    
    // 检查URL是否有效
    if (!mediaUrl.isValid()) {
        throw std::runtime_error("无效的媒体URL");
    }
    
    // 设置媒体源
    m_player->setSource(mediaUrl);
    
    // 记录加载事件
    logPlaybackEvent("加载媒体", filePath);
}

void AudioEngine::updateCurrentSong()
{
    if (m_currentIndex >= 0 && m_currentIndex < m_playlist.size()) {
        const Song& currentSong = m_playlist[m_currentIndex];
        emit currentSongChanged(currentSong);
        emit currentIndexChanged(m_currentIndex);
    } else {
        logError("索引无效，无法更新当前歌曲");
    }
}

void AudioEngine::handlePlaybackFinished()
{
    // 检查播放列表是否为空
    if (m_playlist.isEmpty()) {
        return;
    }
    
    try {
        // 根据播放模式执行相应操作
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
            default:
                playNext();
                break;
        }
    } catch (const std::exception& e) {
        logError(QString("处理播放完成事件时发生异常: %1").arg(e.what()));
    } catch (...) {
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
    // 检查播放列表是否为空
    if (m_playlist.isEmpty()) {
        return -1;
    }
    
    // 检查当前索引是否有效
    if (m_currentIndex < 0 || m_currentIndex >= m_playlist.size()) {
        return 0; // 如果当前索引无效，返回第一首歌曲
    }
    
    int nextIndex = -1;
    
    // 根据播放模式计算下一首歌曲索引
    switch (m_playMode) {
        case AudioTypes::PlayMode::Loop:
            nextIndex = (m_currentIndex + 1) % m_playlist.size();
            break;
            
        case AudioTypes::PlayMode::RepeatOne:
            nextIndex = m_currentIndex;
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
            break;
            
        default:
            nextIndex = (m_currentIndex + 1) % m_playlist.size();
            break;
    }
    
    return nextIndex;
}

int AudioEngine::getPreviousIndex()
{
    // 检查播放列表是否为空
    if (m_playlist.isEmpty()) {
        return -1;
    }
    
    // 检查当前索引是否有效
    if (m_currentIndex < 0 || m_currentIndex >= m_playlist.size()) {
        return 0; // 如果当前索引无效，返回第一首歌曲
    }
    
    int previousIndex = -1;
    
    // 根据播放模式计算上一首歌曲索引
    switch (m_playMode) {
        case AudioTypes::PlayMode::Loop:
            previousIndex = (m_currentIndex - 1 + m_playlist.size()) % m_playlist.size();
            break;
            
        case AudioTypes::PlayMode::RepeatOne:
            previousIndex = m_currentIndex;
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
            break;
            
        default:
            previousIndex = (m_currentIndex - 1 + m_playlist.size()) % m_playlist.size();
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
    // 虽然Qt6的QAudioOutput没有直接的平衡控制，
    // 但我们可以通过音量调整和其他方式来实现类似效果
    
    if (!m_audioOutput) {
        qWarning() << "AudioEngine: 音频输出未初始化，无法应用平衡设置";
        return;
    }
    
    try {
        // 当前实现：记录平衡设置，为将来的实现做准备
        // 在Qt6中，真正的左右声道平衡控制需要通过音频过滤器实现
        
        // 保存设置到配置文件
        AppConfig* config = AppConfig::instance();
        config->setValue("audio/balance", m_balance);
        
        // 如果有FFmpeg解码器正在运行，确保其平衡设置同步
        if (m_ffmpegDecoder && m_ffmpegDecoder->isDecoding()) {
            try {
                m_ffmpegDecoder->setBalance(m_balance);
            } catch (...) {
                qWarning() << "AudioEngine: 同步FFmpeg解码器平衡设置失败";
            }
        }
        
        // 发出信号通知平衡变化
        emit balanceChanged(m_balance);
        
    } catch (const std::exception& e) {
        qCritical() << "AudioEngine: 更新平衡设置异常:" << e.what();
    }
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
    Q_UNUSED(event);
    Q_UNUSED(details);
}

void AudioEngine::logError(const QString& error)
{
    Logger::instance()->error(error, "AudioEngine");
}

// 格式检查
bool AudioEngine::checkAudioFormat(const QString& filePath) const
{
    return isFormatSupported(filePath);
}

QString AudioEngine::getFileExtension(const QString& filePath) const
{
    QFileInfo fileInfo(filePath);
    return fileInfo.suffix().toLower();
}

// 状态转换
AudioTypes::AudioState AudioEngine::convertMediaState(QMediaPlayer::PlaybackState state) const
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

// ====================== 音频引擎类型控制方法 ======================

void AudioEngine::setAudioEngineType(AudioTypes::AudioEngineType type)
{
    QMutexLocker locker(&m_mutex);
    
    if (m_audioEngineType == type) {
        return; // 没有变化，直接返回
    }
    

    
    // 停止当前播放
    bool wasPlaying = (m_state == AudioTypes::AudioState::Playing);
    Song currentSong;
    qint64 currentPosition = 0;
    
    if (wasPlaying) {
        currentSong = m_playlist.isEmpty() ? Song() : m_playlist.at(m_currentIndex);
        currentPosition = m_position;
        stop(); // 停止当前播放
    }
    
    // 更新引擎类型
    m_audioEngineType = type;
    
    // 保存设置到配置文件
    AppConfig* config = AppConfig::instance();
    config->setValue("audio/engine_type", static_cast<int>(type));
    
    // 发出信号通知变化
    emit audioEngineTypeChanged(type);
    
    // 如果之前在播放，使用新引擎继续播放
    if (wasPlaying && currentSong.isValid()) {
        QTimer::singleShot(100, this, [this, currentSong, currentPosition]() {
            play();
            if (currentPosition > 1000) { // 如果位置大于1秒，则跳转
                QTimer::singleShot(500, this, [this, currentPosition]() {
                    seek(currentPosition);
                });
            }
        });
    }
    

}

AudioTypes::AudioEngineType AudioEngine::getAudioEngineType() const
{
    QMutexLocker locker(&m_mutex);
    return m_audioEngineType;
}

QString AudioEngine::getAudioEngineTypeString() const
{
    QMutexLocker locker(&m_mutex);
    switch (m_audioEngineType) {
    case AudioTypes::AudioEngineType::QMediaPlayer:
        return "QMediaPlayer";
    case AudioTypes::AudioEngineType::FFmpeg:
        return "FFmpeg";
    default:
        return "Unknown";
    }
}

// ====================== 播放方式实现 ======================

void AudioEngine::playWithQMediaPlayer(const Song& song)
{
    // 停止FFmpeg解码器（如果正在运行）
    if (m_ffmpegDecoder && m_ffmpegDecoder->isDecoding()) {
        try {
            m_ffmpegDecoder->stopDecoding(); // 修复：使用正确的方法名stopDecoding()
        } catch (...) {
            qWarning() << "AudioEngine: 停止FFmpeg解码器时发生异常";
        }
    }
    
    // 确保QMediaPlayer已初始化
    if (!m_player) {
        logError("QMediaPlayer未初始化");
        return;
    }
    
            QMediaPlayer::PlaybackState playerState = m_player->playbackState();
    
    // 如果已暂停，恢复播放
    if (playerState == QMediaPlayer::PausedState) {
        m_player->play();
        m_state = AudioTypes::AudioState::Playing;
        m_userPaused = false;
        emit stateChanged(m_state);
        
        if (m_positionTimer) {
            m_positionTimer->start();
        }
        return;
    }
    
    // 如果正在播放，确保状态同步
    if (playerState == QMediaPlayer::PlayingState) {
        if (m_state != AudioTypes::AudioState::Playing) {
            m_state = AudioTypes::AudioState::Playing;
            m_userPaused = false;
            emit stateChanged(m_state);
        }
        return;
    }
    
    // 如果处于停止状态，需要重新加载媒体
    if (playerState == QMediaPlayer::StoppedState) {
        // 设置音频输出
        if (m_player->audioOutput() != m_audioOutput) {
            m_player->setAudioOutput(m_audioOutput);
        }
        
        if (m_audioOutput) {
            m_audioOutput->setVolume(m_volume / 100.0);
            m_audioOutput->setMuted(m_muted);
            
            // 确保音频输出设备正确设置
            QAudioDevice device = m_audioOutput->device();
            if (device.description().isEmpty()) {
                QMediaDevices mediaDevices;
                QList<QAudioDevice> audioOutputs = mediaDevices.audioOutputs();
                if (!audioOutputs.isEmpty()) {
                    QAudioDevice defaultDevice = audioOutputs.first();
                    m_audioOutput->setDevice(defaultDevice);
                }
            }
        }
        
        // 加载并播放
        m_state = AudioTypes::AudioState::Loading;
        emit stateChanged(AudioTypes::AudioState::Loading);
        
        loadMedia(song.filePath());
        
        // 在loadMedia后调用play()开始播放
        m_player->play();
        m_state = AudioTypes::AudioState::Playing;
        m_userPaused = false;
        emit stateChanged(m_state);
        
        if (m_positionTimer) {
            m_positionTimer->start();
        }
        
        logPlaybackEvent("QMediaPlayer开始播放", song.title());
        updateCurrentSong();
        addToHistory(song);
        
        return;
    }
    
    // 其他状态的处理
    m_player->play();
    m_state = AudioTypes::AudioState::Playing;
    m_userPaused = false;
    emit stateChanged(m_state);
    
    if (m_positionTimer) {
        m_positionTimer->start();
    }
    
    logPlaybackEvent("QMediaPlayer开始播放", song.title());
}

void AudioEngine::playWithFFmpeg(const Song& song)
{
    // 停止QMediaPlayer（如果正在播放）
    if (m_player && m_player->playbackState() != QMediaPlayer::StoppedState) {
        m_player->stop();
    }
    
    // 确保FFmpeg解码器已初始化
    if (!m_ffmpegDecoder) {
        qWarning() << "AudioEngine: FFmpeg解码器未初始化，回退到QMediaPlayer";
        playWithQMediaPlayer(song);
        return;
    }
    
    try {
        if (m_ffmpegDecoder->openFile(song.filePath())) {
            // 应用当前的音效设置
            m_ffmpegDecoder->setBalance(m_balance);
            
            if (m_ffmpegDecoder->startDecoding()) {
                // FFmpeg解码器播放成功，设置状态
                m_state = AudioTypes::AudioState::Playing;
                m_userPaused = false;
                emit stateChanged(m_state);
                
                if (m_positionTimer) {
                    m_positionTimer->start();
                }
                
                logPlaybackEvent("FFmpeg开始播放", song.title());
                updateCurrentSong();
                addToHistory(song);
                
                return;
            } else {
                qWarning() << "AudioEngine: FFmpegDecoder::startDecoding失败，回退到QMediaPlayer";
            }
        } else {
            qWarning() << "AudioEngine: FFmpegDecoder::openFile失败，回退到QMediaPlayer";
        }
    } catch (const std::exception& e) {
        qCritical() << "AudioEngine: FFmpegDecoder操作异常:" << e.what() << "，回退到QMediaPlayer";
    } catch (...) {
        qCritical() << "AudioEngine: FFmpegDecoder操作未知异常，回退到QMediaPlayer";
    }
    
    // 如果FFmpeg播放失败，自动回退到QMediaPlayer
    playWithQMediaPlayer(song);
}

// ==================== 缺失方法实现 ====================

void AudioEngine::saveBalanceSettings()
{
    // 使用AppConfig保存平衡设置
    try {
        AppConfig::instance()->setValue("Audio/Balance", m_balance);
        AppConfig::instance()->saveConfig();
    } catch (const std::exception& e) {
        qWarning() << "AudioEngine: 保存平衡设置失败:" << e.what();
    }
}

void AudioEngine::loadBalanceSettings()
{
    // 从AppConfig加载平衡设置
    try {
        double savedBalance = AppConfig::instance()->getValue("Audio/Balance", 0.0).toDouble();
        m_balance = std::clamp(savedBalance, -1.0, 1.0);
        
        // 应用平衡设置到当前解码器
        if (m_ffmpegDecoder && m_ffmpegDecoder->isDecoding()) {
            m_ffmpegDecoder->setBalance(m_balance);
        }
        
        emit balanceChanged(m_balance);
    } catch (const std::exception& e) {
        qWarning() << "AudioEngine: 加载平衡设置失败:" << e.what();
        m_balance = 0.0; // 使用默认值
    }
}

void AudioEngine::debugAudioState() const
{
    // 调试信息已移除
}

void AudioEngine::initializeFFmpegDecoder()
{
    try {
        if (!m_ffmpegDecoder) {
            m_ffmpegDecoder = new FFmpegDecoder(this);
        }
        
        if (m_ffmpegDecoder->initialize()) {
            setupFFmpegConnections();
            m_ffmpegDecoder->setBalance(m_balance);
        } else {
            logError("FFmpeg解码器初始化失败");
        }
    } catch (const std::exception& e) {
        logError(QString("FFmpeg解码器初始化异常: %1").arg(e.what()));
    } catch (...) {
        logError("FFmpeg解码器初始化未知异常");
    }
}

void AudioEngine::cleanupFFmpegDecoder()
{
    try {
        if (m_ffmpegDecoder) {
            if (m_ffmpegDecoder->isDecoding()) {
                m_ffmpegDecoder->stopDecoding();
            }
            m_ffmpegDecoder->closeFile();
            m_ffmpegDecoder->cleanup();
            delete m_ffmpegDecoder;
            m_ffmpegDecoder = nullptr;
        }
    } catch (const std::exception& e) {
        logError(QString("清理FFmpeg解码器异常: %1").arg(e.what()));
    } catch (...) {
        logError("清理FFmpeg解码器未知异常");
    }
}

void AudioEngine::updateVULevels()
{
    QVector<double> levels;
    
    if (m_ffmpegDecoder && m_ffmpegDecoder->isDecoding()) {
        levels = m_ffmpegDecoder->getCurrentLevels();
    } else if (m_player && m_player->playbackState() == QMediaPlayer::PlayingState) {
        levels = QVector<double>(2, 0.0);
    } else {
        levels = QVector<double>(2, 0.0);
    }
    
    m_vuLevels = levels;
    emit vuLevelsChanged(m_vuLevels);
}

QString AudioEngine::getStateString() const
{
    switch (m_state) {
        case AudioTypes::AudioState::Stopped:
            return "Stopped";
        case AudioTypes::AudioState::Playing:
            return "Playing";
        case AudioTypes::AudioState::Paused:
            return "Paused";
        case AudioTypes::AudioState::Loading:
            return "Loading";
        case AudioTypes::AudioState::Error:
            return "Error";
        default:
            return "Unknown";
    }
}

void AudioEngine::setupFFmpegConnections()
{
    if (!m_ffmpegDecoder) {
        logError("FFmpeg解码器未初始化，无法设置连接");
        return;
    }
    
    try {
        connect(m_ffmpegDecoder, &FFmpegDecoder::audioDataReady, 
                this, &AudioEngine::onFFmpegAudioDataReady);
        connect(m_ffmpegDecoder, &FFmpegDecoder::positionChanged, 
                this, &AudioEngine::onFFmpegPositionChanged);
        connect(m_ffmpegDecoder, &FFmpegDecoder::durationChanged, 
                this, &AudioEngine::onFFmpegDurationChanged);
        connect(m_ffmpegDecoder, &FFmpegDecoder::decodingFinished, 
                this, &AudioEngine::onFFmpegDecodingFinished);
        connect(m_ffmpegDecoder, &FFmpegDecoder::errorOccurred, 
                this, &AudioEngine::onFFmpegErrorOccurred);
    } catch (const std::exception& e) {
        logError(QString("设置FFmpeg解码器连接异常: %1").arg(e.what()));
    } catch (...) {
        logError("设置FFmpeg解码器连接未知异常");
    }
}

// ==================== FFmpeg解码器槽方法实现 ====================

void AudioEngine::onFFmpegAudioDataReady(const QVector<double>& levels)
{
    m_realTimeLevels = levels;
    
    if (m_vuEnabled) {
        m_vuLevels = levels;
        emit vuLevelsChanged(m_vuLevels);
    }
}

void AudioEngine::onFFmpegPositionChanged(qint64 position)
{
    m_position = position;
    emit positionChanged(position);
}

void AudioEngine::onFFmpegDurationChanged(qint64 duration)
{
    m_duration = duration;
    emit durationChanged(duration);
}

void AudioEngine::onFFmpegDecodingFinished()
{
    handlePlaybackFinished();
}

void AudioEngine::onFFmpegErrorOccurred(const QString& error)
{
    logError(QString("FFmpeg解码器错误: %1").arg(error));
    emit errorOccurred(error);
    m_state = AudioTypes::AudioState::Error;
    emit stateChanged(m_state);
}
