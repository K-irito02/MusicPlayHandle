#pragma once

#include <QObject>
#include <QMutex>
#include <QTextStream>
#include <QFile>
#include <QDateTime>
#include <QQueue>
#include <QTimer>
#include <QThread>
#include <memory>
#include "../models/errorlog.h"
#include "../models/systemlog.h"

class LogDao;

/**
 * @brief 日志管理器类
 * 
 * 线程安全的单例模式日志管理器，负责：
 * 1. 日志输出到控制台
 * 2. 日志存储到数据库
 * 3. 日志文件输出
 * 4. 日志格式化
 * 5. 日志级别过滤
 */
class Logger : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 日志输出目标
     */
    enum LogTarget {
        Console = 0x01,     // 控制台输出
        Database = 0x02,    // 数据库存储
        File = 0x04,        // 文件输出
        All = Console | Database | File
    };
    Q_DECLARE_FLAGS(LogTargets, LogTarget)

    /**
     * @brief 获取单例实例
     * @return Logger实例
     */
    static Logger* instance();

    /**
     * @brief 初始化日志系统
     * @param logFilePath 日志文件路径
     * @param maxLogFileSize 最大日志文件大小（字节）
     * @param maxLogFiles 最大日志文件数量
     * @return 初始化成功返回true
     */
    bool initialize(const QString& logFilePath = QString(), 
                   qint64 maxLogFileSize = 10 * 1024 * 1024,  // 10MB
                   int maxLogFiles = 5);

    /**
     * @brief 关闭日志系统
     */
    void shutdown();

    // 错误日志记录方法
    void debug(const QString& message, const QString& category = "General",
               const QString& filePath = QString(), int lineNumber = 0,
               const QString& functionName = QString());
    
    void info(const QString& message, const QString& category = "General",
              const QString& filePath = QString(), int lineNumber = 0,
              const QString& functionName = QString());
    
    void warning(const QString& message, const QString& category = "General",
                 const QString& filePath = QString(), int lineNumber = 0,
                 const QString& functionName = QString());
    
    void error(const QString& message, const QString& category = "General",
               const QString& filePath = QString(), int lineNumber = 0,
               const QString& functionName = QString());
    
    void critical(const QString& message, const QString& category = "General",
                  const QString& filePath = QString(), int lineNumber = 0,
                  const QString& functionName = QString());

    // 系统日志记录方法
    void logSystem(SystemLog::LogLevel level, const QString& message,
                   const QString& category = "System",
                   const QString& component = QString(),
                   const QString& operation = QString());

    // 性能日志记录方法
    void logPerformance(const QString& operation, qint64 duration,
                       const QString& component = QString(),
                       qint64 memoryUsage = 0, double cpuUsage = 0.0);

    // 配置方法
    void setLogLevel(ErrorLog::LogLevel level);
    void setLogTargets(LogTargets targets);
    void setMaxLogFileSize(qint64 size);
    void setMaxLogFiles(int count);
    void setConsoleOutput(bool enabled);
    void setDatabaseOutput(bool enabled);
    void setFileOutput(bool enabled);
    void setAsyncMode(bool enabled);

    // 查询方法
    ErrorLog::LogLevel logLevel() const;
    LogTargets logTargets() const;
    qint64 maxLogFileSize() const;
    int maxLogFiles() const;
    bool isConsoleOutputEnabled() const;
    bool isDatabaseOutputEnabled() const;
    bool isFileOutputEnabled() const;
    bool isAsyncMode() const;

    // 文件操作
    void rotateLogFile();
    void clearLogFile();
    QString currentLogFilePath() const;
    qint64 currentLogFileSize() const;

    // 过滤器
    void addCategoryFilter(const QString& category);
    void removeCategoryFilter(const QString& category);
    void clearCategoryFilters();
    bool isCategoryFiltered(const QString& category) const;

signals:
    void logMessage(const QString& message);
    void errorOccurred(const QString& error);
    void logFileRotated(const QString& newFilePath);

private slots:
    void processLogQueue();
    void onDatabaseError(const QString& error);

private:
    /**
     * @brief 构造函数（私有）
     */
    explicit Logger(QObject* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~Logger() override;

    /**
     * @brief 日志消息结构
     */
    struct LogMessage {
        enum Type { ErrorLogType, SystemLogType };
        Type type;
        std::shared_ptr<ErrorLog> errorLog;
        std::shared_ptr<SystemLog> systemLog;
        
        LogMessage(std::shared_ptr<ErrorLog> log) : type(ErrorLogType), errorLog(log) {}
        LogMessage(std::shared_ptr<SystemLog> log) : type(SystemLogType), systemLog(log) {}
    };

    /**
     * @brief 记录日志消息
     * @param errorLog 错误日志对象
     */
    void logMessage(const ErrorLog& errorLog);

    /**
     * @brief 记录系统日志消息
     * @param systemLog 系统日志对象
     */
    void logMessage(const SystemLog& systemLog);

    /**
     * @brief 输出到控制台
     * @param message 格式化的消息
     * @param level 日志级别
     */
    void outputToConsole(const QString& message, ErrorLog::LogLevel level);

    /**
     * @brief 输出到文件
     * @param message 格式化的消息
     */
    void outputToFile(const QString& message);

    /**
     * @brief 输出到数据库
     * @param errorLog 错误日志对象
     */
    void outputToDatabase(const ErrorLog& errorLog);

    /**
     * @brief 输出到数据库
     * @param systemLog 系统日志对象
     */
    void outputToDatabase(const SystemLog& systemLog);

    /**
     * @brief 检查日志级别
     * @param level 日志级别
     * @return 是否应该记录此级别的日志
     */
    bool shouldLog(ErrorLog::LogLevel level) const;

    /**
     * @brief 检查日志级别
     * @param level 日志级别
     * @return 是否应该记录此级别的日志
     */
    bool shouldLog(SystemLog::LogLevel level) const;

    /**
     * @brief 格式化错误日志消息
     * @param errorLog 错误日志对象
     * @return 格式化的消息
     */
    QString formatErrorLog(const ErrorLog& errorLog) const;

    /**
     * @brief 格式化系统日志消息
     * @param systemLog 系统日志对象
     * @return 格式化的消息
     */
    QString formatSystemLog(const SystemLog& systemLog) const;

    /**
     * @brief 获取日志级别的颜色代码（用于控制台输出）
     * @param level 日志级别
     * @return ANSI颜色代码
     */
    QString getLogLevelColor(ErrorLog::LogLevel level) const;

    /**
     * @brief 检查并轮换日志文件
     */
    void checkAndRotateLogFile();

    /**
     * @brief 清理旧的日志文件
     */
    void cleanupOldLogFiles();

    /**
     * @brief 创建新的日志文件名
     * @return 新的日志文件名
     */
    QString generateLogFileName() const;

    /**
     * @brief 初始化日志DAO
     */
    void initializeLogDao();

    /**
     * @brief 处理错误日志消息
     * @param errorLog 错误日志对象
     */
    void processErrorLog(const ErrorLog& errorLog);

    /**
     * @brief 处理系统日志消息
     * @param systemLog 系统日志对象
     */
    void processSystemLog(const SystemLog& systemLog);

private:
    static Logger* m_instance;
    static QMutex m_instanceMutex;

    mutable QMutex m_mutex;
    bool m_initialized;
    bool m_asyncMode;

    // 日志配置
    ErrorLog::LogLevel m_logLevel;
    LogTargets m_logTargets;
    qint64 m_maxLogFileSize;
    int m_maxLogFiles;

    // 文件输出
    QString m_logFilePath;
    QFile* m_logFile;
    QTextStream* m_logStream;
    qint64 m_currentLogFileSize;

    // 数据库输出
    LogDao* m_logDao;

    // 异步处理
    QQueue<LogMessage> m_logQueue;
    QTimer* m_processTimer;

    // 过滤器
    QStringList m_categoryFilters;

    // 会话ID（用于跟踪日志）
    QString m_sessionId;

    // 颜色支持
    bool m_colorSupported;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Logger::LogTargets)

// 便利宏定义
#define LOG_DEBUG(message, category) \
    Logger::instance()->debug(message, category, __FILE__, __LINE__, __FUNCTION__)

#define LOG_INFO(message, category) \
    Logger::instance()->info(message, category, __FILE__, __LINE__, __FUNCTION__)

#define LOG_WARNING(message, category) \
    Logger::instance()->warning(message, category, __FILE__, __LINE__, __FUNCTION__)

#define LOG_ERROR(message, category) \
    Logger::instance()->error(message, category, __FILE__, __LINE__, __FUNCTION__)

#define LOG_CRITICAL(message, category) \
    Logger::instance()->critical(message, category, __FILE__, __LINE__, __FUNCTION__)

#define LOG_SYSTEM(level, message, category, component, operation) \
    Logger::instance()->logSystem(level, message, category, component, operation)

#define LOG_PERFORMANCE(operation, duration, component, memoryUsage, cpuUsage) \
    Logger::instance()->logPerformance(operation, duration, component, memoryUsage, cpuUsage)

// End of logger.h