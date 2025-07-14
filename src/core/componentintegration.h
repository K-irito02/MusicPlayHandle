#ifndef COMPONENTINTEGRATION_H
#define COMPONENTINTEGRATION_H

#include <QObject>
#include <QMutex>
#include <QTimer>
#include <QHash>
#include <QQueue>
#include <QThread>
#include <QApplication>
#include <QMainWindow>
#include <QDateTime>

// 前向声明
class AudioEngine;
class TagManager;
class PlaylistManager;
class MainThreadManager;
class AudioWorkerThread;
class DatabaseManager;
class Logger;

// 组件类型枚举
enum class ComponentType {
    AudioEngine,
    TagManager,
    PlaylistManager,
    MainThreadManager,
    AudioWorkerThread,
    DatabaseManager,
    Logger,
    Unknown
};

// 组件状态枚举
enum class ComponentStatus {
    NotInitialized,
    Initializing,
    Ready,
    Error,
    Shutdown
};

// 组件信息结构
struct ComponentInfo {
    ComponentType type;
    QString name;
    QString version;
    ComponentStatus status;
    QObject* instance;
    QDateTime lastUpdated;
    QString errorMessage;
    
    ComponentInfo() : type(ComponentType::Unknown), status(ComponentStatus::NotInitialized), 
                     instance(nullptr), lastUpdated(QDateTime::currentDateTime()) {}
};

// 事件类型枚举
enum class IntegrationEventType {
    ComponentInitialized,
    ComponentReady,
    ComponentError,
    ComponentShutdown,
    AudioStateChanged,
    TagChanged,
    PlaylistChanged,
    DatabaseChanged,
    UIUpdateRequired
};

// 集成事件结构
struct IntegrationEvent {
    IntegrationEventType type;
    ComponentType source;
    QVariant data;
    qint64 timestamp;
    int priority;
    
    IntegrationEvent(IntegrationEventType t, ComponentType s, const QVariant& d = QVariant(), int p = 0)
        : type(t), source(s), data(d), timestamp(QDateTime::currentMSecsSinceEpoch()), priority(p) {}
};

// 组件集成管理器
class ComponentIntegration : public QObject {
    Q_OBJECT
    
public:
    // 单例模式
    static ComponentIntegration* instance();
    static void cleanup();
    
    // 初始化和清理
    bool initialize(QMainWindow* mainWindow);
    void shutdown();
    bool isInitialized() const;
    
    // 组件管理
    bool registerComponent(ComponentType type, QObject* component, const QString& name = QString());
    bool unregisterComponent(ComponentType type);
    QObject* getComponent(ComponentType type) const;
    ComponentInfo getComponentInfo(ComponentType type) const;
    QList<ComponentInfo> getAllComponents() const;
    
    // 组件状态管理
    void setComponentStatus(ComponentType type, ComponentStatus status, const QString& error = QString());
    ComponentStatus getComponentStatus(ComponentType type) const;
    bool isComponentReady(ComponentType type) const;
    bool areAllComponentsReady() const;
    
    // 事件处理
    void postEvent(const IntegrationEvent& event);
    void postEvent(IntegrationEventType type, ComponentType source, const QVariant& data = QVariant());
    
    // 信号槽连接管理
    void connectComponents();
    void disconnectComponents();
    void connectAudioEngineSignals();
    void connectTagManagerSignals();
    void connectPlaylistManagerSignals();
    void connectDatabaseSignals();
    void connectUISignals();
    
    // 状态同步
    void syncComponentStates();
    void syncAudioState();
    void syncTagState();
    void syncPlaylistState();
    void syncDatabaseState();
    void syncUIState();
    
    // 组件通信
    void notifyComponentsOfChange(ComponentType source, const QVariant& data);
    void broadcastEvent(IntegrationEventType type, const QVariant& data);
    
    // 错误处理
    void handleComponentError(ComponentType type, const QString& error);
    void handleCriticalError(const QString& error);
    
    // 性能监控
    void startPerformanceMonitoring();
    void stopPerformanceMonitoring();
    bool isPerformanceMonitoringEnabled() const;
    
    // 调试支持
    void enableDebugMode(bool enabled);
    bool isDebugModeEnabled() const;
    void dumpComponentStates() const;
    void dumpEventQueue() const;
    
    // 配置管理
    void loadConfiguration();
    void saveConfiguration();
    void applyConfiguration();
    
    // 热重载支持
    void enableHotReload(bool enabled);
    bool reloadComponent(ComponentType type);
    void reloadAllComponents();
    
signals:
    // 组件状态变化信号
    void componentRegistered(ComponentType type);
    void componentUnregistered(ComponentType type);
    void componentStatusChanged(ComponentType type, ComponentStatus status);
    void componentError(ComponentType type, const QString& error);
    
    // 初始化信号
    void initializationStarted();
    void initializationProgress(int current, int total);
    void initializationCompleted(bool success);
    void shutdownStarted();
    void shutdownCompleted();
    
    // 事件处理信号
    void eventPosted(IntegrationEventType type, ComponentType source);
    void eventProcessed(IntegrationEventType type, ComponentType source);
    void eventError(IntegrationEventType type, ComponentType source, const QString& error);
    
    // 状态同步信号
    void statesSynchronized();
    void stateSyncError(const QString& error);
    
    // 性能监控信号
    void performanceDataAvailable(const QVariantMap& data);
    void performanceThresholdExceeded(const QString& metric, double value);
    
private slots:
    void processEventQueue();
    void updateComponentStatuses();
    void handlePerformanceTimer();
    void handleComponentTimeout();
    
    // 组件事件处理
    void onAudioEngineStateChanged();
    void onTagManagerChanged();
    void onPlaylistManagerChanged();
    void onDatabaseChanged();
    void onUIUpdateRequired();
    
private:
    explicit ComponentIntegration(QObject* parent = nullptr);
    ~ComponentIntegration();
    
    // 禁用拷贝构造和赋值
    ComponentIntegration(const ComponentIntegration&) = delete;
    ComponentIntegration& operator=(const ComponentIntegration&) = delete;
    
    // 单例实例
    static ComponentIntegration* m_instance;
    static QMutex m_instanceMutex;
    
    // 组件管理
    QHash<ComponentType, ComponentInfo> m_components;
    QMutex m_componentMutex;
    
    // 事件队列
    QQueue<IntegrationEvent> m_eventQueue;
    QMutex m_eventMutex;
    QTimer* m_eventTimer;
    
    // 主窗口引用
    QMainWindow* m_mainWindow;
    
    // 状态管理
    bool m_initialized;
    bool m_shutdownInProgress;
    ComponentStatus m_overallStatus;
    
    // 性能监控
    QTimer* m_performanceTimer;
    QTimer* m_statusTimer;
    bool m_performanceMonitoringEnabled;
    QHash<QString, double> m_performanceMetrics;
    
    // 调试和配置
    bool m_debugMode;
    bool m_hotReloadEnabled;
    QHash<QString, QVariant> m_configuration;
    
    // 超时处理
    QTimer* m_timeoutTimer;
    QHash<ComponentType, QDateTime> m_lastActivity;
    
    // 内部方法
    void initializeCore();
    void initializeComponents();
    void setupTimers();
    void cleanupTimers();
    
    // 组件初始化
    bool initializeAudioEngine();
    bool initializeTagManager();
    bool initializePlaylistManager();
    bool initializeMainThreadManager();
    bool initializeAudioWorkerThread();
    bool initializeDatabaseManager();
    bool initializeLogger();
    
    // 事件处理
    void processIntegrationEvent(const IntegrationEvent& event);
    void handleComponentInitialized(ComponentType type);
    void handleComponentReady(ComponentType type);
    void handleComponentShutdown(ComponentType type);
    
    // 信号槽连接实现（内部使用）
    
    // 状态同步实现（内部使用）
    
    // 性能监控实现
    void collectPerformanceMetrics();
    void updatePerformanceMetrics();
    void checkPerformanceThresholds();
    
    // 配置管理实现
    void loadDefaultConfiguration();
    void validateConfiguration();
    void applyComponentConfiguration();
    
    // 错误处理实现
    void logError(const QString& error);
    void logInfo(const QString& message);
    void logDebug(const QString& message);
    
    // 工具方法
    QString componentTypeToString(ComponentType type) const;
    QString componentStatusToString(ComponentStatus status) const;
    QString eventTypeToString(IntegrationEventType type) const;
    bool isComponentCritical(ComponentType type) const;
    
    // 超时处理
    void startComponentTimeout(ComponentType type);
    void cancelComponentTimeout(ComponentType type);
    void handleComponentTimeoutInternal(ComponentType type);
    
    // 热重载实现
    bool canReloadComponent(ComponentType type) const;
    void saveComponentState(ComponentType type);
    void restoreComponentState(ComponentType type);
    
    // 内部常量
    static const int EVENT_PROCESSING_INTERVAL = 10; // 10ms
    static const int STATUS_UPDATE_INTERVAL = 1000; // 1秒
    static const int PERFORMANCE_UPDATE_INTERVAL = 5000; // 5秒
    static const int COMPONENT_TIMEOUT = 30000; // 30秒
    static const int MAX_EVENT_QUEUE_SIZE = 1000;
};

// 组件集成工具类
class ComponentUtils {
public:
    // 组件类型转换
    static QString componentTypeToString(ComponentType type);
    static ComponentType stringToComponentType(const QString& typeString);
    
    // 状态检查
    static bool isComponentStatusValid(ComponentStatus status);
    static bool isComponentStatusError(ComponentStatus status);
    
    // 事件工具
    static QString eventTypeToString(IntegrationEventType type);
    static IntegrationEventType stringToEventType(const QString& eventString);
    
    // 性能工具
    static double calculateCPUUsage();
    static double calculateMemoryUsage();
    static double calculateThreadUsage();
    
    // 调试工具
    static void dumpComponentInfo(const ComponentInfo& info);
    static void dumpIntegrationEvent(const IntegrationEvent& event);
    
    // 配置工具
    static QVariantMap getDefaultConfiguration();
    static bool validateConfiguration(const QVariantMap& config);
    static void applyConfiguration(const QVariantMap& config);
};

#endif // COMPONENTINTEGRATION_H 