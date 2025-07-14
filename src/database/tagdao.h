#ifndef TAGDAO_H
#define TAGDAO_H

#include <QObject>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariantList>
#include <QStringList>
#include <QDateTime>
#include <QList>
#include "../models/tag.h"

class DatabaseManager;

/**
 * @brief 标签数据访问对象类
 * 
 * 负责标签数据的数据库操作，包括增删改查、关联管理等
 */
class TagDAO : public QObject
{
    Q_OBJECT

public:
    explicit TagDAO(DatabaseManager* dbManager, QObject* parent = nullptr);
    ~TagDAO();

    // 初始化和清理
    bool initialize();
    void cleanup();

    // 基础CRUD操作
    /**
     * @brief 插入标签
     * @param tag 标签对象
     * @return 插入的标签ID，失败返回-1
     */
    int insertTag(const Tag& tag);

    /**
     * @brief 批量插入标签
     * @param tags 标签列表
     * @return 成功插入的标签数量
     */
    int insertTags(const QList<Tag>& tags);

    /**
     * @brief 更新标签
     * @param tag 标签对象
     * @return 是否成功
     */
    bool updateTag(const Tag& tag);

    /**
     * @brief 删除标签
     * @param tagId 标签ID
     * @return 是否成功
     */
    bool deleteTag(int tagId);

    /**
     * @brief 根据名称删除标签
     * @param name 标签名称
     * @return 是否成功
     */
    bool deleteTagByName(const QString& name);

    /**
     * @brief 批量删除标签
     * @param tagIds 标签ID列表
     * @return 成功删除的标签数量
     */
    int deleteTags(const QList<int>& tagIds);

    // 查询操作
    /**
     * @brief 根据ID获取标签
     * @param tagId 标签ID
     * @return 标签对象
     */
    Tag getTag(int tagId);

    /**
     * @brief 根据名称获取标签
     * @param name 标签名称
     * @return 标签对象
     */
    Tag getTagByName(const QString& name);

    /**
     * @brief 获取所有标签
     * @return 标签列表
     */
    QList<Tag> getAllTags();

    /**
     * @brief 获取系统标签
     * @return 系统标签列表
     */
    QList<Tag> getSystemTags();

    /**
     * @brief 获取用户标签
     * @return 用户标签列表
     */
    QList<Tag> getUserTags();

    /**
     * @brief 根据歌曲ID获取标签
     * @param songId 歌曲ID
     * @return 标签列表
     */
    QList<Tag> getTagsBySong(int songId);

    /**
     * @brief 搜索标签
     * @param keyword 关键词
     * @return 标签列表
     */
    QList<Tag> searchTags(const QString& keyword);

    // 统计操作
    /**
     * @brief 获取标签总数
     * @return 标签总数
     */
    int getTagCount();

    /**
     * @brief 获取标签中的歌曲数量
     * @param tagId 标签ID
     * @return 歌曲数量
     */
    int getSongCountInTag(int tagId);

    // 存在性检查
    /**
     * @brief 检查标签是否存在
     * @param tagId 标签ID
     * @return 是否存在
     */
    bool tagExists(int tagId);

    /**
     * @brief 检查标签名称是否存在
     * @param name 标签名称
     * @return 是否存在
     */
    bool nameExists(const QString& name);

    // 标签-歌曲关联操作
    /**
     * @brief 添加歌曲到标签
     * @param songId 歌曲ID
     * @param tagId 标签ID
     * @return 是否成功
     */
    bool addSongToTag(int songId, int tagId);

    /**
     * @brief 从标签中移除歌曲
     * @param songId 歌曲ID
     * @param tagId 标签ID
     * @return 是否成功
     */
    bool removeSongFromTag(int songId, int tagId);

    /**
     * @brief 批量添加歌曲到标签
     * @param songIds 歌曲ID列表
     * @param tagId 标签ID
     * @return 成功添加的歌曲数量
     */
    int addSongsToTag(const QList<int>& songIds, int tagId);

    /**
     * @brief 批量从标签中移除歌曲
     * @param songIds 歌曲ID列表
     * @param tagId 标签ID
     * @return 成功移除的歌曲数量
     */
    int removeSongsFromTag(const QList<int>& songIds, int tagId);

    /**
     * @brief 检查歌曲是否在标签中
     * @param songId 歌曲ID
     * @param tagId 标签ID
     * @return 是否在标签中
     */
    bool isSongInTag(int songId, int tagId);

    /**
     * @brief 获取标签中的所有歌曲ID
     * @param tagId 标签ID
     * @return 歌曲ID列表
     */
    QList<int> getSongIdsInTag(int tagId);

    // 维护操作
    /**
     * @brief 更新标签的歌曲数量
     * @param tagId 标签ID
     * @return 是否成功
     */
    bool updateTagSongCount(int tagId);

    /**
     * @brief 清理孤立的标签-歌曲关联
     * @return 清理的关联数量
     */
    int cleanupOrphanedAssociations();

    // 排序和过滤
    /**
     * @brief 获取按名称排序的标签
     * @param ascending 是否升序
     * @return 标签列表
     */
    QList<Tag> getTagsSortedByName(bool ascending = true);

    /**
     * @brief 获取按歌曲数量排序的标签
     * @param ascending 是否升序
     * @return 标签列表
     */
    QList<Tag> getTagsSortedBySongCount(bool ascending = false);

    /**
     * @brief 获取按创建时间排序的标签
     * @param ascending 是否升序
     * @return 标签列表
     */
    QList<Tag> getTagsSortedByCreateTime(bool ascending = false);

signals:
    /**
     * @brief 标签插入信号
     * @param tag 插入的标签
     */
    void tagInserted(const Tag& tag);

    /**
     * @brief 标签更新信号
     * @param tag 更新的标签
     */
    void tagUpdated(const Tag& tag);

    /**
     * @brief 标签删除信号
     * @param tagId 删除的标签ID
     */
    void tagDeleted(int tagId);

    /**
     * @brief 数据库错误信号
     * @param error 错误信息
     */
    void databaseError(const QString& error);

private:
    DatabaseManager* m_dbManager;

    /**
     * @brief 从查询结果创建标签对象
     * @param query 查询对象
     * @return 标签对象
     */
    Tag createTagFromQuery(const QSqlQuery& query);

    /**
     * @brief 构建搜索条件
     * @param keyword 关键词
     * @param searchFields 搜索字段列表
     * @return SQL条件字符串
     */
    QString buildSearchCondition(const QString& keyword, 
                                const QStringList& searchFields);

    /**
     * @brief 准备查询语句
     * @param query 查询对象
     * @param sql SQL语句
     * @return 是否成功
     */
    bool prepareQuery(QSqlQuery& query, const QString& sql);

    /**
     * @brief 记录错误
     * @param error 错误信息
     */
    void logError(const QString& error);

    /**
     * @brief 记录SQL错误
     * @param query 查询对象
     * @param operation 操作名称
     */
    void logSqlError(const QSqlQuery& query, const QString& operation);

    // SQL语句常量
    struct SQLStatements {
        static const QString INSERT_TAG;
        static const QString UPDATE_TAG;
        static const QString DELETE_TAG;
        static const QString SELECT_TAG_BY_ID;
        static const QString SELECT_TAG_BY_NAME;
        static const QString SELECT_ALL_TAGS;
        static const QString SELECT_SYSTEM_TAGS;
        static const QString SELECT_USER_TAGS;
        static const QString SELECT_TAGS_BY_SONG;
        static const QString SEARCH_TAGS;
        static const QString COUNT_TAGS;
        static const QString COUNT_SONGS_IN_TAG;
        static const QString TAG_EXISTS;
        static const QString NAME_EXISTS;
        static const QString INSERT_SONG_TAG;
        static const QString DELETE_SONG_TAG;
        static const QString SONG_IN_TAG;
        static const QString SELECT_SONG_IDS_IN_TAG;
        static const QString UPDATE_TAG_SONG_COUNT;
        static const QString CLEANUP_ORPHANED_ASSOCIATIONS;
        static const QString SELECT_TAGS_SORTED_BY_NAME;
        static const QString SELECT_TAGS_SORTED_BY_SONG_COUNT;
        static const QString SELECT_TAGS_SORTED_BY_CREATE_TIME;
    };
};

#endif // TAGDAO_H 