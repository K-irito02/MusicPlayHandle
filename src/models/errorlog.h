#pragma once

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QJsonObject>
#include <QMetaType>

/**
 * @brief 错误日志模型类
 * 
 * 用于存储和管理应用程序的错误日志信息
 */
class ErrorLog
{
public:
    /**
     * @brief 日志级别枚举
     */
    enum class LogLevel {
        Debug = 0,
        Info = 1,
        Warning = 2,
        Error = 3,
        Critical = 4
    };

    /**
     * @brief 构造函数
     */
    ErrorLog() = default;
    
    /**
     * @brief 构造函数
     * @param level 日志级别
     * @param category 日志分类
     * @param message 日志消息
     * @param filePath 源文件路径
     * @param lineNumber 行号
     * @param functionName 函数名
     */
    ErrorLog(LogLevel level, const QString& category, const QString& message,
             const QString& filePath = QString(), int lineNumber = 0,
             const QString& functionName = QString());
    
    /**
     * @brief 拷贝构造函数
     */
    ErrorLog(const ErrorLog& other) = default;
    
    /**
     * @brief 赋值操作符
     */
    ErrorLog& operator=(const ErrorLog& other) = default;
    
    /**
     * @brief 析构函数
     */
    ~ErrorLog() = default;

    // Getters
    int id() const { return m_id; }
    qint64 timestamp() const { return m_timestamp; }
    LogLevel level() const { return m_level; }
    QString category() const { return m_category; }
    QString message() const { return m_message; }
    QString filePath() const { return m_filePath; }
    int lineNumber() const { return m_lineNumber; }
    QString functionName() const { return m_functionName; }
    QString threadId() const { return m_threadId; }
    QString userId() const { return m_userId; }
    QString sessionId() const { return m_sessionId; }
    int errorCode() const { return m_errorCode; }
    QString stackTrace() const { return m_stackTrace; }
    QString systemInfo() const { return m_systemInfo; }
    QDateTime createdAt() const { return m_createdAt; }

    // Setters
    void setId(int id) { m_id = id; }
    void setTimestamp(qint64 timestamp) { m_timestamp = timestamp; }
    void setLevel(LogLevel level) { m_level = level; }
    void setCategory(const QString& category) { m_category = category; }
    void setMessage(const QString& message) { m_message = message; }
    void setFilePath(const QString& filePath) { m_filePath = filePath; }
    void setLineNumber(int lineNumber) { m_lineNumber = lineNumber; }
    void setFunctionName(const QString& functionName) { m_functionName = functionName; }
    void setThreadId(const QString& threadId) { m_threadId = threadId; }
    void setUserId(const QString& userId) { m_userId = userId; }
    void setSessionId(const QString& sessionId) { m_sessionId = sessionId; }
    void setErrorCode(int errorCode) { m_errorCode = errorCode; }
    void setStackTrace(const QString& stackTrace) { m_stackTrace = stackTrace; }
    void setSystemInfo(const QString& systemInfo) { m_systemInfo = systemInfo; }
    void setCreatedAt(const QDateTime& createdAt) { m_createdAt = createdAt; }

    /**
     * @brief 转换为JSON对象
     * @return JSON对象
     */
    QJsonObject toJson() const;

    /**
     * @brief 从JSON对象构造
     * @param json JSON对象
     * @return ErrorLog实例
     */
    static ErrorLog fromJson(const QJsonObject& json);

    /**
     * @brief 获取日志级别字符串
     * @return 日志级别字符串
     */
    QString levelString() const;

    /**
     * @brief 获取格式化的日志信息
     * @return 格式化的日志信息
     */
    QString toString() const;

    /**
     * @brief 字符串转日志级别
     * @param levelStr 日志级别字符串
     * @return 日志级别
     */
    static LogLevel stringToLevel(const QString& levelStr);

    /**
     * @brief 日志级别转字符串
     * @param level 日志级别
     * @return 日志级别字符串
     */
    static QString levelToString(LogLevel level);

    /**
     * @brief 设置当前线程ID
     */
    void setCurrentThreadId();

    /**
     * @brief 设置当前系统信息
     */
    void setCurrentSystemInfo();

    /**
     * @brief 比较操作符
     */
    bool operator==(const ErrorLog& other) const;
    bool operator!=(const ErrorLog& other) const;

private:
    int m_id = 0;
    qint64 m_timestamp = 0;
    LogLevel m_level = LogLevel::Info;
    QString m_category;
    QString m_message;
    QString m_filePath;
    int m_lineNumber = 0;
    QString m_functionName;
    QString m_threadId;
    QString m_userId;
    QString m_sessionId;
    int m_errorCode = 0;
    QString m_stackTrace;
    QString m_systemInfo;
    QDateTime m_createdAt;
};

Q_DECLARE_METATYPE(ErrorLog)
Q_DECLARE_METATYPE(ErrorLog::LogLevel) 