#ifndef PLAYINTERFACECONTROLLER_H
#define PLAYINTERFACECONTROLLER_H

#include <QObject>
#include <QTimer>
#include <QMutex>
#include <QDebug>
#include "../../audio/audiotypes.h"
#include "../../models/song.h"
#include "../dialogs/PlayInterface.h"

class AudioEngine;
class TagManager;
class PlaylistManager;
class DatabaseManager;
class Logger;

enum class DisplayMode {
    Cover,
    Lyrics
};

enum class VisualizationType {
    Waveform,
    Spectrum,
    VUMeter,
    Oscilloscope,
    Spectrogram,
    None
};

enum class EqualizerBand {
    Band60Hz,
    Band230Hz,
    Band910Hz,
    Band3_6kHz,
    Band14kHz
};

class PlayInterfaceController : public QObject
{
    Q_OBJECT

public:
    // 静态常量
    static const int UPDATE_INTERVAL;  // 更新间隔（毫秒）

    explicit PlayInterfaceController(PlayInterface* interface, QObject* parent = nullptr);
    ~PlayInterfaceController();

    void initialize();
    void shutdown();
    void setAudioEngine(AudioEngine* audioEngine);
    AudioEngine* getAudioEngine() const;
    void syncWithMainWindow(qint64 position, qint64 duration, int volume, bool muted);
    void syncProgressBar(qint64 position, qint64 duration);
    void syncVolumeControls(int volume, bool muted);
    void setDisplayMode(DisplayMode mode);
    DisplayMode getDisplayMode() const;
    void setVisualizationType(VisualizationType type);
    VisualizationType getVisualizationType() const;
    bool isInitialized() const;
    Song getCurrentSong() const;
    void updatePlayModeButton(int playMode);
    void updateTimeDisplay();
    void updatePlaybackControls();
    void updateVolumeDisplay();
    void updateBalanceDisplay();

signals:
    void playPauseClicked();
    void nextClicked();
    void previousClicked();
    void volumeChanged(int volume);
    void balanceChanged(int value);
    void seekRequested(qint64 position);
    void equalizerChanged(EqualizerBand band, int value);
    void displayModeChanged(DisplayMode mode);
    void visualizationTypeChanged(VisualizationType type);
    void errorOccurred(const QString& error);
    void playModeClicked();
    void playRequested(const Song& song);
    void playModeChangeRequested();
    void nextRequested();
    void previousRequested();
    void muteToggled(bool muted);

private slots:
    // AudioEngine事件
    void onPlaybackStateChanged(AudioTypes::AudioState state);
    void onCurrentSongChanged(const Song& song);
    void onPositionChanged(qint64 position);
    void onDurationChanged(qint64 duration);
    void onVolumeChanged(int volume);
    void onMutedChanged(bool muted);

    // UI事件
    void onPlayPauseClicked();
    void onPlayModeClicked();
    void onNextClicked();
    void onPreviousClicked();
    void onVolumeSliderChanged(int value);
    void onBalanceSliderChanged(int value);
    void onPositionSliderChanged(int value);
    void onDisplayModeClicked();
    void onVisualizationTypeClicked();
    void onEqualizerSliderChanged(const QVector<int>& values);
    void onProgressSliderPressed();
    void onProgressSliderReleased();
    void onMuteButtonClicked();

    // 定时器事件
    void onUpdateTimer();

private:
    void setupConnections();
    void setCurrentSong(const Song& song);
    void handleError(const QString& error);
    void logInfo(const QString& message);
    void logError(const QString& error);
    void logDebug(const QString& message);
    void loadSongInfo(const Song& song);
    QString formatTime(qint64 milliseconds) const;

    // 界面相关
    PlayInterface* m_interface;
    bool m_isProgressBarDragging;  // 进度条是否正在拖动
    
    // 核心组件
    AudioEngine* m_audioEngine;
    TagManager* m_tagManager;
    PlaylistManager* m_playlistManager;
    DatabaseManager* m_databaseManager;
    Logger* m_logger;
    QTimer* m_updateTimer;
    
    // 初始化标志
    bool m_initialized;
    
    // 当前状态
    Song m_currentSong;
    bool m_isPlaying;
    bool m_isPaused;
    bool m_isMuted;
    qint64 m_currentTime;
    qint64 m_totalTime;
    int m_volume;
    int m_balance;
    
    // 显示相关
    DisplayMode m_displayMode;
    VisualizationType m_visualizationType;
    
    // 均衡器
    QVector<int> m_equalizerValues;
    QString m_equalizerPreset;
};

#endif // PLAYINTERFACECONTROLLER_H