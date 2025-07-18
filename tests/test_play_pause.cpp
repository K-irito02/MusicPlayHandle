#include <QApplication>
#include <QDebug>
#include <QTimer>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>
#include <QLabel>

#include "../src/audio/audioengine.h"
#include "../src/audio/audiotypes.h"
#include "../src/models/song.h"

class PlayPauseTestWidget : public QWidget
{
    Q_OBJECT

public:
    PlayPauseTestWidget(QWidget* parent = nullptr) : QWidget(parent)
    {
        setupUI();
        setupConnections();
        
        // 创建测试歌曲
        Song testSong;
        testSong.setId(1);
        testSong.setTitle("测试歌曲");
        testSong.setArtist("测试艺术家");
        testSong.setFilePath("C:/test.mp3"); // 请替换为实际存在的音频文件路径
        
        QList<Song> playlist;
        playlist.append(testSong);
        
        // 设置播放列表
        AudioEngine::instance()->setPlaylist(playlist);
        AudioEngine::instance()->setCurrentIndex(0);
        
        qDebug() << "测试界面初始化完成";
    }

private slots:
    void onPlayPauseClicked()
    {
        qDebug() << "=== 播放/暂停按钮被点击 ===";
        
        AudioEngine* engine = AudioEngine::instance();
        AudioTypes::AudioState currentState = engine->state();
        
        qDebug() << "点击前状态:" << static_cast<int>(currentState);
        engine->debugAudioState();
        
        // 执行播放/暂停切换
        if (currentState == AudioTypes::AudioState::Playing) {
            qDebug() << "执行暂停";
            engine->pause();
        } else {
            qDebug() << "执行播放";
            engine->play();
        }
        
        // 延迟检查状态变化
        QTimer::singleShot(500, [this]() {
            AudioEngine* engine = AudioEngine::instance();
            qDebug() << "操作后状态:" << static_cast<int>(engine->state());
            engine->debugAudioState();
            updateStatusLabel();
        });
    }
    
    void onStateChanged(AudioTypes::AudioState state)
    {
        qDebug() << "状态变化信号:" << static_cast<int>(state);
        updateStatusLabel();
    }
    
    void updateStatusLabel()
    {
        AudioEngine* engine = AudioEngine::instance();
        QString statusText = QString("当前状态: %1")
            .arg(engine->getStateString());
        statusLabel->setText(statusText);
    }

private:
    void setupUI()
    {
        QVBoxLayout* layout = new QVBoxLayout(this);
        
        // 状态显示标签
        statusLabel = new QLabel("状态: 未初始化", this);
        layout->addWidget(statusLabel);
        
        // 播放/暂停按钮
        playPauseButton = new QPushButton("播放/暂停", this);
        layout->addWidget(playPauseButton);
        
        // 调试按钮
        QPushButton* debugButton = new QPushButton("调试信息", this);
        layout->addWidget(debugButton);
        connect(debugButton, &QPushButton::clicked, [this]() {
            AudioEngine::instance()->debugAudioState();
        });
        
        setLayout(layout);
        setWindowTitle("播放暂停功能测试");
        resize(300, 200);
    }
    
    void setupConnections()
    {
        connect(playPauseButton, &QPushButton::clicked, 
                this, &PlayPauseTestWidget::onPlayPauseClicked);
        
        AudioEngine* engine = AudioEngine::instance();
        connect(engine, &AudioEngine::stateChanged,
                this, &PlayPauseTestWidget::onStateChanged);
    }

private:
    QLabel* statusLabel;
    QPushButton* playPauseButton;
};

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    
    PlayPauseTestWidget widget;
    widget.show();
    
    return app.exec();
}

#include "test_play_pause.moc" 