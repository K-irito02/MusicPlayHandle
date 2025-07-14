#ifndef SERVICECONTAINER_H
#define SERVICECONTAINER_H

#include <QObject>
#include <QHash>
#include <QVariant>
#include <memory>
#include <typeinfo>
#include <typeindex>

/**
 * @brief 依赖注入服务容器
 * @details 提供服务注册、获取和生命周期管理功能
 */
class ServiceContainer : public QObject
{
    Q_OBJECT
    
public:
    explicit ServiceContainer(QObject* parent = nullptr);
    ~ServiceContainer();
    
    // 单例模式
    static ServiceContainer* instance();
    static void cleanup();
    
    /**
     * @brief 注册服务
     * @tparam T 服务类型
     * @param service 服务实例
     */
    template<typename T>
    void registerService(std::shared_ptr<T> service) {
        std::type_index typeIndex(typeid(T));
        m_services[typeIndex] = service;
    }
    
    /**
     * @brief 获取服务
     * @tparam T 服务类型
     * @return 服务实例，如果未注册则返回nullptr
     */
    template<typename T>
    std::shared_ptr<T> getService() {
        std::type_index typeIndex(typeid(T));
        auto it = m_services.find(typeIndex);
        if (it != m_services.end()) {
            return std::static_pointer_cast<T>(it->second);
        }
        return nullptr;
    }
    
    /**
     * @brief 检查服务是否已注册
     * @tparam T 服务类型
     * @return 是否已注册
     */
    template<typename T>
    bool hasService() const {
        std::type_index typeIndex(typeid(T));
        return m_services.contains(typeIndex);
    }
    
    /**
     * @brief 注销服务
     * @tparam T 服务类型
     */
    template<typename T>
    void unregisterService() {
        std::type_index typeIndex(typeid(T));
        m_services.remove(typeIndex);
    }
    
    /**
     * @brief 清空所有服务
     */
    void clear();
    
    /**
     * @brief 获取已注册服务数量
     */
    int serviceCount() const;
    
signals:
    void serviceRegistered(const QString& typeName);
    void serviceUnregistered(const QString& typeName);
    
private:
    static ServiceContainer* s_instance;
    QHash<std::type_index, std::shared_ptr<void>> m_services;
    
    // 禁用拷贝构造和赋值
    ServiceContainer(const ServiceContainer&) = delete;
    ServiceContainer& operator=(const ServiceContainer&) = delete;
};

#endif // SERVICECONTAINER_H