#include "structuredlogger.h"
#include <QDir>
#include <QStandardPaths>
#include <QThread>
#include <QCoreApplication>
#include <QDebug>
#include <QFileInfo>
#include <QStringConverter>
#include <QRegularExpression>

// 日志类别定义
Q_LOGGING_CATEGORY(tagCategory, "tag")
Q_LOGGING_CATEGORY(databaseCategory, "database")
Q_LOGGING_CATEGORY(audioCategory, "audio")
Q_LOGGING_CATEGORY(uiCategory, "ui")
Q_LOGGING_CATEGORY(networkCategory, "network")

// 静态成员初始化
StructuredLogger* StructuredLogger::s_instance = nullptr;

StructuredLogger::StructuredLogger(QObject* parent)
    : QObject(parent)
    , m_minLevel(LogLevel::Debug)
    , m_consoleOutput(true)
    , m_fileOutput(true)
    , m_jsonFormat(false)
    , m_logFile(nullptr)
    , m_logStream(nullptr)
    , m_maxFileSize(Constants::Logging::MAX_LOG_FILE_SIZE)
    , m_maxFiles(Constants::Logging::MAX_LOG_FILES)
    , m_totalLogs(0)
    , m_startTime(QDateTime::currentDateTime())
    , m_flushTimer(new QTimer(this))
{
    // 设置定时刷新
    m_flushTimer->setInterval(5000); // 5秒刷新一次
    connect(m_flushTimer, &QTimer::timeout, this, &StructuredLogger::flush);
    m_flushTimer->start();
}

StructuredLogger::~StructuredLogger()
{
    flush();
    if (m_logStream) {
        delete m_logStream;
    }
    if (m_logFile) {
        m_logFile->close();
        delete m_logFile;
    }
}

StructuredLogger* StructuredLogger::instance()
{
    static QMutex mutex;
    QMutexLocker locker(&mutex);
    
    if (!s_instance) {
        s_instance = new StructuredLogger();
        // 默认初始化
        s_instance->initialize();
    }
    return s_instance;
}

void StructuredLogger::cleanup()
{
    if (s_instance) {
        delete s_instance;
        s_instance = nullptr;
    }
}

void StructuredLogger::initialize(const QString& logDir, qint64 maxFileSize, int maxFiles)
{
    QMutexLocker locker(&m_mutex);
    
    m_maxFileSize = maxFileSize;
    m_maxFiles = maxFiles;
    
    // 确定日志目录
    if (logDir.isEmpty()) {
        m_logDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/logs";
    } else {
        m_logDir = logDir;
    }
    
    // 创建日志目录
    QDir dir;
    if (!dir.exists(m_logDir)) {
        dir.mkpath(m_logDir);
    }
    
    // 生成日志文件名
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    m_currentLogFile = m_logDir + QString("/app_%1.log").arg(timestamp);
    
    // 打开日志文件
    if (m_logFile) {
        m_logFile->close();
        delete m_logFile;
    }
    if (m_logStream) {
        delete m_logStream;
    }
    
    m_logFile = new QFile(m_currentLogFile);
    if (m_logFile->open(QIODevice::WriteOnly | QIODevice::Append)) {
        m_logStream = new QTextStream(m_logFile);
        m_logStream->setEncoding(QStringConverter::Utf8);
    } else {
        qWarning() << "Failed to open log file:" << m_currentLogFile;
        m_fileOutput = false;
    }
    
    // 清理旧日志文件
    cleanupOldLogFiles();
    
    // 记录初始化日志
    log(LogLevel::Info, "system", "StructuredLogger initialized", Q_FUNC_INFO, __FILE__, __LINE__,
        QJsonObject{{"logDir", m_logDir}, {"maxFileSize", m_maxFileSize}, {"maxFiles", m_maxFiles}});
}

void StructuredLogger::log(LogLevel level, const QString& category, const QString& message,
                          const QString& function, const QString& file, int line,
                          const QJsonObject& metadata)
{
    if (!shouldLog(level, category)) {
        return;
    }
    
    QMutexLocker locker(&m_mutex);
    
    // 创建日志条目
    LogEntry entry;
    entry.level = level;
    entry.category = category;
    entry.message = message;
    entry.function = function;
    entry.file = QFileInfo(file).fileName(); // 只保留文件名
    entry.line = line;
    entry.metadata = metadata;
    entry.threadId = QString::number(reinterpret_cast<quintptr>(QThread::currentThreadId()), 16);
    
    // 更新统计
    m_levelCounts[level]++;
    m_categoryCounts[category]++;
    m_totalLogs++;
    
    // 输出到控制台
    if (m_consoleOutput) {
        writeToConsole(entry);
    }
    
    // 输出到文件
    if (m_fileOutput && m_logStream) {
        writeToFile(entry);
        checkLogRotation();
    }
    
    // 发送信号
    emit logEntryAdded(entry);
}

void StructuredLogger::setLogLevel(LogLevel minLevel)
{
    QMutexLocker locker(&m_mutex);
    m_minLevel = minLevel;
}

void StructuredLogger::setCategoryFilter(const QStringList& categories)
{
    QMutexLocker locker(&m_mutex);
    m_categoryFilter = categories;
}

void StructuredLogger::setConsoleOutput(bool enabled)
{
    QMutexLocker locker(&m_mutex);
    m_consoleOutput = enabled;
}

void StructuredLogger::setFileOutput(bool enabled)
{
    QMutexLocker locker(&m_mutex);
    m_fileOutput = enabled;
}

void StructuredLogger::setJsonFormat(bool enabled)
{
    QMutexLocker locker(&m_mutex);
    m_jsonFormat = enabled;
}

QJsonObject StructuredLogger::getStatistics() const
{
    QMutexLocker locker(&m_mutex);
    
    QJsonObject stats;
    stats["totalLogs"] = m_totalLogs;
    stats["startTime"] = m_startTime.toString(Qt::ISODate);
    stats["uptime"] = m_startTime.secsTo(QDateTime::currentDateTime());
    
    // 级别统计
    QJsonObject levelStats;
    for (auto it = m_levelCounts.begin(); it != m_levelCounts.end(); ++it) {
        QString levelName;
        switch (it.key()) {
            case LogLevel::Debug: levelName = "debug"; break;
            case LogLevel::Info: levelName = "info"; break;
            case LogLevel::Warning: levelName = "warning"; break;
            case LogLevel::Critical: levelName = "critical"; break;
            case LogLevel::Fatal: levelName = "fatal"; break;
        }
        levelStats[levelName] = it.value();
    }
    stats["levelCounts"] = levelStats;
    
    // 类别统计
    QJsonObject categoryStats;
    for (auto it = m_categoryCounts.begin(); it != m_categoryCounts.end(); ++it) {
        categoryStats[it.key()] = it.value();
    }
    stats["categoryCounts"] = categoryStats;
    
    // 文件信息
    if (m_logFile) {
        stats["currentLogFile"] = m_currentLogFile;
        stats["logFileSize"] = m_logFile->size();
    }
    
    return stats;
}

void StructuredLogger::flush()
{
    QMutexLocker locker(&m_mutex);
    if (m_logStream) {
        m_logStream->flush();
    }
    if (m_logFile) {
        m_logFile->flush();
    }
}

void StructuredLogger::rotateLogFile()
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_logFile) {
        return;
    }
    
    // 关闭当前文件
    m_logFile->close();
    delete m_logStream;
    delete m_logFile;
    
    // 生成新文件名
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    m_currentLogFile = m_logDir + QString("/app_%1.log").arg(timestamp);
    
    // 打开新文件
    m_logFile = new QFile(m_currentLogFile);
    if (m_logFile->open(QIODevice::WriteOnly | QIODevice::Append)) {
        m_logStream = new QTextStream(m_logFile);
        m_logStream->setEncoding(QStringConverter::Utf8);
        
        // 记录轮转日志
        LogEntry entry;
        entry.level = LogLevel::Info;
        entry.category = "system";
        entry.message = "Log file rotated";
        entry.metadata = QJsonObject{{"newFile", m_currentLogFile}};
        writeToFile(entry);
        
        emit logFileRotated(m_currentLogFile);
    } else {
        qWarning() << "Failed to open new log file:" << m_currentLogFile;
        m_fileOutput = false;
    }
    
    // 清理旧文件
    cleanupOldLogFiles();
}

bool StructuredLogger::shouldLog(LogLevel level, const QString& category) const
{
    // 检查级别过滤
    if (level < m_minLevel) {
        return false;
    }
    
    // 检查类别过滤
    if (!m_categoryFilter.isEmpty() && !m_categoryFilter.contains(category)) {
        return false;
    }
    
    return true;
}

void StructuredLogger::writeToFile(const LogEntry& entry)
{
    if (!m_logStream) {
        return;
    }
    
    if (m_jsonFormat) {
        // JSON格式输出
        QJsonDocument doc(entry.toJson());
        *m_logStream << doc.toJson(QJsonDocument::Compact);
    } else {
        // 文本格式输出
        *m_logStream << entry.toString() << Qt::endl;
        
        // 如果有元数据，也输出
        if (!entry.metadata.isEmpty()) {
            QJsonDocument metaDoc(entry.metadata);
            *m_logStream << "  Metadata: " << metaDoc.toJson(QJsonDocument::Compact);
        }
    }
}

void StructuredLogger::writeToConsole(const LogEntry& entry)
{
    QString output;
    if (m_jsonFormat) {
        QJsonDocument doc(entry.toJson());
        output = doc.toJson(QJsonDocument::Compact);
    } else {
        output = entry.toString();
    }
    
    // 根据级别选择输出流
    switch (entry.level) {
        case LogLevel::Debug:
            qDebug().noquote() << output;
            break;
        case LogLevel::Info:
            qInfo().noquote() << output;
            break;
        case LogLevel::Warning:
            qWarning().noquote() << output;
            break;
        case LogLevel::Critical:
        case LogLevel::Fatal:
            qCritical().noquote() << output;
            break;
    }
}

void StructuredLogger::checkLogRotation()
{
    if (m_logFile && m_logFile->size() > m_maxFileSize) {
        rotateLogFile();
    }
}

void StructuredLogger::cleanupOldLogFiles()
{
    QDir logDir(m_logDir);
    QStringList filters;
    filters << "app_*.log";
    
    QFileInfoList logFiles = logDir.entryInfoList(filters, QDir::Files, QDir::Time | QDir::Reversed);
    
    // 删除超过最大数量的文件
    while (logFiles.size() > m_maxFiles) {
        QFileInfo oldestFile = logFiles.takeLast();
        if (QFile::remove(oldestFile.absoluteFilePath())) {
            qDebug() << "Removed old log file:" << oldestFile.fileName();
        }
    }
}