#include "audioworkerthread.h"
#include "../core/logger.h"
#include <QDebug>
#include <QCoreApplication>

// 注册枚举类型到 Qt 元对象系统
Q_DECLARE_METATYPE(AudioWorkerThread::ThreadState)

AudioEffectProcessor::AudioEffectProcessor() : m_equalizerEnabled(false), m_reverbEnabled(false), m_reverbIntensity(0.5), m_balance(0.0), m_crossfadeDuration(0) {
    m_equalizerBands.resize(10);
    m_equalizerBands.fill(0.0);
}

AudioEffectProcessor::~AudioEffectProcessor() {}

// 实现音效方法（简化版）
void AudioEffectProcessor::setEqualizerEnabled(bool enabled) { m_equalizerEnabled = enabled; }
void AudioEffectProcessor::setEqualizerBands(const QVector<double>& bands) { m_equalizerBands = bands; }
// 其他音效设置方法类似

QByteArray AudioEffectProcessor::processAudio(const QByteArray& input) {
    QByteArray output = input;
    // 应用音效（这里简化，实际需实现DSP）
    qDebug() << "[AudioEffect] Processing audio data, size:" << input.size();
    return output;
}

AudioWorkerThread::AudioWorkerThread(QObject* parent) : QThread(parent),
    m_running(false), m_shouldStop(false), m_threadState(ThreadState::Stopped),
    m_mediaPlayer(nullptr), m_audioOutput(nullptr), m_audioSink(nullptr), m_audioBuffer(nullptr) {
    // 注册枚举类型
    qRegisterMetaType<ThreadState>("AudioWorkerThread::ThreadState");
    qRegisterMetaType<AudioTypes::BufferStatus>("AudioTypes::BufferStatus");

    // 初始化媒体播放器
    m_mediaPlayer = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_mediaPlayer->setAudioOutput(m_audioOutput);

    // 连接信号
    connect(m_mediaPlayer, &QMediaPlayer::positionChanged, this, &AudioWorkerThread::positionChanged, Qt::QueuedConnection);
    connect(m_mediaPlayer, &QMediaPlayer::durationChanged, this, &AudioWorkerThread::durationChanged, Qt::QueuedConnection);
    connect(m_mediaPlayer, &QMediaPlayer::playbackStateChanged, this, [this](QMediaPlayer::PlaybackState state) {
        switch (state) {
            case QMediaPlayer::PlayingState:
                emit playbackStarted(m_mediaPlayer->source().toString());
                break;
            case QMediaPlayer::PausedState:
                emit playbackPaused();
                break;
            case QMediaPlayer::StoppedState:
                emit playbackStopped();
                break;
        }
    }, Qt::QueuedConnection);

    // 连接错误信号
    connect(m_mediaPlayer, &QMediaPlayer::errorOccurred, this, &AudioWorkerThread::handleMediaPlayerError, Qt::QueuedConnection);

    // 创建定时器用于更新缓冲状态
    QTimer* bufferTimer = new QTimer(this);
    connect(bufferTimer, &QTimer::timeout, this, &AudioWorkerThread::updateBufferStatus, Qt::QueuedConnection);
    bufferTimer->start(100); // 每100ms更新一次

    qDebug() << "[AudioWorker] Thread and media components initialized";
}

AudioWorkerThread::~AudioWorkerThread() {
    stopThread();

    // 清理媒体组件
    if (m_mediaPlayer) {
        m_mediaPlayer->stop();
        delete m_mediaPlayer;
        m_mediaPlayer = nullptr;
    }

    if (m_audioOutput) {
        delete m_audioOutput;
        m_audioOutput = nullptr;
    }

    if (m_audioSink) {
        delete m_audioSink;
        m_audioSink = nullptr;
    }

    if (m_audioBuffer) {
        delete m_audioBuffer;
        m_audioBuffer = nullptr;
    }

    qDebug() << "[AudioWorker] Thread and media components destroyed";
}

void AudioWorkerThread::startThread() {
    QMutexLocker locker(const_cast<QMutex*>(&m_mutex));
    if (!m_running) {
        m_running = true;
        m_shouldStop = false;
        m_threadState = ThreadState::Running;
        start();
        qDebug() << "[AudioWorker] Thread started";
    } else {
        qDebug() << "[AudioWorker] Thread already running";
    }
}

void AudioWorkerThread::stopThread() {
    QMutexLocker locker(&m_mutex);
    if (m_running) {
        m_shouldStop = true;
        m_threadState = ThreadState::Stopped;
        m_commandQueueCondition.wakeAll();
        locker.unlock();

        // 等待线程结束
        if (!wait(5000)) { // 等待最多5秒
            qWarning() << "[AudioWorker] Thread did not stop gracefully, forcing termination";
            terminate();
            wait();
        }

        m_running = false;
        m_threadState = ThreadState::Stopped;
        qDebug() << "[AudioWorker] Thread stopped";
    } else {
        qDebug() << "[AudioWorker] Thread already stopped";
    }
}



bool AudioWorkerThread::isRunning() const {
    QMutexLocker locker(const_cast<QMutex*>(&m_mutex));
    return m_running;
}

AudioWorkerThread::ThreadState AudioWorkerThread::threadState() const {
    QMutexLocker locker(const_cast<QMutex*>(&m_mutex));
    return m_threadState;
}

void AudioWorkerThread::setEqualizerSettings(bool enabled, const QVector<double>& bands) {
    AudioCommand cmd(AudioCommandType::ApplyEffects);
    cmd.boolParam = enabled;
    cmd.variantParam = QVariant::fromValue(bands);
    enqueueCommand(cmd);
    qDebug() << "[AudioWorker] Enqueued equalizer settings command, enabled:" << enabled;
}

void AudioWorkerThread::setReverbSettings(bool enabled, double intensity) {
    AudioCommand cmd(AudioCommandType::ApplyEffects);
    cmd.boolParam = enabled;
    cmd.variantParam = intensity;
    enqueueCommand(cmd);
    qDebug() << "[AudioWorker] Enqueued reverb settings command, enabled:" << enabled << "intensity:" << intensity;
}

void AudioWorkerThread::setBalanceSettings(double balance) {
    AudioCommand cmd(AudioCommandType::ApplyEffects);
    cmd.variantParam = balance;
    enqueueCommand(cmd);
    qDebug() << "[AudioWorker] Enqueued balance settings command, balance:" << balance;
}

void AudioWorkerThread::setCrossfadeSettings(int duration) {
    AudioCommand cmd(AudioCommandType::ApplyEffects);
    cmd.intParam = duration;
    enqueueCommand(cmd);
    qDebug() << "[AudioWorker] Enqueued crossfade settings command, duration:" << duration;
}

void AudioWorkerThread::preloadMedia(const QString& filePath) {
    AudioCommand cmd(AudioCommandType::LoadMedia);
    cmd.stringParam = filePath;
    enqueueCommand(cmd);
    qDebug() << "[AudioWorker] Enqueued media preload command for file:" << filePath;
}

void AudioWorkerThread::preloadMediaBatch(const QStringList& filePaths) {
    for (const QString& filePath : filePaths) {
        preloadMedia(filePath);
    }
    qDebug() << "[AudioWorker] Enqueued batch media preload command for" << filePaths.size() << "files";
}

void AudioWorkerThread::setBufferSize(int size) {
    // 设置缓冲区大小的实现
    if (size > 0) {
        QMutexLocker locker(&m_mutex);
        if (m_audioBuffer) {
            QByteArray* buffer = new QByteArray(size, 0);
            m_audioBuffer->setBuffer(buffer);
            qDebug() << "[AudioWorker] Buffer size set to:" << size << "bytes";
        }
    }
}

int AudioWorkerThread::bufferSize() const {
    QMutexLocker locker(const_cast<QMutex*>(&m_mutex));
    return m_audioBuffer ? m_audioBuffer->size() : 0;
}

void AudioWorkerThread::playAudio(const QString& filePath) {
    AudioCommand cmd(AudioCommandType::Play);
    cmd.stringParam = filePath;
    enqueueCommand(cmd);
    qDebug() << "[AudioWorker] Enqueued play command for file:" << filePath;
}

void AudioWorkerThread::pauseAudio() {
    AudioCommand cmd(AudioCommandType::Pause);
    enqueueCommand(cmd);
    qDebug() << "[AudioWorker] Enqueued pause command";
}

void AudioWorkerThread::resumeAudio() {
    AudioCommand cmd(AudioCommandType::Play);
    enqueueCommand(cmd);
    qDebug() << "[AudioWorker] Enqueued resume command";
}

void AudioWorkerThread::stopAudio() {
    AudioCommand cmd(AudioCommandType::Stop);
    enqueueCommand(cmd);
    qDebug() << "[AudioWorker] Enqueued stop command";
}

void AudioWorkerThread::seekAudio(qint64 position) {
    qDebug() << "[AudioWorker] 开始执行seekAudio，位置:" << position << "ms";
    AudioCommand cmd(AudioCommandType::Seek);
    cmd.int64Param = position;
    enqueueCommand(cmd);
    qDebug() << "[AudioWorker] seek命令已入队，位置:" << position << "ms";
}

void AudioWorkerThread::setVolume(int volume) {
    AudioCommand cmd(AudioCommandType::SetVolume);
    cmd.intParam = volume;
    enqueueCommand(cmd);
    qDebug() << "[AudioWorker] Enqueued set volume command:" << volume;
}

void AudioWorkerThread::setMuted(bool muted) {
    AudioCommand cmd(AudioCommandType::SetMuted);
    cmd.boolParam = muted;
    enqueueCommand(cmd);
    qDebug() << "[AudioWorker] Enqueued set muted command:" << muted;
}

void AudioWorkerThread::enqueueCommand(const AudioCommand& cmd) {
    QMutexLocker locker(&m_mutex);
    m_commandQueue.enqueue(cmd);
    m_commandQueueCondition.wakeOne();
    qDebug() << "[AudioWorker] Command enqueued, type:" << static_cast<int>(cmd.type);
}



void AudioWorkerThread::run() {
    qDebug() << "[AudioWorker] Thread started running";
    emit threadStateChanged(ThreadState::Running);

    while (!m_shouldStop) {
        AudioCommand cmd;
        {
            QMutexLocker locker(&m_mutex);
            while (m_commandQueue.isEmpty() && !m_shouldStop) {
                m_commandQueueCondition.wait(&m_mutex);
            }
            if (m_shouldStop) break;
            cmd = m_commandQueue.dequeue();
        }
        handleAudioCommand(cmd);
    }

    qDebug() << "[AudioWorker] Thread stopped running";
    emit threadStateChanged(ThreadState::Stopped);
}

void AudioWorkerThread::handleAudioCommand(const AudioCommand& command) {
    switch (command.type) {
        case AudioCommandType::Play:
            if (!command.stringParam.isEmpty()) {
                m_mediaPlayer->setSource(QUrl::fromLocalFile(command.stringParam));
                m_mediaPlayer->play();
            } else {
                m_mediaPlayer->play();
            }
            break;

        case AudioCommandType::Pause:
            m_mediaPlayer->pause();
            break;

        case AudioCommandType::Stop:
            m_mediaPlayer->stop();
            break;

        case AudioCommandType::Seek:
            qDebug() << "[AudioWorker] 执行seek命令，位置:" << command.int64Param << "ms";
            m_mediaPlayer->setPosition(command.int64Param);
            qDebug() << "[AudioWorker] seek命令执行完成";
            break;

        case AudioCommandType::SetVolume:
            m_audioOutput->setVolume(command.intParam / 100.0);
            emit volumeChanged(command.intParam);
            break;

        case AudioCommandType::SetMuted:
            m_audioOutput->setMuted(command.boolParam);
            emit mutedChanged(command.boolParam);
            break;

        case AudioCommandType::LoadMedia:
            preloadAudioFile(command.stringParam);
            break;

        case AudioCommandType::ApplyEffects:
            if (m_effectProcessor) {
                // 处理不同类型的音效命令
                // 这里简化处理，实际应该根据具体参数类型判断
                emit effectsApplied();
            }
            break;

        default:
            qWarning() << "[AudioWorker] Unknown command type:" << static_cast<int>(command.type);
            break;
    }
}

void AudioWorkerThread::handleMediaPlayerError(QMediaPlayer::Error error) {
    QString errorMessage;
    switch (error) {
        case QMediaPlayer::NoError:
            return;
        case QMediaPlayer::ResourceError:
            errorMessage = "无法加载媒体资源";
            break;
        case QMediaPlayer::FormatError:
            errorMessage = "不支持的媒体格式";
            break;
        case QMediaPlayer::NetworkError:
            errorMessage = "网络错误";
            break;
        case QMediaPlayer::AccessDeniedError:
            errorMessage = "访问被拒绝";
            break;
        default:
            errorMessage = "未知错误";
            break;
    }
    
    emit audioError(errorMessage);
    m_threadState = ThreadState::Error;
    emit threadStateChanged(m_threadState);
    qWarning() << "[AudioWorker] Media player error:" << errorMessage;
}

void AudioWorkerThread::handleAudioOutputError() {
    QString errorMessage = "音频输出错误";
    emit audioError(errorMessage);
    m_threadState = ThreadState::Error;
    emit threadStateChanged(m_threadState);
    qWarning() << "[AudioWorker] Audio output error:" << errorMessage;
}

void AudioWorkerThread::updateBufferStatus() {
    if (!m_audioBuffer) return;
    
    int bufferSize = m_audioBuffer->size();
    int bufferUsed = m_audioBuffer->pos();
    int bufferPercentage = (bufferSize > 0) ? (bufferUsed * 100 / bufferSize) : 0;
    
    emit bufferProgressChanged(bufferPercentage);
    
    if (bufferUsed < bufferSize * 0.1) { // 低于10%
        emit bufferStatusChanged(AudioTypes::BufferStatus::Empty);
        emit bufferUnderrun();
    } else if (bufferUsed > bufferSize * 0.9) { // 高于90%
        emit bufferStatusChanged(AudioTypes::BufferStatus::Buffered);
        emit bufferOverflow();
    } else {
        emit bufferStatusChanged(AudioTypes::BufferStatus::Buffering);
    }
}

void AudioWorkerThread::preloadAudioFile(const QString& filePath) {
    QMutexLocker locker(&m_preloadMutex);
    
    if (m_preloadCache.contains(filePath)) {
        emit mediaPreloaded(filePath);
        return;
    }
    
    try {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            throw std::runtime_error("无法打开文件");
        }
        
        QByteArray audioData = file.readAll();
        m_preloadCache.insert(filePath, audioData);
        
        emit mediaPreloaded(filePath);
        emit preloadProgress(filePath, 100);
        
        // 清理缓存，如果超过预设大小
        managePreloadCache();
        
    } catch (const std::exception& e) {
        emit preloadError(filePath, QString::fromUtf8(e.what()));
        qWarning() << "[AudioWorker] Preload error for file:" << filePath << ", error:" << e.what();
    }
}

void AudioWorkerThread::managePreloadCache() {
    const int MAX_CACHE_SIZE = 100 * 1024 * 1024; // 100MB
    const int MAX_CACHE_FILES = 10;
    
    while (!m_preloadCache.isEmpty() && 
           (m_preloadCache.size() > MAX_CACHE_FILES || 
            calculateCacheSize() > MAX_CACHE_SIZE)) {
        m_preloadCache.remove(m_preloadCache.firstKey());
    }
}

qint64 AudioWorkerThread::calculateCacheSize() const {
    qint64 totalSize = 0;
    for (const QByteArray& data : m_preloadCache) {
        totalSize += data.size();
    }
    return totalSize;
}