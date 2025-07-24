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
#include <QTimer> // Added for QTimer::singleShot

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
    
    // 构造函数完成
}

void PreciseSlider::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        qDebug() << "PreciseSlider: 鼠标按下，准备处理点击或拖拽";
        
        // 将坐标转换为相对于PreciseSlider的坐标
        QPoint relativePos = mapFromParent(event->pos());
        
        // 立即计算精确位置并发送信号（此时m_isDragging为false，所以是点击）
        updatePositionFromMouse(relativePos);
        
        // 不立即设置拖拽状态，等待mouseMoveEvent
        event->accept();
    } else {
        QSlider::mousePressEvent(event);
    }
}

void PreciseSlider::mouseMoveEvent(QMouseEvent *event)
{
    // 检查是否按下左键（拖拽状态）
    if (event->buttons() & Qt::LeftButton) {
        if (!m_isDragging) {
            // 第一次移动，设置拖拽状态
            m_isDragging = true;
            qDebug() << "PreciseSlider: 开始拖拽";
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
        
        qDebug() << "PreciseSlider: 拖拽中，预览位置:" << targetPosition;
        
        // 接受事件，避免传递给基类导致额外的处理
        event->accept();
        return;
    }
    
    QSlider::mouseMoveEvent(event);
}

void PreciseSlider::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (m_isDragging) {
            qDebug() << "PreciseSlider: 拖拽结束";
            
            // 将坐标转换为相对于PreciseSlider的坐标
            QPoint relativePos = mapFromParent(event->pos());
            
            // 拖拽结束时发送最终跳转信号
            qint64 finalPosition = positionFromMouseX(relativePos.x());
            qDebug() << "PreciseSlider: 发送精确跳转请求:" << finalPosition;
            emit preciseSeekRequested(finalPosition);
            
            // 重置拖拽状态和预览位置
            m_isDragging = false;
            m_dragPreviewPosition = -1;
            
            // 触发重绘以清除预览
            update();
        }
        
        event->accept();
    } else {
        QSlider::mouseReleaseEvent(event);
    }
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
        
        // 绘制预览线（垂直线）- 浅红色
        QPen previewLinePen(QColor(255, 120, 120), 2); // 浅红色预览线
        painter.setPen(previewLinePen);
        painter.drawLine(previewX, 0, previewX, sliderRect.height());
        
        // 绘制预览点（小圆点）- 深红色
        QBrush previewPointBrush(QColor(200, 0, 0)); // 深红色圆点
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
    } else {
        // 点击时发送跳转信号
        emit preciseSeekRequested(targetPosition);
    }
}

// 设置时长（供外部调用）
void PreciseSlider::setDuration(qint64 duration)
{
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
                
                // 检查点击是否在PreciseSlider区域内
                QPoint localPos = mapFromParent(mouseEvent->pos());
                if (rect().contains(localPos)) {
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
                
                QPoint localPos = mapFromParent(mouseEvent->pos());
                if (rect().contains(localPos)) {
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
        "    color: #ffffff;"
        "    font-family: 'Consolas', 'Monaco', monospace;"
        "    font-size: 11px;"
        "    font-weight: bold;"
        "    background: transparent;"
        "    border: none;"
        "    padding: 2px 4px;"
        "}"
    );
    
    // 组件初始化完成
}

MusicProgressBar::~MusicProgressBar()
{
    // 组件销毁
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
    
    // 设置范围完成
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
    
    // 重置进度条完成
}

void MusicProgressBar::mousePressEvent(QMouseEvent *event)
{
    // 检查点击是否在滑块区域内
    if (m_slider->geometry().contains(event->pos())) {
        // 直接调用PreciseSlider的鼠标事件处理
        m_slider->mousePressEvent(event);
        event->accept();
    } else {
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
    // 检查释放是否在滑块区域内
    if (m_slider->geometry().contains(event->pos())) {
        // 直接调用PreciseSlider的鼠标事件处理
        m_slider->mouseReleaseEvent(event);
        event->accept();
    } else {
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
    qDebug() << "MusicProgressBar: 滑块按下，设置用户交互状态";
    
    // **修复6：改进用户交互状态管理**
    // 当用户按下滑块时，立即设置交互状态，防止外部位置更新干扰
    m_userInteracting = true;
    
    // 获取当前鼠标位置（相对于MusicProgressBar）
    QPoint globalPos = QCursor::pos();
    QPoint localPos = mapFromGlobal(globalPos);
    
    // 检查是否在滑块区域内
    QRect sliderRect = m_slider->geometry();
    if (sliderRect.contains(localPos)) {
        qDebug() << "MusicProgressBar: 点击在滑块区域内";
        
        // 注意：不在按下时就发送seekRequested信号
        // 等待鼠标释放时再决定是否需要跳转
        
        // 手动计算目标位置并更新显示
        qint64 targetPosition = positionFromMouseX(localPos.x());
        int targetSliderValue = sliderValueFromPosition(targetPosition);
        
        // 阻止信号，避免触发额外的处理
        m_slider->blockSignals(true);
        m_slider->setValue(targetSliderValue);
        m_slider->blockSignals(false);
        
        // 更新内部位置和显示
        m_position = targetPosition;
        updateTimeLabels();
        
        qDebug() << "MusicProgressBar: 点击位置计算完成:" << targetPosition;
    }
    
    // 发送滑块按下信号
    emit sliderPressed();
}

void MusicProgressBar::onSliderReleased()
{
    qDebug() << "MusicProgressBar: 滑块释放，处理最终跳转";
    
    // **修复7：确保只在释放时执行一次跳转**
    if (m_userInteracting) {
        // 获取最终位置
        qint64 finalPosition = positionFromSliderValue(m_slider->value());
        
        qDebug() << "MusicProgressBar: 拖拽/点击结束，执行跳转到:" << finalPosition;
        
        // 发送跳转请求
        emit seekRequested(finalPosition);
        
        // 延迟重置交互状态，给AudioEngine处理时间
        QTimer::singleShot(100, this, [this]() {
            m_userInteracting = false;
            qDebug() << "MusicProgressBar: 交互状态重置完成";
        });
    }
    
    // 发送滑块释放信号
    emit sliderReleased();
}

void MusicProgressBar::onSliderMoved(int value)
{
    // 方案二：混合方案 - 当滑块移动时，说明用户正在拖拽
    if (!m_userInteracting) {
        m_userInteracting = true;
        qDebug() << "MusicProgressBar: 检测到拖拽，设置用户交互状态";
    }
    
    // 拖拽时只更新显示，不发送seekRequested信号
    qint64 newPosition = positionFromSliderValue(value);
    m_position = newPosition;
    updateTimeLabels();
    emit positionChanged(newPosition);
    
    qDebug() << "MusicProgressBar: 拖拽中，位置更新为:" << newPosition;
}

void MusicProgressBar::onSliderValueChanged(int value)
{
    qint64 newPosition = positionFromSliderValue(value);
    
    // 方案二：混合方案 - 根据交互状态决定处理方式
    if (m_userInteracting) {
        // 正在拖拽，只更新显示
        if (newPosition != m_position) {
            m_position = newPosition;
            updateTimeLabels();
            qDebug() << "MusicProgressBar: 滑块值变化，更新位置为:" << newPosition;
        }
    } else {
        // 不是拖拽，可能是外部更新，记录日志
        qDebug() << "MusicProgressBar: 外部滑块值变化:" << value << "->" << newPosition;
    }
    // 不是拖拽，可能是点击跳转（已经在onSliderPressed中处理）
}

void MusicProgressBar::updatePosition(qint64 position)
{
    QMutexLocker locker(&m_mutex);
    
    qDebug() << "MusicProgressBar: 收到位置更新请求:" << position << "ms，当前位置:" << m_position << "ms，用户交互状态:" << m_userInteracting;
    
    // **修复8：改进用户交互期间的位置更新处理**
    // 只有在用户没有交互时才更新位置
    // 这样可以避免拖拽时被外部位置更新覆盖，防止进度条跳跃
    if (!m_userInteracting) {
        // 检查位置是否有实际变化，避免无意义的更新
        if (qAbs(m_position - position) > 100) { // 100ms的阈值，避免微小变化
            qDebug() << "MusicProgressBar: 位置变化超过阈值，执行更新";
            m_position = position;
            locker.unlock();
            
            // 使用信号阻塞机制，避免更新时触发用户交互信号
            if (m_slider) {
                m_slider->blockSignals(true);
                updateTimeLabels();
                updateSliderValue();
                m_slider->blockSignals(false);
                qDebug() << "MusicProgressBar: 进度条滑块已更新";
            } else {
                updateTimeLabels();
                updateSliderValue();
                qDebug() << "MusicProgressBar: 时间标签已更新（滑块为空）";
            }
            
            qDebug() << "MusicProgressBar: 外部位置更新完成:" << position << "ms";
        } else {
            qDebug() << "MusicProgressBar: 位置变化小于阈值，跳过更新";
        }
    } else {
        // 在用户交互期间，记录但不应用外部位置更新
        qDebug() << "MusicProgressBar: 用户正在交互，跳过外部位置更新:" << position << "ms，当前用户位置:" << m_position << "ms";
    }
}

void MusicProgressBar::updateDuration(qint64 duration)
{
    QMutexLocker locker(&m_mutex);
    setDurationInternal(duration);
    
    // 在主线程中更新UI
    QMetaObject::invokeMethod(this, [this, duration]() {
        // 更新滑块范围
        int sliderMax = static_cast<int>(duration / 1000); // 转换为秒
        if (sliderMax <= 0) {
            sliderMax = 1; // 确保至少为1秒
        }
        
        m_slider->setRange(0, sliderMax);
        
        // 同时设置PreciseSlider的时长
        if (m_slider) {
            m_slider->setDuration(duration);
        }
        
        // 更新时间标签
        updateTimeLabels();
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
        qDebug() << "MusicProgressBar: 用户正在交互，跳过滑块更新";
        return;
    }
    
    int sliderValue = sliderValueFromPosition(m_position);
    
    locker.unlock();
    
    // 在主线程中更新UI，使用更安全的信号阻塞
    QMetaObject::invokeMethod(this, [this, sliderValue]() {
        if (m_slider && !m_userInteracting) {
            // 双重检查，确保用户没有开始交互
            m_slider->blockSignals(true);
            m_slider->setValue(sliderValue);
            m_slider->blockSignals(false);
            qDebug() << "MusicProgressBar: 滑块值更新为:" << sliderValue;
        }
    }, Qt::QueuedConnection);
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
}

void MusicProgressBar::onPreciseSeekRequested(qint64 position)
{
    // 转发为seekRequested信号
    emit seekRequested(position);
}

void MusicProgressBar::onPrecisePositionChanged(qint64 position)
{
    // 转发为positionChanged信号
    emit positionChanged(position);
}