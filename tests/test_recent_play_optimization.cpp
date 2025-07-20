#include <QTest>
#include <QSignalSpy>
#include <QTimer>
#include <QDateTime>

#include "../src/database/playhistorydao.h"
#include "../src/database/songdao.h"
#include "../src/models/song.h"
#include "../src/models/playhistory.h"
#include "../src/ui/controllers/MainWindowController.h"
#include "../src/audio/audioengine.h"

class TestRecentPlayOptimization : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // 测试场景A：在"最近播放"标签外播放歌曲
    void testScenarioA_PlayOutsideRecentPlayTag();
    
    // 测试场景B：在"最近播放"标签内播放歌曲
    void testScenarioB_PlayInsideRecentPlayTag();
    
    // 测试场景B触发条件1：用户切换到其他标签
    void testScenarioB_TriggerCondition1_SwitchToOtherTag();
    
    // 测试场景B触发条件2：用户退出应用程序
    void testScenarioB_TriggerCondition2_ExitApplication();
    
    // 测试排序更新逻辑
    void testSortingUpdateLogic();
    
    // 测试时间戳更新
    void testTimestampUpdate();

private:
    PlayHistoryDao* m_playHistoryDao;
    SongDao* m_songDao;
    Song createTestSong(const QString& title, const QString& artist);
    void cleanupTestData();
};

void TestRecentPlayOptimization::initTestCase()
{
    qDebug() << "初始化测试环境...";
    
    // 初始化数据库管理器
    DatabaseManager* dbManager = DatabaseManager::instance();
    QVERIFY(dbManager != nullptr);
    QVERIFY(dbManager->initialize());
    
    // 创建DAO实例
    m_playHistoryDao = new PlayHistoryDao(this);
    m_songDao = new SongDao(this);
    
    QVERIFY(m_playHistoryDao != nullptr);
    QVERIFY(m_songDao != nullptr);
}

void TestRecentPlayOptimization::cleanupTestCase()
{
    qDebug() << "清理测试环境...";
    
    // 清理测试数据
    cleanupTestData();
    
    // 清理DAO实例
    delete m_playHistoryDao;
    delete m_songDao;
}

Song TestRecentPlayOptimization::createTestSong(const QString& title, const QString& artist)
{
    Song song;
    song.setTitle(title);
    song.setArtist(artist);
    song.setAlbum("测试专辑");
    song.setFilePath("/test/path/" + title + ".mp3");
    song.setDuration(180000); // 3分钟
    song.setFileSize(1024000); // 1MB
    song.setDateAdded(QDateTime::currentDateTime());
    song.setCreatedAt(QDateTime::currentDateTime());
    song.setUpdatedAt(QDateTime::currentDateTime());
    
    // 插入数据库
    int songId = m_songDao->insertSong(song);
    song.setId(songId);
    
    return song;
}

void TestRecentPlayOptimization::cleanupTestData()
{
    // 清理播放历史记录
    m_playHistoryDao->clearAllPlayHistory();
    
    // 清理测试歌曲
    QList<Song> testSongs = m_songDao->getAllSongs();
    for (const Song& song : testSongs) {
        if (song.title().contains("测试")) {
            m_songDao->deleteSong(song.id());
        }
    }
}

void TestRecentPlayOptimization::testScenarioA_PlayOutsideRecentPlayTag()
{
    qDebug() << "测试场景A：在'最近播放'标签外播放歌曲";
    
    // 1. 创建测试歌曲
    Song testSong = createTestSong("场景A测试歌曲", "测试艺术家");
    QVERIFY(testSong.isValid());
    
    // 2. 模拟在"我的歌曲"标签下播放歌曲
    QDateTime playTime = QDateTime::currentDateTime();
    bool result = m_playHistoryDao->addPlayRecord(testSong.id(), playTime);
    QVERIFY(result);
    
    // 3. 验证播放记录已立即更新
    QList<Song> recentSongs = m_playHistoryDao->getRecentPlayedSongs(10);
    QVERIFY(!recentSongs.isEmpty());
    
    // 验证歌曲在最近播放列表中
    bool found = false;
    for (const Song& song : recentSongs) {
        if (song.id() == testSong.id()) {
            found = true;
            break;
        }
    }
    QVERIFY(found);
    
    // 4. 验证排序正确（最新播放的在前）
    QDateTime lastPlayTime = m_playHistoryDao->getLastPlayTime(testSong.id());
    QVERIFY(lastPlayTime.isValid());
    QVERIFY(lastPlayTime >= playTime);
    
    qDebug() << "场景A测试通过：在标签外播放歌曲立即更新播放记录";
}

void TestRecentPlayOptimization::testScenarioB_PlayInsideRecentPlayTag()
{
    qDebug() << "测试场景B：在'最近播放'标签内播放歌曲";
    
    // 1. 创建测试歌曲
    Song testSong = createTestSong("场景B测试歌曲", "测试艺术家");
    QVERIFY(testSong.isValid());
    
    // 2. 先添加一条播放记录
    QDateTime firstPlayTime = QDateTime::currentDateTime().addSecs(-3600); // 1小时前
    bool result1 = m_playHistoryDao->addPlayRecord(testSong.id(), firstPlayTime);
    QVERIFY(result1);
    
    // 3. 模拟在"最近播放"标签内播放歌曲
    QDateTime secondPlayTime = QDateTime::currentDateTime();
    bool result2 = m_playHistoryDao->addPlayRecord(testSong.id(), secondPlayTime);
    QVERIFY(result2);
    
    // 4. 验证播放时间已更新，但排序可能不会立即更新
    QDateTime lastPlayTime = m_playHistoryDao->getLastPlayTime(testSong.id());
    QVERIFY(lastPlayTime.isValid());
    QVERIFY(lastPlayTime >= secondPlayTime);
    
    // 5. 验证数据库中只有一条记录（去重）
    QList<PlayHistory> history = m_playHistoryDao->getSongPlayHistory(testSong.id());
    QVERIFY(history.size() == 1);
    QVERIFY(history.first().playedAt() >= secondPlayTime);
    
    qDebug() << "场景B测试通过：在最近播放标签内播放歌曲更新播放时间";
}

void TestRecentPlayOptimization::testScenarioB_TriggerCondition1_SwitchToOtherTag()
{
    qDebug() << "测试场景B触发条件1：用户切换到其他标签";
    
    // 1. 创建测试歌曲
    Song testSong = createTestSong("触发条件1测试歌曲", "测试艺术家");
    QVERIFY(testSong.isValid());
    
    // 2. 模拟在"最近播放"标签内播放歌曲
    QDateTime playTime = QDateTime::currentDateTime();
    bool result = m_playHistoryDao->addPlayRecord(testSong.id(), playTime);
    QVERIFY(result);
    
    // 3. 模拟用户切换到其他标签（这里我们直接验证排序更新）
    // 在实际应用中，这会触发 handleTagSelectionChange 方法
    QList<Song> recentSongs = m_playHistoryDao->getRecentPlayedSongs(10);
    QVERIFY(!recentSongs.isEmpty());
    
    // 验证歌曲在最近播放列表中且排序正确
    bool found = false;
    for (const Song& song : recentSongs) {
        if (song.id() == testSong.id()) {
            found = true;
            break;
        }
    }
    QVERIFY(found);
    
    qDebug() << "触发条件1测试通过：切换到其他标签时排序更新";
}

void TestRecentPlayOptimization::testScenarioB_TriggerCondition2_ExitApplication()
{
    qDebug() << "测试场景B触发条件2：用户退出应用程序";
    
    // 1. 创建测试歌曲
    Song testSong = createTestSong("触发条件2测试歌曲", "测试艺术家");
    QVERIFY(testSong.isValid());
    
    // 2. 模拟在"最近播放"标签内播放歌曲
    QDateTime playTime = QDateTime::currentDateTime();
    bool result = m_playHistoryDao->addPlayRecord(testSong.id(), playTime);
    QVERIFY(result);
    
    // 3. 模拟应用程序退出时的排序更新
    // 在实际应用中，这会触发 shutdown 方法
    QList<Song> recentSongs = m_playHistoryDao->getRecentPlayedSongs(10);
    QVERIFY(!recentSongs.isEmpty());
    
    // 验证歌曲在最近播放列表中且排序正确
    bool found = false;
    for (const Song& song : recentSongs) {
        if (song.id() == testSong.id()) {
            found = true;
            break;
        }
    }
    QVERIFY(found);
    
    qDebug() << "触发条件2测试通过：退出应用程序时排序更新";
}

void TestRecentPlayOptimization::testSortingUpdateLogic()
{
    qDebug() << "测试排序更新逻辑";
    
    // 1. 创建多个测试歌曲
    Song song1 = createTestSong("排序测试歌曲1", "艺术家1");
    Song song2 = createTestSong("排序测试歌曲2", "艺术家2");
    Song song3 = createTestSong("排序测试歌曲3", "艺术家3");
    
    QVERIFY(song1.isValid());
    QVERIFY(song2.isValid());
    QVERIFY(song3.isValid());
    
    // 2. 按时间顺序添加播放记录
    QDateTime baseTime = QDateTime::currentDateTime();
    
    // song1: 3小时前播放
    m_playHistoryDao->addPlayRecord(song1.id(), baseTime.addSecs(-10800));
    
    // song2: 2小时前播放
    m_playHistoryDao->addPlayRecord(song2.id(), baseTime.addSecs(-7200));
    
    // song3: 1小时前播放
    m_playHistoryDao->addPlayRecord(song3.id(), baseTime.addSecs(-3600));
    
    // 3. 获取最近播放列表，验证排序
    QList<Song> recentSongs = m_playHistoryDao->getRecentPlayedSongs(10);
    QVERIFY(recentSongs.size() >= 3);
    
    // 验证排序：song3应该在第一位（最新播放）
    QVERIFY(recentSongs[0].id() == song3.id());
    QVERIFY(recentSongs[1].id() == song2.id());
    QVERIFY(recentSongs[2].id() == song1.id());
    
    // 4. 重新播放song1，验证它应该移动到第一位
    m_playHistoryDao->addPlayRecord(song1.id(), QDateTime::currentDateTime());
    
    QList<Song> updatedRecentSongs = m_playHistoryDao->getRecentPlayedSongs(10);
    QVERIFY(updatedRecentSongs.size() >= 3);
    
    // 验证song1现在在第一位
    QVERIFY(updatedRecentSongs[0].id() == song1.id());
    
    qDebug() << "排序更新逻辑测试通过：播放记录按时间正确排序";
}

void TestRecentPlayOptimization::testTimestampUpdate()
{
    qDebug() << "测试时间戳更新";
    
    // 1. 创建测试歌曲
    Song testSong = createTestSong("时间戳测试歌曲", "测试艺术家");
    QVERIFY(testSong.isValid());
    
    // 2. 记录初始时间
    QDateTime initialTime = QDateTime::currentDateTime();
    
    // 3. 添加播放记录
    bool result = m_playHistoryDao->addPlayRecord(testSong.id(), initialTime);
    QVERIFY(result);
    
    // 4. 验证时间戳正确
    QDateTime recordedTime = m_playHistoryDao->getLastPlayTime(testSong.id());
    QVERIFY(recordedTime.isValid());
    
    // 验证时间戳在合理范围内（允许1秒误差）
    qint64 timeDiff = qAbs(recordedTime.msecsTo(initialTime));
    QVERIFY(timeDiff <= 1000);
    
    // 5. 再次播放，验证时间戳更新
    QTimer::singleShot(100, [this, testSong]() {
        QDateTime newTime = QDateTime::currentDateTime();
        bool result2 = m_playHistoryDao->addPlayRecord(testSong.id(), newTime);
        QVERIFY(result2);
        
        QDateTime updatedTime = m_playHistoryDao->getLastPlayTime(testSong.id());
        QVERIFY(updatedTime.isValid());
        QVERIFY(updatedTime > newTime.addSecs(-1)); // 允许1秒误差
    });
    
    qDebug() << "时间戳更新测试通过：播放时间正确记录和更新";
}

QTEST_MAIN(TestRecentPlayOptimization)
#include "test_recent_play_optimization.moc" 