#ifndef IMPROVEDAUDIOENGINE_H
#define IMPROVEDAUDIOENGINE_H

#include <QObject>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QTimer>
#include <QMutex>
#include <QThread>
#include <QElapsedTimer>
#include <memory>

#include "audiotypes.h"
#include "ffmpegdecoder.h"
#include "../models/song.h"
#include "../core/observer.h"
#include "../core/resourcemanager.h"
#include "../core/performancemanager.h"
#include "../threading/audioworkerthread.h"

// 前向声明
class Logger;
class PlayHistoryDao;

/**
 * @brief 改进的音频引擎
 * 使用观察者模式替代单例模式，支持依赖注入和资源管理
 */
class ImprovedAudioEngine : public QObject, 
                           public AudioStateSubject,
                           public AudioVolumeSubject, 
                           public AudioSongSubject,
                           public AudioPlaylistSubject
{
    Q_OBJECT

public:
    /**
     * @brief 音频引擎配置
     */
    struct AudioEngineConfig {
        AudioTypes::AudioEngineType engineType = AudioTypes::AudioEngineType::QMediaPlayer;
        bool enablePerformanceMonitoring = true;
        bool enableResourceLocking = true;
        bool enableAdaptiveDecoding = true;
        int maxHistorySize = 100;
        QString lockId = "DefaultAudioEngine";
        QString ownerName = "ImprovedAudioEngine";
        
        // 性能参数
        double targetCpuUsage = 30.0;
        double maxResponseTime = 16.0; // 16ms for 60fps
        PerformanceManager::PerformanceProfile performanceProfile = 
            PerformanceManager::PerformanceProfile::Balanced;
    };

    explicit ImprovedAudioEngine(const AudioEngineConfig& config, QObject* parent = nullptr);
    ~ImprovedAudioEngine();

    // 禁用拷贝构造和赋值
    ImprovedAudioEngine(const ImprovedAudioEngine&) = delete;
    ImprovedAudioEngine& operator=(const ImprovedAudioEngine&) = delete;

    // 基本播放控制
    bool play();
    bool pause();
    bool stop();
    bool seek(qint64 position);

    // 音量控制
    bool setVolume(int volume);
    bool setMuted(bool muted);
    bool toggleMute();
    int volume() const;
    bool isMuted() const;

    // 播放列表管理
    bool setPlaylist(const QList<Song>& songs);
    bool setCurrentSong(const Song& song);
    bool setCurrentIndex(int index);
    bool playNext();
    bool playPrevious();

    // 播放模式
    bool setPlayMode(AudioTypes::PlayMode mode);
    AudioTypes::PlayMode playMode() const;

    // 音效控制
    bool setEqualizerEnabled(bool enabled);
    bool setEqualizerBands(const QVector<double>& bands);
    bool setBalance(double balance);
    double getBalance() const;
    bool setSpeed(double speed);

    // 音频引擎类型控制
    bool setAudioEngineType(AudioTypes::AudioEngineType type);
    AudioTypes::AudioEngineType getAudioEngineType() const;
    QString getAudioEngineTypeString() const;

    // VU表控制
    bool setVUEnabled(bool enabled);
    bool isVUEnabled() const;
    QVector<double> getVULevels() const;

    // 状态获取
    AudioTypes::AudioState state() const;
    qint64 position() const;
    qint64 duration() const;
    Song currentSong() const;
    int currentIndex() const;
    QList<Song> playlist() const;

    // 有效性检查
    bool isValid() const;
    bool isInitialized() const;

    // 音频格式支持
    bool isFormatSupported(const QString& filePath) const;
    static QStringList supportedFormats();

    // 播放历史
    bool addToHistory(const Song& song);
    QList<Song> playHistory() const;
    bool clearHistory();

    // 性能监控
    PerformanceManager* getPerformanceManager() const;
    ResourceManager& getResourceManager() const;

    // 配置管理
    AudioEngineConfig getConfig() const;
    bool updateConfig(const AudioEngineConfig& newConfig);

    // 状态检查
    bool isResourceLocked() const;
    QString getResourceLockOwner() const;

signals:
    // 基本状态信号（保持兼容性）
    void stateChanged(AudioTypes::AudioState state);
    void positionChanged(qint64 position);
    void durationChanged(qint64 duration);
    void volumeChanged(int volume);
    void mutedChanged(bool muted);
    void currentSongChanged(const Song& song);
    void currentIndexChanged(int index);
    void playlistChanged(const QList<Song>& songs);
    void playModeChanged(AudioTypes::PlayMode mode);
    void errorOccurred(const QString& error);

    // 扩展信号
    void audioEngineTypeChanged(AudioTypes::AudioEngineType type);
    void balanceChanged(double balance);
    void speedChanged(double speed);
    void equalizerChanged(bool enabled, const QVector<double>& bands);
    void vuLevelsChanged(const QVector<double>& levels);
    void vuEnabledChanged(bool enabled);

    // 性能和资源信号
    void performanceWarning(const QString& message);
    void resourceLockAcquired();
    void resourceLockReleased();
    void resourceLockFailed(const QString& reason);

    // 缓冲和错误信号
    void bufferProgressChanged(int progress);
    void bufferStatusChanged(AudioTypes::BufferStatus status);
    void mediaStatusChanged(QMediaPlayer::MediaStatus status);

private slots:
    // 播放器事件处理
    void onMediaPlayerStateChanged(QMediaPlayer::PlaybackState state);
    void onMediaPlayerPositionChanged(qint64 position);
    void onMediaPlayerDurationChanged(qint64 duration);
    void onMediaPlayerMediaStatusChanged(QMediaPlayer::MediaStatus status);
    void onMediaPlayerErrorOccurred(QMediaPlayer::Error error);

    // FFmpeg解码器事件处理
    void onFFmpegAudioDataReady(const QVector<double>& levels);
    void onFFmpegPositionChanged(qint64 position);
    void onFFmpegDurationChanged(qint64 duration);
    void onFFmpegDecodingFinished();
    void onFFmpegErrorOccurred(const QString& error);

    // 性能监控事件
    void onPerformanceUpdated(double cpuUsage, qint64 memoryUsage, double responseTime);
    void onDecodeIntervalChanged(int newInterval, int oldInterval);

    // 资源管理事件
    void onResourceLockConflict(const QString& lockId, const QString& requester, const QString& currentOwner);

    // 定时器事件
    void updatePosition();
    void updateVULevels();
    void syncObservers();

private:
    // 初始化和清理
    bool initialize();
    void cleanup();
    bool initializeAudioComponents();
    void cleanupAudioComponents();
    bool initializeFFmpegDecoder();
    void cleanupFFmpegDecoder();
    bool setupConnections();
    void disconnectConnections();

    // 播放实现
    bool playWithQMediaPlayer(const Song& song);
    bool playWithFFmpeg(const Song& song);
    bool loadMedia(const QString& filePath);

    // 音效处理
    void applyAudioEffects();
    void updateBalance();
    void updateSpeed();
    void processAudioFrame(const QByteArray& audioData);

    // 状态管理
    void updateCurrentSong();
    void handlePlaybackFinished();
    int getNextIndex();
    int getPreviousIndex();
    void shufflePlaylist();

    // 观察者模式事件发布
    void publishStateChanged();
    void publishVolumeChanged();
    void publishSongChanged();
    void publishPlaylistChanged();

    // 资源管理
    bool acquireAudioLock();
    void releaseAudioLock();
    bool validateResourceAccess() const;

    // 性能优化
    void optimizeForPerformance();
    void adjustDecodingParameters();

    // 错误处理
    void handleError(const QString& error);
    void logError(const QString& error);
    void logInfo(const QString& message);

    // 格式检查
    bool checkAudioFormat(const QString& filePath) const;
    QString getFileExtension(const QString& filePath) const;

    // 状态转换
    AudioTypes::AudioState convertMediaState(QMediaPlayer::PlaybackState state) const;

private:
    // 配置
    AudioEngineConfig m_config;
    bool m_isInitialized;

    // 核心音频组件
    QMediaPlayer* m_player;
    QAudioOutput* m_audioOutput;
    AudioWorkerThread* m_audioWorker;
    FFmpegDecoder* m_ffmpegDecoder;

    // 播放状态
    AudioTypes::AudioState m_state;
    qint64 m_position;
    qint64 m_duration;
    int m_volume;
    bool m_muted;
    bool m_userPaused;

    // 播放列表
    QList<Song> m_playlist;
    int m_currentIndex;
    AudioTypes::PlayMode m_playMode;

    // 音效设置
    bool m_equalizerEnabled;
    QVector<double> m_equalizerBands;
    double m_balance;
    double m_speed;

    // VU表相关
    bool m_vuEnabled;
    QVector<double> m_vuLevels;
    QTimer* m_vuTimer;

    // 播放历史
    QList<Song> m_playHistory;
    PlayHistoryDao* m_playHistoryDao;

    // 定时器
    QTimer* m_positionTimer;
    QTimer* m_observerSyncTimer;

    // 资源管理
    std::unique_ptr<ScopedAudioLock> m_resourceLock;
    ResourceManager& m_resourceManager;
    
    // 性能管理
    std::unique_ptr<PerformanceManager> m_performanceManager;
    std::unique_ptr<AdaptiveDecodeController> m_adaptiveController;

    // 线程安全
    mutable QMutex m_stateMutex;
    mutable QMutex m_playlistMutex;
    mutable QMutex m_effectsMutex;

    // 支持的音频格式
    static QStringList m_supportedFormats;

    // 性能计时
    QElapsedTimer m_operationTimer;
    
    // 错误计数
    QAtomicInt m_errorCount;
    static const int MAX_ERROR_COUNT = 10;
};

/**
 * @brief 音频引擎工厂
 * 提供统一的音频引擎创建接口
 */
class AudioEngineFactory {
public:
    static std::unique_ptr<ImprovedAudioEngine> createEngine(
        const ImprovedAudioEngine::AudioEngineConfig& config);
    
    static std::unique_ptr<ImprovedAudioEngine> createDefaultEngine(
        const QString& ownerName = "DefaultOwner");
    
    static std::unique_ptr<ImprovedAudioEngine> createPerformanceOptimizedEngine(
        const QString& ownerName = "PerformanceOwner");
    
    static std::unique_ptr<ImprovedAudioEngine> createPowerSaverEngine(
        const QString& ownerName = "PowerSaverOwner");

private:
    static ImprovedAudioEngine::AudioEngineConfig getDefaultConfig();
    static ImprovedAudioEngine::AudioEngineConfig getPerformanceConfig();
    static ImprovedAudioEngine::AudioEngineConfig getPowerSaverConfig();
};

#endif // IMPROVEDAUDIOENGINE_H