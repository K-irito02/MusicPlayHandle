#ifndef TAGLISTITEM_H
#define TAGLISTITEM_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QPixmap>
#include <QString>

/**
 * @brief 自定义标签列表项控件
 * 
 * 该控件用于显示标签信息，包括：
 * - 左侧：标签图标
 * - 中间：标签名称
 * - 右侧：编辑按钮（仅对可编辑标签显示）
 * 
 * 支持拖拽重排序功能
 */
class TagListItem : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param tagName 标签名称
     * @param iconPath 标签图标路径
     * @param isEditable 是否可编辑（影响编辑按钮的显示）
     * @param isDeletable 是否可删除
     * @param parent 父控件
     */
    explicit TagListItem(const QString& tagName, 
                        const QString& iconPath = QString(),
                        bool isEditable = true,
                        bool isDeletable = true,
                        QWidget *parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~TagListItem();

    /**
     * @brief 获取标签名称
     * @return 标签名称
     */
    QString getTagName() const;

    /**
     * @brief 设置标签名称
     * @param tagName 新的标签名称
     */
    void setTagName(const QString& tagName);

    /**
     * @brief 获取标签图标路径
     * @return 图标路径
     */
    QString getIconPath() const;

    /**
     * @brief 设置标签图标
     * @param iconPath 图标路径
     */
    void setIcon(const QString& iconPath);

    /**
     * @brief 设置是否可编辑
     * @param editable 是否可编辑
     */
    void setEditable(bool editable);

    /**
     * @brief 获取是否可编辑
     * @return 是否可编辑
     */
    bool isEditable() const;

    /**
     * @brief 设置是否可删除
     * @param deletable 是否可删除
     */
    void setDeletable(bool deletable);

    /**
     * @brief 获取是否可删除
     * @return 是否可删除
     */
    bool isDeletable() const;

    /**
     * @brief 设置选中状态
     * @param selected 是否选中
     */
    void setSelected(bool selected);

    /**
     * @brief 获取选中状态
     * @return 是否选中
     */
    bool isSelected() const;

signals:
    /**
     * @brief 编辑按钮被点击信号
     * @param tagName 标签名称
     */
    void editRequested(const QString& tagName);

    /**
     * @brief 标签被点击信号
     * @param tagName 标签名称
     */
    void tagClicked(const QString& tagName);

    /**
     * @brief 标签被双击信号
     * @param tagName 标签名称
     */
    void tagDoubleClicked(const QString& tagName);

protected:
    /**
     * @brief 鼠标点击事件
     * @param event 鼠标事件
     */
    void mousePressEvent(QMouseEvent* event) override;

    /**
     * @brief 鼠标双击事件
     * @param event 鼠标事件
     */
    void mouseDoubleClickEvent(QMouseEvent* event) override;

    /**
     * @brief 绘制事件（用于绘制选中状态）
     * @param event 绘制事件
     */
    void paintEvent(QPaintEvent* event) override;

private slots:
    /**
     * @brief 编辑按钮点击槽函数
     */
    void onEditButtonClicked();

private:
    /**
     * @brief 初始化UI
     */
    void setupUI();

    /**
     * @brief 更新图标显示
     */
    void updateIcon();

    /**
     * @brief 更新编辑按钮可见性
     */
    void updateEditButtonVisibility();

    /**
     * @brief 设置默认图标
     */
    void setDefaultIcon();

private:
    QHBoxLayout* m_layout;          ///< 主布局
    QLabel* m_iconLabel;            ///< 图标标签
    QLabel* m_nameLabel;            ///< 名称标签
    QPushButton* m_editButton;      ///< 编辑按钮
    
    QString m_tagName;              ///< 标签名称
    QString m_iconPath;             ///< 图标路径
    bool m_isEditable;              ///< 是否可编辑
    bool m_isDeletable;             ///< 是否可删除
    bool m_isSelected;              ///< 是否选中
    
    static const int ICON_SIZE = 24;        ///< 图标大小
    static const int BUTTON_SIZE = 20;      ///< 按钮大小
    static const int SPACING = 8;           ///< 间距
};

#endif // TAGLISTITEM_H