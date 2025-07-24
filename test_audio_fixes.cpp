#include <QApplication>
#include <QDebug>
#include <QTimer>
#include <QFileInfo>
#include "src/audio/audioengine.h"
#include "src/ui/widgets/musicprogressbar.h"
#include "src/models/song.h"

/**
 * 音频播放修复测试
 * 测试内容：
 * 1. 音频播放质量和稳定性
 * 2. 平衡控制功能
 * 3. 进度条拖拽不重新播放
 */
class AudioFixTester : public QObject
{
    Q_OBJECT
    
public:
    AudioFixTester(QObject* parent = nullptr) : QObject(parent)
    {
        m_audioEngine = AudioEngine::instance();
        m_progressBar = new MusicProgressBar();
        
        setupConnections();
        setupTestSong();
    }
    
    ~AudioFixTester()
    {
        AudioEngine::cleanup();
        delete m_progressBar;
    }
    
    void runTests()
    {
        qDebug() << "========== 开始音频修复测试 ==========";
        
        // 测试1：音频播放质量
        testAudioPlayback();
        
        // 延迟执行其他测试
        QTimer::singleShot(2000, this, &AudioFixTester::testBalanceControl);
        QTimer::singleShot(4000, this, &AudioFixTester::testProgressBarDrag);
        QTimer::singleShot(6000, this, &AudioFixTester::testSeekOperation);
        QTimer::singleShot(8000, this, &AudioFixTester::finishTests);
    }
    
private slots:
    void testAudioPlayback()
    {
        qDebug() << "\n--- 测试1：音频播放质量 ---";
        
        // 检查音频引擎状态
        m_audioEngine->debugAudioState();
        
        // 开始播放
        qDebug() << "开始播放测试歌曲...";
        m_audioEngine->play();
        
        // 检查播放状态
        QTimer::singleShot(1000, this, [this]() {
            AudioTypes::AudioState state = m_audioEngine->state();
            qDebug() << "播放状态:" << (int)state;
            qDebug() << "当前位置:" << m_audioEngine->position() << "ms";
            qDebug() << "总时长:" << m_audioEngine->duration() << "ms";
            qDebug() << "音量:" << m_audioEngine->volume();
            qDebug() << "是否静音:" << m_audioEngine->isMuted();
            
            if (state == AudioTypes::AudioState::Playing) {
                qDebug() << "✓ 音频播放正常";
            } else {
                qWarning() << "✗ 音频播放异常";
            }
        });
    }
    
    void testBalanceControl()
    {
        qDebug() << "\n--- 测试2：平衡控制功能 ---";
        
        // 测试不同的平衡值
        qDebug() << "测试左声道 (balance = -1.0)";
        m_audioEngine->setBalance(-1.0);
        
        QTimer::singleShot(1000, this, [this]() {
            qDebug() << "当前平衡值:" << m_audioEngine->getBalance();
            
            qDebug() << "测试右声道 (balance = 1.0)";
            m_audioEngine->setBalance(1.0);
            
            QTimer::singleShot(1000, this, [this]() {
                qDebug() << "当前平衡值:" << m_audioEngine->getBalance();
                
                qDebug() << "恢复中央 (balance = 0.0)";
                m_audioEngine->setBalance(0.0);
                
                qDebug() << "✓ 平衡控制测试完成";
            });
        });
    }
    
    void testProgressBarDrag()
    {
        qDebug() << "\n--- 测试3：进度条拖拽功能 ---";
        
        // 连接进度条信号
        connect(m_progressBar, &MusicProgressBar::seekRequested, this, [this](qint64 position) {
            qDebug() << "进度条请求跳转到:" << position << "ms";
            
            // 检查是否会重新播放（不应该）
            AudioTypes::AudioState stateBefore = m_audioEngine->state();
            
            m_audioEngine->seek(position);
            
            QTimer::singleShot(100, this, [this, stateBefore, position]() {
                AudioTypes::AudioState stateAfter = m_audioEngine->state();
                qint64 actualPosition = m_audioEngine->position();
                
                qDebug() << "跳转前状态:" << (int)stateBefore;
                qDebug() << "跳转后状态:" << (int)stateAfter;
                qDebug() << "期望位置:" << position << "ms";
                qDebug() << "实际位置:" << actualPosition << "ms";
                
                if (stateAfter == stateBefore && qAbs(actualPosition - position) < 1000) {
                    qDebug() << "✓ 进度条跳转正常，没有重新播放";
                } else {
                    qWarning() << "✗ 进度条跳转异常";
                }
            });
        });
        
        // 模拟进度条拖拽
        qint64 totalDuration = m_audioEngine->duration();
        if (totalDuration > 10000) { // 至少10秒
            qint64 targetPosition = totalDuration / 3; // 跳转到1/3位置
            qDebug() << "模拟拖拽进度条到:" << targetPosition << "ms";
            
            // 模拟用户拖拽
            m_progressBar->updatePosition(targetPosition);
            emit m_progressBar->seekRequested(targetPosition);
        }
    }
    
    void testSeekOperation()
    {
        qDebug() << "\n--- 测试4：Seek操作稳定性 ---";
        
        qint64 totalDuration = m_audioEngine->duration();
        if (totalDuration > 20000) { // 至少20秒
            // 测试多次快速seek
            QVector<qint64> seekPositions = {
                totalDuration / 4,
                totalDuration / 2,
                totalDuration * 3 / 4,
                1000 // 回到开头附近
            };
            
            int index = 0;
            QTimer* seekTimer = new QTimer(this);
            connect(seekTimer, &QTimer::timeout, [this, seekPositions, &index, seekTimer]() {
                if (index < seekPositions.size()) {
                    qint64 position = seekPositions[index];
                    qDebug() << "快速seek测试" << (index + 1) << ":" << position << "ms";
                    m_audioEngine->seek(position);
                    index++;
                } else {
                    seekTimer->stop();
                    qDebug() << "✓ 多次seek操作完成";
                }
            });
            
            seekTimer->start(500); // 每500ms一次seek
        }
    }
    
    void finishTests()
    {
        qDebug() << "\n========== 音频修复测试完成 ==========";
        qDebug() << "测试总结：";
        qDebug() << "1. 音频播放：统一使用QMediaPlayer，消除双重播放冲突";
        qDebug() << "2. 平衡控制：确保设置正确保存和应用";
        qDebug() << "3. 进度条拖拽：用户交互期间避免外部更新干扰";
        qDebug() << "4. Seek操作：直接使用主线程QMediaPlayer，避免多实例冲突";
        
        // 停止播放
        m_audioEngine->stop();
        
        // 退出应用
        QTimer::singleShot(1000, qApp, &QApplication::quit);
    }
    
private:
    void setupConnections()
    {
        // 连接AudioEngine信号到进度条
        connect(m_audioEngine, &AudioEngine::positionChanged,
                m_progressBar, &MusicProgressBar::updatePosition);
        connect(m_audioEngine, &AudioEngine::durationChanged,
                m_progressBar, &MusicProgressBar::updateDuration);
    }
    
    void setupTestSong()
    {
        // 创建测试歌曲列表
        QList<Song> testSongs;
        
        // 查找可用的音频文件进行测试
        QStringList possiblePaths = {
            "/usr/share/sounds/alsa/Front_Left.wav",
            "/usr/share/sounds/alsa/Front_Right.wav",
            "/usr/share/sounds/alsa/Rear_Left.wav",
            "/usr/share/sounds/test.wav",
            "/tmp/test.mp3",
            "test.mp3",
            "test.wav"
        };
        
        QString validPath;
        for (const QString& path : possiblePaths) {
            if (QFileInfo::exists(path)) {
                validPath = path;
                break;
            }
        }
        
        if (!validPath.isEmpty()) {
            Song testSong;
            testSong.setFilePath(validPath);
            testSong.setTitle("测试音频");
            testSong.setArtist("测试艺术家");
            testSong.setAlbum("测试专辑");
            
            testSongs.append(testSong);
            m_audioEngine->setPlaylist(testSongs);
            m_audioEngine->setCurrentIndex(0);
            
            qDebug() << "使用测试音频文件:" << validPath;
        } else {
            qWarning() << "未找到可用的测试音频文件，某些测试可能无法执行";
        }
    }
    
private:
    AudioEngine* m_audioEngine;
    MusicProgressBar* m_progressBar;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    AudioFixTester tester;
    tester.runTests();
    
    return app.exec();
}

#include "test_audio_fixes.moc"