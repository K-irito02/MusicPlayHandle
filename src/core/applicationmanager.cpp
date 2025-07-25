#include "applicationmanager.h"
#include "logger.h"
#include "appconfig.h"
#include "../database/databasemanager.h"
#include "../../mainwindow.h"

#include <QApplication>
#include <QObject>
#include <QMutex>
#include <QMutexLocker>


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

#include <QDebug>

bool ApplicationManager::initialize(QApplication* app, int argc, char* argv[])
{
    qDebug() << "ApplicationManager::initialize() - 开始初始化";
    
    Q_UNUSED(argc)
    Q_UNUSED(argv)
    if (m_initialized) {
        qDebug() << "ApplicationManager::initialize() - 已经初始化，直接返回";
        return true;
    }

    m_app = app;
    m_state = ApplicationState::Initializing;
    qDebug() << "ApplicationManager::initialize() - 状态设置为 Initializing";
    
    try {
        // 初始化核心组件
        qDebug() << "ApplicationManager::initialize() - 开始初始化核心组件";
        initializeCore();
        qDebug() << "ApplicationManager::initialize() - 核心组件初始化完成";
        
        // 初始化数据库
        qDebug() << "ApplicationManager::initialize() - 开始初始化数据库";
        initializeDatabase();
        qDebug() << "ApplicationManager::initialize() - 数据库初始化完成";
        
        // 初始化其他组件
        qDebug() << "ApplicationManager::initialize() - 开始初始化其他组件";
        initializeComponents();
        qDebug() << "ApplicationManager::initialize() - 其他组件初始化完成";
        
        // 初始化UI
        qDebug() << "ApplicationManager::initialize() - 开始初始化UI";
        initializeUI();
        qDebug() << "ApplicationManager::initialize() - UI初始化完成";
        
        m_initialized = true;
        m_state = ApplicationState::Running;
        qDebug() << "ApplicationManager::initialize() - 初始化成功完成";
        
        return true;
    } catch (const std::exception& e) {
        QString error = QString("初始化失败: %1").arg(e.what());
        qCritical() << "ApplicationManager::initialize() - 捕获到异常:" << error;
        m_state = ApplicationState::Error;
        return false;
    } catch (...) {
        QString error = "初始化失败: 未知错误";
        qCritical() << "ApplicationManager::initialize() - 捕获到未知异常:" << error;
        m_state = ApplicationState::Error;
        return false;
    }
}

bool ApplicationManager::start()
{
    qDebug() << "ApplicationManager::start() - 开始启动应用程序";
    
    if (!m_initialized) {
        qDebug() << "ApplicationManager::start() - 应用程序未初始化，启动失败";
        return false;
    }
    
    if (m_running) {
        qDebug() << "ApplicationManager::start() - 应用程序已在运行";
        return true;
    }
    
    m_running = true;
    qDebug() << "ApplicationManager::start() - 应用程序启动成功";
    
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
    qDebug() << "ApplicationManager::initializeDatabase() - 开始初始化数据库管理器";
    
    qDebug() << "ApplicationManager::initializeDatabase() - 获取DatabaseManager实例";
    m_databaseManager = DatabaseManager::instance();
    if (!m_databaseManager) {
        qCritical() << "ApplicationManager::initializeDatabase() - 获取DatabaseManager实例失败";
        throw std::runtime_error("Failed to get DatabaseManager instance");
    }
    qDebug() << "ApplicationManager::initializeDatabase() - DatabaseManager实例获取成功";
    
    qDebug() << "ApplicationManager::initializeDatabase() - 获取数据库路径";
    QString dbPath = AppConfig::instance()->databasePath();
    qDebug() << "ApplicationManager::initializeDatabase() - 数据库路径:" << dbPath;
    
    qDebug() << "ApplicationManager::initializeDatabase() - 调用DatabaseManager::initialize";
    if (!m_databaseManager->initialize(dbPath)) {
        QString error = QString("数据库初始化失败: %1").arg(m_databaseManager->getLastError());
        qCritical() << "ApplicationManager::initializeDatabase() - 数据库初始化失败:" << error;
        throw std::runtime_error(error.toStdString());
    }
    qDebug() << "ApplicationManager::initializeDatabase() - 数据库初始化成功";
}

void ApplicationManager::initializeComponents()
{
    // 测试管理器已删除，暂时不需要其他组件
}

void ApplicationManager::initializeUI()
{
    qDebug() << "ApplicationManager::initializeUI() - 开始创建主窗口";
    
    m_mainWindow = new MainWindow();
    if (m_mainWindow) {
        qDebug() << "ApplicationManager::initializeUI() - 主窗口创建成功，准备显示";
        m_mainWindow->show();
        qDebug() << "ApplicationManager::initializeUI() - 主窗口显示完成";
    } else {
        qDebug() << "ApplicationManager::initializeUI() - 主窗口创建失败";
        logError("Failed to create MainWindow instance");
    }
}

// 槽函数实现
void ApplicationManager::logError(const QString& message)
{
    if (m_logger) {
        m_logger->error(message);
    }
}

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