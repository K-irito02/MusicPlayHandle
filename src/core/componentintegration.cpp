#include "componentintegration.h"

// QDebug 运算符重载实现
QDebug operator<<(QDebug debug, const ComponentType& type) {
    QDebugStateSaver saver(debug);
    debug.nospace();
    switch (type) {
        case ComponentType::AudioEngine: return debug << "AudioEngine";
        case ComponentType::TagManager: return debug << "TagManager";
        case ComponentType::PlaylistManager: return debug << "PlaylistManager";
        case ComponentType::MainThreadManager: return debug << "MainThreadManager";
        case ComponentType::AudioWorkerThread: return debug << "AudioWorkerThread";
        case ComponentType::DatabaseManager: return debug << "DatabaseManager";
        case ComponentType::Logger: return debug << "Logger";
        case ComponentType::Unknown: return debug << "Unknown";
        default: return debug << "Invalid";
    }
}

QDebug operator<<(QDebug debug, const ComponentStatus& status) {
    QDebugStateSaver saver(debug);
    debug.nospace();
    switch (status) {
        case ComponentStatus::NotInitialized: return debug << "NotInitialized";
        case ComponentStatus::Initializing: return debug << "Initializing";
        case ComponentStatus::Ready: return debug << "Ready";
        case ComponentStatus::Error: return debug << "Error";
        case ComponentStatus::Shutdown: return debug << "Shutdown";
        default: return debug << "Invalid";
    }
}

QDebug operator<<(QDebug debug, const IntegrationEventType& type) {
    QDebugStateSaver saver(debug);
    debug.nospace();
    switch (type) {
        case IntegrationEventType::ComponentInitialized: return debug << "ComponentInitialized";
        case IntegrationEventType::ComponentReady: return debug << "ComponentReady";
        case IntegrationEventType::ComponentError: return debug << "ComponentError";
        case IntegrationEventType::ComponentShutdown: return debug << "ComponentShutdown";
        case IntegrationEventType::AudioStateChanged: return debug << "AudioStateChanged";
        case IntegrationEventType::TagChanged: return debug << "TagChanged";
        case IntegrationEventType::PlaylistChanged: return debug << "PlaylistChanged";
        case IntegrationEventType::DatabaseChanged: return debug << "DatabaseChanged";
        case IntegrationEventType::UIUpdateRequired: return debug << "UIUpdateRequired";
        default: return debug << "Invalid";
    }
}
#include "logger.h"
#include "applicationmanager.h"
#include "performancemanager.h"
#include "../audio/audioengine.h"
#include "../managers/tagmanager.h"
#include "../managers/playlistmanager.h"
#include "../threading/mainthreadmanager.h"
#include "../threading/audioworkerthread.h"
#include "../database/databasemanager.h"

#include <QDebug>
#include <QCoreApplication>
#include <QMetaObject>
#include <QThread>
#include <QMutexLocker>

// 静态实例
static ComponentIntegration* s_instance = nullptr;
static QMutex s_instanceMutex;

ComponentIntegration* ComponentIntegration::instance()
{
    QMutexLocker locker(&s_instanceMutex);
    if (!s_instance) {
        s_instance = new ComponentIntegration();
    }
    return s_instance;
}

void ComponentIntegration::cleanup()
{
    QMutexLocker locker(&s_instanceMutex);
    if (s_instance) {
        s_instance->shutdown();
        delete s_instance;
        s_instance = nullptr;
    }
}

ComponentIntegration::ComponentIntegration(QObject* parent)
    : QObject(parent)
    , m_initialized(false)
    , m_shutdownInProgress(false)
    , m_overallStatus(ComponentStatus::NotInitialized)
    , m_performanceMonitoringEnabled(false)
    , m_debugMode(false)
    , m_hotReloadEnabled(false)
    , m_mainWindow(nullptr)
    , m_eventTimer(nullptr)
    , m_performanceTimer(nullptr)
    , m_statusTimer(nullptr)
    , m_timeoutTimer(nullptr)
{
    // 初始化定时器
    m_eventTimer = new QTimer(this);
    m_eventTimer->setSingleShot(false);
    m_eventTimer->setInterval(10); // 10ms间隔处理事件
    connect(m_eventTimer, &QTimer::timeout, this, &ComponentIntegration::processEventQueue);
    
    m_statusTimer = new QTimer(this);
    m_statusTimer->setSingleShot(false);
    m_statusTimer->setInterval(1000); // 1秒间隔更新状态
    connect(m_statusTimer, &QTimer::timeout, this, &ComponentIntegration::updateComponentStatuses);
    
    m_performanceTimer = new QTimer(this);
    m_performanceTimer->setSingleShot(false);
    m_performanceTimer->setInterval(5000); // 5秒间隔性能监控
    connect(m_performanceTimer, &QTimer::timeout, this, &ComponentIntegration::handlePerformanceTimer);
}

ComponentIntegration::~ComponentIntegration()
{
    shutdown();
}

bool ComponentIntegration::initialize(QMainWindow* mainWindow)
{
    QMutexLocker locker(&m_componentMutex);
    
    if (m_initialized) {
        return true;
    }
    
    m_mainWindow = mainWindow;
    
    emit initializationStarted();
    
    try {
        // 启动事件处理定时器
        m_eventTimer->start();
        m_statusTimer->start();
        
        // 加载配置
        loadConfiguration();
        
        m_initialized = true;
        
        emit initializationCompleted(true);
        
        if (m_debugMode) {
            qDebug() << "ComponentIntegration: 初始化完成";
        }
        
        return true;
        
    } catch (const std::exception& e) {
        QString error = QString("ComponentIntegration初始化失败: %1").arg(e.what());
        qCritical() << error;
        emit initializationCompleted(false);
        return false;
    } catch (...) {
        QString error = "ComponentIntegration初始化失败: 未知异常";
        qCritical() << error;
        emit initializationCompleted(false);
        return false;
    }
}

void ComponentIntegration::shutdown()
{
    QMutexLocker locker(&m_componentMutex);
    
    if (!m_initialized) {
        return;
    }
    
    emit shutdownStarted();
    
    try {
        // 停止定时器
        if (m_eventTimer) {
            m_eventTimer->stop();
        }
        if (m_statusTimer) {
            m_statusTimer->stop();
        }
        if (m_performanceTimer) {
            m_performanceTimer->stop();
        }
        
        // 断开所有连接
        disconnectComponents();
        
        // 清理组件
        for (auto it = m_components.begin(); it != m_components.end(); ++it) {
            setComponentStatus(it.key(), ComponentStatus::Shutdown);
        }
        m_components.clear();
        
        // 清理事件队列
        m_eventQueue.clear();
        
        // 保存配置
        saveConfiguration();
        
        m_initialized = false;
        
        emit shutdownCompleted();
        
        if (m_debugMode) {
            qDebug() << "ComponentIntegration: 关闭完成";
        }
        
    } catch (const std::exception& e) {
        qCritical() << "ComponentIntegration关闭异常:" << e.what();
    } catch (...) {
        qCritical() << "ComponentIntegration关闭未知异常";
    }
}

bool ComponentIntegration::isInitialized() const
{
    QMutexLocker locker(&m_componentMutex);
    return m_initialized;
}

bool ComponentIntegration::registerComponent(ComponentType type, QObject* component, const QString& name)
{
    QMutexLocker locker(&m_componentMutex);
    
    if (!component) {
        qWarning() << "ComponentIntegration: 尝试注册空组件";
        return false;
    }
    
    if (m_components.contains(type)) {
        qWarning() << "ComponentIntegration: 组件类型已存在:" << static_cast<int>(type);
        return false;
    }
    
    ComponentInfo info;
    info.type = type;
    info.name = name.isEmpty() ? QString("Component_%1").arg(static_cast<int>(type)) : name;
    info.status = ComponentStatus::NotInitialized;
    info.instance = component;
    info.lastUpdated = QDateTime::currentDateTime();
    
    m_components[type] = info;
    
    emit componentRegistered(type);
    
    if (m_debugMode) {
        qDebug() << "ComponentIntegration: 注册组件:" << info.name;
    }
    
    return true;
}

bool ComponentIntegration::unregisterComponent(ComponentType type)
{
    QMutexLocker locker(&m_componentMutex);
    
    if (!m_components.contains(type)) {
        return false;
    }
    
    setComponentStatus(type, ComponentStatus::Shutdown);
    m_components.remove(type);
    
    emit componentUnregistered(type);
    
    if (m_debugMode) {
        qDebug() << "ComponentIntegration: 注销组件:" << static_cast<int>(type);
    }
    
    return true;
}

QObject* ComponentIntegration::getComponent(ComponentType type) const
{
    QMutexLocker locker(&m_componentMutex);
    
    auto it = m_components.find(type);
    if (it != m_components.end()) {
        return it->instance;
    }
    
    return nullptr;
}

ComponentInfo ComponentIntegration::getComponentInfo(ComponentType type) const
{
    QMutexLocker locker(&m_componentMutex);
    
    auto it = m_components.find(type);
    if (it != m_components.end()) {
        return it.value();
    }
    
    return ComponentInfo();
}

QList<ComponentInfo> ComponentIntegration::getAllComponents() const
{
    QMutexLocker locker(&m_componentMutex);
    return m_components.values();
}

void ComponentIntegration::setComponentStatus(ComponentType type, ComponentStatus status, const QString& error)
{
    QMutexLocker locker(&m_componentMutex);
    
    auto it = m_components.find(type);
    if (it != m_components.end()) {
        it->status = status;
        it->lastUpdated = QDateTime::currentDateTime();
        it->errorMessage = error;
        
        emit componentStatusChanged(type, status);
        
        if (status == ComponentStatus::Error) {
            emit componentError(type, error);
        }
        
        if (m_debugMode) {
            qDebug() << "ComponentIntegration: 组件状态变化:" << static_cast<int>(type) << "->" << static_cast<int>(status);
        }
    }
}

ComponentStatus ComponentIntegration::getComponentStatus(ComponentType type) const
{
    QMutexLocker locker(&m_componentMutex);
    
    auto it = m_components.find(type);
    if (it != m_components.end()) {
        return it->status;
    }
    
    return ComponentStatus::NotInitialized;
}

bool ComponentIntegration::isComponentReady(ComponentType type) const
{
    return getComponentStatus(type) == ComponentStatus::Ready;
}

bool ComponentIntegration::areAllComponentsReady() const
{
    QMutexLocker locker(&m_componentMutex);
    
    for (const auto& info : m_components) {
        if (info.status != ComponentStatus::Ready) {
            return false;
        }
    }
    
    return !m_components.isEmpty();
}

void ComponentIntegration::postEvent(const IntegrationEvent& event)
{
    QMutexLocker locker(&m_eventMutex);
    
    m_eventQueue.enqueue(event);
    
    emit eventPosted(event.type, event.source);
    
    if (m_debugMode) {
        qDebug() << "ComponentIntegration: 发布事件:" << static_cast<int>(event.type) << "来源:" << static_cast<int>(event.source);
    }
}

void ComponentIntegration::postEvent(IntegrationEventType type, ComponentType source, const QVariant& data)
{
    IntegrationEvent event(type, source, data);
    postEvent(event);
}

void ComponentIntegration::connectComponents()
{
    if (m_debugMode) {
        qDebug() << "ComponentIntegration: 连接组件信号槽";
    }
    
    connectAudioEngineSignals();
    connectTagManagerSignals();
    connectPlaylistManagerSignals();
    connectDatabaseSignals();
    connectUISignals();
}

void ComponentIntegration::disconnectComponents()
{
    if (m_debugMode) {
        qDebug() << "ComponentIntegration: 断开组件信号槽";
    }
    
    // 断开所有连接
    for (const auto& info : m_components) {
        if (info.instance) {
            disconnect(info.instance, nullptr, this, nullptr);
            disconnect(this, nullptr, info.instance, nullptr);
        }
    }
}

void ComponentIntegration::connectAudioEngineSignals()
{
    AudioEngine* audioEngine = qobject_cast<AudioEngine*>(getComponent(ComponentType::AudioEngine));
    if (audioEngine) {
        // 连接音频引擎信号
        connect(audioEngine, &AudioEngine::stateChanged, this, &ComponentIntegration::onAudioEngineStateChanged);
        // 可以添加更多信号连接
    }
}

void ComponentIntegration::connectTagManagerSignals()
{
    TagManager* tagManager = qobject_cast<TagManager*>(getComponent(ComponentType::TagManager));
    if (tagManager) {
        // 连接标签管理器信号
        // connect(tagManager, &TagManager::tagChanged, this, &ComponentIntegration::onTagManagerChanged);
    }
}

void ComponentIntegration::connectPlaylistManagerSignals()
{
    PlaylistManager* playlistManager = qobject_cast<PlaylistManager*>(getComponent(ComponentType::PlaylistManager));
    if (playlistManager) {
        // 连接播放列表管理器信号
        // connect(playlistManager, &PlaylistManager::playlistChanged, this, &ComponentIntegration::onPlaylistManagerChanged);
    }
}

void ComponentIntegration::connectDatabaseSignals()
{
    DatabaseManager* dbManager = qobject_cast<DatabaseManager*>(getComponent(ComponentType::DatabaseManager));
    if (dbManager) {
        // 连接数据库管理器信号
        // connect(dbManager, &DatabaseManager::databaseChanged, this, &ComponentIntegration::onDatabaseChanged);
    }
}

void ComponentIntegration::connectUISignals()
{
    // 连接UI相关信号
    if (m_mainWindow) {
        // 可以连接主窗口信号
    }
}

void ComponentIntegration::syncComponentStates()
{
    if (m_debugMode) {
        qDebug() << "ComponentIntegration: 同步组件状态";
    }
    
    syncAudioState();
    syncTagState();
    syncPlaylistState();
    syncDatabaseState();
    syncUIState();
    
    emit statesSynchronized();
}

void ComponentIntegration::syncAudioState()
{
    // 同步音频状态
}

void ComponentIntegration::syncTagState()
{
    // 同步标签状态
}

void ComponentIntegration::syncPlaylistState()
{
    // 同步播放列表状态
}

void ComponentIntegration::syncDatabaseState()
{
    // 同步数据库状态
}

void ComponentIntegration::syncUIState()
{
    // 同步UI状态
}

void ComponentIntegration::notifyComponentsOfChange(ComponentType source, const QVariant& data)
{
    postEvent(IntegrationEventType::ComponentReady, source, data);
}

void ComponentIntegration::broadcastEvent(IntegrationEventType type, const QVariant& data)
{
    postEvent(type, ComponentType::Unknown, data);
}

void ComponentIntegration::handleComponentError(ComponentType type, const QString& error)
{
    setComponentStatus(type, ComponentStatus::Error, error);
    
    if (m_debugMode) {
        qDebug() << "ComponentIntegration: 组件错误:" << static_cast<int>(type) << error;
    }
}

void ComponentIntegration::handleCriticalError(const QString& error)
{
    qCritical() << "ComponentIntegration: 严重错误:" << error;
    
    // 可以触发应用程序关闭或重启
}

void ComponentIntegration::startPerformanceMonitoring()
{
    m_performanceMonitoringEnabled = true;
    m_performanceTimer->start();
    
    if (m_debugMode) {
        qDebug() << "ComponentIntegration: 启动性能监控";
    }
}

void ComponentIntegration::stopPerformanceMonitoring()
{
    m_performanceMonitoringEnabled = false;
    m_performanceTimer->stop();
    
    if (m_debugMode) {
        qDebug() << "ComponentIntegration: 停止性能监控";
    }
}

bool ComponentIntegration::isPerformanceMonitoringEnabled() const
{
    return m_performanceMonitoringEnabled;
}

void ComponentIntegration::enableDebugMode(bool enabled)
{
    m_debugMode = enabled;
    
    if (enabled) {
        qDebug() << "ComponentIntegration: 启用调试模式";
    }
}

bool ComponentIntegration::isDebugModeEnabled() const
{
    return m_debugMode;
}

void ComponentIntegration::dumpComponentStates() const
{
    qDebug() << "=== ComponentIntegration 组件状态 ===";
    
    for (auto it = m_components.begin(); it != m_components.end(); ++it) {
        const ComponentInfo& info = it.value();
        qDebug() << QString("组件: %1, 状态: %2, 错误: %3")
                    .arg(info.name)
                    .arg(static_cast<int>(info.status))
                    .arg(info.errorMessage);
    }
    
    qDebug() << "===============================";
}

void ComponentIntegration::dumpEventQueue() const
{
    qDebug() << "=== ComponentIntegration 事件队列 ===";
    qDebug() << "队列大小:" << m_eventQueue.size();
    qDebug() << "===============================";
}

void ComponentIntegration::loadConfiguration()
{
    // 加载配置
    if (m_debugMode) {
        qDebug() << "ComponentIntegration: 加载配置";
    }
}

void ComponentIntegration::saveConfiguration()
{
    // 保存配置
    if (m_debugMode) {
        qDebug() << "ComponentIntegration: 保存配置";
    }
}

void ComponentIntegration::applyConfiguration()
{
    // 应用配置
    if (m_debugMode) {
        qDebug() << "ComponentIntegration: 应用配置";
    }
}

void ComponentIntegration::enableHotReload(bool enabled)
{
    m_hotReloadEnabled = enabled;
    
    if (m_debugMode) {
        qDebug() << "ComponentIntegration: 热重载" << (enabled ? "启用" : "禁用");
    }
}

bool ComponentIntegration::reloadComponent(ComponentType type)
{
    if (!m_hotReloadEnabled) {
        return false;
    }
    
    // 重载组件逻辑
    if (m_debugMode) {
        qDebug() << "ComponentIntegration: 重载组件:" << static_cast<int>(type);
    }
    
    return true;
}

void ComponentIntegration::reloadAllComponents()
{
    if (!m_hotReloadEnabled) {
        return;
    }
    
    for (auto it = m_components.begin(); it != m_components.end(); ++it) {
        reloadComponent(it.key());
    }
}

// 私有槽函数实现
void ComponentIntegration::processEventQueue()
{
    QMutexLocker locker(&m_eventMutex);
    
    while (!m_eventQueue.isEmpty()) {
        IntegrationEvent event = m_eventQueue.dequeue();
        
        try {
            // 处理事件
            switch (event.type) {
                case IntegrationEventType::ComponentInitialized:
                    // 处理组件初始化事件
                    break;
                case IntegrationEventType::ComponentReady:
                    // 处理组件就绪事件
                    break;
                case IntegrationEventType::ComponentError:
                    // 处理组件错误事件
                    break;
                case IntegrationEventType::AudioStateChanged:
                    // 处理音频状态变化事件
                    break;
                default:
                    break;
            }
            
            emit eventProcessed(event.type, event.source);
            
        } catch (const std::exception& e) {
            QString error = QString("事件处理异常: %1").arg(e.what());
            emit eventError(event.type, event.source, error);
        } catch (...) {
            QString error = "事件处理未知异常";
            emit eventError(event.type, event.source, error);
        }
    }
}

void ComponentIntegration::updateComponentStatuses()
{
    // 更新组件状态
    for (auto it = m_components.begin(); it != m_components.end(); ++it) {
        ComponentInfo& info = it.value();
        
        // 检查组件是否仍然有效
        if (info.instance && !info.instance->parent()) {
            // 组件可能已被删除
            setComponentStatus(it.key(), ComponentStatus::Error, "组件实例无效");
        }
    }
}

void ComponentIntegration::handlePerformanceTimer()
{
    if (!m_performanceMonitoringEnabled) {
        return;
    }
    
    // 收集性能数据
    QVariantMap performanceData;
    performanceData["timestamp"] = QDateTime::currentMSecsSinceEpoch();
    performanceData["componentCount"] = m_components.size();
    performanceData["eventQueueSize"] = m_eventQueue.size();
    
    emit performanceDataAvailable(performanceData);
}

void ComponentIntegration::handleComponentTimeout()
{
    // 处理组件超时
}

// 组件事件处理槽函数
void ComponentIntegration::onAudioEngineStateChanged()
{
    postEvent(IntegrationEventType::AudioStateChanged, ComponentType::AudioEngine);
}

void ComponentIntegration::onTagManagerChanged()
{
    postEvent(IntegrationEventType::TagChanged, ComponentType::TagManager);
}

void ComponentIntegration::onPlaylistManagerChanged()
{
    postEvent(IntegrationEventType::PlaylistChanged, ComponentType::PlaylistManager);
}

void ComponentIntegration::onDatabaseChanged()
{
    postEvent(IntegrationEventType::DatabaseChanged, ComponentType::DatabaseManager);
}

void ComponentIntegration::onUIUpdateRequired()
{
    postEvent(IntegrationEventType::UIUpdateRequired, ComponentType::Unknown);
}

#include "componentintegration.moc"