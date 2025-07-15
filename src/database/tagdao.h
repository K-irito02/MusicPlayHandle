#ifndef TAGDAO_H
#define TAGDAO_H

#include "basedao.h"
#include "../models/tag.h"
#include <QList>

/**
 * @brief 标签数据访问对象
 * 
 * 提供标签相关的数据库操作
 */
class TagDao : public BaseDao
{
    Q_OBJECT

public:
    explicit TagDao(QObject* parent = nullptr);
    
    /**
     * @brief 添加标签
     * @param tag 标签对象
     * @return 添加成功返回标签ID，失败返回-1
     */
    int addTag(const Tag& tag);
    
    /**
     * @brief 根据ID获取标签
     * @param id 标签ID
     * @return 标签对象，如果不存在返回空对象
     */
    Tag getTagById(int id);
    
    /**
     * @brief 根据名称获取标签
     * @param name 标签名称
     * @return 标签对象，如果不存在返回空对象
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
     * @brief 搜索标签
     * @param keyword 关键词
     * @return 匹配的标签列表
     */
    QList<Tag> searchTags(const QString& keyword);
    
    /**
     * @brief 更新标签
     * @param tag 标签对象
     * @return 更新是否成功
     */
    bool updateTag(const Tag& tag);
    
    /**
     * @brief 删除标签
     * @param id 标签ID
     * @return 删除是否成功
     */
    bool deleteTag(int id);
    
    /**
     * @brief 检查标签是否存在
     * @param name 标签名称
     * @return 是否存在
     */
    bool tagExists(const QString& name);
    
    /**
     * @brief 检查是否为系统标签
     * @param id 标签ID
     * @return 是否为系统标签
     */
    bool isSystemTag(int id);
    
    /**
     * @brief 获取标签总数
     * @return 标签总数
     */
    int getTagCount();
    
    /**
     * @brief 获取用户标签总数
     * @return 用户标签总数
     */
    int getUserTagCount();

private:
    /**
     * @brief 从查询结果创建标签对象
     * @param query 查询结果
     * @return 标签对象
     */
    Tag createTagFromQuery(const QSqlQuery& query);
};

#endif // TAGDAO_H