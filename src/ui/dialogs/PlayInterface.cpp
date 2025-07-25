#include "PlayInterface.h"
#include "ui_PlayInterface.h"
#include "../controllers/playinterfacecontroller.h"
#include "../widgets/musicprogressbar.h"
#include "../../audio/audioengine.h"
#include <QProgressBar>

PlayInterface::PlayInterface(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::PlayInterface)
    , m_controller(nullptr)
    , m_updateTimer(nullptr)
    , m_customProgressBar(nullptr)
    , m_audioEngine(nullptr)
    , m_waveformView(nullptr)
    , m_spectrumView(nullptr)
    , m_waveformScene(nullptr)
    , m_spectrumScene(nullptr)
    , m_isPlaying(false)
    , m_currentTime(0)
    , m_totalTime(0)
    , m_volume(50)
    , m_balance(0)
    , m_isMuted(false)
    , m_displayMode(0)
    , m_vuLevels(2, 0.0)
    , m_vuUpdateTimer(nullptr)
    , m_currentLyricIndex(-1)
    , m_currentEngineType(AudioTypes::AudioEngineType::QMediaPlayer) // 默认QMediaPlayer
{
    try {
        ui->setupUi(this);
        
        // 设置窗口属性
        setWindowTitle("音频可视化界面");
        setWindowFlags(Qt::Window | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
        setMinimumSize(800, 600);
        resize(1200, 800);
        
        // 创建控制器
        m_controller = new PlayInterfaceController(this, this);
        
        // 设置自定义进度条
        setupProgressBar();
        
        // 设置连接
        setupConnections();
        
        // 设置UI
        setupUI();
        
        // 设置可视化
        setupVisualization();
        
        // 初始化控制器
        if (m_controller) {
            m_controller->initialize();
        }
        
        // 初始化平衡设置
        if (m_audioEngine) {
            double currentBalance = m_audioEngine->getBalance();
            m_balance = static_cast<int>(currentBalance * 100);
            updateBalanceDisplay();
        }
        
        qDebug() << "PlayInterface: Initialization completed successfully";
        
    } catch (const std::exception& e) {
        qCritical() << "PlayInterface: Exception during initialization:" << e.what();
    } catch (...) {
        qCritical() << "PlayInterface: Unknown exception during initialization";
    }
}

PlayInterface::~PlayInterface()
{
    // 注销AudioEngine界面
    if (m_audioEngine) {
        m_audioEngine->unregisterInterface(this);
    }
    
    if (m_controller) {
        m_controller->shutdown();
        delete m_controller;
        m_controller = nullptr;
    }
    if (ui) {
        delete ui;
        ui = nullptr;
    }
}

// AudioEngine设置 - 修复方法
void PlayInterface::setAudioEngine(AudioEngine* audioEngine)
{
    // 先注销旧的AudioEngine
    if (m_audioEngine) {
        m_audioEngine->unregisterInterface(this);
        
        // 断开之前的连接
        disconnect(m_audioEngine, nullptr, this, nullptr);
        if (m_customProgressBar) {
            disconnect(m_customProgressBar, nullptr, m_audioEngine, nullptr);
            disconnect(m_audioEngine, nullptr, m_customProgressBar, nullptr);
        }
    }
    
    m_audioEngine = audioEngine;
    
    if (m_audioEngine) {
        // 注册到AudioEngine
        m_audioEngine->registerInterface(this);
        
        // 使用DirectConnection避免信号循环，直接更新UI
        connect(m_audioEngine, &AudioEngine::positionChanged, 
                this, [this](qint64 position) {
                    setCurrentTime(position);
                    if (m_customProgressBar) {
                        m_customProgressBar->setPosition(position);
                    }
                }, Qt::DirectConnection);
        
        connect(m_audioEngine, &AudioEngine::durationChanged, 
                this, [this](qint64 duration) {
                    setTotalTime(duration);
                    if (m_customProgressBar) {
                        m_customProgressBar->setDuration(duration);
                    }
                }, Qt::DirectConnection);
        
        connect(m_audioEngine, &AudioEngine::balanceChanged, 
                this, [this](double balance) {
                    int balanceInt = static_cast<int>(balance * 100);
                    setBalance(balanceInt);
                    updateBalanceDisplay();
                }, Qt::DirectConnection);
        
        connect(m_audioEngine, &AudioEngine::stateChanged, 
                this, [this](AudioTypes::AudioState state) {
                    bool isPlaying = (state == AudioTypes::AudioState::Playing);
                    setPlaybackState(isPlaying);
                }, Qt::DirectConnection);
        
        connect(m_audioEngine, &AudioEngine::volumeChanged, 
                this, [this](int volume) {
                    setVolume(volume);
                }, Qt::DirectConnection);
        
        connect(m_audioEngine, &AudioEngine::mutedChanged, 
                this, [this](bool muted) {
                    setMuted(muted);
                }, Qt::DirectConnection);
        
        connect(m_audioEngine, &AudioEngine::playModeChanged, 
                this, [this](AudioTypes::PlayMode mode) {
                    updatePlayModeDisplay(mode);
                }, Qt::DirectConnection);
        
        connect(m_audioEngine, &AudioEngine::currentSongChanged, 
                this, [this](const Song& song) {
                    if (song.isValid()) {
                        setSongTitle(song.title());
                        setSongArtist(song.artist());
                        setSongAlbum(song.album());
                    }
                }, Qt::DirectConnection);
        
        connect(m_audioEngine, &AudioEngine::audioEngineTypeChanged, 
                this, &PlayInterface::onAudioEngineTypeChanged, Qt::DirectConnection);
        
        // 添加VU表信号连接
        connect(m_audioEngine, &AudioEngine::vuLevelsChanged, 
                this, [this](const QVector<double>& levels) {
                    if (levels.size() >= 2) {
                        updateVUMeter(levels[0], levels[1]);
                    }
                }, Qt::DirectConnection);
        
        // 自定义进度条连接 - 使用QueuedConnection避免阻塞
        if (m_customProgressBar) {
            connect(m_customProgressBar, &MusicProgressBar::seekRequested, 
                    m_audioEngine, &AudioEngine::seek, Qt::QueuedConnection);
        }
        
        // 同步当前状态
        setCurrentTime(m_audioEngine->position());
        setTotalTime(m_audioEngine->duration());
        setVolume(m_audioEngine->volume());
        setMuted(m_audioEngine->isMuted());
        
        // 同步当前歌曲信息
        Song currentSong = m_audioEngine->currentSong();
        if (currentSong.isValid()) {
            setSongTitle(currentSong.title());
            setSongArtist(currentSong.artist());
            setSongAlbum(currentSong.album());
        }
        
        // 同步播放模式
        updatePlayModeDisplay(m_audioEngine->playMode());
        
        // 同步自定义进度条状态
        if (m_customProgressBar) {
            m_customProgressBar->setPosition(m_audioEngine->position());
            m_customProgressBar->setDuration(m_audioEngine->duration());
        }
        
        // 获取当前音频引擎类型并更新按钮
        m_currentEngineType = m_audioEngine->getAudioEngineType();
        onAudioEngineTypeChanged(m_currentEngineType);
        
        // 加载平衡设置
        m_audioEngine->loadBalanceSettings();
        
        // 获取当前平衡设置并更新UI
        double currentBalance = m_audioEngine->getBalance();
        m_balance = static_cast<int>(currentBalance * 100);
        updateBalanceDisplay();
        
        qDebug() << "PlayInterface: 平衡设置已加载，当前值:" << m_balance;
        
        qDebug() << "PlayInterface: AudioEngine连接完成";
    }
}

// 设置自定义进度条 - 修复方法
void PlayInterface::setupProgressBar()
{
    if (!ui || !ui->slider_progress) {
        qDebug() << "PlayInterface: UI组件未初始化，跳过自定义进度条设置";
        return;
    }
    
    // 创建自定义进度条
    m_customProgressBar = new MusicProgressBar(this);
    m_customProgressBar->setObjectName("customProgressBar");
    
    // 获取原始滑块的几何属性和父布局
    QRect originalGeometry = ui->slider_progress->geometry();
    QWidget* parentWidget = ui->slider_progress->parentWidget();
    QLayout* parentLayout = parentWidget ? parentWidget->layout() : nullptr;
    
    if (parentWidget && parentLayout) {
        // 隐藏UI文件中的时间标签，因为自定义进度条有自己的时间显示
        if (ui->label_current_time) {
            ui->label_current_time->hide();
        }
        if (ui->label_total_time) {
            ui->label_total_time->hide();
        }
        
        // 从布局中移除原始滑块
        parentLayout->removeWidget(ui->slider_progress);
        ui->slider_progress->hide();
        
        // 将自定义进度条添加到布局中
        parentLayout->addWidget(m_customProgressBar);
        
        // 设置初始范围和位置
        m_customProgressBar->setRange(0, 1000);  // 默认范围
        m_customProgressBar->setPosition(0);
        
        // 设置大小策略
        m_customProgressBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_customProgressBar->setMinimumHeight(originalGeometry.height());
        
        qDebug() << "PlayInterface: 自定义进度条设置完成，隐藏了重复的时间标签";
    } else {
        qDebug() << "PlayInterface: 无法获取原始滑块的父布局";
        delete m_customProgressBar;
        m_customProgressBar = nullptr;
    }
}

// UI状态同步方法实现
void PlayInterface::setPlaybackState(bool isPlaying)
{
    m_isPlaying = isPlaying;
    updatePlaybackControls();
}

void PlayInterface::setCurrentTime(qint64 time)
{
    m_currentTime = time;
    
    // 更新UI进度条（如果存在且未被拖动）
    if (ui && ui->slider_progress && !ui->slider_progress->isSliderDown()) {
        ui->slider_progress->setValue(static_cast<int>(time));
    }
    
    // 更新自定义进度条（通过AudioEngine信号自动更新）
    // 这里不需要手动更新，因为AudioEngine会发送positionChanged信号
    
    updateTimeDisplay();
}

void PlayInterface::setTotalTime(qint64 time)
{
    m_totalTime = time;
    
    // 更新UI进度条
    if (ui && ui->slider_progress) {
        ui->slider_progress->setMaximum(static_cast<int>(time));
    }
    
    // 更新自定义进度条（通过AudioEngine信号自动更新）
    // 这里不需要手动更新，因为AudioEngine会发送durationChanged信号
    
    updateTimeDisplay();
}

void PlayInterface::setVolume(int volume)
{
    if (m_volume != volume) {
        m_volume = volume;
        
        // 更新滑块值（不触发信号）
        if (ui && ui->slider_main_volume) {
            ui->slider_main_volume->blockSignals(true);
            ui->slider_main_volume->setValue(volume);
            ui->slider_main_volume->blockSignals(false);
        }
        
        // 更新显示
        updateVolumeDisplay();
    }
}

void PlayInterface::setVolumeSliderValue(int value)
{
    m_volume = value;
    if (ui && ui->slider_main_volume) {
        ui->slider_main_volume->blockSignals(true);
        ui->slider_main_volume->setValue(value);
        ui->slider_main_volume->blockSignals(false);
    }
    updateVolumeDisplay();
}

void PlayInterface::setBalance(int balance)
{
    m_balance = balance;
    // 如果有平衡控制滑块，在这里更新
}

void PlayInterface::setMuted(bool muted)
{
    m_isMuted = muted;
    // 更新音量显示
    updateVolumeDisplay();
}

void PlayInterface::setSongTitle(const QString& title)
{
    if (ui && ui->label_current_song_title) {
        ui->label_current_song_title->setText(title);
    }
}

void PlayInterface::setSongArtist(const QString& artist)
{
    if (ui && ui->label_current_song_artist) {
        ui->label_current_song_artist->setText(artist);
    }
}

void PlayInterface::setSongAlbum(const QString& album)
{
    if (ui && ui->label_current_song_album) {
        ui->label_current_song_album->setText(album);
    }
}

void PlayInterface::setSongCover(const QPixmap& cover)
{
    if (ui && ui->label_album_cover) {
        ui->label_album_cover->setPixmap(cover);
    }
}

void PlayInterface::setLyrics(const QString& lyrics)
{
    if (ui && ui->textEdit_lyrics) {
        ui->textEdit_lyrics->setText(lyrics);
    }
}

void PlayInterface::updateWaveform(const QVector<float>& data)
{
    Q_UNUSED(data);
    // 实现将在后续添加
}

void PlayInterface::updateSpectrum(const QVector<float>& data)
{
    Q_UNUSED(data);
    // 实现将在后续添加
}

void PlayInterface::updateVUMeter(float leftLevel, float rightLevel)
{
    Q_UNUSED(leftLevel);
    Q_UNUSED(rightLevel);
    // 实现将在后续添加
}

void PlayInterface::setDisplayMode(int mode)
{
    m_displayMode = mode;
    updateDisplayMode();
}

void PlayInterface::setEqualizerValues(const QVector<int>& values)
{
    Q_UNUSED(values);
    // 实现将在后续添加
}

QVector<int> PlayInterface::getEqualizerValues() const
{
    return m_equalizerValues;
}

void PlayInterface::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    
    // 显示时同步当前歌曲信息
    if (m_controller && m_audioEngine) {
        Song currentSong = m_audioEngine->currentSong();
        if (currentSong.isValid()) {
            m_controller->setCurrentSong(currentSong);
        }
    }
}

void PlayInterface::updatePlayModeButton(const QString& text, const QString& iconPath, const QString& tooltip)
{
    if (ui && ui->pushButton_play_mode) {
        ui->pushButton_play_mode->setText(text);
        ui->pushButton_play_mode->setIcon(QIcon(iconPath));
        ui->pushButton_play_mode->setToolTip(tooltip);
    }
}

// 播放模式显示更新 - 新增方法
void PlayInterface::updatePlayModeDisplay(AudioTypes::PlayMode mode)
{
    if (!ui || !ui->pushButton_play_mode) return;
    
    QString iconPath;
    QString tooltip;
    
    switch (mode) {
    case AudioTypes::PlayMode::Loop:
        iconPath = ":/new/prefix1/images/listCycle.png";
        tooltip = "播放模式：列表循环";
        break;
    case AudioTypes::PlayMode::RepeatOne:
        iconPath = ":/new/prefix1/images/singleCycle.png";
        tooltip = "播放模式：单曲循环";
        break;
    case AudioTypes::PlayMode::Random:
        iconPath = ":/new/prefix1/images/shufflePlay.png";
        tooltip = "播放模式：随机播放";
        break;
    default:
        iconPath = ":/new/prefix1/images/listCycle.png";
        tooltip = "播放模式：列表循环";
        break;
    }
    
    ui->pushButton_play_mode->setIcon(QIcon(iconPath));
    ui->pushButton_play_mode->setToolTip(tooltip);
    ui->pushButton_play_mode->setIconSize(QSize(28, 28));
    
    qDebug() << "PlayInterface: 播放模式显示更新为" << static_cast<int>(mode);
}

// 进度条控制方法
void PlayInterface::setProgressBarPosition(qint64 position)
{
    setCurrentTime(position);
}

void PlayInterface::setProgressBarDuration(qint64 duration)
{
    setTotalTime(duration);
}

void PlayInterface::updateProgressDisplay()
{
    updateTimeDisplay();
}

// 音量控制方法
void PlayInterface::updateVolumeControls()
{
    updateVolumeDisplay();
}

void PlayInterface::onVolumeSliderChanged(int value)
{
    // 避免信号循环：只有当值真正改变时才设置
    if (m_volume != value) {
        m_volume = value;
        
        if (m_audioEngine) {
            // 临时断开信号连接避免循环
            m_audioEngine->blockSignals(true);
            m_audioEngine->setVolume(value);
            m_audioEngine->blockSignals(false);
            qDebug() << "PlayInterface: 设置音量到" << value;
        }
        
        // 更新UI显示
        updateVolumeLabel(value);
        
        // 发送音量变化信号
        emit volumeChanged(value);
    }
}

void PlayInterface::onVolumeSliderValueChanged(int value)
{
    // 避免重复调用
    if (m_volume != value) {
        onVolumeSliderChanged(value);
    }
}

void PlayInterface::updateVolumeLabel(int value)
{
    if (ui && ui->label_volume_value) {
        ui->label_volume_value->setText(QString("%1%").arg(value));
    }
}

void PlayInterface::updateMuteButtonIcon()
{
    // 实现将在后续添加
}

// 槽函数实现 - 直接操作AudioEngine，采用与主界面相同的逻辑
void PlayInterface::onPlayPauseClicked()
{
    try {
        qDebug() << "PlayInterface: 播放/暂停按钮点击";
        
        if (m_audioEngine) {
            // 检查当前状态
            AudioTypes::AudioState currentState = m_audioEngine->state();
            qDebug() << "PlayInterface: 当前音频状态:" << static_cast<int>(currentState);
            
            // 检查播放列表
            int playlistSize = m_audioEngine->playlist().size();
            int currentIndex = m_audioEngine->currentIndex();
            qDebug() << "PlayInterface: 当前播放列表大小:" << playlistSize;
            qDebug() << "PlayInterface: 当前播放索引:" << currentIndex;
            
            // 如果播放列表为空或索引无效
            if (playlistSize == 0 || currentIndex < 0) {
                qDebug() << "PlayInterface: 播放列表为空，显示提示";
                emit playPauseClicked(); // 发送信号让主界面处理
                return;
            }
            
            // 播放列表不为空，正常切换播放状态
            if (currentState == AudioTypes::AudioState::Playing) {
                m_audioEngine->pause();
            } else {
                m_audioEngine->play();
            }
        } else {
            emit playPauseClicked(); // 发送信号让主界面处理
        }
        
    } catch (const std::exception& e) {
        qCritical() << "PlayInterface: onPlayPauseClicked异常:" << e.what();
    } catch (...) {
        qCritical() << "PlayInterface: onPlayPauseClicked未知异常";
    }
}

void PlayInterface::onPlayModeClicked()
{
    qDebug() << "PlayInterface: 播放模式按钮点击";
    if (m_audioEngine) {
        // 循环切换播放模式
        AudioTypes::PlayMode currentMode = m_audioEngine->playMode();
        AudioTypes::PlayMode nextMode;
        
        switch (currentMode) {
        case AudioTypes::PlayMode::Loop:
            nextMode = AudioTypes::PlayMode::RepeatOne;
            break;
        case AudioTypes::PlayMode::RepeatOne:
            nextMode = AudioTypes::PlayMode::Random;
            break;
        case AudioTypes::PlayMode::Random:
            nextMode = AudioTypes::PlayMode::Loop;
            break;
        default:
            nextMode = AudioTypes::PlayMode::Loop;
            break;
        }
        
        m_audioEngine->setPlayMode(nextMode);
        qDebug() << "PlayInterface: 播放模式切换到" << static_cast<int>(nextMode);
    } else {
        emit playModeClicked();
    }
}

void PlayInterface::onNextClicked()
{
    qDebug() << "PlayInterface: 下一首按钮点击";
    if (!m_audioEngine) {
        emit nextClicked();
        return;
    }
    
    // 检查播放列表是否为空或当前索引无效
    int playlistSize = m_audioEngine->playlist().size();
    int currentIndex = m_audioEngine->currentIndex();
    
    qDebug() << "PlayInterface: 播放列表大小:" << playlistSize;
    qDebug() << "PlayInterface: 当前索引:" << currentIndex;
    
    // 如果播放列表为空或索引无效
    if (playlistSize == 0 || currentIndex < 0) {
        qDebug() << "PlayInterface: 播放列表为空，显示提示";
        emit nextClicked(); // 发送信号让主界面处理
        return;
    }
    
    // 播放列表不为空，正常切换到下一首
    m_audioEngine->playNext();
    qDebug() << "PlayInterface: 发送下一首请求";
}

void PlayInterface::onPreviousClicked()
{
    qDebug() << "PlayInterface: 上一首按钮点击";
    if (!m_audioEngine) {
        emit previousClicked();
        return;
    }
    
    // 检查播放列表是否为空或当前索引无效
    int playlistSize = m_audioEngine->playlist().size();
    int currentIndex = m_audioEngine->currentIndex();
    
    qDebug() << "PlayInterface: 播放列表大小:" << playlistSize;
    qDebug() << "PlayInterface: 当前索引:" << currentIndex;
    
    // 如果播放列表为空或索引无效
    if (playlistSize == 0 || currentIndex < 0) {
        qDebug() << "PlayInterface: 播放列表为空，显示提示";
        emit previousClicked(); // 发送信号让主界面处理
        return;
    }
    
    // 播放列表不为空，正常切换到上一首
    m_audioEngine->playPrevious();
    qDebug() << "PlayInterface: 发送上一首请求";
}

void PlayInterface::onMuteButtonPressed()
{
    qDebug() << "PlayInterface: 静音按钮点击";
    if (m_audioEngine) {
        m_audioEngine->toggleMute();
        qDebug() << "PlayInterface: 切换静音状态";
    } else {
        m_isMuted = !m_isMuted;
        updateMuteButtonState();
        emit muteButtonClicked();
    }
}

// 进度条控制槽函数
void PlayInterface::onProgressSliderPressed()
{
    if (m_customProgressBar) {
        emit progressSliderPressed();
    }
}

void PlayInterface::onProgressSliderReleased()
{
    if (m_customProgressBar) {
        emit progressSliderReleased();
    }
}

void PlayInterface::onProgressSliderMoved(int value)
{
    if (m_customProgressBar) {
        qint64 position = (static_cast<qint64>(value) * m_totalTime) / 1000;
        emit positionChanged(position);
    }
}

void PlayInterface::onPositionSliderChanged(int value)
{
    if (m_customProgressBar) {
        qint64 position = (static_cast<qint64>(value) * m_totalTime) / 1000;
        emit seekRequested(position);
    }
}



void PlayInterface::onDisplayModeClicked()
{
    m_displayMode = (m_displayMode + 1) % 3;
    updateDisplayMode();
    emit displayModeClicked();
}

void PlayInterface::onVisualizationTypeClicked()
{
    emit visualizationTypeClicked();
}

void PlayInterface::onEqualizerSliderChanged()
{
    QVector<int> values = getEqualizerValues();
    emit equalizerChanged(values);
}

void PlayInterface::onLyricClicked(qint64 timestamp)
{
    emit lyricClicked(timestamp);
}

void PlayInterface::onUpdateTimer()
{
    updateTimeDisplay();
    updateVisualization();
}

void PlayInterface::onMuteButtonClicked()
{
    if (m_audioEngine) {
        m_audioEngine->toggleMute();
    } else {
        emit muteToggled(!m_isMuted);
    }
}

// 私有辅助方法
void PlayInterface::setupConnections()
{
    try {
        // 控制按钮连接
        connect(ui->pushButton_play_pause_song, &QPushButton::clicked, this, &PlayInterface::onPlayPauseClicked);
        connect(ui->pushButton_play_mode, &QPushButton::clicked, this, &PlayInterface::onPlayModeClicked);
        connect(ui->pushButton_previous_song, &QPushButton::clicked, this, &PlayInterface::onPreviousClicked);
        connect(ui->pushButton_next_song, &QPushButton::clicked, this, &PlayInterface::onNextClicked);
        
        // 音频引擎切换按钮连接
        connect(ui->pushButton_audio_engine, &QPushButton::clicked, this, &PlayInterface::onAudioEngineButtonClicked);
        
        // 进度条控件
        if (ui && ui->slider_progress) {
            connect(ui->slider_progress, &QSlider::sliderPressed, this, &PlayInterface::onProgressSliderPressed);
            connect(ui->slider_progress, &QSlider::sliderReleased, this, &PlayInterface::onProgressSliderReleased);
            connect(ui->slider_progress, &QSlider::sliderMoved, this, &PlayInterface::onProgressSliderMoved);
        }
        
        // 音量控件 - 修复：使用正确的控件名称
        if (ui && ui->slider_main_volume) {
            connect(ui->slider_main_volume, &QSlider::valueChanged, this, &PlayInterface::onVolumeSliderValueChanged);
        }
        
        // 平衡控件
        if (ui && ui->slider_balance) {
            connect(ui->slider_balance, &QSlider::valueChanged, this, &PlayInterface::onBalanceSliderChanged);
        }
        
    } catch (const std::exception& e) {
        qCritical() << "PlayInterface: Exception in setupConnections:" << e.what();
    }
}

void PlayInterface::setupUI()
{
    setWindowTitle("播放界面");
}

void PlayInterface::setupVisualization()
{
    // 实现将在后续添加
}

void PlayInterface::updateTimeDisplay()
{
    if (ui) {
        if (ui->label_current_time) {
            ui->label_current_time->setText(formatTime(m_currentTime));
        }
        if (ui->label_total_time) {
            ui->label_total_time->setText(formatTime(m_totalTime));
        }
    }
}

void PlayInterface::updateVolumeDisplay()
{
    if (ui) {
        // 更新音量标签
        updateVolumeLabel(m_volume);
    }
}

void PlayInterface::updatePlaybackControls()
{
    if (ui && ui->pushButton_play_pause_song) {
        // 更新播放/暂停按钮图标
        ui->pushButton_play_pause_song->setIcon(QIcon(m_isPlaying ? ":/new/prefix1/images/pauseIcon.png" : ":/new/prefix1/images/playIcon.png"));
        
        // 更新按钮样式
        QString styleSheet;
        if (m_isPlaying) {
            // 播放状态：蓝色背景
            styleSheet = "QPushButton#pushButton_play_pause_song { "
                        "background-color: #0078d4; "
                        "border: 2px solid #005a9e; "
                        "border-radius: 8px; "
                        "padding: 0px; "
                        "min-width: 50px; "
                        "max-width: 50px; "
                        "min-height: 50px; "
                        "max-height: 50px; "
                        "} "
                        "QPushButton#pushButton_play_pause_song:hover { "
                        "background-color: #005a9e; "
                        "border-color: #004578; "
                        "} "
                        "QPushButton#pushButton_play_pause_song:pressed { "
                        "background-color: #004578; "
                        "border-color: #003366; "
                        "}";
        } else {
            // 暂停状态：深色背景
            styleSheet = "QPushButton#pushButton_play_pause_song { "
                        "background-color: #2d2d2d; "
                        "border: 2px solid #0078d4; "
                        "border-radius: 8px; "
                        "padding: 0px; "
                        "min-width: 50px; "
                        "max-width: 50px; "
                        "min-height: 50px; "
                        "max-height: 50px; "
                        "} "
                        "QPushButton#pushButton_play_pause_song:hover { "
                        "background-color: #0078d4; "
                        "border-color: #005a9e; "
                        "} "
                        "QPushButton#pushButton_play_pause_song:pressed { "
                        "background-color: #005a9e; "
                        "border-color: #004578; "
                        "}";
        }
        ui->pushButton_play_pause_song->setStyleSheet(styleSheet);
    }
}

void PlayInterface::updateMuteButtonState()
{
    // 实现将在后续添加
}

void PlayInterface::updateDisplayMode()
{
    // 实现将在后续添加
}

void PlayInterface::updateVisualization()
{
    // 实现将在后续添加
}

void PlayInterface::updateEqualizerDisplay()
{
    // 实现将在后续添加
}

void PlayInterface::updateLyricDisplay()
{
    // 实现将在后续添加
}

QString PlayInterface::formatTime(qint64 milliseconds) const
{
    int seconds = milliseconds / 1000;
    int minutes = seconds / 60;
    seconds = seconds % 60;
    return QString("%1:%2").arg(minutes).arg(seconds, 2, 10, QChar('0'));
}

// ==================== VU表和平衡控制相关方法 ====================

void PlayInterface::updateVUMeterLevels(const QVector<double>& levels)
{
    m_vuLevels = levels;
    updateVUMeterDisplay();
}

void PlayInterface::updateVUMeterDisplay()
{
    if (!ui) return;
    
    // 查找VU表组件
    QProgressBar* leftVU = findChild<QProgressBar*>("progressBar_left_channel");
    QProgressBar* rightVU = findChild<QProgressBar*>("progressBar_right_channel");
    
    if (leftVU && rightVU && m_vuLevels.size() >= 2) {
        // 更新左声道VU表
        int leftValue = static_cast<int>(m_vuLevels[0] * 100);
        leftVU->setValue(leftValue);
        
        // 更新右声道VU表
        int rightValue = static_cast<int>(m_vuLevels[1] * 100);
        rightVU->setValue(rightValue);
        
        // 根据电平设置颜色
        if (leftValue > 80) {
            leftVU->setStyleSheet("QProgressBar::chunk { background-color: #ff4444; }");
        } else if (leftValue > 60) {
            leftVU->setStyleSheet("QProgressBar::chunk { background-color: #ffaa00; }");
        } else {
            leftVU->setStyleSheet("QProgressBar::chunk { background-color: #00aa00; }");
        }
        
        if (rightValue > 80) {
            rightVU->setStyleSheet("QProgressBar::chunk { background-color: #ff4444; }");
        } else if (rightValue > 60) {
            rightVU->setStyleSheet("QProgressBar::chunk { background-color: #00aa00; }");
        } else {
            rightVU->setStyleSheet("QProgressBar::chunk { background-color: #00aa00; }");
        }
    }
}



void PlayInterface::onBalanceSliderChanged(int value)
{
    m_balance = value;
    
    // 发送信号给AudioEngine
    if (m_audioEngine) {
        // 将-100到100的范围转换为-1.0到1.0
        double balance = value / 100.0;
        m_audioEngine->setBalance(balance);
    }
    
    // 更新显示
    updateBalanceDisplay();
    
    // 保存设置
    if (m_audioEngine) {
        m_audioEngine->saveBalanceSettings();
    }
    
    emit balanceChanged(value);
}

// ==================== 音频引擎切换实现 ====================

void PlayInterface::onAudioEngineButtonClicked()
{
    try {
        if (!m_audioEngine) {
            qWarning() << "PlayInterface: AudioEngine未设置，无法切换音频引擎";
            return;
        }
        
        // 切换音频引擎类型
        AudioTypes::AudioEngineType newType;
        if (m_currentEngineType == AudioTypes::AudioEngineType::QMediaPlayer) {
            newType = AudioTypes::AudioEngineType::FFmpeg;
        } else {
            newType = AudioTypes::AudioEngineType::QMediaPlayer;
        }
        
        qDebug() << "PlayInterface: 用户点击切换音频引擎，从" 
                 << (m_currentEngineType == AudioTypes::AudioEngineType::QMediaPlayer ? "QMediaPlayer" : "FFmpeg")
                 << "切换到" 
                 << (newType == AudioTypes::AudioEngineType::QMediaPlayer ? "QMediaPlayer" : "FFmpeg");
        
        // 使用QueuedConnection确保在主线程中执行
        QMetaObject::invokeMethod(this, [this, newType]() {
            try {
                m_audioEngine->setAudioEngineType(newType);
            } catch (const std::exception& e) {
                qCritical() << "PlayInterface: 切换音频引擎异常:" << e.what();
            } catch (...) {
                qCritical() << "PlayInterface: 切换音频引擎未知异常";
            }
        }, Qt::QueuedConnection);
        
    } catch (const std::exception& e) {
        qCritical() << "PlayInterface: onAudioEngineButtonClicked异常:" << e.what();
    } catch (...) {
        qCritical() << "PlayInterface: onAudioEngineButtonClicked未知异常";
    }
}

void PlayInterface::onAudioEngineTypeChanged(AudioTypes::AudioEngineType type)
{
    try {
        m_currentEngineType = type;
        
        // 更新按钮文本和样式
        if (ui && ui->pushButton_audio_engine) {
            QString buttonText;
            QString tooltipText;
            bool isFFmpeg = (type == AudioTypes::AudioEngineType::FFmpeg);
            
            if (isFFmpeg) {
                buttonText = "FFmpeg";
                tooltipText = "当前使用FFmpeg引擎播放\n支持实时音效处理（平衡控制等）\n点击切换到QMediaPlayer";
                ui->pushButton_audio_engine->setChecked(true);
            } else {
                buttonText = "QMediaPlayer";
                tooltipText = "当前使用QMediaPlayer播放\n不受音效处理影响，音质纯净\n点击切换到FFmpeg";
                ui->pushButton_audio_engine->setChecked(false);
            }
            
            ui->pushButton_audio_engine->setText(buttonText);
            ui->pushButton_audio_engine->setToolTip(tooltipText);
            
            qDebug() << "PlayInterface: 音频引擎按钮更新为:" << buttonText;
        }
        
        // 更新平衡控制的可用性提示
        updateBalanceDisplay();
        
    } catch (const std::exception& e) {
        qCritical() << "PlayInterface: onAudioEngineTypeChanged异常:" << e.what();
    } catch (...) {
        qCritical() << "PlayInterface: onAudioEngineTypeChanged未知异常";
    }
}

void PlayInterface::updateBalanceDisplay()
{
    if (!ui || !ui->label_balance_value) {
        return;
    }
    
    // 显示当前平衡值
    QString balanceText;
    if (m_balance < 0) {
        balanceText = QString("平衡: 左 %1%").arg(-m_balance);
    } else if (m_balance > 0) {
        balanceText = QString("平衡: 右 %1%").arg(m_balance);
    } else {
        balanceText = "平衡: 中央";
    }
    
    // 根据音频引擎类型添加提示
    if (m_currentEngineType == AudioTypes::AudioEngineType::FFmpeg) {
        balanceText += " (已生效)";
        ui->label_balance_value->setStyleSheet("color: #81A1C1;"); // 蓝色表示生效
    } else {
        balanceText += " (切换到FFmpeg生效)";
        ui->label_balance_value->setStyleSheet("color: #D08770;"); // 橙色表示未生效
    }
    
    ui->label_balance_value->setText(balanceText);
}

void PlayInterface::mousePressEvent(QMouseEvent *event)
{
    try {
        // 检查是否在FFmpeg模式下
        if (m_audioEngine && m_currentEngineType == AudioTypes::AudioEngineType::FFmpeg) {
            // FFmpeg模式下，增加额外的安全检查
            if (!m_audioEngine->isValid()) {
                qWarning() << "PlayInterface: FFmpeg模式下AudioEngine无效，忽略鼠标事件";
                event->ignore();
                return;
            }
        }
        
        // 正常处理鼠标事件
        QDialog::mousePressEvent(event);
        
    } catch (const std::exception& e) {
        qCritical() << "PlayInterface: mousePressEvent异常:" << e.what();
        event->ignore();
    } catch (...) {
        qCritical() << "PlayInterface: mousePressEvent未知异常";
        event->ignore();
    }
}

void PlayInterface::mouseReleaseEvent(QMouseEvent *event)
{
    try {
        // 检查是否在FFmpeg模式下
        if (m_audioEngine && m_currentEngineType == AudioTypes::AudioEngineType::FFmpeg) {
            // FFmpeg模式下，增加额外的安全检查
            if (!m_audioEngine->isValid()) {
                qWarning() << "PlayInterface: FFmpeg模式下AudioEngine无效，忽略鼠标事件";
                event->ignore();
                return;
            }
        }
        
        // 正常处理鼠标事件
        QDialog::mouseReleaseEvent(event);
        
    } catch (const std::exception& e) {
        qCritical() << "PlayInterface: mouseReleaseEvent异常:" << e.what();
        event->ignore();
    } catch (...) {
        qCritical() << "PlayInterface: mouseReleaseEvent未知异常";
        event->ignore();
    }
}

void PlayInterface::mouseMoveEvent(QMouseEvent *event)
{
    try {
        // 检查是否在FFmpeg模式下
        if (m_audioEngine && m_currentEngineType == AudioTypes::AudioEngineType::FFmpeg) {
            // FFmpeg模式下，增加额外的安全检查
            if (!m_audioEngine->isValid()) {
                qWarning() << "PlayInterface: FFmpeg模式下AudioEngine无效，忽略鼠标事件";
                event->ignore();
                return;
            }
        }
        
        // 正常处理鼠标事件
        QDialog::mouseMoveEvent(event);
        
    } catch (const std::exception& e) {
        qCritical() << "PlayInterface: mouseMoveEvent异常:" << e.what();
        event->ignore();
    } catch (...) {
        qCritical() << "PlayInterface: mouseMoveEvent未知异常";
        event->ignore();
    }
}