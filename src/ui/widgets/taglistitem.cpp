#include "taglistitem.h"
#include <QMouseEvent>
#include <QPainter>
#include <QStyleOption>
#include <QApplication>
#include <QStyle>
#include <QDebug>

TagListItem::TagListItem(const QString& tagName, 
                        const QString& iconPath,
                        bool isEditable,
                        bool isDeletable,
                        QWidget *parent)
    : QWidget(parent)
    , m_layout(nullptr)
    , m_iconLabel(nullptr)
    , m_nameLabel(nullptr)
    , m_editButton(nullptr)
    , m_tagName(tagName)
    , m_iconPath(iconPath)
    , m_isEditable(isEditable)
    , m_isDeletable(isDeletable)
    , m_isSelected(false)
{
    setupUI();
    updateIcon();
    updateEditButtonVisibility();
}

QString TagListItem::getTagName() const
{
    return m_tagName;
}

void TagListItem::setTagName(const QString& tagName)
{
    if (m_tagName != tagName) {
        m_tagName = tagName;
        if (m_nameLabel) {
            m_nameLabel->setText(m_tagName);
        }
    }
}

QString TagListItem::getIconPath() const
{
    return m_iconPath;
}

void TagListItem::setIcon(const QString& iconPath)
{
    if (m_iconPath != iconPath) {
        m_iconPath = iconPath;
        updateIcon();
    }
}

void TagListItem::setEditable(bool editable)
{
    if (m_isEditable != editable) {
        m_isEditable = editable;
        updateEditButtonVisibility();
    }
}

bool TagListItem::isEditable() const
{
    return m_isEditable;
}

void TagListItem::setDeletable(bool deletable)
{
    m_isDeletable = deletable;
}

bool TagListItem::isDeletable() const
{
    return m_isDeletable;
}

void TagListItem::setSelected(bool selected)
{
    if (m_isSelected != selected) {
        m_isSelected = selected;
        update(); // 触发重绘
    }
}

bool TagListItem::isSelected() const
{
    return m_isSelected;
}

void TagListItem::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        emit tagClicked(m_tagName);
    }
    QWidget::mousePressEvent(event);
}

void TagListItem::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        emit tagDoubleClicked(m_tagName);
    }
    QWidget::mouseDoubleClickEvent(event);
}

void TagListItem::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);
    
    // 绘制选中状态背景
    if (m_isSelected) {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        
        // 使用系统选中颜色
        QColor selectedColor = palette().highlight().color();
        selectedColor.setAlpha(50); // 半透明效果
        
        painter.fillRect(rect(), selectedColor);
        
        // 绘制选中边框
        QPen pen(palette().highlight().color());
        pen.setWidth(2);
        painter.setPen(pen);
        painter.drawRect(rect().adjusted(1, 1, -1, -1));
    }
}

void TagListItem::onEditButtonClicked()
{
    emit editRequested(m_tagName);
}

void TagListItem::setupUI()
{
    // 创建主布局
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(SPACING, SPACING/2, SPACING, SPACING/2);
    m_layout->setSpacing(SPACING);
    
    // 创建图标标签
    m_iconLabel = new QLabel(this);
    m_iconLabel->setFixedSize(ICON_SIZE, ICON_SIZE);
    m_iconLabel->setScaledContents(true);
    m_iconLabel->setAlignment(Qt::AlignCenter);
    
    // 创建名称标签
    m_nameLabel = new QLabel(m_tagName, this);
    m_nameLabel->setStyleSheet(
        "QLabel {"
        "    color: #FFFFFF;"
        "    font-size: 14px;"
        "    font-weight: normal;"
        "    background: transparent;"
        "}"
    );
    
    // 创建编辑按钮
    m_editButton = new QPushButton(this);
    m_editButton->setFixedSize(BUTTON_SIZE, BUTTON_SIZE);
    m_editButton->setText("✎"); // 使用Unicode编辑符号
    m_editButton->setStyleSheet(
        "QPushButton {"
        "    background-color: transparent;"
        "    border: 1px solid #555555;"
        "    border-radius: 10px;"
        "    color: #CCCCCC;"
        "    font-size: 12px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: #444444;"
        "    border-color: #777777;"
        "    color: #FFFFFF;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #333333;"
        "    border-color: #999999;"
        "}"
    );
    m_editButton->setToolTip("编辑标签");
    
    // 连接编辑按钮信号
    connect(m_editButton, &QPushButton::clicked, this, &TagListItem::onEditButtonClicked);
    
    // 添加控件到布局
    m_layout->addWidget(m_iconLabel);
    m_layout->addWidget(m_nameLabel, 1); // 名称标签占据剩余空间
    m_layout->addWidget(m_editButton);
    
    // 设置整体样式
    setStyleSheet(
        "TagListItem {"
        "    background-color: transparent;"
        "    border: none;"
        "}"
        "TagListItem:hover {"
        "    background-color: rgba(255, 255, 255, 20);"
        "}"
    );
    
    // 设置固定高度
    setFixedHeight(ICON_SIZE + SPACING);
}

void TagListItem::updateIcon()
{
    if (!m_iconLabel) {
        return;
    }
    
    if (!m_iconPath.isEmpty()) {
        // 加载自定义图标
        QPixmap pixmap(m_iconPath);
        if (!pixmap.isNull()) {
            m_iconLabel->setPixmap(pixmap.scaled(ICON_SIZE, ICON_SIZE, 
                                               Qt::KeepAspectRatio, 
                                               Qt::SmoothTransformation));
        } else {
            // 如果加载失败，使用默认图标
            setDefaultIcon();
        }
    } else {
        // 使用默认图标
        setDefaultIcon();
    }
}

void TagListItem::setDefaultIcon()
{
    if (!m_iconLabel) {
        return;
    }
    
    // 创建默认图标（简单的彩色方块）
    QPixmap defaultIcon(ICON_SIZE, ICON_SIZE);
    defaultIcon.fill(Qt::transparent);
    
    QPainter painter(&defaultIcon);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // 根据标签名称生成不同颜色
    QColor iconColor;
    if (m_tagName == "我的歌曲") {
        iconColor = QColor("#4CAF50"); // 绿色
    } else if (m_tagName == "最近播放") {
        iconColor = QColor("#2196F3"); // 蓝色
    } else if (m_tagName == "我的收藏") {
        iconColor = QColor("#FF9800"); // 橙色
    } else {
        iconColor = QColor("#9C27B0"); // 紫色
    }
    
    painter.setBrush(iconColor);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(2, 2, ICON_SIZE-4, ICON_SIZE-4, 4, 4);
    
    // 在图标中央绘制简单符号
    painter.setPen(QPen(Qt::white, 2));
    painter.setFont(QFont("Arial", 10, QFont::Bold));
    
    QString symbol;
    if (m_tagName == "我的歌曲") {
        symbol = "♪";
    } else if (m_tagName == "最近播放") {
        symbol = "⟲";
    } else if (m_tagName == "我的收藏") {
        symbol = "♥";
    } else {
        symbol = "#";
    }
    
    painter.drawText(defaultIcon.rect(), Qt::AlignCenter, symbol);
    
    m_iconLabel->setPixmap(defaultIcon);
}

void TagListItem::updateEditButtonVisibility()
{
    if (m_editButton) {
        m_editButton->setVisible(m_isEditable);
    }
}