#include "systemlog.h"
#include <QJsonObject>
#include <QThread>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QDebug>

SystemLog::SystemLog(LogLevel level, const QString& category, const QString& message,
                     const QString& component, const QString& operation)
    : m_timestamp(QDateTime::currentMSecsSinceEpoch())
    , m_level(level)
    , m_category(category)
    , m_message(message)
    , m_component(component)
    , m_operation(operation)
    , m_createdAt(QDateTime::currentDateTime())
{
    setCurrentThreadId();
}

QJsonObject SystemLog::toJson() const
{
    QJsonObject json;
    json["id"] = m_id;
    json["timestamp"] = m_timestamp;
    json["level"] = static_cast<int>(m_level);
    json["category"] = m_category;
    json["message"] = m_message;
    json["component"] = m_component;
    json["operation"] = m_operation;
    json["duration"] = m_duration;
    json["memoryUsage"] = m_memoryUsage;
    json["cpuUsage"] = m_cpuUsage;
    json["threadId"] = m_threadId;
    json["sessionId"] = m_sessionId;
    json["metadata"] = m_metadata;
    json["createdAt"] = m_createdAt.toString(Qt::ISODate);
    
    return json;
}

SystemLog SystemLog::fromJson(const QJsonObject& json)
{
    SystemLog systemLog;
    systemLog.m_id = json["id"].toInt();
    systemLog.m_timestamp = json["timestamp"].toVariant().toLongLong();
    systemLog.m_level = static_cast<LogLevel>(json["level"].toInt());
    systemLog.m_category = json["category"].toString();
    systemLog.m_message = json["message"].toString();
    systemLog.m_component = json["component"].toString();
    systemLog.m_operation = json["operation"].toString();
    systemLog.m_duration = json["duration"].toVariant().toLongLong();
    systemLog.m_memoryUsage = json["memoryUsage"].toVariant().toLongLong();
    systemLog.m_cpuUsage = json["cpuUsage"].toDouble();
    systemLog.m_threadId = json["threadId"].toString();
    systemLog.m_sessionId = json["sessionId"].toString();
    systemLog.m_metadata = json["metadata"].toString();
    systemLog.m_createdAt = QDateTime::fromString(json["createdAt"].toString(), Qt::ISODate);
    
    return systemLog;
}

QString SystemLog::levelString() const
{
    return levelToString(m_level);
}

QString SystemLog::toString() const
{
    QString result = QString("[%1] [%2] %3")
        .arg(levelString())
        .arg(m_category)
        .arg(m_message);
    
    if (!m_component.isEmpty()) {
        result += QString(" [Component: %1]").arg(m_component);
    }
    
    if (!m_operation.isEmpty()) {
        result += QString(" [Operation: %1]").arg(m_operation);
    }
    
    if (m_duration > 0) {
        result += QString(" [Duration: %1ms]").arg(m_duration);
    }
    
    if (!m_threadId.isEmpty()) {
        result += QString(" [Thread: %1]").arg(m_threadId);
    }
    
    return result;
}

SystemLog::LogLevel SystemLog::stringToLevel(const QString& levelStr)
{
    if (levelStr.compare("Debug", Qt::CaseInsensitive) == 0) {
        return LogLevel::Debug;
    } else if (levelStr.compare("Info", Qt::CaseInsensitive) == 0) {
        return LogLevel::Info;
    } else if (levelStr.compare("Warning", Qt::CaseInsensitive) == 0) {
        return LogLevel::Warning;
    } else if (levelStr.compare("Error", Qt::CaseInsensitive) == 0) {
        return LogLevel::Error;
    } else if (levelStr.compare("Critical", Qt::CaseInsensitive) == 0) {
        return LogLevel::Critical;
    }
    
    return LogLevel::Info;
}

QString SystemLog::levelToString(LogLevel level)
{
    switch (level) {
    case LogLevel::Debug:
        return "Debug";
    case LogLevel::Info:
        return "Info";
    case LogLevel::Warning:
        return "Warning";
    case LogLevel::Error:
        return "Error";
    case LogLevel::Critical:
        return "Critical";
    default:
        return "Info";
    }
}

void SystemLog::setCurrentThreadId()
{
    QThread* currentThread = QThread::currentThread();
    if (currentThread) {
        m_threadId = QString::number(reinterpret_cast<quintptr>(currentThread), 16);
    }
}

void SystemLog::setPerformanceMetrics(qint64 duration, qint64 memoryUsage, double cpuUsage)
{
    m_duration = duration;
    m_memoryUsage = memoryUsage;
    m_cpuUsage = cpuUsage;
}

void SystemLog::addMetadata(const QString& key, const QString& value)
{
    QJsonObject metadataObj;
    
    // 如果已存在元数据，解析现有的
    if (!m_metadata.isEmpty()) {
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(m_metadata.toUtf8(), &error);
        if (error.error == QJsonParseError::NoError && doc.isObject()) {
            metadataObj = doc.object();
        }
    }
    
    // 添加新的元数据
    metadataObj[key] = value;
    
    // 序列化回字符串
    QJsonDocument doc(metadataObj);
    m_metadata = doc.toJson(QJsonDocument::Compact);
}

bool SystemLog::operator==(const SystemLog& other) const
{
    return m_id == other.m_id &&
           m_timestamp == other.m_timestamp &&
           m_level == other.m_level &&
           m_category == other.m_category &&
           m_message == other.m_message &&
           m_component == other.m_component &&
           m_operation == other.m_operation &&
           m_duration == other.m_duration &&
           m_memoryUsage == other.m_memoryUsage &&
           qFuzzyCompare(m_cpuUsage, other.m_cpuUsage) &&
           m_threadId == other.m_threadId &&
           m_sessionId == other.m_sessionId &&
           m_metadata == other.m_metadata &&
           m_createdAt == other.m_createdAt;
}

bool SystemLog::operator!=(const SystemLog& other) const
{
    return !(*this == other);
} 