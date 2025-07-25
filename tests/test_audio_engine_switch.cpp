#include <QApplication>
#include <QDebug>
#include <QTimer>
#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QSlider>
#include "src/audio/audioengine.h"
#include "src/ui/dialogs/improvedplayinterface.h"
#include "src/models/song.h"

/**
 * 音频引擎切换功能测试
 * 测试内容：
 * 1. QMediaPlayer和FFmpeg引擎切换
 * 2. 平衡控制在不同引擎下的效果
 * 3. 进度条拖拽在两种引擎下的表现
 * 4. 音频质量对比
 */
class AudioEngineSwitchTester : public QWidget
{
    Q_OBJECT
    
public:
    AudioEngineSwitchTester(QWidget* parent = nullptr) : QWidget(parent)
    {
        setupUI();
        setupAudio();
        setupConnections();
    }
    
    ~AudioEngineSwitchTester()
    {
        AudioEngine::cleanup();
    }
    
private slots:
    void onEngineSwitch()
    {
        if (!m_audioEngine) return;
        
        AudioTypes::AudioEngineType currentType = m_audioEngine->getAudioEngineType();
        AudioTypes::AudioEngineType newType = (currentType == AudioTypes::AudioEngineType::QMediaPlayer) ?
            AudioTypes::AudioEngineType::FFmpeg : AudioTypes::AudioEngineType::QMediaPlayer;
        
        qDebug() << "手动切换音频引擎到:" << (newType == AudioTypes::AudioEngineType::FFmpeg ? "FFmpeg" : "QMediaPlayer");
        m_audioEngine->setAudioEngineType(newType);
    }
    
    void onEngineTypeChanged(AudioTypes::AudioEngineType type)
    {
        QString engineName = (type == AudioTypes::AudioEngineType::FFmpeg) ? "FFmpeg" : "QMediaPlayer";
        m_engineLabel->setText("当前引擎: " + engineName);
        m_switchButton->setText("切换到 " + ((type == AudioTypes::AudioEngineType::FFmpeg) ? "QMediaPlayer" : "FFmpeg"));
        
        // 更新平衡控制提示
        if (type == AudioTypes::AudioEngineType::FFmpeg) {
            m_balanceLabel->setText("平衡控制: 有效 (FFmpeg引擎)");
            m_balanceLabel->setStyleSheet("color: green;");
        } else {
            m_balanceLabel->setText("平衡控制: 无效 (QMediaPlayer引擎)");
            m_balanceLabel->setStyleSheet("color: orange;");
        }
        
        qDebug() << "音频引擎已切换到:" << engineName;
    }
    
    void onPlayPause()
    {
        if (!m_audioEngine) return;
        
        if (m_audioEngine->state() == AudioTypes::AudioState::Playing) {
            m_audioEngine->pause();
            m_playButton->setText("播放");
        } else {
            m_audioEngine->play();
            m_playButton->setText("暂停");
        }
    }
    
    void onBalanceChanged(int value)
    {
        if (!m_audioEngine) return;
        
        double balance = value / 100.0;
        m_audioEngine->setBalance(balance);
        
        QString balanceText = QString("平衡: %1").arg(value);
        if (value < 0) {
            balanceText = QString("平衡: 左 %1").arg(-value);
        } else if (value > 0) {
            balanceText = QString("平衡: 右 %1").arg(value);
        } else {
            balanceText = "平衡: 中央";
        }
        
        // 根据引擎类型添加效果提示
        AudioTypes::AudioEngineType currentType = m_audioEngine->getAudioEngineType();
        if (currentType == AudioTypes::AudioEngineType::FFmpeg) {
            balanceText += " (已生效)";
        } else {
            balanceText += " (切换到FFmpeg生效)";
        }
        
        m_balanceValueLabel->setText(balanceText);
    }
    
    void onSeekTest()
    {
        if (!m_audioEngine) return;
        
        qint64 duration = m_audioEngine->duration();
        if (duration > 10000) { // 至少10秒
            qint64 targetPosition = duration / 3; // 跳转到1/3位置
            qDebug() << "测试跳转到:" << targetPosition << "ms，当前引擎:" << m_audioEngine->getAudioEngineTypeString();
            m_audioEngine->seek(targetPosition);
        }
    }
    
    void updateStatus()
    {
        if (!m_audioEngine) return;
        
        AudioTypes::AudioState state = m_audioEngine->state();
        qint64 position = m_audioEngine->position();
        qint64 duration = m_audioEngine->duration();
        
        QString statusText = QString("状态: %1 | 位置: %2/%3ms")
            .arg(state == AudioTypes::AudioState::Playing ? "播放中" :
                 state == AudioTypes::AudioState::Paused ? "暂停" : "停止")
            .arg(position)
            .arg(duration);
        
        m_statusLabel->setText(statusText);
    }
    
private:
    void setupUI()
    {
        setWindowTitle("音频引擎切换测试");
        setMinimumSize(400, 300);
        
        QVBoxLayout* layout = new QVBoxLayout(this);
        
        // 引擎状态显示
        m_engineLabel = new QLabel("当前引擎: QMediaPlayer");
        m_engineLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
        layout->addWidget(m_engineLabel);
        
        // 切换按钮
        m_switchButton = new QPushButton("切换到 FFmpeg");
        connect(m_switchButton, &QPushButton::clicked, this, &AudioEngineSwitchTester::onEngineSwitch);
        layout->addWidget(m_switchButton);
        
        // 播放控制
        m_playButton = new QPushButton("播放");
        connect(m_playButton, &QPushButton::clicked, this, &AudioEngineSwitchTester::onPlayPause);
        layout->addWidget(m_playButton);
        
        // 平衡控制
        m_balanceLabel = new QLabel("平衡控制: 无效 (QMediaPlayer引擎)");
        layout->addWidget(m_balanceLabel);
        
        m_balanceSlider = new QSlider(Qt::Horizontal);
        m_balanceSlider->setRange(-100, 100);
        m_balanceSlider->setValue(0);
        connect(m_balanceSlider, &QSlider::valueChanged, this, &AudioEngineSwitchTester::onBalanceChanged);
        layout->addWidget(m_balanceSlider);
        
        m_balanceValueLabel = new QLabel("平衡: 中央");
        layout->addWidget(m_balanceValueLabel);
        
        // 跳转测试按钮
        QPushButton* seekButton = new QPushButton("跳转测试");
        connect(seekButton, &QPushButton::clicked, this, &AudioEngineSwitchTester::onSeekTest);
        layout->addWidget(seekButton);
        
        // 状态显示
        m_statusLabel = new QLabel("状态: 未初始化");
        m_statusLabel->setStyleSheet("font-size: 10px; color: gray;");
        layout->addWidget(m_statusLabel);
        
        // 使用说明
        QLabel* helpLabel = new QLabel(
            "使用说明:\n"
            "1. 点击'切换引擎'测试QMediaPlayer和FFmpeg切换\n"
            "2. QMediaPlayer: 纯净音质，平衡控制无效\n"
            "3. FFmpeg: 支持实时音效处理，平衡控制有效\n"
            "4. 在不同引擎下测试播放、暂停、跳转功能"
        );
        helpLabel->setStyleSheet("font-size: 9px; color: gray; padding: 10px;");
        helpLabel->setWordWrap(true);
        layout->addWidget(helpLabel);
    }
    
    void setupAudio()
    {
        m_audioEngine = AudioEngine::instance();
        
        // 创建测试歌曲
        Song testSong;
        testSong.setTitle("测试音频文件");
        testSong.setArtist("测试艺术家");
        testSong.setAlbum("测试专辑");
        testSong.setFilePath("/usr/share/sounds/alsa/Front_Left.wav"); // 系统测试音频
        
        QList<Song> playlist;
        playlist.append(testSong);
        
        m_audioEngine->setPlaylist(playlist);
        m_audioEngine->setCurrentIndex(0);
        
        qDebug() << "测试音频设置完成";
    }
    
    void setupConnections()
    {
        // 连接音频引擎信号
        connect(m_audioEngine, &AudioEngine::audioEngineTypeChanged,
                this, &AudioEngineSwitchTester::onEngineTypeChanged);
        
        // 定时更新状态
        QTimer* timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &AudioEngineSwitchTester::updateStatus);
        timer->start(500); // 每500ms更新一次
        
        // 初始化状态
        onEngineTypeChanged(m_audioEngine->getAudioEngineType());
    }
    
private:
    AudioEngine* m_audioEngine;
    QLabel* m_engineLabel;
    QPushButton* m_switchButton;
    QPushButton* m_playButton;
    QLabel* m_balanceLabel;
    QSlider* m_balanceSlider;
    QLabel* m_balanceValueLabel;
    QLabel* m_statusLabel;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    AudioEngineSwitchTester tester;
    tester.show();
    
    qDebug() << "=== 音频引擎切换测试启动 ===";
    qDebug() << "测试功能:";
    qDebug() << "1. QMediaPlayer ↔ FFmpeg 切换";
    qDebug() << "2. 平衡控制效果对比";
    qDebug() << "3. 跳转功能测试";
    qDebug() << "4. 播放状态管理";
    
    return app.exec();
}

#include "test_audio_engine_switch.moc"