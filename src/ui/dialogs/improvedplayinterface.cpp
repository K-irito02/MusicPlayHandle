#include "improvedplayinterface.h"
#include "ui_playInterface.h"
#include "../controllers/playinterfacecontroller.h"
#include "../widgets/musicprogressbar.h"
#include "../../audio/improvedaudioengine.h"
#include "../../core/logger.h"
#include "../../core/resourcemanager.h"
#include <QProgressBar>
#include <QMessageBox>
#include <QShowEvent>
#include <QHideEvent>
#include <QCloseEvent>
#include <QMouseEvent>
#include <QTime>
#include <QDebug>
#include <QSharedPointer>
#include <QIcon>

// ImprovedPlayInterface 实现

ImprovedPlayInterface::ImprovedPlayInterface(QWidget *parent, 
                                           const InterfaceConfig& config)
    : QDialog(parent)
    , m_config(config)
    , m_isResourceLocked(false)
    , m_resourceLockAcquired(false)
    , ui(new Ui::PlayInterface)
    , m_controller(nullptr)
    , statusLabel(nullptr)
    , engineTypeLabel(nullptr)
    , currentTimeLabel(nullptr)
    , totalTimeLabel(nullptr)
    , balanceLabel(nullptr)
    , leftVUMeter(nullptr)
    , rightVUMeter(nullptr)
    , cpuUsageLabel(nullptr)
    , memoryUsageLabel(nullptr)
    , responseTimeLabel(nullptr)
    , m_customProgressBar(nullptr)
    , m_updateTimer(nullptr)
    , m_performanceTimer(nullptr)
    , m_waveformView(nullptr)
    , m_spectrumView(nullptr)
    , m_waveformScene(nullptr)
    , m_spectrumScene(nullptr)
    , m_audioEngine(nullptr)
    , m_isPlaying(false)
    , m_currentTime(0)
    , m_totalTime(0)
    , m_volume(50)
    , m_balance(0)
    , m_isMuted(false)
    , m_displayMode(0)
    , m_playMode(AudioTypes::PlayMode::Loop)
    , m_currentLyricIndex(-1)
    , m_currentEngineType(AudioTypes::AudioEngineType::QMediaPlayer)
    , m_resourceLock(nullptr)
    , m_isRegisteredWithAudioEngine(false)
    , m_errorCount(0)
    , m_isInterfaceValid(false)
    , m_isInitialized(false)
    , statusLayout(nullptr)
{
    try {
        ui->setupUi(this);

        // 初始化定时器
        m_updateTimer = new QTimer(this);
        m_performanceTimer = new QTimer(this);

        // 初始化计时器
        m_interfaceTimer.start();
        m_lastErrorTime.start();

        // 初始化界面
        setupUI();
        setupConnections();
        setupVisualization();
        setupProgressBar();
        setupPerformanceMonitoring();

        // 设置窗口属性
        setWindowTitle(m_config.interfaceName);
        setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);

        m_isInitialized = true;
        m_isInterfaceValid = true;
        // 初始化接口
        initializeInterface();
        
        qDebug() << "ImprovedPlayInterface: 初始化完成，界面名称:" << m_config.interfaceName;
    } catch (const std::exception& e) {
        qCritical() << "ImprovedPlayInterface: 初始化失败:" << e.what();
        m_isInterfaceValid = false;
        throw;
    }
}

ImprovedPlayInterface::~ImprovedPlayInterface()
{
    cleanupInterface();
    qDebug() << "ImprovedPlayInterface: 已销毁";
}



void ImprovedPlayInterface::setAudioEngine(std::shared_ptr<ImprovedAudioEngine> audioEngine)
{
    // 先从旧引擎注销
    unregisterFromAudioEngine();
    
    m_audioEngine = audioEngine;
    m_weakAudioEngine = audioEngine; // 设置弱引用
    
    if (m_audioEngine) {
        // 更新引擎类型
        m_currentEngineType = m_audioEngine->getAudioEngineType();
        updateEngineTypeDisplay();
        
        // 注册到新的音频引擎
        if (registerWithAudioEngine()) {
            qDebug() << "ImprovedPlayInterface: 成功注册到音频引擎";
        } else {
            qWarning() << "ImprovedPlayInterface: 注册到音频引擎失败";
        }
    }
}

std::shared_ptr<ImprovedAudioEngine> ImprovedPlayInterface::getAudioEngine() const
{
    return m_audioEngine;
}

void ImprovedPlayInterface::onNotify(const AudioEvents::StateChanged& event)
{
    if (!m_isInterfaceValid) {
        return;
    }
    
    try {
        auto state = static_cast<AudioTypes::AudioState>(event.state);
        handlePlaybackStateChange(state);
        setCurrentTime(event.position);
        setTotalTime(event.duration);
        
        // 更新状态标签
        if (statusLabel) {
            QString stateText;
            switch (state) {
                case AudioTypes::AudioState::Playing:
                    stateText = "播放中";
                    break;
                case AudioTypes::AudioState::Paused:
                    stateText = "已暂停";
                    break;
                case AudioTypes::AudioState::Stopped:
                    stateText = "已停止";
                    break;
                case AudioTypes::AudioState::Loading:
                    stateText = "加载中";
                    break;
                case AudioTypes::AudioState::Error:
                    stateText = "错误";
                    break;
                default:
                    stateText = "未知状态";
            }
            statusLabel->setText("状态: " + stateText);
        }
        
        if (!event.errorMessage.isEmpty()) {
            handleAudioEngineError(event.errorMessage);
        }
        
        logInterfaceEvent("状态变化", QString("状态:%1 位置:%2/%3")
                         .arg(static_cast<int>(state))
                         .arg(event.position).arg(event.duration));
    } catch (const std::exception& e) {
        qWarning() << "ImprovedPlayInterface: 处理状态变化事件异常:" << e.what();
        m_errorCount++;
    }
}

void ImprovedPlayInterface::onNotify(const AudioEvents::VolumeChanged& event)
{
    if (!m_isInterfaceValid) {
        return;
    }
    
    try {
        handleVolumeChange(event.volume, event.muted, event.balance);
        
        logInterfaceEvent("音量变化", QString("音量:%1 静音:%2 平衡:%3")
                         .arg(event.volume).arg(event.muted).arg(event.balance));
    } catch (const std::exception& e) {
        qWarning() << "ImprovedPlayInterface: 处理音量变化事件异常:" << e.what();
        m_errorCount++;
    }
}

void ImprovedPlayInterface::onNotify(const AudioEvents::SongChanged& event)
{
    if (!m_isInterfaceValid) {
        return;
    }
    
    try {
        handleSongChange(event);
        
        logInterfaceEvent("歌曲变化", QString("标题:%1 艺术家:%2")
                         .arg(event.title).arg(event.artist));
    } catch (const std::exception& e) {
        qWarning() << "ImprovedPlayInterface: 处理歌曲变化事件异常:" << e.what();
        m_errorCount++;
    }
}

void ImprovedPlayInterface::onNotify(const AudioEvents::PlaylistChanged& event)
{
    if (!m_isInterfaceValid) {
        return;
    }
    
    try {
        // 更新播放列表相关UI
        updatePlayModeDisplay(static_cast<AudioTypes::PlayMode>(event.playMode));
        
        logInterfaceEvent("播放列表变化", QString("歌曲数:%1 当前索引:%2")
                         .arg(event.songs.size()).arg(event.currentIndex));
    } catch (const std::exception& e) {
        qWarning() << "ImprovedPlayInterface: 处理播放列表变化事件异常:" << e.what();
        m_errorCount++;
    }
}

void ImprovedPlayInterface::onNotify(const AudioEvents::PerformanceInfo& event)
{
    if (!m_isInterfaceValid || !m_config.enablePerformanceMonitoring) {
        return;
    }
    
    try {
        // 更新性能数据
        m_performanceData.cpuUsage = event.cpuUsage;
        m_performanceData.memoryUsage = event.memoryUsage;
        m_performanceData.responseTime = event.responseTime;
        m_performanceData.bufferLevel = event.bufferLevel;
        m_performanceData.engineType = event.engineType;
        
        // 更新UI显示
        updatePerformanceDisplay();
        
        // 检查性能警告
        if (event.cpuUsage > 80.0) {
            showPerformanceWarning("CPU使用率过高: " + QString::number(event.cpuUsage, 'f', 1) + "%");
        }
        
        if (event.responseTime > 50.0) {
            showPerformanceWarning("响应时间过长: " + QString::number(event.responseTime, 'f', 1) + "ms");
        }
        
        if (event.bufferLevel < 20) {
            showPerformanceWarning("缓冲区水平过低: " + QString::number(event.bufferLevel) + "%");
        }
        
        // 记录性能日志
        logInterfaceEvent("性能更新", QString("CPU:%1% 内存:%2MB 响应:%3ms 缓冲:%4%")
                         .arg(event.cpuUsage, 0, 'f', 1)
                         .arg(event.memoryUsage / 1024.0 / 1024.0, 0, 'f', 1)
                         .arg(event.responseTime, 0, 'f', 1)
                         .arg(event.bufferLevel));
        
    } catch (const std::exception& e) {
        qWarning() << "ImprovedPlayInterface: 处理性能信息事件异常:" << e.what();
        m_errorCount++;
        
        if (m_errorCount > MAX_ERROR_COUNT) {
            qCritical() << "ImprovedPlayInterface: 性能监控错误次数过多，停止监控";
            stopPerformanceMonitoring();
        }
    }
}

QString ImprovedPlayInterface::getObserverName() const
{
    return m_config.interfaceName;
}

void ImprovedPlayInterface::setPlaybackState(bool isPlaying)
{
    if (m_isPlaying != isPlaying) {
        m_isPlaying = isPlaying;
        updatePlaybackControls();
    }
}

void ImprovedPlayInterface::setCurrentTime(qint64 time)
{
    if (m_currentTime != time) {
        m_currentTime = time;
        setProgressBarPosition(time);
        updateTimeDisplay();
        
        // 更新当前时间标签
        if (currentTimeLabel) {
            currentTimeLabel->setText(formatTime(time));
        }
    }
}

void ImprovedPlayInterface::setTotalTime(qint64 time)
{
    if (m_totalTime != time) {
        m_totalTime = time;
        setProgressBarDuration(time);
        updateTimeDisplay();
        
        // 更新总时间标签
        if (totalTimeLabel) {
            totalTimeLabel->setText(formatTime(time));
        }
    }
}

void ImprovedPlayInterface::setVolume(int volume)
{
    if (m_volume != volume) {
        m_volume = volume;
        setVolumeSliderValue(volume);
        updateVolumeLabel(volume);
    }
}

void ImprovedPlayInterface::setBalance(int balance)
{
    if (m_balance != balance) {
        m_balance = balance;
        updateBalanceDisplay();
        
        // 更新平衡标签
        if (balanceLabel) {
            QString balanceText;
            if (balance == 0) {
                balanceText = "平衡: 中央";
            } else if (balance < 0) {
                balanceText = QString("平衡: 左 %1%").arg(-balance);
            } else {
                balanceText = QString("平衡: 右 %1%").arg(balance);
            }
            balanceLabel->setText(balanceText);
        }
    }
}

void ImprovedPlayInterface::setMuted(bool muted)
{
    if (m_isMuted != muted) {
        m_isMuted = muted;
        updateMuteButtonIcon();
    }
}

void ImprovedPlayInterface::setSongTitle(const QString& title)
{
    if (ui && ui->label_current_song_title) {
        ui->label_current_song_title->setText(title);
    }
}

void ImprovedPlayInterface::setSongArtist(const QString& artist)
{
    if (ui && ui->label_current_song_artist) {
        ui->label_current_song_artist->setText(artist);
    }
}

void ImprovedPlayInterface::setSongAlbum(const QString& album)
{
    if (ui && ui->label_current_song_album) {
        ui->label_current_song_album->setText(album);
    }
}

void ImprovedPlayInterface::setSongCover(const QPixmap& cover)
{
    if (ui && ui->label_album_cover) {
        ui->label_album_cover->setPixmap(cover.scaled(ui->label_album_cover->size(), 
                                              Qt::KeepAspectRatio, 
                                              Qt::SmoothTransformation));
    }
}

void ImprovedPlayInterface::setLyrics(const QString& lyrics)
{
    if (ui && ui->textEdit_lyrics) {
        ui->textEdit_lyrics->setPlainText(lyrics);
    }
}

void ImprovedPlayInterface::setProgressBarPosition(qint64 position)
{
    if (m_customProgressBar) {
        m_customProgressBar->setPosition(position);
    }
}

void ImprovedPlayInterface::setProgressBarDuration(qint64 duration)
{
    if (m_customProgressBar) {
        m_customProgressBar->setDuration(duration);
    }
}

void ImprovedPlayInterface::updateProgressDisplay()
{
    updateTimeDisplay();
}

void ImprovedPlayInterface::updateVolumeControls()
{
    updateVolumeDisplay();
    updateMuteButtonState();
}

void ImprovedPlayInterface::setVolumeSliderValue(int value)
{
    if (ui && ui->slider_main_volume) {
        ui->slider_main_volume->blockSignals(true);
        ui->slider_main_volume->setValue(value);
        ui->slider_main_volume->blockSignals(false);
    }
    
    // 更新音量标签
    updateVolumeLabel(value);
}

void ImprovedPlayInterface::updateVolumeLabel(int value)
{
    if (ui && ui->label_volume_value) {
        ui->label_volume_value->setText(QString("%1%").arg(value));
    }
}

void ImprovedPlayInterface::updateMuteButtonIcon()
{
    // 注意：UI文件中没有pushButton_mute组件
    // if (ui && ui->pushButton_mute) {
    //     QString iconPath = m_isMuted ? ":/icons/muted.png" : ":/icons/volume.png";
    //     ui->pushButton_mute->setIcon(QIcon(iconPath));
    // }
}

void ImprovedPlayInterface::updatePlayModeButton(const QString& text, 
                                                const QString& iconPath, 
                                                const QString& tooltip)
{
    if (ui && ui->pushButton_play_mode) {
        ui->pushButton_play_mode->setText(text);
        ui->pushButton_play_mode->setIcon(QIcon(iconPath));
        ui->pushButton_play_mode->setToolTip(tooltip);
    }
}

void ImprovedPlayInterface::setDisplayMode(int mode)
{
    if (m_displayMode != mode) {
        m_displayMode = mode;
        updateDisplayMode();
    }
}

void ImprovedPlayInterface::updatePlayModeDisplay(AudioTypes::PlayMode mode)
{
    QString text, iconPath, tooltip;
    
    switch (mode) {
    case AudioTypes::PlayMode::Loop:
        text = "循环";
        iconPath = ":/icons/loop.png";
        tooltip = "循环播放";
        break;
    case AudioTypes::PlayMode::Random:
        text = "随机";
        iconPath = ":/icons/random.png";
        tooltip = "随机播放";
        break;
    case AudioTypes::PlayMode::RepeatOne:
        text = "单曲";
        iconPath = ":/icons/single.png";
        tooltip = "单曲循环";
        break;
    default:
        text = "循环";
        iconPath = ":/icons/loop.png";
        tooltip = "循环播放";
        break;
    }
    
    updatePlayModeButton(text, iconPath, tooltip);
}

void ImprovedPlayInterface::updateWaveform(const QVector<float>& data)
{
    if (!m_config.enableVisualization || !m_waveformScene) {
        return;
    }
    
    // 更新波形显示（简化实现）
    m_waveformScene->clear();
    
    if (data.isEmpty()) {
        return;
    }
    
    qreal width = m_waveformScene->width();
    qreal height = m_waveformScene->height();
    qreal step = width / data.size();
    
    for (int i = 0; i < data.size(); ++i) {
        qreal x = i * step;
        qreal y = height / 2 - (data[i] * height / 2);
        
        m_waveformScene->addLine(x, height / 2, x, y, QPen(Qt::green));
    }
}

void ImprovedPlayInterface::updateSpectrum(const QVector<float>& data)
{
    if (!m_config.enableVisualization || !m_spectrumScene) {
        return;
    }
    
    // 更新频谱显示（简化实现）
    m_spectrumScene->clear();
    
    if (data.isEmpty()) {
        return;
    }
    
    qreal width = m_spectrumScene->width();
    qreal height = m_spectrumScene->height();
    qreal barWidth = width / data.size();
    
    for (int i = 0; i < data.size(); ++i) {
        qreal x = i * barWidth;
        qreal barHeight = data[i] * height;
        
        QColor color = QColor::fromHsv(i * 360 / data.size(), 255, 255);
        m_spectrumScene->addRect(x, height - barHeight, barWidth, barHeight, 
                                QPen(color), QBrush(color));
    }
}

void ImprovedPlayInterface::updateVUMeter(float leftLevel, float rightLevel)
{
    m_vuLevels[0] = leftLevel;
    m_vuLevels[1] = rightLevel;
    updateVUMeterDisplay();
}

void ImprovedPlayInterface::updateVUMeterLevels(const QVector<double>& levels)
{
    if (levels.size() >= 2) {
        m_vuLevels[0] = levels[0];
        m_vuLevels[1] = levels[1];
        updateVUMeterDisplay();
    }
}

void ImprovedPlayInterface::setEqualizerValues(const QVector<int>& values)
{
    m_equalizerValues = values;
    updateEqualizerDisplay();
}

QVector<int> ImprovedPlayInterface::getEqualizerValues() const
{
    return m_equalizerValues;
}

void ImprovedPlayInterface::updatePerformanceInfo(const AudioEvents::PerformanceInfo& perfInfo)
{
    m_performanceData.cpuUsage = perfInfo.cpuUsage;
    m_performanceData.memoryUsage = perfInfo.memoryUsage;
    m_performanceData.responseTime = perfInfo.responseTime;
    m_performanceData.bufferLevel = perfInfo.bufferLevel;
    m_performanceData.engineType = perfInfo.engineType;
    m_performanceData.updateTimer.restart();
    
    updatePerformanceDisplay();
}

void ImprovedPlayInterface::showPerformanceWarning(const QString& warning)
{
    if (m_lastErrorTime.isValid() && m_lastErrorTime.elapsed() < 5000) {
        return; // 限制警告频率
    }
    
    m_lastErrorTime.restart();
    
    logInterfaceEvent("性能警告", warning);
    emit performanceIssueDetected(warning);
    
    // 可以在状态栏显示警告
    if (statusLabel) {
        statusLabel->setText("⚠️ " + warning);
    }
}

bool ImprovedPlayInterface::isInterfaceValid() const
{
    return m_isInterfaceValid;
}



void ImprovedPlayInterface::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    
    if (m_config.enablePerformanceMonitoring) {
        startPerformanceMonitoring();
    }
    
    // 请求资源锁
    if (m_config.enableResourceLocking && !m_isResourceLocked) {
        requestResourceLock();
    }
    
    logInterfaceEvent("界面显示");
}

void ImprovedPlayInterface::hideEvent(QHideEvent* event)
{
    QDialog::hideEvent(event);
    
    stopPerformanceMonitoring();
    
    logInterfaceEvent("界面隐藏");
}

void ImprovedPlayInterface::closeEvent(QCloseEvent* event)
{
    cleanupInterface();
    QDialog::closeEvent(event);
    
    logInterfaceEvent("界面关闭");
}

void ImprovedPlayInterface::mousePressEvent(QMouseEvent* event)
{
    // 线程安全的鼠标事件处理
    QDialog::mousePressEvent(event);
}

void ImprovedPlayInterface::mouseReleaseEvent(QMouseEvent* event)
{
    QDialog::mouseReleaseEvent(event);
}

void ImprovedPlayInterface::mouseMoveEvent(QMouseEvent* event)
{
    QDialog::mouseMoveEvent(event);
}

// 私有槽函数实现

void ImprovedPlayInterface::onPlayPauseClicked()
{
    if (m_audioEngine) {
        if (m_isPlaying) {
            m_audioEngine->pause();
        } else {
            m_audioEngine->play();
        }
    }
    emit playPauseClicked();
}

void ImprovedPlayInterface::onPlayModeClicked()
{
    emit playModeClicked();
}

void ImprovedPlayInterface::onPreviousClicked()
{
    if (m_audioEngine) {
        m_audioEngine->playPrevious();
    }
    emit previousClicked();
}

void ImprovedPlayInterface::onNextClicked()
{
    if (m_audioEngine) {
        m_audioEngine->playNext();
    }
    emit nextClicked();
}

void ImprovedPlayInterface::onVolumeSliderChanged(int value)
{
    if (m_audioEngine) {
        m_audioEngine->setVolume(value);
    }
    emit volumeChanged(value);
}

void ImprovedPlayInterface::onBalanceSliderChanged(int value)
{
    if (m_audioEngine) {
        m_audioEngine->setBalance(value / 100.0);
    }
    emit balanceChanged(value);
}

void ImprovedPlayInterface::onPositionSliderChanged(int value)
{
    if (m_audioEngine && m_totalTime > 0) {
        qint64 position = (static_cast<qint64>(value) * m_totalTime) / 100;
        m_audioEngine->seek(position);
    }
    emit positionChanged(value);
}

void ImprovedPlayInterface::onMuteButtonClicked()
{
    if (m_audioEngine) {
        m_audioEngine->toggleMute();
    }
    emit muteToggled(!m_isMuted);
}

void ImprovedPlayInterface::onDisplayModeClicked()
{
    emit displayModeClicked();
}

void ImprovedPlayInterface::onVisualizationTypeClicked()
{
    emit visualizationTypeClicked();
}

void ImprovedPlayInterface::onEqualizerSliderChanged()
{
    // 收集均衡器值
    QVector<int> values;
    // 这里应该从UI收集均衡器滑块的值
    
    if (m_audioEngine) {
        QVector<double> bands;
        for (int value : values) {
            bands.append(value / 100.0);
        }
        m_audioEngine->setEqualizerBands(bands);
    }
    
    emit equalizerChanged(values);
}

void ImprovedPlayInterface::onLyricClicked(qint64 timestamp)
{
    if (m_audioEngine) {
        m_audioEngine->seek(timestamp);
    }
    emit lyricClicked(timestamp);
}

void ImprovedPlayInterface::onAudioEngineButtonClicked()
{
    // 切换音频引擎类型
    if (m_audioEngine) {
        AudioTypes::AudioEngineType currentType = m_audioEngine->getAudioEngineType();
        AudioTypes::AudioEngineType newType = (currentType == AudioTypes::AudioEngineType::QMediaPlayer) ?
                                             AudioTypes::AudioEngineType::FFmpeg :
                                             AudioTypes::AudioEngineType::QMediaPlayer;
        m_audioEngine->setAudioEngineType(newType);
    }
}

void ImprovedPlayInterface::onAudioEngineTypeChanged(AudioTypes::AudioEngineType type)
{
    m_currentEngineType = type;
    updateEngineTypeDisplay();
}

void ImprovedPlayInterface::updateEngineTypeDisplay()
{
    if (!engineTypeLabel) {
        return;
    }
    
    QString engineText;
    switch (m_currentEngineType) {
        case AudioTypes::AudioEngineType::QMediaPlayer:
            engineText = "QMediaPlayer";
            break;
        case AudioTypes::AudioEngineType::FFmpeg:
            engineText = "FFmpeg";
            break;
        default:
            engineText = "未知引擎";
    }
    
    engineTypeLabel->setText("引擎: " + engineText);
    
    // 更新音频引擎按钮文本
    if (ui && ui->pushButton_audio_engine) {
        ui->pushButton_audio_engine->setText(engineText);
    }
}

void ImprovedPlayInterface::onProgressSliderPressed()
{
    emit progressSliderPressed();
}

void ImprovedPlayInterface::onProgressSliderReleased()
{
    emit progressSliderReleased();
}

void ImprovedPlayInterface::onProgressSliderMoved(int value)
{
    onPositionSliderChanged(value);
}

void ImprovedPlayInterface::onVolumeSliderValueChanged(int value)
{
    onVolumeSliderChanged(value);
}

void ImprovedPlayInterface::onMuteButtonPressed()
{
    onMuteButtonClicked();
}

void ImprovedPlayInterface::onUpdateTimer()
{
    if (!m_isInterfaceValid) {
        return;
    }
    
    // 定期更新界面
    updateTimeDisplay();
    updateVUMeterDisplay();
    
    // 检查错误计数
    if (m_errorCount > MAX_ERROR_COUNT) {
        qWarning() << "ImprovedPlayInterface: 错误次数过多，停止更新";
        m_updateTimer->stop();
    }
}

void ImprovedPlayInterface::onPerformanceTimer()
{
    if (m_config.enablePerformanceMonitoring) {
        updatePerformanceDisplay();
    }
}

void ImprovedPlayInterface::onResourceLockAcquired()
{
    m_isResourceLocked = true;
    m_resourceLockAcquired = true;
    updateResourceLockStatus();
    emit resourceLockRequested("资源锁获取成功");
    logInterfaceEvent("资源锁获取成功");
}

void ImprovedPlayInterface::onResourceLockReleased()
{
    m_isResourceLocked = false;
    m_resourceLockAcquired = false;
    updateResourceLockStatus();
    emit resourceLockReleased();
    logInterfaceEvent("资源锁释放");
}

void ImprovedPlayInterface::updateResourceLockStatus()
{
    if (!statusLabel) {
        return;
    }

    if (m_resourceLockAcquired) {
        statusLabel->setText("资源已锁定");
        statusLabel->setStyleSheet("QLabel { color: red; font-weight: bold; }");
    } else {
        statusLabel->setText("就绪");
        statusLabel->setStyleSheet("QLabel { color: #333333; }");
    }

    // 更新界面元素的启用状态
    if (ui) {
        const bool enabled = !m_resourceLockAcquired;
        ui->pushButton_play_pause_song->setEnabled(enabled);
        ui->pushButton_next_song->setEnabled(enabled);
        ui->pushButton_previous_song->setEnabled(enabled);
        ui->pushButton_audio_engine->setEnabled(enabled);
        ui->slider_progress->setEnabled(enabled);
    }
}

void ImprovedPlayInterface::onResourceLockFailed(const QString& reason)
{
    m_isResourceLocked = false;
    emit resourceLockRequested(reason);
    logInterfaceEvent("资源锁获取失败", reason);
}

// 私有方法实现

void ImprovedPlayInterface::initializeInterface()
{
    try {
        // 初始化时更新引擎类型显示
        updateEngineTypeDisplay();
        
        // 启动接口计时器
        m_interfaceTimer.start();
        
        // 标记初始化完成
        m_isInitialized = true;
        
        qDebug() << "ImprovedPlayInterface::initializeInterface: 接口初始化成功";
    } catch (const std::exception& e) {
        qCritical() << "ImprovedPlayInterface::initializeInterface: 初始化失败:" << e.what();
        throw;
    }
}

void ImprovedPlayInterface::setupUI()
{
    if (!ui) {
        qCritical() << "ImprovedPlayInterface::setupUI: ui is null";
        return;
    }

    setWindowTitle(m_config.interfaceName + " - 改进音频界面");
    
    // 创建状态布局
    QFrame* statusFrame = new QFrame(this);
    statusFrame->setFrameShape(QFrame::Box);
    statusFrame->setFrameShadow(QFrame::Raised);
    
    statusLayout = new QHBoxLayout(statusFrame);
    
    // 初始化状态标签和引擎类型标签
    statusLabel = new QLabel("就绪", statusFrame);
    engineTypeLabel = new QLabel(statusFrame);
    updateEngineTypeDisplay(); // 更新引擎类型显示
    
    // 将标签添加到状态布局
    statusLayout->addWidget(statusLabel);
    statusLayout->addStretch();
    statusLayout->addWidget(engineTypeLabel);
    
    // 将状态框架添加到主布局的底部
    if (ui->verticalLayout_main) {
        ui->verticalLayout_main->addWidget(statusFrame);
    }
    
    // 创建其他UI组件
    if (!currentTimeLabel) {
        currentTimeLabel = new QLabel("00:00", this);
    }
    
    if (!totalTimeLabel) {
        totalTimeLabel = new QLabel("00:00", this);
    }
    
    if (!balanceLabel) {
        balanceLabel = new QLabel("平衡: 中央", this);
    }
    
    if (!leftVUMeter) {
        leftVUMeter = new QLabel("L: 0%", this);
    }
    
    if (!rightVUMeter) {
        rightVUMeter = new QLabel("R: 0%", this);
    }
    
    if (!cpuUsageLabel) {
        cpuUsageLabel = new QLabel("CPU: 0%", this);
    }
    
    if (!memoryUsageLabel) {
        memoryUsageLabel = new QLabel("内存: 0MB", this);
    }
    
    if (!responseTimeLabel) {
        responseTimeLabel = new QLabel("响应: 0ms", this);
    }
    setWindowFlags(Qt::Window | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
    setMinimumSize(800, 600);
    resize(1200, 800);
    
    // 设置界面样式
    setStyleSheet(R"(
        QDialog {
            background-color: #2b2b2b;
            color: #ffffff;
        }
        QPushButton {
            background-color: #404040;
            border: 1px solid #606060;
            border-radius: 5px;
            padding: 5px;
        }
        QPushButton:hover {
            background-color: #505050;
        }
        QPushButton:pressed {
            background-color: #303030;
        }
        QSlider::groove:horizontal {
            border: 1px solid #999999;
            height: 8px;
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #B1B1B1, stop:1 #c4c4c4);
            margin: 2px 0;
        }
        QSlider::handle:horizontal {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #b4b4b4, stop:1 #8f8f8f);
            border: 1px solid #5c5c5c;
            width: 18px;
            margin: -2px 0;
            border-radius: 3px;
        }
    )");
}

void ImprovedPlayInterface::setupConnections()
{
    if (!ui) {
        qCritical() << "ImprovedPlayInterface::setupConnections: ui is null";
        return;
    }

    try {
        // 播放控制按钮信号
        connect(ui->pushButton_play_pause_song, &QPushButton::clicked, this, &ImprovedPlayInterface::onPlayPauseClicked);
        connect(ui->pushButton_next_song, &QPushButton::clicked, this, &ImprovedPlayInterface::onNextClicked);
        connect(ui->pushButton_previous_song, &QPushButton::clicked, this, &ImprovedPlayInterface::onPreviousClicked);
        connect(ui->pushButton_play_mode, &QPushButton::clicked, this, &ImprovedPlayInterface::onPlayModeClicked);
        
        // 进度条信号
        connect(ui->slider_progress, &QSlider::sliderPressed, this, &ImprovedPlayInterface::onProgressSliderPressed);
        connect(ui->slider_progress, &QSlider::sliderReleased, this, &ImprovedPlayInterface::onProgressSliderReleased);
        connect(ui->slider_progress, &QSlider::valueChanged, this, &ImprovedPlayInterface::onProgressSliderMoved);
        
        // 音量控制信号
        connect(ui->slider_main_volume, &QSlider::valueChanged, this, &ImprovedPlayInterface::onVolumeSliderValueChanged);
        connect(ui->slider_balance, &QSlider::valueChanged, this, &ImprovedPlayInterface::onBalanceSliderChanged);
        
        // 定时器信号
        if (m_updateTimer) {
            connect(m_updateTimer, &QTimer::timeout, this, &ImprovedPlayInterface::onUpdateTimer);
            m_updateTimer->start(m_config.updateInterval);
        }
        
        if (m_config.enablePerformanceMonitoring && m_performanceTimer) {
            connect(m_performanceTimer, &QTimer::timeout, this, &ImprovedPlayInterface::onPerformanceTimer);
            m_performanceTimer->start(m_config.performanceUpdateInterval);
        }
        
    } catch (const std::exception& e) {
        qCritical() << "ImprovedPlayInterface::setupConnections: 设置信号槽连接失败:" << e.what();
    }
}

void ImprovedPlayInterface::setupVisualization()
{
    if (!m_config.enableVisualization) {
        return;
    }

    if (!ui || !ui->verticalLayout_visualization) {
        qWarning() << "ImprovedPlayInterface::setupVisualization: ui或可视化布局为空";
        return;
    }
    
    try {
        // 创建波形视图
        if (!m_waveformView) {
            m_waveformView = new QGraphicsView(this);
            m_waveformScene = new QGraphicsScene(this);
            m_waveformView->setScene(m_waveformScene);
            m_waveformView->setMinimumHeight(100);
            m_waveformView->setMaximumHeight(150);
        }
        
        // 创建频谱视图
        if (!m_spectrumView) {
            m_spectrumView = new QGraphicsView(this);
            m_spectrumScene = new QGraphicsScene(this);
            m_spectrumView->setScene(m_spectrumScene);
            m_spectrumView->setMinimumHeight(100);
            m_spectrumView->setMaximumHeight(150);
        }
        
        // 添加到可视化布局中
        ui->verticalLayout_visualization->addWidget(m_waveformView);
        ui->verticalLayout_visualization->addWidget(m_spectrumView);
        
        // 设置视图样式
        QString viewStyle = "QGraphicsView { background-color: #1e1e1e; border: 1px solid #3e3e3e; }";
        m_waveformView->setStyleSheet(viewStyle);
        m_spectrumView->setStyleSheet(viewStyle);
        
    } catch (const std::exception& e) {
        qCritical() << "ImprovedPlayInterface::setupVisualization: 设置可视化组件失败:" << e.what();
    }
}

void ImprovedPlayInterface::setupProgressBar()
{
    if (!ui || !ui->slider_progress) {
        qWarning() << "ImprovedPlayInterface::setupProgressBar: ui或进度条控件未找到";
        return;
    }

    try {
        // 设置进度条的范围和初始值
        ui->slider_progress->setRange(0, 100);
        ui->slider_progress->setValue(0);
        
        // 设置进度条样式和属性
        ui->slider_progress->setMinimumHeight(20);
        ui->slider_progress->setMaximumHeight(30);
        
        // 连接信号槽
        connect(ui->slider_progress, &QSlider::valueChanged,
                this, &ImprovedPlayInterface::onProgressSliderMoved);
        connect(ui->slider_progress, &QSlider::sliderPressed,
                this, &ImprovedPlayInterface::onProgressSliderPressed);
        connect(ui->slider_progress, &QSlider::sliderReleased,
                this, &ImprovedPlayInterface::onProgressSliderReleased);
        
        // 设置进度条样式
        ui->slider_progress->setStyleSheet(R"(
            QSlider::groove:horizontal {
                border: 1px solid #999999;
                height: 8px;
                background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #B1B1B1, stop:1 #c4c4c4);
                margin: 2px 0;
            }
            QSlider::handle:horizontal {
                background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #b4b4b4, stop:1 #8f8f8f);
                border: 1px solid #5c5c5c;
                width: 18px;
                margin: -2px 0;
                border-radius: 3px;
            }
        )");
        
    } catch (const std::exception& e) {
        qCritical() << "ImprovedPlayInterface::setupProgressBar: 设置进度条失败:" << e.what();
    }
}

void ImprovedPlayInterface::setupPerformanceMonitoring()
{
    if (!m_config.enablePerformanceMonitoring || !ui) {
        return;
    }

    try {
        // 初始化性能监控定时器
        if (!m_performanceTimer) {
            m_performanceTimer = new QTimer(this);
            m_performanceTimer->setInterval(m_config.performanceUpdateInterval);
            connect(m_performanceTimer, &QTimer::timeout, this, &ImprovedPlayInterface::onPerformanceTimer);
        }

        // 初始化性能显示标签 - 注释掉不存在的UI成员
        /*
        if (ui->label_cpu_usage) {
            ui->label_cpu_usage->setText("CPU: 0.0%");
            ui->label_cpu_usage->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            ui->label_cpu_usage->setStyleSheet("color: #333333;");
        }

        if (ui->label_memory_usage) {
            ui->label_memory_usage->setText("内存: 0.0 MB");
            ui->label_memory_usage->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            ui->label_memory_usage->setStyleSheet("color: #333333;");
        }

        if (ui->label_response_time) {
            ui->label_response_time->setText("响应: 0.0 ms");
            ui->label_response_time->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            ui->label_response_time->setStyleSheet("color: #333333;");
        }

        // 初始化缓冲区进度条
        if (ui->progressBar_buffer) {
            ui->progressBar_buffer->setRange(0, 100);
            ui->progressBar_buffer->setValue(0);
            ui->progressBar_buffer->setFormat("%p%");
            ui->progressBar_buffer->setStyleSheet(R"(
                QProgressBar {
                    border: 1px solid #999999;
                    border-radius: 3px;
                    text-align: center;
                    background: #f0f0f0;
                }
                QProgressBar::chunk {
                    background-color: #4CAF50;
                    border-radius: 2px;
                }
            )");
        }
        */

        // 启动性能监控
        m_performanceTimer->start();
        m_performanceData.updateTimer.start();
        
        qDebug() << "ImprovedPlayInterface::setupPerformanceMonitoring: 性能监控已启动";
        
    } catch (const std::exception& e) {
        qCritical() << "ImprovedPlayInterface::setupPerformanceMonitoring: 设置性能监控失败:" << e.what();
    }
}



void ImprovedPlayInterface::updatePerformanceDisplay()
{
    if (!ui || !m_config.enablePerformanceMonitoring) {
        return;
    }

    try {
        // 注释掉不存在的UI成员使用
        /*
        // 更新CPU使用率
        if (ui->label_cpu_usage) {
            QString cpuText = QString("CPU: %1%").arg(m_performanceData.cpuUsage, 0, 'f', 1);
            ui->label_cpu_usage->setText(cpuText);
            
            // 根据CPU使用率设置颜色
            if (m_performanceData.cpuUsage > 80.0) {
                ui->label_cpu_usage->setStyleSheet("color: #FF4444;"); // 红色
            } else if (m_performanceData.cpuUsage > 60.0) {
                ui->label_cpu_usage->setStyleSheet("color: #FFAA00;"); // 橙色
            } else {
                ui->label_cpu_usage->setStyleSheet("color: #333333;"); // 正常色
            }
        }

        // 更新内存使用
        if (ui->label_memory_usage) {
            double memoryMB = m_performanceData.memoryUsage / 1024.0 / 1024.0;
            QString memoryText = QString("内存: %1 MB").arg(memoryMB, 0, 'f', 1);
            ui->label_memory_usage->setText(memoryText);
            
            // 根据内存使用设置颜色
            if (memoryMB > 1024.0) { // 超过1GB
                ui->label_memory_usage->setStyleSheet("color: #FF4444;");
            } else if (memoryMB > 512.0) { // 超过512MB
                ui->label_memory_usage->setStyleSheet("color: #FFAA00;");
            } else {
                ui->label_memory_usage->setStyleSheet("color: #333333;");
            }
        }

        // 更新响应时间
        if (ui->label_response_time) {
            QString responseText = QString("响应: %1 ms").arg(m_performanceData.responseTime, 0, 'f', 1);
            ui->label_response_time->setText(responseText);
            
            // 根据响应时间设置颜色
            if (m_performanceData.responseTime > 50.0) {
                ui->label_response_time->setStyleSheet("color: #FF4444;");
            } else if (m_performanceData.responseTime > 30.0) {
                ui->label_response_time->setStyleSheet("color: #FFAA00;");
            } else {
                ui->label_response_time->setStyleSheet("color: #333333;");
            }
        }

        // 更新缓冲区水平
        if (ui->progressBar_buffer) {
            ui->progressBar_buffer->setValue(m_performanceData.bufferLevel);
            
            // 根据缓冲区水平设置颜色
            QString styleSheet = R"(
                QProgressBar {
                    border: 1px solid #999999;
                    border-radius: 3px;
                    text-align: center;
                    background: #f0f0f0;
                }
                QProgressBar::chunk {
                    border-radius: 2px;
                    %1
                }
            )";
            
            QString chunkStyle;
            if (m_performanceData.bufferLevel < 20) {
                chunkStyle = "background-color: #FF4444;"; // 红色
            } else if (m_performanceData.bufferLevel < 40) {
                chunkStyle = "background-color: #FFAA00;"; // 橙色
            } else {
                chunkStyle = "background-color: #4CAF50;"; // 绿色
            }
            
            ui->progressBar_buffer->setStyleSheet(styleSheet.arg(chunkStyle));
        }
        */
        
    } catch (const std::exception& e) {
        qWarning() << "ImprovedPlayInterface::updatePerformanceDisplay: 更新性能显示失败:" << e.what();
    }
}



void ImprovedPlayInterface::startPerformanceMonitoring()
{
    if (!m_config.enablePerformanceMonitoring || !ui) {
        return;
    }

    try {
        // 初始化性能监控组件
        setupPerformanceMonitoring();
        
        // 重置错误计数
        m_errorCount = 0;
        
        // 重置性能数据
        m_performanceData = PerformanceData();
        
        qDebug() << "ImprovedPlayInterface::startPerformanceMonitoring: 性能监控已启动";
        
    } catch (const std::exception& e) {
        qCritical() << "ImprovedPlayInterface::startPerformanceMonitoring: 启动性能监控失败:" << e.what();
    }
}

void ImprovedPlayInterface::stopPerformanceMonitoring()
{
    try {
        if (m_performanceTimer) {
            m_performanceTimer->stop();
        }
        
        // 清理性能显示
        if (ui) {
            // 注释掉不存在的UI成员
            /*
            if (ui->label_cpu_usage) {
                ui->label_cpu_usage->setText("CPU: --");
                ui->label_cpu_usage->setStyleSheet("");
            }
            
            if (ui->label_memory_usage) {
                ui->label_memory_usage->setText("内存: --");
                ui->label_memory_usage->setStyleSheet("");
            }
            
            if (ui->label_response_time) {
                ui->label_response_time->setText("响应: --");
                ui->label_response_time->setStyleSheet("");
            }
            
            if (ui->progressBar_buffer) {
                ui->progressBar_buffer->setValue(0);
                ui->progressBar_buffer->setStyleSheet("");
            }
            */
        }
        
        qDebug() << "ImprovedPlayInterface::stopPerformanceMonitoring: 性能监控已停止";
        
    } catch (const std::exception& e) {
        qWarning() << "ImprovedPlayInterface::stopPerformanceMonitoring: 停止性能监控失败:" << e.what();
    }
}

void ImprovedPlayInterface::cleanupInterface()
{
    // 停止性能监控
    stopPerformanceMonitoring();
    
    // 清理状态标签和布局
    if (statusLabel) {
        delete statusLabel;
        statusLabel = nullptr;
    }
    
    if (engineTypeLabel) {
        delete engineTypeLabel;
        engineTypeLabel = nullptr;
    }
    
    if (currentTimeLabel) {
        delete currentTimeLabel;
        currentTimeLabel = nullptr;
    }
    
    if (totalTimeLabel) {
        delete totalTimeLabel;
        totalTimeLabel = nullptr;
    }
    
    if (balanceLabel) {
        delete balanceLabel;
        balanceLabel = nullptr;
    }
    
    if (leftVUMeter) {
        delete leftVUMeter;
        leftVUMeter = nullptr;
    }
    
    if (rightVUMeter) {
        delete rightVUMeter;
        rightVUMeter = nullptr;
    }
    
    if (cpuUsageLabel) {
        delete cpuUsageLabel;
        cpuUsageLabel = nullptr;
    }
    
    if (memoryUsageLabel) {
        delete memoryUsageLabel;
        memoryUsageLabel = nullptr;
    }
    
    if (responseTimeLabel) {
        delete responseTimeLabel;
        responseTimeLabel = nullptr;
    }
    
    if (statusLayout) {
        delete statusLayout;
        statusLayout = nullptr;
    }
    // 停止所有定时器
    if (m_updateTimer) {
        m_updateTimer->stop();
    }
    
    if (m_performanceTimer) {
        m_performanceTimer->stop();
    }
    
    // 从音频引擎注销
    unregisterFromAudioEngine();
    
    // 释放资源锁
    releaseResourceLock();
    
    // 清理UI
    if (ui) {
        delete ui;
        ui = nullptr;
    }
    
    m_isInitialized = false;
}

void ImprovedPlayInterface::disconnectFromAudioEngine()
{
    if (m_audioEngine) {
        // 断开所有连接
        disconnect(m_audioEngine.get(), nullptr, this, nullptr);
    }
}

bool ImprovedPlayInterface::registerWithAudioEngine()
{
    if (!m_audioEngine) {
        return false;
    }
    
    try {
        // 创建观察者智能指针 - 使用QSharedPointer
        auto stateObserver = QSharedPointer<AudioStateObserver>(static_cast<AudioStateObserver*>(this), [](AudioStateObserver*){});
        auto volumeObserver = QSharedPointer<AudioVolumeObserver>(static_cast<AudioVolumeObserver*>(this), [](AudioVolumeObserver*){});
        auto songObserver = QSharedPointer<AudioSongObserver>(static_cast<AudioSongObserver*>(this), [](AudioSongObserver*){});
        auto playlistObserver = QSharedPointer<AudioPlaylistObserver>(static_cast<AudioPlaylistObserver*>(this), [](AudioPlaylistObserver*){});
        auto performanceObserver = QSharedPointer<AudioPerformanceObserver>(static_cast<AudioPerformanceObserver*>(this), [](AudioPerformanceObserver*){});
        
        // 注册到音频引擎 - 明确指定Subject类型以避免模糊调用
        static_cast<AudioStateSubject*>(m_audioEngine.get())->addObserver(stateObserver);
        static_cast<AudioVolumeSubject*>(m_audioEngine.get())->addObserver(volumeObserver);
        static_cast<AudioSongSubject*>(m_audioEngine.get())->addObserver(songObserver);
        static_cast<AudioPlaylistSubject*>(m_audioEngine.get())->addObserver(playlistObserver);
        // 注意：ImprovedAudioEngine暂时不支持AudioPerformanceSubject
        // static_cast<AudioPerformanceSubject*>(m_audioEngine.get())->addObserver(performanceObserver);
        
        m_isRegisteredWithAudioEngine = true;
        return true;
    } catch (const std::exception& e) {
        qWarning() << "ImprovedPlayInterface: 注册观察者失败:" << e.what();
        return false;
    }
}

void ImprovedPlayInterface::unregisterFromAudioEngine()
{
    if (m_audioEngine && m_isRegisteredWithAudioEngine) {
        disconnectFromAudioEngine();
        m_isRegisteredWithAudioEngine = false;
    }
}

void ImprovedPlayInterface::updatePlaybackControls()
{
    if (ui && ui->pushButton_play_pause_song) {
        QString text = m_isPlaying ? "暂停" : "播放";
        QString iconPath = m_isPlaying ? ":/icons/pause.png" : ":/icons/play.png";
        ui->pushButton_play_pause_song->setText(text);
        ui->pushButton_play_pause_song->setIcon(QIcon(iconPath));
    }
}

void ImprovedPlayInterface::updateTimeDisplay()
{
    if (currentTimeLabel) {
        currentTimeLabel->setText(formatTime(m_currentTime));
    }
    
    if (totalTimeLabel) {
        totalTimeLabel->setText(formatTime(m_totalTime));
    }
}

void ImprovedPlayInterface::updateVolumeDisplay()
{
    setVolumeSliderValue(m_volume);
    updateVolumeLabel(m_volume);
}

void ImprovedPlayInterface::updateMuteButtonState()
{
    updateMuteButtonIcon();
}

void ImprovedPlayInterface::updateDisplayMode()
{
    // 根据显示模式更新界面
}

void ImprovedPlayInterface::updateVisualization()
{
    if (!m_config.enableVisualization) {
        return;
    }
    
    // 更新可视化显示
}

void ImprovedPlayInterface::updateEqualizerDisplay()
{
    // 更新均衡器显示
}

void ImprovedPlayInterface::updateLyricDisplay()
{
    // 更新歌词显示
}

void ImprovedPlayInterface::updateBalanceDisplay()
{
    if (balanceLabel) {
        balanceLabel->setText(QString("平衡: %1").arg(m_balance));
    }
}

void ImprovedPlayInterface::updateVUMeterDisplay()
{
    if (!m_config.enableVUMeter || m_vuLevels.size() < 2) {
        return;
    }
    
    // 更新VU表显示
    if (leftVUMeter) {
        leftVUMeter->setText(QString("L: %1%").arg(static_cast<int>(m_vuLevels[0] * 100)));
    }
    
    if (rightVUMeter) {
        rightVUMeter->setText(QString("R: %1%").arg(static_cast<int>(m_vuLevels[1] * 100)));
    }
}

// 删除重复的函数定义

bool ImprovedPlayInterface::requestResourceLock()
{
    try {
        QString lockId = m_config.interfaceName + "_PlayInterface";
        m_resourceLock = ResourceManager::instance().createScopedLock(
            lockId, m_config.interfaceName, 5000);
        
        if (m_resourceLock && *m_resourceLock) {
            onResourceLockAcquired();
            return true;
        } else {
            onResourceLockFailed("无法获取资源锁");
            return false;
        }
    } catch (const std::exception& e) {
        onResourceLockFailed(QString("资源锁异常: %1").arg(e.what()));
        return false;
    }
}

void ImprovedPlayInterface::releaseResourceLock()
{
    if (m_resourceLock) {
        m_resourceLock.reset();
        onResourceLockReleased();
    }
}

void ImprovedPlayInterface::handleResourceConflict(const QString& conflictReason)
{
    logInterfaceEvent("资源冲突", conflictReason);
    showErrorMessage("资源冲突", "音频资源被其他组件占用: " + conflictReason);
}

void ImprovedPlayInterface::handleAudioEngineError(const QString& error)
{
    m_errorCount++;
    logInterfaceEvent("音频引擎错误", error);
    
    if (m_errorCount <= MAX_ERROR_COUNT) {
        showErrorMessage("音频引擎错误", error);
    }
}

void ImprovedPlayInterface::handlePlaybackStateChange(AudioTypes::AudioState state)
{
    switch (state) {
        case AudioTypes::AudioState::Playing:
            m_isPlaying = true;
            break;
        case AudioTypes::AudioState::Paused:
        case AudioTypes::AudioState::Stopped:
            m_isPlaying = false;
            break;
        case AudioTypes::AudioState::Loading:
            // 保持当前状态
            break;
        case AudioTypes::AudioState::Error:
            m_isPlaying = false;
            break;
    }
    
    updatePlaybackControls();
}

// 已删除重复的函数定义

void ImprovedPlayInterface::handleVolumeChange(int volume, bool muted, double balance)
{
    setVolume(volume);
    setMuted(muted);
    setBalance(static_cast<int>(balance * 100));
}

void ImprovedPlayInterface::handleSongChange(const AudioEvents::SongChanged& songInfo)
{
    setSongTitle(songInfo.title);
    setSongArtist(songInfo.artist);
    setSongAlbum(songInfo.album);
    setTotalTime(songInfo.duration);
}

QString ImprovedPlayInterface::formatTime(qint64 milliseconds) const
{
    if (milliseconds < 0) {
        return "00:00";
    }
    
    int seconds = static_cast<int>(milliseconds / 1000);
    int minutes = seconds / 60;
    seconds %= 60;
    
    return QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));
}

void ImprovedPlayInterface::logInterfaceEvent(const QString& event, const QString& details)
{
    QString logMessage = QString("[%1] %2").arg(m_config.interfaceName, event);
    if (!details.isEmpty()) {
        logMessage += " - " + details;
    }
    
    qDebug() << logMessage;
    
    // 这里可以集成到项目的日志系统
    // LOG_INFO("ImprovedPlayInterface", logMessage);
}

void ImprovedPlayInterface::showErrorMessage(const QString& title, const QString& message)
{
    QMessageBox::warning(this, title, message);
}

// PlayInterfaceFactory 实现

std::unique_ptr<ImprovedPlayInterface> PlayInterfaceFactory::createInterface(
    QWidget* parent, const ImprovedPlayInterface::InterfaceConfig& config)
{
    try {
        return std::make_unique<ImprovedPlayInterface>(parent, config);
    } catch (const std::exception& e) {
        qCritical() << "PlayInterfaceFactory: 创建界面失败:" << e.what();
        return nullptr;
    }
}

std::unique_ptr<ImprovedPlayInterface> PlayInterfaceFactory::createPerformanceOptimizedInterface(QWidget* parent)
{
    ImprovedPlayInterface::InterfaceConfig config;
    config.interfaceName = "PerformanceOptimized";
    config.enablePerformanceMonitoring = true;
    config.enableVisualization = true;
    config.enableVUMeter = true;
    config.updateInterval = 30; // 高刷新率
    config.performanceUpdateInterval = 500; // 频繁性能更新
    
    return createInterface(parent, config);
}

std::unique_ptr<ImprovedPlayInterface> PlayInterfaceFactory::createMinimalInterface(QWidget* parent)
{
    ImprovedPlayInterface::InterfaceConfig config;
    config.interfaceName = "Minimal";
    config.enablePerformanceMonitoring = false;
    config.enableVisualization = false;
    config.enableVUMeter = false;
    config.updateInterval = 100; // 低刷新率
    config.performanceUpdateInterval = 2000; // 较少性能更新
    
    return createInterface(parent, config);
}