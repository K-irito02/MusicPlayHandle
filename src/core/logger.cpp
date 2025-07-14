#include "logger.h"
#include "../database/logdao.h"
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QMutexLocker>
#include <QDebug>
#include <QUuid>
#include <iostream>

// 静态成员变量定义
Logger* Logger::m_instance = nullptr;
QMutex Logger::m_instanceMutex;

Logger::Logger(QObject* parent)
    : QObject(parent)
    , m_initialized(false)
    , m_asyncMode(true)
    , m_logLevel(ErrorLog::LogLevel::Debug)
    , m_logTargets(LogTarget::All)
    , m_maxLogFileSize(10 * 1024 * 1024)  // 10MB
    , m_maxLogFiles(5)
    , m_logFile(nullptr)
    , m_logStream(nullptr)
    , m_currentLogFileSize(0)
    , m_logDao(nullptr)
    , m_processTimer(nullptr)
    , m_sessionId(QUuid::createUuid().toString())
    , m_colorSupported(true)
{
    // 初始化异步处理定时器
    m_processTimer = new QTimer(this);
    m_processTimer->setInterval(100);  // 100ms
    connect(m_processTimer, &QTimer::timeout, this, &Logger::processLogQueue);
}

Logger::~Logger()
{
    shutdown();
}

Logger* Logger::instance()
{
    if (!m_instance) {
        QMutexLocker locker(&m_instanceMutex);
        if (!m_instance) {
            m_instance = new Logger();
        }
    }
    return m_instance;
}

bool Logger::initialize(const QString& logFilePath, qint64 maxLogFileSize, int maxLogFiles)
{
    QMutexLocker locker(&m_mutex);
    
    if (m_initialized) {
        return true;
    }
    
    // 设置配置
    m_maxLogFileSize = maxLogFileSize;
    m_maxLogFiles = maxLogFiles;
    
    // 初始化日志文件
    if (logFilePath.isEmpty()) {
        QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(appDataDir);
        m_logFilePath = appDataDir + "/logs/application.log";
    } else {
        m_logFilePath = logFilePath;
    }
    
    // 创建日志目录
    QFileInfo fileInfo(m_logFilePath);
    QDir logDir = fileInfo.dir();
    if (!logDir.exists()) {
        if (!logDir.mkpath(".")) {
            qCritical() << "Failed to create log directory:" << logDir.path();
            return false;
        }
    }
    
    // 初始化日志文件
    m_logFile = new QFile(m_logFilePath, this);
    if (!m_logFile->open(QIODevice::WriteOnly | QIODevice::Append)) {
        qCritical() << "Failed to open log file:" << m_logFilePath;
        return false;
    }
    
    m_logStream = new QTextStream(m_logFile);
    // Qt6中QTextStream默认使用UTF-8编码，无需设置
    m_currentLogFileSize = m_logFile->size();
    
    // 初始化数据库日志
    initializeLogDao();
    
    // 启动异步处理
    if (m_asyncMode) {
        m_processTimer->start();
    }
    
    m_initialized = true;
    
    // 记录启动日志
    info("Logger initialized successfully", "Logger");
    
    return true;
}

void Logger::shutdown()
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_initialized) {
        return;
    }
    
    // 停止异步处理
    if (m_processTimer) {
        m_processTimer->stop();
    }
    
    // 处理剩余的日志消息
    processLogQueue();
    
    // 关闭文件流
    if (m_logStream) {
        m_logStream->flush();
        delete m_logStream;
        m_logStream = nullptr;
    }
    
    if (m_logFile) {
        m_logFile->close();
        delete m_logFile;
        m_logFile = nullptr;
    }
    
    m_initialized = false;
    
    qDebug() << "Logger shutdown completed";
}

void Logger::debug(const QString& message, const QString& category,
                   const QString& filePath, int lineNumber, const QString& functionName)
{
    if (!shouldLog(ErrorLog::LogLevel::Debug)) {
        return;
    }
    
    ErrorLog errorLog(ErrorLog::LogLevel::Debug, category, message, filePath, lineNumber, functionName);
    errorLog.setSessionId(m_sessionId);
    
    logMessage(errorLog);
}

void Logger::info(const QString& message, const QString& category,
                  const QString& filePath, int lineNumber, const QString& functionName)
{
    if (!shouldLog(ErrorLog::LogLevel::Info)) {
        return;
    }
    
    ErrorLog errorLog(ErrorLog::LogLevel::Info, category, message, filePath, lineNumber, functionName);
    errorLog.setSessionId(m_sessionId);
    
    logMessage(errorLog);
}

void Logger::warning(const QString& message, const QString& category,
                     const QString& filePath, int lineNumber, const QString& functionName)
{
    if (!shouldLog(ErrorLog::LogLevel::Warning)) {
        return;
    }
    
    ErrorLog errorLog(ErrorLog::LogLevel::Warning, category, message, filePath, lineNumber, functionName);
    errorLog.setSessionId(m_sessionId);
    
    logMessage(errorLog);
}

void Logger::error(const QString& message, const QString& category,
                   const QString& filePath, int lineNumber, const QString& functionName)
{
    if (!shouldLog(ErrorLog::LogLevel::Error)) {
        return;
    }
    
    ErrorLog errorLog(ErrorLog::LogLevel::Error, category, message, filePath, lineNumber, functionName);
    errorLog.setSessionId(m_sessionId);
    
    logMessage(errorLog);
}

void Logger::critical(const QString& message, const QString& category,
                      const QString& filePath, int lineNumber, const QString& functionName)
{
    if (!shouldLog(ErrorLog::LogLevel::Critical)) {
        return;
    }
    
    ErrorLog errorLog(ErrorLog::LogLevel::Critical, category, message, filePath, lineNumber, functionName);
    errorLog.setSessionId(m_sessionId);
    
    logMessage(errorLog);
}

void Logger::logSystem(SystemLog::LogLevel level, const QString& message,
                       const QString& category, const QString& component, const QString& operation)
{
    if (!shouldLog(level)) {
        return;
    }
    
    SystemLog systemLog(level, category, message, component, operation);
    systemLog.setSessionId(m_sessionId);
    
    logMessage(systemLog);
}

void Logger::logPerformance(const QString& operation, qint64 duration,
                           const QString& component, qint64 memoryUsage, double cpuUsage)
{
    SystemLog systemLog(SystemLog::LogLevel::Info, "Performance", 
                       QString("Operation completed: %1").arg(operation), 
                       component, operation);
    systemLog.setSessionId(m_sessionId);
    systemLog.setPerformanceMetrics(duration, memoryUsage, cpuUsage);
    
    logMessage(systemLog);
}

void Logger::logMessage(const ErrorLog& errorLog)
{
    if (!m_initialized) {
        return;
    }
    
    if (isCategoryFiltered(errorLog.category())) {
        return;
    }
    
    if (m_asyncMode) {
        QMutexLocker locker(&m_mutex);
        m_logQueue.enqueue(LogMessage(std::make_shared<ErrorLog>(errorLog)));
    } else {
        processErrorLog(errorLog);
    }
}

void Logger::logMessage(const SystemLog& systemLog)
{
    if (!m_initialized) {
        return;
    }
    
    if (isCategoryFiltered(systemLog.category())) {
        return;
    }
    
    if (m_asyncMode) {
        QMutexLocker locker(&m_mutex);
        m_logQueue.enqueue(LogMessage(std::make_shared<SystemLog>(systemLog)));
    } else {
        processSystemLog(systemLog);
    }
}

void Logger::processLogQueue()
{
    QMutexLocker locker(&m_mutex);
    
    while (!m_logQueue.isEmpty()) {
        LogMessage logMessage = m_logQueue.dequeue();
        
        if (logMessage.type == LogMessage::ErrorLogType) {
            processErrorLog(*logMessage.errorLog);
        } else if (logMessage.type == LogMessage::SystemLogType) {
            processSystemLog(*logMessage.systemLog);
        }
    }
}

void Logger::processErrorLog(const ErrorLog& errorLog)
{
    QString formattedMessage = formatErrorLog(errorLog);
    
    // 输出到控制台
    if (m_logTargets & LogTarget::Console) {
        outputToConsole(formattedMessage, errorLog.level());
    }
    
    // 输出到文件
    if (m_logTargets & LogTarget::File) {
        outputToFile(formattedMessage);
    }
    
    // 输出到数据库
    if (m_logTargets & LogTarget::Database) {
        outputToDatabase(errorLog);
    }
    
    emit logMessage(formattedMessage);
}

void Logger::processSystemLog(const SystemLog& systemLog)
{
    QString formattedMessage = formatSystemLog(systemLog);
    
    // 输出到控制台
    if (m_logTargets & LogTarget::Console) {
        // 将SystemLog级别转换为ErrorLog级别用于控制台输出
        ErrorLog::LogLevel level = static_cast<ErrorLog::LogLevel>(static_cast<int>(systemLog.level()));
        outputToConsole(formattedMessage, level);
    }
    
    // 输出到文件
    if (m_logTargets & LogTarget::File) {
        outputToFile(formattedMessage);
    }
    
    // 输出到数据库
    if (m_logTargets & LogTarget::Database) {
        outputToDatabase(systemLog);
    }
    
    emit logMessage(formattedMessage);
}

bool Logger::shouldLog(ErrorLog::LogLevel level) const
{
    return static_cast<int>(level) >= static_cast<int>(m_logLevel);
}

bool Logger::shouldLog(SystemLog::LogLevel level) const
{
    return static_cast<int>(level) >= static_cast<int>(m_logLevel);
}

void Logger::initializeLogDao()
{
    if (!m_logDao) {
        m_logDao = new LogDao(this);
        connect(m_logDao, &LogDao::databaseError, this, &Logger::onDatabaseError);
    }
}

void Logger::onDatabaseError(const QString& error)
{
    // 避免无限递归，直接输出到控制台
    qCritical() << "Database log error:" << error;
    emit errorOccurred(error);
}

// 输出方法实现

void Logger::outputToConsole(const QString& message, ErrorLog::LogLevel level)
{
    if (m_colorSupported) {
        QString colorCode = getLogLevelColor(level);
        QString resetCode = "\033[0m";
        std::cout << colorCode.toStdString() << message.toStdString() << resetCode.toStdString() << std::endl;
    } else {
        std::cout << message.toStdString() << std::endl;
    }
}

void Logger::outputToFile(const QString& message)
{
    if (!m_logStream) {
        return;
    }
    
    *m_logStream << message << Qt::endl;
    m_logStream->flush();
    
    m_currentLogFileSize += message.toUtf8().size() + 1;  // +1 for newline
    
    // 检查文件大小并轮换
    if (m_currentLogFileSize > m_maxLogFileSize) {
        checkAndRotateLogFile();
    }
}

void Logger::outputToDatabase(const ErrorLog& errorLog)
{
    if (!m_logDao) {
        return;
    }
    
    // 异步插入数据库
    QMetaObject::invokeMethod(m_logDao, [this, errorLog]() {
        m_logDao->insertErrorLog(errorLog);
    }, Qt::QueuedConnection);
}

void Logger::outputToDatabase(const SystemLog& systemLog)
{
    if (!m_logDao) {
        return;
    }
    
    // 异步插入数据库
    QMetaObject::invokeMethod(m_logDao, [this, systemLog]() {
        m_logDao->insertSystemLog(systemLog);
    }, Qt::QueuedConnection);
}

// 格式化方法实现

QString Logger::formatErrorLog(const ErrorLog& errorLog) const
{
    QString timestamp = QDateTime::fromMSecsSinceEpoch(errorLog.timestamp()).toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString level = errorLog.levelString();
    QString category = errorLog.category();
    QString message = errorLog.message();
    QString threadId = errorLog.threadId();
    
    QString formattedMessage = QString("[%1] [%2] [%3] %4")
        .arg(timestamp)
        .arg(level)
        .arg(category)
        .arg(message);
    
    if (!errorLog.filePath().isEmpty()) {
        QString fileName = QFileInfo(errorLog.filePath()).fileName();
        formattedMessage += QString(" (%1:%2)").arg(fileName).arg(errorLog.lineNumber());
    }
    
    if (!errorLog.functionName().isEmpty()) {
        formattedMessage += QString(" in %1").arg(errorLog.functionName());
    }
    
    if (!threadId.isEmpty()) {
        formattedMessage += QString(" [Thread: %1]").arg(threadId);
    }
    
    return formattedMessage;
}

QString Logger::formatSystemLog(const SystemLog& systemLog) const
{
    QString timestamp = QDateTime::fromMSecsSinceEpoch(systemLog.timestamp()).toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString level = systemLog.levelString();
    QString category = systemLog.category();
    QString message = systemLog.message();
    QString component = systemLog.component();
    QString operation = systemLog.operation();
    QString threadId = systemLog.threadId();
    
    QString formattedMessage = QString("[%1] [%2] [%3] %4")
        .arg(timestamp)
        .arg(level)
        .arg(category)
        .arg(message);
    
    if (!component.isEmpty()) {
        formattedMessage += QString(" [Component: %1]").arg(component);
    }
    
    if (!operation.isEmpty()) {
        formattedMessage += QString(" [Operation: %1]").arg(operation);
    }
    
    if (systemLog.duration() > 0) {
        formattedMessage += QString(" [Duration: %1ms]").arg(systemLog.duration());
    }
    
    if (systemLog.memoryUsage() > 0) {
        formattedMessage += QString(" [Memory: %1KB]").arg(systemLog.memoryUsage() / 1024);
    }
    
    if (systemLog.cpuUsage() > 0) {
        formattedMessage += QString(" [CPU: %1%]").arg(systemLog.cpuUsage(), 0, 'f', 2);
    }
    
    if (!threadId.isEmpty()) {
        formattedMessage += QString(" [Thread: %1]").arg(threadId);
    }
    
    return formattedMessage;
}

QString Logger::getLogLevelColor(ErrorLog::LogLevel level) const
{
    switch (level) {
    case ErrorLog::LogLevel::Debug:
        return "\033[36m";  // Cyan
    case ErrorLog::LogLevel::Info:
        return "\033[32m";  // Green
    case ErrorLog::LogLevel::Warning:
        return "\033[33m";  // Yellow
    case ErrorLog::LogLevel::Error:
        return "\033[31m";  // Red
    case ErrorLog::LogLevel::Critical:
        return "\033[35m";  // Magenta
    default:
        return "\033[0m";   // Reset
    }
}

// 配置方法实现

void Logger::setLogLevel(ErrorLog::LogLevel level)
{
    QMutexLocker locker(&m_mutex);
    m_logLevel = level;
}

void Logger::setLogTargets(LogTargets targets)
{
    QMutexLocker locker(&m_mutex);
    m_logTargets = targets;
}

void Logger::setMaxLogFileSize(qint64 size)
{
    QMutexLocker locker(&m_mutex);
    m_maxLogFileSize = size;
}

void Logger::setMaxLogFiles(int count)
{
    QMutexLocker locker(&m_mutex);
    m_maxLogFiles = count;
}

void Logger::setConsoleOutput(bool enabled)
{
    QMutexLocker locker(&m_mutex);
    if (enabled) {
        m_logTargets |= LogTarget::Console;
    } else {
        m_logTargets &= ~LogTarget::Console;
    }
}

void Logger::setDatabaseOutput(bool enabled)
{
    QMutexLocker locker(&m_mutex);
    if (enabled) {
        m_logTargets |= LogTarget::Database;
    } else {
        m_logTargets &= ~LogTarget::Database;
    }
}

void Logger::setFileOutput(bool enabled)
{
    QMutexLocker locker(&m_mutex);
    if (enabled) {
        m_logTargets |= LogTarget::File;
    } else {
        m_logTargets &= ~LogTarget::File;
    }
}

void Logger::setAsyncMode(bool enabled)
{
    QMutexLocker locker(&m_mutex);
    m_asyncMode = enabled;
    
    if (enabled && m_initialized) {
        m_processTimer->start();
    } else if (!enabled && m_processTimer) {
        m_processTimer->stop();
        processLogQueue();  // 处理剩余消息
    }
}

// 查询方法实现

ErrorLog::LogLevel Logger::logLevel() const
{
    QMutexLocker locker(&m_mutex);
    return m_logLevel;
}

Logger::LogTargets Logger::logTargets() const
{
    QMutexLocker locker(&m_mutex);
    return m_logTargets;
}

qint64 Logger::maxLogFileSize() const
{
    QMutexLocker locker(&m_mutex);
    return m_maxLogFileSize;
}

int Logger::maxLogFiles() const
{
    QMutexLocker locker(&m_mutex);
    return m_maxLogFiles;
}

bool Logger::isConsoleOutputEnabled() const
{
    QMutexLocker locker(&m_mutex);
    return m_logTargets & LogTarget::Console;
}

bool Logger::isDatabaseOutputEnabled() const
{
    QMutexLocker locker(&m_mutex);
    return m_logTargets & LogTarget::Database;
}

bool Logger::isFileOutputEnabled() const
{
    QMutexLocker locker(&m_mutex);
    return m_logTargets & LogTarget::File;
}

bool Logger::isAsyncMode() const
{
    QMutexLocker locker(&m_mutex);
    return m_asyncMode;
}

// 文件操作方法实现

void Logger::rotateLogFile()
{
    QMutexLocker locker(&m_mutex);
    checkAndRotateLogFile();
}

void Logger::clearLogFile()
{
    QMutexLocker locker(&m_mutex);
    
    if (m_logFile && m_logFile->isOpen()) {
        m_logFile->resize(0);
        m_currentLogFileSize = 0;
    }
}

QString Logger::currentLogFilePath() const
{
    QMutexLocker locker(&m_mutex);
    return m_logFilePath;
}

qint64 Logger::currentLogFileSize() const
{
    QMutexLocker locker(&m_mutex);
    return m_currentLogFileSize;
}

void Logger::checkAndRotateLogFile()
{
    if (!m_logFile || !m_logStream) {
        return;
    }
    
    // 关闭当前文件
    m_logStream->flush();
    delete m_logStream;
    m_logStream = nullptr;
    
    m_logFile->close();
    
    // 生成新的文件名
    QString newFileName = generateLogFileName();
    QFileInfo fileInfo(m_logFilePath);
    QString newFilePath = fileInfo.dir().absoluteFilePath(newFileName);
    
    // 重命名当前文件
    QFile::rename(m_logFilePath, newFilePath);
    
    // 打开新文件
    if (m_logFile->open(QIODevice::WriteOnly | QIODevice::Append)) {
        m_logStream = new QTextStream(m_logFile);
        // Qt6中QTextStream默认使用UTF-8编码，无需设置
        m_currentLogFileSize = 0;
    }
    
    // 清理旧文件
    cleanupOldLogFiles();
    
    emit logFileRotated(newFilePath);
}

void Logger::cleanupOldLogFiles()
{
    QFileInfo fileInfo(m_logFilePath);
    QDir logDir = fileInfo.dir();
    QString baseName = fileInfo.baseName();
    
    // 查找所有相关的日志文件
    QStringList nameFilters;
    nameFilters << baseName + "_*.log";
    
    QFileInfoList logFiles = logDir.entryInfoList(nameFilters, QDir::Files, QDir::Time);
    
    // 保留最新的 m_maxLogFiles 个文件
    while (logFiles.size() > m_maxLogFiles) {
        QFileInfo oldestFile = logFiles.takeLast();
        QFile::remove(oldestFile.absoluteFilePath());
    }
}

QString Logger::generateLogFileName() const
{
    QFileInfo fileInfo(m_logFilePath);
    QString baseName = fileInfo.baseName();
    QString suffix = fileInfo.suffix();
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    
    return QString("%1_%2.%3").arg(baseName).arg(timestamp).arg(suffix);
}

// 过滤器方法实现

void Logger::addCategoryFilter(const QString& category)
{
    QMutexLocker locker(&m_mutex);
    if (!m_categoryFilters.contains(category)) {
        m_categoryFilters.append(category);
    }
}

void Logger::removeCategoryFilter(const QString& category)
{
    QMutexLocker locker(&m_mutex);
    m_categoryFilters.removeAll(category);
}

void Logger::clearCategoryFilters()
{
    QMutexLocker locker(&m_mutex);
    m_categoryFilters.clear();
}

bool Logger::isCategoryFiltered(const QString& category) const
{
    QMutexLocker locker(&m_mutex);
    return !m_categoryFilters.isEmpty() && !m_categoryFilters.contains(category);
}

// #include "logger.moc" 