#ifndef IMPROVEDPLAYINTERFACE_H
#define IMPROVEDPLAYINTERFACE_H

#include <QDialog>
#include <QTimer>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QVector>
#include <QPixmap>
#include <QElapsedTimer>
#include <QLabel>
#include <QHBoxLayout>
#include <memory>

#include "../../audio/audiotypes.h"
#include "../../core/observer.h"
#include "../../core/resourcemanager.h"

namespace Ui {
class PlayInterface;
}

// 前向声明
class PlayInterfaceController;
class MusicProgressBar;
class ImprovedAudioEngine;
class PerformanceManager;

/**
 * @brief 改进的播放界面
 * 使用观察者模式，支持资源管理和性能监控
 */
class ImprovedPlayInterface : public QDialog,
                             public AudioStateObserver,
                             public AudioVolumeObserver, 
                             public AudioSongObserver,
                             public AudioPlaylistObserver,
                             public AudioPerformanceObserver
{
    Q_OBJECT

public:
    // 资源锁状态
    bool isResourceLocked() const { return m_isResourceLocked; }
    bool isResourceLockAcquired() const { return m_resourceLock != nullptr; }

    /**
     * @brief 界面配置
     */
    struct InterfaceConfig {
        QString interfaceName;
        bool enablePerformanceMonitoring;
        bool enableResourceLocking;
        bool enableVUMeter;
        bool enableVisualization;
        int updateInterval;
        int performanceUpdateInterval;
        
        InterfaceConfig() : 
            interfaceName("ImprovedPlayInterface"),
            enablePerformanceMonitoring(true),
            enableResourceLocking(true),
            enableVUMeter(true),
            enableVisualization(true),
            updateInterval(50), // 20fps更新
            performanceUpdateInterval(1000) // 1秒性能更新
        {}
    };

    explicit ImprovedPlayInterface(QWidget *parent = nullptr, 
                                   const InterfaceConfig& config = InterfaceConfig());
    ~ImprovedPlayInterface();

    // 音频引擎设置
    void setAudioEngine(std::shared_ptr<ImprovedAudioEngine> audioEngine);
    std::shared_ptr<ImprovedAudioEngine> getAudioEngine() const;

    // 观察者模式接口实现
    void onNotify(const AudioEvents::StateChanged& event) override;
    void onNotify(const AudioEvents::VolumeChanged& event) override;
    void onNotify(const AudioEvents::SongChanged& event) override;
    void onNotify(const AudioEvents::PlaylistChanged& event) override;
    void onNotify(const AudioEvents::PerformanceInfo& event) override;

    QString getObserverName() const override;

    // 界面状态控制
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
    void updatePlayModeDisplay(AudioTypes::PlayMode mode);

    // 可视化相关
    void updateWaveform(const QVector<float>& data);
    void updateSpectrum(const QVector<float>& data);
    void updateVUMeter(float leftLevel, float rightLevel);
    void updateVUMeterLevels(const QVector<double>& levels);

    // 均衡器相关
    void setEqualizerValues(const QVector<int>& values);
    QVector<int> getEqualizerValues() const;

    // 性能监控显示
    void updatePerformanceInfo(const AudioEvents::PerformanceInfo& perfInfo);
    void showPerformanceWarning(const QString& warning);

    // 状态查询
    bool isMuted() const { return m_isMuted; }
    InterfaceConfig getConfig() const { return m_config; }

    // 界面有效性
    bool isInterfaceValid() const;

    // 事件处理重写
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

signals:
    // 播放控制信号
    void playPauseClicked();
    void playModeClicked();
    void previousClicked();
    void nextClicked();
    void volumeChanged(int volume);
    void balanceChanged(int balance);
    void positionChanged(qint64 position);
    void durationChanged(qint64 duration);
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

    // 资源和性能信号
    void resourceLockRequested(const QString& reason);
    void resourceLockReleased();
    void performanceIssueDetected(const QString& issue);

private slots:
    // UI控件事件处理
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

    // 音频引擎切换
    void onAudioEngineButtonClicked();
    void onAudioEngineTypeChanged(AudioTypes::AudioEngineType type);

    // 进度条和音量控制槽函数
    void onProgressSliderPressed();
    void onProgressSliderReleased();
    void onProgressSliderMoved(int value);
    void onVolumeSliderValueChanged(int value);
    void onMuteButtonPressed();

    // 定时器更新
    void onUpdateTimer();
    void onPerformanceTimer();

    // 资源管理
    void onResourceLockAcquired();
    void onResourceLockReleased();
    void onResourceLockFailed(const QString& reason);

private:
    // 初始化方法
    void initializeInterface();
    void setupUI();
    void setupConnections();
    void setupVisualization();
    void setupProgressBar();
    void setupPerformanceMonitoring();

    // 清理方法
    void cleanupInterface();
    void disconnectFromAudioEngine();

    // 观察者注册管理
    bool registerWithAudioEngine();
    void unregisterFromAudioEngine();

    // 界面更新方法
    void updatePlaybackControls();
    void updateTimeDisplay();
    void updateVolumeDisplay();
    void updateMuteButtonState();
    void updateDisplayMode();
    void updateVisualization();
    void updateEqualizerDisplay();
    void updateLyricDisplay();
    void updateBalanceDisplay();
    void updateVUMeterDisplay();
    void updateEngineTypeDisplay();  // 添加更新引擎类型显示的方法

    // 性能监控方法
    void startPerformanceMonitoring();
    void stopPerformanceMonitoring();
    void updatePerformanceDisplay();

    // 资源管理方法
    bool requestResourceLock();
    void releaseResourceLock();
    void handleResourceConflict(const QString& conflictReason);
    void updateResourceLockStatus();

    // 事件处理助手方法
    void handleAudioEngineError(const QString& error);
    void handlePlaybackStateChange(AudioTypes::AudioState state);
    void handleVolumeChange(int volume, bool muted, double balance);
    void handleSongChange(const AudioEvents::SongChanged& songInfo);

    // 实用方法
    QString formatTime(qint64 milliseconds) const;
    void logInterfaceEvent(const QString& event, const QString& details = QString());
    void showErrorMessage(const QString& title, const QString& message);

private:
    // 配置
    InterfaceConfig m_config;

    // 资源管理
    bool m_isResourceLocked = false;
    bool m_resourceLockAcquired = false;

    // UI组件
    Ui::PlayInterface *ui = nullptr;
    PlayInterfaceController *m_controller = nullptr;
    QLabel *statusLabel = nullptr;
    QLabel *engineTypeLabel = nullptr;
    QLabel *currentTimeLabel = nullptr;
    QLabel *totalTimeLabel = nullptr;
    QLabel *balanceLabel = nullptr;
    QLabel *leftVUMeter = nullptr;
    QLabel *rightVUMeter = nullptr;
    QLabel *cpuUsageLabel = nullptr;
    QLabel *memoryUsageLabel = nullptr;
    QLabel *responseTimeLabel = nullptr;

    // 自定义组件
    MusicProgressBar* m_customProgressBar = nullptr;

    // 定时器
    QTimer *m_updateTimer = nullptr;
    QTimer *m_performanceTimer = nullptr;

    // 可视化组件
    QGraphicsView *m_waveformView = nullptr;
    QGraphicsView *m_spectrumView = nullptr;
    QGraphicsScene *m_waveformScene = nullptr;
    QGraphicsScene *m_spectrumScene = nullptr;

    // 音频引擎
    std::shared_ptr<ImprovedAudioEngine> m_audioEngine = nullptr;
    std::weak_ptr<ImprovedAudioEngine> m_weakAudioEngine; // 防止循环引用

    // 播放状态
    bool m_isPlaying = false;
    qint64 m_currentTime = 0;
    qint64 m_totalTime = 0;
    int m_volume = 50;
    int m_balance = 0;
    bool m_isMuted = false;
    int m_displayMode = 0;
    AudioTypes::PlayMode m_playMode = AudioTypes::PlayMode::Loop;

    // 均衡器值
    QVector<int> m_equalizerValues;

    // VU表相关
    QVector<double> m_vuLevels{2, 0.0};

    // 歌词相关
    int m_currentLyricIndex = -1;

    // 音频引擎类型
    AudioTypes::AudioEngineType m_currentEngineType = AudioTypes::AudioEngineType::QMediaPlayer;

    // 资源管理
    std::unique_ptr<ScopedAudioLock> m_resourceLock = nullptr;

    // 性能监控
    struct PerformanceData {
        double cpuUsage = 0.0;
        qint64 memoryUsage = 0;
        double responseTime = 0.0;
        int bufferLevel = 0;
        QString engineType;
        QElapsedTimer updateTimer;

        PerformanceData() {
            updateTimer.start();
        }
    } m_performanceData;

    // 观察者状态
    bool m_isRegisteredWithAudioEngine = false;
    QElapsedTimer m_interfaceTimer;

    // 错误处理
    int m_errorCount = 0;
    static const int MAX_ERROR_COUNT = 5;
    QElapsedTimer m_lastErrorTime;

    // 界面状态
    bool m_isInterfaceValid = false;
    bool m_isInitialized = false;
    
    // UI组件
    QHBoxLayout* statusLayout = nullptr;
};

/**
 * @brief 播放界面工厂
 * 提供统一的播放界面创建接口
 */
class PlayInterfaceFactory {
public:
    static std::unique_ptr<ImprovedPlayInterface> createInterface(
        QWidget* parent = nullptr,
        const ImprovedPlayInterface::InterfaceConfig& config = ImprovedPlayInterface::InterfaceConfig());

    static std::unique_ptr<ImprovedPlayInterface> createPerformanceOptimizedInterface(
        QWidget* parent = nullptr);

    static std::unique_ptr<ImprovedPlayInterface> createMinimalInterface(
        QWidget* parent = nullptr);

private:
    static ImprovedPlayInterface::InterfaceConfig getDefaultConfig();
    static ImprovedPlayInterface::InterfaceConfig getPerformanceConfig();
    static ImprovedPlayInterface::InterfaceConfig getMinimalConfig();
};

#endif // IMPROVEDPLAYINTERFACE_H