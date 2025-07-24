#ifndef AUDIOENGINE_H
#define AUDIOENGINE_H

#include <QObject>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QAudioDevice>
#include <QAudioSink>
#include <QAudioBuffer>
#include <QUrl>
#include <QTimer>
#include <QMutex>
#include <QList>
#include <QRecursiveMutex>
#include <QVector>
#include "ffmpegdecoder.h"

#include "../models/song.h"
#include "audiotypes.h"
#include "../threading/audioworkerthread.h"

// 前向声明
class Logger;
class PlayHistoryDao;

// 音频引擎类 - 单例模式
class AudioEngine : public QObject {
    Q_OBJECT
    
public:
    static AudioEngine* instance();
    static void cleanup();
    
    // 播放控制
    void play();
    void pause();
    void stop();
    void seek(qint64 position);
    void setPosition(qint64 position);
    
    // 音量控制
    void setVolume(int volume);
    void setMuted(bool muted);
    void toggleMute();
    int volume() const;
    bool isMuted() const;
    
    // 播放列表管理
    void setPlaylist(const QList<Song>& songs);
    void setCurrentSong(const Song& song);
    void setCurrentIndex(int index);
    void playNext();
    void playPrevious();
    
    // 播放模式
    void setPlayMode(AudioTypes::PlayMode mode);
    AudioTypes::PlayMode playMode() const;
    
    // 音效控制
    void setEqualizerEnabled(bool enabled);
    void setEqualizerBands(const QVector<double>& bands);
    void setBalance(double balance);
    double getBalance() const;
    void setSpeed(double speed);
    
    // 音频引擎类型控制
    void setAudioEngineType(AudioTypes::AudioEngineType type);
    AudioTypes::AudioEngineType getAudioEngineType() const;
    QString getAudioEngineTypeString() const;
    
    // VU表控制
    void setVUEnabled(bool enabled);
    bool isVUEnabled() const;
    QVector<double> getVULevels() const;
    
    // 平衡控制持久化
    void saveBalanceSettings();
    void loadBalanceSettings();
    
    // 状态获取
    AudioTypes::AudioState state() const;
    qint64 position() const;
    qint64 duration() const;
    Song currentSong() const;
    int currentIndex() const;
    QList<Song> playlist() const;
    
    // 音频格式支持
    bool isFormatSupported(const QString& filePath) const;
    static QStringList supportedFormats();
    
    // 播放历史
    void addToHistory(const Song& song);
    QList<Song> playHistory() const;
    void clearHistory();
    
    // 测试方法
    void testAudioSystem();
    
    // 调试方法
    void debugAudioState() const;
    QString getStateString() const;
    
signals:
    // 播放状态信号
    void stateChanged(AudioTypes::AudioState state);
    void positionChanged(qint64 position);
    void durationChanged(qint64 duration);
    void volumeChanged(int volume);
    void mutedChanged(bool muted);
    void playbackStateChanged(int state);
    
    // 播放列表信号
    void currentSongChanged(const Song& song);
    void currentIndexChanged(int index);
    void playlistChanged(const QList<Song>& songs);
    void playModeChanged(AudioTypes::PlayMode mode);
    
    // 错误信号
    void errorOccurred(const QString& error);
    void mediaStatusChanged(QMediaPlayer::MediaStatus status);
    
    // 缓冲信号
    void bufferProgressChanged(int progress);
    void bufferStatusChanged(AudioTypes::BufferStatus status);
    
    // 音效信号
    void equalizerChanged(bool enabled, const QVector<double>& bands);
    void balanceChanged(double balance);
    void speedChanged(double speed);
    
    // 音频引擎类型信号
    void audioEngineTypeChanged(AudioTypes::AudioEngineType type);
    
    // VU表信号
    void vuLevelsChanged(const QVector<double>& levels);
    void vuEnabledChanged(bool enabled);
    
private slots:
    void handlePlaybackStateChanged(QMediaPlayer::PlaybackState state);
    void onPositionChanged(qint64 position);
    void onDurationChanged(qint64 duration);
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);
    void onErrorOccurred(QMediaPlayer::Error error);
    void onBufferProgressChanged(int progress);
    void onPlaybackFinished();
    void updatePlaybackPosition();
    
private:
    explicit AudioEngine(QObject* parent = nullptr);
    ~AudioEngine();
    
    // 禁用拷贝构造和赋值操作
    AudioEngine(const AudioEngine&) = delete;
    AudioEngine& operator=(const AudioEngine&) = delete;
    
    // 单例实例
    static AudioEngine* m_instance;
    static QMutex m_instanceMutex;
    
    // 核心音频组件
    QMediaPlayer* m_player = nullptr;
    qreal m_currentVolume = 50.0;
    QAudioOutput* m_audioOutput;
    AudioWorkerThread* m_audioWorker;
    
    // 播放状态
    AudioTypes::AudioState m_state;
    qint64 m_position;
    qint64 m_duration;
    int m_volume;
    bool m_muted;
    bool m_userPaused; // 用户主动暂停标志
    
    // 播放列表
    QList<Song> m_playlist;
    int m_currentIndex;
    AudioTypes::PlayMode m_playMode;
    
    // 音效设置
    bool m_equalizerEnabled;
    QVector<double> m_equalizerBands;
    double m_balance;
    double m_speed;
    
    // 音频引擎类型
    AudioTypes::AudioEngineType m_audioEngineType;
    
    // 播放历史
    QList<Song> m_playHistory;
    int m_maxHistorySize;
    
    // 播放历史数据访问对象
    PlayHistoryDao* m_playHistoryDao;
    
    // 内部定时器
    QTimer* m_positionTimer;
    QTimer* m_bufferTimer;
    
    // VU表相关
    bool m_vuEnabled;
    QVector<double> m_vuLevels;
    QTimer* m_vuTimer;
    
    // FFmpeg解码器
    FFmpegDecoder* m_ffmpegDecoder;
    
    // 音频数据相关
    QVector<double> m_realTimeLevels;
    
    // 线程安全
    mutable QRecursiveMutex m_mutex;
    
    // 支持的音频格式
    static QStringList m_supportedFormats;
    
    // 内部方法
    void initializeAudio();
    void cleanupAudio();
    void connectSignals();
    void disconnectSignals();
    
    // 播放方式实现
    void playWithQMediaPlayer(const Song& song);
    void playWithFFmpeg(const Song& song);
    
    void loadMedia(const QString& filePath);
    void updateCurrentSong();
    void handlePlaybackFinished();
    void shufflePlaylist();
    int getNextIndex();
    int getPreviousIndex();
    
    // 音效处理
    void applyAudioEffects();
    void updateBalance();
    void updateSpeed();
    void updateVULevels();
    void processAudioFrame(const QByteArray& audioData);
    
    // FFmpeg解码器管理
    void initializeFFmpegDecoder();
    void cleanupFFmpegDecoder();
    void setupFFmpegConnections();
    
    // 音频数据处理
    void onFFmpegAudioDataReady(const QVector<double>& levels);
    void onFFmpegPositionChanged(qint64 position);
    void onFFmpegDurationChanged(qint64 duration);
    void onFFmpegDecodingFinished();
    void onFFmpegErrorOccurred(const QString& error);
    
    // 日志记录
    void logPlaybackEvent(const QString& event, const QString& details = QString());
    void logError(const QString& error);
    
    // 格式检查
    bool checkAudioFormat(const QString& filePath) const;
    QString getFileExtension(const QString& filePath) const;
    
    // 状态转换
    AudioTypes::AudioState convertMediaState(QMediaPlayer::PlaybackState state) const;
};

// 音频工具类
class AudioUtils {
public:
    // 时间格式化
    static QString formatTime(qint64 milliseconds);
    static QString formatDuration(qint64 duration);
    
    // 音量转换
    static int linearToDb(int linear);
    static int dbToLinear(int db);
    
    // 文件信息
    static bool isAudioFile(const QString& filePath);
    static QString getAudioFormat(const QString& filePath);
    static qint64 getAudioDuration(const QString& filePath);
    
    // 音效计算
    static QVector<double> calculateSpectrum(const QByteArray& audioData);
    static double calculateRMS(const QByteArray& audioData);
    static double calculatePeak(const QByteArray& audioData);
};

#endif // AUDIOENGINE_H