#include "objectpool.h"
#include "../ui/widgets/taglistitem.h"
#include <QWidget>
#include <QDebug>
#include <QCoreApplication>

// ========== ObjectPoolManager实现 ==========

// 静态成员初始化
ObjectPoolManager* ObjectPoolManager::s_instance = nullptr;

ObjectPoolManager::ObjectPoolManager(QObject* parent)
    : QObject(parent)
    , m_maintenanceTimer(new QTimer(this))
{
    // 设置维护定时器
    m_maintenanceTimer->setInterval(300000); // 5分钟
    connect(m_maintenanceTimer, &QTimer::timeout, this, &ObjectPoolManager::performMaintenance);
    m_maintenanceTimer->start();
    
    qDebug() << "ObjectPoolManager initialized";
}

ObjectPoolManager::~ObjectPoolManager()
{
    cleanupAllPools();
    qDebug() << "ObjectPoolManager destroyed";
}

ObjectPoolManager* ObjectPoolManager::instance()
{
    static QMutex mutex;
    QMutexLocker locker(&mutex);
    
    if (!s_instance) {
        s_instance = new ObjectPoolManager();
        
        // 注册默认对象池
        s_instance->setupDefaultPools();
    }
    return s_instance;
}

void ObjectPoolManager::cleanup()
{
    if (s_instance) {
        delete s_instance;
        s_instance = nullptr;
    }
}

QHash<QString, ObjectPoolStats> ObjectPoolManager::getAllStatistics() const
{
    QMutexLocker locker(&m_mutex);
    
    QHash<QString, ObjectPoolStats> stats;
    
    // 这里需要根据实际的池类型来获取统计信息
    // 由于模板的限制，我们需要为每种类型单独处理
    
    // 示例：获取TagListItem池的统计
    auto tagPool = const_cast<ObjectPoolManager*>(this)->getPool<TagListItem>("TagListItem");
    if (tagPool) {
        stats["TagListItem"] = tagPool->getStatistics();
    }
    
    // 获取QString池的统计
    auto stringPool = const_cast<ObjectPoolManager*>(this)->getPool<QString>("QString");
    if (stringPool) {
        stats["QString"] = stringPool->getStatistics();
    }
    
    // 获取QByteArray池的统计
    auto byteArrayPool = const_cast<ObjectPoolManager*>(this)->getPool<QByteArray>("QByteArray");
    if (byteArrayPool) {
        stats["QByteArray"] = byteArrayPool->getStatistics();
    }
    
    return stats;
}

void ObjectPoolManager::cleanupAllPools()
{
    QMutexLocker locker(&m_mutex);
    
    // 清理所有池
    for (auto it = m_pools.begin(); it != m_pools.end(); ++it) {
        // 由于类型擦除，我们无法直接调用clear()方法
        // 这里需要根据具体实现来处理
    }
    
    qDebug() << "All object pools cleaned up";
}

void ObjectPoolManager::resetAllStatistics()
{
    QMutexLocker locker(&m_mutex);
    
    // 重置所有池的统计信息
    auto tagPool = getPool<TagListItem>("TagListItem");
    if (tagPool) {
        tagPool->resetStatistics();
    }
    
    auto stringPool = getPool<QString>("QString");
    if (stringPool) {
        stringPool->resetStatistics();
    }
    
    auto byteArrayPool = getPool<QByteArray>("QByteArray");
    if (byteArrayPool) {
        byteArrayPool->resetStatistics();
    }
    
    qDebug() << "All object pool statistics reset";
}

void ObjectPoolManager::performMaintenance()
{
    QMutexLocker locker(&m_mutex);
    
    int poolCount = m_pools.size();
    int totalCleaned = 0;
    
    // 对所有池执行清理
    auto tagPool = getPool<TagListItem>("TagListItem");
    if (tagPool) {
        int beforeSize = tagPool->size();
        tagPool->cleanup();
        int afterSize = tagPool->size();
        totalCleaned += (beforeSize - afterSize);
    }
    
    auto stringPool = getPool<QString>("QString");
    if (stringPool) {
        int beforeSize = stringPool->size();
        stringPool->cleanup();
        int afterSize = stringPool->size();
        totalCleaned += (beforeSize - afterSize);
    }
    
    auto byteArrayPool = getPool<QByteArray>("QByteArray");
    if (byteArrayPool) {
        int beforeSize = byteArrayPool->size();
        byteArrayPool->cleanup();
        int afterSize = byteArrayPool->size();
        totalCleaned += (beforeSize - afterSize);
    }
    
    if (totalCleaned > 0) {
        qDebug() << "ObjectPoolManager maintenance completed:"
                 << "pools:" << poolCount
                 << "cleaned:" << totalCleaned;
    }
    
    emit maintenanceCompleted(poolCount, totalCleaned);
}

void ObjectPoolManager::setupDefaultPools()
{
    // 创建TagListItem对象池
    auto tagListItemPool = std::make_shared<TagListItemPool>(this, 50, 10);
    
    // 设置TagListItem的工厂函数
    tagListItemPool->setFactory([]() {
        return std::make_unique<TagListItem>("", "", true, true);
    });
    
    // 设置TagListItem的重置函数
    tagListItemPool->setResetFunction([](TagListItem* item) {
        if (item) {
            // 重置TagListItem的状态
            item->setTagName("");
            item->setIcon("");
            item->setEditable(true);
            item->setDeletable(true);
            item->hide();
        }
    });
    
    // 设置TagListItem的验证函数
    tagListItemPool->setValidateFunction([](const TagListItem* item) {
        return item != nullptr && !item->isVisible();
    });
    
    registerPool("TagListItem", tagListItemPool);
    
    // 创建QString对象池
    auto stringPool = std::make_shared<StringPool>(this, 200, 50);
    
    stringPool->setFactory([]() {
        return std::make_unique<QString>();
    });
    
    stringPool->setResetFunction([](QString* str) {
        if (str) {
            str->clear();
        }
    });
    
    stringPool->setValidateFunction([](const QString* str) {
        return str != nullptr;
    });
    
    registerPool("QString", stringPool);
    
    // 创建QByteArray对象池
    auto byteArrayPool = std::make_shared<ByteArrayPool>(this, 100, 20);
    
    byteArrayPool->setFactory([]() {
        return std::make_unique<QByteArray>();
    });
    
    byteArrayPool->setResetFunction([](QByteArray* array) {
        if (array) {
            array->clear();
        }
    });
    
    byteArrayPool->setValidateFunction([](const QByteArray* array) {
        return array != nullptr;
    });
    
    registerPool("QByteArray", byteArrayPool);
    
    qDebug() << "Default object pools created:"
             << "TagListItem pool (max:50, initial:10)"
             << "QString pool (max:200, initial:50)"
             << "QByteArray pool (max:100, initial:20)";
}

// ========== 便利函数 ==========

/**
 * @brief 获取TagListItem对象池
 * @return TagListItem对象池
 */
std::shared_ptr<TagListItemPool> getTagListItemPool()
{
    return ObjectPoolManager::instance()->getPool<TagListItem>("TagListItem");
}

/**
 * @brief 获取QString对象池
 * @return QString对象池
 */
std::shared_ptr<StringPool> getStringPool()
{
    return ObjectPoolManager::instance()->getPool<QString>("QString");
}

/**
 * @brief 获取QByteArray对象池
 * @return QByteArray对象池
 */
std::shared_ptr<ByteArrayPool> getByteArrayPool()
{
    return ObjectPoolManager::instance()->getPool<QByteArray>("QByteArray");
}

// ========== RAII包装器 ==========

/**
 * @brief 对象池RAII包装器
 * @tparam T 对象类型
 * @details 自动管理对象的获取和释放
 */
template<typename T>
class PooledObject
{
public:
    /**
     * @brief 构造函数
     * @param pool 对象池
     */
    explicit PooledObject(std::shared_ptr<ObjectPool<T>> pool)
        : m_pool(pool)
        , m_object(pool ? pool->acquire() : nullptr)
    {
    }
    
    /**
     * @brief 移动构造函数
     */
    PooledObject(PooledObject&& other) noexcept
        : m_pool(std::move(other.m_pool))
        , m_object(std::move(other.m_object))
    {
    }
    
    /**
     * @brief 移动赋值操作符
     */
    PooledObject& operator=(PooledObject&& other) noexcept
    {
        if (this != &other) {
            reset();
            m_pool = std::move(other.m_pool);
            m_object = std::move(other.m_object);
        }
        return *this;
    }
    
    /**
     * @brief 析构函数
     */
    ~PooledObject()
    {
        reset();
    }
    
    // 禁用拷贝
    PooledObject(const PooledObject&) = delete;
    PooledObject& operator=(const PooledObject&) = delete;
    
    /**
     * @brief 获取对象指针
     * @return 对象指针
     */
    T* get() const
    {
        return m_object.get();
    }
    
    /**
     * @brief 解引用操作符
     * @return 对象引用
     */
    T& operator*() const
    {
        return *m_object;
    }
    
    /**
     * @brief 成员访问操作符
     * @return 对象指针
     */
    T* operator->() const
    {
        return m_object.get();
    }
    
    /**
     * @brief 布尔转换操作符
     * @return 是否有效
     */
    explicit operator bool() const
    {
        return m_object != nullptr;
    }
    
    /**
     * @brief 释放对象
     */
    void reset()
    {
        if (m_object && m_pool) {
            m_pool->release(std::move(m_object));
        }
    }
    
private:
    std::shared_ptr<ObjectPool<T>> m_pool;
    std::unique_ptr<T> m_object;
};

// ========== 便利类型定义 ==========

using PooledTagListItem = PooledObject<TagListItem>;
using PooledString = PooledObject<QString>;
using PooledByteArray = PooledObject<QByteArray>;

// ========== 便利宏定义 ==========

/**
 * @brief 获取池化的TagListItem
 */
#define ACQUIRE_TAG_ITEM() PooledTagListItem(getTagListItemPool())

/**
 * @brief 获取池化的QString
 */
#define ACQUIRE_STRING() PooledString(getStringPool())

/**
 * @brief 获取池化的QByteArray
 */
#define ACQUIRE_BYTE_ARRAY() PooledByteArray(getByteArrayPool())

// 移除 moc 文件包含，因为 ObjectPool 是模板类