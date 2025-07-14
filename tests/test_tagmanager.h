#ifndef TEST_TAGMANAGER_H
#define TEST_TAGMANAGER_H

#include <QtTest/QtTest>
#include <QObject>
#include <QSqlDatabase>
#include <QTemporaryDir>
#include <memory>

// 前置声明
class ITagManager;
class IDatabaseManager;
class ServiceContainer;
class Tag;

/**
 * @brief TagManager单元测试类
 * @details 测试TagManager的所有核心功能，包括CRUD操作、系统标签处理等
 */
class TestTagManager : public QObject
{
    Q_OBJECT
    
public:
    TestTagManager();
    ~TestTagManager();
    
private slots:
    // 测试框架方法
    void initTestCase();        // 整个测试开始前执行一次
    void cleanupTestCase();     // 整个测试结束后执行一次
    void init();                // 每个测试方法开始前执行
    void cleanup();             // 每个测试方法结束后执行
    
    // ========== 基础功能测试 ==========
    
    /**
     * @brief 测试标签创建功能
     */
    void testCreateTag();
    
    /**
     * @brief 测试标签创建失败情况
     */
    void testCreateTagFailure();
    
    /**
     * @brief 测试获取标签功能
     */
    void testGetTag();
    
    /**
     * @brief 测试获取不存在的标签
     */
    void testGetNonExistentTag();
    
    /**
     * @brief 测试更新标签功能
     */
    void testUpdateTag();
    
    /**
     * @brief 测试删除标签功能
     */
    void testDeleteTag();
    
    /**
     * @brief 测试获取所有标签
     */
    void testGetAllTags();
    
    // ========== 系统标签测试 ==========
    
    /**
     * @brief 测试系统标签识别
     */
    void testIsSystemTag();
    
    /**
     * @brief 测试系统标签不能删除
     */
    void testCannotDeleteSystemTag();
    
    /**
     * @brief 测试系统标签不能编辑
     */
    void testCannotEditSystemTag();
    
    /**
     * @brief 测试获取系统标签列表
     */
    void testGetSystemTags();
    
    /**
     * @brief 测试获取用户标签列表
     */
    void testGetUserTags();
    
    // ========== 标签-歌曲关联测试 ==========
    
    /**
     * @brief 测试添加歌曲到标签
     */
    void testAddSongToTag();
    
    /**
     * @brief 测试从标签移除歌曲
     */
    void testRemoveSongFromTag();
    
    /**
     * @brief 测试获取标签中的歌曲
     */
    void testGetSongsInTag();
    
    /**
     * @brief 测试获取歌曲的标签
     */
    void testGetTagsForSong();
    
    /**
     * @brief 测试批量添加歌曲到标签
     */
    void testBatchAddSongsToTag();
    
    /**
     * @brief 测试批量移除歌曲从标签
     */
    void testBatchRemoveSongsFromTag();
    
    // ========== 统计功能测试 ==========
    
    /**
     * @brief 测试获取标签歌曲数量
     */
    void testGetTagSongCount();
    
    /**
     * @brief 测试获取标签统计信息
     */
    void testGetTagStatistics();
    
    /**
     * @brief 测试获取最常用标签
     */
    void testGetMostUsedTags();
    
    // ========== 搜索和过滤测试 ==========
    
    /**
     * @brief 测试按名称搜索标签
     */
    void testSearchTagsByName();
    
    /**
     * @brief 测试按类型过滤标签
     */
    void testFilterTagsByType();
    
    /**
     * @brief 测试按歌曲数量过滤标签
     */
    void testFilterTagsBySongCount();
    
    // ========== 边界条件和错误处理测试 ==========
    
    /**
     * @brief 测试空标签名称处理
     */
    void testEmptyTagName();
    
    /**
     * @brief 测试重复标签名称处理
     */
    void testDuplicateTagName();
    
    /**
     * @brief 测试标签名称长度限制
     */
    void testTagNameLengthLimit();
    
    /**
     * @brief 测试无效标签ID处理
     */
    void testInvalidTagId();
    
    /**
     * @brief 测试数据库连接失败处理
     */
    void testDatabaseConnectionFailure();
    
    // ========== 性能测试 ==========
    
    /**
     * @brief 测试大量标签创建性能
     */
    void testBulkTagCreationPerformance();
    
    /**
     * @brief 测试大量标签查询性能
     */
    void testBulkTagQueryPerformance();
    
    /**
     * @brief 测试标签-歌曲关联性能
     */
    void testTagSongAssociationPerformance();
    
    // ========== 并发测试 ==========
    
    /**
     * @brief 测试并发标签创建
     */
    void testConcurrentTagCreation();
    
    /**
     * @brief 测试并发标签读写
     */
    void testConcurrentTagReadWrite();
    
private:
    // ========== 辅助方法 ==========
    
    /**
     * @brief 创建测试标签
     * @param name 标签名称
     * @param isSystem 是否为系统标签
     * @return 创建的标签
     */
    std::shared_ptr<Tag> createTestTag(const QString& name, bool isSystem = false);
    
    /**
     * @brief 创建测试歌曲ID列表
     * @param count 歌曲数量
     * @return 歌曲ID列表
     */
    QList<int> createTestSongIds(int count);
    
    /**
     * @brief 验证标签数据
     * @param tag 标签对象
     * @param expectedName 期望的名称
     * @param expectedIsSystem 期望的系统标签状态
     */
    void verifyTag(const std::shared_ptr<Tag>& tag, const QString& expectedName, bool expectedIsSystem);
    
    /**
     * @brief 清理测试数据
     */
    void cleanupTestData();
    
    /**
     * @brief 设置模拟数据库
     */
    void setupMockDatabase();
    
    /**
     * @brief 创建依赖注入容器
     */
    void setupDependencyInjection();
    
    /**
     * @brief 测量执行时间
     * @param operation 要测量的操作
     * @return 执行时间（毫秒）
     */
    template<typename Func>
    qint64 measureExecutionTime(Func operation) {
        QElapsedTimer timer;
        timer.start();
        operation();
        return timer.elapsed();
    }
    
private:
    // 测试环境
    std::unique_ptr<QTemporaryDir> m_tempDir;
    std::unique_ptr<ServiceContainer> m_serviceContainer;
    std::shared_ptr<ITagManager> m_tagManager;
    std::shared_ptr<IDatabaseManager> m_databaseManager;
    
    // 测试数据
    QList<int> m_testTagIds;
    QList<int> m_testSongIds;
    
    // 测试配置
    static const int PERFORMANCE_TEST_COUNT = 1000;
    static const int BULK_TEST_COUNT = 100;
    static const qint64 MAX_ACCEPTABLE_TIME_MS = 1000;
};

/**
 * @brief 标签管理器模拟类
 * @details 用于测试的TagManager模拟实现
 */
class MockTagManager : public ITagManager
{
public:
    MockTagManager();
    ~MockTagManager() override;
    
    // ITagManager接口实现
    QList<std::shared_ptr<Tag>> getAllTags() const override;
    std::shared_ptr<Tag> getTag(int tagId) const override;
    std::shared_ptr<Tag> getTagByName(const QString& name) const override;
    bool createTag(const std::shared_ptr<Tag>& tag) override;
    bool updateTag(const std::shared_ptr<Tag>& tag) override;
    bool deleteTag(int tagId) override;
    
    QList<std::shared_ptr<Tag>> getSystemTags() const override;
    QList<std::shared_ptr<Tag>> getUserTags() const override;
    bool isSystemTag(const QString& name) const override;
    
    bool addSongToTag(int tagId, int songId) override;
    bool removeSongFromTag(int tagId, int songId) override;
    QList<int> getSongsInTag(int tagId) const override;
    QList<int> getTagsForSong(int songId) const override;
    
    int getTagSongCount(int tagId) const override;
    QHash<QString, QVariant> getTagStatistics() const override;
    
    QList<std::shared_ptr<Tag>> searchTags(const QString& keyword) const override;
    
    // 模拟控制方法
    void setFailureMode(bool enabled) { m_failureMode = enabled; }
    void setDatabaseError(bool enabled) { m_databaseError = enabled; }
    void clearTestData();
    
private:
    QHash<int, std::shared_ptr<Tag>> m_tags;
    QHash<int, QList<int>> m_tagSongs; // tagId -> songIds
    QHash<int, QList<int>> m_songTags; // songId -> tagIds
    int m_nextTagId;
    bool m_failureMode;
    bool m_databaseError;
};

#endif // TEST_TAGMANAGER_H