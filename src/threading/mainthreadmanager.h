#ifndef MAINTHREADMANAGER_H
#define MAINTHREADMANAGER_H

#include <QObject>
#include <QMutex>
#include <QQueue>
#include <QTimer>
#include <QThread>
#include <QCoreApplication>
#include <QMetaObject>
#include <QList>
#include <QListWidget>
#include <QDateTime>
#include <QProgressBar>
#include <QLabel>
#include <QStatusBar>
#include <QPushButton>
#include <QTextEdit>
#include <QTabWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QComboBox>
#include <QGroupBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QToolBar>
#include <QPixmap>
#include <QSlider>
#include <QTableWidget>
#include <QMenu>
#include <QAction>
#include <QWidget>
#include <functional>

// 事件类型
enum class UIEventType {
    PlaybackUpdate,
    DatabaseUpdate,
    FileUpdate,
    AudioUpdate,
    TagUpdate,
    PlaylistUpdate,
    ErrorUpdate,
    StatusUpdate,
    ProgressUpdate,
    Generic
};

// UI事件结构
struct UIEvent {
    UIEventType type;
    QString description;
    QVariant data;
    qint64 timestamp;
    int priority;
    
    UIEvent(UIEventType t = UIEventType::Generic, const QString& desc = QString(), 
            const QVariant& d = QVariant(), int p = 0)
        : type(t), description(desc), data(d), 
          timestamp(QDateTime::currentMSecsSinceEpoch()), priority(p) {}
};

// UI更新任务
struct UIUpdateTask {
    std::function<void()> function;
    QString description;
    int priority;
    qint64 timestamp;
    bool delayed;
    int delayMs;
    
    UIUpdateTask(std::function<void()> func, const QString& desc = QString(), 
                 int p = 0, bool d = false, int delay = 0)
        : function(func), description(desc), priority(p), 
          timestamp(QDateTime::currentMSecsSinceEpoch()), delayed(d), delayMs(delay) {}
};

// 主线程管理器
class MainThreadManager : public QObject {
    Q_OBJECT
    
public:
    // 单例模式
    static MainThreadManager* instance();
    static void cleanup();
    
    // 线程安全的UI更新调度
    void scheduleUIUpdate(std::function<void()> updateFunction, 
                         const QString& description = QString(), int priority = 0);
    void scheduleUIUpdateDelayed(std::function<void()> updateFunction, int delayMs,
                                const QString& description = QString(), int priority = 0);
    
    // 批量UI更新
    void batchUIUpdates(const QList<std::function<void()>>& updates, 
                       const QString& batchDescription = QString());
    
    // 事件处理
    void handleUIEvent(const UIEvent& event);
    void handlePlaybackEvent(const QVariant& data);
    void handleDatabaseEvent(const QVariant& data);
    void handleFileEvent(const QVariant& data);
    void handleAudioEvent(const QVariant& data);
    void handleTagEvent(const QVariant& data);
    void handlePlaylistEvent(const QVariant& data);
    void handleErrorEvent(const QVariant& data);
    
    // 线程检查
    bool isMainThread() const;
    bool isCurrentThreadMainThread() const;
    
    // 优先级管理
    void setUpdatePriority(int priority);
    int getUpdatePriority() const;
    
    // 定时器管理
    void setUpdateInterval(int intervalMs);
    int getUpdateInterval() const;
    void pauseUpdates();
    void resumeUpdates();
    bool isUpdatesPaused() const;
    
    // 统计信息
    int getPendingUpdateCount() const;
    int getProcessedUpdateCount() const;
    int getFailedUpdateCount() const;
    qint64 getAverageProcessingTime() const;
    
    // 批处理设置
    void setBatchSize(int size);
    int getBatchSize() const;
    void setBatchTimeout(int timeoutMs);
    int getBatchTimeout() const;
    
    // 错误处理
    void setErrorHandler(std::function<void(const QString&)> handler);
    
    // 调试和监控
    void enableDebugMode(bool enabled);
    bool isDebugModeEnabled() const;
    void dumpPendingUpdates() const;
    
signals:
    // 更新处理信号
    void uiUpdateScheduled(const QString& description);
    void uiUpdateProcessed(const QString& description);
    void uiUpdateFailed(const QString& description, const QString& error);
    
    // 批处理信号
    void batchUpdateStarted(int count);
    void batchUpdateProgress(int current, int total);
    void batchUpdateFinished(int processed, int failed);
    
    // 事件处理信号
    void eventProcessed(UIEventType type, const QString& description);
    void eventFailed(UIEventType type, const QString& error);
    
    // 状态信号
    void updatesPaused();
    void updatesResumed();
    void queueCleared();
    
    // 统计信号
    void statisticsUpdated(int pending, int processed, int failed);
    
private slots:
    void processUIUpdateQueue();
    void processDelayedUpdates();
    void processBatchUpdates();
    void updateStatistics();
    void handleTimeout();
    
private:
    explicit MainThreadManager(QObject* parent = nullptr);
    ~MainThreadManager();
    
    // 禁用拷贝构造和赋值
    MainThreadManager(const MainThreadManager&) = delete;
    MainThreadManager& operator=(const MainThreadManager&) = delete;
    
    // 单例实例
    static MainThreadManager* m_instance;
    static QMutex m_instanceMutex;
    
    // 更新队列
    QQueue<UIUpdateTask> m_updateQueue;
    QQueue<UIUpdateTask> m_delayedQueue;
    QQueue<UIUpdateTask> m_batchQueue;
    
    // 线程安全
    QMutex m_updateMutex;
    QMutex m_delayedMutex;
    QMutex m_batchMutex;
    QMutex m_statisticsMutex;
    
    // 定时器
    QTimer* m_updateTimer;
    QTimer* m_delayedTimer;
    QTimer* m_batchTimer;
    QTimer* m_statisticsTimer;
    
    // 设置
    int m_updateInterval;
    int m_defaultPriority;
    int m_batchSize;
    int m_batchTimeout;
    bool m_updatesPaused;
    bool m_debugMode;
    
    // 统计信息
    int m_pendingUpdateCount;
    int m_processedUpdateCount;
    int m_failedUpdateCount;
    qint64 m_totalProcessingTime;
    qint64 m_maxProcessingTime;
    qint64 m_minProcessingTime;
    
    // 错误处理
    std::function<void(const QString&)> m_errorHandler;
    
    // 内部方法
    void initializeTimers();
    void cleanupTimers();
    void configureUpdateTimer();
    void configureDelayedTimer();
    void configureBatchTimer();
    void configureStatisticsTimer();
    
    // 队列处理
    void executeUIUpdateTask(const UIUpdateTask& task);
    void executeUIUpdateBatch(const QList<UIUpdateTask>& tasks);
    void clearUpdateQueue();
    void clearDelayedQueue();
    void clearBatchQueue();
    
    // 优先级处理
    void sortUpdateQueue();
    void insertByPriority(UIUpdateTask& task);
    
    // 事件处理实现
    void processUIEvent(const UIEvent& event);
    void handleEventInternal(UIEventType type, const QVariant& data);
    
    // 错误处理
    void handleUpdateError(const QString& error, const QString& taskDescription);
    void logError(const QString& error);
    void logInfo(const QString& message);
    void logDebug(const QString& message);
    
    // 统计计算
    void updateProcessingStatistics(qint64 processingTime);
    void resetStatistics();
    
    // 线程检查
    bool validateMainThread() const;
    void ensureMainThread() const;
    
    // 调试工具
    QString formatUpdateTask(const UIUpdateTask& task) const;
    QString formatUIEvent(const UIEvent& event) const;
    void dumpQueueState() const;
    
    // 内部常量
    static const int DEFAULT_UPDATE_INTERVAL = 16; // 60 FPS
    static const int DEFAULT_BATCH_SIZE = 10;
    static const int DEFAULT_BATCH_TIMEOUT = 100;
    static const int MAX_QUEUE_SIZE = 1000;
    static const int STATISTICS_UPDATE_INTERVAL = 1000;
};

// 线程安全的UI更新工具类
class ThreadSafeUIUpdater {
public:
    // 基本UI更新
    template<typename Widget>
    static void updateWidget(Widget* widget, std::function<void(Widget*)> updateFunc) {
        if (!widget) return;
        
        MainThreadManager::instance()->scheduleUIUpdate([widget, updateFunc]() {
            if (widget) {
                updateFunc(widget);
            }
        }, QString("Update %1").arg(widget->objectName()));
    }
    
    // 进度条更新
    static void updateProgressBar(QProgressBar* progressBar, int value);
    
    // 标签更新
    static void updateLabel(QLabel* label, const QString& text);
    
    // 列表更新
    static void updateListWidget(QListWidget* listWidget, const QStringList& items);
    
    // 状态栏更新
    static void updateStatusBar(QStatusBar* statusBar, const QString& message, int timeout = 0);
    
    // 按钮状态更新
    static void updateButton(QPushButton* button, const QString& text, bool enabled = true);
    
    // 滑块更新
    static void updateSlider(QSlider* slider, int value);
    
    // 文本框更新
    static void updateTextEdit(QTextEdit* textEdit, const QString& text);
    
    // 表格更新
    static void updateTableWidget(QTableWidget* table, int row, int column, const QString& text);
    
    // 树形控件更新
    static void updateTreeWidget(QTreeWidget* tree, QTreeWidgetItem* item, int column, const QString& text);
    
    // 组合框更新
    static void updateComboBox(QComboBox* comboBox, const QStringList& items);
    
    // 分组框更新
    static void updateGroupBox(QGroupBox* groupBox, const QString& title, bool enabled = true);
    
    // 复选框更新
    static void updateCheckBox(QCheckBox* checkBox, bool checked);
    
    // 单选按钮更新
    static void updateRadioButton(QRadioButton* radioButton, bool checked);
    
    // 图片更新
    static void updateImageLabel(QLabel* label, const QPixmap& pixmap);
    
    // 工具栏更新
    static void updateToolBar(QToolBar* toolBar, bool visible);
    
    // 菜单更新
    static void updateMenu(QMenu* menu, bool enabled);
    
    // 动作更新
    static void updateAction(QAction* action, bool enabled, const QString& text = QString());
    
    // 窗口更新
    static void updateWindow(QWidget* window, const QString& title);
    
    // 焦点更新
    static void setWidgetFocus(QWidget* widget);
    
    // 样式更新
    static void updateWidgetStyle(QWidget* widget, const QString& styleSheet);
    
    // 批量更新
    static void batchUpdateWidgets(const QList<std::function<void()>>& updates);
};

#endif // MAINTHREADMANAGER_H 
