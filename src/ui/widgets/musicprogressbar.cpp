#include "musicprogressbar.h"
#include <QApplication>
#include <QDebug>
#include <QStyleOptionSlider>
#include <QToolTip>
#include <QSlider>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QStyle>
#include <QMutex>
#include <QMutexLocker>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <algorithm>

// ==================== PreciseSlider 实现 ====================

PreciseSlider::PreciseSlider(Qt::Orientation orientation, QWidget *parent)
    : QSlider(orientation, parent)
    , m_isDragging(false)
    , m_duration(0)
    , m_dragPreviewPosition(-1)
{
    setTracking(true);
    setMouseTracking(true);
    
    // 确保能接收鼠标事件
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_TransparentForMouseEvents, false);
    
    // 拖拽预览功能通过paintEvent实现，无需定时器
    
    // 安装事件过滤器到父组件
    if (parent) {
        parent->installEventFilter(this);
    }
    
    qDebug() << "[PreciseSlider] 构造函数完成，组件地址:" << this;
    qDebug() << "[PreciseSlider] 父组件地址:" << parent;
    qDebug() << "[PreciseSlider] 初始时长:" << m_duration << "ms";
    qDebug() << "[PreciseSlider] 鼠标事件透明:" << testAttribute(Qt::WA_TransparentForMouseEvents);
}

void PreciseSlider::mousePressEvent(QMouseEvent *event)
{
    qDebug() << "[PreciseSlider] ===== mousePressEvent 开始 =====";
    qDebug() << "[PreciseSlider] 原始点击位置:" << event->pos();
    qDebug() << "[PreciseSlider] 当前时长:" << m_duration << "ms";
    
    if (event->button() == Qt::LeftButton) {
        qDebug() << "[PreciseSlider] ✓ 左键点击，开始精确处理";
        
        // 将坐标转换为相对于PreciseSlider的坐标
        QPoint relativePos = mapFromParent(event->pos());
        qDebug() << "[PreciseSlider] 相对点击位置:" << relativePos;
        
        // 立即计算精确位置并发送信号（此时m_isDragging为false，所以是点击）
        updatePositionFromMouse(relativePos);
        
        // 不立即设置拖拽状态，等待mouseMoveEvent
        qDebug() << "[PreciseSlider] ✓ 精确点击处理完成";
        event->accept();
    } else {
        qDebug() << "[PreciseSlider] ✗ 不是左键点击，交给基类处理";
        QSlider::mousePressEvent(event);
    }
    
    qDebug() << "[PreciseSlider] ===== mousePressEvent 结束 =====";
}

void PreciseSlider::mouseMoveEvent(QMouseEvent *event)
{
    // 检查是否按下左键（拖拽状态）
    if (event->buttons() & Qt::LeftButton) {
        if (!m_isDragging) {
            // 第一次移动，设置拖拽状态
            m_isDragging = true;
            qDebug() << "[PreciseSlider] 检测到拖拽开始";
        }
        
        // 将坐标转换为相对于PreciseSlider的坐标
        QPoint relativePos = mapFromParent(event->pos());
        
        // 计算目标位置
        qint64 targetPosition = positionFromMouseX(relativePos.x());
        
        // 存储拖拽预览位置
        m_dragPreviewPosition = targetPosition;
        
        // 触发重绘以显示预览位置
        update();
        
        // 拖拽时不发送位置变化信号，只在释放时发送最终跳转信号
        // 这样可以避免与外部位置更新产生冲突
        
        // 接受事件，避免传递给基类导致额外的处理
        event->accept();
        return;
    }
    
    QSlider::mouseMoveEvent(event);
}

void PreciseSlider::mouseReleaseEvent(QMouseEvent *event)
{
    qDebug() << "[PreciseSlider] ===== mouseReleaseEvent 开始 =====";
    
    if (event->button() == Qt::LeftButton) {
        if (m_isDragging) {
            qDebug() << "[PreciseSlider] ✓ 拖拽结束，发送最终跳转信号";
            
            // 将坐标转换为相对于PreciseSlider的坐标
            QPoint relativePos = mapFromParent(event->pos());
            
            // 拖拽结束时发送最终跳转信号
            qint64 finalPosition = positionFromMouseX(relativePos.x());
            emit preciseSeekRequested(finalPosition);
            
            // 重置拖拽状态和预览位置
            m_isDragging = false;
            m_dragPreviewPosition = -1;
            
            // 触发重绘以清除预览
            update();
            
            qDebug() << "[PreciseSlider] ✓ 拖拽结束处理完成，最终位置:" << finalPosition << "ms";
        } else {
            qDebug() << "[PreciseSlider] ✓ 点击释放，无需额外处理";
        }
        
        event->accept();
    } else {
        qDebug() << "[PreciseSlider] ✗ 不是左键释放，交给基类处理";
        QSlider::mouseReleaseEvent(event);
    }
    
    qDebug() << "[PreciseSlider] ===== mouseReleaseEvent 结束 =====";
}

void PreciseSlider::paintEvent(QPaintEvent *event)
{
    // 先调用基类的绘制方法
    QSlider::paintEvent(event);
    
    // 如果正在拖拽且有有效的预览位置，绘制预览
    if (m_isDragging && m_dragPreviewPosition >= 0 && m_duration > 0) {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        
        // 计算预览位置的x坐标
        QRect sliderRect = rect();
        double ratio = static_cast<double>(m_dragPreviewPosition) / m_duration;
        int previewX = static_cast<int>(ratio * sliderRect.width());
        
        // 绘制预览线（垂直线）
        QPen previewLinePen(QColor(0, 120, 215), 2); // 蓝色预览线
        painter.setPen(previewLinePen);
        painter.drawLine(previewX, 0, previewX, sliderRect.height());
        
        // 绘制预览点（小圆点）
        QBrush previewPointBrush(QColor(0, 120, 215));
        painter.setBrush(previewPointBrush);
        painter.setPen(Qt::NoPen);
        int centerY = sliderRect.height() / 2;
        painter.drawEllipse(previewX - 3, centerY - 3, 6, 6);
    }
}

qint64 PreciseSlider::positionFromMouseX(int x) const
{
    if (m_duration <= 0) {
        return 0;
    }
    
    // 获取滑块的有效区域（相对于PreciseSlider本身）
    QRect sliderRect = rect();
    int sliderWidth = sliderRect.width();
    
    if (sliderWidth <= 0) {
        return 0;
    }
    
    // 确保x坐标在有效范围内
    x = std::clamp(x, 0, sliderWidth);
    
    // 计算相对位置比例
    double ratio = static_cast<double>(x) / sliderWidth;
    ratio = std::clamp(ratio, 0.0, 1.0);
    
    // 转换为时间位置（毫秒）
    qint64 position = static_cast<qint64>(ratio * m_duration);
    
    // 确保位置在有效范围内
    position = std::clamp(position, static_cast<qint64>(0), m_duration);
    
    return position;
}

void PreciseSlider::updatePositionFromMouse(const QPoint& pos)
{
    qint64 targetPosition = positionFromMouseX(pos.x());
    
    if (m_isDragging) {
        // 拖拽时使用防抖机制，不在这里发送信号
        // 调试信息已移除以提高性能
    } else {
        // 点击时发送跳转信号
        qDebug() << "[PreciseSlider] 点击，准备发送preciseSeekRequested信号:" << targetPosition << "ms";
        emit preciseSeekRequested(targetPosition);
        qDebug() << "[PreciseSlider] 点击，preciseSeekRequested信号已发送:" << targetPosition << "ms";
    }
}

// 设置时长（供外部调用）
void PreciseSlider::setDuration(qint64 duration)
{
    qDebug() << "[PreciseSlider] 设置时长:" << duration << "ms";
    m_duration = duration;
}

// 已移除onDragTimerTimeout方法，因为使用paintEvent实现拖拽预览

bool PreciseSlider::eventFilter(QObject *obj, QEvent *event)
{
    // 只处理父组件的鼠标事件
    if (obj == parent()) {
        switch (event->type()) {
            case QEvent::MouseButtonPress: {
                QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
                qDebug() << "[PreciseSlider] 事件过滤器接收到鼠标按下事件:" << mouseEvent->pos();
                
                // 检查点击是否在PreciseSlider区域内
                QPoint localPos = mapFromParent(mouseEvent->pos());
                if (rect().contains(localPos)) {
                    qDebug() << "[PreciseSlider] 点击在PreciseSlider区域内，处理事件";
                    mousePressEvent(mouseEvent);
                    return true; // 事件已处理
                }
                break;
            }
            case QEvent::MouseMove: {
                QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
                QPoint localPos = mapFromParent(mouseEvent->pos());
                if (rect().contains(localPos)) {
                    mouseMoveEvent(mouseEvent);
                    return true; // 事件已处理
                }
                break;
            }
            case QEvent::MouseButtonRelease: {
                QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
                qDebug() << "[PreciseSlider] 事件过滤器接收到鼠标释放事件:" << mouseEvent->pos();
                
                QPoint localPos = mapFromParent(mouseEvent->pos());
                if (rect().contains(localPos)) {
                    qDebug() << "[PreciseSlider] 释放在PreciseSlider区域内，处理事件";
                    mouseReleaseEvent(mouseEvent);
                    return true; // 事件已处理
                }
                break;
            }
            default:
                break;
        }
    }
    
    return QSlider::eventFilter(obj, event);
}

// ==================== MusicProgressBar 实现 ====================

MusicProgressBar::MusicProgressBar(QWidget *parent)
    : QWidget(parent)
    , m_slider(nullptr)
    , m_currentTimeLabel(nullptr)
    , m_totalTimeLabel(nullptr)
    , m_mainLayout(nullptr)
    , m_timeLayout(nullptr)
    , m_position(0)
    , m_duration(0)
    , m_minimum(0)
    , m_maximum(0)
    , m_userInteracting(false)
    , m_enabled(true)
    , m_seekDebounceTimer(nullptr)
    , m_pendingSeekPosition(-1)
    , m_progressBarStyle("")
    , m_timeLabelsStyle("")
{
    setupUI();
    setupConnections();
    
    // 设置蓝色主题样式
    setProgressBarStyle(
        "QSlider::groove:horizontal {"
        "    border: 1px solid #4A90E2;"
        "    height: 8px;"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #E3F2FD, stop:1 #BBDEFB);"
        "    margin: 2px 0;"
        "    border-radius: 4px;"
        "}"
        "QSlider::handle:horizontal {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #2196F3, stop:1 #1976D2);"
        "    border: 1px solid #1565C0;"
        "    width: 18px;"
        "    margin: -2px 0;"
        "    border-radius: 9px;"
        "}"
        "QSlider::handle:horizontal:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #42A5F5, stop:1 #1E88E5);"
        "}"
        "QSlider::sub-page:horizontal {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #2196F3, stop:1 #1976D2);"
        "    border: 1px solid #1565C0;"
        "    height: 8px;"
        "    border-radius: 4px;"
        "}"
    );
    
    setTimeLabelsStyle(
        "QLabel {"
        "    color: #333333;"
        "    font-family: 'Consolas', 'Monaco', monospace;"
        "    font-size: 11px;"
        "    font-weight: bold;"
        "    background: transparent;"
        "    border: none;"
        "    padding: 2px 4px;"
        "}"
    );
    
    qDebug() << "[MusicProgressBar] 组件初始化完成";
}

MusicProgressBar::~MusicProgressBar()
{
    qDebug() << "[MusicProgressBar] 组件销毁";
}

void MusicProgressBar::setupUI()
{
    // 创建主布局（使用QVBoxLayout实现垂直布局）
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(2);
    
    // 创建进度条滑块
    m_slider = new PreciseSlider(Qt::Horizontal, this);
    m_slider->setMinimum(0);
    m_slider->setMaximum(0);
    m_slider->setValue(0);
    m_slider->setTracking(true);
    m_slider->setMouseTracking(true);
    
    // 移除这行代码，让QSlider能够正常响应点击事件
    // m_slider->setPageStep(0);
    
    // 创建时间标签布局
    m_timeLayout = new QHBoxLayout();
    m_timeLayout->setContentsMargins(0, 0, 0, 0);
    m_timeLayout->setSpacing(0);
    
    // 创建时间标签
    m_currentTimeLabel = new QLabel("00:00", this);
    m_currentTimeLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_currentTimeLabel->setMinimumWidth(45);
    m_currentTimeLabel->setMaximumWidth(60);
    
    m_totalTimeLabel = new QLabel("00:00", this);
    m_totalTimeLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_totalTimeLabel->setMinimumWidth(45);
    m_totalTimeLabel->setMaximumWidth(60);
    
    // 添加到时间布局
    m_timeLayout->addWidget(m_currentTimeLabel);
    m_timeLayout->addStretch(); // 弹性空间
    m_timeLayout->addWidget(m_totalTimeLabel);
    
    // 添加到主布局（先添加滑块，再添加时间标签布局）
    m_mainLayout->addWidget(m_slider);
    m_mainLayout->addLayout(m_timeLayout);
    
    // 设置布局
    setLayout(m_mainLayout);
    
    // 设置最小尺寸
    setMinimumHeight(40);
    setMaximumHeight(60);
    
    // 启用鼠标追踪 - 修复：确保能够接收鼠标事件
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
}

void MusicProgressBar::setupConnections()
{
    // 连接精确滑块信号
    connect(m_slider, &PreciseSlider::preciseSeekRequested, this, &MusicProgressBar::onPreciseSeekRequested);
    connect(m_slider, &PreciseSlider::precisePositionChanged, this, &MusicProgressBar::onPrecisePositionChanged);
    
    // 保留传统滑块信号连接（兼容性）
    connect(m_slider, &PreciseSlider::sliderPressed, this, &MusicProgressBar::onSliderPressed);
    connect(m_slider, &PreciseSlider::sliderReleased, this, &MusicProgressBar::onSliderReleased);
    connect(m_slider, &PreciseSlider::sliderMoved, this, &MusicProgressBar::onSliderMoved);
    connect(m_slider, &PreciseSlider::valueChanged, this, &MusicProgressBar::onSliderValueChanged);
}

void MusicProgressBar::setPosition(qint64 position)
{
    QMutexLocker locker(&m_mutex);
    setPositionInternal(position);
}

void MusicProgressBar::setDuration(qint64 duration)
{
    QMutexLocker locker(&m_mutex);
    setDurationInternal(duration);
    
    // 同时设置PreciseSlider的时长（在主线程中）
    QMetaObject::invokeMethod(this, [this, duration]() {
        if (m_slider) {
            m_slider->setDuration(duration);
        }
    }, Qt::QueuedConnection);
}

void MusicProgressBar::setRange(qint64 minimum, qint64 maximum)
{
    QMutexLocker locker(&m_mutex);
    
    m_minimum = minimum;
    m_maximum = maximum;
    
    // 更新滑块范围（使用毫秒值，但限制在合理范围内）
    int sliderMin = static_cast<int>(minimum / 1000); // 转换为秒
    int sliderMax = static_cast<int>(maximum / 1000); // 转换为秒
    
    // 确保范围有效
    if (sliderMax <= sliderMin) {
        sliderMax = sliderMin + 1;
    }
    
    // 在主线程中更新UI
    QMetaObject::invokeMethod(this, [this, sliderMin, sliderMax]() {
        m_slider->setRange(sliderMin, sliderMax);
    }, Qt::QueuedConnection);
    
    qDebug() << "[MusicProgressBar] 设置范围:" << minimum << "-" << maximum;
}

qint64 MusicProgressBar::position() const
{
    QMutexLocker locker(&m_mutex);
    return m_position;
}

qint64 MusicProgressBar::duration() const
{
    QMutexLocker locker(&m_mutex);
    return m_duration;
}

qint64 MusicProgressBar::minimum() const
{
    QMutexLocker locker(&m_mutex);
    return m_minimum;
}

qint64 MusicProgressBar::maximum() const
{
    QMutexLocker locker(&m_mutex);
    return m_maximum;
}

void MusicProgressBar::setEnabled(bool enabled)
{
    QMutexLocker locker(&m_mutex);
    m_enabled = enabled;
    
    // 在主线程中更新UI
    QMetaObject::invokeMethod(this, [this, enabled]() {
        m_slider->setEnabled(enabled);
        QWidget::setEnabled(enabled);
    }, Qt::QueuedConnection);
}

bool MusicProgressBar::isEnabled() const
{
    QMutexLocker locker(&m_mutex);
    return m_enabled;
}

void MusicProgressBar::setProgressBarStyle(const QString& style)
{
    m_progressBarStyle = style;
    if (m_slider) {
        m_slider->setStyleSheet(style);
    }
}

void MusicProgressBar::setTimeLabelsStyle(const QString& style)
{
    m_timeLabelsStyle = style;
    if (m_currentTimeLabel) {
        m_currentTimeLabel->setStyleSheet(style);
    }
    if (m_totalTimeLabel) {
        m_totalTimeLabel->setStyleSheet(style);
    }
}

void MusicProgressBar::reset()
{
    QMutexLocker locker(&m_mutex);
    
    m_position = 0;
    m_duration = 0;
    m_minimum = 0;
    m_maximum = 0;
    m_userInteracting = false;
    m_pendingSeekPosition = -1;
    
    // 停止延迟定时器
    if (m_seekDebounceTimer) {
        m_seekDebounceTimer->stop();
    }
    
    // 在主线程中更新UI
    QMetaObject::invokeMethod(this, [this]() {
        m_slider->setRange(0, 0);
        m_slider->setValue(0);
        updateTimeLabels();
    }, Qt::QueuedConnection);
    
    qDebug() << "[MusicProgressBar] 重置进度条";
}

void MusicProgressBar::mousePressEvent(QMouseEvent *event)
{
    qDebug() << "[MusicProgressBar] mousePressEvent - 事件位置:" << event->pos();
    qDebug() << "[MusicProgressBar] 滑块区域:" << m_slider->geometry();
    
    // 检查点击是否在滑块区域内
    if (m_slider->geometry().contains(event->pos())) {
        qDebug() << "[MusicProgressBar] 点击在滑块区域内，交给PreciseSlider处理";
        // 直接调用PreciseSlider的鼠标事件处理
        m_slider->mousePressEvent(event);
        event->accept();
    } else {
        qDebug() << "[MusicProgressBar] 点击不在滑块区域内，交给基类处理";
        QWidget::mousePressEvent(event);
    }
}

void MusicProgressBar::mouseMoveEvent(QMouseEvent *event)
{
    // 只处理悬停提示
    if (!m_userInteracting) {
        updateTooltip(event->pos());
    }
    
    // 如果鼠标在滑块区域内，传递给PreciseSlider
    if (m_slider->geometry().contains(event->pos())) {
        m_slider->mouseMoveEvent(event);
    }
    
    QWidget::mouseMoveEvent(event);
}

void MusicProgressBar::mouseReleaseEvent(QMouseEvent *event)
{
    qDebug() << "[MusicProgressBar] mouseReleaseEvent - 事件位置:" << event->pos();
    
    // 检查释放是否在滑块区域内
    if (m_slider->geometry().contains(event->pos())) {
        qDebug() << "[MusicProgressBar] 释放在滑块区域内，交给PreciseSlider处理";
        // 直接调用PreciseSlider的鼠标事件处理
        m_slider->mouseReleaseEvent(event);
        event->accept();
    } else {
        qDebug() << "[MusicProgressBar] 释放不在滑块区域内，交给基类处理";
        QWidget::mouseReleaseEvent(event);
    }
}

void MusicProgressBar::enterEvent(QEnterEvent *event)
{
    Q_UNUSED(event)
    // 鼠标进入时的处理
    QWidget::enterEvent(event);
}

void MusicProgressBar::leaveEvent(QEvent *event)
{
    // 隐藏工具提示
    QToolTip::hideText();
    QWidget::leaveEvent(event);
}

void MusicProgressBar::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    // 重新计算布局
}

void MusicProgressBar::onSliderPressed()
{
    qDebug() << "[MusicProgressBar] ===== onSliderPressed 开始 =====";
    
    // 获取当前鼠标位置（相对于MusicProgressBar）
    QPoint globalPos = QCursor::pos();
    QPoint localPos = mapFromGlobal(globalPos);
    
    qDebug() << "[MusicProgressBar] 全局鼠标位置:" << globalPos;
    qDebug() << "[MusicProgressBar] 组件本地位置:" << localPos;
    
    // 检查是否在滑块区域内
    QRect sliderRect = m_slider->geometry();
    if (sliderRect.contains(localPos)) {
        qDebug() << "[MusicProgressBar] ✓ 点击在滑块区域内，开始处理";
        
        // 方案二：混合方案 - 点击时使用精确计算
        qint64 targetPosition = positionFromMouseX(localPos.x());
        qDebug() << "[MusicProgressBar] 精确计算的目标位置:" << targetPosition << "ms";
        
        // 立即跳转
        qDebug() << "[MusicProgressBar] 准备发送seekRequested信号，位置:" << targetPosition << "ms";
        emit seekRequested(targetPosition);
        
        // 手动设置滑块值，提供即时视觉反馈
        int targetSliderValue = sliderValueFromPosition(targetPosition);
        m_slider->blockSignals(true);
        m_slider->setValue(targetSliderValue);
        m_slider->blockSignals(false);
        
        qDebug() << "[MusicProgressBar] ✓ 点击跳转完成:" << targetPosition << "ms, 滑块值:" << targetSliderValue;
    } else {
        qDebug() << "[MusicProgressBar] ✗ 点击不在滑块区域内";
    }
    
    // 发送滑块按下信号
    emit sliderPressed();
    qDebug() << "[MusicProgressBar] ===== onSliderPressed 结束 =====";
}

void MusicProgressBar::onSliderReleased()
{
    qDebug() << "[MusicProgressBar] ===== onSliderReleased 开始 =====";
    qDebug() << "[MusicProgressBar] 用户交互状态:" << m_userInteracting;
    
    // 方案二：混合方案 - 只有在真正拖拽过的情况下才发送seekRequested信号
    if (m_userInteracting) {
        qint64 seekPosition = positionFromSliderValue(m_slider->value());
        qDebug() << "[MusicProgressBar] 检测到拖拽，跳转到:" << seekPosition << "ms";
        emit seekRequested(seekPosition);
    } else {
        qDebug() << "[MusicProgressBar] 未检测到拖拽，跳过跳转";
    }
    
    // 重置交互状态
    m_userInteracting = false;
    emit sliderReleased();
    
    qDebug() << "[MusicProgressBar] ===== onSliderReleased 结束 =====";
}

void MusicProgressBar::onSliderMoved(int value)
{
    qDebug() << "[MusicProgressBar] ===== onSliderMoved 开始 =====";
    
    // 方案二：混合方案 - 当滑块移动时，说明用户正在拖拽
    if (!m_userInteracting) {
        m_userInteracting = true;
        qDebug() << "[MusicProgressBar] 检测到拖拽开始，设置交互状态";
    }
    
    // 拖拽时只更新显示，不发送seekRequested信号
    qint64 newPosition = positionFromSliderValue(value);
    m_position = newPosition;
    updateTimeLabels();
    emit positionChanged(newPosition);
    
    qDebug() << "[MusicProgressBar] 拖拽中，位置:" << newPosition << "ms";
    qDebug() << "[MusicProgressBar] ===== onSliderMoved 结束 =====";
}

void MusicProgressBar::onSliderValueChanged(int value)
{
    qDebug() << "[MusicProgressBar] ===== onSliderValueChanged 开始 =====";
    qDebug() << "[MusicProgressBar] 滑块值变化:" << value << "，用户交互状态:" << m_userInteracting;
    
    qint64 newPosition = positionFromSliderValue(value);
    qDebug() << "[MusicProgressBar] 计算得到新位置:" << newPosition << "ms";
    
    // 方案二：混合方案 - 根据交互状态决定处理方式
    if (!m_userInteracting) {
        // 不是拖拽，可能是点击跳转（已经在onSliderPressed中处理）
        qDebug() << "[MusicProgressBar] 非拖拽状态，跳过处理（点击已在onSliderPressed中处理）";
    } else {
        // 正在拖拽，只更新显示
        qDebug() << "[MusicProgressBar] 正在拖拽中，只更新显示";
        if (newPosition != m_position) {
            m_position = newPosition;
            updateTimeLabels();
        }
    }
    
    qDebug() << "[MusicProgressBar] ===== onSliderValueChanged 结束 =====";
}

void MusicProgressBar::updatePosition(qint64 position)
{
    QMutexLocker locker(&m_mutex);
    
    // 修复：只有在用户没有交互时才更新位置
    // 这样可以避免拖拽时被外部位置更新覆盖
    if (!m_userInteracting) {
        m_position = position;
        locker.unlock();
        updateTimeLabels();
        updateSliderValue();
        qDebug() << "[MusicProgressBar] 位置更新完成:" << position << "ms";
    } else {
        qDebug() << "[MusicProgressBar] 用户正在交互，跳过位置更新:" << position << "ms";
    }
}

void MusicProgressBar::updateDuration(qint64 duration)
{
    QMutexLocker locker(&m_mutex);
    setDurationInternal(duration);
    
    // 在主线程中更新UI
    QMetaObject::invokeMethod(this, [this, duration]() {
        qDebug() << "[MusicProgressBar] updateDuration - 设置时长:" << duration << "ms";
        
        // 更新滑块范围
        int sliderMax = static_cast<int>(duration / 1000); // 转换为秒
        if (sliderMax <= 0) {
            sliderMax = 1; // 确保至少为1秒
        }
        
        m_slider->setRange(0, sliderMax);
        qDebug() << "[MusicProgressBar] updateDuration - 滑块范围设置为: 0 -" << sliderMax << "秒";
        
        // 同时设置PreciseSlider的时长
        if (m_slider) {
            m_slider->setDuration(duration);
            qDebug() << "[MusicProgressBar] updateDuration - PreciseSlider时长设置为:" << duration << "ms";
        }
        
        // 更新时间标签
        updateTimeLabels();
        
        qDebug() << "[MusicProgressBar] updateDuration - 完成";
    }, Qt::QueuedConnection);
}

void MusicProgressBar::updateTimeLabels()
{
    if (!m_currentTimeLabel || !m_totalTimeLabel) {
        return;
    }
    
    QMutexLocker locker(&m_mutex);
    
    QString currentTime = formatTime(m_position);
    QString totalTime = formatTime(m_duration);
    
    locker.unlock();
    
    // 在主线程中更新UI
    QMetaObject::invokeMethod(m_currentTimeLabel, [this, currentTime]() {
        m_currentTimeLabel->setText(currentTime);
    }, Qt::QueuedConnection);
    
    QMetaObject::invokeMethod(m_totalTimeLabel, [this, totalTime]() {
        m_totalTimeLabel->setText(totalTime);
    }, Qt::QueuedConnection);
}

void MusicProgressBar::updateSliderValue()
{
    if (!m_slider) {
        return;
    }
    
    QMutexLocker locker(&m_mutex);
    
    // 如果正在拖拽，不更新滑块
    if (m_userInteracting) {
        qDebug() << "[MusicProgressBar] 跳过滑块更新，正在拖拽中";
        return;
    }
    
    int sliderValue = sliderValueFromPosition(m_position);
    
    locker.unlock();
    
    // 在主线程中更新UI
    QMetaObject::invokeMethod(m_slider, [this, sliderValue]() {
        m_slider->blockSignals(true);
        m_slider->setValue(sliderValue);
        m_slider->blockSignals(false);
    }, Qt::QueuedConnection);
    
    qDebug() << "[MusicProgressBar] 滑块更新完成，值:" << sliderValue;
}

void MusicProgressBar::updateTooltip(const QPoint& position)
{
    if (!m_enabled || m_duration <= 0) {
        return;
    }
    
    QMutexLocker locker(&m_mutex);
    
    // 计算鼠标位置对应的时间 - 修复：使用相对于组件的坐标
    qint64 hoverPosition = positionFromMouseX(position.x());
    QString timeText = formatTime(hoverPosition);
    
    locker.unlock();
    
    // 显示工具提示 - 修复：使用正确的全局坐标
    QPoint globalPos = mapToGlobal(position);
    QToolTip::showText(globalPos, timeText, this);
    
    qDebug() << "[MusicProgressBar] 悬停提示: 位置=" << position.x() << ", 时间=" << timeText;
}

QString MusicProgressBar::formatTime(qint64 milliseconds) const
{
    int seconds = static_cast<int>(milliseconds / 1000);
    int minutes = seconds / 60;
    int hours = minutes / 60;
    
    seconds %= 60;
    minutes %= 60;
    
    if (hours > 0) {
        return QString("%1:%2:%3")
            .arg(hours, 2, 10, QChar('0'))
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'));
    } else {
        return QString("%1:%2")
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'));
    }
}

qint64 MusicProgressBar::positionFromSliderValue(int value) const
{
    if (m_duration <= 0) {
        return 0;
    }
    
    // 滑块值是以秒为单位，转换为毫秒
    return static_cast<qint64>(value) * 1000;
}

int MusicProgressBar::sliderValueFromPosition(qint64 position) const
{
    if (m_duration <= 0) {
        return 0;
    }
    
    // 位置是毫秒，转换为秒
    int value = static_cast<int>(position / 1000);
    
    // 确保值在有效范围内
    int maxValue = static_cast<int>(m_duration / 1000);
    return std::clamp(value, 0, maxValue);
}

qint64 MusicProgressBar::positionFromMouseX(int x) const
{
    if (!m_slider || m_duration <= 0) {
        return 0;
    }
    
    // 方案二：混合方案 - 使用简化的精确计算
    // 获取滑块的有效区域（相对于滑块本身）
    QRect sliderRect = m_slider->geometry();
    int sliderWidth = sliderRect.width();
    
    if (sliderWidth <= 0) {
        return 0;
    }
    
    // 计算相对于滑块的位置
    int relativeX = x - m_slider->x();
    relativeX = std::clamp(relativeX, 0, sliderWidth);
    
    // 计算相对位置比例
    double ratio = static_cast<double>(relativeX) / sliderWidth;
    ratio = std::clamp(ratio, 0.0, 1.0);
    
    // 转换为时间位置（毫秒）
    qint64 position = static_cast<qint64>(ratio * m_duration);
    
    // 确保位置在有效范围内
    position = std::clamp(position, static_cast<qint64>(0), m_duration);
    
    qDebug() << "[MusicProgressBar] 精确位置计算: x=" << x 
             << ", sliderX=" << m_slider->x()
             << ", relativeX=" << relativeX 
             << ", sliderWidth=" << sliderWidth
             << ", ratio=" << ratio 
             << ", duration=" << m_duration
             << ", position=" << position << "ms";
    
    return position;
}

void MusicProgressBar::setPositionInternal(qint64 position)
{
    m_position = std::clamp(position, m_minimum, m_maximum);
    
    // 在主线程中更新UI
    QMetaObject::invokeMethod(this, [this]() {
        updateTimeLabels();
        updateSliderValue();
    }, Qt::QueuedConnection);
}

void MusicProgressBar::setDurationInternal(qint64 duration)
{
    m_duration = std::max(duration, static_cast<qint64>(0));
    m_maximum = m_duration;
    
    // 更新滑块范围
    int maxSliderValue = static_cast<int>(m_duration / 1000);
    
    // 在主线程中更新UI
    QMetaObject::invokeMethod(this, [this, maxSliderValue]() {
        m_slider->setMaximum(maxSliderValue);
        updateTimeLabels();
    }, Qt::QueuedConnection);
    
    qDebug() << "[MusicProgressBar] 设置时长:" << duration << "ms";
}

void MusicProgressBar::onPreciseSeekRequested(qint64 position)
{
    qDebug() << "[MusicProgressBar] ===== onPreciseSeekRequested 开始 =====";
    qDebug() << "[MusicProgressBar] 接收到preciseSeekRequested信号，位置:" << position << "ms";
    
    // 转发为seekRequested信号
    emit seekRequested(position);
    
    qDebug() << "[MusicProgressBar] 已转发seekRequested信号，位置:" << position << "ms";
    qDebug() << "[MusicProgressBar] ===== onPreciseSeekRequested 结束 =====";
}

void MusicProgressBar::onPrecisePositionChanged(qint64 position)
{
    qDebug() << "[MusicProgressBar] ===== onPrecisePositionChanged 开始 =====";
    qDebug() << "[MusicProgressBar] 接收到precisePositionChanged信号，位置:" << position << "ms";
    
    // 转发为positionChanged信号
    emit positionChanged(position);
    
    qDebug() << "[MusicProgressBar] 已转发positionChanged信号，位置:" << position << "ms";
    qDebug() << "[MusicProgressBar] ===== onPrecisePositionChanged 结束 =====";
}