#include "PlaybackControlWidget.h"
#include "../../audio/audioengine.h"
#include "../../core/Logger.h"
#include <QIcon>
#include <QToolTip>
#include <QApplication>
#include <QStyle>

PlaybackControlWidget::PlaybackControlWidget(QWidget *parent)
    : QWidget(parent)
    , m_mainLayout(nullptr)
    , m_verticalLayout(nullptr)
    , m_playPauseButton(nullptr)
    , m_previousButton(nullptr)
    , m_nextButton(nullptr)
    , m_playModeButton(nullptr)
    , m_progressBar(nullptr)
    , m_volumeFrame(nullptr)
    , m_volumeLayout(nullptr)
    , m_volumeSlider(nullptr)
    , m_muteButton(nullptr)
    , m_volumeLabel(nullptr)
    , m_isPlaying(false)
    , m_currentTime(0)
    , m_totalTime(0)
    , m_volume(50)
    , m_isMuted(false)
    , m_playMode(AudioTypes::PlayMode::ListLoop)
    , m_layoutMode(Horizontal)
    , m_audioEngine(nullptr)
{
    setupUI();
    setupConnections();
    applyStyles();
    
    Logger::instance()->info("PlaybackControlWidget 初始化完成");
}

PlaybackControlWidget::~PlaybackControlWidget()
{
    Logger::instance()->info("PlaybackControlWidget 销毁");
}

void PlaybackControlWidget::setupUI()
{
    // 创建主布局
    m_mainLayout = new QHBoxLayout(this);
    m_mainLayout->setContentsMargins(5, 5, 5, 5);
    m_mainLayout->setSpacing(10);
    
    // 创建播放控制按钮
    m_previousButton = new QPushButton(this);
    m_previousButton->setIcon(QIcon(":/new/prefix1/images/lastSongIcon.png"));
    m_previousButton->setIconSize(QSize(32, 32));
    m_previousButton->setToolTip("上一首");
    m_previousButton->setFixedSize(40, 40);
    
    m_playPauseButton = new QPushButton(this);
    m_playPauseButton->setIcon(QIcon(":/new/prefix1/images/playIcon.png"));
    m_playPauseButton->setIconSize(QSize(40, 40));
    m_playPauseButton->setToolTip("播放/暂停");
    m_playPauseButton->setFixedSize(48, 48);
    
    m_nextButton = new QPushButton(this);
    m_nextButton->setIcon(QIcon(":/new/prefix1/images/followingSongIcon.png"));
    m_nextButton->setIconSize(QSize(32, 32));
    m_nextButton->setToolTip("下一首");
    m_nextButton->setFixedSize(40, 40);
    
    m_playModeButton = new QPushButton(this);
    m_playModeButton->setIcon(QIcon(":/new/prefix1/images/listCycle.png"));
    m_playModeButton->setIconSize(QSize(32, 32));
    m_playModeButton->setToolTip("播放模式：列表循环");
    m_playModeButton->setFixedSize(40, 40);
    
    // 创建进度条
    m_progressBar = new MusicProgressBar(this);
    m_progressBar->setMinimumWidth(200);
    
    // 创建音量控制
    m_volumeFrame = new QFrame(this);
    m_volumeLayout = new QHBoxLayout(m_volumeFrame);
    m_volumeLayout->setContentsMargins(0, 0, 0, 0);
    m_volumeLayout->setSpacing(5);
    
    m_muteButton = new QPushButton(m_volumeFrame);
    m_muteButton->setIcon(QIcon(":/new/prefix1/images/volumeIcon.png"));
    m_muteButton->setIconSize(QSize(24, 24));
    m_muteButton->setToolTip("静音/取消静音");
    m_muteButton->setFixedSize(32, 32);
    
    m_volumeSlider = new QSlider(Qt::Horizontal, m_volumeFrame);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(50);
    m_volumeSlider->setFixedWidth(80);
    m_volumeSlider->setToolTip("音量控制");
    
    m_volumeLabel = new QLabel("50", m_volumeFrame);
    m_volumeLabel->setFixedWidth(30);
    m_volumeLabel->setAlignment(Qt::AlignCenter);
    
    m_volumeLayout->addWidget(m_muteButton);
    m_volumeLayout->addWidget(m_volumeSlider);
    m_volumeLayout->addWidget(m_volumeLabel);
    
    setupLayouts();
}

void PlaybackControlWidget::setupConnections()
{
    // 播放控制按钮连接
    connect(m_playPauseButton, &QPushButton::clicked, this, &PlaybackControlWidget::onPlayPauseButtonClicked);
    connect(m_previousButton, &QPushButton::clicked, this, &PlaybackControlWidget::onPreviousButtonClicked);
    connect(m_nextButton, &QPushButton::clicked, this, &PlaybackControlWidget::onNextButtonClicked);
    connect(m_playModeButton, &QPushButton::clicked, this, &PlaybackControlWidget::onPlayModeButtonClicked);
    
    // 进度条连接
    connect(m_progressBar, &MusicProgressBar::seekRequested, this, &PlaybackControlWidget::onProgressBarSeekRequested);
    connect(m_progressBar, &MusicProgressBar::positionChanged, this, &PlaybackControlWidget::onProgressBarPositionChanged);
    
    // 音量控制连接
    connect(m_volumeSlider, &QSlider::valueChanged, this, &PlaybackControlWidget::onVolumeSliderChanged);
    connect(m_muteButton, &QPushButton::clicked, this, &PlaybackControlWidget::onMuteButtonClicked);
}

void PlaybackControlWidget::setupLayouts()
{
    // 清空当前布局
    QLayoutItem* item;
    while ((item = m_mainLayout->takeAt(0)) != nullptr) {
        delete item;
    }
    
    if (m_layoutMode == Horizontal) {
        // 水平布局：播放按钮 | 进度条 | 音量控制
        m_mainLayout->addWidget(m_previousButton);
        m_mainLayout->addWidget(m_playPauseButton);
        m_mainLayout->addWidget(m_nextButton);
        m_mainLayout->addWidget(m_playModeButton);
        m_mainLayout->addStretch(1);
        m_mainLayout->addWidget(m_progressBar, 3);
        m_mainLayout->addStretch(1);
        m_mainLayout->addWidget(m_volumeFrame);
    } else if (m_layoutMode == Vertical) {
        // 垂直布局：播放按钮在上，进度条在中，音量控制在下
        if (!m_verticalLayout) {
            m_verticalLayout = new QVBoxLayout();
        }
        
        QHBoxLayout* buttonLayout = new QHBoxLayout();
        buttonLayout->addWidget(m_previousButton);
        buttonLayout->addWidget(m_playPauseButton);
        buttonLayout->addWidget(m_nextButton);
        buttonLayout->addWidget(m_playModeButton);
        
        m_verticalLayout->addLayout(buttonLayout);
        m_verticalLayout->addWidget(m_progressBar);
        m_verticalLayout->addWidget(m_volumeFrame);
        
        m_mainLayout->addLayout(m_verticalLayout);
    } else { // Compact
        // 紧凑布局：只显示核心控件
        m_mainLayout->addWidget(m_playPauseButton);
        m_mainLayout->addWidget(m_progressBar, 1);
        m_mainLayout->addWidget(m_muteButton);
        m_mainLayout->addWidget(m_volumeSlider);
    }
}

void PlaybackControlWidget::setAudioEngine(AudioEngine* audioEngine)
{
    m_audioEngine = audioEngine;
    
    if (m_audioEngine) {
        // 连接音频引擎信号以保持同步
        connect(m_audioEngine, &AudioEngine::stateChanged, this, [this](AudioTypes::AudioState state) {
            updatePlaybackState(state == AudioTypes::AudioState::Playing);
        });
        
        connect(m_audioEngine, &AudioEngine::positionChanged, this, &PlaybackControlWidget::updateCurrentTime);
        connect(m_audioEngine, &AudioEngine::durationChanged, this, &PlaybackControlWidget::updateTotalTime);
        connect(m_audioEngine, &AudioEngine::volumeChanged, this, &PlaybackControlWidget::updateVolume);
        connect(m_audioEngine, &AudioEngine::mutedChanged, this, &PlaybackControlWidget::updateMuted);
        connect(m_audioEngine, &AudioEngine::playModeChanged, this, &PlaybackControlWidget::updatePlayMode);
        
        Logger::instance()->info("PlaybackControlWidget 已连接到音频引擎");
    }
}

void PlaybackControlWidget::setPlaybackState(bool isPlaying)
{
    if (m_isPlaying != isPlaying) {
        m_isPlaying = isPlaying;
        updatePlayPauseButton();
    }
}

void PlaybackControlWidget::setCurrentTime(qint64 time)
{
    if (m_currentTime != time) {
        m_currentTime = time;
        m_progressBar->setPosition(time);
    }
}

void PlaybackControlWidget::setTotalTime(qint64 time)
{
    if (m_totalTime != time) {
        m_totalTime = time;
        m_progressBar->setDuration(time);
    }
}

void PlaybackControlWidget::setVolume(int volume)
{
    if (m_volume != volume) {
        m_volume = volume;
        m_volumeSlider->setValue(volume);
        updateVolumeLabel();
        updateVolumeButton();
    }
}

void PlaybackControlWidget::setMuted(bool muted)
{
    if (m_isMuted != muted) {
        m_isMuted = muted;
        updateVolumeButton();
    }
}

void PlaybackControlWidget::setPlayMode(AudioTypes::PlayMode mode)
{
    if (m_playMode != mode) {
        m_playMode = mode;
        updatePlayModeButton();
    }
}

void PlaybackControlWidget::setLayoutMode(LayoutMode mode)
{
    if (m_layoutMode != mode) {
        m_layoutMode = mode;
        rebuildLayout();
    }
}

void PlaybackControlWidget::setProgressBarVisible(bool visible)
{
    m_progressBar->setVisible(visible);
}

void PlaybackControlWidget::setVolumeControlVisible(bool visible)
{
    m_volumeFrame->setVisible(visible);
}

void PlaybackControlWidget::setPlayModeButtonVisible(bool visible)
{
    m_playModeButton->setVisible(visible);
}

void PlaybackControlWidget::setControlButtonStyle(const QString& style)
{
    m_buttonStyle = style;
    applyStyles();
}

void PlaybackControlWidget::setProgressBarStyle(const QString& style)
{
    m_progressStyle = style;
    if (m_progressBar) {
        m_progressBar->setProgressBarStyle(style);
    }
}

void PlaybackControlWidget::setVolumeSliderStyle(const QString& style)
{
    m_volumeStyle = style;
    if (m_volumeSlider) {
        m_volumeSlider->setStyleSheet(style);
    }
}

// 公共槽函数实现
void PlaybackControlWidget::updatePlaybackState(bool isPlaying)
{
    setPlaybackState(isPlaying);
}

void PlaybackControlWidget::updateCurrentTime(qint64 time)
{
    setCurrentTime(time);
}

void PlaybackControlWidget::updateTotalTime(qint64 time)
{
    setTotalTime(time);
}

void PlaybackControlWidget::updateVolume(int volume)
{
    setVolume(volume);
}

void PlaybackControlWidget::updateMuted(bool muted)
{
    setMuted(muted);
}

void PlaybackControlWidget::updatePlayMode(AudioTypes::PlayMode mode)
{
    setPlayMode(mode);
}

// 私有槽函数实现
void PlaybackControlWidget::onPlayPauseButtonClicked()
{
    Logger::instance()->debug("播放/暂停按钮被点击");
    emit playPauseClicked();
}

void PlaybackControlWidget::onPreviousButtonClicked()
{
    Logger::instance()->debug("上一首按钮被点击");
    emit previousClicked();
}

void PlaybackControlWidget::onNextButtonClicked()
{
    Logger::instance()->debug("下一首按钮被点击");
    emit nextClicked();
}

void PlaybackControlWidget::onPlayModeButtonClicked()
{
    Logger::instance()->debug("播放模式按钮被点击");
    emit playModeClicked();
}

void PlaybackControlWidget::onVolumeSliderChanged(int value)
{
    if (m_volume != value) {
        m_volume = value;
        updateVolumeLabel();
        updateVolumeButton();
        Logger::instance()->debug(QString("音量滑块改变: %1").arg(value));
        emit volumeChanged(value);
    }
}

void PlaybackControlWidget::onMuteButtonClicked()
{
    m_isMuted = !m_isMuted;
    updateVolumeButton();
    Logger::instance()->debug(QString("静音按钮被点击: %1").arg(m_isMuted ? "静音" : "取消静音"));
    emit muteToggled(m_isMuted);
}

void PlaybackControlWidget::onProgressBarSeekRequested(qint64 position)
{
    Logger::instance()->debug(QString("进度条跳转请求: %1").arg(formatTime(position)));
    emit seekRequested(position);
}

void PlaybackControlWidget::onProgressBarPositionChanged(qint64 position)
{
    emit positionChanged(position);
}

// 私有方法实现
void PlaybackControlWidget::updatePlayPauseButton()
{
    if (m_playPauseButton) {
        QString iconPath = m_isPlaying ? ":/new/prefix1/images/pauseIcon.png" : ":/new/prefix1/images/playIcon.png";
        QString tooltip = m_isPlaying ? "暂停" : "播放";
        
        m_playPauseButton->setIcon(QIcon(iconPath));
        m_playPauseButton->setToolTip(tooltip);
    }
}

void PlaybackControlWidget::updatePlayModeButton()
{
    if (m_playModeButton) {
        QString iconPath = getPlayModeIcon(m_playMode);
        QString tooltip = getPlayModeTooltip(m_playMode);
        
        m_playModeButton->setIcon(QIcon(iconPath));
        m_playModeButton->setToolTip(tooltip);
    }
}

void PlaybackControlWidget::updateVolumeButton()
{
    if (m_muteButton) {
        QString iconPath = getVolumeIcon(m_volume, m_isMuted);
        QString tooltip = m_isMuted ? "取消静音" : "静音";
        
        m_muteButton->setIcon(QIcon(iconPath));
        m_muteButton->setToolTip(tooltip);
    }
}

void PlaybackControlWidget::updateVolumeLabel()
{
    if (m_volumeLabel) {
        m_volumeLabel->setText(QString::number(m_volume));
    }
}

void PlaybackControlWidget::applyStyles()
{
    if (!m_buttonStyle.isEmpty()) {
        if (m_playPauseButton) m_playPauseButton->setStyleSheet(m_buttonStyle);
        if (m_previousButton) m_previousButton->setStyleSheet(m_buttonStyle);
        if (m_nextButton) m_nextButton->setStyleSheet(m_buttonStyle);
        if (m_playModeButton) m_playModeButton->setStyleSheet(m_buttonStyle);
        if (m_muteButton) m_muteButton->setStyleSheet(m_buttonStyle);
    }
    
    if (!m_volumeStyle.isEmpty() && m_volumeSlider) {
        m_volumeSlider->setStyleSheet(m_volumeStyle);
    }
}

void PlaybackControlWidget::rebuildLayout()
{
    setupLayouts();
}

QString PlaybackControlWidget::getPlayModeIcon(AudioTypes::PlayMode mode) const
{
    switch (mode) {
        case AudioTypes::PlayMode::ListLoop:
            return ":/new/prefix1/images/listCycle.png";
        case AudioTypes::PlayMode::SingleLoop:
            return ":/new/prefix1/images/singleCycle.png";
        case AudioTypes::PlayMode::Random:
            return ":/new/prefix1/images/randomPlay.png";
        case AudioTypes::PlayMode::Sequential:
            return ":/new/prefix1/images/sequentialPlay.png";
        default:
            return ":/new/prefix1/images/listCycle.png";
    }
}

QString PlaybackControlWidget::getPlayModeTooltip(AudioTypes::PlayMode mode) const
{
    switch (mode) {
        case AudioTypes::PlayMode::ListLoop:
            return "播放模式：列表循环";
        case AudioTypes::PlayMode::SingleLoop:
            return "播放模式：单曲循环";
        case AudioTypes::PlayMode::Random:
            return "播放模式：随机播放";
        case AudioTypes::PlayMode::Sequential:
            return "播放模式：顺序播放";
        default:
            return "播放模式：列表循环";
    }
}

QString PlaybackControlWidget::getVolumeIcon(int volume, bool muted) const
{
    if (muted || volume == 0) {
        return ":/new/prefix1/images/muteIcon.png";
    } else if (volume < 30) {
        return ":/new/prefix1/images/volumeLowIcon.png";
    } else if (volume < 70) {
        return ":/new/prefix1/images/volumeIcon.png";
    } else {
        return ":/new/prefix1/images/volumeHighIcon.png";
    }
}

QString PlaybackControlWidget::formatTime(qint64 milliseconds) const
{
    qint64 seconds = milliseconds / 1000;
    qint64 minutes = seconds / 60;
    seconds %= 60;
    
    return QString("%1:%2").arg(minutes).arg(seconds, 2, 10, QChar('0'));
}