#include "logdao.h"
#include "databasemanager.h"
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariant>
#include <QDebug>

LogDao::LogDao(QObject* parent)
    : BaseDao(parent)
{
}

int LogDao::addLog(const LogEntry& entry)
{
    const QString sql = R"(
        INSERT INTO logs (level, message, category, timestamp)
        VALUES (?, ?, ?, ?)
    )";
    
    QSqlQuery query = prepareQuery(sql);
    query.addBindValue(entry.level);
    query.addBindValue(entry.message);
    query.addBindValue(entry.category);
    query.addBindValue(entry.timestamp);
    
    if (query.exec()) {
        return query.lastInsertId().toInt();
    } else {
        logError("addLog", query.lastError().text());
        return -1;
    }
}

int LogDao::addLog(const QString& level, const QString& message, const QString& category)
{
    LogEntry entry(level, message, category);
    return addLog(entry);
}

LogEntry LogDao::getLogById(int id)
{
    const QString sql = "SELECT * FROM logs WHERE id = ?";
    QSqlQuery query = prepareQuery(sql);
    query.addBindValue(id);
    
    if (query.exec() && query.next()) {
        return createLogEntryFromQuery(query);
    }
    
    return LogEntry(); // 返回空对象
}

QList<LogEntry> LogDao::getAllLogs(int limit)
{
    QList<LogEntry> logs;
    QString sql = "SELECT * FROM logs ORDER BY timestamp DESC";
    
    if (limit > 0) {
        sql += QString(" LIMIT %1").arg(limit);
    }
    
    QSqlQuery query = executeQuery(sql);
    
    while (query.next()) {
        logs.append(createLogEntryFromQuery(query));
    }
    
    return logs;
}

QList<LogEntry> LogDao::getLogsByLevel(const QString& level, int limit)
{
    QList<LogEntry> logs;
    QString sql = "SELECT * FROM logs WHERE level = ? ORDER BY timestamp DESC";
    
    if (limit > 0) {
        sql += QString(" LIMIT %1").arg(limit);
    }
    
    QSqlQuery query = prepareQuery(sql);
    query.addBindValue(level);
    
    if (query.exec()) {
        while (query.next()) {
            logs.append(createLogEntryFromQuery(query));
        }
    } else {
        logError("getLogsByLevel", query.lastError().text());
    }
    
    return logs;
}

QList<LogEntry> LogDao::getLogsByCategory(const QString& category, int limit)
{
    QList<LogEntry> logs;
    QString sql = "SELECT * FROM logs WHERE category = ? ORDER BY timestamp DESC";
    
    if (limit > 0) {
        sql += QString(" LIMIT %1").arg(limit);
    }
    
    QSqlQuery query = prepareQuery(sql);
    query.addBindValue(category);
    
    if (query.exec()) {
        while (query.next()) {
            logs.append(createLogEntryFromQuery(query));
        }
    } else {
        logError("getLogsByCategory", query.lastError().text());
    }
    
    return logs;
}

QList<LogEntry> LogDao::getLogsByTimeRange(const QDateTime& startTime, const QDateTime& endTime, int limit)
{
    QList<LogEntry> logs;
    QString sql = "SELECT * FROM logs WHERE timestamp BETWEEN ? AND ? ORDER BY timestamp DESC";
    
    if (limit > 0) {
        sql += QString(" LIMIT %1").arg(limit);
    }
    
    QSqlQuery query = prepareQuery(sql);
    query.addBindValue(startTime);
    query.addBindValue(endTime);
    
    if (query.exec()) {
        while (query.next()) {
            logs.append(createLogEntryFromQuery(query));
        }
    } else {
        logError("getLogsByTimeRange", query.lastError().text());
    }
    
    return logs;
}

QList<LogEntry> LogDao::searchLogs(const QString& keyword, int limit)
{
    QList<LogEntry> logs;
    QString sql = R"(
        SELECT * FROM logs 
        WHERE message LIKE ? OR category LIKE ? 
        ORDER BY timestamp DESC
    )";
    
    if (limit > 0) {
        sql += QString(" LIMIT %1").arg(limit);
    }
    
    QSqlQuery query = prepareQuery(sql);
    QString searchPattern = "%" + keyword + "%";
    query.addBindValue(searchPattern);
    query.addBindValue(searchPattern);
    
    if (query.exec()) {
        while (query.next()) {
            logs.append(createLogEntryFromQuery(query));
        }
    } else {
        logError("searchLogs", query.lastError().text());
    }
    
    return logs;
}

int LogDao::deleteLogsBefore(const QDateTime& beforeTime)
{
    const QString sql = "DELETE FROM logs WHERE timestamp < ?";
    QSqlQuery query = prepareQuery(sql);
    query.addBindValue(beforeTime);
    
    if (query.exec()) {
        return query.numRowsAffected();
    } else {
        logError("deleteLogsBefore", query.lastError().text());
        return -1;
    }
}

int LogDao::deleteLogsByLevel(const QString& level)
{
    const QString sql = "DELETE FROM logs WHERE level = ?";
    QSqlQuery query = prepareQuery(sql);
    query.addBindValue(level);
    
    if (query.exec()) {
        return query.numRowsAffected();
    } else {
        logError("deleteLogsByLevel", query.lastError().text());
        return -1;
    }
}

int LogDao::clearAllLogs()
{
    const QString sql = "DELETE FROM logs";
    QSqlQuery query = executeQuery(sql);
    
    if (query.lastError().isValid()) {
        logError("clearAllLogs", query.lastError().text());
        return -1;
    }
    
    return query.numRowsAffected();
}

int LogDao::getLogCount()
{
    const QString sql = "SELECT COUNT(*) FROM logs";
    QSqlQuery query = executeQuery(sql);
    
    if (query.next()) {
        return query.value(0).toInt();
    }
    
    return 0;
}

int LogDao::getLogCountByLevel(const QString& level)
{
    const QString sql = "SELECT COUNT(*) FROM logs WHERE level = ?";
    QSqlQuery query = prepareQuery(sql);
    query.addBindValue(level);
    
    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    
    return 0;
}

QStringList LogDao::getAllLogLevels()
{
    QStringList levels;
    const QString sql = "SELECT DISTINCT level FROM logs ORDER BY level";
    QSqlQuery query = executeQuery(sql);
    
    while (query.next()) {
        levels.append(query.value(0).toString());
    }
    
    return levels;
}

QStringList LogDao::getAllLogCategories()
{
    QStringList categories;
    const QString sql = "SELECT DISTINCT category FROM logs WHERE category IS NOT NULL AND category != '' ORDER BY category";
    QSqlQuery query = executeQuery(sql);
    
    while (query.next()) {
        categories.append(query.value(0).toString());
    }
    
    return categories;
}

LogEntry LogDao::createLogEntryFromQuery(const QSqlQuery& query)
{
    LogEntry entry;
    entry.id = query.value("id").toInt();
    entry.level = query.value("level").toString();
    entry.message = query.value("message").toString();
    entry.category = query.value("category").toString();
    entry.timestamp = query.value("timestamp").toDateTime();
    
    return entry;
}

int LogDao::insertSystemLog(const SystemLog& systemLog)
{
    return addLog(LogEntry(
        systemLog.levelString(),
        systemLog.message(),
        systemLog.category()
    ));
}

int LogDao::insertErrorLog(const ErrorLog& errorLog)
{
    return addLog(LogEntry(
        errorLog.levelString(),
        errorLog.message(),
        errorLog.category()
    ));
}