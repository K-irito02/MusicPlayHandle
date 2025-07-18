#ifndef MUSICPROGRESSBAR_H
#define MUSICPROGRESSBAR_H

#include <QWidget>
#include <QSlider>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTimer>
#include <QMouseEvent>
#include <QStyleOptionSlider>
#include <QStyle>
#include <QToolTip>
#include <QMutex>
#include <QMutexLocker>

/**
 * @brief 精确滑块组件
 * 
 * 继承自QSlider，重写鼠标事件处理，实现精确的点击和拖拽功能
 */
class PreciseSlider : public QSlider
{
    Q_OBJECT
    
public:
    explicit PreciseSlider(Qt::Orientation orientation, QWidget *parent = nullptr);
    
    // 设置时长（供外部调用）
    void setDuration(qint64 duration);
    
    // 重写鼠标事件处理（改为public以便外部调用）
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    
protected:
    // 重写绘制事件以支持拖拽预览
    void paintEvent(QPaintEvent *event) override;
    
    // 事件过滤器
    bool eventFilter(QObject *obj, QEvent *event) override;
    
signals:
    // 精确位置信号
    void preciseSeekRequested(qint64 position);
    void precisePositionChanged(qint64 position);
    
private:
    bool m_isDragging;              // 是否正在拖拽
    qint64 m_duration;              // 总时长（毫秒）
    qint64 m_dragPreviewPosition;   // 拖拽预览位置（用于绘制）
    // 精确位置计算
    qint64 positionFromMouseX(int x) const;
    void updatePositionFromMouse(const QPoint& pos);
};

/**
 * @brief 自定义音乐进度条组件
 * 
 * 该组件包含：
 * - 精确进度条滑块
 * - 当前播放时间显示
 * - 音乐总时长显示
 * - 鼠标悬停时间提示
 * - 线程安全的位置更新
 */
class MusicProgressBar : public QWidget
{
    Q_OBJECT
    
public:
    explicit MusicProgressBar(QWidget *parent = nullptr);
    ~MusicProgressBar();
    
    // 设置播放位置和总时长
    void setPosition(qint64 position);
    void setDuration(qint64 duration);
    void setRange(qint64 minimum, qint64 maximum);
    
    // 获取当前值
    qint64 position() const;
    qint64 duration() const;
    qint64 minimum() const;
    qint64 maximum() const;
    
    // 设置是否可拖拽
    void setEnabled(bool enabled);
    bool isEnabled() const;
    
    // 设置样式
    void setProgressBarStyle(const QString& style);
    void setTimeLabelsStyle(const QString& style);
    
    // 重置进度条
    void reset();
    
protected:
    // 事件处理
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    
private slots:
    // 精确滑块信号处理
    void onPreciseSeekRequested(qint64 position);
    void onPrecisePositionChanged(qint64 position);
    
    // 传统滑块信号处理（保留兼容性）
    void onSliderPressed();
    void onSliderReleased();
    void onSliderMoved(int value);
    void onSliderValueChanged(int value);
    
public slots:
    // 外部调用的槽函数
    void updatePosition(qint64 position);
    void updateDuration(qint64 duration);
    
signals:
    // 信号
    void positionChanged(qint64 position);  // 位置改变（拖拽时）
    void seekRequested(qint64 position);    // 请求跳转（拖拽结束时）
    void sliderPressed();                   // 滑块被按下
    void sliderReleased();                  // 滑块被释放
    
private:
    // UI组件
    PreciseSlider* m_slider;        // 精确进度条滑块
    QLabel* m_currentTimeLabel;     // 当前时间标签
    QLabel* m_totalTimeLabel;       // 总时长标签
    QVBoxLayout* m_mainLayout;      // 主布局（垂直布局）
    QHBoxLayout* m_timeLayout;      // 时间标签布局
    
    // 数据成员
    qint64 m_position;              // 当前位置（毫秒）
    qint64 m_duration;              // 总时长（毫秒）
    qint64 m_minimum;               // 最小值
    qint64 m_maximum;               // 最大值
    
    // 状态标志
    bool m_userInteracting;         // 用户是否正在交互（点击或拖拽）
    bool m_enabled;                 // 是否启用
    
    // 线程安全
    mutable QMutex m_mutex;         // 互斥锁
    
    // 防抖定时器
    QTimer* m_seekDebounceTimer;    // 跳转防抖定时器
    qint64 m_pendingSeekPosition;   // 待跳转位置
    
    // 样式
    QString m_progressBarStyle;     // 进度条样式
    QString m_timeLabelsStyle;      // 时间标签样式
    
    // 私有方法
    void setupUI();                 // 设置UI
    void setupConnections();        // 设置信号连接
    void updateTimeLabels();        // 更新时间标签
    void updateSliderValue();       // 更新滑块值
    void updateTooltip(const QPoint& position); // 更新工具提示
    
    // 时间格式化
    QString formatTime(qint64 milliseconds) const;
    
    // 位置计算
    qint64 positionFromSliderValue(int value) const;
    int sliderValueFromPosition(qint64 position) const;
    qint64 positionFromMouseX(int x) const;
    
    // 线程安全的设置方法
    void setPositionInternal(qint64 position);
    void setDurationInternal(qint64 duration);
};

#endif // MUSICPROGRESSBAR_H