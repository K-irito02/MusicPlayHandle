#ifndef PLAYINTERFACECONTROLLER_H
#define PLAYINTERFACECONTROLLER_H

#include <QObject>
#include <QTimer>
#include <QPixmap>
#include <QVector>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QGraphicsTextItem>
#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsLinearLayout>
#include <QPainter>
#include <QBrush>
#include <QPen>
#include <QColor>
#include <QFont>
#include <QFontMetrics>
#include <QTextDocument>
#include <QTextCursor>
#include <QScrollBar>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QParallelAnimationGroup>
#include <QSequentialAnimationGroup>
#include <QMutex>
#include <QSettings>

// 前向声明
class PlayInterface;
class AudioEngine;
class TagManager;
class PlaylistManager;
class DatabaseManager;
class Logger;

#include "../../models/song.h"
#include "../../models/playlist.h"
#include "../../audio/audiotypes.h"
#include "../../core/logger.h"

// 显示模式枚举
enum class DisplayMode {
    Lyrics,
    Cover,
    Visualization,
    Mixed
};

// 可视化类型枚举
enum class VisualizationType {
    Waveform,
    Spectrum,
    VUMeter,
    Oscilloscope,
    Bars,
    Particles
};

// 均衡器频段枚举
enum class EqualizerBand {
    Band32Hz,
    Band64Hz,
    Band125Hz,
    Band250Hz,
    Band500Hz,
    Band1kHz,
    Band2kHz,
    Band4kHz,
    Band8kHz,
    Band16kHz
};

// 歌词行结构
struct LyricLine {
    qint64 timestamp;
    QString text;
    QString translation;
    bool isHighlighted;
    
    LyricLine() : timestamp(0), isHighlighted(false) {}
};

// 可视化数据结构
struct VisualizationData {
    QVector<float> waveform;
    QVector<float> spectrum;
    float leftLevel;
    float rightLevel;
    float peakLeft;
    float peakRight;
    qint64 timestamp;
    
    VisualizationData() : leftLevel(0.0f), rightLevel(0.0f), peakLeft(0.0f), peakRight(0.0f), timestamp(0) {}
};

class PlayInterfaceController : public QObject
{
    Q_OBJECT

public:
    explicit PlayInterfaceController(PlayInterface* interface, QObject* parent = nullptr);
    ~PlayInterfaceController();

    // 初始化和清理
    bool initialize();
    void shutdown();
    bool isInitialized() const;

    // 播放控制
    void play();
    void pause();
    void stop();
    void next();
    void previous();
    void seek(qint64 position);
    void setVolume(int volume);
    void setBalance(int balance);
    void setMuted(bool muted);
    
    // 歌曲信息
    void setCurrentSong(const Song& song);
    Song getCurrentSong() const;
    void loadSongInfo(const Song& song);
    void loadSongLyrics(const Song& song);
    void loadSongCover(const Song& song);
    
    // 显示模式
    void setDisplayMode(DisplayMode mode);
    DisplayMode getDisplayMode() const;
    void toggleDisplayMode();
    
    // 可视化
    void setVisualizationType(VisualizationType type);
    VisualizationType getVisualizationType() const;
    void updateVisualization(const VisualizationData& data);
    void enableVisualization(bool enabled);
    
    // 均衡器
    void setEqualizerValue(EqualizerBand band, int value);
    int getEqualizerValue(EqualizerBand band) const;
    void setEqualizerPreset(const QString& preset);
    QString getEqualizerPreset() const;
    void resetEqualizer();
    
    // 歌词
    void setLyrics(const QList<LyricLine>& lyrics);
    QList<LyricLine> getLyrics() const;
    void updateLyricHighlight(qint64 currentTime);
    void scrollToCurrentLyric();
    
    // 动画效果
    void startSpinAnimation();
    void stopSpinAnimation();
    void startPulseAnimation();
    void stopPulseAnimation();
    void fadeIn();
    void fadeOut();

signals:
    // 播放控制信号
    void playRequested();
    void pauseRequested();
    void stopRequested();
    void nextRequested();
    void previousRequested();
    void seekRequested(qint64 position);
    void volumeChanged(int volume);
    void balanceChanged(int balance);
    void muteToggled(bool muted);
    
    // 显示模式信号
    void displayModeChanged(DisplayMode mode);
    void visualizationTypeChanged(VisualizationType type);
    
    // 均衡器信号
    void equalizerChanged(EqualizerBand band, int value);
    void equalizerPresetChanged(const QString& preset);
    
    // 歌词信号
    void lyricClicked(qint64 timestamp);
    void lyricScrolled(int position);
    
    // 错误信号
    void errorOccurred(const QString& error);

public slots:
    // 播放状态槽
    void onPlaybackStateChanged(AudioTypes::AudioState state);
    void onCurrentSongChanged(const Song& song);
    void onPositionChanged(qint64 position);
    void onDurationChanged(qint64 duration);
    void onVolumeChanged(int volume);
    void onMutedChanged(bool muted);
    
    // 音频数据槽
    void onAudioDataAvailable(const VisualizationData& data);
    void onSpectrumDataAvailable(const QVector<float>& spectrum);
    void onLevelDataAvailable(float leftLevel, float rightLevel);
    
    // UI事件槽
    void onPlayPauseClicked();
    void onStopClicked();
    void onNextClicked();
    void onPreviousClicked();
    void onVolumeSliderChanged(int value);
    void onBalanceSliderChanged(int value);
    void onPositionSliderChanged(int value);
    void onMuteButtonClicked();
    void onDisplayModeClicked();
    void onVisualizationTypeClicked();
    void onEqualizerSliderChanged(int value);
    void onLyricClicked(qint64 timestamp);

private slots:
    void onUpdateTimer();
    void onVisualizationTimer();
    void onLyricUpdateTimer();
    void onAnimationTimer();
    void onCoverLoadFinished();
    void onLyricLoadFinished();

private:
    PlayInterface* m_interface;
    AudioEngine* m_audioEngine;
    TagManager* m_tagManager;
    PlaylistManager* m_playlistManager;
    DatabaseManager* m_databaseManager;
    Logger* m_logger;
    
    // 当前状态
    Song m_currentSong;
    DisplayMode m_displayMode;
    VisualizationType m_visualizationType;
    bool m_initialized;
    bool m_isPlaying;
    bool m_isPaused;
    bool m_isMuted;
    qint64 m_currentTime;
    qint64 m_totalTime;
    int m_volume;
    int m_balance;
    
    // 可视化数据
    VisualizationData m_visualizationData;
    QVector<float> m_waveformHistory;
    QVector<float> m_spectrumHistory;
    bool m_visualizationEnabled;
    
    // 均衡器
    QVector<int> m_equalizerValues;
    QString m_equalizerPreset;
    
    // 歌词
    QList<LyricLine> m_lyrics;
    int m_currentLyricIndex;
    QTextDocument* m_lyricDocument;
    
    // 图形对象
    QGraphicsScene* m_waveformScene;
    QGraphicsScene* m_spectrumScene;
    QGraphicsScene* m_coverScene;
    QGraphicsScene* m_lyricScene;
    QGraphicsPixmapItem* m_coverItem;
    QGraphicsTextItem* m_lyricItem;
    
    // 动画
    QPropertyAnimation* m_spinAnimation;
    QPropertyAnimation* m_pulseAnimation;
    QPropertyAnimation* m_fadeAnimation;
    QParallelAnimationGroup* m_animationGroup;
    
    // 定时器
    QTimer* m_updateTimer;
    QTimer* m_visualizationTimer;
    QTimer* m_lyricUpdateTimer;
    QTimer* m_animationTimer;
    
    // 线程安全
    QMutex m_mutex;
    
    // 设置
    QSettings* m_settings;
    
    // 内部方法
    void setupConnections();
    void setupVisualization();
    void setupAnimations();
    void loadSettings();
    void saveSettings();
    
    // 可视化绘制
    void drawWaveform(const QVector<float>& data);
    void drawSpectrum(const QVector<float>& data);
    void drawVUMeter(float leftLevel, float rightLevel);
    void drawOscilloscope(const QVector<float>& data);
    void drawBars(const QVector<float>& data);
    void drawParticles(const QVector<float>& data);
    
    // 歌词处理
    void parseLyrics(const QString& lyricText);
    void updateLyricDisplay();
    void highlightCurrentLyric();
    void scrollToLyric(int index);
    
    // 封面处理
    void loadCoverImage(const QString& coverPath);
    void setCoverImage(const QPixmap& cover);
    void startCoverAnimation();
    void stopCoverAnimation();
    
    // UI更新
    void updateTimeDisplay();
    void updateVolumeDisplay();
    void updateBalanceDisplay();
    void updatePlaybackControls();
    void updateSongInfo();
    void updateEqualizerDisplay();
    void updateVisualizationDisplay();
    
    // 工具方法
    QString formatTime(qint64 milliseconds) const;
    QColor getSpectrumColor(int band) const;
    QColor getVUMeterColor(float level) const;
    float calculateRMS(const QVector<float>& data) const;
    float calculatePeak(const QVector<float>& data) const;
    QPixmap createDefaultCover() const;
    QString findLyricFile(const Song& song) const;
    QString findCoverFile(const Song& song) const;
    
    // 错误处理
    void handleError(const QString& error);
    void logInfo(const QString& message);
    void logError(const QString& error);
    void logDebug(const QString& message);
    
    // 常量
    static const int UPDATE_INTERVAL = 50; // ms
    static const int VISUALIZATION_UPDATE_INTERVAL = 16; // ms (~60fps)
    static const int LYRIC_UPDATE_INTERVAL = 100; // ms
    static const int ANIMATION_UPDATE_INTERVAL = 16; // ms
    static const int SPECTRUM_BANDS = 32;
    static const int WAVEFORM_SAMPLES = 1024;
    static const int EQUALIZER_BANDS = 10;
    static constexpr float VU_METER_DECAY = 0.95f;
    static constexpr float PEAK_HOLD_TIME = 1000.0f; // ms
};

#endif // PLAYINTERFACECONTROLLER_H 