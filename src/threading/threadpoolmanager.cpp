#include "threadpoolmanager.h"
#include "../core/logger.h"
#include <QApplication>
#include <QThread>
#include <QDebug>
#include <QElapsedTimer>
#include <QRandomGenerator>
#include <QCoreApplication>
#include <algorithm>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>

// CancellableTask 实现

CancellableTask::CancellableTask()
    : m_cancelled(0)
    , m_priority(NormalPriority)
    , m_executionTime(0)
{
    setAutoDelete(true); // QRunnable会自动删除
}

CancellableTask::~CancellableTask() = default;

void CancellableTask::cancel()
{
    m_cancelled.storeRelease(1);
}

bool CancellableTask::isCancelled() const
{
    return m_cancelled.loadAcquire() != 0;
}

void CancellableTask::setPriority(Priority priority)
{
    m_priority = priority;
}

CancellableTask::Priority CancellableTask::getPriority() const
{
    return m_priority;
}

void CancellableTask::setTaskId(const QString& id)
{
    m_taskId = id;
}

QString CancellableTask::getTaskId() const
{
    return m_taskId;
}

void CancellableTask::setTaskType(const QString& type)
{
    m_taskType = type;
}

QString CancellableTask::getTaskType() const
{
    return m_taskType;
}

qint64 CancellableTask::getExecutionTime() const
{
    return m_executionTime;
}

void CancellableTask::run()
{
    if (isCancelled()) {
        return;
    }
    
    m_executionTimer.start();
    
    try {
        execute();
    } catch (const std::exception& e) {
        qWarning() << "Task" << m_taskId << "执行异常:" << e.what();
    } catch (...) {
        qWarning() << "Task" << m_taskId << "执行未知异常";
    }
    
    m_executionTime = m_executionTimer.elapsed();
}

bool CancellableTask::shouldContinue() const
{
    return !isCancelled();
}

// AudioProcessingTask 实现

AudioProcessingTask::AudioProcessingTask(const QByteArray& audioData, AudioProcessCallback callback)
    : m_audioData(audioData)
    , m_callback(callback)
{
    setTaskType("AudioProcessing");
}

void AudioProcessingTask::execute()
{
    if (!shouldContinue() || !m_callback) {
        return;
    }
    
    // 模拟音频处理
    QThread::msleep(10); // 模拟处理时间
    
    if (shouldContinue() && m_callback) {
        m_callback(m_audioData);
    }
}

// DecodeTask 实现

DecodeTask::DecodeTask(const QString& filePath, DecodeCallback callback)
    : m_filePath(filePath)
    , m_callback(callback)
{
    setTaskType("Decode");
}

void DecodeTask::execute()
{
    if (!shouldContinue() || !m_callback) {
        return;
    }
    
    // 模拟解码过程
    bool success = false;
    QString error;
    
    try {
        // 检查文件是否存在
        QFileInfo fileInfo(m_filePath);
        if (!fileInfo.exists()) {
            error = "文件不存在: " + m_filePath;
        } else if (!fileInfo.isReadable()) {
            error = "文件不可读: " + m_filePath;
        } else {
            // 模拟解码时间（基于文件大小）
            qint64 fileSize = fileInfo.size();
            int sleepTime = qMin(static_cast<int>(fileSize / (1024 * 1024)), 100); // 最多100ms
            
            for (int i = 0; i < sleepTime && shouldContinue(); i += 10) {
                QThread::msleep(10);
            }
            
            if (shouldContinue()) {
                success = true;
            }
        }
    } catch (const std::exception& e) {
        error = QString("解码异常: %1").arg(e.what());
    }
    
    if (shouldContinue() && m_callback) {
        m_callback(m_filePath, success, error);
    }
}

// PreloadTask 实现

PreloadTask::PreloadTask(const QString& filePath, PreloadCallback callback)
    : m_filePath(filePath)
    , m_callback(callback)
{
    setTaskType("Preload");
}

void PreloadTask::execute()
{
    if (!shouldContinue() || !m_callback) {
        return;
    }
    
    QByteArray data;
    bool success = false;
    
    try {
        QFile file(m_filePath);
        if (file.open(QIODevice::ReadOnly)) {
            qint64 fileSize = file.size();
            const qint64 maxPreloadSize = 10 * 1024 * 1024; // 10MB限制
            
            if (fileSize <= maxPreloadSize) {
                data = file.readAll();
                success = !data.isEmpty();
            } else {
                // 文件太大，只预加载部分
                data = file.read(maxPreloadSize);
                success = !data.isEmpty();
            }
        }
    } catch (const std::exception& e) {
        qWarning() << "PreloadTask: 预加载失败" << m_filePath << e.what();
    }
    
    if (shouldContinue() && m_callback) {
        m_callback(m_filePath, data, success);
    }
}

// SmartThreadPool 实现

SmartThreadPool::SmartThreadPool(QObject* parent)
    : QObject(parent)
    , m_threadPool(new QThreadPool(this))
    , m_adaptiveThreadCount(false)
    , m_adaptiveTimer(new QTimer(this))
    , m_currentLoad(0.0)
    , m_statsTimer(new QTimer(this))
{
    // 设置默认线程数
    int defaultThreadCount = QThread::idealThreadCount();
    m_threadPool->setMaxThreadCount(defaultThreadCount);
    
    // 设置统计定时器
    connect(m_statsTimer, &QTimer::timeout, this, &SmartThreadPool::updateStatistics);
    m_statsTimer->start(1000); // 每秒更新统计
    
    // 设置自适应定时器
    connect(m_adaptiveTimer, &QTimer::timeout, this, &SmartThreadPool::adjustThreadCountBasedOnLoad);
    
    qDebug() << "SmartThreadPool: 初始化完成，线程数:" << defaultThreadCount;
}

SmartThreadPool::~SmartThreadPool()
{
    clear();
    m_threadPool->waitForDone();
    
    qDebug() << "SmartThreadPool: 已销毁";
}

void SmartThreadPool::setMaxThreadCount(int maxThreadCount)
{
    int oldCount = m_threadPool->maxThreadCount();
    m_threadPool->setMaxThreadCount(maxThreadCount);
    
    qDebug() << "SmartThreadPool: 最大线程数从" << oldCount << "调整到" << maxThreadCount;
    emit threadCountAdjusted(oldCount, maxThreadCount);
}

void SmartThreadPool::setExpiryTimeout(int expiryTimeout)
{
    m_threadPool->setExpiryTimeout(expiryTimeout);
    qDebug() << "SmartThreadPool: 线程过期时间设置为" << expiryTimeout << "ms";
}

int SmartThreadPool::maxThreadCount() const
{
    return m_threadPool->maxThreadCount();
}

int SmartThreadPool::activeThreadCount() const
{
    return m_threadPool->activeThreadCount();
}

void SmartThreadPool::submitTask(CancellableTask* task)
{
    if (!task) {
        return;
    }
    
    QMutexLocker locker(&m_taskMutex);
    
    // 设置任务ID（如果没有的话）
    if (task->getTaskId().isEmpty()) {
        task->setTaskId(QString("Task_%1_%2")
                       .arg(QRandomGenerator::global()->generate())
                       .arg(QDateTime::currentMSecsSinceEpoch()));
    }
    
    m_activeTasks[task->getTaskId()] = task;
    
    // 根据优先级提交任务
    int priority = QThread::NormalPriority;
    switch (task->getPriority()) {
    case CancellableTask::LowPriority:
        priority = QThread::LowPriority;
        break;
    case CancellableTask::HighPriority:
        priority = QThread::HighPriority;
        break;
    case CancellableTask::CriticalPriority:
        priority = QThread::HighestPriority;
        break;
    default:
        priority = QThread::NormalPriority;
        break;
    }
    
    // 注意：CancellableTask继承自QRunnable，没有finished信号
    // 任务完成将通过其他方式处理
    
    m_threadPool->start(task, priority);
    
    QMutexLocker statsLocker(&m_statsMutex);
    m_statistics.totalTasksSubmitted++;
    
    qDebug() << "SmartThreadPool: 提交任务" << task->getTaskId() 
             << "类型:" << task->getTaskType() << "优先级:" << task->getPriority();
}

void SmartThreadPool::submitTaskWithPriority(CancellableTask* task, CancellableTask::Priority priority)
{
    if (task) {
        task->setPriority(priority);
        submitTask(task);
    }
}

void SmartThreadPool::submitTasks(const QVector<CancellableTask*>& tasks)
{
    for (CancellableTask* task : tasks) {
        submitTask(task);
    }
    
    qDebug() << "SmartThreadPool: 批量提交" << tasks.size() << "个任务";
}

void SmartThreadPool::cancelTask(const QString& taskId)
{
    QMutexLocker locker(&m_taskMutex);
    
    auto it = m_activeTasks.find(taskId);
    if (it != m_activeTasks.end()) {
        it.value()->cancel();
        
        QMutexLocker statsLocker(&m_statsMutex);
        m_statistics.totalTasksCancelled++;
        
        emit taskCancelled(taskId);
        qDebug() << "SmartThreadPool: 取消任务" << taskId;
    }
}

void SmartThreadPool::cancelTasksByType(const QString& taskType)
{
    QMutexLocker locker(&m_taskMutex);
    
    int cancelledCount = 0;
    for (auto it = m_activeTasks.begin(); it != m_activeTasks.end(); ++it) {
        if (it.value()->getTaskType() == taskType) {
            it.value()->cancel();
            cancelledCount++;
            emit taskCancelled(it.key());
        }
    }
    
    QMutexLocker statsLocker(&m_statsMutex);
    m_statistics.totalTasksCancelled += cancelledCount;
    
    qDebug() << "SmartThreadPool: 取消" << cancelledCount << "个类型为" << taskType << "的任务";
}

void SmartThreadPool::cancelAllTasks()
{
    QMutexLocker locker(&m_taskMutex);
    
    for (auto it = m_activeTasks.begin(); it != m_activeTasks.end(); ++it) {
        it.value()->cancel();
        emit taskCancelled(it.key());
    }
    
    QMutexLocker statsLocker(&m_statsMutex);
    m_statistics.totalTasksCancelled += m_activeTasks.size();
    
    qDebug() << "SmartThreadPool: 取消所有活跃任务";
}

bool SmartThreadPool::waitForDone(int msecs)
{
    return m_threadPool->waitForDone(msecs);
}

void SmartThreadPool::clear()
{
    cancelAllTasks();
    m_threadPool->clear();
    
    QMutexLocker locker(&m_taskMutex);
    m_activeTasks.clear();
    
    qDebug() << "SmartThreadPool: 清理完成";
}

SmartThreadPool::PoolStatistics SmartThreadPool::getStatistics() const
{
    QMutexLocker locker(&m_statsMutex);
    
    PoolStatistics stats = m_statistics;
    stats.activeThreads = activeThreadCount();
    stats.currentPendingTasks = m_activeTasks.size();
    
    // 计算平均执行时间
    if (!m_executionTimes.isEmpty()) {
        qint64 total = 0;
        qint64 maxTime = 0;
        for (qint64 time : m_executionTimes) {
            total += time;
            maxTime = qMax(maxTime, time);
        }
        stats.avgExecutionTime = static_cast<double>(total) / m_executionTimes.size();
        stats.maxExecutionTime = maxTime;
        stats.totalExecutionTime = total;
    }
    
    return stats;
}

void SmartThreadPool::resetStatistics()
{
    QMutexLocker locker(&m_statsMutex);
    m_statistics = PoolStatistics();
    m_executionTimes.clear();
    
    qDebug() << "SmartThreadPool: 统计信息已重置";
}

void SmartThreadPool::enableAdaptiveThreadCount(bool enabled)
{
    m_adaptiveThreadCount = enabled;
    
    if (enabled) {
        m_loadMonitorTimer.start();
        m_adaptiveTimer->start(5000); // 每5秒检查一次
        qDebug() << "SmartThreadPool: 启用自适应线程数";
    } else {
        m_adaptiveTimer->stop();
        qDebug() << "SmartThreadPool: 禁用自适应线程数";
    }
}

bool SmartThreadPool::isAdaptiveThreadCountEnabled() const
{
    return m_adaptiveThreadCount;
}

void SmartThreadPool::adjustThreadCountBasedOnLoad()
{
    if (!m_adaptiveThreadCount) {
        return;
    }
    
    double currentLoad = calculateCurrentLoad();
    int currentThreadCount = m_threadPool->maxThreadCount();
    int optimalThreadCount = calculateOptimalThreadCount();
    
    if (optimalThreadCount != currentThreadCount) {
        setMaxThreadCount(optimalThreadCount);
        
        qDebug() << "SmartThreadPool: 自适应调整线程数从" << currentThreadCount 
                 << "到" << optimalThreadCount << "，当前负载:" << currentLoad;
    }
}

void SmartThreadPool::updateStatistics()
{
    cleanupCompletedTasks();
    
    QMutexLocker locker(&m_statsMutex);
    emit poolStatisticsUpdated(getStatistics());
}

void SmartThreadPool::onTaskCompleted()
{
    CancellableTask* task = qobject_cast<CancellableTask*>(sender());
    if (task) {
        QString taskId = task->getTaskId();
        qint64 executionTime = task->getExecutionTime();
        
        {
            QMutexLocker locker(&m_taskMutex);
            m_activeTasks.remove(taskId);
        }
        
        {
            QMutexLocker statsLocker(&m_statsMutex);
            m_statistics.totalTasksCompleted++;
            
            // 记录执行时间
            m_executionTimes.append(executionTime);
            if (m_executionTimes.size() > 100) { // 保持最近100个
                m_executionTimes.removeFirst();
            }
        }
        
        emit taskCompleted(taskId, executionTime);
        
        qDebug() << "SmartThreadPool: 任务" << taskId << "完成，耗时:" << executionTime << "ms";
    }
}

double SmartThreadPool::calculateCurrentLoad() const
{
    int activeThreads = activeThreadCount();
    int maxThreads = maxThreadCount();
    
    if (maxThreads == 0) {
        return 0.0;
    }
    
    return static_cast<double>(activeThreads) / maxThreads;
}

int SmartThreadPool::calculateOptimalThreadCount() const
{
    double load = calculateCurrentLoad();
    int currentCount = maxThreadCount();
    int idealCount = QThread::idealThreadCount();
    
    if (load > 0.9) {
        // 高负载，增加线程
        return qMin(currentCount + 1, idealCount * 2);
    } else if (load < 0.3) {
        // 低负载，减少线程
        return qMax(currentCount - 1, idealCount / 2);
    }
    
    return currentCount; // 负载适中，保持不变
}

void SmartThreadPool::cleanupCompletedTasks()
{
    QMutexLocker locker(&m_taskMutex);
    
    auto it = m_activeTasks.begin();
    while (it != m_activeTasks.end()) {
        if (it.value()->isCancelled()) {
            it = m_activeTasks.erase(it);
        } else {
            ++it;
        }
    }
}

// ThreadPoolManager 实现

ThreadPoolManager* ThreadPoolManager::s_instance = nullptr;
QMutex ThreadPoolManager::s_instanceMutex;

ThreadPoolManager::ThreadPoolManager(QObject* parent)
    : QObject(parent)
    , m_adaptiveManagement(true)
    , m_globalStatsTimer(new QTimer(this))
    , m_adaptiveTimer(new QTimer(this))
{
    initializePools();
    
    // 设置全局统计定时器
    connect(m_globalStatsTimer, &QTimer::timeout, this, &ThreadPoolManager::updateGlobalStatistics);
    m_globalStatsTimer->start(2000); // 每2秒更新全局统计
    
    // 设置自适应管理定时器
    connect(m_adaptiveTimer, &QTimer::timeout, this, &ThreadPoolManager::performAdaptiveManagement);
    if (m_adaptiveManagement) {
        m_adaptiveTimer->start(10000); // 每10秒进行自适应管理
    }
    
    qDebug() << "ThreadPoolManager: 初始化完成";
}

ThreadPoolManager::~ThreadPoolManager()
{
    shutdownAllPools();
    qDebug() << "ThreadPoolManager: 已销毁";
}

ThreadPoolManager& ThreadPoolManager::instance()
{
    QMutexLocker locker(&s_instanceMutex);
    if (!s_instance) {
        s_instance = new ThreadPoolManager();
    }
    return *s_instance;
}

SmartThreadPool* ThreadPoolManager::getPool(PoolType type)
{
    auto it = m_pools.find(type);
    if (it != m_pools.end()) {
        return it->get();
    }
    return nullptr;
}

void ThreadPoolManager::submitAudioTask(CancellableTask* task)
{
    if (auto pool = getPool(AudioProcessingPool)) {
        pool->submitTask(task);
    }
}

void ThreadPoolManager::submitDecodeTask(CancellableTask* task)
{
    if (auto pool = getPool(DecodingPool)) {
        pool->submitTask(task);
    }
}

void ThreadPoolManager::submitFileIOTask(CancellableTask* task)
{
    if (auto pool = getPool(FileIOPool)) {
        pool->submitTask(task);
    }
}

void ThreadPoolManager::submitGeneralTask(CancellableTask* task)
{
    if (auto pool = getPool(GeneralPool)) {
        pool->submitTask(task);
    }
}

void ThreadPoolManager::optimizeForPerformance()
{
    // 性能模式：增加线程数，减少过期时间
    setPoolConfiguration(AudioProcessingPool, QThread::idealThreadCount() * 2, 10000);
    setPoolConfiguration(DecodingPool, QThread::idealThreadCount(), 15000);
    setPoolConfiguration(FileIOPool, 4, 20000);
    setPoolConfiguration(GeneralPool, QThread::idealThreadCount(), 15000);
    
    qDebug() << "ThreadPoolManager: 切换到性能模式";
}

void ThreadPoolManager::optimizeForPowerSaving()
{
    // 省电模式：减少线程数，增加过期时间
    int conservativeThreadCount = qMax(2, QThread::idealThreadCount() / 2);
    setPoolConfiguration(AudioProcessingPool, conservativeThreadCount, 60000);
    setPoolConfiguration(DecodingPool, conservativeThreadCount, 60000);
    setPoolConfiguration(FileIOPool, 2, 60000);
    setPoolConfiguration(GeneralPool, conservativeThreadCount, 60000);
    
    qDebug() << "ThreadPoolManager: 切换到省电模式";
}

void ThreadPoolManager::optimizeForBalance()
{
    // 平衡模式：标准配置
    int balancedThreadCount = QThread::idealThreadCount();
    setPoolConfiguration(AudioProcessingPool, balancedThreadCount, 30000);
    setPoolConfiguration(DecodingPool, balancedThreadCount, 30000);
    setPoolConfiguration(FileIOPool, 3, 30000);
    setPoolConfiguration(GeneralPool, balancedThreadCount, 30000);
    
    qDebug() << "ThreadPoolManager: 切换到平衡模式";
}

ThreadPoolManager::GlobalStatistics ThreadPoolManager::getGlobalStatistics() const
{
    QMutexLocker locker(&m_globalStatsMutex);
    return m_globalStats;
}

void ThreadPoolManager::setPoolConfiguration(PoolType type, int maxThreads, int expiryTimeout)
{
    m_poolConfigs[type] = {maxThreads, expiryTimeout};
    
    if (auto pool = getPool(type)) {
        configurePool(type, pool);
    }
    
    qDebug() << "ThreadPoolManager: 配置池" << poolTypeToString(type) 
             << "线程数:" << maxThreads << "过期时间:" << expiryTimeout;
}

void ThreadPoolManager::enableAdaptiveManagement(bool enabled)
{
    m_adaptiveManagement = enabled;
    
    if (enabled) {
        m_performanceTimer.start();
        m_adaptiveTimer->start(10000);
        qDebug() << "ThreadPoolManager: 启用自适应管理";
    } else {
        m_adaptiveTimer->stop();
        qDebug() << "ThreadPoolManager: 禁用自适应管理";
    }
}

bool ThreadPoolManager::isAdaptiveManagementEnabled() const
{
    return m_adaptiveManagement;
}

void ThreadPoolManager::cleanupAllPools()
{
    for (auto& pool : m_pools) {
        pool->clear();
    }
    
    qDebug() << "ThreadPoolManager: 清理所有线程池";
}

void ThreadPoolManager::shutdownAllPools()
{
    for (auto& pool : m_pools) {
        pool->clear();
        pool->waitForDone();
    }
    
    m_pools.clear();
    qDebug() << "ThreadPoolManager: 关闭所有线程池";
}

void ThreadPoolManager::onPoolStatisticsUpdated(const SmartThreadPool::PoolStatistics& stats)
{
    // 处理来自单个池的统计更新
    updateGlobalStatistics();
}

void ThreadPoolManager::updateGlobalStatistics()
{
    QMutexLocker locker(&m_globalStatsMutex);
    
    m_globalStats.poolStats.clear();
    m_globalStats.totalActiveThreads = 0;
    m_globalStats.totalPendingTasks = 0;
    
    for (auto it = m_pools.begin(); it != m_pools.end(); ++it) {
        PoolType type = it.key();
        auto& pool = it.value();
        
        auto stats = pool->getStatistics();
        m_globalStats.poolStats[type] = stats;
        m_globalStats.totalActiveThreads += stats.activeThreads;
        m_globalStats.totalPendingTasks += stats.currentPendingTasks;
    }
    
    // 计算系统负载（简化）
    int totalThreads = 0;
    for (auto& pool : m_pools) {
        totalThreads += pool->maxThreadCount();
    }
    
    if (totalThreads > 0) {
        m_globalStats.systemLoad = static_cast<double>(m_globalStats.totalActiveThreads) / totalThreads;
    }
    
    emit globalStatisticsUpdated(m_globalStats);
}

void ThreadPoolManager::performAdaptiveManagement()
{
    if (!m_adaptiveManagement) {
        return;
    }
    
    analyzeSystemPerformance();
    adjustPoolsBasedOnLoad();
}

void ThreadPoolManager::initializePools()
{
    // 创建专用线程池
    m_pools[AudioProcessingPool] = std::make_unique<SmartThreadPool>();
    m_pools[DecodingPool] = std::make_unique<SmartThreadPool>();
    m_pools[FileIOPool] = std::make_unique<SmartThreadPool>();
    m_pools[GeneralPool] = std::make_unique<SmartThreadPool>();
    
    // 配置每个池
    for (auto it = m_pools.begin(); it != m_pools.end(); ++it) {
        PoolType type = it.key();
        auto& pool = it.value();
        
        configurePool(type, pool.get());
        
        // 连接统计信号
        connect(pool.get(), &SmartThreadPool::poolStatisticsUpdated,
                this, &ThreadPoolManager::onPoolStatisticsUpdated);
    }
    
    // 设置默认配置
    optimizeForBalance();
    
    qDebug() << "ThreadPoolManager: 初始化了" << m_pools.size() << "个线程池";
}

void ThreadPoolManager::configurePool(PoolType type, SmartThreadPool* pool)
{
    if (!pool) {
        return;
    }
    
    PoolConfig config = m_poolConfigs.value(type, {4, 30000});
    pool->setMaxThreadCount(config.maxThreads);
    pool->setExpiryTimeout(config.expiryTimeout);
    
    // 特殊配置
    switch (type) {
    case AudioProcessingPool:
        pool->enableAdaptiveThreadCount(true);
        break;
    case DecodingPool:
        pool->enableAdaptiveThreadCount(true);
        break;
    case FileIOPool:
        // 文件I/O通常不需要太多线程
        pool->enableAdaptiveThreadCount(false);
        break;
    case GeneralPool:
        pool->enableAdaptiveThreadCount(true);
        break;
    }
}

QString ThreadPoolManager::poolTypeToString(PoolType type) const
{
    switch (type) {
    case AudioProcessingPool: return "AudioProcessing";
    case DecodingPool: return "Decoding";
    case FileIOPool: return "FileIO";
    case GeneralPool: return "General";
    default: return "Unknown";
    }
}

void ThreadPoolManager::analyzeSystemPerformance()
{
    // 简化的系统性能分析
    auto globalStats = getGlobalStatistics();
    
    if (globalStats.systemLoad > 0.8) {
        emit poolPerformanceWarning(GeneralPool, "系统负载过高");
    }
    
    // 检查各个池的性能
    for (auto it = globalStats.poolStats.begin(); it != globalStats.poolStats.end(); ++it) {
        PoolType type = it.key();
        const auto& stats = it.value();
        
        if (stats.currentPendingTasks > 50) {
            emit poolPerformanceWarning(type, "待处理任务过多");
        }
        
        if (stats.avgExecutionTime > 5000) { // 5秒
            emit poolPerformanceWarning(type, "平均执行时间过长");
        }
    }
}

void ThreadPoolManager::adjustPoolsBasedOnLoad()
{
    auto globalStats = getGlobalStatistics();
    
    // 根据负载调整线程数
    for (auto it = m_pools.begin(); it != m_pools.end(); ++it) {
        PoolType type = it.key();
        auto& pool = it.value();
        
        if (pool->isAdaptiveThreadCountEnabled()) {
            pool->adjustThreadCountBasedOnLoad();
        }
    }
}