#ifndef PLAYINTERFACE_H
#define PLAYINTERFACE_H

#include <QDialog>
#include <QTimer>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QVector>
#include <QPixmap>
#include <QHBoxLayout>
#include "../../audio/audiotypes.h"

namespace Ui {
class PlayInterface;
}

class PlayInterfaceController;
class MusicProgressBar;
class AudioEngine;

class PlayInterface : public QDialog
{
    Q_OBJECT

public:
    explicit PlayInterface(QWidget *parent = nullptr);
    ~PlayInterface();

    // 播放状态控制
    void setPlaybackState(bool isPlaying);
    void setCurrentTime(qint64 time);
    void setTotalTime(qint64 time);
    void setVolume(int volume);
    void setBalance(int balance);
    void setMuted(bool muted);
    
    // 歌曲信息显示
    void setSongTitle(const QString& title);
    void setSongArtist(const QString& artist);
    void setSongAlbum(const QString& album);
    void setSongCover(const QPixmap& cover);
    void setLyrics(const QString& lyrics);
    
    // 进度条控制
    void setProgressBarPosition(qint64 position);
    void setProgressBarDuration(qint64 duration);
    void updateProgressDisplay();
    
    // 音量控制
    void updateVolumeControls();
    void setVolumeSliderValue(int value);
    void updateVolumeLabel(int value);
    void updateMuteButtonIcon();
    
    // 播放模式和显示控制
    void updatePlayModeButton(const QString& text, const QString& iconPath, const QString& tooltip);
    void setDisplayMode(int mode);
    void updatePlayModeDisplay(AudioTypes::PlayMode mode); // 新增：播放模式显示更新
    
    // 可视化相关
    void updateWaveform(const QVector<float>& data);
    void updateSpectrum(const QVector<float>& data);
    void updateVUMeter(float leftLevel, float rightLevel);
    
    // 均衡器相关
    void setEqualizerValues(const QVector<int>& values);
    QVector<int> getEqualizerValues() const;
    
    // 状态查询
    bool isMuted() const { return m_isMuted; }
    
    // 控制器访问
    PlayInterfaceController* getController() const { return m_controller; }
    
    // AudioEngine设置 - 新增
    void setAudioEngine(AudioEngine* audioEngine);
    
    // 事件处理
    void showEvent(QShowEvent* event) override;

signals:
    void playPauseClicked();
    void playModeClicked();
    void previousClicked();
    void nextClicked();
    void volumeChanged(int volume);
    void balanceChanged(int balance);
    void positionChanged(qint64 position);
    void muteToggled(bool muted);
    void displayModeClicked();
    void visualizationTypeClicked();
    void equalizerChanged(const QVector<int>& values);
    void lyricClicked(qint64 timestamp);
    
    // 进度条和音量控制信号
    void seekRequested(qint64 position);
    void progressSliderPressed();
    void progressSliderReleased();
    void volumeSliderChanged(int value);
    void muteButtonClicked();

private slots:
    void onPlayPauseClicked();
    void onPlayModeClicked();
    void onPreviousClicked();
    void onNextClicked();
    void onVolumeSliderChanged(int value);
    void onBalanceSliderChanged(int value);
    void onPositionSliderChanged(int value);
    void onMuteButtonClicked();
    void onDisplayModeClicked();
    void onVisualizationTypeClicked();
    void onEqualizerSliderChanged();
    void onLyricClicked(qint64 timestamp);
    void onUpdateTimer();
    
    // 进度条和音量控制槽函数
    void onProgressSliderPressed();
    void onProgressSliderReleased();
    void onProgressSliderMoved(int value);
    void onVolumeSliderValueChanged(int value);
    void onMuteButtonPressed();

private:
    void setupConnections();
    void setupUI();
    void setupVisualization();
    void setupProgressBar(); // 新增：设置自定义进度条
    void updatePlaybackControls();
    void updateTimeDisplay();
    void updateVolumeDisplay();
    void updateMuteButtonState();
    void updateDisplayMode();
    void updateVisualization();
    void updateEqualizerDisplay();
    void updateLyricDisplay();
    QString formatTime(qint64 milliseconds) const;

private:
    Ui::PlayInterface *ui;
    PlayInterfaceController *m_controller;
    QTimer *m_updateTimer;
    
    // 自定义进度条组件 - 新增
    MusicProgressBar* m_customProgressBar;
    AudioEngine* m_audioEngine; // 新增：AudioEngine引用
    
    // 可视化组件
    QGraphicsView *m_waveformView;
    QGraphicsView *m_spectrumView;
    QGraphicsScene *m_waveformScene;
    QGraphicsScene *m_spectrumScene;
    
    // 播放状态
    bool m_isPlaying;
    qint64 m_currentTime;
    qint64 m_totalTime;
    int m_volume;
    int m_balance;
    bool m_isMuted;
    int m_displayMode;
    
    // 均衡器值
    QVector<int> m_equalizerValues;
    
    // 歌词相关
    int m_currentLyricIndex;
};

#endif // PLAYINTERFACE_H