#include "resourcemanager.h"
#include "logger.h"
#include <QApplication>
#include <QThread>
#include <QDebug>
#include <QElapsedTimer>
#include <QRandomGenerator>
#include <cstring>
#include <QDateTime>

// AudioResourceLock 静态成员初始化
QMutex AudioResourceLock::s_globalLockMutex;
QMap<QString, QString> AudioResourceLock::s_activeLocks;

// AudioResourceLock 实现

AudioResourceLock::AudioResourceLock(const QString& lockId, const QString& ownerName)
    : m_lockId(lockId)
    , m_ownerName(ownerName)
    , m_isHeld(false)
    , m_acquireTime(0)
{
}

AudioResourceLock::~AudioResourceLock()
{
    release();
}

bool AudioResourceLock::tryAcquire(int timeoutMs)
{
    QElapsedTimer timer;
    timer.start();
    
    while (timer.elapsed() < timeoutMs) {
        {
            QMutexLocker locker(&s_globalLockMutex);
            
            auto it = s_activeLocks.find(m_lockId);
            if (it == s_activeLocks.end()) {
                // 锁可用，获取它
                s_activeLocks[m_lockId] = m_ownerName;
                m_isHeld = true;
                m_acquireTime = QDateTime::currentMSecsSinceEpoch();
                
                qDebug() << "AudioResourceLock: 锁" << m_lockId 
                         << "被" << m_ownerName << "成功获取";
                return true;
            } else if (it.value() == m_ownerName) {
                // 同一所有者重复获取
                m_isHeld = true;
                return true;
            }
        }
        
        // 等待一小段时间后重试
        QThread::msleep(10);
    }
    
    qWarning() << "AudioResourceLock: 锁" << m_lockId 
               << "获取超时，当前持有者:" << s_activeLocks.value(m_lockId);
    return false;
}

void AudioResourceLock::release()
{
    if (!m_isHeld) {
        return;
    }
    
    QMutexLocker locker(&s_globalLockMutex);
    
    auto it = s_activeLocks.find(m_lockId);
    if (it != s_activeLocks.end() && it.value() == m_ownerName) {
        s_activeLocks.erase(it);
        m_isHeld = false;
        m_acquireTime = 0;
        
        qDebug() << "AudioResourceLock: 锁" << m_lockId 
                 << "被" << m_ownerName << "释放";
    }
}

bool AudioResourceLock::isHeld() const
{
    return m_isHeld;
}

// ScopedAudioLock 实现

ScopedAudioLock::ScopedAudioLock(const QString& lockId, const QString& ownerName, int timeoutMs)
    : m_lock(std::make_unique<AudioResourceLock>(lockId, ownerName))
{
    if (!m_lock->tryAcquire(timeoutMs)) {
        m_lock.reset(); // 获取失败，清空指针
    }
}

ScopedAudioLock::~ScopedAudioLock()
{
    // m_lock的析构函数会自动调用release()
}

// MemoryPool 实现

MemoryPool::MemoryPool(size_t blockSize, size_t initialBlocks)
    : m_blockSize(blockSize)
    , m_totalBlocks(0)
{
    expandPool(initialBlocks);
    qDebug() << "MemoryPool: 创建内存池，块大小:" << blockSize 
             << "初始块数:" << initialBlocks;
}

MemoryPool::~MemoryPool()
{
    QMutexLocker locker(&m_mutex);
    
    // 释放所有分配的内存块
    for (void* block : m_blocks) {
        std::free(block);
    }
    
    qDebug() << "MemoryPool: 销毁内存池，总分配:" << m_stats.totalAllocations
             << "总释放:" << m_stats.totalDeallocations;
}

void* MemoryPool::allocate()
{
    QMutexLocker locker(&m_mutex);
    
    if (m_freeBlocks.isEmpty()) {
        // 没有可用块，扩展池
        expandPool(qMax(static_cast<size_t>(10), m_totalBlocks / 4));
    }
    
    if (m_freeBlocks.isEmpty()) {
        qWarning() << "MemoryPool: 内存池耗尽，块大小:" << m_blockSize;
        return nullptr;
    }
    
    void* block = m_freeBlocks.takeLast();
    
    // 更新统计
    m_stats.totalAllocations++;
    m_stats.currentUsedBlocks++;
    m_stats.peakUsedBlocks = qMax(m_stats.peakUsedBlocks, m_stats.currentUsedBlocks);
    
    return block;
}

void MemoryPool::deallocate(void* ptr)
{
    if (!ptr) {
        return;
    }
    
    QMutexLocker locker(&m_mutex);
    
    // 验证指针是否属于这个池
    bool found = false;
    for (void* block : m_blocks) {
        if (block == ptr) {
            found = true;
            break;
        }
    }
    
    if (!found) {
        qWarning() << "MemoryPool: 尝试释放不属于此池的内存:" << ptr;
        return;
    }
    
    m_freeBlocks.append(ptr);
    
    // 更新统计
    m_stats.totalDeallocations++;
    m_stats.currentUsedBlocks--;
}

MemoryPool::PoolStats MemoryPool::getStats() const
{
    QMutexLocker locker(&m_mutex);
    return m_stats;
}

void MemoryPool::resetStats()
{
    QMutexLocker locker(&m_mutex);
    m_stats.totalAllocations = 0;
    m_stats.totalDeallocations = 0;
    m_stats.peakUsedBlocks = m_stats.currentUsedBlocks;
    m_stats.poolExpansions = 0;
}

void MemoryPool::expandPool(size_t additionalBlocks)
{
    for (size_t i = 0; i < additionalBlocks; ++i) {
        void* block = std::malloc(m_blockSize);
        if (!block) {
            qCritical() << "MemoryPool: 内存分配失败，块大小:" << m_blockSize;
            break;
        }
        
        m_blocks.append(block);
        m_freeBlocks.append(block);
    }
    
    m_totalBlocks += additionalBlocks;
    m_stats.poolExpansions++;
    
    qDebug() << "MemoryPool: 池扩展" << additionalBlocks 
             << "块，总块数:" << m_totalBlocks;
}

// SmartMemoryManager 实现

SmartMemoryManager& SmartMemoryManager::instance()
{
    static SmartMemoryManager instance;
    return instance;
}

SmartMemoryManager::SmartMemoryManager()
    : m_cleanupTimer(new QTimer())
{
    // 预分配常用大小的内存池
    preallocateBuffers();
    
    // 设置定期清理
    connect(m_cleanupTimer, &QTimer::timeout, this, &SmartMemoryManager::performCleanup);
    m_cleanupTimer->start(30000); // 30秒清理一次
    
    qDebug() << "SmartMemoryManager: 初始化完成";
}

SmartMemoryManager::~SmartMemoryManager()
{
    m_cleanupTimer->stop();
    delete m_cleanupTimer;
    
    qDebug() << "SmartMemoryManager: 已销毁";
}

QByteArray SmartMemoryManager::allocateAudioBuffer(size_t size)
{
    QMutexLocker poolLocker(&m_poolMutex);
    
    MemoryPool* pool = getPoolForSize(size);
    if (pool) {
        void* memory = pool->allocate();
        if (memory) {
            QMutexLocker statsLocker(&m_statsMutex);
            m_stats.totalAllocated += size;
            m_stats.currentUsage += size;
            m_stats.peakUsage = qMax(m_stats.peakUsage, m_stats.currentUsage);
            m_stats.poolHitRate = 
                (m_stats.poolHitRate * 9 + 100) / 10; // 池命中
            
            return QByteArray(static_cast<char*>(memory), static_cast<int>(size));
        }
    }
    
    // 池未命中或分配失败，使用标准分配
    QMutexLocker statsLocker(&m_statsMutex);
    m_stats.totalAllocated += size;
    m_stats.currentUsage += size;
    m_stats.peakUsage = qMax(m_stats.peakUsage, m_stats.currentUsage);
    m_stats.poolHitRate = (m_stats.poolHitRate * 9) / 10; // 池未命中
    
    return QByteArray(static_cast<int>(size), 0);
}

void SmartMemoryManager::deallocateAudioBuffer(QByteArray& buffer)
{
    if (buffer.isEmpty()) {
        return;
    }
    
    size_t size = static_cast<size_t>(buffer.size());
    
    QMutexLocker poolLocker(&m_poolMutex);
    
    MemoryPool* pool = getPoolForSize(size);
    if (pool) {
        pool->deallocate(buffer.data());
    }
    
    QMutexLocker statsLocker(&m_statsMutex);
    m_stats.totalDeallocated += size;
    m_stats.currentUsage -= size;
    
    buffer.clear(); // 清空缓冲区
}

void SmartMemoryManager::preallocateBuffers()
{
    // 预分配常用音频缓冲区大小的内存池
    QVector<size_t> commonSizes = {
        1024,      // 1KB
        4096,      // 4KB
        8192,      // 8KB
        16384,     // 16KB
        32768,     // 32KB
        65536,     // 64KB
        131072,    // 128KB
        262144,    // 256KB
        524288,    // 512KB
        1048576    // 1MB
    };
    
    for (size_t size : commonSizes) {
        m_memoryPools[size] = std::make_unique<MemoryPool>(size, 10);
    }
    
    qDebug() << "SmartMemoryManager: 预分配了" << commonSizes.size() << "个内存池";
}

SmartMemoryManager::MemoryStats SmartMemoryManager::getMemoryStats() const
{
    QMutexLocker locker(&m_statsMutex);
    return m_stats;
}

void SmartMemoryManager::resetMemoryStats()
{
    QMutexLocker locker(&m_statsMutex);
    m_stats = MemoryStats();
    
    qDebug() << "SmartMemoryManager: 内存统计已重置";
}

void SmartMemoryManager::cleanupUnusedBuffers()
{
    QMutexLocker locker(&m_poolMutex);
    
    // 简单的清理策略：重置所有池的统计信息
    for (auto& pool : m_memoryPools) {
        pool.second->resetStats();
    }
}

void SmartMemoryManager::forceGarbageCollection()
{
    // 强制垃圾回收（简化实现）
    cleanupUnusedBuffers();
    
    qDebug() << "SmartMemoryManager: 强制垃圾回收完成";
}

MemoryPool* SmartMemoryManager::getPoolForSize(size_t size)
{
    size_t poolSize = roundUpToPoolSize(size);
    
    auto it = m_memoryPools.find(poolSize);
    if (it != m_memoryPools.end()) {
        return it->second.get();
    }
    
    // 没有找到合适的池，创建一个新的
    if (poolSize <= 1048576) { // 最大1MB的池
        m_memoryPools[poolSize] = std::make_unique<MemoryPool>(poolSize, 5);
        return m_memoryPools[poolSize].get();
    }
    
    return nullptr; // 太大了，不使用池
}

size_t SmartMemoryManager::roundUpToPoolSize(size_t size) const
{
    // 向上取整到2的幂次
    size_t poolSize = 1024; // 最小1KB
    while (poolSize < size && poolSize < 1048576) { // 最大1MB
        poolSize *= 2;
    }
    return poolSize;
}

void SmartMemoryManager::performCleanup()
{
    cleanupUnusedBuffers();
    
    QMutexLocker locker(&m_statsMutex);
    qDebug() << "SmartMemoryManager: 定期清理完成，当前使用:" 
             << m_stats.currentUsage / 1024 << "KB";
}

// ResourceManager 实现

ResourceManager* ResourceManager::s_instance = nullptr;
QMutex ResourceManager::s_instanceMutex;

ResourceManager::ResourceManager(QObject *parent)
    : QObject(parent)
    , m_memoryManager(&SmartMemoryManager::instance())
    , m_monitoringTimer(new QTimer(this))
    , m_isMonitoring(false)
    , m_cleanupTimer(new QTimer(this))
{
    initializeResourcePools();
    
    // 设置监控定时器
    connect(m_monitoringTimer, &QTimer::timeout, this, &ResourceManager::updateResourceStats);
    
    // 设置清理定时器
    connect(m_cleanupTimer, &QTimer::timeout, this, &ResourceManager::onScheduledCleanup);
    
    qDebug() << "ResourceManager: 初始化完成";
}

ResourceManager::~ResourceManager()
{
    stopResourceMonitoring();
    
    qDebug() << "ResourceManager: 已销毁";
}

ResourceManager& ResourceManager::instance()
{
    QMutexLocker locker(&s_instanceMutex);
    if (!s_instance) {
        s_instance = new ResourceManager();
    }
    return *s_instance;
}

bool ResourceManager::requestAudioLock(const QString& lockId, const QString& ownerName, int timeoutMs)
{
    QMutexLocker locker(&m_lockMutex);
    
    auto it = m_locks.find(lockId);
    if (it != m_locks.end()) {
        // 锁已存在，检查是否可以获取
        if (it.value()->tryAcquire(timeoutMs)) {
            emit audioLockAcquired(lockId, ownerName);
            return true;
        } else {
            emit audioLockConflict(lockId, ownerName, it.value()->getOwnerName());
            return false;
        }
    }
    
    // 创建新锁
    auto lock = QSharedPointer<AudioResourceLock>::create(lockId, ownerName);
    if (lock->tryAcquire(timeoutMs)) {
        m_locks[lockId] = lock;
        emit audioLockAcquired(lockId, ownerName);
        return true;
    }
    
    return false;
}

void ResourceManager::releaseAudioLock(const QString& lockId)
{
    QMutexLocker locker(&m_lockMutex);
    
    auto it = m_locks.find(lockId);
    if (it != m_locks.end()) {
        QString ownerName = it.value()->getOwnerName();
        it.value()->release();
        m_locks.erase(it);
        
        emit audioLockReleased(lockId, ownerName);
    }
}

bool ResourceManager::isAudioLocked(const QString& lockId) const
{
    QMutexLocker locker(&m_lockMutex);
    
    auto it = m_locks.find(lockId);
    return it != m_locks.end() && it.value()->isHeld();
}

QStringList ResourceManager::getActiveLocks() const
{
    QMutexLocker locker(&m_lockMutex);
    return m_locks.keys();
}

std::unique_ptr<ScopedAudioLock> ResourceManager::createScopedLock(const QString& lockId, 
                                                                   const QString& ownerName, 
                                                                   int timeoutMs)
{
    auto scopedLock = std::make_unique<ScopedAudioLock>(lockId, ownerName, timeoutMs);
    
    if (*scopedLock) {
        QMutexLocker locker(&m_statsMutex);
        m_stats.activeLocks++;
        return scopedLock;
    }
    
    return nullptr;
}

SmartMemoryManager& ResourceManager::getMemoryManager()
{
    return *m_memoryManager;
}

void ResourceManager::startResourceMonitoring(int intervalMs)
{
    if (m_isMonitoring) {
        return;
    }
    
    m_isMonitoring = true;
    m_monitoringTimer->start(intervalMs);
    
    qDebug() << "ResourceManager: 开始资源监控，间隔:" << intervalMs << "ms";
}

void ResourceManager::stopResourceMonitoring()
{
    if (!m_isMonitoring) {
        return;
    }
    
    m_isMonitoring = false;
    m_monitoringTimer->stop();
    
    qDebug() << "ResourceManager: 停止资源监控";
}

bool ResourceManager::isResourceMonitoringActive() const
{
    return m_isMonitoring;
}

void ResourceManager::performResourceCleanup()
{
    // 清理过期的锁
    cleanupExpiredLocks();
    
    // 清理内存池
    m_memoryManager->cleanupUnusedBuffers();
    
    // 更新统计
    updateMemoryStats();
    
    QMutexLocker locker(&m_statsMutex);
    m_stats.resourceCleanupCount++;
    
    qDebug() << "ResourceManager: 资源清理完成";
}

void ResourceManager::scheduleResourceCleanup(int delayMs)
{
    m_cleanupTimer->start(delayMs);
}

ResourceManager::ResourceStats ResourceManager::getResourceStats() const
{
    QMutexLocker locker(&m_statsMutex);
    return m_stats;
}

void ResourceManager::resetResourceStats()
{
    QMutexLocker locker(&m_statsMutex);
    m_stats = ResourceStats();
    
    qDebug() << "ResourceManager: 资源统计已重置";
}

void ResourceManager::updateResourceStats()
{
    QMutexLocker locker(&m_statsMutex);
    
    // 更新活跃锁数量
    m_stats.activeLocks = m_locks.size();
    
    // 更新内存统计
    updateMemoryStats();
    
    emit resourceStatsUpdated(m_stats);
}

void ResourceManager::onScheduledCleanup()
{
    performResourceCleanup();
    m_cleanupTimer->stop(); // 单次清理
}

void ResourceManager::initializeResourcePools()
{
    // 初始化资源池（如果需要的话）
    qDebug() << "ResourceManager: 资源池初始化完成";
}

void ResourceManager::cleanupExpiredLocks()
{
    QMutexLocker locker(&m_lockMutex);
    
    auto it = m_locks.begin();
    while (it != m_locks.end()) {
        if (!it.value()->isHeld()) {
            it = m_locks.erase(it);
        } else {
            ++it;
        }
    }
}

void ResourceManager::updateMemoryStats()
{
    auto memStats = m_memoryManager->getMemoryStats();
    
    m_stats.totalMemoryUsage = memStats.currentUsage;
    m_stats.peakMemoryUsage = qMax(m_stats.peakMemoryUsage, memStats.currentUsage);
    m_stats.memoryPoolHitRate = memStats.poolHitRate;
    
    // 检查内存使用阈值
    const qint64 memoryThreshold = 512 * 1024 * 1024; // 512MB
    if (memStats.currentUsage > memoryThreshold) {
        emit memoryUsageHigh(memStats.currentUsage, memoryThreshold);
    }
}