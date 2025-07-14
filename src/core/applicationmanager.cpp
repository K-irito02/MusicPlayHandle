#include "applicationmanager.h"
#include "logger.h"
#include "appconfig.h"
#include "../database/databasemanager.h"
#include "../../mainwindow.h"

#include <QApplication>
#include <QObject>
#include <QMutex>
#include <QMutexLocker>
#include <QDebug>

ApplicationManager* ApplicationManager::m_instance = nullptr;
QMutex ApplicationManager::m_instanceMutex;

ApplicationManager::ApplicationManager(QObject* parent)
    : QObject(parent)
    , m_app(nullptr)
    , m_state(ApplicationState::NotStarted)
    , m_phase(InitializationPhase::PreInit)
    , m_initialized(false)
    , m_running(false)
    , m_mainWindow(nullptr)
    , m_audioEngine(nullptr)
    , m_tagManager(nullptr)
    , m_playlistManager(nullptr)
    , m_componentIntegration(nullptr)
    , m_mainThreadManager(nullptr)
    , m_databaseManager(nullptr)
    , m_logger(nullptr)
    , m_appConfig(nullptr)
    , m_splashScreen(nullptr)
    , m_systemTrayIcon(nullptr)
    , m_systemTrayMenu(nullptr)
    , m_translator(nullptr)
    , m_qtTranslator(nullptr)
    , m_initializationTimer(nullptr)
    , m_performanceTimer(nullptr)
    , m_updateCheckTimer(nullptr)
    , m_telemetryTimer(nullptr)
    , m_networkManager(nullptr)
    , m_performanceMonitorTimer(nullptr)
    , m_singleInstanceTray(nullptr)
{
}

ApplicationManager::~ApplicationManager()
{
    shutdown();
}

ApplicationManager* ApplicationManager::instance()
{
    QMutexLocker locker(&m_instanceMutex);
    if (!m_instance) {
        m_instance = new ApplicationManager();
    }
    return m_instance;
}

void ApplicationManager::cleanup()
{
    QMutexLocker locker(&m_instanceMutex);
    if (m_instance) {
        delete m_instance;
        m_instance = nullptr;
    }
}

bool ApplicationManager::initialize(QApplication* app, int argc, char* argv[])
{
    Q_UNUSED(argc)
    Q_UNUSED(argv)
    if (m_initialized) {
        return true;
    }

    m_app = app;
    m_state = ApplicationState::Initializing;
    
    try {
        // 初始化核心组件
        initializeCore();
        
        // 初始化数据库
        initializeDatabase();
        
        // 初始化其他组件
        initializeComponents();
        
        // 初始化UI
        initializeUI();
        
        m_initialized = true;
        m_state = ApplicationState::Running;
        
        return true;
    } catch (const std::exception& e) {
        m_state = ApplicationState::Error;
        return false;
    }
}

bool ApplicationManager::start()
{
    if (!m_initialized) {
        return false;
    }
    
    if (m_mainWindow) {
        m_mainWindow->show();
    }
    
    m_running = true;
    return true;
}

void ApplicationManager::shutdown()
{
    m_running = false;
    m_state = ApplicationState::Shutting;
    
    // 清理组件
    if (m_mainWindow) {
        m_mainWindow->close();
        delete m_mainWindow;
        m_mainWindow = nullptr;
    }
    
    // 测试管理器已删除
    
    if (m_databaseManager) {
        m_databaseManager = nullptr;
    }
    
    if (m_logger) {
        m_logger = nullptr;
    }
    
    if (m_appConfig) {
        m_appConfig = nullptr;
    }
}

void ApplicationManager::enableDebugMode(bool enabled)
{
    m_config.enableDebugMode = enabled;
}

void ApplicationManager::enableDeveloperMode(bool enabled)
{
    m_config.enableDeveloperMode = enabled;
}

// TestManager* ApplicationManager::getTestManager() const
// {
//     return m_testManager;
// }
// 测试管理器已删除

void ApplicationManager::initializeCore()
{
    m_logger = Logger::instance();
    m_appConfig = AppConfig::instance();
}

void ApplicationManager::initializeDatabase()
{
    m_databaseManager = DatabaseManager::instance();
    QString dbPath = AppConfig::instance()->databasePath();
    if (!m_databaseManager->initialize(dbPath)) {
        qCritical() << "数据库初始化失败:" << m_databaseManager->lastError();
        // 可选：弹窗或设置错误状态
    }
}

void ApplicationManager::initializeComponents()
{
    // 测试管理器已删除，暂时不需要其他组件
}

void ApplicationManager::initializeUI()
{
    m_mainWindow = new MainWindow();
}

// 槽函数实现
void ApplicationManager::onInitializationTimer()
{
    // 初始化定时器超时处理
    qWarning() << "ApplicationManager initialization timeout";
}

void ApplicationManager::onPerformanceTimer()
{
    // 性能监控定时器处理
    // 这里可以收集性能指标
}

void ApplicationManager::onUpdateCheckTimer()
{
    // 检查更新定时器处理
    // 这里可以检查应用程序更新
}

void ApplicationManager::onTelemetryTimer()
{
    // 遥测数据定时器处理
    // 这里可以发送遥测数据
}

void ApplicationManager::onSystemTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
    Q_UNUSED(reason)
    // 系统托盘激活处理
    // 这里可以处理系统托盘点击事件
}

void ApplicationManager::onUpdateReply()
{
    // 更新检查回复处理
    // 这里可以处理更新检查的网络回复
}

void ApplicationManager::onApplicationStateChanged(Qt::ApplicationState state)
{
    Q_UNUSED(state)
    // 应用程序状态变化处理
    // 这里可以处理应用程序状态变化
}

void ApplicationManager::onAboutToQuit()
{
    // 应用程序即将退出处理
    shutdown();
} 