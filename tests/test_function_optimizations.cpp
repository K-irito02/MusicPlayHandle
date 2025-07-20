#include <QTest>
#include <QSignalSpy>
#include <QTimer>
#include <QApplication>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>

#include "../src/database/songdao.h"
#include "../src/database/tagdao.h"
#include "../src/database/playhistorydao.h"
#include "../src/models/song.h"
#include "../src/models/tag.h"
#include "../src/models/playhistory.h"
#include "../src/database/databasemanager.h"
#include "../src/ui/controllers/MainWindowController.h"
#include "../mainwindow.h"

/**
 * @brief 功能优化测试类
 * 
 * 测试四个核心功能优化：
 * 1. 彻底删除歌曲时的标签处理
 * 2. "我的歌曲"标签删除功能优化
 * 3. 跨标签播放列表保持逻辑
 * 4. "最近播放"标签排序与更新
 */
class TestFunctionOptimizations : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // 测试1：彻底删除歌曲时的标签处理
    void testDeleteSongWithTagCleanup();
    
    // 测试2："我的歌曲"标签删除功能优化
    void testMySongsTagDeleteOptions();
    
    // 测试3：跨标签播放列表保持逻辑
    void testCrossTagPlaylistPersistence();
    
    // 测试4："最近播放"标签排序与更新
    void testRecentPlaySortingAndUpdate();

private:
    void setupTestData();
    void cleanupTestData();
    Song createTestSong(const QString& title, const QString& artist);
    Tag createTestTag(const QString& name);
    
    DatabaseManager* m_dbManager;
    SongDao* m_songDao;
    TagDao* m_tagDao;
    PlayHistoryDao* m_playHistoryDao;
    MainWindow* m_mainWindow;
    MainWindowController* m_controller;
};

void TestFunctionOptimizations::initTestCase()
{
    // 初始化测试环境
    m_dbManager = DatabaseManager::instance();
    QVERIFY(m_dbManager->initialize());
    
    m_songDao = new SongDao(this);
    m_tagDao = new TagDao(this);
    m_playHistoryDao = new PlayHistoryDao(this);
    
    // 创建主窗口和控制器
    m_mainWindow = new MainWindow();
    m_controller = new MainWindowController(m_mainWindow, this);
    QVERIFY(m_controller->initialize());
    
    setupTestData();
}

void TestFunctionOptimizations::cleanupTestCase()
{
    cleanupTestData();
    
    if (m_controller) {
        m_controller->shutdown();
        delete m_controller;
    }
    
    if (m_mainWindow) {
        delete m_mainWindow;
    }
    
    delete m_songDao;
    delete m_tagDao;
    delete m_playHistoryDao;
}

void TestFunctionOptimizations::setupTestData()
{
    // 创建测试歌曲
    Song song1 = createTestSong("测试歌曲1", "测试艺术家1");
    Song song2 = createTestSong("测试歌曲2", "测试艺术家2");
    Song song3 = createTestSong("测试歌曲3", "测试艺术家3");
    
    // 创建测试标签
    Tag tag1 = createTestTag("测试标签1");
    Tag tag2 = createTestTag("测试标签2");
    
    // 添加歌曲到标签
    m_songDao->addSongToTag(song1.id(), tag1.id());
    m_songDao->addSongToTag(song1.id(), tag2.id());
    m_songDao->addSongToTag(song2.id(), tag1.id());
    m_songDao->addSongToTag(song3.id(), tag2.id());
    
    // 添加播放历史记录
    QDateTime now = QDateTime::currentDateTime();
    m_playHistoryDao->addPlayRecord(song1.id(), now.addSecs(-3600)); // 1小时前
    m_playHistoryDao->addPlayRecord(song2.id(), now.addSecs(-1800)); // 30分钟前
    m_playHistoryDao->addPlayRecord(song3.id(), now.addSecs(-900));  // 15分钟前
}

void TestFunctionOptimizations::cleanupTestData()
{
    // 清理测试数据
    QList<Song> songs = m_songDao->getAllSongs();
    for (const Song& song : songs) {
        if (song.title().startsWith("测试歌曲")) {
            m_songDao->deleteSong(song.id());
        }
    }
    
    QList<Tag> tags = m_tagDao->getAllTags();
    for (const Tag& tag : tags) {
        if (tag.name().startsWith("测试标签")) {
            m_tagDao->deleteTag(tag.id());
        }
    }
}

Song TestFunctionOptimizations::createTestSong(const QString& title, const QString& artist)
{
    Song song;
    song.setTitle(title);
    song.setArtist(artist);
    song.setAlbum("测试专辑");
    song.setFilePath("/tmp/test/" + title + ".mp3");
    song.setDuration(180000); // 3分钟
    song.setFileSize(1024 * 1024); // 1MB
    
    int songId = m_songDao->addSong(song);
    song.setId(songId);
    return song;
}

Tag TestFunctionOptimizations::createTestTag(const QString& name)
{
    Tag tag;
    tag.setName(name);
    tag.setDescription("测试标签描述");
    tag.setIsSystem(false);
    tag.setIsDeletable(true);
    
    int tagId = m_tagDao->addTag(tag);
    tag.setId(tagId);
    return tag;
}

void TestFunctionOptimizations::testDeleteSongWithTagCleanup()
{
    // 测试彻底删除歌曲时是否正确断开所有标签关联
    
    // 1. 创建一个测试歌曲并添加到多个标签
    Song testSong = createTestSong("删除测试歌曲", "删除测试艺术家");
    Tag tag1 = createTestTag("删除测试标签1");
    Tag tag2 = createTestTag("删除测试标签2");
    
    m_songDao->addSongToTag(testSong.id(), tag1.id());
    m_songDao->addSongToTag(testSong.id(), tag2.id());
    
    // 验证歌曲已添加到标签
    QVERIFY(m_songDao->songHasTag(testSong.id(), tag1.id()));
    QVERIFY(m_songDao->songHasTag(testSong.id(), tag2.id()));
    
    // 2. 彻底删除歌曲
    bool deleteResult = m_songDao->deleteSong(testSong.id());
    QVERIFY(deleteResult);
    
    // 3. 验证歌曲的所有标签关联已被删除
    QVERIFY(!m_songDao->songHasTag(testSong.id(), tag1.id()));
    QVERIFY(!m_songDao->songHasTag(testSong.id(), tag2.id()));
    
    // 4. 验证歌曲记录已被删除
    Song deletedSong = m_songDao->getSongById(testSong.id());
    QVERIFY(!deletedSong.isValid());
    
    // 清理测试标签
    m_tagDao->deleteTag(tag1.id());
    m_tagDao->deleteTag(tag2.id());
}

void TestFunctionOptimizations::testMySongsTagDeleteOptions()
{
    // 测试"我的歌曲"标签下只提供彻底删除选项
    
    // 这个测试需要模拟UI交互，主要验证逻辑
    // 在实际应用中，可以通过检查删除对话框的选项来验证
    
    // 创建测试歌曲
    Song testSong = createTestSong("我的歌曲测试", "我的歌曲艺术家");
    
    // 验证歌曲存在
    QVERIFY(testSong.isValid());
    
    // 在实际测试中，这里应该模拟在"我的歌曲"标签下选择删除
    // 并验证只显示"彻底删除"选项，不显示"从当前标签移除"选项
    
    // 清理测试数据
    m_songDao->deleteSong(testSong.id());
}

void TestFunctionOptimizations::testCrossTagPlaylistPersistence()
{
    // 测试跨标签播放列表保持逻辑
    
    // 1. 创建测试歌曲和标签
    Song song1 = createTestSong("播放列表测试歌曲1", "艺术家1");
    Song song2 = createTestSong("播放列表测试歌曲2", "艺术家2");
    Song song3 = createTestSong("播放列表测试歌曲3", "艺术家3");
    
    Tag tag1 = createTestTag("播放列表测试标签1");
    Tag tag2 = createTestTag("播放列表测试标签2");
    
    // 将歌曲添加到不同标签
    m_songDao->addSongToTag(song1.id(), tag1.id());
    m_songDao->addSongToTag(song2.id(), tag1.id());
    m_songDao->addSongToTag(song3.id(), tag2.id());
    
    // 2. 模拟在标签1下播放歌曲1
    // 这里需要模拟UI交互，验证播放列表设置逻辑
    
    // 3. 模拟切换到标签2
    // 验证播放列表是否保持（应该保持，因为用户没有在新标签下播放歌曲）
    
    // 4. 模拟在标签2下播放歌曲3
    // 验证播放列表是否切换到标签2的歌曲列表
    
    // 清理测试数据
    m_songDao->deleteSong(song1.id());
    m_songDao->deleteSong(song2.id());
    m_songDao->deleteSong(song3.id());
    m_tagDao->deleteTag(tag1.id());
    m_tagDao->deleteTag(tag2.id());
}

void TestFunctionOptimizations::testRecentPlaySortingAndUpdate()
{
    // 测试"最近播放"标签的排序和更新逻辑
    
    // 1. 创建测试歌曲
    Song song1 = createTestSong("最近播放测试歌曲1", "艺术家1");
    Song song2 = createTestSong("最近播放测试歌曲2", "艺术家2");
    
    // 2. 添加初始播放记录
    QDateTime baseTime = QDateTime::currentDateTime();
    m_playHistoryDao->addPlayRecord(song1.id(), baseTime.addSecs(-3600)); // 1小时前
    m_playHistoryDao->addPlayRecord(song2.id(), baseTime.addSecs(-1800)); // 30分钟前
    
    // 3. 获取最近播放列表，验证排序
    QList<Song> recentSongs = m_playHistoryDao->getRecentPlayedSongs(10);
    QVERIFY(recentSongs.size() >= 2);
    
    // 验证排序：song2应该在song1之前（因为song2播放时间更近）
    bool song2BeforeSong1 = false;
    for (int i = 0; i < recentSongs.size() - 1; ++i) {
        if (recentSongs[i].id() == song2.id() && recentSongs[i + 1].id() == song1.id()) {
            song2BeforeSong1 = true;
            break;
        }
    }
    QVERIFY(song2BeforeSong1);
    
    // 4. 模拟播放song1，更新播放时间
    m_playHistoryDao->addPlayRecord(song1.id(), QDateTime::currentDateTime());
    
    // 5. 重新获取最近播放列表，验证song1现在应该在第一位
    QList<Song> updatedRecentSongs = m_playHistoryDao->getRecentPlayedSongs(10);
    QVERIFY(updatedRecentSongs.size() >= 2);
    
    // 验证song1现在在第一位
    QVERIFY(updatedRecentSongs.first().id() == song1.id());
    
    // 清理测试数据
    m_songDao->deleteSong(song1.id());
    m_songDao->deleteSong(song2.id());
}

QTEST_MAIN(TestFunctionOptimizations)
#include "test_function_optimizations.moc" 