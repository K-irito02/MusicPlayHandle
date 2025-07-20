#include <QTest>
#include <QDateTime>
#include <QDebug>

#include "../src/database/playhistorydao.h"
#include "../src/database/songdao.h"
#include "../src/models/song.h"

class TestSortingFix : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // 测试最近播放列表排序
    void testRecentPlaySorting();
    
    // 测试时间戳显示
    void testTimestampDisplay();

private:
    PlayHistoryDao* m_playHistoryDao;
    SongDao* m_songDao;
    Song createTestSong(const QString& title, const QString& artist);
    void cleanupTestData();
};

void TestSortingFix::initTestCase()
{
    qDebug() << "初始化排序修复测试环境...";
    
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

void TestSortingFix::cleanupTestCase()
{
    qDebug() << "清理排序修复测试环境...";
    
    // 清理测试数据
    cleanupTestData();
    
    // 清理DAO实例
    delete m_playHistoryDao;
    delete m_songDao;
}

Song TestSortingFix::createTestSong(const QString& title, const QString& artist)
{
    Song song;
    song.setTitle(title);
    song.setArtist(artist);
    song.setAlbum("排序测试专辑");
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

void TestSortingFix::cleanupTestData()
{
    // 清理播放历史记录
    m_playHistoryDao->clearAllPlayHistory();
    
    // 清理测试歌曲
    QList<Song> testSongs = m_songDao->getAllSongs();
    for (const Song& song : testSongs) {
        if (song.title().contains("排序测试")) {
            m_songDao->deleteSong(song.id());
        }
    }
}

void TestSortingFix::testRecentPlaySorting()
{
    qDebug() << "测试最近播放列表排序...";
    
    // 1. 创建测试歌曲
    Song song1 = createTestSong("排序测试歌曲1", "艺术家1");
    Song song2 = createTestSong("排序测试歌曲2", "艺术家2");
    Song song3 = createTestSong("排序测试歌曲3", "艺术家3");
    Song song4 = createTestSong("排序测试歌曲4", "艺术家4");
    Song song5 = createTestSong("排序测试歌曲5", "艺术家5");
    
    QVERIFY(song1.isValid());
    QVERIFY(song2.isValid());
    QVERIFY(song3.isValid());
    QVERIFY(song4.isValid());
    QVERIFY(song5.isValid());
    
    // 2. 按照图片中的时间顺序添加播放记录
    QDateTime baseTime = QDateTime::fromString("2025-07-20 20:00:00", "yyyy-MM-dd hh:mm:ss");
    
    // 按照图片中的时间顺序（从早到晚）
    m_playHistoryDao->addPlayRecord(song5.id(), baseTime.addSecs(0));    // 20:00:05
    m_playHistoryDao->addPlayRecord(song4.id(), baseTime.addSecs(93));   // 20:01:33
    m_playHistoryDao->addPlayRecord(song2.id(), baseTime.addSecs(328));  // 20:05:28
    m_playHistoryDao->addPlayRecord(song3.id(), baseTime.addSecs(356));  // 20:05:56
    m_playHistoryDao->addPlayRecord(song1.id(), baseTime.addSecs(484));  // 20:08:04
    m_playHistoryDao->addPlayRecord(song3.id(), baseTime.addSecs(671));  // 20:11:11
    m_playHistoryDao->addPlayRecord(song2.id(), baseTime.addSecs(862));  // 20:14:22
    m_playHistoryDao->addPlayRecord(song1.id(), baseTime.addSecs(1048)); // 20:17:28
    m_playHistoryDao->addPlayRecord(song1.id(), baseTime.addSecs(1835)); // 20:30:35
    
    // 3. 获取最近播放列表
    QList<Song> recentSongs = m_playHistoryDao->getRecentPlayedSongs(10);
    QVERIFY(recentSongs.size() >= 5);
    
    qDebug() << "获取到的最近播放列表:";
    for (int i = 0; i < recentSongs.size(); ++i) {
        const Song& song = recentSongs[i];
        QDateTime playTime = song.lastPlayedTime();
        QString timeStr = playTime.toString("hh:mm:ss");
        qDebug() << QString("  %1. %2 - %3 (%4)").arg(i + 1).arg(song.artist()).arg(song.title()).arg(timeStr);
    }
    
    // 4. 验证排序正确（最新播放的在前）
    // 根据图片，正确的排序应该是：
    // 1. 电鸟个灯泡 - 梦灯笼 (20:30:35) - song1
    // 2. 可有道,初梦 - Cécile Corbel (20:17:28) - song1
    // 3. 接个吻,开一枪 - YOUTH (20:14:22) - song2
    // 4. 姜鹏Calm,很美味 - Lover Boy 88 (20:11:11) - song3
    // 5. 灰澈 - 森林 (20:08:04) - song1
    
    // 验证第一首是最新播放的
    QVERIFY(recentSongs[0].lastPlayedTime() >= recentSongs[1].lastPlayedTime());
    QVERIFY(recentSongs[1].lastPlayedTime() >= recentSongs[2].lastPlayedTime());
    QVERIFY(recentSongs[2].lastPlayedTime() >= recentSongs[3].lastPlayedTime());
    QVERIFY(recentSongs[3].lastPlayedTime() >= recentSongs[4].lastPlayedTime());
    
    // 验证第一首是20:30:35的歌曲
    QDateTime expectedFirstTime = baseTime.addSecs(1835); // 20:30:35
    QDateTime actualFirstTime = recentSongs[0].lastPlayedTime();
    qint64 timeDiff = qAbs(actualFirstTime.msecsTo(expectedFirstTime));
    QVERIFY(timeDiff <= 1000); // 允许1秒误差
    
    qDebug() << "排序测试通过：最近播放列表按时间正确排序";
}

void TestSortingFix::testTimestampDisplay()
{
    qDebug() << "测试时间戳显示...";
    
    // 1. 创建测试歌曲
    Song testSong = createTestSong("时间戳测试歌曲", "测试艺术家");
    QVERIFY(testSong.isValid());
    
    // 2. 添加播放记录
    QDateTime playTime = QDateTime::fromString("2025-07-20 20:08:04", "yyyy-MM-dd hh:mm:ss");
    bool result = m_playHistoryDao->addPlayRecord(testSong.id(), playTime);
    QVERIFY(result);
    
    // 3. 获取最近播放列表
    QList<Song> recentSongs = m_playHistoryDao->getRecentPlayedSongs(10);
    QVERIFY(!recentSongs.isEmpty());
    
    // 4. 验证时间戳格式
    Song retrievedSong = recentSongs.first();
    QDateTime retrievedTime = retrievedSong.lastPlayedTime();
    QString timeStr = retrievedTime.toString("yyyy/MM-dd/hh-mm-ss");
    
    qDebug() << "期望时间格式: 2025/07-20/20-08-04";
    qDebug() << "实际时间格式:" << timeStr;
    
    QVERIFY(timeStr == "2025/07-20/20-08-04");
    
    qDebug() << "时间戳显示测试通过：时间格式正确";
}

QTEST_MAIN(TestSortingFix)
#include "test_sorting_fix.moc" 