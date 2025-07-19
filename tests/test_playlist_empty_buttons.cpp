#include <QTest>
#include <QSignalSpy>
#include <QApplication>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include "../src/ui/controllers/MainWindowController.h"
#include "../src/audio/audioengine.h"
#include "../src/models/song.h"
#include "../src/models/tag.h"

class TestPlaylistEmptyButtons : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    void testNextButtonWithEmptyPlaylist();
    void testPreviousButtonWithEmptyPlaylist();
    void testNextButtonWithValidPlaylist();
    void testPreviousButtonWithValidPlaylist();

private:
    MainWindow* createTestMainWindow();
    void setupTestData();
    
    MainWindow* m_testWindow;
    MainWindowController* m_controller;
    AudioEngine* m_audioEngine;
};

void TestPlaylistEmptyButtons::initTestCase()
{
    // 创建测试主窗口
    m_testWindow = createTestMainWindow();
    QVERIFY(m_testWindow != nullptr);
    
    // 创建控制器
    m_controller = new MainWindowController(m_testWindow, this);
    QVERIFY(m_controller != nullptr);
    
    // 初始化控制器
    bool initResult = m_controller->initialize();
    QVERIFY(initResult);
    
    // 获取AudioEngine实例
    m_audioEngine = AudioEngine::instance();
    QVERIFY(m_audioEngine != nullptr);
    
    // 设置测试数据
    setupTestData();
}

void TestPlaylistEmptyButtons::cleanupTestCase()
{
    if (m_controller) {
        m_controller->deleteLater();
        m_controller = nullptr;
    }
    
    if (m_testWindow) {
        m_testWindow->deleteLater();
        m_testWindow = nullptr;
    }
}

void TestPlaylistEmptyButtons::testNextButtonWithEmptyPlaylist()
{
    qDebug() << "=== 测试播放列表为空时下一曲按钮 ===";
    
    // 清空播放列表
    m_audioEngine->setPlaylist(QList<Song>());
    m_audioEngine->setCurrentIndex(-1);
    
    QCOMPARE(m_audioEngine->playlist().size(), 0);
    QCOMPARE(m_audioEngine->currentIndex(), -1);
    
    // 模拟点击下一曲按钮
    m_controller->onNextButtonClicked();
    
    // 等待一段时间让异步操作完成
    QTest::qWait(200);
    
    // 验证是否启动了新播放（播放列表应该不为空）
    QVERIFY(m_audioEngine->playlist().size() > 0);
    QVERIFY(m_audioEngine->currentIndex() >= 0);
    
    qDebug() << "播放列表大小:" << m_audioEngine->playlist().size();
    qDebug() << "当前索引:" << m_audioEngine->currentIndex();
}

void TestPlaylistEmptyButtons::testPreviousButtonWithEmptyPlaylist()
{
    qDebug() << "=== 测试播放列表为空时上一曲按钮 ===";
    
    // 清空播放列表
    m_audioEngine->setPlaylist(QList<Song>());
    m_audioEngine->setCurrentIndex(-1);
    
    QCOMPARE(m_audioEngine->playlist().size(), 0);
    QCOMPARE(m_audioEngine->currentIndex(), -1);
    
    // 模拟点击上一曲按钮
    m_controller->onPreviousButtonClicked();
    
    // 等待一段时间让异步操作完成
    QTest::qWait(200);
    
    // 验证是否启动了新播放（播放列表应该不为空）
    QVERIFY(m_audioEngine->playlist().size() > 0);
    QVERIFY(m_audioEngine->currentIndex() >= 0);
    
    qDebug() << "播放列表大小:" << m_audioEngine->playlist().size();
    qDebug() << "当前索引:" << m_audioEngine->currentIndex();
}

void TestPlaylistEmptyButtons::testNextButtonWithValidPlaylist()
{
    qDebug() << "=== 测试播放列表有效时下一曲按钮 ===";
    
    // 创建测试播放列表
    QList<Song> testPlaylist;
    Song song1, song2, song3;
    song1.setId(1);
    song1.setTitle("测试歌曲1");
    song1.setArtist("测试艺术家1");
    song1.setFilePath("/path/to/song1.mp3");
    
    song2.setId(2);
    song2.setTitle("测试歌曲2");
    song2.setArtist("测试艺术家2");
    song2.setFilePath("/path/to/song2.mp3");
    
    song3.setId(3);
    song3.setTitle("测试歌曲3");
    song3.setArtist("测试艺术家3");
    song3.setFilePath("/path/to/song3.mp3");
    
    testPlaylist << song1 << song2 << song3;
    
    // 设置播放列表
    m_audioEngine->setPlaylist(testPlaylist);
    m_audioEngine->setCurrentIndex(0);
    
    QCOMPARE(m_audioEngine->playlist().size(), 3);
    QCOMPARE(m_audioEngine->currentIndex(), 0);
    
    // 模拟点击下一曲按钮
    m_controller->onNextButtonClicked();
    
    // 等待一段时间让异步操作完成
    QTest::qWait(100);
    
    // 验证是否切换到下一首歌曲
    QCOMPARE(m_audioEngine->playlist().size(), 3);
    QCOMPARE(m_audioEngine->currentIndex(), 1);
    
    qDebug() << "当前索引:" << m_audioEngine->currentIndex();
}

void TestPlaylistEmptyButtons::testPreviousButtonWithValidPlaylist()
{
    qDebug() << "=== 测试播放列表有效时上一曲按钮 ===";
    
    // 创建测试播放列表
    QList<Song> testPlaylist;
    Song song1, song2, song3;
    song1.setId(1);
    song1.setTitle("测试歌曲1");
    song1.setArtist("测试艺术家1");
    song1.setFilePath("/path/to/song1.mp3");
    
    song2.setId(2);
    song2.setTitle("测试歌曲2");
    song2.setArtist("测试艺术家2");
    song2.setFilePath("/path/to/song2.mp3");
    
    song3.setId(3);
    song3.setTitle("测试歌曲3");
    song3.setArtist("测试艺术家3");
    song3.setFilePath("/path/to/song3.mp3");
    
    testPlaylist << song1 << song2 << song3;
    
    // 设置播放列表，当前播放第二首
    m_audioEngine->setPlaylist(testPlaylist);
    m_audioEngine->setCurrentIndex(1);
    
    QCOMPARE(m_audioEngine->playlist().size(), 3);
    QCOMPARE(m_audioEngine->currentIndex(), 1);
    
    // 模拟点击上一曲按钮
    m_controller->onPreviousButtonClicked();
    
    // 等待一段时间让异步操作完成
    QTest::qWait(100);
    
    // 验证是否切换到上一首歌曲
    QCOMPARE(m_audioEngine->playlist().size(), 3);
    QCOMPARE(m_audioEngine->currentIndex(), 0);
    
    qDebug() << "当前索引:" << m_audioEngine->currentIndex();
}

MainWindow* TestPlaylistEmptyButtons::createTestMainWindow()
{
    // 创建一个简单的测试主窗口
    MainWindow* window = new MainWindow();
    
    // 创建必要的UI控件
    QListWidget* tagListWidget = new QListWidget(window);
    tagListWidget->setObjectName("listWidget_my_tags");
    
    QListWidget* songListWidget = new QListWidget(window);
    songListWidget->setObjectName("listWidget_songs");
    
    QPushButton* playButton = new QPushButton("播放", window);
    playButton->setObjectName("pushButton_play_pause");
    
    QPushButton* nextButton = new QPushButton("下一曲", window);
    nextButton->setObjectName("pushButton_next");
    
    QPushButton* previousButton = new QPushButton("上一曲", window);
    previousButton->setObjectName("pushButton_previous");
    
    QLabel* songTitleLabel = new QLabel("未选择歌曲", window);
    songTitleLabel->setObjectName("label_song_title");
    
    // 设置布局
    QVBoxLayout* layout = new QVBoxLayout(window);
    layout->addWidget(tagListWidget);
    layout->addWidget(songListWidget);
    layout->addWidget(playButton);
    layout->addWidget(previousButton);
    layout->addWidget(nextButton);
    layout->addWidget(songTitleLabel);
    
    window->setLayout(layout);
    
    return window;
}

void TestPlaylistEmptyButtons::setupTestData()
{
    // 这里可以设置一些测试数据，比如添加标签和歌曲
    // 为了简化测试，我们暂时不添加具体数据
    qDebug() << "测试数据设置完成";
}

QTEST_MAIN(TestPlaylistEmptyButtons)
#include "test_playlist_empty_buttons.moc" 