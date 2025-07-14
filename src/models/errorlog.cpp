#include "errorlog.h"
#include <QJsonObject>
#include <QThread>
#include <QSysInfo>
#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>

ErrorLog::ErrorLog(LogLevel level, const QString& category, const QString& message,
                   const QString& filePath, int lineNumber, const QString& functionName)
    : m_timestamp(QDateTime::currentMSecsSinceEpoch())
    , m_level(level)
    , m_category(category)
    , m_message(message)
    , m_filePath(filePath)
    , m_lineNumber(lineNumber)
    , m_functionName(functionName)
    , m_createdAt(QDateTime::currentDateTime())
{
    setCurrentThreadId();
    setCurrentSystemInfo();
}

QJsonObject ErrorLog::toJson() const
{
    QJsonObject json;
    json["id"] = m_id;
    json["timestamp"] = m_timestamp;
    json["level"] = static_cast<int>(m_level);
    json["category"] = m_category;
    json["message"] = m_message;
    json["filePath"] = m_filePath;
    json["lineNumber"] = m_lineNumber;
    json["functionName"] = m_functionName;
    json["threadId"] = m_threadId;
    json["userId"] = m_userId;
    json["sessionId"] = m_sessionId;
    json["errorCode"] = m_errorCode;
    json["stackTrace"] = m_stackTrace;
    json["systemInfo"] = m_systemInfo;
    json["createdAt"] = m_createdAt.toString(Qt::ISODate);
    
    return json;
}

ErrorLog ErrorLog::fromJson(const QJsonObject& json)
{
    ErrorLog errorLog;
    errorLog.m_id = json["id"].toInt();
    errorLog.m_timestamp = json["timestamp"].toVariant().toLongLong();
    errorLog.m_level = static_cast<LogLevel>(json["level"].toInt());
    errorLog.m_category = json["category"].toString();
    errorLog.m_message = json["message"].toString();
    errorLog.m_filePath = json["filePath"].toString();
    errorLog.m_lineNumber = json["lineNumber"].toInt();
    errorLog.m_functionName = json["functionName"].toString();
    errorLog.m_threadId = json["threadId"].toString();
    errorLog.m_userId = json["userId"].toString();
    errorLog.m_sessionId = json["sessionId"].toString();
    errorLog.m_errorCode = json["errorCode"].toInt();
    errorLog.m_stackTrace = json["stackTrace"].toString();
    errorLog.m_systemInfo = json["systemInfo"].toString();
    errorLog.m_createdAt = QDateTime::fromString(json["createdAt"].toString(), Qt::ISODate);
    
    return errorLog;
}

QString ErrorLog::levelString() const
{
    return levelToString(m_level);
}

QString ErrorLog::toString() const
{
    QString result = QString("[%1] [%2] %3")
        .arg(levelString())
        .arg(m_category)
        .arg(m_message);
    
    if (!m_filePath.isEmpty()) {
        result += QString(" (%1:%2)").arg(m_filePath).arg(m_lineNumber);
    }
    
    if (!m_functionName.isEmpty()) {
        result += QString(" in %1").arg(m_functionName);
    }
    
    if (!m_threadId.isEmpty()) {
        result += QString(" [Thread: %1]").arg(m_threadId);
    }
    
    return result;
}

ErrorLog::LogLevel ErrorLog::stringToLevel(const QString& levelStr)
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

QString ErrorLog::levelToString(LogLevel level)
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

void ErrorLog::setCurrentThreadId()
{
    QThread* currentThread = QThread::currentThread();
    if (currentThread) {
        m_threadId = QString::number(reinterpret_cast<quintptr>(currentThread), 16);
    }
}

void ErrorLog::setCurrentSystemInfo()
{
    QStringList systemInfo;
    systemInfo << QString("OS: %1").arg(QSysInfo::prettyProductName());
    systemInfo << QString("Kernel: %1").arg(QSysInfo::kernelVersion());
    systemInfo << QString("Architecture: %1").arg(QSysInfo::currentCpuArchitecture());
    systemInfo << QString("App: %1 %2").arg(QCoreApplication::applicationName())
                                        .arg(QCoreApplication::applicationVersion());
    
    m_systemInfo = systemInfo.join(" | ");
}

bool ErrorLog::operator==(const ErrorLog& other) const
{
    return m_id == other.m_id &&
           m_timestamp == other.m_timestamp &&
           m_level == other.m_level &&
           m_category == other.m_category &&
           m_message == other.m_message &&
           m_filePath == other.m_filePath &&
           m_lineNumber == other.m_lineNumber &&
           m_functionName == other.m_functionName &&
           m_threadId == other.m_threadId &&
           m_userId == other.m_userId &&
           m_sessionId == other.m_sessionId &&
           m_errorCode == other.m_errorCode &&
           m_stackTrace == other.m_stackTrace &&
           m_systemInfo == other.m_systemInfo &&
           m_createdAt == other.m_createdAt;
}

bool ErrorLog::operator!=(const ErrorLog& other) const
{
    return !(*this == other);
} 