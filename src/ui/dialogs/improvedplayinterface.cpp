#include "improvedplayinterface.h"
#include "ui_PlayInterface.h"
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

// ImprovedPlayInterface 实现

ImprovedPlayInterface::ImprovedPlayInterface(QWidget *parent, 
                                           const InterfaceConfig& config)
    : QDialog(parent)
    , m_config(config)
    , ui(new Ui::PlayInterface)
    , m_controller(nullptr)
    , m_customProgressBar(nullptr)
    , m_updateTimer(new QTimer(this))
    , m_performanceTimer(new QTimer(this))
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
    , m_vuLevels(2, 0.0)
    , m_currentLyricIndex(-1)
    , m_currentEngineType(AudioTypes::AudioEngineType::QMediaPlayer)
    , m_isResourceLocked(false)
    , m_isRegisteredWithAudioEngine(false)
    , m_errorCount(0)
    , m_isInterfaceValid(false)
    , m_isInitialized(false)
{
    try {
        initializeInterface();
        m_isInterfaceValid = true;
        
        qDebug() << "ImprovedPlayInterface: 初始化完成，界面名称:" << m_config.interfaceName;
    } catch (const std::exception& e) {
        qCritical() << "ImprovedPlayInterface: 初始化失败:" << e.what();
        m_isInterfaceValid = false;
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
        handlePlaybackStateChange(event.state);
        setCurrentTime(event.position);
        setTotalTime(event.duration);
        
        if (!event.errorMessage.isEmpty()) {
            handleAudioEngineError(event.errorMessage);
        }
        
        logInterfaceEvent("状态变化", QString("状态:%1 位置:%2/%3")
                         .arg(static_cast<int>(event.state))
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
        updatePerformanceInfo(event);
        
        // 检查性能警告
        if (event.cpuUsage > 80.0) {
            showPerformanceWarning("CPU使用率过高: " + QString::number(event.cpuUsage, 'f', 1) + "%");
        }
        
        if (event.responseTime > 50.0) {
            showPerformanceWarning("响应时间过长: " + QString::number(event.responseTime, 'f', 1) + "ms");
        }
        
    } catch (const std::exception& e) {
        qWarning() << "ImprovedPlayInterface: 处理性能信息事件异常:" << e.what();
        m_errorCount++;
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
    }
}

void ImprovedPlayInterface::setTotalTime(qint64 time)
{
    if (m_totalTime != time) {
        m_totalTime = time;
        setProgressBarDuration(time);
        updateTimeDisplay();
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
    if (ui && ui->songTitleLabel) {
        ui->songTitleLabel->setText(title);
    }
}

void ImprovedPlayInterface::setSongArtist(const QString& artist)
{
    if (ui && ui->artistLabel) {
        ui->artistLabel->setText(artist);
    }
}

void ImprovedPlayInterface::setSongAlbum(const QString& album)
{
    if (ui && ui->albumLabel) {
        ui->albumLabel->setText(album);
    }
}

void ImprovedPlayInterface::setSongCover(const QPixmap& cover)
{
    if (ui && ui->coverLabel) {
        ui->coverLabel->setPixmap(cover.scaled(ui->coverLabel->size(), 
                                              Qt::KeepAspectRatio, 
                                              Qt::SmoothTransformation));
    }
}

void ImprovedPlayInterface::setLyrics(const QString& lyrics)
{
    if (ui && ui->lyricsTextEdit) {
        ui->lyricsTextEdit->setPlainText(lyrics);
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
    if (ui && ui->volumeSlider) {
        ui->volumeSlider->setValue(value);
    }
}

void ImprovedPlayInterface::updateVolumeLabel(int value)
{
    if (ui && ui->volumeLabel) {
        ui->volumeLabel->setText(QString("%1%").arg(value));
    }
}

void ImprovedPlayInterface::updateMuteButtonIcon()
{
    if (ui && ui->muteButton) {
        QString iconPath = m_isMuted ? ":/icons/muted.png" : ":/icons/volume.png";
        ui->muteButton->setIcon(QIcon(iconPath));
    }
}

void ImprovedPlayInterface::updatePlayModeButton(const QString& text, 
                                                const QString& iconPath, 
                                                const QString& tooltip)
{
    if (ui && ui->playModeButton) {
        ui->playModeButton->setText(text);
        ui->playModeButton->setIcon(QIcon(iconPath));
        ui->playModeButton->setToolTip(tooltip);
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
    case AudioTypes::PlayMode::Sequential:
        text = "顺序";
        iconPath = ":/icons/sequential.png";
        tooltip = "顺序播放";
        break;
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
    case AudioTypes::PlayMode::Single:
        text = "单曲";
        iconPath = ":/icons/single.png";
        tooltip = "单曲循环";
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
    if (ui && ui->statusLabel) {
        ui->statusLabel->setText("⚠️ " + warning);
    }
}

bool ImprovedPlayInterface::isInterfaceValid() const
{
    return m_isInterfaceValid;
}

bool ImprovedPlayInterface::isResourceLocked() const
{
    return m_isResourceLocked;
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
    
    // 更新UI显示
    if (ui && ui->engineTypeLabel) {
        QString typeText = (type == AudioTypes::AudioEngineType::QMediaPlayer) ? "QMediaPlayer" : "FFmpeg";
        ui->engineTypeLabel->setText(typeText);
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
    emit resourceLockAcquired();
    logInterfaceEvent("资源锁获取成功");
}

void ImprovedPlayInterface::onResourceLockReleased()
{
    m_isResourceLocked = false;
    emit resourceLockReleased();
    logInterfaceEvent("资源锁释放");
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
    ui->setupUi(this);
    
    setupUI();
    setupProgressBar();
    setupVisualization();
    setupConnections();
    
    if (m_config.enablePerformanceMonitoring) {
        setupPerformanceMonitoring();
    }
    
    m_interfaceTimer.start();
    m_isInitialized = true;
}

void ImprovedPlayInterface::setupUI()
{
    setWindowTitle(m_config.interfaceName + " - 改进音频界面");
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
    // 连接UI控件信号 (检查ui和控件是否存在)
    if (ui) {
        if (ui->playPauseButton) {
            connect(ui->playPauseButton, &QPushButton::clicked, this, &ImprovedPlayInterface::onPlayPauseClicked);
        }
        
        if (ui->previousButton) {
            connect(ui->previousButton, &QPushButton::clicked, this, &ImprovedPlayInterface::onPreviousClicked);
        }
        
        if (ui->nextButton) {
            connect(ui->nextButton, &QPushButton::clicked, this, &ImprovedPlayInterface::onNextClicked);
        }
        
        if (ui->volumeSlider) {
            connect(ui->volumeSlider, &QSlider::valueChanged, this, &ImprovedPlayInterface::onVolumeSliderChanged);
        }
        
        if (ui->muteButton) {
            connect(ui->muteButton, &QPushButton::clicked, this, &ImprovedPlayInterface::onMuteButtonClicked);
        }
    }
    
    // 连接定时器
    connect(m_updateTimer, &QTimer::timeout, this, &ImprovedPlayInterface::onUpdateTimer);
    m_updateTimer->start(m_config.updateInterval);
    
    if (m_config.enablePerformanceMonitoring) {
        connect(m_performanceTimer, &QTimer::timeout, this, &ImprovedPlayInterface::onPerformanceTimer);
        m_performanceTimer->start(m_config.performanceUpdateInterval);
    }
}

void ImprovedPlayInterface::setupVisualization()
{
    if (!m_config.enableVisualization) {
        return;
    }
    
    // 创建波形视图
    m_waveformView = new QGraphicsView(this);
    m_waveformScene = new QGraphicsScene(this);
    m_waveformView->setScene(m_waveformScene);
    
    // 创建频谱视图
    m_spectrumView = new QGraphicsView(this);
    m_spectrumScene = new QGraphicsScene(this);
    m_spectrumView->setScene(m_spectrumScene);
    
    // 添加到布局中（这里需要根据实际UI布局调整）
    // layout->addWidget(m_waveformView);
    // layout->addWidget(m_spectrumView);
}

void ImprovedPlayInterface::setupProgressBar()
{
    m_customProgressBar = new MusicProgressBar(this);
    
    connect(m_customProgressBar, &MusicProgressBar::positionChanged, 
            this, &ImprovedPlayInterface::onPositionSliderChanged);
    connect(m_customProgressBar, &MusicProgressBar::sliderPressed,
            this, &ImprovedPlayInterface::onProgressSliderPressed);
    connect(m_customProgressBar, &MusicProgressBar::sliderReleased,
            this, &ImprovedPlayInterface::onProgressSliderReleased);
    
    // 将自定义进度条添加到布局中
    // layout->addWidget(m_customProgressBar);
}

void ImprovedPlayInterface::setupPerformanceMonitoring()
{
    // 性能监控相关设置
    m_performanceData.updateTimer.start();
}

void ImprovedPlayInterface::cleanupInterface()
{
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
        // 创建观察者智能指针
        auto stateObserver = std::shared_ptr<AudioStateObserver>(this, [](AudioStateObserver*){});
        auto volumeObserver = std::shared_ptr<AudioVolumeObserver>(this, [](AudioVolumeObserver*){});
        auto songObserver = std::shared_ptr<AudioSongObserver>(this, [](AudioSongObserver*){});
        auto playlistObserver = std::shared_ptr<AudioPlaylistObserver>(this, [](AudioPlaylistObserver*){});
        auto performanceObserver = std::shared_ptr<AudioPerformanceObserver>(this, [](AudioPerformanceObserver*){});
        
        // 注册到音频引擎
        m_audioEngine->addObserver(stateObserver);
        m_audioEngine->addObserver(volumeObserver);
        m_audioEngine->addObserver(songObserver);
        m_audioEngine->addObserver(playlistObserver);
        m_audioEngine->addObserver(performanceObserver);
        
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
    if (ui && ui->playPauseButton) {
        QString text = m_isPlaying ? "暂停" : "播放";
        QString iconPath = m_isPlaying ? ":/icons/pause.png" : ":/icons/play.png";
        ui->playPauseButton->setText(text);
        ui->playPauseButton->setIcon(QIcon(iconPath));
    }
}

void ImprovedPlayInterface::updateTimeDisplay()
{
    if (ui) {
        if (ui->currentTimeLabel) {
            ui->currentTimeLabel->setText(formatTime(m_currentTime));
        }
        
        if (ui->totalTimeLabel) {
            ui->totalTimeLabel->setText(formatTime(m_totalTime));
        }
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
    if (ui && ui->balanceLabel) {
        ui->balanceLabel->setText(QString("平衡: %1").arg(m_balance));
    }
}

void ImprovedPlayInterface::updateVUMeterDisplay()
{
    if (!m_config.enableVUMeter || m_vuLevels.size() < 2) {
        return;
    }
    
    // 更新VU表显示
    if (ui && ui->leftVUMeter) {
        ui->leftVUMeter->setValue(static_cast<int>(m_vuLevels[0] * 100));
    }
    
    if (ui && ui->rightVUMeter) {
        ui->rightVUMeter->setValue(static_cast<int>(m_vuLevels[1] * 100));
    }
}

void ImprovedPlayInterface::startPerformanceMonitoring()
{
    if (m_performanceTimer && !m_performanceTimer->isActive()) {
        m_performanceTimer->start(m_config.performanceUpdateInterval);
    }
}

void ImprovedPlayInterface::stopPerformanceMonitoring()
{
    if (m_performanceTimer) {
        m_performanceTimer->stop();
    }
}

void ImprovedPlayInterface::updatePerformanceDisplay()
{
    if (!m_config.enablePerformanceMonitoring || !ui) {
        return;
    }
    
    // 更新性能显示
    if (ui->cpuUsageLabel) {
        ui->cpuUsageLabel->setText(QString("CPU: %1%").arg(m_performanceData.cpuUsage, 0, 'f', 1));
    }
    
    if (ui->memoryUsageLabel) {
        qint64 memoryMB = m_performanceData.memoryUsage / 1024 / 1024;
        ui->memoryUsageLabel->setText(QString("内存: %1MB").arg(memoryMB));
    }
    
    if (ui->responseTimeLabel) {
        ui->responseTimeLabel->setText(QString("响应: %1ms").arg(m_performanceData.responseTime, 0, 'f', 1));
    }
}

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

void ImprovedPlayInterface::handlePlaybackStateChange(AudioEvents::StateChanged::State state)
{
    bool isPlaying = (state == AudioEvents::StateChanged::State::Playing);
    setPlaybackState(isPlaying);
}

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