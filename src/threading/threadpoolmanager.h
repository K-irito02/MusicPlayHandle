#ifndef THREADPOOLMANAGER_H
#define THREADPOOLMANAGER_H

#include <QObject>
#include <QThreadPool>
#include <QRunnable>
#include <QMutex>
#include <QTimer>
#include <QElapsedTimer>
#include <QAtomicInt>
#include <QAtomicPointer>
#include <QQueue>
#include <QMap>
#include <QVector>
#include <functional>
#include <memory>

/**
 * @brief 可取消的任务基类
 */
class CancellableTask : public QRunnable {
public:
    CancellableTask();
    virtual ~CancellableTask();
    
    // 取消任务
    void cancel();
    bool isCancelled() const;
    
    // 任务优先级
    enum Priority {
        LowPriority = 0,
        NormalPriority = 1,
        HighPriority = 2,
        CriticalPriority = 3
    };
    
    void setPriority(Priority priority);
    Priority getPriority() const;
    
    // 任务ID
    void setTaskId(const QString& id);
    QString getTaskId() const;
    
    // 任务类型
    void setTaskType(const QString& type);
    QString getTaskType() const;
    
    // 执行时间统计
    qint64 getExecutionTime() const;
    
protected:
    // 子类需要实现的运行方法
    void run() override final;
    virtual void execute() = 0;
    
    // 检查是否应该继续执行
    bool shouldContinue() const;

private:
    QAtomicInt m_cancelled;
    Priority m_priority;
    QString m_taskId;
    QString m_taskType;
    QElapsedTimer m_executionTimer;
    qint64 m_executionTime;
};

/**
 * @brief 音频处理任务
 */
class AudioProcessingTask : public CancellableTask {
public:
    using AudioProcessCallback = std::function<void(const QByteArray&)>;
    
    AudioProcessingTask(const QByteArray& audioData, AudioProcessCallback callback);
    
protected:
    void execute() override;

private:
    QByteArray m_audioData;
    AudioProcessCallback m_callback;
};

/**
 * @brief 解码任务
 */
class DecodeTask : public CancellableTask {
public:
    using DecodeCallback = std::function<void(const QString&, bool, const QString&)>;
    
    DecodeTask(const QString& filePath, DecodeCallback callback);
    
protected:
    void execute() override;

private:
    QString m_filePath;
    DecodeCallback m_callback;
};

/**
 * @brief 文件预加载任务
 */
class PreloadTask : public CancellableTask {
public:
    using PreloadCallback = std::function<void(const QString&, const QByteArray&, bool)>;
    
    PreloadTask(const QString& filePath, PreloadCallback callback);
    
protected:
    void execute() override;

private:
    QString m_filePath;
    PreloadCallback m_callback;
};

/**
 * @brief 智能线程池
 * 支持优先级、任务取消、性能监控
 */
class SmartThreadPool : public QObject {
    Q_OBJECT
    
public:
    explicit SmartThreadPool(QObject* parent = nullptr);
    ~SmartThreadPool();
    
    // 线程池配置
    void setMaxThreadCount(int maxThreadCount);
    void setExpiryTimeout(int expiryTimeout);
    int maxThreadCount() const;
    int activeThreadCount() const;
    
    // 任务提交
    void submitTask(CancellableTask* task);
    void submitTaskWithPriority(CancellableTask* task, CancellableTask::Priority priority);
    
    // 批量任务提交
    void submitTasks(const QVector<CancellableTask*>& tasks);
    
    // 任务取消
    void cancelTask(const QString& taskId);
    void cancelTasksByType(const QString& taskType);
    void cancelAllTasks();
    
    // 等待任务完成
    bool waitForDone(int msecs = -1);
    void clear();
    
    // 性能监控
    struct PoolStatistics {
        int totalTasksSubmitted = 0;
        int totalTasksCompleted = 0;
        int totalTasksCancelled = 0;
        int currentPendingTasks = 0;
        int activeThreads = 0;
        double avgExecutionTime = 0.0;
        double maxExecutionTime = 0.0;
        qint64 totalExecutionTime = 0;
    };
    
    PoolStatistics getStatistics() const;
    void resetStatistics();
    
    // 自适应线程数
    void enableAdaptiveThreadCount(bool enabled);
    bool isAdaptiveThreadCountEnabled() const;
    void adjustThreadCountBasedOnLoad();

signals:
    void taskCompleted(const QString& taskId, qint64 executionTime);
    void taskCancelled(const QString& taskId);
    void taskFailed(const QString& taskId, const QString& error);
    void poolStatisticsUpdated(const PoolStatistics& stats);
    void threadCountAdjusted(int oldCount, int newCount);

private slots:
    void updateStatistics();
    void onTaskCompleted();

private:
    QThreadPool* m_threadPool;
    
    // 任务管理
    QMutex m_taskMutex;
    QMap<QString, CancellableTask*> m_activeTasks;
    QQueue<CancellableTask*> m_pendingTasks;
    
    // 统计信息
    mutable QMutex m_statsMutex;
    PoolStatistics m_statistics;
    QVector<qint64> m_executionTimes;
    QTimer* m_statsTimer;
    
    // 自适应线程数
    bool m_adaptiveThreadCount;
    QTimer* m_adaptiveTimer;
    QElapsedTimer m_loadMonitorTimer;
    double m_currentLoad;
    
    // 私有方法
    void processTaskQueue();
    void updateTaskStatistics(CancellableTask* task);
    double calculateCurrentLoad() const;
    int calculateOptimalThreadCount() const;
    void cleanupCompletedTasks();
};

/**
 * @brief 线程池管理器
 * 管理多个专用线程池
 */
class ThreadPoolManager : public QObject {
    Q_OBJECT
    
public:
    enum PoolType {
        AudioProcessingPool,    // 音频处理专用
        DecodingPool,          // 解码专用
        FileIOPool,            // 文件I/O专用
        GeneralPool            // 通用任务
    };
    
    static ThreadPoolManager& instance();
    
    // 线程池访问
    SmartThreadPool* getPool(PoolType type);
    
    // 任务提交快捷方法
    void submitAudioTask(CancellableTask* task);
    void submitDecodeTask(CancellableTask* task);
    void submitFileIOTask(CancellableTask* task);
    void submitGeneralTask(CancellableTask* task);
    
    // 性能优化
    void optimizeForPerformance();
    void optimizeForPowerSaving();
    void optimizeForBalance();
    
    // 全局统计
    struct GlobalStatistics {
        QMap<PoolType, SmartThreadPool::PoolStatistics> poolStats;
        int totalActiveThreads = 0;
        int totalPendingTasks = 0;
        double systemLoad = 0.0;
    };
    
    GlobalStatistics getGlobalStatistics() const;
    
    // 配置管理
    void setPoolConfiguration(PoolType type, int maxThreads, int expiryTimeout);
    void enableAdaptiveManagement(bool enabled);
    bool isAdaptiveManagementEnabled() const;
    
    // 清理和关闭
    void cleanupAllPools();
    void shutdownAllPools();

signals:
    void globalStatisticsUpdated(const GlobalStatistics& stats);
    void poolPerformanceWarning(PoolType poolType, const QString& warning);

private slots:
    void onPoolStatisticsUpdated(const SmartThreadPool::PoolStatistics& stats);
    void updateGlobalStatistics();
    void performAdaptiveManagement();

private:
    ThreadPoolManager(QObject* parent = nullptr);
    ~ThreadPoolManager();
    
    static ThreadPoolManager* s_instance;
    static QMutex s_instanceMutex;
    
    // 线程池
    QMap<PoolType, std::unique_ptr<SmartThreadPool>> m_pools;
    
    // 全局统计
    mutable QMutex m_globalStatsMutex;
    GlobalStatistics m_globalStats;
    QTimer* m_globalStatsTimer;
    
    // 自适应管理
    bool m_adaptiveManagement;
    QTimer* m_adaptiveTimer;
    QElapsedTimer m_performanceTimer;
    
    // 配置
    struct PoolConfig {
        int maxThreads = 4;
        int expiryTimeout = 30000; // 30秒
    };
    QMap<PoolType, PoolConfig> m_poolConfigs;
    
    // 私有方法
    void initializePools();
    void configurePool(PoolType type, SmartThreadPool* pool);
    QString poolTypeToString(PoolType type) const;
    void analyzeSystemPerformance();
    void adjustPoolsBasedOnLoad();
};

// 便利宏定义
#define SUBMIT_AUDIO_TASK(task) ThreadPoolManager::instance().submitAudioTask(task)
#define SUBMIT_DECODE_TASK(task) ThreadPoolManager::instance().submitDecodeTask(task)
#define SUBMIT_FILE_IO_TASK(task) ThreadPoolManager::instance().submitFileIOTask(task)
#define SUBMIT_GENERAL_TASK(task) ThreadPoolManager::instance().submitGeneralTask(task)

#endif // THREADPOOLMANAGER_H