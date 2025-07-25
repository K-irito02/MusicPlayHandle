#ifndef OBJECTPOOL_H
#define OBJECTPOOL_H

#include <QObject>
#include <QQueue>
#include <QMutex>
#include <QMutexLocker>
#include <QTimer>
#include <QElapsedTimer>
#include <QDateTime>
#include <QHash>
#include <QDebug>
#include <list>
#include <memory>
#include <functional>
#include <type_traits>

/**
 * @brief 对象池统计信息
 */
struct ObjectPoolStats {
    size_t totalCreated = 0;      // 总创建数量
    size_t totalAcquired = 0;     // 总获取数量
    size_t totalReleased = 0;     // 总释放数量
    size_t currentPoolSize = 0;   // 当前池大小
    size_t currentInUse = 0;      // 当前使用中数量
    size_t maxPoolSize = 0;       // 最大池大小
    int hitRate = 0;           // 命中率（百分比）
    qint64 avgAcquireTime = 0; // 平均获取时间（微秒）
    
    /**
     * @brief 计算命中率
     */
    void calculateHitRate() {
        if (totalAcquired > 0) {
            size_t hits = totalAcquired - totalCreated;
            hitRate = static_cast<int>((hits * 100) / totalAcquired);
        }
    }
};

/**
 * @brief 通用对象池模板类
 * @tparam T 对象类型
 * @details 提供线程安全的对象池实现，支持自动清理和统计
 */
class ObjectPoolBase : public QObject
{
    Q_OBJECT

protected:
    explicit ObjectPoolBase(QObject* parent = nullptr) : QObject(parent) {}

signals:
    void objectsCleaned(int count);
    void poolStatusChanged(int poolSize, int inUse);
};

template<typename T>
class ObjectPool : public ObjectPoolBase
{
    // 移除static_assert，因为我们可以通过工厂函数创建对象

public:
    using ObjectPtr = std::unique_ptr<T>;
    using FactoryFunc = std::function<ObjectPtr()>;
    using ResetFunc = std::function<void(T*)>;
    using ValidateFunc = std::function<bool(const T*)>;
    
    /**
     * @brief 构造函数
     * @param parent 父对象
     * @param maxSize 最大池大小
     * @param initialSize 初始池大小
     */
    explicit ObjectPool(QObject* parent = nullptr, size_t maxSize = 100, size_t initialSize = 10)
        : ObjectPoolBase(parent)
        , m_maxSize(maxSize)
        , m_initialSize(initialSize)
        , m_cleanupTimer(new QTimer(this))
        , m_cleanupInterval(60000) // 1分钟
        , m_maxIdleTime(300000)    // 5分钟
        , m_factory(nullptr) // 工厂函数需要后续设置
        , m_resetFunc(nullptr)
        , m_validateFunc(nullptr)
        , m_enableStats(true)
    {
        // 设置清理定时器
        m_cleanupTimer->setInterval(m_cleanupInterval);
        connect(m_cleanupTimer, &QTimer::timeout, this, QOverload<>::of(&ObjectPool::cleanup));
        m_cleanupTimer->start();
        
        // 预创建对象
        preallocate();
    }
    
    /**
     * @brief 析构函数
     */
    ~ObjectPool() {
        clear();
    }
    
    /**
     * @brief 设置对象工厂函数
     * @param factory 工厂函数
     */
    void setFactory(const FactoryFunc& factory) {
        QMutexLocker locker(&m_mutex);
        m_factory = factory;
    }
    
    /**
     * @brief 设置对象重置函数
     * @param resetFunc 重置函数
     */
    void setResetFunction(const ResetFunc& resetFunc) {
        QMutexLocker locker(&m_mutex);
        m_resetFunc = resetFunc;
    }
    
    /**
     * @brief 设置对象验证函数
     * @param validateFunc 验证函数
     */
    void setValidateFunction(const ValidateFunc& validateFunc) {
        QMutexLocker locker(&m_mutex);
        m_validateFunc = validateFunc;
    }
    
    /**
     * @brief 获取对象
     * @return 对象指针
     */
    ObjectPtr acquire() {
        QElapsedTimer timer;
        if (m_enableStats) {
            timer.start();
        }
        
        QMutexLocker locker(&m_mutex);
        
        ObjectPtr obj;
        
        // 尝试从池中获取
        while (!m_pool.empty()) {
            auto poolItem = std::move(m_pool.front());
            m_pool.pop_front();
            
            // 验证对象是否有效
            if (!m_validateFunc || m_validateFunc(poolItem.object.get())) {
                obj = std::move(poolItem.object);
                break;
            }
            // 无效对象直接丢弃
        }
        
        // 如果池中没有可用对象，创建新对象
        if (!obj) {
            obj = m_factory();
            if (m_enableStats) {
                m_stats.totalCreated++;
            }
        }
        
        // 重置对象状态
        if (obj && m_resetFunc) {
            m_resetFunc(obj.get());
        }
        
        // 更新统计
        if (m_enableStats) {
            m_stats.totalAcquired++;
            m_stats.currentInUse++;
            m_stats.currentPoolSize = m_pool.size();
            
            qint64 elapsed = timer.nsecsElapsed() / 1000; // 转换为微秒
            m_stats.avgAcquireTime = (m_stats.avgAcquireTime + elapsed) / 2;
            
            m_stats.calculateHitRate();
            emit poolStatusChanged(m_stats.currentPoolSize, m_stats.currentInUse);
        }
        
        return obj;
    }
    
    /**
     * @brief 释放对象回池
     * @param obj 要释放的对象
     */
    void release(ObjectPtr obj) {
        if (!obj) {
            return;
        }
        
        QMutexLocker locker(&m_mutex);
        
        // 检查池是否已满
        if (m_pool.size() >= m_maxSize) {
            // 池已满，直接丢弃对象
            if (m_enableStats) {
                m_stats.currentInUse--;
            }
            return;
        }
        
        // 验证对象是否可以回收
        if (m_validateFunc && !m_validateFunc(obj.get())) {
            // 对象无效，直接丢弃
            if (m_enableStats) {
                m_stats.currentInUse--;
            }
            return;
        }
        
        // 将对象放回池中
        PoolItem item;
        item.object = std::move(obj);
        item.timestamp = QDateTime::currentMSecsSinceEpoch();
        
        m_pool.push_back(std::move(item));
        
        // 更新统计
        if (m_enableStats) {
            m_stats.totalReleased++;
            m_stats.currentInUse--;
            m_stats.currentPoolSize = m_pool.size();
            m_stats.maxPoolSize = qMax(m_stats.maxPoolSize, m_stats.currentPoolSize);
            emit poolStatusChanged(m_stats.currentPoolSize, m_stats.currentInUse);
        }
    }
    
    /**
     * @brief 获取池大小
     * @return 当前池中对象数量
     */
    size_t size() const {
        QMutexLocker locker(&m_mutex);
        return m_pool.size();
    }
    
    /**
     * @brief 获取使用中的对象数量
     * @return 使用中的对象数量
     */
    int inUseCount() const {
        QMutexLocker locker(&m_mutex);
        return m_stats.currentInUse;
    }
    
    /**
     * @brief 检查池是否为空
     * @return 是否为空
     */
    bool isEmpty() const {
        QMutexLocker locker(&m_mutex);
        return m_pool.empty();
    }
    
    /**
     * @brief 清空池
     */
    void clear() {
        QMutexLocker locker(&m_mutex);
        m_pool.clear();
        
        if (m_enableStats) {
            m_stats.currentPoolSize = 0;
            emit poolStatusChanged(m_stats.currentPoolSize, m_stats.currentInUse);
        }
    }
    
    /**
     * @brief 预分配对象
     * @param count 预分配数量（默认使用初始大小）
     */
    void preallocate(size_t count = 0) {
        if (count == 0) {
            count = m_initialSize;
        }
        
        QMutexLocker locker(&m_mutex);
        
        for (size_t i = 0; i < count && m_pool.size() < m_maxSize; ++i) {
            PoolItem item;
            item.object = m_factory();
            item.timestamp = QDateTime::currentMSecsSinceEpoch();
            
            m_pool.push_back(std::move(item));
            
            if (m_enableStats) {
                m_stats.totalCreated++;
            }
        }
        
        if (m_enableStats) {
            m_stats.currentPoolSize = m_pool.size();
            m_stats.maxPoolSize = qMax(m_stats.maxPoolSize, m_stats.currentPoolSize);
            emit poolStatusChanged(m_stats.currentPoolSize, m_stats.currentInUse);
        }
    }
    
    /**
     * @brief 获取统计信息
     * @return 统计信息
     */
    ObjectPoolStats getStatistics() const {
        QMutexLocker locker(&m_mutex);
        return m_stats;
    }
    
    /**
     * @brief 重置统计信息
     */
    void resetStatistics() {
        QMutexLocker locker(&m_mutex);
        m_stats = ObjectPoolStats();
        m_stats.currentPoolSize = m_pool.size();
        m_stats.currentInUse = m_stats.totalAcquired - m_stats.totalReleased;
        emit poolStatusChanged(m_stats.currentPoolSize, m_stats.currentInUse);
    }
    
    /**
     * @brief 启用/禁用统计
     * @param enabled 是否启用
     */
    void setStatisticsEnabled(bool enabled) {
        QMutexLocker locker(&m_mutex);
        m_enableStats = enabled;
    }
    
    /**
     * @brief 设置清理间隔
     * @param interval 间隔时间（毫秒）
     */
    void setCleanupInterval(int interval) {
        m_cleanupInterval = interval;
        m_cleanupTimer->setInterval(interval);
    }
    
    /**
     * @brief 设置最大空闲时间
     * @param maxIdleTime 最大空闲时间（毫秒）
     */
    void setMaxIdleTime(qint64 maxIdleTime) {
        QMutexLocker locker(&m_mutex);
        m_maxIdleTime = maxIdleTime;
    }
    
    /**
     * @brief 设置最大池大小
     * @param maxSize 最大大小
     */
    void setMaxSize(size_t maxSize) {
        QMutexLocker locker(&m_mutex);
        m_maxSize = maxSize;
        
        // 如果当前池大小超过新的最大值，清理多余对象
        while (m_pool.size() > m_maxSize) {
            m_pool.pop_front();
        }
        
        if (m_enableStats) {
            m_stats.currentPoolSize = m_pool.size();
            emit poolStatusChanged(m_stats.currentPoolSize, m_stats.currentInUse);
        }
    }
    
public:
    /**
     * @brief 清理过期对象
     */
    void cleanup() {
        QMutexLocker locker(&m_mutex);
        
        qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
        int cleanedCount = 0;
        
        // 清理过期对象
        while (!m_pool.empty()) {
            const auto& front = m_pool.front();
            if (currentTime - front.timestamp > m_maxIdleTime) {
                m_pool.pop_front();
                cleanedCount++;
            } else {
                break; // 队列是按时间排序的，后面的对象更新
            }
        }
        
        if (cleanedCount > 0) {
            if (m_enableStats) {
                m_stats.currentPoolSize = m_pool.size();
                emit poolStatusChanged(m_stats.currentPoolSize, m_stats.currentInUse);
            }
            
            qDebug() << QString("ObjectPool cleaned up") << cleanedCount << QString("expired objects");
            emit ObjectPoolBase::objectsCleaned(cleanedCount);
        }
    }
    
// 信号已移至基类 ObjectPoolBase
    
private:
    /**
     * @brief 池中的对象项
     */
    struct PoolItem {
        ObjectPtr object;
        qint64 timestamp; // 创建或最后使用时间
        
        // 禁用拷贝构造和赋值
        PoolItem() = default;
        PoolItem(PoolItem&&) = default;
        PoolItem& operator=(PoolItem&&) = default;
        PoolItem(const PoolItem&) = delete;
        PoolItem& operator=(const PoolItem&) = delete;
    };
    
    mutable QMutex m_mutex;
    std::list<PoolItem> m_pool; // 使用std::list避免拷贝问题
    
    size_t m_maxSize;
    size_t m_initialSize;
    
    QTimer* m_cleanupTimer;
    int m_cleanupInterval;
    qint64 m_maxIdleTime;
    
    FactoryFunc m_factory;
    ResetFunc m_resetFunc;
    ValidateFunc m_validateFunc;
    
    bool m_enableStats;
    ObjectPoolStats m_stats;
};

// ========== 特化实现 ==========

/**
 * @brief TagListItem对象池
 */
class TagListItem; // 前置声明
using TagListItemPool = ObjectPool<TagListItem>;

/**
 * @brief QWidget对象池
 */
class QWidget;
using WidgetPool = ObjectPool<QWidget>;

/**
 * @brief 字符串对象池
 */
using StringPool = ObjectPool<QString>;

/**
 * @brief 字节数组对象池
 */
using ByteArrayPool = ObjectPool<QByteArray>;

/**
 * @brief 对象池管理器
 * @details 管理多个对象池实例
 */
class ObjectPoolManager : public QObject
{
    Q_OBJECT
    
public:
    explicit ObjectPoolManager(QObject* parent = nullptr);
    ~ObjectPoolManager();
    
    // 单例模式
    static ObjectPoolManager* instance();
    static void cleanup();
    
    /**
     * @brief 注册对象池
     * @param name 池名称
     * @param pool 池对象
     */
    template<typename T>
    void registerPool(const QString& name, std::shared_ptr<ObjectPool<T>> pool) {
        QMutexLocker locker(&m_mutex);
        m_pools[name] = pool;
    }
    
    /**
     * @brief 获取对象池
     * @param name 池名称
     * @return 池对象
     */
    template<typename T>
    std::shared_ptr<ObjectPool<T>> getPool(const QString& name) {
        QMutexLocker locker(&m_mutex);
        auto it = m_pools.find(name);
        if (it != m_pools.end()) {
            return std::static_pointer_cast<ObjectPool<T>>(it.value());
        }
        return nullptr;
    }
    
    /**
     * @brief 获取所有池的统计信息
     * @return 统计信息映射
     */
    QHash<QString, ObjectPoolStats> getAllStatistics() const;
    
    /**
     * @brief 清理所有池
     */
    void cleanupAllPools();
    
    /**
     * @brief 重置所有统计
     */
    void resetAllStatistics();
    
    /**
     * @brief 设置默认对象池
     */
    void setupDefaultPools();
    
public slots:
    /**
     * @brief 定期清理
     */
    void performMaintenance();
    
signals:
    /**
     * @brief 维护完成信号
     * @param poolCount 池数量
     * @param totalCleaned 总清理数量
     */
    void maintenanceCompleted(int poolCount, int totalCleaned);
    
private:
    static ObjectPoolManager* s_instance;
    mutable QMutex m_mutex;
    
    QHash<QString, std::shared_ptr<void>> m_pools;
    QTimer* m_maintenanceTimer;
};

#endif // OBJECTPOOL_H