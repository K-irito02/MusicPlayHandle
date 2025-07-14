#ifndef STRUCTUREDLOGGER_H
#define STRUCTUREDLOGGER_H

#include <QObject>
#include <QLoggingCategory>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonDocument>
#include <QMutex>
#include <QFile>
#include <QTextStream>
#include <QTimer>
#include "constants.h"

// 日志类别声明
Q_DECLARE_LOGGING_CATEGORY(tagCategory)
Q_DECLARE_LOGGING_CATEGORY(databaseCategory)
Q_DECLARE_LOGGING_CATEGORY(audioCategory)
Q_DECLARE_LOGGING_CATEGORY(uiCategory)
Q_DECLARE_LOGGING_CATEGORY(networkCategory)

/**
 * @brief 日志级别枚举
 */
enum class LogLevel {
    Debug = 0,
    Info = 1,
    Warning = 2,
    Critical = 3,
    Fatal = 4
};

/**
 * @brief 结构化日志条目
 */
struct LogEntry {
    QDateTime timestamp;
    LogLevel level;
    QString category;
    QString message;
    QString function;
    QString file;
    int line;
    QJsonObject metadata;
    QString threadId;
    
    LogEntry() : level(LogLevel::Info), line(0) {
        timestamp = QDateTime::currentDateTime();
    }
    
    /**
     * @brief 转换为JSON对象
     * @return JSON对象
     */
    QJsonObject toJson() const {
        QJsonObject obj;
        obj["timestamp"] = timestamp.toString(Qt::ISODate);
        obj["level"] = static_cast<int>(level);
        obj["levelName"] = levelToString(level);
        obj["category"] = category;
        obj["message"] = message;
        obj["function"] = function;
        obj["file"] = file;
        obj["line"] = line;
        obj["threadId"] = threadId;
        if (!metadata.isEmpty()) {
            obj["metadata"] = metadata;
        }
        return obj;
    }
    
    /**
     * @brief 转换为可读字符串
     * @return 格式化的日志字符串
     */
    QString toString() const {
        return QString("[%1] [%2] [%3] %4 (%5:%6 in %7)")
               .arg(timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz"))
               .arg(levelToString(level))
               .arg(category)
               .arg(message)
               .arg(file)
               .arg(line)
               .arg(function);
    }
    
private:
    static QString levelToString(LogLevel level) {
        switch (level) {
            case LogLevel::Debug: return "DEBUG";
            case LogLevel::Info: return "INFO";
            case LogLevel::Warning: return "WARNING";
            case LogLevel::Critical: return "CRITICAL";
            case LogLevel::Fatal: return "FATAL";
            default: return "UNKNOWN";
        }
    }
};

/**
 * @brief 结构化日志记录器
 * @details 提供结构化日志记录功能，支持JSON格式输出和元数据
 */
class StructuredLogger : public QObject
{
    Q_OBJECT
    
public:
    explicit StructuredLogger(QObject* parent = nullptr);
    ~StructuredLogger();
    
    // 单例模式
    static StructuredLogger* instance();
    static void cleanup();
    
    /**
     * @brief 初始化日志系统
     * @param logDir 日志目录
     * @param maxFileSize 最大文件大小（字节）
     * @param maxFiles 最大文件数量
     */
    void initialize(const QString& logDir = QString(),
                   qint64 maxFileSize = Constants::Logging::MAX_LOG_FILE_SIZE,
                   int maxFiles = Constants::Logging::MAX_LOG_FILES);
    
    /**
     * @brief 记录日志
     * @param level 日志级别
     * @param category 日志类别
     * @param message 日志消息
     * @param function 函数名
     * @param file 文件名
     * @param line 行号
     * @param metadata 元数据
     */
    void log(LogLevel level, const QString& category, const QString& message,
            const QString& function = QString(), const QString& file = QString(),
            int line = 0, const QJsonObject& metadata = QJsonObject());
    
    /**
     * @brief 设置日志级别过滤
     * @param minLevel 最小日志级别
     */
    void setLogLevel(LogLevel minLevel);
    
    /**
     * @brief 设置类别过滤
     * @param categories 允许的类别列表（空表示允许所有）
     */
    void setCategoryFilter(const QStringList& categories);
    
    /**
     * @brief 启用/禁用控制台输出
     * @param enabled 是否启用
     */
    void setConsoleOutput(bool enabled);
    
    /**
     * @brief 启用/禁用文件输出
     * @param enabled 是否启用
     */
    void setFileOutput(bool enabled);
    
    /**
     * @brief 启用/禁用JSON格式输出
     * @param enabled 是否启用
     */
    void setJsonFormat(bool enabled);
    
    /**
     * @brief 获取日志统计信息
     * @return 统计信息
     */
    QJsonObject getStatistics() const;
    
    /**
     * @brief 刷新日志缓冲区
     */
    void flush();
    
public slots:
    /**
     * @brief 轮转日志文件
     */
    void rotateLogFile();
    
signals:
    /**
     * @brief 新日志条目信号
     * @param entry 日志条目
     */
    void logEntryAdded(const LogEntry& entry);
    
    /**
     * @brief 日志文件轮转信号
     * @param newFileName 新文件名
     */
    void logFileRotated(const QString& newFileName);
    
private:
    /**
     * @brief 检查是否应该记录此日志
     * @param level 日志级别
     * @param category 日志类别
     * @return 是否应该记录
     */
    bool shouldLog(LogLevel level, const QString& category) const;
    
    /**
     * @brief 写入日志到文件
     * @param entry 日志条目
     */
    void writeToFile(const LogEntry& entry);
    
    /**
     * @brief 写入日志到控制台
     * @param entry 日志条目
     */
    void writeToConsole(const LogEntry& entry);
    
    /**
     * @brief 检查是否需要轮转日志文件
     */
    void checkLogRotation();
    
    /**
     * @brief 清理旧日志文件
     */
    void cleanupOldLogFiles();
    
private:
    static StructuredLogger* s_instance;
    mutable QMutex m_mutex;
    
    // 配置
    LogLevel m_minLevel;
    QStringList m_categoryFilter;
    bool m_consoleOutput;
    bool m_fileOutput;
    bool m_jsonFormat;
    
    // 文件管理
    QString m_logDir;
    QString m_currentLogFile;
    QFile* m_logFile;
    QTextStream* m_logStream;
    qint64 m_maxFileSize;
    int m_maxFiles;
    
    // 统计
    QHash<LogLevel, int> m_levelCounts;
    QHash<QString, int> m_categoryCounts;
    int m_totalLogs;
    QDateTime m_startTime;
    
    // 定时器
    QTimer* m_flushTimer;
};

// 便利宏定义
#define LOG_TAG_OPERATION(operation, tagName) \
    StructuredLogger::instance()->log(LogLevel::Info, Constants::Logging::CATEGORY_GENERAL, \
                                     QString("Tag operation: %1").arg(operation), \
                                     Q_FUNC_INFO, __FILE__, __LINE__, \
                                     QJsonObject{{"operation", operation}, {"tagName", tagName}})

#define LOG_DATABASE_QUERY(query, duration) \
    StructuredLogger::instance()->log(LogLevel::Debug, Constants::Logging::CATEGORY_DATABASE, \
                                     QString("Database query executed"), \
                                     Q_FUNC_INFO, __FILE__, __LINE__, \
                                     QJsonObject{{"query", query}, {"duration_ms", duration}})

#define LOG_AUDIO_EVENT(event, details) \
    StructuredLogger::instance()->log(LogLevel::Info, Constants::Logging::CATEGORY_AUDIO, \
                                     QString("Audio event: %1").arg(event), \
                                     Q_FUNC_INFO, __FILE__, __LINE__, \
                                     QJsonObject{{"event", event}, {"details", details}})

#define LOG_UI_ACTION(action, widget) \
    StructuredLogger::instance()->log(LogLevel::Debug, Constants::Logging::CATEGORY_UI, \
                                     QString("UI action: %1").arg(action), \
                                     Q_FUNC_INFO, __FILE__, __LINE__, \
                                     QJsonObject{{"action", action}, {"widget", widget}})

#define LOG_ERROR(category, message, errorCode) \
    StructuredLogger::instance()->log(LogLevel::Critical, category, message, \
                                     Q_FUNC_INFO, __FILE__, __LINE__, \
                                     QJsonObject{{"errorCode", errorCode}})

#define LOG_PERFORMANCE(operation, duration, metadata) \
    StructuredLogger::instance()->log(LogLevel::Info, "performance", \
                                     QString("Performance: %1 took %2ms").arg(operation).arg(duration), \
                                     Q_FUNC_INFO, __FILE__, __LINE__, metadata)

#endif // STRUCTUREDLOGGER_H