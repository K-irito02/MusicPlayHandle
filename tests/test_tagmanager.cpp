#include "test_tagmanager.h"
#include "../src/core/servicecontainer.h"
#include "../src/core/itagmanager.h"
#include "../src/core/idatabasemanager.h"
#include "../src/core/result.h"
#include "../src/models/tag.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>
#include <QDir>
#include <QElapsedTimer>
#include <QThread>
#include <QtConcurrent>
#include <QFuture>
#include <QRandomGenerator>

// ========== TestTagManager实现 ==========

TestTagManager::TestTagManager()
    : m_tempDir(nullptr)
    , m_serviceContainer(nullptr)
    , m_tagManager(nullptr)
    , m_databaseManager(nullptr)
{
}

TestTagManager::~TestTagManager()
{
    cleanupTestData();
}

void TestTagManager::initTestCase()
{
    // 创建临时目录
    m_tempDir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_tempDir->isValid());
    
    // 设置依赖注入
    setupDependencyInjection();
    
    // 设置模拟数据库
    setupMockDatabase();
    
    qDebug() << "Test case initialized with temp dir:" << m_tempDir->path();
}

void TestTagManager::cleanupTestCase()
{
    cleanupTestData();
    
    if (m_serviceContainer) {
        m_serviceContainer->clear();
    }
    
    // 关闭数据库连接
    QSqlDatabase::removeDatabase("test_connection");
    
    qDebug() << "Test case cleanup completed";
}

void TestTagManager::init()
{
    // 每个测试开始前清理数据
    cleanupTestData();
    
    // 重置模拟对象状态
    if (auto mockManager = std::dynamic_pointer_cast<MockTagManager>(m_tagManager)) {
        mockManager->setFailureMode(false);
        mockManager->setDatabaseError(false);
        mockManager->clearTestData();
    }
}

void TestTagManager::cleanup()
{
    // 每个测试结束后清理
    cleanupTestData();
}

// ========== 基础功能测试 ==========

void TestTagManager::testCreateTag()
{
    // 准备测试数据
    auto tag = createTestTag("测试标签");
    
    // 执行测试
    bool result = m_tagManager->createTag(tag);
    
    // 验证结果
    QVERIFY(result);
    QVERIFY(tag->getId() > 0);
    
    // 验证标签已保存
    auto savedTag = m_tagManager->getTag(tag->getId());
    QVERIFY(savedTag != nullptr);
    QCOMPARE(savedTag->getName(), "测试标签");
}

void TestTagManager::testCreateTagFailure()
{
    // 设置失败模式
    if (auto mockManager = std::dynamic_pointer_cast<MockTagManager>(m_tagManager)) {
        mockManager->setFailureMode(true);
    }
    
    auto tag = createTestTag("失败标签");
    
    // 执行测试
    bool result = m_tagManager->createTag(tag);
    
    // 验证失败
    QVERIFY(!result);
}

void TestTagManager::testGetTag()
{
    // 创建测试标签
    auto tag = createTestTag("获取测试标签");
    m_tagManager->createTag(tag);
    int tagId = tag->getId();
    
    // 执行测试
    auto retrievedTag = m_tagManager->getTag(tagId);
    
    // 验证结果
    QVERIFY(retrievedTag != nullptr);
    QCOMPARE(retrievedTag->getId(), tagId);
    QCOMPARE(retrievedTag->getName(), "获取测试标签");
}

void TestTagManager::testGetNonExistentTag()
{
    // 尝试获取不存在的标签
    auto tag = m_tagManager->getTag(99999);
    
    // 验证返回空指针
    QVERIFY(tag == nullptr);
}

void TestTagManager::testUpdateTag()
{
    // 创建测试标签
    auto tag = createTestTag("原始标签");
    m_tagManager->createTag(tag);
    
    // 修改标签
    tag->setName("更新后标签");
    tag->setDescription("更新后描述");
    
    // 执行更新
    bool result = m_tagManager->updateTag(tag);
    
    // 验证结果
    QVERIFY(result);
    
    // 验证更新已保存
    auto updatedTag = m_tagManager->getTag(tag->getId());
    QVERIFY(updatedTag != nullptr);
    QCOMPARE(updatedTag->getName(), "更新后标签");
    QCOMPARE(updatedTag->getDescription(), "更新后描述");
}

void TestTagManager::testDeleteTag()
{
    // 创建测试标签
    auto tag = createTestTag("待删除标签");
    m_tagManager->createTag(tag);
    int tagId = tag->getId();
    
    // 执行删除
    bool result = m_tagManager->deleteTag(tagId);
    
    // 验证结果
    QVERIFY(result);
    
    // 验证标签已删除
    auto deletedTag = m_tagManager->getTag(tagId);
    QVERIFY(deletedTag == nullptr);
}

void TestTagManager::testGetAllTags()
{
    // 创建多个测试标签
    auto tag1 = createTestTag("标签1");
    auto tag2 = createTestTag("标签2");
    auto tag3 = createTestTag("标签3");
    
    m_tagManager->createTag(tag1);
    m_tagManager->createTag(tag2);
    m_tagManager->createTag(tag3);
    
    // 获取所有标签
    auto allTags = m_tagManager->getAllTags();
    
    // 验证结果
    QVERIFY(allTags.size() >= 3);
    
    // 验证包含创建的标签
    QStringList tagNames;
    for (const auto& tag : allTags) {
        tagNames << tag->getName();
    }
    
    QVERIFY(tagNames.contains("标签1"));
    QVERIFY(tagNames.contains("标签2"));
    QVERIFY(tagNames.contains("标签3"));
}

// ========== 系统标签测试 ==========

void TestTagManager::testIsSystemTag()
{
    // 测试系统标签
    QVERIFY(m_tagManager->isSystemTag("我的歌曲"));
    QVERIFY(m_tagManager->isSystemTag("我的收藏"));
    QVERIFY(m_tagManager->isSystemTag("最近播放"));
    
    // 测试用户标签
    QVERIFY(!m_tagManager->isSystemTag("用户标签"));
    QVERIFY(!m_tagManager->isSystemTag("自定义标签"));
}

void TestTagManager::testCannotDeleteSystemTag()
{
    // 尝试删除系统标签
    auto systemTag = createTestTag("我的歌曲", true);
    m_tagManager->createTag(systemTag);
    
    // 尝试删除
    bool result = m_tagManager->deleteTag(systemTag->getId());
    
    // 验证删除失败
    QVERIFY(!result);
    
    // 验证标签仍然存在
    auto tag = m_tagManager->getTag(systemTag->getId());
    QVERIFY(tag != nullptr);
}

void TestTagManager::testCannotEditSystemTag()
{
    // 创建系统标签
    auto systemTag = createTestTag("我的收藏", true);
    m_tagManager->createTag(systemTag);
    
    // 尝试修改
    systemTag->setName("修改后的系统标签");
    bool result = m_tagManager->updateTag(systemTag);
    
    // 验证修改失败
    QVERIFY(!result);
}

void TestTagManager::testGetSystemTags()
{
    // 创建系统标签和用户标签
    auto systemTag1 = createTestTag("我的歌曲", true);
    auto systemTag2 = createTestTag("我的收藏", true);
    auto userTag = createTestTag("用户标签", false);
    
    m_tagManager->createTag(systemTag1);
    m_tagManager->createTag(systemTag2);
    m_tagManager->createTag(userTag);
    
    // 获取系统标签
    auto systemTags = m_tagManager->getSystemTags();
    
    // 验证结果
    QVERIFY(systemTags.size() >= 2);
    
    // 验证都是系统标签
    for (const auto& tag : systemTags) {
        QVERIFY(tag->isSystemTag());
    }
}

void TestTagManager::testGetUserTags()
{
    // 创建系统标签和用户标签
    auto systemTag = createTestTag("我的歌曲", true);
    auto userTag1 = createTestTag("用户标签1", false);
    auto userTag2 = createTestTag("用户标签2", false);
    
    m_tagManager->createTag(systemTag);
    m_tagManager->createTag(userTag1);
    m_tagManager->createTag(userTag2);
    
    // 获取用户标签
    auto userTags = m_tagManager->getUserTags();
    
    // 验证结果
    QVERIFY(userTags.size() >= 2);
    
    // 验证都是用户标签
    for (const auto& tag : userTags) {
        QVERIFY(!tag->isSystemTag());
    }
}

// ========== 标签-歌曲关联测试 ==========

void TestTagManager::testAddSongToTag()
{
    // 创建测试标签
    auto tag = createTestTag("歌曲标签");
    m_tagManager->createTag(tag);
    
    // 添加歌曲到标签
    bool result = m_tagManager->addSongToTag(tag->getId(), 1001);
    
    // 验证结果
    QVERIFY(result);
    
    // 验证歌曲已添加
    auto songs = m_tagManager->getSongsInTag(tag->getId());
    QVERIFY(songs.contains(1001));
}

void TestTagManager::testRemoveSongFromTag()
{
    // 创建测试标签并添加歌曲
    auto tag = createTestTag("歌曲标签");
    m_tagManager->createTag(tag);
    m_tagManager->addSongToTag(tag->getId(), 1001);
    
    // 移除歌曲
    bool result = m_tagManager->removeSongFromTag(tag->getId(), 1001);
    
    // 验证结果
    QVERIFY(result);
    
    // 验证歌曲已移除
    auto songs = m_tagManager->getSongsInTag(tag->getId());
    QVERIFY(!songs.contains(1001));
}

void TestTagManager::testGetSongsInTag()
{
    // 创建测试标签
    auto tag = createTestTag("多歌曲标签");
    m_tagManager->createTag(tag);
    
    // 添加多首歌曲
    QList<int> songIds = {1001, 1002, 1003};
    for (int songId : songIds) {
        m_tagManager->addSongToTag(tag->getId(), songId);
    }
    
    // 获取标签中的歌曲
    auto songs = m_tagManager->getSongsInTag(tag->getId());
    
    // 验证结果
    QCOMPARE(songs.size(), 3);
    for (int songId : songIds) {
        QVERIFY(songs.contains(songId));
    }
}

void TestTagManager::testGetTagsForSong()
{
    // 创建多个标签
    auto tag1 = createTestTag("标签1");
    auto tag2 = createTestTag("标签2");
    auto tag3 = createTestTag("标签3");
    
    m_tagManager->createTag(tag1);
    m_tagManager->createTag(tag2);
    m_tagManager->createTag(tag3);
    
    // 将歌曲添加到多个标签
    int songId = 2001;
    m_tagManager->addSongToTag(tag1->getId(), songId);
    m_tagManager->addSongToTag(tag2->getId(), songId);
    
    // 获取歌曲的标签
    auto tags = m_tagManager->getTagsForSong(songId);
    
    // 验证结果
    QCOMPARE(tags.size(), 2);
    QVERIFY(tags.contains(tag1->getId()));
    QVERIFY(tags.contains(tag2->getId()));
    QVERIFY(!tags.contains(tag3->getId()));
}

// ========== 辅助方法实现 ==========

std::shared_ptr<Tag> TestTagManager::createTestTag(const QString& name, bool isSystem)
{
    auto tag = std::make_shared<Tag>();
    tag->setName(name);
    tag->setDescription(QString("测试描述：%1").arg(name));
    tag->setSystemTag(isSystem);
    tag->setColor("#FF0000");
    tag->setIconPath(":/icons/tag.png");
    return tag;
}

QList<int> TestTagManager::createTestSongIds(int count)
{
    QList<int> songIds;
    for (int i = 0; i < count; ++i) {
        songIds << (3000 + i);
    }
    return songIds;
}

void TestTagManager::verifyTag(const std::shared_ptr<Tag>& tag, const QString& expectedName, bool expectedIsSystem)
{
    QVERIFY(tag != nullptr);
    QCOMPARE(tag->getName(), expectedName);
    QCOMPARE(tag->isSystemTag(), expectedIsSystem);
}

void TestTagManager::cleanupTestData()
{
    m_testTagIds.clear();
    m_testSongIds.clear();
}

void TestTagManager::setupMockDatabase()
{
    // 创建内存数据库用于测试
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "test_connection");
    db.setDatabaseName(":memory:");
    
    if (!db.open()) {
        QFAIL("Failed to open test database");
    }
    
    // 创建测试表结构
    QSqlQuery query(db);
    
    // 创建标签表
    QString createTagsTable = R"(
        CREATE TABLE tags (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL UNIQUE,
            description TEXT,
            color TEXT,
            icon_path TEXT,
            is_system INTEGER DEFAULT 0,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    )";
    
    if (!query.exec(createTagsTable)) {
        QFAIL("Failed to create tags table");
    }
    
    // 创建标签-歌曲关联表
    QString createTagSongsTable = R"(
        CREATE TABLE tag_songs (
            tag_id INTEGER,
            song_id INTEGER,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            PRIMARY KEY (tag_id, song_id),
            FOREIGN KEY (tag_id) REFERENCES tags(id) ON DELETE CASCADE
        )
    )";
    
    if (!query.exec(createTagSongsTable)) {
        QFAIL("Failed to create tag_songs table");
    }
}

void TestTagManager::setupDependencyInjection()
{
    m_serviceContainer = std::make_unique<ServiceContainer>();
    
    // 注册模拟的TagManager
    auto mockTagManager = std::make_shared<MockTagManager>();
    m_serviceContainer->registerService<ITagManager>(mockTagManager);
    m_tagManager = mockTagManager;
    
    // 注册其他必要的服务...
}

// ========== MockTagManager实现 ==========

MockTagManager::MockTagManager()
    : m_nextTagId(1)
    , m_failureMode(false)
    , m_databaseError(false)
{
}

MockTagManager::~MockTagManager()
{
}

QList<std::shared_ptr<Tag>> MockTagManager::getAllTags() const
{
    if (m_databaseError) {
        return QList<std::shared_ptr<Tag>>();
    }
    
    QList<std::shared_ptr<Tag>> result;
    for (auto it = m_tags.begin(); it != m_tags.end(); ++it) {
        result.append(it.value());
    }
    return result;
}

std::shared_ptr<Tag> MockTagManager::getTag(int tagId) const
{
    if (m_databaseError) {
        return nullptr;
    }
    
    return m_tags.value(tagId, nullptr);
}

std::shared_ptr<Tag> MockTagManager::getTagByName(const QString& name) const
{
    if (m_databaseError) {
        return nullptr;
    }
    
    for (auto it = m_tags.begin(); it != m_tags.end(); ++it) {
        if (it.value()->getName() == name) {
            return it.value();
        }
    }
    return nullptr;
}

bool MockTagManager::createTag(const std::shared_ptr<Tag>& tag)
{
    if (m_failureMode || m_databaseError) {
        return false;
    }
    
    // 检查重复名称
    if (getTagByName(tag->getName()) != nullptr) {
        return false;
    }
    
    // 分配ID
    tag->setId(m_nextTagId++);
    m_tags[tag->getId()] = tag;
    
    return true;
}

bool MockTagManager::updateTag(const std::shared_ptr<Tag>& tag)
{
    if (m_failureMode || m_databaseError) {
        return false;
    }
    
    // 系统标签不能编辑
    if (tag->isSystemTag()) {
        return false;
    }
    
    if (m_tags.contains(tag->getId())) {
        m_tags[tag->getId()] = tag;
        return true;
    }
    
    return false;
}

bool MockTagManager::deleteTag(int tagId)
{
    if (m_failureMode || m_databaseError) {
        return false;
    }
    
    auto tag = getTag(tagId);
    if (!tag) {
        return false;
    }
    
    // 系统标签不能删除
    if (tag->isSystemTag()) {
        return false;
    }
    
    m_tags.remove(tagId);
    m_tagSongs.remove(tagId);
    
    // 从歌曲标签映射中移除
    for (auto it = m_songTags.begin(); it != m_songTags.end(); ++it) {
        it.value().removeAll(tagId);
    }
    
    return true;
}

QList<std::shared_ptr<Tag>> MockTagManager::getSystemTags() const
{
    QList<std::shared_ptr<Tag>> result;
    for (auto it = m_tags.begin(); it != m_tags.end(); ++it) {
        if (it.value()->isSystemTag()) {
            result.append(it.value());
        }
    }
    return result;
}

QList<std::shared_ptr<Tag>> MockTagManager::getUserTags() const
{
    QList<std::shared_ptr<Tag>> result;
    for (auto it = m_tags.begin(); it != m_tags.end(); ++it) {
        if (!it.value()->isSystemTag()) {
            result.append(it.value());
        }
    }
    return result;
}

bool MockTagManager::isSystemTag(const QString& name) const
{
    QStringList systemTags = {"我的歌曲", "我的收藏", "最近播放", "本地音乐", "下载音乐"};
    return systemTags.contains(name);
}

bool MockTagManager::addSongToTag(int tagId, int songId)
{
    if (m_failureMode || m_databaseError) {
        return false;
    }
    
    if (!m_tags.contains(tagId)) {
        return false;
    }
    
    if (!m_tagSongs[tagId].contains(songId)) {
        m_tagSongs[tagId].append(songId);
    }
    
    if (!m_songTags[songId].contains(tagId)) {
        m_songTags[songId].append(tagId);
    }
    
    return true;
}

bool MockTagManager::removeSongFromTag(int tagId, int songId)
{
    if (m_failureMode || m_databaseError) {
        return false;
    }
    
    m_tagSongs[tagId].removeAll(songId);
    m_songTags[songId].removeAll(tagId);
    
    return true;
}

QList<int> MockTagManager::getSongsInTag(int tagId) const
{
    if (m_databaseError) {
        return QList<int>();
    }
    
    return m_tagSongs.value(tagId, QList<int>());
}

QList<int> MockTagManager::getTagsForSong(int songId) const
{
    if (m_databaseError) {
        return QList<int>();
    }
    
    return m_songTags.value(songId, QList<int>());
}

int MockTagManager::getTagSongCount(int tagId) const
{
    if (m_databaseError) {
        return -1;
    }
    
    return m_tagSongs.value(tagId, QList<int>()).size();
}

QHash<QString, QVariant> MockTagManager::getTagStatistics() const
{
    QHash<QString, QVariant> stats;
    stats["totalTags"] = m_tags.size();
    stats["systemTags"] = getSystemTags().size();
    stats["userTags"] = getUserTags().size();
    return stats;
}

QList<std::shared_ptr<Tag>> MockTagManager::searchTags(const QString& keyword) const
{
    QList<std::shared_ptr<Tag>> result;
    for (auto it = m_tags.begin(); it != m_tags.end(); ++it) {
        if (it.value()->getName().contains(keyword, Qt::CaseInsensitive)) {
            result.append(it.value());
        }
    }
    return result;
}

void MockTagManager::clearTestData()
{
    m_tags.clear();
    m_tagSongs.clear();
    m_songTags.clear();
    m_nextTagId = 1;
}

// 注册测试类
QTEST_MAIN(TestTagManager)
#include "test_tagmanager.moc"