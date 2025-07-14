#include "logdao.h"
#include "databasemanager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDateTime>
#include <QDebug>

// SQL语句常量定义
const QString LogDao::INSERT_ERROR_LOG_SQL = R"(
    INSERT INTO error_logs (
        timestamp, level, category, message, file_path, line_number, 
        function_name, thread_id, user_id, session_id, error_code, 
        stack_trace, system_info, created_at
    ) VALUES (
        :timestamp, :level, :category, :message, :file_path, :line_number,
        :function_name, :thread_id, :user_id, :session_id, :error_code,
        :stack_trace, :system_info, :created_at
    )
)";

const QString LogDao::INSERT_SYSTEM_LOG_SQL = R"(
    INSERT INTO system_logs (
        timestamp, level, category, message, component, operation, 
        duration, memory_usage, cpu_usage, thread_id, session_id, 
        metadata, created_at
    ) VALUES (
        :timestamp, :level, :category, :message, :component, :operation,
        :duration, :memory_usage, :cpu_usage, :thread_id, :session_id,
        :metadata, :created_at
    )
)";

const QString LogDao::SELECT_ERROR_LOG_BY_ID_SQL = R"(
    SELECT id, timestamp, level, category, message, file_path, line_number,
           function_name, thread_id, user_id, session_id, error_code,
           stack_trace, system_info, created_at
    FROM error_logs WHERE id = :id
)";

const QString LogDao::SELECT_SYSTEM_LOG_BY_ID_SQL = R"(
    SELECT id, timestamp, level, category, message, component, operation,
           duration, memory_usage, cpu_usage, thread_id, session_id,
           metadata, created_at
    FROM system_logs WHERE id = :id
)";

const QString LogDao::DELETE_ERROR_LOG_SQL = "DELETE FROM error_logs WHERE id = :id";
const QString LogDao::DELETE_SYSTEM_LOG_SQL = "DELETE FROM system_logs WHERE id = :id";

const QString LogDao::CLEANUP_ERROR_LOGS_SQL = R"(
    DELETE FROM error_logs 
    WHERE created_at < datetime('now', '-' || :days || ' days')
)";

const QString LogDao::CLEANUP_SYSTEM_LOGS_SQL = R"(
    DELETE FROM system_logs 
    WHERE created_at < datetime('now', '-' || :days || ' days')
)";

const QString LogDao::COUNT_ERROR_LOGS_SQL = "SELECT COUNT(*) FROM error_logs";
const QString LogDao::COUNT_SYSTEM_LOGS_SQL = "SELECT COUNT(*) FROM system_logs";
const QString LogDao::CLEAR_ERROR_LOGS_SQL = "DELETE FROM error_logs";
const QString LogDao::CLEAR_SYSTEM_LOGS_SQL = "DELETE FROM system_logs";

LogDao::LogDao(QObject* parent)
    : QObject(parent)
{
}

LogDao::~LogDao() = default;

QSqlDatabase LogDao::getDatabase()
{
    DatabaseManager* dbManager = DatabaseManager::instance();
    if (!dbManager->isValid()) {
        qCritical() << "Database not connected in LogDao";
        return QSqlDatabase();
    }
    return dbManager->database();
}

void LogDao::logError(const QString& error)
{
    qCritical() << "LogDao error:" << error;
    emit databaseError(error);
}

int LogDao::insertErrorLog(const ErrorLog& errorLog)
{
    QMutexLocker locker(&m_mutex);
    
    QSqlDatabase db = getDatabase();
    if (!db.isValid()) {
        logError("Invalid database connection");
        return -1;
    }
    
    QSqlQuery query(db);
    if (!query.prepare(INSERT_ERROR_LOG_SQL)) {
        logError("Failed to prepare insert error log query: " + query.lastError().text());
        return -1;
    }
    
    bindErrorLogParams(query, errorLog);
    
    if (!query.exec()) {
        logError("Failed to insert error log: " + query.lastError().text());
        return -1;
    }
    
    int id = query.lastInsertId().toInt();
    emit errorLogInserted(id, errorLog);
    
    return id;
}

ErrorLog LogDao::getErrorLog(int id)
{
    QMutexLocker locker(&m_mutex);
    
    QSqlDatabase db = getDatabase();
    if (!db.isValid()) {
        logError("Invalid database connection");
        return ErrorLog();
    }
    
    QSqlQuery query(db);
    if (!query.prepare(SELECT_ERROR_LOG_BY_ID_SQL)) {
        logError("Failed to prepare select error log query: " + query.lastError().text());
        return ErrorLog();
    }
    
    query.bindValue(":id", id);
    
    if (!query.exec()) {
        logError("Failed to select error log: " + query.lastError().text());
        return ErrorLog();
    }
    
    if (query.next()) {
        return createErrorLogFromQuery(query);
    }
    
    return ErrorLog();
}

bool LogDao::deleteErrorLog(int id)
{
    QMutexLocker locker(&m_mutex);
    
    QSqlDatabase db = getDatabase();
    if (!db.isValid()) {
        logError("Invalid database connection");
        return false;
    }
    
    QSqlQuery query(db);
    if (!query.prepare(DELETE_ERROR_LOG_SQL)) {
        logError("Failed to prepare delete error log query: " + query.lastError().text());
        return false;
    }
    
    query.bindValue(":id", id);
    
    if (!query.exec()) {
        logError("Failed to delete error log: " + query.lastError().text());
        return false;
    }
    
    return query.numRowsAffected() > 0;
}

int LogDao::cleanupErrorLogs(int days)
{
    QMutexLocker locker(&m_mutex);
    
    QSqlDatabase db = getDatabase();
    if (!db.isValid()) {
        logError("Invalid database connection");
        return -1;
    }
    
    QSqlQuery query(db);
    if (!query.prepare(CLEANUP_ERROR_LOGS_SQL)) {
        logError("Failed to prepare cleanup error logs query: " + query.lastError().text());
        return -1;
    }
    
    query.bindValue(":days", days);
    
    if (!query.exec()) {
        logError("Failed to cleanup error logs: " + query.lastError().text());
        return -1;
    }
    
    return query.numRowsAffected();
}

void LogDao::bindErrorLogParams(QSqlQuery& query, const ErrorLog& errorLog)
{
    query.bindValue(":timestamp", errorLog.timestamp());
    query.bindValue(":level", ErrorLog::levelToString(errorLog.level()));
    query.bindValue(":category", errorLog.category());
    query.bindValue(":message", errorLog.message());
    query.bindValue(":file_path", errorLog.filePath());
    query.bindValue(":line_number", errorLog.lineNumber());
    query.bindValue(":function_name", errorLog.functionName());
    query.bindValue(":thread_id", errorLog.threadId());
    query.bindValue(":user_id", errorLog.userId());
    query.bindValue(":session_id", errorLog.sessionId());
    query.bindValue(":error_code", errorLog.errorCode());
    query.bindValue(":stack_trace", errorLog.stackTrace());
    query.bindValue(":system_info", errorLog.systemInfo());
    query.bindValue(":created_at", errorLog.createdAt().toString(Qt::ISODate));
}

ErrorLog LogDao::createErrorLogFromQuery(const QSqlQuery& query)
{
    ErrorLog errorLog;
    
    errorLog.setId(query.value("id").toInt());
    errorLog.setTimestamp(query.value("timestamp").toLongLong());
    errorLog.setLevel(ErrorLog::stringToLevel(query.value("level").toString()));
    errorLog.setCategory(query.value("category").toString());
    errorLog.setMessage(query.value("message").toString());
    errorLog.setFilePath(query.value("file_path").toString());
    errorLog.setLineNumber(query.value("line_number").toInt());
    errorLog.setFunctionName(query.value("function_name").toString());
    errorLog.setThreadId(query.value("thread_id").toString());
    errorLog.setUserId(query.value("user_id").toString());
    errorLog.setSessionId(query.value("session_id").toString());
    errorLog.setErrorCode(query.value("error_code").toInt());
    errorLog.setStackTrace(query.value("stack_trace").toString());
    errorLog.setSystemInfo(query.value("system_info").toString());
    errorLog.setCreatedAt(QDateTime::fromString(query.value("created_at").toString(), Qt::ISODate));
    
    return errorLog;
}

// 系统日志相关方法实现

int LogDao::insertSystemLog(const SystemLog& systemLog)
{
    QMutexLocker locker(&m_mutex);
    
    QSqlDatabase db = getDatabase();
    if (!db.isValid()) {
        logError("Invalid database connection");
        return -1;
    }
    
    QSqlQuery query(db);
    if (!query.prepare(INSERT_SYSTEM_LOG_SQL)) {
        logError("Failed to prepare insert system log query: " + query.lastError().text());
        return -1;
    }
    
    bindSystemLogParams(query, systemLog);
    
    if (!query.exec()) {
        logError("Failed to insert system log: " + query.lastError().text());
        return -1;
    }
    
    int id = query.lastInsertId().toInt();
    emit systemLogInserted(id, systemLog);
    
    return id;
}

SystemLog LogDao::getSystemLog(int id)
{
    QMutexLocker locker(&m_mutex);
    
    QSqlDatabase db = getDatabase();
    if (!db.isValid()) {
        logError("Invalid database connection");
        return SystemLog();
    }
    
    QSqlQuery query(db);
    if (!query.prepare(SELECT_SYSTEM_LOG_BY_ID_SQL)) {
        logError("Failed to prepare select system log query: " + query.lastError().text());
        return SystemLog();
    }
    
    query.bindValue(":id", id);
    
    if (!query.exec()) {
        logError("Failed to select system log: " + query.lastError().text());
        return SystemLog();
    }
    
    if (query.next()) {
        return createSystemLogFromQuery(query);
    }
    
    return SystemLog();
}

bool LogDao::deleteSystemLog(int id)
{
    QMutexLocker locker(&m_mutex);
    
    QSqlDatabase db = getDatabase();
    if (!db.isValid()) {
        logError("Invalid database connection");
        return false;
    }
    
    QSqlQuery query(db);
    if (!query.prepare(DELETE_SYSTEM_LOG_SQL)) {
        logError("Failed to prepare delete system log query: " + query.lastError().text());
        return false;
    }
    
    query.bindValue(":id", id);
    
    if (!query.exec()) {
        logError("Failed to delete system log: " + query.lastError().text());
        return false;
    }
    
    return query.numRowsAffected() > 0;
}

int LogDao::cleanupSystemLogs(int days)
{
    QMutexLocker locker(&m_mutex);
    
    QSqlDatabase db = getDatabase();
    if (!db.isValid()) {
        logError("Invalid database connection");
        return -1;
    }
    
    QSqlQuery query(db);
    if (!query.prepare(CLEANUP_SYSTEM_LOGS_SQL)) {
        logError("Failed to prepare cleanup system logs query: " + query.lastError().text());
        return -1;
    }
    
    query.bindValue(":days", days);
    
    if (!query.exec()) {
        logError("Failed to cleanup system logs: " + query.lastError().text());
        return -1;
    }
    
    return query.numRowsAffected();
}

void LogDao::bindSystemLogParams(QSqlQuery& query, const SystemLog& systemLog)
{
    query.bindValue(":timestamp", systemLog.timestamp());
    query.bindValue(":level", SystemLog::levelToString(systemLog.level()));
    query.bindValue(":category", systemLog.category());
    query.bindValue(":message", systemLog.message());
    query.bindValue(":component", systemLog.component());
    query.bindValue(":operation", systemLog.operation());
    query.bindValue(":duration", systemLog.duration());
    query.bindValue(":memory_usage", systemLog.memoryUsage());
    query.bindValue(":cpu_usage", systemLog.cpuUsage());
    query.bindValue(":thread_id", systemLog.threadId());
    query.bindValue(":session_id", systemLog.sessionId());
    query.bindValue(":metadata", systemLog.metadata());
    query.bindValue(":created_at", systemLog.createdAt().toString(Qt::ISODate));
}

SystemLog LogDao::createSystemLogFromQuery(const QSqlQuery& query)
{
    SystemLog systemLog;
    
    systemLog.setId(query.value("id").toInt());
    systemLog.setTimestamp(query.value("timestamp").toLongLong());
    systemLog.setLevel(SystemLog::stringToLevel(query.value("level").toString()));
    systemLog.setCategory(query.value("category").toString());
    systemLog.setMessage(query.value("message").toString());
    systemLog.setComponent(query.value("component").toString());
    systemLog.setOperation(query.value("operation").toString());
    systemLog.setDuration(query.value("duration").toLongLong());
    systemLog.setMemoryUsage(query.value("memory_usage").toLongLong());
    systemLog.setCpuUsage(query.value("cpu_usage").toDouble());
    systemLog.setThreadId(query.value("thread_id").toString());
    systemLog.setSessionId(query.value("session_id").toString());
    systemLog.setMetadata(query.value("metadata").toString());
    systemLog.setCreatedAt(QDateTime::fromString(query.value("created_at").toString(), Qt::ISODate));
    
    return systemLog;
}

// 查询和统计方法实现

QList<ErrorLog> LogDao::queryErrorLogs(
    ErrorLog::LogLevel level, const QString& category,
    const QDateTime& startTime, const QDateTime& endTime,
    int limit, int offset)
{
    QMutexLocker locker(&m_mutex);
    
    QList<ErrorLog> logs;
    QSqlDatabase db = getDatabase();
    if (!db.isValid()) {
        logError("Invalid database connection");
        return logs;
    }
    
    QString sql = buildErrorLogQuery(level, category, startTime, endTime, limit, offset);
    QSqlQuery query(db);
    
    if (!query.prepare(sql)) {
        logError("Failed to prepare query error logs: " + query.lastError().text());
        return logs;
    }
    
    // 绑定参数
    if (level != ErrorLog::LogLevel::Debug) {
        query.bindValue(":level", ErrorLog::levelToString(level));
    }
    if (!category.isEmpty()) {
        query.bindValue(":category", category);
    }
    if (startTime.isValid()) {
        query.bindValue(":start_time", startTime.toString(Qt::ISODate));
    }
    if (endTime.isValid()) {
        query.bindValue(":end_time", endTime.toString(Qt::ISODate));
    }
    query.bindValue(":limit", limit);
    query.bindValue(":offset", offset);
    
    if (!query.exec()) {
        logError("Failed to execute query error logs: " + query.lastError().text());
        return logs;
    }
    
    while (query.next()) {
        logs.append(createErrorLogFromQuery(query));
    }
    
    return logs;
}

QList<SystemLog> LogDao::querySystemLogs(
    SystemLog::LogLevel level, const QString& category,
    const QString& component, const QString& operation,
    const QDateTime& startTime, const QDateTime& endTime,
    int limit, int offset)
{
    QMutexLocker locker(&m_mutex);
    
    QList<SystemLog> logs;
    QSqlDatabase db = getDatabase();
    if (!db.isValid()) {
        logError("Invalid database connection");
        return logs;
    }
    
    QString sql = buildSystemLogQuery(level, category, component, operation, 
                                     startTime, endTime, limit, offset);
    QSqlQuery query(db);
    
    if (!query.prepare(sql)) {
        logError("Failed to prepare query system logs: " + query.lastError().text());
        return logs;
    }
    
    // 绑定参数
    if (level != SystemLog::LogLevel::Debug) {
        query.bindValue(":level", SystemLog::levelToString(level));
    }
    if (!category.isEmpty()) {
        query.bindValue(":category", category);
    }
    if (!component.isEmpty()) {
        query.bindValue(":component", component);
    }
    if (!operation.isEmpty()) {
        query.bindValue(":operation", operation);
    }
    if (startTime.isValid()) {
        query.bindValue(":start_time", startTime.toString(Qt::ISODate));
    }
    if (endTime.isValid()) {
        query.bindValue(":end_time", endTime.toString(Qt::ISODate));
    }
    query.bindValue(":limit", limit);
    query.bindValue(":offset", offset);
    
    if (!query.exec()) {
        logError("Failed to execute query system logs: " + query.lastError().text());
        return logs;
    }
    
    while (query.next()) {
        logs.append(createSystemLogFromQuery(query));
    }
    
    return logs;
}

int LogDao::getLogCount(const QString& tableName)
{
    QMutexLocker locker(&m_mutex);
    
    QSqlDatabase db = getDatabase();
    if (!db.isValid()) {
        logError("Invalid database connection");
        return -1;
    }
    
    QString sql = QString("SELECT COUNT(*) FROM %1").arg(tableName);
    QSqlQuery query(db);
    
    if (!query.exec(sql)) {
        logError("Failed to get log count: " + query.lastError().text());
        return -1;
    }
    
    if (query.next()) {
        return query.value(0).toInt();
    }
    
    return 0;
}

bool LogDao::clearErrorLogs()
{
    QMutexLocker locker(&m_mutex);
    
    QSqlDatabase db = getDatabase();
    if (!db.isValid()) {
        logError("Invalid database connection");
        return false;
    }
    
    QSqlQuery query(db);
    if (!query.exec(CLEAR_ERROR_LOGS_SQL)) {
        logError("Failed to clear error logs: " + query.lastError().text());
        return false;
    }
    
    return true;
}

bool LogDao::clearSystemLogs()
{
    QMutexLocker locker(&m_mutex);
    
    QSqlDatabase db = getDatabase();
    if (!db.isValid()) {
        logError("Invalid database connection");
        return false;
    }
    
    QSqlQuery query(db);
    if (!query.exec(CLEAR_SYSTEM_LOGS_SQL)) {
        logError("Failed to clear system logs: " + query.lastError().text());
        return false;
    }
    
    return true;
}

// 统计方法实现

QMap<ErrorLog::LogLevel, int> LogDao::getErrorLogStatistics(
    const QDateTime& startTime, const QDateTime& endTime)
{
    QMutexLocker locker(&m_mutex);
    
    QMap<ErrorLog::LogLevel, int> statistics;
    QSqlDatabase db = getDatabase();
    if (!db.isValid()) {
        logError("Invalid database connection");
        return statistics;
    }
    
    QString sql = "SELECT level, COUNT(*) FROM error_logs";
    QStringList whereConditions;
    
    if (startTime.isValid()) {
        whereConditions << "created_at >= :start_time";
    }
    if (endTime.isValid()) {
        whereConditions << "created_at <= :end_time";
    }
    
    if (!whereConditions.isEmpty()) {
        sql += " WHERE " + whereConditions.join(" AND ");
    }
    
    sql += " GROUP BY level";
    
    QSqlQuery query(db);
    if (!query.prepare(sql)) {
        logError("Failed to prepare error log statistics query: " + query.lastError().text());
        return statistics;
    }
    
    if (startTime.isValid()) {
        query.bindValue(":start_time", startTime.toString(Qt::ISODate));
    }
    if (endTime.isValid()) {
        query.bindValue(":end_time", endTime.toString(Qt::ISODate));
    }
    
    if (!query.exec()) {
        logError("Failed to execute error log statistics query: " + query.lastError().text());
        return statistics;
    }
    
    while (query.next()) {
        QString levelStr = query.value("level").toString();
        int count = query.value(1).toInt();
        ErrorLog::LogLevel level = ErrorLog::stringToLevel(levelStr);
        statistics[level] = count;
    }
    
    return statistics;
}

QMap<SystemLog::LogLevel, int> LogDao::getSystemLogStatistics(
    const QDateTime& startTime, const QDateTime& endTime)
{
    QMutexLocker locker(&m_mutex);
    
    QMap<SystemLog::LogLevel, int> statistics;
    QSqlDatabase db = getDatabase();
    if (!db.isValid()) {
        logError("Invalid database connection");
        return statistics;
    }
    
    QString sql = "SELECT level, COUNT(*) FROM system_logs";
    QStringList whereConditions;
    
    if (startTime.isValid()) {
        whereConditions << "created_at >= :start_time";
    }
    if (endTime.isValid()) {
        whereConditions << "created_at <= :end_time";
    }
    
    if (!whereConditions.isEmpty()) {
        sql += " WHERE " + whereConditions.join(" AND ");
    }
    
    sql += " GROUP BY level";
    
    QSqlQuery query(db);
    if (!query.prepare(sql)) {
        logError("Failed to prepare system log statistics query: " + query.lastError().text());
        return statistics;
    }
    
    if (startTime.isValid()) {
        query.bindValue(":start_time", startTime.toString(Qt::ISODate));
    }
    if (endTime.isValid()) {
        query.bindValue(":end_time", endTime.toString(Qt::ISODate));
    }
    
    if (!query.exec()) {
        logError("Failed to execute system log statistics query: " + query.lastError().text());
        return statistics;
    }
    
    while (query.next()) {
        QString levelStr = query.value("level").toString();
        int count = query.value(1).toInt();
        SystemLog::LogLevel level = SystemLog::stringToLevel(levelStr);
        statistics[level] = count;
    }
    
    return statistics;
}

// 批量操作方法实现

int LogDao::batchInsertErrorLogs(const QList<ErrorLog>& errorLogs)
{
    QMutexLocker locker(&m_mutex);
    
    QSqlDatabase db = getDatabase();
    if (!db.isValid()) {
        logError("Invalid database connection");
        return 0;
    }
    
    if (errorLogs.isEmpty()) {
        return 0;
    }
    
    db.transaction();
    
    QSqlQuery query(db);
    if (!query.prepare(INSERT_ERROR_LOG_SQL)) {
        logError("Failed to prepare batch insert error logs query: " + query.lastError().text());
        db.rollback();
        return 0;
    }
    
    int successCount = 0;
    for (const ErrorLog& errorLog : errorLogs) {
        bindErrorLogParams(query, errorLog);
        
        if (query.exec()) {
            successCount++;
        } else {
            logError("Failed to insert error log in batch: " + query.lastError().text());
        }
    }
    
    if (successCount > 0) {
        db.commit();
    } else {
        db.rollback();
    }
    
    return successCount;
}

int LogDao::batchInsertSystemLogs(const QList<SystemLog>& systemLogs)
{
    QMutexLocker locker(&m_mutex);
    
    QSqlDatabase db = getDatabase();
    if (!db.isValid()) {
        logError("Invalid database connection");
        return 0;
    }
    
    if (systemLogs.isEmpty()) {
        return 0;
    }
    
    db.transaction();
    
    QSqlQuery query(db);
    if (!query.prepare(INSERT_SYSTEM_LOG_SQL)) {
        logError("Failed to prepare batch insert system logs query: " + query.lastError().text());
        db.rollback();
        return 0;
    }
    
    int successCount = 0;
    for (const SystemLog& systemLog : systemLogs) {
        bindSystemLogParams(query, systemLog);
        
        if (query.exec()) {
            successCount++;
        } else {
            logError("Failed to insert system log in batch: " + query.lastError().text());
        }
    }
    
    if (successCount > 0) {
        db.commit();
    } else {
        db.rollback();
    }
    
    return successCount;
}

// SQL构建方法实现

QString LogDao::buildErrorLogQuery(
    ErrorLog::LogLevel level, const QString& category,
    const QDateTime& startTime, const QDateTime& endTime,
    int limit, int offset)
{
    QString sql = R"(
        SELECT id, timestamp, level, category, message, file_path, line_number,
               function_name, thread_id, user_id, session_id, error_code,
               stack_trace, system_info, created_at
        FROM error_logs
    )";
    
    QStringList whereConditions;
    
    if (level != ErrorLog::LogLevel::Debug) {
        whereConditions << "level = :level";
    }
    if (!category.isEmpty()) {
        whereConditions << "category = :category";
    }
    if (startTime.isValid()) {
        whereConditions << "created_at >= :start_time";
    }
    if (endTime.isValid()) {
        whereConditions << "created_at <= :end_time";
    }
    
    if (!whereConditions.isEmpty()) {
        sql += " WHERE " + whereConditions.join(" AND ");
    }
    
    sql += " ORDER BY created_at DESC";
    
    // 添加限制和偏移
    if (limit > 0) {
        sql += QString(" LIMIT %1").arg(limit);
    }
    if (offset > 0) {
        sql += QString(" OFFSET %1").arg(offset);
    }
    
    return sql;
}

QString LogDao::buildSystemLogQuery(
    SystemLog::LogLevel level, const QString& category,
    const QString& component, const QString& operation,
    const QDateTime& startTime, const QDateTime& endTime,
    int limit, int offset)
{
    QString sql = R"(
        SELECT id, timestamp, level, category, message, component, operation,
               duration, memory_usage, cpu_usage, thread_id, session_id,
               metadata, created_at
        FROM system_logs
    )";
    
    QStringList whereConditions;
    
    if (level != SystemLog::LogLevel::Debug) {
        whereConditions << "level = :level";
    }
    if (!category.isEmpty()) {
        whereConditions << "category = :category";
    }
    if (!component.isEmpty()) {
        whereConditions << "component = :component";
    }
    if (!operation.isEmpty()) {
        whereConditions << "operation = :operation";
    }
    if (startTime.isValid()) {
        whereConditions << "created_at >= :start_time";
    }
    if (endTime.isValid()) {
        whereConditions << "created_at <= :end_time";
    }
    
    if (!whereConditions.isEmpty()) {
        sql += " WHERE " + whereConditions.join(" AND ");
    }
    
    sql += " ORDER BY created_at DESC";
    
    // 添加限制和偏移
    if (limit > 0) {
        sql += QString(" LIMIT %1").arg(limit);
    }
    if (offset > 0) {
        sql += QString(" OFFSET %1").arg(offset);
    }
    
    return sql;
}

// #include "logdao.moc"