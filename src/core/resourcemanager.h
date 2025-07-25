#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H

#include <QObject>
#include <QMutex>
#include <QSharedPointer>
#include <QWeakPointer>
#include <QMap>
#include <QTimer>
#include <QElapsedTimer>
#include <QAtomicInt>
#include <QScopedPointer>
#include <memory>
#include <map>

/**
 * @brief RAII资源包装器基类
 * 确保资源的自动管理和异常安全
 */
template<typename ResourceType>
class ResourceWrapper {
public:
    explicit ResourceWrapper(ResourceType* resource) 
        : m_resource(resource), m_isValid(resource != nullptr) {}
    
    virtual ~ResourceWrapper() {
        cleanup();
    }
    
    // 禁用拷贝构造和赋值
    ResourceWrapper(const ResourceWrapper&) = delete;
    ResourceWrapper& operator=(const ResourceWrapper&) = delete;
    
    // 移动构造和赋值
    ResourceWrapper(ResourceWrapper&& other) noexcept
        : m_resource(other.m_resource), m_isValid(other.m_isValid) {
        other.m_resource = nullptr;
        other.m_isValid = false;
    }
    
    ResourceWrapper& operator=(ResourceWrapper&& other) noexcept {
        if (this != &other) {
            cleanup();
            m_resource = other.m_resource;
            m_isValid = other.m_isValid;
            other.m_resource = nullptr;
            other.m_isValid = false;
        }
        return *this;
    }
    
    ResourceType* get() const { return m_resource; }
    ResourceType* operator->() const { return m_resource; }
    ResourceType& operator*() const { return *m_resource; }
    
    bool isValid() const { return m_isValid && m_resource != nullptr; }
    
    ResourceType* release() {
        ResourceType* temp = m_resource;
        m_resource = nullptr;
        m_isValid = false;
        return temp;
    }

protected:
    virtual void cleanup() {
        if (m_resource) {
            delete m_resource;
            m_resource = nullptr;
        }
        m_isValid = false;
    }

private:
    ResourceType* m_resource;
    bool m_isValid;
};

/**
 * @brief 音频资源独占锁
 * 确保同一时间只有一个组件可以控制音频资源
 */
class AudioResourceLock {
public:
    AudioResourceLock(const QString& lockId, const QString& ownerName);
    ~AudioResourceLock();
    
    bool tryAcquire(int timeoutMs = 5000);
    void release();
    bool isHeld() const;
    
    QString getLockId() const { return m_lockId; }
    QString getOwnerName() const { return m_ownerName; }
    qint64 getAcquireTime() const { return m_acquireTime; }
    
private:
    QString m_lockId;
    QString m_ownerName;
    bool m_isHeld;
    qint64 m_acquireTime;
    static QMutex s_globalLockMutex;
    static QMap<QString, QString> s_activeLocks; // lockId -> ownerName
};

/**
 * @brief 智能音频资源锁（RAII）
 */
class ScopedAudioLock {
public:
    explicit ScopedAudioLock(const QString& lockId, const QString& ownerName, int timeoutMs = 5000);
    ~ScopedAudioLock();
    
    bool isLocked() const { return m_lock && m_lock->isHeld(); }
    explicit operator bool() const { return isLocked(); }

private:
    std::unique_ptr<AudioResourceLock> m_lock;
};

/**
 * @brief 内存池管理器
 * 用于管理音频缓冲区和减少内存分配开销
 */
class MemoryPool {
public:
    explicit MemoryPool(size_t blockSize, size_t initialBlocks = 10);
    ~MemoryPool();
    
    void* allocate();
    void deallocate(void* ptr);
    
    size_t getBlockSize() const { return m_blockSize; }
    size_t getTotalBlocks() const { return m_totalBlocks; }
    size_t getAvailableBlocks() const { return m_freeBlocks.size(); }
    size_t getUsedBlocks() const { return m_totalBlocks - m_freeBlocks.size(); }
    
    // 统计信息
    struct PoolStats {
        size_t totalAllocations = 0;
        size_t totalDeallocations = 0;
        size_t currentUsedBlocks = 0;
        size_t peakUsedBlocks = 0;
        size_t poolExpansions = 0;
    };
    
    PoolStats getStats() const;
    void resetStats();

private:
    size_t m_blockSize;
    size_t m_totalBlocks;
    QVector<void*> m_blocks;
    QVector<void*> m_freeBlocks;
    mutable QMutex m_mutex;
    
    PoolStats m_stats;
    
    void expandPool(size_t additionalBlocks = 10);
};

/**
 * @brief 智能内存管理器
 * 针对音频数据提供优化的内存分配
 */
class SmartMemoryManager : public QObject {
    Q_OBJECT
public:
    static SmartMemoryManager& instance();
    
    // 音频缓冲区分配
    QByteArray allocateAudioBuffer(size_t size);
    void deallocateAudioBuffer(QByteArray& buffer);
    
    // 预分配常用大小的缓冲区
    void preallocateBuffers();
    
    // 内存统计
    struct MemoryStats {
        qint64 totalAllocated = 0;
        qint64 totalDeallocated = 0;
        qint64 currentUsage = 0;
        qint64 peakUsage = 0;
        int poolHitRate = 0; // 内存池命中率
    };
    
    MemoryStats getMemoryStats() const;
    void resetMemoryStats();
    
    // 内存清理
    void cleanupUnusedBuffers();
    void forceGarbageCollection();

private:
    SmartMemoryManager();
    ~SmartMemoryManager();
    
    // 不同大小的内存池
    std::map<size_t, std::unique_ptr<MemoryPool>> m_memoryPools;
    mutable QMutex m_poolMutex;
    
    MemoryStats m_stats;
    mutable QMutex m_statsMutex;
    
    QTimer* m_cleanupTimer;
    
    MemoryPool* getPoolForSize(size_t size);
    size_t roundUpToPoolSize(size_t size) const;
    
private slots:
    void performCleanup();
};

/**
 * @brief 资源管理器主类
 * 整合所有资源管理功能
 */
class ResourceManager : public QObject
{
    Q_OBJECT

public:
    explicit ResourceManager(QObject *parent = nullptr);
    ~ResourceManager();

    // 单例访问
    static ResourceManager& instance();
    
    // 音频资源锁管理
    bool requestAudioLock(const QString& lockId, const QString& ownerName, int timeoutMs = 5000);
    void releaseAudioLock(const QString& lockId);
    bool isAudioLocked(const QString& lockId) const;
    QStringList getActiveLocks() const;
    
    // 创建作用域锁
    std::unique_ptr<ScopedAudioLock> createScopedLock(const QString& lockId, 
                                                      const QString& ownerName, 
                                                      int timeoutMs = 5000);
    
    // 内存管理
    SmartMemoryManager& getMemoryManager();
    
    // 资源监控
    void startResourceMonitoring(int intervalMs = 1000);
    void stopResourceMonitoring();
    bool isResourceMonitoringActive() const;
    
    // 资源清理
    void performResourceCleanup();
    void scheduleResourceCleanup(int delayMs = 5000);
    
    // 资源统计
    struct ResourceStats {
        int activeLocks = 0;
        qint64 totalMemoryUsage = 0;
        qint64 peakMemoryUsage = 0;
        int memoryPoolHitRate = 0;
        qint64 resourceCleanupCount = 0;
    };
    
    ResourceStats getResourceStats() const;
    void resetResourceStats();

signals:
    // 资源锁信号
    void audioLockAcquired(const QString& lockId, const QString& ownerName);
    void audioLockReleased(const QString& lockId, const QString& ownerName);
    void audioLockConflict(const QString& lockId, const QString& requester, const QString& currentOwner);
    
    // 内存管理信号
    void memoryUsageHigh(qint64 usage, qint64 threshold);
    void memoryPoolExhausted(size_t blockSize);
    void memoryCleanupPerformed(qint64 freedBytes);
    
    // 资源监控信号
    void resourceStatsUpdated(const ResourceStats& stats);

private slots:
    void updateResourceStats();
    void onScheduledCleanup();

private:
    static ResourceManager* s_instance;
    static QMutex s_instanceMutex;
    
    // 资源锁管理
    QMap<QString, QSharedPointer<AudioResourceLock>> m_locks;
    mutable QMutex m_lockMutex;
    
    // 内存管理
    SmartMemoryManager* m_memoryManager;
    
    // 资源监控
    QTimer* m_monitoringTimer;
    bool m_isMonitoring;
    
    // 定期清理
    QTimer* m_cleanupTimer;
    
    // 统计信息
    mutable QMutex m_statsMutex;
    ResourceStats m_stats;
    
    // 私有方法
    void initializeResourcePools();
    void cleanupExpiredLocks();
    void updateMemoryStats();
};

// 便利宏定义
#define SCOPED_AUDIO_LOCK(lockId, ownerName) \
    auto scopedLock = ResourceManager::instance().createScopedLock(lockId, ownerName); \
    if (!scopedLock || !*scopedLock) { \
        qWarning() << "Failed to acquire audio lock:" << lockId; \
        return; \
    }

#define SCOPED_AUDIO_LOCK_RETURN(lockId, ownerName, returnValue) \
    auto scopedLock = ResourceManager::instance().createScopedLock(lockId, ownerName); \
    if (!scopedLock || !*scopedLock) { \
        qWarning() << "Failed to acquire audio lock:" << lockId; \
        return returnValue; \
    }

#endif // RESOURCEMANAGER_H