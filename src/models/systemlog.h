#pragma once

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QJsonObject>
#include <QMetaType>

/**
 * @brief 系统日志模型类
 * 
 * 用于存储和管理应用程序的系统日志信息
 */
class SystemLog
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
    SystemLog() = default;
    
    /**
     * @brief 构造函数
     * @param level 日志级别
     * @param category 日志分类
     * @param message 日志消息
     * @param component 组件名称
     * @param operation 操作名称
     */
    SystemLog(LogLevel level, const QString& category, const QString& message,
              const QString& component = QString(), const QString& operation = QString());
    
    /**
     * @brief 拷贝构造函数
     */
    SystemLog(const SystemLog& other) = default;
    
    /**
     * @brief 赋值操作符
     */
    SystemLog& operator=(const SystemLog& other) = default;
    
    /**
     * @brief 析构函数
     */
    ~SystemLog() = default;

    // Getters
    int id() const { return m_id; }
    qint64 timestamp() const { return m_timestamp; }
    LogLevel level() const { return m_level; }
    QString category() const { return m_category; }
    QString message() const { return m_message; }
    QString component() const { return m_component; }
    QString operation() const { return m_operation; }
    qint64 duration() const { return m_duration; }
    qint64 memoryUsage() const { return m_memoryUsage; }
    double cpuUsage() const { return m_cpuUsage; }
    QString threadId() const { return m_threadId; }
    QString sessionId() const { return m_sessionId; }
    QString metadata() const { return m_metadata; }
    QDateTime createdAt() const { return m_createdAt; }

    // Setters
    void setId(int id) { m_id = id; }
    void setTimestamp(qint64 timestamp) { m_timestamp = timestamp; }
    void setLevel(LogLevel level) { m_level = level; }
    void setCategory(const QString& category) { m_category = category; }
    void setMessage(const QString& message) { m_message = message; }
    void setComponent(const QString& component) { m_component = component; }
    void setOperation(const QString& operation) { m_operation = operation; }
    void setDuration(qint64 duration) { m_duration = duration; }
    void setMemoryUsage(qint64 memoryUsage) { m_memoryUsage = memoryUsage; }
    void setCpuUsage(double cpuUsage) { m_cpuUsage = cpuUsage; }
    void setThreadId(const QString& threadId) { m_threadId = threadId; }
    void setSessionId(const QString& sessionId) { m_sessionId = sessionId; }
    void setMetadata(const QString& metadata) { m_metadata = metadata; }
    void setCreatedAt(const QDateTime& createdAt) { m_createdAt = createdAt; }

    /**
     * @brief 转换为JSON对象
     * @return JSON对象
     */
    QJsonObject toJson() const;

    /**
     * @brief 从JSON对象构造
     * @param json JSON对象
     * @return SystemLog实例
     */
    static SystemLog fromJson(const QJsonObject& json);

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
     * @brief 设置性能指标
     * @param duration 执行时长（毫秒）
     * @param memoryUsage 内存使用量（字节）
     * @param cpuUsage CPU使用率（百分比）
     */
    void setPerformanceMetrics(qint64 duration, qint64 memoryUsage, double cpuUsage);

    /**
     * @brief 添加元数据
     * @param key 键
     * @param value 值
     */
    void addMetadata(const QString& key, const QString& value);

    /**
     * @brief 比较操作符
     */
    bool operator==(const SystemLog& other) const;
    bool operator!=(const SystemLog& other) const;

private:
    int m_id = 0;
    qint64 m_timestamp = 0;
    LogLevel m_level = LogLevel::Info;
    QString m_category;
    QString m_message;
    QString m_component;
    QString m_operation;
    qint64 m_duration = 0;
    qint64 m_memoryUsage = 0;
    double m_cpuUsage = 0.0;
    QString m_threadId;
    QString m_sessionId;
    QString m_metadata;
    QDateTime m_createdAt;
};

Q_DECLARE_METATYPE(SystemLog)
Q_DECLARE_METATYPE(SystemLog::LogLevel) 