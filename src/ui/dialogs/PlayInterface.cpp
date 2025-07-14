#include "PlayInterface.h"
#include "ui_PlayInterface.h"
#include "../controllers/playinterfacecontroller.h"

PlayInterface::PlayInterface(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PlayInterface)
    , m_controller(nullptr)
    , m_updateTimer(nullptr)
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
{
    ui->setupUi(this);
    
    // 创建控制器
    m_controller = new PlayInterfaceController(this, this);
    
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
}

PlayInterface::~PlayInterface()
{
    if (m_controller) {
        m_controller->shutdown();
        delete m_controller;
    }
    delete ui;
}

void PlayInterface::setPlaybackState(bool isPlaying)
{
    m_isPlaying = isPlaying;
    updatePlaybackControls();
}

void PlayInterface::setCurrentTime(qint64 time)
{
    m_currentTime = time;
    updateTimeDisplay();
}

void PlayInterface::setTotalTime(qint64 time)
{
    m_totalTime = time;
    updateTimeDisplay();
}

void PlayInterface::setVolume(int volume)
{
    m_volume = volume;
    updateVolumeDisplay();
}

void PlayInterface::setBalance(int balance)
{
    m_balance = balance;
}

void PlayInterface::setMuted(bool muted)
{
    m_isMuted = muted;
    updateVolumeDisplay();
}

void PlayInterface::setSongTitle(const QString& title)
{
    // 设置歌曲标题
    Q_UNUSED(title)
}

void PlayInterface::setSongArtist(const QString& artist)
{
    // 设置歌曲艺术家
    Q_UNUSED(artist)
}

void PlayInterface::setSongAlbum(const QString& album)
{
    // 设置歌曲专辑
    Q_UNUSED(album)
}

void PlayInterface::setSongCover(const QPixmap& cover)
{
    // 设置歌曲封面
    Q_UNUSED(cover)
}

void PlayInterface::setLyrics(const QString& lyrics)
{
    // 设置歌词
    Q_UNUSED(lyrics)
}

void PlayInterface::updateWaveform(const QVector<float>& data)
{
    // 更新波形显示
    Q_UNUSED(data)
}

void PlayInterface::updateSpectrum(const QVector<float>& data)
{
    // 更新频谱显示
    Q_UNUSED(data)
}

void PlayInterface::updateVUMeter(float leftLevel, float rightLevel)
{
    // 更新VU表
    Q_UNUSED(leftLevel)
    Q_UNUSED(rightLevel)
}

void PlayInterface::setDisplayMode(int mode)
{
    m_displayMode = mode;
    emit displayModeChanged(mode);
}

void PlayInterface::setEqualizerValues(const QVector<int>& values)
{
    // 设置均衡器值
    Q_UNUSED(values)
}

QVector<int> PlayInterface::getEqualizerValues() const
{
    // 返回均衡器值
    return QVector<int>();
}

void PlayInterface::setupConnections()
{
    // 设置信号连接
    // 这里需要根据UI文件中的控件名称来连接
}

void PlayInterface::setupUI()
{
    // 设置UI初始状态
    setWindowTitle("播放界面");
    
    // 创建更新定时器
    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, this, &PlayInterface::onUpdateTimer);
    m_updateTimer->start(50); // 20fps
}

void PlayInterface::setupVisualization()
{
    // 设置可视化组件
    // 这里需要根据实际的UI控件来设置
}

void PlayInterface::updateTimeDisplay()
{
    // 更新时间显示
}

void PlayInterface::updateVolumeDisplay()
{
    // 更新音量显示
}

void PlayInterface::updatePlaybackControls()
{
    // 更新播放控制按钮
}

QString PlayInterface::formatTime(qint64 milliseconds) const
{
    int seconds = milliseconds / 1000;
    int minutes = seconds / 60;
    seconds = seconds % 60;
    return QString("%1:%2").arg(minutes).arg(seconds, 2, 10, QChar('0'));
}

// 槽函数实现
void PlayInterface::onPlayPauseClicked()
{
    emit playPauseClicked();
}

void PlayInterface::onStopClicked()
{
    emit stopClicked();
}

void PlayInterface::onPreviousClicked()
{
    emit previousClicked();
}

void PlayInterface::onNextClicked()
{
    emit nextClicked();
}

void PlayInterface::onVolumeSliderChanged(int value)
{
    emit volumeChanged(value);
}

void PlayInterface::onBalanceSliderChanged(int value)
{
    emit balanceChanged(value);
}

void PlayInterface::onPositionSliderChanged(int value)
{
    emit positionChanged(value);
}

void PlayInterface::onMuteButtonClicked()
{
    emit muteToggled(!m_isMuted);
}

void PlayInterface::onEqualizerSliderChanged(int value)
{
    Q_UNUSED(value)
    // 均衡器滑块改变
}

void PlayInterface::onDisplayModeChanged()
{
    // 显示模式改变
    m_displayMode = (m_displayMode + 1) % 3;
    emit displayModeChanged(m_displayMode);
}

void PlayInterface::onUpdateTimer()
{
    // 更新定时器
    if (m_controller) {
        // 这里可以更新界面状态
    }
}