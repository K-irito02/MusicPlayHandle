#ifndef LAZYLOADER_H
#define LAZYLOADER_H

#include <QObject>
#include <QMutex>
#include <QMutexLocker>
#include <QList>
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrent>
#include <functional>
#include <memory>
#include "constants.h"

/**
 * @brief 延迟加载器基类
 * @tparam T 加载的数据类型
 */
template<typename T>
class LazyLoader : public QObject
{
public:
    explicit LazyLoader(QObject* parent = nullptr)
        : QObject(parent), m_loaded(false), m_loading(false)
    {
    }
    
    virtual ~LazyLoader() = default;
    
    /**
     * @brief 获取数据（如果未加载则触发加载）
     * @return 数据的常量引用
     */
    const QList<T>& getData()
    {
        QMutexLocker locker(&m_mutex);
        
        if (!m_loaded && !m_loading) {
            loadData();
        }
        
        return m_data;
    }
    
    /**
     * @brief 异步获取数据
     * @param callback 数据加载完成后的回调函数
     */
    void getDataAsync(std::function<void(const QList<T>&)> callback)
    {
        QMutexLocker locker(&m_mutex);
        
        if (m_loaded) {
            // 数据已加载，直接调用回调
            callback(m_data);
            return;
        }
        
        // 添加回调到队列
        m_callbacks.append(callback);
        
        if (!m_loading) {
            loadDataAsync();
        }
    }
    
    /**
     * @brief 检查数据是否已加载
     * @return 是否已加载
     */
    bool isLoaded() const
    {
        QMutexLocker locker(&m_mutex);
        return m_loaded;
    }
    
    /**
     * @brief 检查是否正在加载
     * @return 是否正在加载
     */
    bool isLoading() const
    {
        QMutexLocker locker(&m_mutex);
        return m_loading;
    }
    
    /**
     * @brief 强制重新加载数据
     */
    void reload()
    {
        QMutexLocker locker(&m_mutex);
        m_loaded = false;
        m_data.clear();
        loadData();
    }
    
    /**
     * @brief 异步重新加载数据
     * @param callback 加载完成后的回调函数
     */
    void reloadAsync(std::function<void(const QList<T>&)> callback = nullptr)
    {
        QMutexLocker locker(&m_mutex);
        m_loaded = false;
        m_data.clear();
        
        if (callback) {
            m_callbacks.append(callback);
        }
        
        loadDataAsync();
    }
    
    /**
     * @brief 清空数据并标记为未加载
     */
    void clear()
    {
        QMutexLocker locker(&m_mutex);
        m_loaded = false;
        m_loading = false;
        m_data.clear();
        m_callbacks.clear();
    }
    
    /**
     * @brief 获取数据数量
     * @return 数据数量
     */
    int count() const
    {
        QMutexLocker locker(&m_mutex);
        return m_loaded ? m_data.size() : 0;
    }
    
    /**
     * @brief 预加载数据（不阻塞）
     */
    void preload()
    {
        QMutexLocker locker(&m_mutex);
        if (!m_loaded && !m_loading) {
            loadDataAsync();
        }
    }
    
protected:
    /**
     * @brief 纯虚函数：子类实现具体的数据加载逻辑
     * @return 加载的数据列表
     */
    virtual QList<T> doLoadData() = 0;
    
    /**
     * @brief 同步加载数据
     */
    void loadData()
    {
        if (m_loading) return;
        
        m_loading = true;
        
        try {
            m_data = doLoadData();
            m_loaded = true;
            emit dataLoaded(m_data);
        } catch (const std::exception& e) {
            qWarning() << "LazyLoader: Failed to load data:" << e.what();
            emit loadError(QString::fromStdString(e.what()));
        }
        
        m_loading = false;
    }
    
    /**
     * @brief 异步加载数据
     */
    void loadDataAsync()
    {
        if (m_loading) return;
        
        m_loading = true;
        
        // 创建异步任务
        auto future = QtConcurrent::run([this]() {
            return doLoadData();
        });
        
        // 监听任务完成
        auto watcher = new QFutureWatcher<QList<T>>(this);
        connect(watcher, &QFutureWatcher<QList<T>>::finished, [this, watcher]() {
            QMutexLocker locker(&m_mutex);
            
            try {
                m_data = watcher->result();
                m_loaded = true;
                
                // 调用所有回调
                for (const auto& callback : m_callbacks) {
                    callback(m_data);
                }
                m_callbacks.clear();
                
                emit dataLoaded(m_data);
            } catch (const std::exception& e) {
                qWarning() << "LazyLoader: Async load failed:" << e.what();
                emit loadError(QString::fromStdString(e.what()));
            }
            
            m_loading = false;
            watcher->deleteLater();
        });
        
        watcher->setFuture(future);
    }
    
signals:
    /**
     * @brief 数据加载完成信号
     * @param data 加载的数据
     */
    void dataLoaded(const QList<T>& data);
    
    /**
     * @brief 数据加载错误信号
     * @param error 错误信息
     */
    void loadError(const QString& error);
    
private:
    mutable QMutex m_mutex;
    QList<T> m_data;
    bool m_loaded;
    bool m_loading;
    QList<std::function<void(const QList<T>&)>> m_callbacks;
};

/**
 * @brief 延迟标签列表加载器
 */
class LazyTagList : public LazyLoader<class Tag>
{
    Q_OBJECT
    
public:
    explicit LazyTagList(QObject* parent = nullptr);
    
    /**
     * @brief 设置过滤条件
     * @param systemOnly 是否只加载系统标签
     * @param userOnly 是否只加载用户标签
     */
    void setFilter(bool systemOnly = false, bool userOnly = false);
    
protected:
    QList<Tag> doLoadData() override;
    
private:
    bool m_systemOnly;
    bool m_userOnly;
};

/**
 * @brief 延迟歌曲列表加载器
 */
class LazySongList : public LazyLoader<class Song>
{
    Q_OBJECT
    
public:
    explicit LazySongList(int tagId = -1, QObject* parent = nullptr);
    
    /**
     * @brief 设置标签ID过滤
     * @param tagId 标签ID，-1表示加载所有歌曲
     */
    void setTagFilter(int tagId);
    
protected:
    QList<Song> doLoadData() override;
    
private:
    int m_tagId;
};

#endif // LAZYLOADER_H