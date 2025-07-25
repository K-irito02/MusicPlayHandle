#ifndef CACHE_H
#define CACHE_H

#include <QHash>
#include <QTimer>
#include <QDateTime>
#include <QMutex>
#include <QMutexLocker>
#include <QDebug>
#include "constants.h"

/**
 * @brief 缓存项结构
 * @tparam T 缓存数据类型
 */
template<typename T>
struct CacheItem {
    T data;
    QDateTime timestamp;
    QDateTime lastAccessed;
    int accessCount;
    
    CacheItem() : accessCount(0) {}
    CacheItem(const T& d) : data(d), timestamp(QDateTime::currentDateTime()), 
                           lastAccessed(QDateTime::currentDateTime()), accessCount(1) {}
};

/**
 * @brief 线程安全的LRU缓存实现
 * @tparam K 键类型
 * @tparam V 值类型
 */
template<typename K, typename V>
class Cache : public QObject
{
public:
    explicit Cache(int maxSize = Constants::Performance::CACHE_SIZE_LIMIT, 
                  int cleanupIntervalMs = Constants::Performance::CLEANUP_INTERVAL_MS,
                  QObject* parent = nullptr)
        : QObject(parent), m_maxSize(maxSize), m_cleanupInterval(cleanupIntervalMs)
    {
        // 设置定期清理定时器
        m_cleanupTimer = new QTimer(this);
        connect(m_cleanupTimer, &QTimer::timeout, this, &Cache::cleanup);
        m_cleanupTimer->start(m_cleanupInterval);
    }
    
    /**
     * @brief 插入或更新缓存项
     * @param key 键
     * @param value 值
     */
    void put(const K& key, const V& value)
    {
        QMutexLocker locker(&m_mutex);
        
        // 如果已存在，更新数据
        if (m_cache.contains(key)) {
            m_cache[key].data = value;
            m_cache[key].timestamp = QDateTime::currentDateTime();
            m_cache[key].lastAccessed = QDateTime::currentDateTime();
            m_cache[key].accessCount++;
            return;
        }
        
        // 如果缓存已满，移除最少使用的项
        if (m_cache.size() >= m_maxSize) {
            removeLeastRecentlyUsed();
        }
        
        // 插入新项
        m_cache[key] = CacheItem<V>(value);
        m_insertionOrder.append(key);
        
        qDebug() << QString("Cache: Added item with key") << key << QString(", cache size:") << m_cache.size();
    }
    
    /**
     * @brief 获取缓存项
     * @param key 键
     * @return 值的可选对象
     */
    std::optional<V> get(const K& key)
    {
        QMutexLocker locker(&m_mutex);
        
        auto it = m_cache.find(key);
        if (it != m_cache.end()) {
            // 更新访问信息
            it->lastAccessed = QDateTime::currentDateTime();
            it->accessCount++;
            
            // 更新LRU顺序
            m_insertionOrder.removeOne(key);
            m_insertionOrder.append(key);
            
            return it->data;
        }
        
        return std::nullopt;
    }
    
    /**
     * @brief 检查键是否存在
     * @param key 键
     * @return 是否存在
     */
    bool contains(const K& key) const
    {
        QMutexLocker locker(&m_mutex);
        return m_cache.contains(key);
    }
    
    /**
     * @brief 移除缓存项
     * @param key 键
     * @return 是否移除成功
     */
    bool remove(const K& key)
    {
        QMutexLocker locker(&m_mutex);
        
        if (m_cache.remove(key) > 0) {
            m_insertionOrder.removeOne(key);
            qDebug() << QString("Cache: Removed item with key") << key;
            return true;
        }
        return false;
    }
    
    /**
     * @brief 清空缓存
     */
    void clear()
    {
        QMutexLocker locker(&m_mutex);
        m_cache.clear();
        m_insertionOrder.clear();
        qDebug() << QString("Cache: Cleared all items");
    }
    
    /**
     * @brief 获取缓存大小
     * @return 缓存项数量
     */
    int size() const
    {
        QMutexLocker locker(&m_mutex);
        return m_cache.size();
    }
    
    /**
     * @brief 获取缓存命中率
     * @return 命中率（0.0-1.0）
     */
    double hitRate() const
    {
        QMutexLocker locker(&m_mutex);
        if (m_totalRequests == 0) return 0.0;
        return static_cast<double>(m_hits) / m_totalRequests;
    }
    
    /**
     * @brief 获取缓存统计信息
     * @return 统计信息字符串
     */
    QString getStatistics() const
    {
        QMutexLocker locker(&m_mutex);
        return QString("Cache Statistics: Size=%1/%2, Hits=%3, Requests=%4, Hit Rate=%.2f%%")
               .arg(m_cache.size())
               .arg(m_maxSize)
               .arg(m_hits)
               .arg(m_totalRequests)
               .arg(hitRate() * 100);
    }
    
    /**
     * @brief 设置最大缓存大小
     * @param maxSize 最大大小
     */
    void setMaxSize(int maxSize)
    {
        QMutexLocker locker(&m_mutex);
        m_maxSize = maxSize;
        
        // 如果当前大小超过新的最大值，清理多余项
        while (m_cache.size() > m_maxSize) {
            removeLeastRecentlyUsed();
        }
    }
    
    /**
     * @brief 获取所有键
     * @return 键列表
     */
    QList<K> keys() const
    {
        QMutexLocker locker(&m_mutex);
        return m_cache.keys();
    }
    
public slots:
    /**
     * @brief 清理过期和少用的缓存项
     */
    void cleanup()
    {
        QMutexLocker locker(&m_mutex);
        
        const QDateTime now = QDateTime::currentDateTime();
        const qint64 maxAge = 3600; // 1小时
        
        QList<K> toRemove;
        
        // 查找过期项
        for (auto it = m_cache.begin(); it != m_cache.end(); ++it) {
            if (it->timestamp.secsTo(now) > maxAge) {
                toRemove.append(it.key());
            }
        }
        
        // 移除过期项
        for (const K& key : toRemove) {
            m_cache.remove(key);
            m_insertionOrder.removeOne(key);
        }
        
        if (!toRemove.isEmpty()) {
            qDebug() << QString("Cache: Cleaned up") << toRemove.size() << QString("expired items");
        }
    }
    
private:
    /**
     * @brief 移除最少使用的项
     */
    void removeLeastRecentlyUsed()
    {
        if (m_insertionOrder.isEmpty()) return;
        
        // 找到访问次数最少且最久未访问的项
        K lruKey = m_insertionOrder.first();
        int minAccessCount = m_cache[lruKey].accessCount;
        QDateTime oldestAccess = m_cache[lruKey].lastAccessed;
        
        for (const K& key : m_insertionOrder) {
            const auto& item = m_cache[key];
            if (item.accessCount < minAccessCount || 
                (item.accessCount == minAccessCount && item.lastAccessed < oldestAccess)) {
                lruKey = key;
                minAccessCount = item.accessCount;
                oldestAccess = item.lastAccessed;
            }
        }
        
        m_cache.remove(lruKey);
        m_insertionOrder.removeOne(lruKey);
        
        qDebug() << QString("Cache: Removed LRU item with key") << lruKey;
    }
    
private:
    mutable QMutex m_mutex;
    QHash<K, CacheItem<V>> m_cache;
    QList<K> m_insertionOrder; // 用于LRU跟踪
    int m_maxSize;
    int m_cleanupInterval;
    QTimer* m_cleanupTimer;
    
    // 统计信息
    mutable int m_hits = 0;
    mutable int m_totalRequests = 0;
};

// 便利类型别名
using TagCache = Cache<int, class Tag>;
using StringCache = Cache<QString, QString>;
using IntCache = Cache<int, int>;

#endif // CACHE_H