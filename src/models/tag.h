#ifndef TAG_H
#define TAG_H

#include <QString>
#include <QDateTime>
#include <QVariant>
#include <QJsonObject>
#include <QMetaType>

/**
 * @brief 标签数据模型类
 * 
 * 表示一个音乐标签的所有信息，包括基本信息、外观设置、统计信息等
 */
class Tag
{
public:
    /**
     * @brief 标签类型枚举
     */
    enum TagType {
        UserTag = 0,    ///< 用户标签
        SystemTag = 1   ///< 系统标签
    };
    
    /**
     * @brief 默认构造函数
     */
    Tag();
    
    /**
     * @brief 带参数构造函数
     * @param name 标签名称
     * @param description 标签描述
     * @param type 标签类型
     */
    Tag(const QString& name, const QString& description = QString(), 
        TagType type = UserTag);
    
    /**
     * @brief 拷贝构造函数
     * @param other 其他标签对象
     */
    Tag(const Tag& other);
    
    /**
     * @brief 赋值操作符
     * @param other 其他标签对象
     * @return 当前对象引用
     */
    Tag& operator=(const Tag& other);
    
    /**
     * @brief 相等操作符
     * @param other 其他标签对象
     * @return 是否相等
     */
    bool operator==(const Tag& other) const;
    
    /**
     * @brief 不等操作符
     * @param other 其他标签对象
     * @return 是否不等
     */
    bool operator!=(const Tag& other) const;
    
    // Getters
    int id() const { return m_id; }
    QString name() const { return m_name; }
    QString description() const { return m_description; }
    QString coverPath() const { return m_coverPath; }
    QString color() const { return m_color; }
    TagType tagType() const { return m_tagType; }
    bool isSystem() const { return m_isSystem; }
    bool isDeletable() const { return m_isDeletable; }
    int sortOrder() const { return m_sortOrder; }
    int songCount() const { return m_songCount; }
    QDateTime createdAt() const { return m_createdAt; }
    QDateTime updatedAt() const { return m_updatedAt; }
    
    // Setters
    void setId(int id) { m_id = id; }
    void setName(const QString& name) { m_name = name; }
    void setDescription(const QString& description) { m_description = description; }
    void setCoverPath(const QString& coverPath) { m_coverPath = coverPath; }
    void setColor(const QString& color) { m_color = color; }
    void setTagType(TagType type) { m_tagType = type; }
    void setIsSystem(bool isSystem) { m_isSystem = isSystem; }
    void setIsDeletable(bool isDeletable) { m_isDeletable = isDeletable; }
    void setSortOrder(int sortOrder) { m_sortOrder = sortOrder; }
    void setSongCount(int songCount) { m_songCount = songCount; }
    void setCreatedAt(const QDateTime& createdAt) { m_createdAt = createdAt; }
    void setUpdatedAt(const QDateTime& updatedAt) { m_updatedAt = updatedAt; }
    
    /**
     * @brief 检查标签是否有效
     * @return 是否有效
     */
    bool isValid() const;
    
    /**
     * @brief 获取显示名称
     * @return 显示名称
     */
    QString displayName() const;
    
    /**
     * @brief 获取标签类型字符串
     * @return 标签类型字符串
     */
    QString tagTypeString() const;
    
    /**
     * @brief 增加歌曲数量
     * @param count 增加的数量
     */
    void addSongCount(int count = 1);
    
    /**
     * @brief 减少歌曲数量
     * @param count 减少的数量
     */
    void removeSongCount(int count = 1);
    
    /**
     * @brief 重置歌曲数量
     * @param count 新的数量
     */
    void resetSongCount(int count = 0);
    
    /**
     * @brief 检查是否可以删除
     * @return 是否可以删除
     */
    bool canDelete() const;
    
    /**
     * @brief 检查是否可以编辑
     * @return 是否可以编辑
     */
    bool canEdit() const;
    
    /**
     * @brief 创建系统标签
     * @param name 标签名称
     * @param description 标签描述
     * @param sortOrder 排序顺序
     * @param isDeletable 是否可删除
     * @return 系统标签实例
     */
    static Tag createSystemTag(const QString& name, const QString& description, 
                               int sortOrder, bool isDeletable = false);
    
    /**
     * @brief 创建用户标签
     * @param name 标签名称
     * @param description 标签描述
     * @param color 标签颜色
     * @return 用户标签实例
     */
    static Tag createUserTag(const QString& name, const QString& description = QString(), 
                             const QString& color = "#3498db");
    
    /**
     * @brief 从JSON对象创建Tag实例
     * @param json JSON对象
     * @return Tag实例
     */
    static Tag fromJson(const QJsonObject& json);
    
    /**
     * @brief 转换为JSON对象
     * @return JSON对象
     */
    QJsonObject toJson() const;
    
    /**
     * @brief 从QVariantMap创建Tag实例
     * @param map QVariantMap
     * @return Tag实例
     */
    static Tag fromVariantMap(const QVariantMap& map);
    
    /**
     * @brief 转换为QVariantMap
     * @return QVariantMap
     */
    QVariantMap toVariantMap() const;
    
    /**
     * @brief 清空数据
     */
    void clear();
    
    /**
     * @brief 判断是否为空
     * @return 是否为空
     */
    bool isEmpty() const;
    
    /**
     * @brief 获取调试字符串
     * @return 调试字符串
     */
    QString toString() const;
    
    // 系统标签常量
    static const QString DEFAULT_TAG_NAME;      ///< 默认标签名称
    static const QString MY_MUSIC_TAG_NAME;     ///< 我的歌曲标签名称
    static const QString FAVORITE_TAG_NAME;     ///< 收藏标签名称
    
private:
    int m_id;                           ///< 标签ID
    QString m_name;                     ///< 标签名称
    QString m_description;              ///< 标签描述
    QString m_coverPath;                ///< 标签封面路径
    QString m_color;                    ///< 标签颜色
    TagType m_tagType;                  ///< 标签类型
    bool m_isSystem;                    ///< 是否系统标签
    bool m_isDeletable;                 ///< 是否可删除
    int m_sortOrder;                    ///< 排序序号
    int m_songCount;                    ///< 歌曲数量
    QDateTime m_createdAt;              ///< 创建时间
    QDateTime m_updatedAt;              ///< 更新时间
    
    /**
     * @brief 更新修改时间
     */
    void updateModifiedTime();
};

// 使Tag可以在QVariant中使用
Q_DECLARE_METATYPE(Tag)

#endif // TAG_H 