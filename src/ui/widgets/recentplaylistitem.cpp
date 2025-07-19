#include "recentplaylistitem.h"
#include <QMouseEvent>
#include <QPainter>
#include <QStyleOption>
#include <QApplication>
#include <QFontMetrics>
#include <QToolTip>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QPixmap>
#include <QIcon>
#include <QEnterEvent>

RecentPlayListItem::RecentPlayListItem(const Song& song, const QDateTime& playTime, QWidget* parent)
    : QWidget(parent)
    , m_song(song)
    , m_playTime(playTime)
    , m_selected(false)
    , m_hovered(false)
    , m_layout(nullptr)
    , m_iconLabel(nullptr)
    , m_titleLabel(nullptr)
    , m_artistLabel(nullptr)
    , m_timeLabel(nullptr)
    , m_playButton(nullptr)
    , m_menuButton(nullptr)
{
    setupUI();
    setupConnections();
    updateDisplay();
    updateStyle();
    
    // 设置固定高度
    setFixedHeight(ITEM_HEIGHT);
    
    // 设置工具提示
    QString tooltip = QString("文件: %1\n时长: %2\n播放时间: %3")
                     .arg(song.filePath())
                     .arg(QString::number(song.duration()))
                     .arg(playTime.toString("yyyy/MM-dd/hh-mm-ss"));
    setToolTip(tooltip);
}

RecentPlayListItem::~RecentPlayListItem()
{
}

void RecentPlayListItem::setPlayTime(const QDateTime& playTime)
{
    m_playTime = playTime;
    updateDisplay();
}

void RecentPlayListItem::updateDisplay()
{
    if (!m_titleLabel || !m_artistLabel || !m_timeLabel) {
        return;
    }
    
    // 设置标题和艺术家
    QString title = m_song.title().isEmpty() ? "未知标题" : m_song.title();
    QString artist = m_song.artist().isEmpty() ? "未知艺术家" : m_song.artist();
    
    m_titleLabel->setText(title);
    m_artistLabel->setText(artist);
    
    // 设置播放时间
    if (m_playTime.isValid()) {
        QString timeStr = m_playTime.toString("yyyy/MM-dd/hh-mm-ss");
        m_timeLabel->setText(timeStr);
    } else {
        m_timeLabel->setText("未知时间");
    }
    
    // 设置图标
    if (m_iconLabel) {
        // 这里可以设置歌曲图标，暂时使用默认图标
        m_iconLabel->setPixmap(QPixmap(":/new/prefix1/images/playIcon.png").scaled(24, 24, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}

void RecentPlayListItem::setSelected(bool selected)
{
    if (m_selected != selected) {
        m_selected = selected;
        updateStyle();
    }
}

void RecentPlayListItem::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        emit itemClicked(m_song);
    }
    QWidget::mousePressEvent(event);
}

void RecentPlayListItem::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        emit itemDoubleClicked(m_song);
    }
    QWidget::mouseDoubleClickEvent(event);
}

void RecentPlayListItem::enterEvent(QEnterEvent* event)
{
    m_hovered = true;
    updateStyle();
    QWidget::enterEvent(event);
}

void RecentPlayListItem::leaveEvent(QEvent* event)
{
    m_hovered = false;
    updateStyle();
    QWidget::leaveEvent(event);
}

void RecentPlayListItem::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // 绘制背景
    QColor backgroundColor;
    if (m_selected) {
        backgroundColor = QColor(52, 152, 219); // 选中时的蓝色
    } else if (m_hovered) {
        backgroundColor = QColor(236, 240, 241); // 悬停时的浅灰色
    } else {
        backgroundColor = Qt::white;
    }
    
    painter.fillRect(rect(), backgroundColor);
    
    // 绘制边框
    if (m_selected || m_hovered) {
        QPen pen;
        pen.setColor(QColor(189, 195, 199));
        pen.setWidth(1);
        painter.setPen(pen);
        painter.drawRect(rect().adjusted(0, 0, -1, -1));
    }
    
    QWidget::paintEvent(event);
}

void RecentPlayListItem::setupUI()
{
    // 创建主布局
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(8, 4, 8, 4);
    m_layout->setSpacing(8);
    
    // 创建图标标签
    m_iconLabel = new QLabel(this);
    m_iconLabel->setFixedSize(24, 24);
    m_iconLabel->setScaledContents(true);
    m_layout->addWidget(m_iconLabel);
    
    // 创建标题和艺术家标签的垂直布局
    QVBoxLayout* textLayout = new QVBoxLayout();
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(2);
    
    m_titleLabel = new QLabel(this);
    m_titleLabel->setFont(QFont("Microsoft YaHei", 9, QFont::Medium));
    m_titleLabel->setTextFormat(Qt::PlainText);
    textLayout->addWidget(m_titleLabel);
    
    m_artistLabel = new QLabel(this);
    m_artistLabel->setFont(QFont("Microsoft YaHei", 8));
    m_artistLabel->setTextFormat(Qt::PlainText);
    m_artistLabel->setStyleSheet("color: #7f8c8d;");
    textLayout->addWidget(m_artistLabel);
    
    m_layout->addLayout(textLayout);
    m_layout->addStretch();
    
    // 创建时间标签
    m_timeLabel = new QLabel(this);
    m_timeLabel->setFont(QFont("Microsoft YaHei", 8));
    m_timeLabel->setTextFormat(Qt::PlainText);
    m_timeLabel->setStyleSheet("color: #95a5a6;");
    m_timeLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_layout->addWidget(m_timeLabel);
    
    // 创建播放按钮
    m_playButton = new QPushButton(this);
    m_playButton->setFixedSize(24, 24);
    m_playButton->setIcon(QIcon(":/new/prefix1/images/playIcon.png"));
    m_playButton->setIconSize(QSize(16, 16));
    m_playButton->setStyleSheet(
        "QPushButton {"
        "    border: none;"
        "    background-color: transparent;"
        "    border-radius: 12px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #3498db;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #2980b9;"
        "}"
    );
    m_layout->addWidget(m_playButton);
    
    // 创建菜单按钮
    m_menuButton = new QPushButton(this);
    m_menuButton->setFixedSize(24, 24);
    m_menuButton->setIcon(QIcon(":/new/prefix1/images/manageIcon.png"));
    m_menuButton->setIconSize(QSize(16, 16));
    m_menuButton->setStyleSheet(
        "QPushButton {"
        "    border: none;"
        "    background-color: transparent;"
        "    border-radius: 12px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #e74c3c;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #c0392b;"
        "}"
    );
    m_layout->addWidget(m_menuButton);
}

void RecentPlayListItem::setupConnections()
{
    if (m_playButton) {
        connect(m_playButton, &QPushButton::clicked, [this]() {
            emit playButtonClicked(m_song);
        });
    }
    
    if (m_menuButton) {
        connect(m_menuButton, &QPushButton::clicked, [this]() {
            // 这里可以显示上下文菜单
            // 暂时不实现，由父组件处理
        });
    }
}

void RecentPlayListItem::updateStyle()
{
    // 根据选中和悬停状态更新样式
    QString styleSheet;
    
    if (m_selected) {
        styleSheet = "RecentPlayListItem { background-color: #3498db; color: white; }";
    } else if (m_hovered) {
        styleSheet = "RecentPlayListItem { background-color: #ecf0f1; }";
    } else {
        styleSheet = "RecentPlayListItem { background-color: white; }";
    }
    
    setStyleSheet(styleSheet);
    
    // 更新文本颜色
    if (m_titleLabel) {
        if (m_selected) {
            m_titleLabel->setStyleSheet("color: white;");
            m_artistLabel->setStyleSheet("color: #bdc3c7;");
            m_timeLabel->setStyleSheet("color: #bdc3c7;");
        } else {
            m_titleLabel->setStyleSheet("color: black;");
            m_artistLabel->setStyleSheet("color: #7f8c8d;");
            m_timeLabel->setStyleSheet("color: #95a5a6;");
        }
    }
} 