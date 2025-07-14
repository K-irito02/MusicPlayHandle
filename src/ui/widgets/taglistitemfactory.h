#ifndef TAGLISTITEMFACTORY_H
#define TAGLISTITEMFACTORY_H

#include <memory>
#include <QString>
#include <QWidget>
#include "taglistitem.h"
#include "../../core/constants.h"

/**
 * @brief TagListItem工厂类
 * @details 提供统一的TagListItem创建接口，确保不同类型标签的一致性
 */
class TagListItemFactory
{
public:
    /**
     * @brief 创建系统标签项
     * @param name 标签名称
     * @param parent 父控件
     * @return 系统标签项的智能指针
     */
    static std::unique_ptr<TagListItem> createSystemTag(const QString& name, QWidget* parent = nullptr)
    {
        QString iconPath = getSystemTagIcon(name);
        auto item = std::make_unique<TagListItem>(name, iconPath, false, false, parent);
        
        // 设置系统标签的特殊样式
        item->setStyleSheet(getSystemTagStyleSheet());
        
        return item;
    }
    
    /**
     * @brief 创建用户标签项
     * @param name 标签名称
     * @param iconPath 图标路径（可选）
     * @param parent 父控件
     * @return 用户标签项的智能指针
     */
    static std::unique_ptr<TagListItem> createUserTag(const QString& name, 
                                                      const QString& iconPath = QString(), 
                                                      QWidget* parent = nullptr)
    {
        QString finalIconPath = iconPath.isEmpty() ? getDefaultUserTagIcon() : iconPath;
        auto item = std::make_unique<TagListItem>(name, finalIconPath, true, true, parent);
        
        // 设置用户标签的样式
        item->setStyleSheet(getUserTagStyleSheet());
        
        return item;
    }
    
    /**
     * @brief 创建只读标签项
     * @param name 标签名称
     * @param iconPath 图标路径（可选）
     * @param parent 父控件
     * @return 只读标签项的智能指针
     */
    static std::unique_ptr<TagListItem> createReadOnlyTag(const QString& name,
                                                          const QString& iconPath = QString(),
                                                          QWidget* parent = nullptr)
    {
        QString finalIconPath = iconPath.isEmpty() ? getDefaultUserTagIcon() : iconPath;
        auto item = std::make_unique<TagListItem>(name, finalIconPath, false, false, parent);
        
        // 设置只读标签的样式
        item->setStyleSheet(getReadOnlyTagStyleSheet());
        
        return item;
    }
    
    /**
     * @brief 批量创建系统标签
     * @param parent 父控件
     * @return 系统标签项列表
     */
    static QList<std::unique_ptr<TagListItem>> createAllSystemTags(QWidget* parent = nullptr)
    {
        QList<std::unique_ptr<TagListItem>> items;
        const auto systemTags = Constants::SystemTags::getAll();
        
        for (const QString& tagName : systemTags) {
            items.append(createSystemTag(tagName, parent));
        }
        
        return items;
    }
    
    /**
     * @brief 根据标签名称自动选择创建类型
     * @param name 标签名称
     * @param iconPath 图标路径（可选）
     * @param parent 父控件
     * @return 标签项的智能指针
     */
    static std::unique_ptr<TagListItem> createAutoTag(const QString& name,
                                                      const QString& iconPath = QString(),
                                                      QWidget* parent = nullptr)
    {
        if (Constants::SystemTags::isSystemTag(name)) {
            return createSystemTag(name, parent);
        } else {
            return createUserTag(name, iconPath, parent);
        }
    }
    
    /**
     * @brief 克隆标签项
     * @param original 原始标签项
     * @param parent 新的父控件
     * @return 克隆的标签项
     */
    static std::unique_ptr<TagListItem> cloneTag(const TagListItem* original, QWidget* parent = nullptr)
    {
        if (!original) {
            return nullptr;
        }
        
        auto cloned = std::make_unique<TagListItem>(
            original->getTagName(),
            original->getIconPath(),
            original->isEditable(),
            original->isDeletable(),
            parent
        );
        
        cloned->setSelected(original->isSelected());
        cloned->setStyleSheet(original->styleSheet());
        
        return cloned;
    }
    
private:
    /**
     * @brief 获取系统标签图标路径
     * @param tagName 标签名称
     * @return 图标路径
     */
    static QString getSystemTagIcon(const QString& tagName)
    {
        if (tagName == Constants::SystemTags::MY_SONGS) {
            return QStringLiteral(":/images/playlistIcon.png");
        } else if (tagName == Constants::SystemTags::FAVORITES) {
            return QStringLiteral(":/images/addToListIcon.png");
        } else if (tagName == Constants::SystemTags::RECENT_PLAYED) {
            return QStringLiteral(":/images/followingSongIcon.png");
        } else if (tagName == Constants::SystemTags::DEFAULT_TAG) {
            return QStringLiteral(":/images/createIcon.png");
        }
        return QStringLiteral(":/images/playlistIcon.png"); // 默认图标
    }
    
    /**
     * @brief 获取默认用户标签图标
     * @return 图标路径
     */
    static QString getDefaultUserTagIcon()
    {
        return QStringLiteral(":/images/editLabel.png");
    }
    
    /**
     * @brief 获取系统标签样式表
     * @return 样式表字符串
     */
    static QString getSystemTagStyleSheet()
    {
        return QStringLiteral(
            "TagListItem {"
            "    background-color: #f5f5f5;"
            "    border: 1px solid #e0e0e0;"
            "    border-radius: 4px;"
            "    padding: 4px;"
            "}"
            "TagListItem:hover {"
            "    background-color: #eeeeee;"
            "}"
            "TagListItem QLabel {"
            "    color: #666666;"
            "    font-weight: bold;"
            "}"
        );
    }
    
    /**
     * @brief 获取用户标签样式表
     * @return 样式表字符串
     */
    static QString getUserTagStyleSheet()
    {
        return QStringLiteral(
            "TagListItem {"
            "    background-color: #ffffff;"
            "    border: 1px solid #d0d0d0;"
            "    border-radius: 4px;"
            "    padding: 4px;"
            "}"
            "TagListItem:hover {"
            "    background-color: #f0f8ff;"
            "    border-color: #2196f3;"
            "}"
            "TagListItem QLabel {"
            "    color: #333333;"
            "}"
        );
    }
    
    /**
     * @brief 获取只读标签样式表
     * @return 样式表字符串
     */
    static QString getReadOnlyTagStyleSheet()
    {
        return QStringLiteral(
            "TagListItem {"
            "    background-color: #fafafa;"
            "    border: 1px solid #e8e8e8;"
            "    border-radius: 4px;"
            "    padding: 4px;"
            "}"
            "TagListItem QLabel {"
            "    color: #888888;"
            "}"
        );
    }
};

#endif // TAGLISTITEMFACTORY_H