#include "performancemanager.h"
#include "logger.h"
#include <QApplication>
#include <QThread>
#include <QProcess>
#include <QSysInfo>
#include <QStandardPaths>
#include <QDir>
#include <QtMath>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QRegularExpression>

// PerformanceManager 实现

PerformanceManager::PerformanceManager(QObject *parent)
    : QObject(parent)
    , m_monitoringTimer(new QTimer(this))
    , m_isMonitoring(false)
    , m_monitoringInterval(1000)
    , m_currentDecodeInterval(20)
    , m_adaptiveDecodingEnabled(true)
    , m_targetCpuUsage(30.0)
    , m_targetResponseTime(16.0)
    , m_currentProfile(PerformanceProfile::Balanced)
    , m_currentCpuUsage(0)
    , m_currentMemoryUsage(0)
    , m_currentResponseTime(0)
    , m_currentBufferLevel(0)
    , m_maxCpuUsage(80.0)
    , m_maxMemoryUsage(1024 * 1024 * 1024) // 1GB
    , m_maxResponseTime(50.0)
{
    // 初始化性能统计
    m_stats = PerformanceStats();
    m_statsTimer.start();
    
    // 设置定时器
    connect(m_monitoringTimer, &QTimer::timeout, this, &PerformanceManager::updatePerformanceMetrics);
    m_monitoringTimer->setInterval(m_monitoringInterval);
    
    // 初始化性能配置
    initializePerformanceProfile(m_currentProfile);
    
    qDebug() << "PerformanceManager: 初始化完成";
}

PerformanceManager::~PerformanceManager()
{
    stopMonitoring();
    qDebug() << "PerformanceManager: 已销毁";
}

void PerformanceManager::startMonitoring()
{
    if (m_isMonitoring) {
        return;
    }
    
    m_isMonitoring = true;
    m_performanceTimer.start();
    m_monitoringTimer->start();
    
    qDebug() << "PerformanceManager: 开始性能监控";
    emit performanceUpdated(0, 0, 0);
}

void PerformanceManager::stopMonitoring()
{
    if (!m_isMonitoring) {
        return;
    }
    
    m_isMonitoring = false;
    m_monitoringTimer->stop();
    
    qDebug() << "PerformanceManager: 停止性能监控";
}

bool PerformanceManager::isMonitoring() const
{
    return m_isMonitoring;
}

void PerformanceManager::setTargetCpuUsage(double percentage)
{
    m_targetCpuUsage = qBound(5.0, percentage, 95.0);
    qDebug() << "PerformanceManager: 设置目标CPU使用率:" << m_targetCpuUsage << "%";
}

void PerformanceManager::setTargetResponseTime(double milliseconds)
{
    m_targetResponseTime = qBound(1.0, milliseconds, 100.0);
    qDebug() << "PerformanceManager: 设置目标响应时间:" << m_targetResponseTime << "ms";
}

void PerformanceManager::enableAdaptiveDecoding(bool enabled)
{
    m_adaptiveDecodingEnabled = enabled;
    qDebug() << "PerformanceManager: 自适应解码" << (enabled ? "启用" : "禁用");
}

bool PerformanceManager::isAdaptiveDecodingEnabled() const
{
    return m_adaptiveDecodingEnabled;
}

int PerformanceManager::getCurrentDecodeInterval() const
{
    return m_currentDecodeInterval.loadAcquire();
}

void PerformanceManager::setDecodeInterval(int intervalMs)
{
    int oldInterval = m_currentDecodeInterval.loadAcquire();
    intervalMs = qBound(10, intervalMs, 100); // 10-100ms范围
    
    if (oldInterval != intervalMs) {
        m_currentDecodeInterval.storeRelease(intervalMs);
        emit decodeIntervalChanged(intervalMs, oldInterval);
        
        QMutexLocker locker(&m_statsMutex);
        m_stats.adjustmentCount++;
        
        qDebug() << "PerformanceManager: 解码间隔调整从" << oldInterval << "ms到" << intervalMs << "ms";
    }
}

void PerformanceManager::adjustDecodeFrequency()
{
    if (!m_adaptiveDecodingEnabled || !shouldAdjustFrequency()) {
        return;
    }
    
    double currentCpu = m_currentCpuUsage.loadAcquire() / 100.0;
    double currentResponse = m_currentResponseTime.loadAcquire() / 1000.0;
    int currentInterval = m_currentDecodeInterval.loadAcquire();
    
    int newInterval = calculateOptimalInterval(currentCpu, m_targetCpuUsage);
    
    // 考虑响应时间因素
    if (currentResponse > m_targetResponseTime) {
        newInterval = qMin(newInterval + 5, 50); // 增加间隔降低负载
    }
    
    // 考虑缓冲区状态
    int bufferLevel = m_currentBufferLevel.loadAcquire();
    if (bufferLevel < 20) { // 缓冲区不足
        newInterval = qMax(newInterval - 5, 10); // 减少间隔提高响应
    } else if (bufferLevel > 80) { // 缓冲区充足
        newInterval = qMin(newInterval + 3, 40); // 适度增加间隔节能
    }
    
    if (newInterval != currentInterval) {
        setDecodeInterval(newInterval);
        
        QString reason = QString("CPU:%.1f%% 响应:%.1fms 缓冲:%d%%")
                        .arg(currentCpu).arg(currentResponse).arg(bufferLevel);
        emit adaptiveAdjustmentMade(reason, newInterval);
        
        logPerformanceAdjustment(reason, currentInterval, newInterval);
    }
}

double PerformanceManager::getCurrentCpuUsage() const
{
    return m_currentCpuUsage.loadAcquire() / 100.0;
}

qint64 PerformanceManager::getCurrentMemoryUsage() const
{
    return m_currentMemoryUsage.loadAcquire();
}

double PerformanceManager::getAverageResponseTime() const
{
    return calculateAverageResponseTime();
}

int PerformanceManager::getBufferLevel() const
{
    return m_currentBufferLevel.loadAcquire();
}

void PerformanceManager::reportAudioEnginePerformance(const QString& engineType, 
                                                     double cpuUsage, 
                                                     qint64 memoryUsage,
                                                     double responseTime)
{
    m_currentEngineType = engineType;
    m_currentCpuUsage.storeRelease(static_cast<int>(cpuUsage * 100));
    m_currentMemoryUsage.storeRelease(memoryUsage);
    m_currentResponseTime.storeRelease(static_cast<int>(responseTime * 1000));
    
    updateHistory(cpuUsage, memoryUsage, responseTime);
    updateStatistics(cpuUsage, memoryUsage, responseTime);
    
    // 发布性能事件
    AudioEvents::PerformanceInfo perfInfo;
    perfInfo.cpuUsage = cpuUsage;
    perfInfo.memoryUsage = memoryUsage;
    perfInfo.bufferLevel = m_currentBufferLevel.loadAcquire();
    perfInfo.responseTime = responseTime;
    perfInfo.engineType = engineType;
    
    notifyObservers(perfInfo);
    
    // 触发自适应调整
    if (m_adaptiveDecodingEnabled) {
        adjustDecodeFrequency();
    }
}

void PerformanceManager::reportBufferLevel(int level)
{
    m_currentBufferLevel.storeRelease(qBound(0, level, 100));
}

void PerformanceManager::reportBufferUnderrun()
{
    QMutexLocker locker(&m_statsMutex);
    m_stats.bufferUnderruns++;
    emit bufferUnderrunDetected();
    
    // 紧急调整：降低解码间隔
    if (m_adaptiveDecodingEnabled) {
        int currentInterval = m_currentDecodeInterval.loadAcquire();
        int newInterval = qMax(currentInterval - 5, 10);
        setDecodeInterval(newInterval);
        emit adaptiveAdjustmentMade("缓冲区欠载", newInterval);
    }
}

void PerformanceManager::reportBufferOverflow()
{
    QMutexLocker locker(&m_statsMutex);
    m_stats.bufferOverflows++;
    emit bufferOverflowDetected();
    
    // 调整：增加解码间隔
    if (m_adaptiveDecodingEnabled) {
        int currentInterval = m_currentDecodeInterval.loadAcquire();
        int newInterval = qMin(currentInterval + 3, 50);
        setDecodeInterval(newInterval);
        emit adaptiveAdjustmentMade("缓冲区溢出", newInterval);
    }
}

void PerformanceManager::setPerformanceThresholds(double maxCpuUsage, 
                                                 qint64 maxMemoryUsage, 
                                                 double maxResponseTime)
{
    m_maxCpuUsage = maxCpuUsage;
    m_maxMemoryUsage = maxMemoryUsage;
    m_maxResponseTime = maxResponseTime;
    
    qDebug() << "PerformanceManager: 设置性能阈值 - CPU:" << maxCpuUsage 
             << "% 内存:" << maxMemoryUsage/1024/1024 << "MB 响应:" << maxResponseTime << "ms";
}

void PerformanceManager::setPerformanceProfile(PerformanceProfile profile)
{
    if (m_currentProfile == profile) {
        return;
    }
    
    m_currentProfile = profile;
    initializePerformanceProfile(profile);
    
    qDebug() << "PerformanceManager: 切换性能配置到" << static_cast<int>(profile);
}

PerformanceManager::PerformanceProfile PerformanceManager::getPerformanceProfile() const
{
    return m_currentProfile;
}

void PerformanceManager::enableResourceMonitoring(bool enabled)
{
    if (enabled) {
        startMonitoring();
    } else {
        stopMonitoring();
    }
}

void PerformanceManager::setMonitoringInterval(int intervalMs)
{
    m_monitoringInterval = qBound(100, intervalMs, 5000);
    m_monitoringTimer->setInterval(m_monitoringInterval);
    
    qDebug() << "PerformanceManager: 监控间隔设置为" << m_monitoringInterval << "ms";
}

PerformanceManager::PerformanceStats PerformanceManager::getPerformanceStats() const
{
    QMutexLocker locker(&m_statsMutex);
    
    PerformanceStats stats = m_stats;
    stats.totalRunTime = m_statsTimer.elapsed();
    
    return stats;
}

void PerformanceManager::resetPerformanceStats()
{
    QMutexLocker locker(&m_statsMutex);
    m_stats = PerformanceStats();
    m_statsTimer.restart();
    
    qDebug() << "PerformanceManager: 性能统计已重置";
}

void PerformanceManager::updatePerformanceMetrics()
{
    if (!m_isMonitoring) {
        return;
    }
    
    // 计算性能指标
    double cpuUsage = calculateCpuUsage();
    qint64 memoryUsage = calculateMemoryUsage();
    double responseTime = calculateAverageResponseTime();
    
    // 更新原子变量
    m_currentCpuUsage.storeRelease(static_cast<int>(cpuUsage * 100));
    m_currentMemoryUsage.storeRelease(memoryUsage);
    m_currentResponseTime.storeRelease(static_cast<int>(responseTime * 1000));
    
    // 更新历史数据
    updateHistory(cpuUsage, memoryUsage, responseTime);
    updateStatistics(cpuUsage, memoryUsage, responseTime);
    
    // 发出性能更新信号
    emit performanceUpdated(cpuUsage, memoryUsage, responseTime);
    
    // 检查阈值
    checkPerformanceThresholds();
    
    // 发布观察者事件
    AudioEvents::PerformanceInfo perfInfo;
    perfInfo.cpuUsage = cpuUsage;
    perfInfo.memoryUsage = memoryUsage;
    perfInfo.bufferLevel = m_currentBufferLevel.loadAcquire();
    perfInfo.responseTime = responseTime;
    perfInfo.engineType = m_currentEngineType;
    
    notifyObservers(perfInfo);
}

void PerformanceManager::checkPerformanceThresholds()
{
    double cpu = getCurrentCpuUsage();
    qint64 memory = getCurrentMemoryUsage();
    double response = getAverageResponseTime();
    
    if (cpu > m_maxCpuUsage) {
        emit cpuUsageHigh(cpu);
        emit performanceThresholdExceeded("CPU", cpu, m_maxCpuUsage);
    }
    
    if (memory > m_maxMemoryUsage) {
        emit memoryUsageHigh(memory);
        emit performanceThresholdExceeded("Memory", memory, m_maxMemoryUsage);
    }
    
    if (response > m_maxResponseTime) {
        emit responseTimeSlow(response);
        emit performanceThresholdExceeded("ResponseTime", response, m_maxResponseTime);
    }
}

void PerformanceManager::initializePerformanceProfile(PerformanceProfile profile)
{
    switch (profile) {
    case PerformanceProfile::PowerSaver:
        m_targetCpuUsage = 15.0;
        m_targetResponseTime = 50.0;
        setDecodeInterval(40); // 25fps
        m_monitoringInterval = 2000; // 较低监控频率
        break;
        
    case PerformanceProfile::Balanced:
        m_targetCpuUsage = 30.0;
        m_targetResponseTime = 20.0;
        setDecodeInterval(25); // 40fps
        m_monitoringInterval = 1000; // 标准监控频率
        break;
        
    case PerformanceProfile::Performance:
        m_targetCpuUsage = 50.0;
        m_targetResponseTime = 10.0;
        setDecodeInterval(16); // 60fps
        m_monitoringInterval = 500; // 高频监控
        break;
        
    case PerformanceProfile::Custom:
        // 保持当前设置
        break;
    }
    
    if (m_monitoringTimer) {
        m_monitoringTimer->setInterval(m_monitoringInterval);
    }
}

double PerformanceManager::calculateCpuUsage()
{
    // 简化的CPU使用率计算
    // 在实际应用中，应该使用平台特定的API
    static QElapsedTimer timer;
    static qint64 lastCpuTime = 0;
    
    if (!timer.isValid()) {
        timer.start();
        return 0.0;
    }
    
    qint64 elapsed = timer.restart();
    qint64 cpuTime = QThread::currentThread()->isRunning() ? elapsed : 0;
    
    double usage = 0.0;
    if (elapsed > 0) {
        usage = static_cast<double>(cpuTime - lastCpuTime) / elapsed * 100.0;
        usage = qBound(0.0, usage, 100.0);
    }
    
    lastCpuTime = cpuTime;
    return usage;
}

qint64 PerformanceManager::calculateMemoryUsage()
{
    // 获取当前进程内存使用量
    // 这是一个简化实现，实际应该使用平台特定的API
    
#ifdef Q_OS_LINUX
    QFile file("/proc/self/status");
    if (file.open(QIODevice::ReadOnly)) {
        QTextStream stream(&file);
        QString line;
        while (stream.readLineInto(&line)) {
            if (line.startsWith("VmRSS:")) {
                QStringList parts = line.split(QRegularExpression("\\s+"));
                if (parts.size() >= 2) {
                    return parts[1].toLongLong() * 1024; // KB to bytes
                }
            }
        }
    }
#endif
    
    // 回退到估算值
    return QCoreApplication::applicationPid() * 1024 * 1024; // 粗略估算
}

double PerformanceManager::calculateAverageResponseTime()
{
    QMutexLocker locker(&m_historyMutex);
    
    if (m_responseTimeHistory.isEmpty()) {
        return 0.0;
    }
    
    double sum = 0.0;
    for (double time : m_responseTimeHistory) {
        sum += time;
    }
    
    return sum / m_responseTimeHistory.size();
}

void PerformanceManager::updateHistory(double cpu, qint64 memory, double responseTime)
{
    QMutexLocker locker(&m_historyMutex);
    
    // 添加新数据
    m_cpuHistory.enqueue(cpu);
    m_memoryHistory.enqueue(memory);
    m_responseTimeHistory.enqueue(responseTime);
    
    // 保持历史大小限制
    while (m_cpuHistory.size() > MAX_HISTORY_SIZE) {
        m_cpuHistory.dequeue();
    }
    while (m_memoryHistory.size() > MAX_HISTORY_SIZE) {
        m_memoryHistory.dequeue();
    }
    while (m_responseTimeHistory.size() > MAX_HISTORY_SIZE) {
        m_responseTimeHistory.dequeue();
    }
}

void PerformanceManager::updateStatistics(double cpu, qint64 memory, double responseTime)
{
    QMutexLocker locker(&m_statsMutex);
    
    // 更新平均值
    if (m_stats.avgCpuUsage == 0.0) {
        m_stats.avgCpuUsage = cpu;
    } else {
        m_stats.avgCpuUsage = (m_stats.avgCpuUsage * 0.9) + (cpu * 0.1);
    }
    
    if (m_stats.avgMemoryUsage == 0) {
        m_stats.avgMemoryUsage = memory;
    } else {
        m_stats.avgMemoryUsage = (m_stats.avgMemoryUsage * 9 + memory) / 10;
    }
    
    if (m_stats.avgResponseTime == 0.0) {
        m_stats.avgResponseTime = responseTime;
    } else {
        m_stats.avgResponseTime = (m_stats.avgResponseTime * 0.9) + (responseTime * 0.1);
    }
    
    // 更新峰值
    m_stats.maxCpuUsage = qMax(m_stats.maxCpuUsage, cpu);
    m_stats.maxMemoryUsage = qMax(m_stats.maxMemoryUsage, memory);
    m_stats.maxResponseTime = qMax(m_stats.maxResponseTime, responseTime);
}

bool PerformanceManager::shouldAdjustFrequency() const
{
    // 避免频繁调整
    return m_lastAdjustmentTime.elapsed() > 2000; // 至少2秒间隔
}

int PerformanceManager::calculateOptimalInterval(double currentCpu, double targetCpu) const
{
    int currentInterval = m_currentDecodeInterval.loadAcquire();
    
    if (qAbs(currentCpu - targetCpu) < 2.0) {
        return currentInterval; // 差异不大，不调整
    }
    
    double ratio = currentCpu / targetCpu;
    int newInterval = static_cast<int>(currentInterval * ratio);
    
    return qBound(10, newInterval, 100);
}

void PerformanceManager::logPerformanceAdjustment(const QString& reason, int oldInterval, int newInterval)
{
    QString logMessage = QString("性能调整: %1 - 解码间隔从%2ms调整到%3ms")
                        .arg(reason).arg(oldInterval).arg(newInterval);
    
    qDebug() << "PerformanceManager:" << logMessage;
    
    // 这里可以集成到项目的日志系统
    // LOG_INFO("PerformanceManager", logMessage);
}

// AdaptiveDecodeController 实现

AdaptiveDecodeController::AdaptiveDecodeController(PerformanceManager* perfManager, QObject* parent)
    : QObject(parent)
    , m_performanceManager(perfManager)
    , m_enabled(true)
    , m_minInterval(10)
    , m_maxInterval(100)
    , m_currentRecommendedInterval(20)
    , m_adjustmentSensitivity(1.0)
    , m_stabilizationDelay(1000)
    , m_isStabilizing(false)
{
    if (m_performanceManager) {
        connect(m_performanceManager, &PerformanceManager::performanceUpdated,
                this, &AdaptiveDecodeController::onPerformanceUpdated);
    }
    
    qDebug() << "AdaptiveDecodeController: 初始化完成";
}

AdaptiveDecodeController::~AdaptiveDecodeController()
{
    qDebug() << "AdaptiveDecodeController: 已销毁";
}

void AdaptiveDecodeController::setEnabled(bool enabled)
{
    if (m_enabled != enabled) {
        m_enabled = enabled;
        emit adaptationStatusChanged(enabled);
        qDebug() << "AdaptiveDecodeController: 自适应控制" << (enabled ? "启用" : "禁用");
    }
}

bool AdaptiveDecodeController::isEnabled() const
{
    return m_enabled;
}

void AdaptiveDecodeController::setMinInterval(int minMs)
{
    m_minInterval = qBound(5, minMs, m_maxInterval - 1);
    qDebug() << "AdaptiveDecodeController: 最小间隔设置为" << m_minInterval << "ms";
}

void AdaptiveDecodeController::setMaxInterval(int maxMs)
{
    m_maxInterval = qBound(m_minInterval + 1, maxMs, 200);
    qDebug() << "AdaptiveDecodeController: 最大间隔设置为" << m_maxInterval << "ms";
}

int AdaptiveDecodeController::getMinInterval() const
{
    return m_minInterval;
}

int AdaptiveDecodeController::getMaxInterval() const
{
    return m_maxInterval;
}

void AdaptiveDecodeController::setAdjustmentSensitivity(double sensitivity)
{
    m_adjustmentSensitivity = qBound(0.1, sensitivity, 2.0);
    qDebug() << "AdaptiveDecodeController: 调整敏感度设置为" << m_adjustmentSensitivity;
}

void AdaptiveDecodeController::setStabilizationDelay(int delayMs)
{
    m_stabilizationDelay = qBound(100, delayMs, 5000);
    qDebug() << "AdaptiveDecodeController: 稳定延迟设置为" << m_stabilizationDelay << "ms";
}

double AdaptiveDecodeController::getAdjustmentSensitivity() const
{
    return m_adjustmentSensitivity;
}

int AdaptiveDecodeController::getStabilizationDelay() const
{
    return m_stabilizationDelay;
}

int AdaptiveDecodeController::getRecommendedInterval() const
{
    return m_currentRecommendedInterval;
}

void AdaptiveDecodeController::onPerformanceUpdated(double cpuUsage, qint64 memoryUsage, double responseTime)
{
    Q_UNUSED(memoryUsage)
    
    if (!m_enabled || m_isStabilizing) {
        return;
    }
    
    int newInterval = calculateAdaptiveInterval(cpuUsage, responseTime);
    
    if (shouldMakeAdjustment(newInterval)) {
        m_currentRecommendedInterval = newInterval;
        emit intervalRecommendationChanged(newInterval);
        
        m_lastAdjustmentTime.restart();
        applyStabilization();
        
        qDebug() << "AdaptiveDecodeController: 推荐间隔调整为" << newInterval << "ms"
                 << "CPU:" << cpuUsage << "% 响应:" << responseTime << "ms";
    }
}

int AdaptiveDecodeController::calculateAdaptiveInterval(double cpuUsage, double responseTime) const
{
    // 基于CPU使用率的基础间隔
    int baseInterval = 20; // 默认20ms (50fps)
    
    if (cpuUsage > 80.0) {
        baseInterval = 50; // 20fps，降低负载
    } else if (cpuUsage > 60.0) {
        baseInterval = 33; // 30fps
    } else if (cpuUsage > 40.0) {
        baseInterval = 25; // 40fps
    } else if (cpuUsage < 20.0) {
        baseInterval = 16; // 60fps，高性能
    }
    
    // 响应时间调整因子
    double responseFactor = 1.0;
    if (responseTime > 30.0) {
        responseFactor = 1.5; // 响应慢，增加间隔
    } else if (responseTime < 10.0) {
        responseFactor = 0.8; // 响应快，可以降低间隔
    }
    
    // 应用敏感度调整
    int adjustedInterval = static_cast<int>(baseInterval * responseFactor * m_adjustmentSensitivity);
    
    return qBound(m_minInterval, adjustedInterval, m_maxInterval);
}

bool AdaptiveDecodeController::shouldMakeAdjustment(int newInterval) const
{
    // 检查是否有足够的变化值得调整
    int currentInterval = m_currentRecommendedInterval;
    int difference = qAbs(newInterval - currentInterval);
    
    // 至少5ms的差异才调整
    if (difference < 5) {
        return false;
    }
    
    // 检查稳定延迟
    if (m_lastAdjustmentTime.isValid() && 
        m_lastAdjustmentTime.elapsed() < m_stabilizationDelay) {
        return false;
    }
    
    return true;
}

void AdaptiveDecodeController::applyStabilization()
{
    m_isStabilizing = true;
    
    QTimer::singleShot(m_stabilizationDelay, this, [this]() {
        m_isStabilizing = false;
    });
}