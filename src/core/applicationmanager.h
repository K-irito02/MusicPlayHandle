#ifndef APPLICATIONMANAGER_H
#define APPLICATIONMANAGER_H

#include <QObject>
#include <QApplication>
#include <QMainWindow>
#include <QSplashScreen>
#include <QTimer>
#include <QThread>
#include <QMutex>
#include <QEventLoop>
#include <QProgressBar>
#include <QLabel>
#include <QPixmap>
#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QTranslator>
#include <QLocale>
#include <QVariant>
#include <QVariantMap>
#include <QHash>
#include <QStringList>
#include <QStyleFactory>
#include <QFontDatabase>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QProcess>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QCryptographicHash>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QHash>

// 前向声明
class MainWindow;
class AudioEngine;
class TagManager;
class PlaylistManager;
class ComponentIntegration;
class MainThreadManager;
class DatabaseManager;
class Logger;
class AppConfig;

// 应用程序状态枚举
enum class ApplicationState {
    NotStarted,
    Initializing,
    Loading,
    Running,
    Minimized,
    Suspended,
    Shutting,
    Crashed,
    Error
};

// 初始化阶段枚举
enum class InitializationPhase {
    PreInit,
    CoreInit,
    DatabaseInit,
    ComponentInit,
    UIInit,
    IntegrationInit,
    PostInit,
    Complete
};

// 应用程序配置结构
struct AppConfiguration {
    QString appName;
    QString appVersion;
    QString appDescription;
    QString organizationName;
    QString organizationDomain;
    QString configPath;
    QString dataPath;
    QString logPath;
    QString tempPath;
    QString locale;
    QString theme;
    bool enableSplashScreen;
    bool enableSystemTray;
    bool enableAutoStart;
    bool enableCrashReporting;
    bool enableTelemetry;
    bool enableUpdates;
    bool enableDeveloperMode;
    bool enableDebugMode;
    int logLevel;
    int maxLogFiles;
    int maxLogSize;
    
    AppConfiguration() : 
        appName("Qt6音频播放器"),
        appVersion("1.0.0"),
        appDescription("基于Qt6的音频播放器"),
        organizationName("Qt6音频播放器开发团队"),
        organizationDomain("musicplayer.qt6.com"),
        locale("zh_CN"),
        theme("dark"),
        enableSplashScreen(true),
        enableSystemTray(true),
        enableAutoStart(false),
        enableCrashReporting(true),
        enableTelemetry(false),
        enableUpdates(true),
        enableDeveloperMode(false),
        enableDebugMode(false),
        logLevel(2),
        maxLogFiles(10),
        maxLogSize(10485760) {} // 10MB
};

// 应用程序管理器
class ApplicationManager : public QObject {
    Q_OBJECT
    
public:
    // 单例模式
    static ApplicationManager* instance();
    static void cleanup();
    
    // 应用程序生命周期
    bool initialize(QApplication* app, int argc, char* argv[]);
    bool start();
    bool stop();
    bool restart();
    void shutdown();
    
    // 状态管理
    ApplicationState getState() const;
    InitializationPhase getCurrentPhase() const;
    bool isInitialized() const;
    bool isRunning() const;
    
    // 配置管理
    void setConfiguration(const AppConfiguration& config);
    AppConfiguration getConfiguration() const;
    void loadConfiguration();
    void saveConfiguration();
    void resetConfiguration();
    
    // 组件管理
    AudioEngine* getAudioEngine() const;
    TagManager* getTagManager() const;
    PlaylistManager* getPlaylistManager() const;
    ComponentIntegration* getComponentIntegration() const;
    MainThreadManager* getMainThreadManager() const;
    // TestManager* getTestManager() const;  // 测试组件已删除
    DatabaseManager* getDatabaseManager() const;
    Logger* getLogger() const;
    MainWindow* getMainWindow() const;
    
    // 国际化支持
    void setLocale(const QString& locale);
    QString getLocale() const;
    void loadTranslations();
    void changeLanguage(const QString& language);
    
    // 主题支持
    void setTheme(const QString& theme);
    QString getTheme() const;
    void loadTheme();
    void applyTheme();
    
    // 系统托盘
    void enableSystemTray(bool enabled);
    bool isSystemTrayEnabled() const;
    void showSystemTrayMessage(const QString& title, const QString& message);
    
    // 启动画面
    void showSplashScreen();
    void hideSplashScreen();
    void updateSplashScreen(const QString& message, int progress = -1);
    
    // 错误处理
    void handleCriticalError(const QString& error);
    void handleFatalError(const QString& error);
    void reportCrash(const QString& crashInfo);
    
    // 更新检查
    void checkForUpdates();
    void downloadUpdate(const QString& updateUrl);
    void installUpdate();
    
    // 性能监控
    void startPerformanceMonitoring();
    void stopPerformanceMonitoring();
    QVariantMap getPerformanceMetrics() const;
    
    // 调试支持
    void enableDebugMode(bool enabled);
    bool isDebugModeEnabled() const;
    void enableDeveloperMode(bool enabled);
    bool isDeveloperModeEnabled() const;
    
    // 遥测数据
    void enableTelemetry(bool enabled);
    bool isTelemetryEnabled() const;
    void sendTelemetryData(const QVariantMap& data);
    
    // 命令行处理
    void parseCommandLine(int argc, char* argv[]);
    QStringList getCommandLineArguments() const;
    bool hasCommandLineOption(const QString& option) const;
    QString getCommandLineValue(const QString& option) const;
    
    // 单实例检查
    bool isAnotherInstanceRunning() const;
    void bringToFront();
    void sendMessageToOtherInstance(const QString& message);
    
    // 自动启动
    void enableAutoStart(bool enabled);
    bool isAutoStartEnabled() const;
    
    // 备份和恢复
    bool createBackup();
    bool restoreBackup(const QString& backupPath);
    QStringList getAvailableBackups() const;
    
signals:
    // 状态变化信号
    void stateChanged(ApplicationState state);
    void phaseChanged(InitializationPhase phase);
    void initializationProgress(int progress, const QString& message);
    void initializationCompleted(bool success);
    
    // 配置变化信号
    void configurationChanged();
    void localeChanged(const QString& locale);
    void themeChanged(const QString& theme);
    
    // 系统信号
    void systemTrayActivated(QSystemTrayIcon::ActivationReason reason);
    void updateAvailable(const QString& version, const QString& url);
    void updateDownloaded(const QString& filePath);
    void updateInstalled(bool success);
    
    // 错误信号
    void criticalError(const QString& error);
    void fatalError(const QString& error);
    void crashDetected(const QString& crashInfo);
    
    // 性能信号
    void performanceMetricsUpdated(const QVariantMap& metrics);
    void performanceThresholdExceeded(const QString& metric, double value);
    
    // 消息信号
    void messageReceived(const QString& message);
    
private slots:
    void onInitializationTimer();
    void onPerformanceTimer();
    void onUpdateCheckTimer();
    void onTelemetryTimer();
    void onSystemTrayActivated(QSystemTrayIcon::ActivationReason reason);
    void onUpdateReply();
    void onApplicationStateChanged(Qt::ApplicationState state);
    void onAboutToQuit();
    
private:
    explicit ApplicationManager(QObject* parent = nullptr);
    ~ApplicationManager();
    
    // 禁用拷贝构造和赋值
    ApplicationManager(const ApplicationManager&) = delete;
    ApplicationManager& operator=(const ApplicationManager&) = delete;
    
    // 单例实例
    static ApplicationManager* m_instance;
    static QMutex m_instanceMutex;
    
    // 应用程序状态
    QApplication* m_app;
    ApplicationState m_state;
    InitializationPhase m_phase;
    AppConfiguration m_config;
    bool m_initialized;
    bool m_running;
    
    // 核心组件
    MainWindow* m_mainWindow;
    AudioEngine* m_audioEngine;
    TagManager* m_tagManager;
    PlaylistManager* m_playlistManager;
    ComponentIntegration* m_componentIntegration;
    MainThreadManager* m_mainThreadManager;
    // TestManager* m_testManager;  // 测试组件已删除
    DatabaseManager* m_databaseManager;
    Logger* m_logger;
    AppConfig* m_appConfig;
    
    // UI组件
    QSplashScreen* m_splashScreen;
    QSystemTrayIcon* m_systemTrayIcon;
    QMenu* m_systemTrayMenu;
    
    // 国际化
    QTranslator* m_translator;
    QTranslator* m_qtTranslator;
    
    // 定时器
    QTimer* m_initializationTimer;
    QTimer* m_performanceTimer;
    QTimer* m_updateCheckTimer;
    QTimer* m_telemetryTimer;
    
    // 网络
    QNetworkAccessManager* m_networkManager;
    
    // 命令行
    QStringList m_commandLineArgs;
    QHash<QString, QString> m_commandLineOptions;
    
    // 性能监控
    QVariantMap m_performanceMetrics;
    QTimer* m_performanceMonitorTimer;
    
    // 单实例检查
    QString m_instanceKey;
    QSystemTrayIcon* m_singleInstanceTray;
    
    // 更新管理
    QString m_updateUrl;
    QString m_updateVersion;
    QString m_downloadedUpdatePath;
    
    // 内部方法
    void initializeCore();
    void initializeDatabase();
    void initializeComponents();
    void initializeUI();
    void initializeIntegration();
    void finalizeInitialization();
    
    // 配置管理
    void loadDefaultConfiguration();
    void createConfigDirectories();
    void validateConfiguration();
    
    // 组件创建
    void createCoreComponents();
    void createManagerComponents();
    void createUIComponents();
    // void createTestComponents();  // 测试组件已删除
    
    // 连接管理
    void connectComponents();
    void disconnectComponents();
    void setupSignalConnections();
    
    // 国际化实现
    void loadTranslation(const QString& locale);
    void installTranslator();
    void removeTranslator();
    
    // 主题实现
    void loadThemeFromFile(const QString& themeFile);
    void applyStyleSheet(const QString& styleSheet);
    void loadFonts();
    
    // 系统托盘实现
    void createSystemTrayIcon();
    void createSystemTrayMenu();
    void updateSystemTrayIcon();
    void updateSystemTrayMenu();
    
    // 启动画面实现
    void createSplashScreen();
    void updateSplashScreenMessage(const QString& message, int progress);
    
    // 错误处理实现
    void setupErrorHandling();
    void handleComponentError(const QString& component, const QString& error);
    void generateCrashReport(const QString& error);
    void sendCrashReport(const QString& report);
    
    // 更新检查实现
    void performUpdateCheck();
    void parseUpdateResponse(const QByteArray& response);
    void downloadUpdateFile(const QString& url);
    void verifyUpdateFile(const QString& filePath);
    
    // 性能监控实现
    void collectPerformanceMetrics();
    void analyzePerformanceMetrics();
    void reportPerformanceIssues();
    
    // 遥测实现
    void collectTelemetryData();
    void sendTelemetryDataInternal(const QVariantMap& data);
    
    // 命令行处理
    void parseCommandLineInternal(int argc, char* argv[]);
    void handleCommandLineOption(const QString& option, const QString& value);
    
    // 单实例实现
    void checkForRunningInstance();
    void createInstanceCheck();
    void handleInstanceMessage(const QString& message);
    
    // 自动启动实现
    void setupAutoStart();
    void removeAutoStart();
    bool isAutoStartConfigured() const;
    
    // 备份实现
    QString createBackupInternal();
    bool restoreBackupInternal(const QString& backupPath);
    void cleanupOldBackups();
    
    // 工具方法
    QString getApplicationDataPath() const;
    QString getApplicationConfigPath() const;
    QString getApplicationLogPath() const;
    QString getApplicationTempPath() const;
    QString getApplicationBackupPath() const;
    bool createDirectory(const QString& path) const;
    bool removeDirectory(const QString& path) const;
    QString generateInstanceKey() const;
    
    // 验证方法
    bool validateComponentIntegrity() const;
    bool validateDatabaseIntegrity() const;
    bool validateConfigurationIntegrity() const;
    
    // 日志方法
    void logInfo(const QString& message);
    void logWarning(const QString& message);
    void logError(const QString& error);
    void logDebug(const QString& message);
    
    // 常量
    static const QString DEFAULT_THEME;
    static const QString DEFAULT_LOCALE;
    static const int INITIALIZATION_TIMEOUT = 30000; // 30秒
    static const int PERFORMANCE_UPDATE_INTERVAL = 5000; // 5秒
    static const int UPDATE_CHECK_INTERVAL = 3600000; // 1小时
    static const int TELEMETRY_INTERVAL = 86400000; // 24小时
    static const int SPLASH_SCREEN_TIMEOUT = 5000; // 5秒
    static const int MAX_BACKUP_FILES = 10;
};

#endif // APPLICATIONMANAGER_H 