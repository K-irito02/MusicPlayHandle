#ifndef AUDIOWORKERTHREAD_H
#define AUDIOWORKERTHREAD_H

#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QQueue>
#include <QTimer>
#include <QBuffer>
#include <QByteArray>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QAudioSink>
#include <memory>

#include "../models/song.h"
#include "../audio/audiotypes.h"

// 音频命令类型
enum class AudioCommandType {
    Play,
    Pause,
    Stop,
    Seek,
    SetVolume,
    SetMuted,
    LoadMedia,
    ApplyEffects
};

// 音频命令结构
struct AudioCommand {
    AudioCommandType type;
    QString stringParam;
    qint64 int64Param;
    int intParam;
    bool boolParam;
    QVariant variantParam;
    
    AudioCommand(AudioCommandType t = AudioCommandType::Play) : type(t) {}
};

// 音频效果处理器
class AudioEffectProcessor {
public:
    AudioEffectProcessor();
    ~AudioEffectProcessor();
    
    // 音效控制
    void setEqualizerEnabled(bool enabled);
    void setEqualizerBands(const QVector<double>& bands);
    void setReverb(bool enabled, double intensity = 0.5);
    void setBalance(double balance);
    void setCrossfadeDuration(int duration);
    
    // 音频数据处理
    QByteArray processAudio(const QByteArray& input);
    
    // 获取当前设置
    bool isEqualizerEnabled() const { return m_equalizerEnabled; }
    QVector<double> equalizerBands() const { return m_equalizerBands; }
    bool isReverbEnabled() const { return m_reverbEnabled; }
    double reverbIntensity() const { return m_reverbIntensity; }
    double balance() const { return m_balance; }
    int crossfadeDuration() const { return m_crossfadeDuration; }
    
private:
    bool m_equalizerEnabled;
    QVector<double> m_equalizerBands;
    bool m_reverbEnabled;
    double m_reverbIntensity;
    double m_balance;
    int m_crossfadeDuration;
    
    // 音效处理方法
    QByteArray applyEqualizer(const QByteArray& input);
    QByteArray applyReverb(const QByteArray& input);
    QByteArray applyBalance(const QByteArray& input);
    QByteArray applyCrossfade(const QByteArray& input);
    
    // 音频数据转换
    QVector<double> byteArrayToDouble(const QByteArray& data);
    QByteArray doubleToByteArray(const QVector<double>& data);
};

// 音频工作线程
class AudioWorkerThread : public QThread {
    Q_OBJECT
    
public:
    explicit AudioWorkerThread(QObject* parent = nullptr);
    ~AudioWorkerThread();
    
    // 线程控制
    void startThread();
    void stopThread();
    bool isRunning() const;
    
    // 播放控制命令
    void playAudio(const QString& filePath);
    void pauseAudio();
    void resumeAudio();
    void stopAudio();
    void seekAudio(qint64 position);
    
    // 音量控制命令
    void setVolume(int volume);
    void setMuted(bool muted);
    
    // 音效处理命令
    void setEqualizerSettings(bool enabled, const QVector<double>& bands);
    void setReverbSettings(bool enabled, double intensity);
    void setBalanceSettings(double balance);
    void setCrossfadeSettings(int duration);
    
    // 媒体预加载
    void preloadMedia(const QString& filePath);
    void preloadMediaBatch(const QStringList& filePaths);
    
    // 缓冲区管理
    void setBufferSize(int size);
    int bufferSize() const;
    
    // 线程状态
    enum class ThreadState {
        Stopped,
        Running,
        Paused,
        Error
    };
    
    ThreadState threadState() const;
    
signals:
    // 播放状态信号
    void audioLoaded(const Song& song);
    void playbackStarted(const QString& filePath);
    void playbackPaused();
    void playbackResumed();
    void playbackStopped();
    void playbackFinished();
    
    // 位置和时长信号
    void positionChanged(qint64 position);
    void durationChanged(qint64 duration);
    
    // 音量信号
    void volumeChanged(int volume);
    void mutedChanged(bool muted);
    
    // 错误信号
    void audioError(const QString& error);
    void threadError(const QString& error);
    
    // 缓冲信号
    void bufferStatusChanged(AudioTypes::BufferStatus status);
    void bufferProgressChanged(int percentage);
    void bufferUnderrun();
    void bufferOverflow();
    
    // 音效信号
    void effectsApplied();
    void effectsError(const QString& error);
    
    // 预加载信号
    void mediaPreloaded(const QString& filePath);
    void preloadProgress(const QString& filePath, int percentage);
    void preloadError(const QString& filePath, const QString& error);
    
    // 线程状态信号
    void threadStateChanged(ThreadState state);
    
protected:
    void run() override;
    
private slots:
    void handleAudioCommand(const AudioCommand& command);
    void updateBufferStatus();
    void handleMediaPlayerError(QMediaPlayer::Error error);
    void handleAudioOutputError();
    
private:
    // 线程控制
    QMutex m_mutex;
    QWaitCondition m_waitCondition;
    bool m_running;
    bool m_shouldStop;
    ThreadState m_threadState;
    
    // 命令队列
    QQueue<AudioCommand> m_commandQueue;
    QMutex m_commandMutex;
    
    // 音频组件
    QMediaPlayer* m_mediaPlayer;
    QAudioOutput* m_audioOutput;
    QAudioSink* m_audioSink;
    
    // 音频缓冲区
    QBuffer* m_audioBuffer;
    QByteArray m_audioData;
    int m_bufferSize;
    
    // 音效处理器
    std::unique_ptr<AudioEffectProcessor> m_effectProcessor;
    
    // 当前状态
    QString m_currentFilePath;
    qint64 m_currentPosition;
    qint64 m_currentDuration;
    int m_currentVolume;
    bool m_currentMuted;
    
    // 预加载缓存
    QMap<QString, QByteArray> m_preloadCache;
    QMutex m_preloadMutex;
    
    // 内部定时器
    QTimer* m_bufferTimer;
    QTimer* m_positionTimer;
    
    // 内部方法
    void initializeAudioComponents();
    void cleanupAudioComponents();
    void setupConnections();
    void disconnectConnections();
    
    // 命令处理
    void processCommandQueue();
    void executeCommand(const AudioCommand& command);
    
    // 播放控制实现
    void handlePlayCommand(const QString& filePath);
    void handlePauseCommand();
    void handleResumeCommand();
    void handleStopCommand();
    void handleSeekCommand(qint64 position);
    
    // 音量控制实现
    void handleVolumeCommand(int volume);
    void handleMutedCommand(bool muted);
    
    // 音效处理实现
    void handleEqualizerCommand(bool enabled, const QVector<double>& bands);
    void handleReverbCommand(bool enabled, double intensity);
    void handleBalanceCommand(double balance);
    void handleCrossfadeCommand(int duration);
    
    // 媒体加载
    QByteArray loadAudioFile(const QString& filePath);
    bool validateAudioFile(const QString& filePath);
    Song createSongFromFile(const QString& filePath);
    
    // 音频数据处理
    QByteArray processAudioData(const QByteArray& rawData);
    void bufferAudioData(const QByteArray& audioData);
    void updateAudioBuffer();
    
    // 预加载管理
    void preloadAudioFile(const QString& filePath);
    void clearPreloadCache();
    void managePreloadCache();
    
    // 错误处理
    void handleError(const QString& error);
    void logError(const QString& error);
    void logInfo(const QString& message);
    
    // 状态更新
    void updateThreadState(ThreadState newState);
    void updatePosition();
    void updateDuration();
    
    // 线程安全方法
    void addCommand(const AudioCommand& command);
    AudioCommand getNextCommand();
    bool hasCommands() const;
    
    // 音频格式支持
    bool isFormatSupported(const QString& filePath) const;
    QString getAudioFormat(const QString& filePath) const;
};

#endif // AUDIOWORKERTHREAD_H 
