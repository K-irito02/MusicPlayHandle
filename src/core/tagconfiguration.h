#ifndef TAGCONFIGURATION_H
#define TAGCONFIGURATION_H

#include <QObject>
#include <QSettings>
#include <QStringList>
#include <QColor>
#include <QMutex>
#include <QMutexLocker>
#include "constants.h"

/**
 * @brief 标签配置管理类
 * @details 负责管理标签相关的配置信息，包括系统标签、颜色、图标等
 */
class TagConfiguration : public QObject
{
    Q_OBJECT
    
public:
    explicit TagConfiguration(QObject* parent = nullptr);
    ~TagConfiguration();
    
    // 单例模式
    static TagConfiguration* instance();
    static void cleanup();
    
    /**
     * @brief 从设置文件加载配置
     * @param settingsPath 设置文件路径（可选）
     */
    void loadFromSettings(const QString& settingsPath = QString());
    
    /**
     * @brief 保存配置到设置文件
     */
    void saveToSettings();
    
    // 系统标签配置
    /**
     * @brief 获取系统标签列表
     * @return 系统标签列表
     */
    QStringList getSystemTags() const;
    
    /**
     * @brief 设置系统标签列表
     * @param tags 系统标签列表
     */
    void setSystemTags(const QStringList& tags);
    
    /**
     * @brief 检查是否为系统标签
     * @param name 标签名称
     * @return 是否为系统标签
     */
    bool isSystemTag(const QString& name) const;
    
    /**
     * @brief 添加系统标签
     * @param name 标签名称
     * @return 是否添加成功
     */
    bool addSystemTag(const QString& name);
    
    /**
     * @brief 移除系统标签
     * @param name 标签名称
     * @return 是否移除成功
     */
    bool removeSystemTag(const QString& name);
    
    // 标签颜色配置
    /**
     * @brief 获取标签颜色
     * @param tagName 标签名称
     * @return 标签颜色
     */
    QColor getTagColor(const QString& tagName) const;
    
    /**
     * @brief 设置标签颜色
     * @param tagName 标签名称
     * @param color 颜色
     */
    void setTagColor(const QString& tagName, const QColor& color);
    
    /**
     * @brief 获取默认标签颜色
     * @return 默认颜色
     */
    QColor getDefaultTagColor() const;
    
    /**
     * @brief 设置默认标签颜色
     * @param color 默认颜色
     */
    void setDefaultTagColor(const QColor& color);
    
    // 标签图标配置
    /**
     * @brief 获取标签图标路径
     * @param tagName 标签名称
     * @return 图标路径
     */
    QString getTagIcon(const QString& tagName) const;
    
    /**
     * @brief 设置标签图标
     * @param tagName 标签名称
     * @param iconPath 图标路径
     */
    void setTagIcon(const QString& tagName, const QString& iconPath);
    
    /**
     * @brief 获取默认标签图标
     * @return 默认图标路径
     */
    QString getDefaultTagIcon() const;
    
    /**
     * @brief 设置默认标签图标
     * @param iconPath 默认图标路径
     */
    void setDefaultTagIcon(const QString& iconPath);
    
    // 显示配置
    /**
     * @brief 获取是否显示系统标签
     * @return 是否显示
     */
    bool getShowSystemTags() const;
    
    /**
     * @brief 设置是否显示系统标签
     * @param show 是否显示
     */
    void setShowSystemTags(bool show);
    
    /**
     * @brief 获取是否允许编辑系统标签
     * @return 是否允许编辑
     */
    bool getAllowEditSystemTags() const;
    
    /**
     * @brief 设置是否允许编辑系统标签
     * @param allow 是否允许编辑
     */
    void setAllowEditSystemTags(bool allow);
    
    /**
     * @brief 获取标签排序方式
     * @return 排序方式（0=名称，1=创建时间，2=使用频率）
     */
    int getTagSortOrder() const;
    
    /**
     * @brief 设置标签排序方式
     * @param order 排序方式
     */
    void setTagSortOrder(int order);
    
    // 行为配置
    /**
     * @brief 获取是否自动创建标签
     * @return 是否自动创建
     */
    bool getAutoCreateTags() const;
    
    /**
     * @brief 设置是否自动创建标签
     * @param autoCreate 是否自动创建
     */
    void setAutoCreateTags(bool autoCreate);
    
    /**
     * @brief 获取最大标签数量限制
     * @return 最大数量（-1表示无限制）
     */
    int getMaxTagCount() const;
    
    /**
     * @brief 设置最大标签数量限制
     * @param maxCount 最大数量
     */
    void setMaxTagCount(int maxCount);
    
    /**
     * @brief 重置为默认配置
     */
    void resetToDefaults();
    
    /**
     * @brief 验证配置有效性
     * @return 是否有效
     */
    bool validateConfiguration() const;
    
signals:
    /**
     * @brief 配置已更改信号
     * @param key 更改的配置键
     */
    void configurationChanged(const QString& key);
    
    /**
     * @brief 系统标签列表已更改信号
     */
    void systemTagsChanged();
    
    /**
     * @brief 标签颜色已更改信号
     * @param tagName 标签名称
     * @param color 新颜色
     */
    void tagColorChanged(const QString& tagName, const QColor& color);
    
private:
    /**
     * @brief 初始化默认配置
     */
    void initializeDefaults();
    
    /**
     * @brief 发出配置更改信号
     * @param key 配置键
     */
    void emitConfigurationChanged(const QString& key);
    
private:
    static TagConfiguration* s_instance;
    mutable QMutex m_mutex;
    QSettings* m_settings;
    
    // 缓存的配置值
    QStringList m_systemTags;
    QHash<QString, QColor> m_tagColors;
    QHash<QString, QString> m_tagIcons;
    QColor m_defaultTagColor;
    QString m_defaultTagIcon;
    bool m_showSystemTags;
    bool m_allowEditSystemTags;
    int m_tagSortOrder;
    bool m_autoCreateTags;
    int m_maxTagCount;
    
    // 配置键常量
    static const QString KEY_SYSTEM_TAGS;
    static const QString KEY_TAG_COLORS;
    static const QString KEY_TAG_ICONS;
    static const QString KEY_DEFAULT_TAG_COLOR;
    static const QString KEY_DEFAULT_TAG_ICON;
    static const QString KEY_SHOW_SYSTEM_TAGS;
    static const QString KEY_ALLOW_EDIT_SYSTEM_TAGS;
    static const QString KEY_TAG_SORT_ORDER;
    static const QString KEY_AUTO_CREATE_TAGS;
    static const QString KEY_MAX_TAG_COUNT;
    
    // 禁用拷贝构造和赋值
    TagConfiguration(const TagConfiguration&) = delete;
    TagConfiguration& operator=(const TagConfiguration&) = delete;
};

#endif // TAGCONFIGURATION_H