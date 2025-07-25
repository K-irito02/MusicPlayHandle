#ifndef PERFORMANCEMANAGER_H
#define PERFORMANCEMANAGER_H

#include <QObject>
#include <QTimer>
#include <QElapsedTimer>
#include <QThread>
#include <QMutex>
#include <QAtomicInt>
#include <QAtomicInteger>
#include <QAtomicPointer>
#include <QQueue>
#include <QVector>
#include "observer.h"

/**
 * @brief 性能管理器
 * 用于动态调整解码频率、监控系统性能和管理资源使用
 */
class PerformanceManager : public QObject, public AudioPerformanceSubject
{
    Q_OBJECT

public:
    explicit PerformanceManager(QObject *parent = nullptr);
    ~PerformanceManager();

    // 性能监控控制
    void startMonitoring();
    void stopMonitoring();
    bool isMonitoring() const;

    // 动态频率调整
    void setTargetCpuUsage(double percentage);
    void setTargetResponseTime(double milliseconds);
    void enableAdaptiveDecoding(bool enabled);
    bool isAdaptiveDecodingEnabled() const;

    // 解码频率控制
    int getCurrentDecodeInterval() const;
    void setDecodeInterval(int intervalMs);
    void adjustDecodeFrequency();

    // 性能指标获取
    double getCurrentCpuUsage() const;
    qint64 getCurrentMemoryUsage() const;
    double getAverageResponseTime() const;
    int getBufferLevel() const;

    // 音频引擎性能
    void reportAudioEnginePerformance(const QString& engineType, 
                                     double cpuUsage, 
                                     qint64 memoryUsage,
                                     double responseTime);

    // 缓冲区状态
    void reportBufferLevel(int level);
    void reportBufferUnderrun();
    void reportBufferOverflow();

    // 性能阈值设置
    void setPerformanceThresholds(double maxCpuUsage, 
                                 qint64 maxMemoryUsage, 
                                 double maxResponseTime);

    // 性能预设
    enum class PerformanceProfile {
        PowerSaver,      // 省电模式：低CPU使用，较低刷新率
        Balanced,        // 平衡模式：中等性能和功耗
        Performance,     // 性能模式：高刷新率，高响应性
        Custom          // 自定义模式：用户自定义参数
    };

    void setPerformanceProfile(PerformanceProfile profile);
    PerformanceProfile getPerformanceProfile() const;

    // 资源监控
    void enableResourceMonitoring(bool enabled);
    void setMonitoringInterval(int intervalMs);

    // 性能统计
    struct PerformanceStats {
        double avgCpuUsage = 0.0;
        double maxCpuUsage = 0.0;
        qint64 avgMemoryUsage = 0;
        qint64 maxMemoryUsage = 0;
        double avgResponseTime = 0.0;
        double maxResponseTime = 0.0;
        int bufferUnderruns = 0;
        int bufferOverflows = 0;
        qint64 totalRunTime = 0;
        int adjustmentCount = 0;
    };

    PerformanceStats getPerformanceStats() const;
    void resetPerformanceStats();

signals:
    // 性能监控信号
    void performanceUpdated(double cpuUsage, qint64 memoryUsage, double responseTime);
    void performanceThresholdExceeded(const QString& metric, double value, double threshold);
    
    // 频率调整信号
    void decodeIntervalChanged(int newInterval, int oldInterval);
    void adaptiveAdjustmentMade(const QString& reason, int newInterval);
    
    // 资源警告信号
    void cpuUsageHigh(double usage);
    void memoryUsageHigh(qint64 usage);
    void responseTimeSlow(double time);
    void bufferUnderrunDetected();
    void bufferOverflowDetected();

private slots:
    void updatePerformanceMetrics();
    void checkPerformanceThresholds();

private:
    // 性能监控
    QTimer* m_monitoringTimer;
    QElapsedTimer m_performanceTimer;
    bool m_isMonitoring;
    int m_monitoringInterval;

    // 动态频率调整
    QAtomicInt m_currentDecodeInterval;
    bool m_adaptiveDecodingEnabled;
    double m_targetCpuUsage;
    double m_targetResponseTime;
    PerformanceProfile m_currentProfile;

    // 性能指标
    QAtomicInt m_currentCpuUsage;      // 乘以100存储，避免浮点原子操作
    QAtomicInteger<qint64> m_currentMemoryUsage;
    QAtomicInt m_currentResponseTime;   // 乘以1000存储
    QAtomicInt m_currentBufferLevel;

    // 性能阈值
    double m_maxCpuUsage;
    qint64 m_maxMemoryUsage;
    double m_maxResponseTime;

    // 历史数据（用于计算平均值）
    mutable QMutex m_historyMutex;
    QQueue<double> m_cpuHistory;
    QQueue<qint64> m_memoryHistory;
    QQueue<double> m_responseTimeHistory;
    static const int MAX_HISTORY_SIZE = 50;

    // 性能统计
    mutable QMutex m_statsMutex;
    PerformanceStats m_stats;
    QElapsedTimer m_statsTimer;

    // 音频引擎信息
    QString m_currentEngineType;
    QElapsedTimer m_lastAdjustmentTime;

    // 私有方法
    void initializePerformanceProfile(PerformanceProfile profile);
    void adjustFrequencyBasedOnCpu(double cpuUsage);
    void adjustFrequencyBasedOnResponseTime(double responseTime);
    void adjustFrequencyBasedOnBuffer(int bufferLevel);
    
    double calculateCpuUsage() const;
    qint64 calculateMemoryUsage() const;
    double calculateAverageResponseTime() const;
    
    void updateHistory(double cpu, qint64 memory, double responseTime);
    void updateStatistics(double cpu, qint64 memory, double responseTime);
    
    bool shouldAdjustFrequency() const;
    int calculateOptimalInterval(double currentCpu, double targetCpu) const;
    
    void logPerformanceAdjustment(const QString& reason, int oldInterval, int newInterval);
};

/**
 * @brief 智能解码频率控制器
 * 基于系统性能动态调整FFmpeg解码频率
 */
class AdaptiveDecodeController : public QObject
{
    Q_OBJECT

public:
    explicit AdaptiveDecodeController(PerformanceManager* perfManager, QObject* parent = nullptr);
    ~AdaptiveDecodeController();

    // 控制接口
    void setEnabled(bool enabled);
    bool isEnabled() const;
    
    void setMinInterval(int minMs);
    void setMaxInterval(int maxMs);
    int getMinInterval() const;
    int getMaxInterval() const;

    // 自适应算法参数
    void setAdjustmentSensitivity(double sensitivity);  // 0.1 - 2.0
    void setStabilizationDelay(int delayMs);           // 防止频繁调整
    
    double getAdjustmentSensitivity() const;
    int getStabilizationDelay() const;

    // 获取当前推荐的解码间隔
    int getRecommendedInterval() const;

signals:
    void intervalRecommendationChanged(int newInterval);
    void adaptationStatusChanged(bool enabled);

private slots:
    void onPerformanceUpdated(double cpuUsage, qint64 memoryUsage, double responseTime);

private:
    PerformanceManager* m_performanceManager;
    bool m_enabled;
    
    // 间隔范围
    int m_minInterval;
    int m_maxInterval;
    int m_currentRecommendedInterval;
    
    // 自适应算法参数
    double m_adjustmentSensitivity;
    int m_stabilizationDelay;
    
    // 状态跟踪
    QElapsedTimer m_lastAdjustmentTime;
    bool m_isStabilizing;
    
    // 算法实现
    int calculateAdaptiveInterval(double cpuUsage, double responseTime) const;
    bool shouldMakeAdjustment(int newInterval) const;
    void applyStabilization();
};

#endif // PERFORMANCEMANAGER_H