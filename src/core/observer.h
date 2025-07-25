#ifndef OBSERVER_H
#define OBSERVER_H

#include <QObject>
#include <QSet>
#include <QMutex>
#include <QSharedPointer>
#include <QWeakPointer>
#include <functional>
#include <memory>

/**
 * @brief 观察者模式基础接口
 * 用于替代AudioEngine的单例模式，实现松耦合架构
 */
template<typename EventType>
class IObserver {
public:
    virtual ~IObserver() = default;
    virtual void onNotify(const EventType& event) = 0;
    virtual QString getObserverName() const { return "UnknownObserver"; }
};

/**
 * @brief 主题/被观察者基础类
 * 管理观察者的注册、注销和通知
 */
template<typename EventType>
class Subject {
public:
    using ObserverPtr = QSharedPointer<IObserver<EventType>>;
    using WeakObserverPtr = QWeakPointer<IObserver<EventType>>;
    
    Subject() = default;
    virtual ~Subject() = default;
    
    // 禁用拷贝构造和赋值
    Subject(const Subject&) = delete;
    Subject& operator=(const Subject&) = delete;
    
    /**
     * @brief 添加观察者
     * @param observer 观察者智能指针
     * @return 是否添加成功
     */
    bool addObserver(ObserverPtr observer) {
        if (!observer) {
            return false;
        }
        
        QMutexLocker locker(&m_mutex);
        
        // 检查是否已存在
        auto it = std::find_if(m_observers.begin(), m_observers.end(),
            [&observer](const WeakObserverPtr& weak) {
                if (auto locked = weak.lock()) {
                    return locked.get() == observer.get();
                }
                return false;
            });
            
        if (it != m_observers.end()) {
            return false; // 已存在
        }
        
        m_observers.append(observer.toWeakRef());
        cleanupInvalidObservers();
        
        qDebug() << "Subject: 添加观察者" << observer->getObserverName() 
                 << "，当前观察者数量:" << m_observers.size();
        return true;
    }
    
    /**
     * @brief 移除观察者
     * @param observer 观察者智能指针
     * @return 是否移除成功
     */
    bool removeObserver(ObserverPtr observer) {
        if (!observer) {
            return false;
        }
        
        QMutexLocker locker(&m_mutex);
        
                 auto it = std::remove_if(m_observers.begin(), m_observers.end(),
             [&observer](const WeakObserverPtr& weak) {
                if (auto locked = weak.lock()) {
                    return locked.get() == observer.get();
                }
                return true; // 移除失效的弱指针
            });
            
        bool removed = (it != m_observers.end());
        m_observers.erase(it, m_observers.end());
        
        if (removed) {
            qDebug() << "Subject: 移除观察者" << observer->getObserverName()
                     << "，当前观察者数量:" << m_observers.size();
        }
        
        return removed;
    }
    
    /**
     * @brief 通知所有观察者
     * @param event 事件数据
     */
    void notifyObservers(const EventType& event) {
        QMutexLocker locker(&m_mutex);
        
        // 先清理失效的观察者
        cleanupInvalidObservers();
        
        // 复制观察者列表以避免在通知过程中修改
        QList<ObserverPtr> validObservers;
        for (const auto& weak : m_observers) {
            if (auto locked = weak.lock()) {
                validObservers.append(locked);
            }
        }
        
        locker.unlock();
        
        // 异步通知观察者，避免阻塞
        for (const auto& observer : validObservers) {
            try {
                // 使用Qt的事件系统进行异步通知
                QMetaObject::invokeMethod(observer.get(), [observer, event]() {
                    try {
                        observer->onNotify(event);
                    } catch (const std::exception& e) {
                        qWarning() << "Observer notification error:" << e.what();
                    } catch (...) {
                        qWarning() << "Unknown observer notification error";
                    }
                }, Qt::QueuedConnection);
            } catch (const std::exception& e) {
                qWarning() << "Failed to notify observer:" << e.what();
            }
        }
    }
    
    /**
     * @brief 获取观察者数量
     */
    int getObserverCount() const {
        QMutexLocker locker(&m_mutex);
        return m_observers.size();
    }
    
    /**
     * @brief 清理所有观察者
     */
    void clearObservers() {
        QMutexLocker locker(&m_mutex);
        m_observers.clear();
        qDebug() << "Subject: 清理所有观察者";
    }

protected:
    /**
     * @brief 清理失效的观察者
     */
    void cleanupInvalidObservers() {
        auto it = std::remove_if(m_observers.begin(), m_observers.end(),
            [](const WeakObserverPtr& weak) {
                return weak.expired();
            });
        m_observers.erase(it, m_observers.end());
    }

private:
    QList<WeakObserverPtr> m_observers;
    mutable QMutex m_mutex;
};

/**
 * @brief 音频事件类型定义
 */
namespace AudioEvents {
    struct StateChanged {
        enum class State { Playing, Paused, Stopped, Error } state;
        qint64 position = 0;
        qint64 duration = 0;
        QString errorMessage;
    };
    
    struct VolumeChanged {
        int volume = 50;
        bool muted = false;
        double balance = 0.0;
    };
    
    struct SongChanged {
        QString title;
        QString artist;
        QString album;
        QString filePath;
        qint64 duration = 0;
        int index = -1;
    };
    
    struct PlaylistChanged {
        QStringList songs;
        int currentIndex = -1;
        enum class PlayMode { Sequential, Loop, Random, Single } playMode;
    };
    
    struct PerformanceInfo {
        double cpuUsage = 0.0;
        qint64 memoryUsage = 0;
        int bufferLevel = 0;
        double responseTime = 0.0;
        QString engineType;
    };
}

// 类型别名，简化使用
using AudioStateObserver = IObserver<AudioEvents::StateChanged>;
using AudioVolumeObserver = IObserver<AudioEvents::VolumeChanged>;
using AudioSongObserver = IObserver<AudioEvents::SongChanged>;
using AudioPlaylistObserver = IObserver<AudioEvents::PlaylistChanged>;
using AudioPerformanceObserver = IObserver<AudioEvents::PerformanceInfo>;

using AudioStateSubject = Subject<AudioEvents::StateChanged>;
using AudioVolumeSubject = Subject<AudioEvents::VolumeChanged>;
using AudioSongSubject = Subject<AudioEvents::SongChanged>;
using AudioPlaylistSubject = Subject<AudioEvents::PlaylistChanged>;
using AudioPerformanceSubject = Subject<AudioEvents::PerformanceInfo>;

#endif // OBSERVER_H