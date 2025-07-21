#ifndef PLAYBACKCONTROLWIDGET_H
#define PLAYBACKCONTROLWIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFrame>
#include <QTimer>
#include "musicprogressbar.h"
#include "../../audio/AudioTypes.h"

QT_BEGIN_NAMESPACE
class AudioEngine;
QT_END_NAMESPACE

/**
 * @brief 通用播放控制组件
 * 
 * 该组件封装了播放控制的所有功能，包括：
 * - 播放/暂停按钮
 * - 上一曲/下一曲按钮
 * - 播放模式切换按钮
 * - 音乐进度条
 * - 音量控制滑块
 * 
 * 可以在主界面和音频可视化界面中复用，确保两个界面的同步
 */
class PlaybackControlWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PlaybackControlWidget(QWidget *parent = nullptr);
    ~PlaybackControlWidget();

    // 设置音频引擎（用于同步状态）
    void setAudioEngine(AudioEngine* audioEngine);
    
    // 播放控制
    void setPlaybackState(bool isPlaying);
    void setCurrentTime(qint64 time);
    void setTotalTime(qint64 time);
    void setVolume(int volume);
    void setMuted(bool muted);
    void setPlayMode(AudioTypes::PlayMode mode);
    
    // 获取当前状态
    bool isPlaying() const { return m_isPlaying; }
    qint64 currentTime() const { return m_currentTime; }
    qint64 totalTime() const { return m_totalTime; }
    int volume() const { return m_volume; }
    bool isMuted() const { return m_isMuted; }
    AudioTypes::PlayMode playMode() const { return m_playMode; }
    
    // 启用/禁用控件
    void setEnabled(bool enabled) override;
    void setProgressBarEnabled(bool enabled);
    void setVolumeControlEnabled(bool enabled);
    
    // 样式设置
    void setControlButtonStyle(const QString& style);
    void setProgressBarStyle(const QString& style);
    void setVolumeSliderStyle(const QString& style);
    
    // 布局模式
    enum LayoutMode {
        Horizontal,  // 水平布局（适合底部控制栏）
        Vertical,    // 垂直布局（适合侧边栏）
        Compact      // 紧凑布局（适合小空间）
    };
    void setLayoutMode(LayoutMode mode);
    
    // 显示/隐藏特定控件
    void setProgressBarVisible(bool visible);
    void setVolumeControlVisible(bool visible);
    void setPlayModeButtonVisible(bool visible);

signals:
    // 播放控制信号
    void playPauseClicked();
    void previousClicked();
    void nextClicked();
    void playModeClicked();
    
    // 进度控制信号
    void seekRequested(qint64 position);
    void positionChanged(qint64 position);
    
    // 音量控制信号
    void volumeChanged(int volume);
    void muteToggled(bool muted);

public slots:
    // 外部状态更新槽函数
    void updatePlaybackState(bool isPlaying);
    void updateCurrentTime(qint64 time);
    void updateTotalTime(qint64 time);
    void updateVolume(int volume);
    void updateMuted(bool muted);
    void updatePlayMode(AudioTypes::PlayMode mode);

private slots:
    // 内部控件信号处理
    void onPlayPauseButtonClicked();
    void onPreviousButtonClicked();
    void onNextButtonClicked();
    void onPlayModeButtonClicked();
    void onVolumeSliderChanged(int value);
    void onMuteButtonClicked();
    void onProgressBarSeekRequested(qint64 position);
    void onProgressBarPositionChanged(qint64 position);

private:
    // UI组件
    QHBoxLayout* m_mainLayout;
    QVBoxLayout* m_verticalLayout;
    
    // 播放控制按钮
    QPushButton* m_playPauseButton;
    QPushButton* m_previousButton;
    QPushButton* m_nextButton;
    QPushButton* m_playModeButton;
    
    // 进度控制
    MusicProgressBar* m_progressBar;
    
    // 音量控制
    QFrame* m_volumeFrame;
    QHBoxLayout* m_volumeLayout;
    QSlider* m_volumeSlider;
    QPushButton* m_muteButton;
    QLabel* m_volumeLabel;
    
    // 状态变量
    bool m_isPlaying;
    qint64 m_currentTime;
    qint64 m_totalTime;
    int m_volume;
    bool m_isMuted;
    AudioTypes::PlayMode m_playMode;
    LayoutMode m_layoutMode;
    
    // 音频引擎引用
    AudioEngine* m_audioEngine;
    
    // 样式字符串
    QString m_buttonStyle;
    QString m_progressStyle;
    QString m_volumeStyle;
    
    // 私有方法
    void setupUI();
    void setupConnections();
    void setupLayouts();
    void updatePlayPauseButton();
    void updatePlayModeButton();
    void updateVolumeButton();
    void updateVolumeLabel();
    void applyStyles();
    void rebuildLayout();
    
    // 图标和文本更新
    QString getPlayModeIcon(AudioTypes::PlayMode mode) const;
    QString getPlayModeTooltip(AudioTypes::PlayMode mode) const;
    QString getVolumeIcon(int volume, bool muted) const;
    QString formatTime(qint64 milliseconds) const;
};

#endif // PLAYBACKCONTROLWIDGET_H