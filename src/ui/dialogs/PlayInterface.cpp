#include "PlayInterface.h"
#include "ui_PlayInterface.h"
#include "../controllers/playinterfacecontroller.h"
#include "../widgets/musicprogressbar.h"
#include "../../audio/audioengine.h"

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
    , m_currentLyricIndex(-1)
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
        
        qDebug() << "PlayInterface: Initialization completed successfully";
        
    } catch (const std::exception& e) {
        qCritical() << "PlayInterface: Exception during initialization:" << e.what();
    } catch (...) {
        qCritical() << "PlayInterface: Unknown exception during initialization";
    }
}

PlayInterface::~PlayInterface()
{
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
    if (m_audioEngine) {
        // 断开之前的连接
        disconnect(m_audioEngine, nullptr, this, nullptr);
        if (m_customProgressBar) {
            disconnect(m_audioEngine, nullptr, m_customProgressBar, nullptr);
            disconnect(m_customProgressBar, nullptr, m_audioEngine, nullptr);
        }
    }
    
    m_audioEngine = audioEngine;
    
    // 设置给控制器
    if (m_controller) {
        m_controller->setAudioEngine(audioEngine);
        
        // 同步当前歌曲信息
        if (audioEngine) {
            Song currentSong = audioEngine->currentSong();
            if (currentSong.isValid()) {
                m_controller->setCurrentSong(currentSong);
            }
        }
    }
    
    // 设置给自定义进度条
    if (m_customProgressBar && m_audioEngine) {
        // 连接自定义进度条信号到PlayInterface
        connect(m_customProgressBar, &MusicProgressBar::sliderPressed, 
                this, &PlayInterface::onProgressSliderPressed);
        connect(m_customProgressBar, &MusicProgressBar::sliderReleased, 
                this, &PlayInterface::onProgressSliderReleased);
        connect(m_customProgressBar, &MusicProgressBar::positionChanged, 
                this, [this](qint64 position) {
                    emit positionChanged(position);
                });
        connect(m_customProgressBar, &MusicProgressBar::seekRequested, 
                this, [this](qint64 position) {
                    if (m_audioEngine) {
                        qDebug() << "PlayInterface: 自定义进度条请求跳转到" << position;
                        m_audioEngine->seek(position);
                    }
                });
        
        // 连接AudioEngine信号到自定义进度条
        connect(m_audioEngine, &AudioEngine::positionChanged, 
                m_customProgressBar, &MusicProgressBar::setPosition);
        connect(m_audioEngine, &AudioEngine::durationChanged, 
                m_customProgressBar, &MusicProgressBar::setDuration);
    }
    
    // 连接AudioEngine状态信号到PlayInterface
    if (m_audioEngine) {
        connect(m_audioEngine, &AudioEngine::stateChanged, this, [this](AudioTypes::AudioState state) {
            setPlaybackState(state == AudioTypes::AudioState::Playing);
        });
        
        connect(m_audioEngine, &AudioEngine::positionChanged, this, &PlayInterface::setCurrentTime);
        connect(m_audioEngine, &AudioEngine::durationChanged, this, &PlayInterface::setTotalTime);
        connect(m_audioEngine, &AudioEngine::volumeChanged, this, [this](int volume) {
            // 避免信号循环
            if (m_volume != volume) {
                m_volume = volume;
                setVolume(volume);
            }
        });
        connect(m_audioEngine, &AudioEngine::mutedChanged, this, &PlayInterface::setMuted);
        connect(m_audioEngine, &AudioEngine::currentSongChanged, this, [this](const Song& song) {
            setSongTitle(song.title());
            setSongArtist(song.artist());
            setSongAlbum(song.album());
        });
        
        // 添加播放模式同步
        connect(m_audioEngine, &AudioEngine::playModeChanged, this, [this](AudioTypes::PlayMode mode) {
            updatePlayModeDisplay(mode);
        });
        
        // 同步当前状态
        setPlaybackState(m_audioEngine->state() == AudioTypes::AudioState::Playing);
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
    qDebug() << "PlayInterface: 播放/暂停按钮点击";
    if (!m_audioEngine) {
        qDebug() << "PlayInterface: AudioEngine未设置";
        emit playPauseClicked();
        return;
    }
    
    // 采用与主界面相同的逻辑
    AudioTypes::AudioState currentState = m_audioEngine->state();
    int playlistSize = m_audioEngine->playlist().size();
    int currentIndex = m_audioEngine->currentIndex();
    
    qDebug() << "PlayInterface: 当前音频状态:" << static_cast<int>(currentState);
    qDebug() << "PlayInterface: 当前播放列表大小:" << playlistSize;
    qDebug() << "PlayInterface: 当前播放索引:" << currentIndex;
    
    // 首先检查播放列表是否为空或索引无效
    if (playlistSize == 0 || currentIndex < 0) {
        qDebug() << "PlayInterface: 播放列表为空，显示提示";
        // 这里应该触发主界面的 startNewPlayback 逻辑
        // 但由于播放界面独立，只能显示提示
        emit playPauseClicked(); // 发送信号让主界面处理
        return;
    }
    
    // 播放列表不为空时的播放/暂停逻辑
    switch (currentState) {
    case AudioTypes::AudioState::Playing:
        m_audioEngine->pause();
        qDebug() << "PlayInterface: 发送暂停请求";
        break;
        
    case AudioTypes::AudioState::Paused:
        m_audioEngine->play();
        qDebug() << "PlayInterface: 发送播放请求";
        break;
        
    case AudioTypes::AudioState::Loading:
        qDebug() << "PlayInterface: 正在加载媒体文件...";
        break;
        
    case AudioTypes::AudioState::Error:
        m_audioEngine->play();
        qDebug() << "PlayInterface: 错误状态，尝试重新播放";
        break;
        
    default:
        m_audioEngine->play();
        qDebug() << "PlayInterface: 默认播放";
        break;
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

void PlayInterface::onBalanceSliderChanged(int value)
{
    if (m_audioEngine) {
        m_audioEngine->setBalance(value / 100.0);
    }
    emit balanceChanged(value);
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
        // 连接播放控制按钮
        if (ui && ui->pushButton_play_pause_song) {
            connect(ui->pushButton_play_pause_song, &QPushButton::clicked, this, &PlayInterface::onPlayPauseClicked);
        }
        
        if (ui && ui->pushButton_previous_song) {
            connect(ui->pushButton_previous_song, &QPushButton::clicked, this, &PlayInterface::onPreviousClicked);
        }
        
        if (ui && ui->pushButton_next_song) {
            connect(ui->pushButton_next_song, &QPushButton::clicked, this, &PlayInterface::onNextClicked);
        }
        
        if (ui && ui->pushButton_play_mode) {
            connect(ui->pushButton_play_mode, &QPushButton::clicked, this, &PlayInterface::onPlayModeClicked);
        }
        
        // 连接进度条控件
        if (ui && ui->slider_progress) {
            connect(ui->slider_progress, &QSlider::sliderPressed, this, &PlayInterface::onProgressSliderPressed);
            connect(ui->slider_progress, &QSlider::sliderReleased, this, &PlayInterface::onProgressSliderReleased);
            connect(ui->slider_progress, &QSlider::sliderMoved, this, &PlayInterface::onProgressSliderMoved);
        }
        
        // 连接音量控件
        if (ui && ui->slider_main_volume) {
            connect(ui->slider_main_volume, &QSlider::valueChanged, this, &PlayInterface::onVolumeSliderValueChanged);
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