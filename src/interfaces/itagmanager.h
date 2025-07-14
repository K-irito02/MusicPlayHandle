#ifndef ITAGMANAGER_H
#define ITAGMANAGER_H

#include <QObject>
#include <QList>
#include <QString>
#include <QColor>
#include <QPixmap>
#include "../models/tag.h"

/**
 * @brief 标签管理器抽象接口
 * @details 定义标签管理的核心功能接口，便于测试和扩展
 */
class ITagManager
{
public:
    virtual ~ITagManager() = default;
    
    // 基础CRUD操作
    /**
     * @brief 获取所有标签
     * @return 标签列表
     */
    virtual QList<Tag> getAllTags() const = 0;
    
    /**
     * @brief 根据ID获取标签
     * @param tagId 标签ID
     * @return 标签对象，如果不存在则返回无效标签
     */
    virtual Tag getTag(int tagId) const = 0;
    
    /**
     * @brief 根据名称获取标签
     * @param name 标签名称
     * @return 标签对象，如果不存在则返回无效标签
     */
    virtual Tag getTagByName(const QString& name) const = 0;
    
    /**
     * @brief 创建标签
     * @param tag 标签对象
     * @return 是否创建成功
     */
    virtual bool createTag(const Tag& tag) = 0;
    
    /**
     * @brief 更新标签
     * @param tag 标签对象
     * @return 是否更新成功
     */
    virtual bool updateTag(const Tag& tag) = 0;
    
    /**
     * @brief 删除标签
     * @param tagId 标签ID
     * @return 是否删除成功
     */
    virtual bool deleteTag(int tagId) = 0;
    
    // 系统标签相关
    /**
     * @brief 获取系统标签列表
     * @return 系统标签列表
     */
    virtual QStringList getSystemTags() const = 0;
    
    /**
     * @brief 检查是否为系统标签
     * @param name 标签名称
     * @return 是否为系统标签
     */
    virtual bool isSystemTag(const QString& name) const = 0;
    
    // 标签与歌曲关联
    /**
     * @brief 为歌曲添加标签
     * @param songId 歌曲ID
     * @param tagId 标签ID
     * @return 是否添加成功
     */
    virtual bool addTagToSong(int songId, int tagId) = 0;
    
    /**
     * @brief 从歌曲移除标签
     * @param songId 歌曲ID
     * @param tagId 标签ID
     * @return 是否移除成功
     */
    virtual bool removeTagFromSong(int songId, int tagId) = 0;
    
    /**
     * @brief 获取歌曲的所有标签
     * @param songId 歌曲ID
     * @return 标签列表
     */
    virtual QList<Tag> getTagsForSong(int songId) const = 0;
    
    /**
     * @brief 获取标签下的所有歌曲ID
     * @param tagId 标签ID
     * @return 歌曲ID列表
     */
    virtual QList<int> getSongsForTag(int tagId) const = 0;
    
    // 统计信息
    /**
     * @brief 获取标签数量
     * @return 标签总数
     */
    virtual int getTagCount() const = 0;
    
    /**
     * @brief 获取标签下的歌曲数量
     * @param tagId 标签ID
     * @return 歌曲数量
     */
    virtual int getSongCountForTag(int tagId) const = 0;
};

#endif // ITAGMANAGER_H