#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QMutex>
#include <QList>
#include <QDateTime>
#include "../models/errorlog.h"
#include "../models/systemlog.h"

/**
 * @brief 日志数据访问对象
 * 
 * 提供错误日志和系统日志的数据库操作接口
 */
class LogDao : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit LogDao(QObject* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~LogDao() override;

    // 错误日志操作
    
    /**
     * @brief 插入错误日志
     * @param errorLog 错误日志对象
     * @return 成功返回生成的ID，失败返回-1
     */
    int insertErrorLog(const ErrorLog& errorLog);
    
    /**
     * @brief 获取错误日志
     * @param id 日志ID
     * @return 错误日志对象，若不存在则返回空对象
     */
    ErrorLog getErrorLog(int id);
    
    /**
     * @brief 查询错误日志
     * @param level 日志级别过滤（可选）
     * @param category 分类过滤（可选）
     * @param startTime 开始时间（可选）
     * @param endTime 结束时间（可选）
     * @param limit 数量限制，默认100
     * @param offset 偏移量，默认0
     * @return 错误日志列表
     */
    QList<ErrorLog> queryErrorLogs(
        ErrorLog::LogLevel level = ErrorLog::LogLevel::Debug,
        const QString& category = QString(),
        const QDateTime& startTime = QDateTime(),
        const QDateTime& endTime = QDateTime(),
        int limit = 100,
        int offset = 0
    );
    
    /**
     * @brief 删除错误日志
     * @param id 日志ID
     * @return 成功返回true，失败返回false
     */
    bool deleteErrorLog(int id);
    
    /**
     * @brief 清理过期的错误日志
     * @param days 保留天数
     * @return 删除的记录数
     */
    int cleanupErrorLogs(int days = 30);
    
    // 系统日志操作
    
    /**
     * @brief 插入系统日志
     * @param systemLog 系统日志对象
     * @return 成功返回生成的ID，失败返回-1
     */
    int insertSystemLog(const SystemLog& systemLog);
    
    /**
     * @brief 获取系统日志
     * @param id 日志ID
     * @return 系统日志对象，若不存在则返回空对象
     */
    SystemLog getSystemLog(int id);
    
    /**
     * @brief 查询系统日志
     * @param level 日志级别过滤（可选）
     * @param category 分类过滤（可选）
     * @param component 组件过滤（可选）
     * @param operation 操作过滤（可选）
     * @param startTime 开始时间（可选）
     * @param endTime 结束时间（可选）
     * @param limit 数量限制，默认100
     * @param offset 偏移量，默认0
     * @return 系统日志列表
     */
    QList<SystemLog> querySystemLogs(
        SystemLog::LogLevel level = SystemLog::LogLevel::Debug,
        const QString& category = QString(),
        const QString& component = QString(),
        const QString& operation = QString(),
        const QDateTime& startTime = QDateTime(),
        const QDateTime& endTime = QDateTime(),
        int limit = 100,
        int offset = 0
    );
    
    /**
     * @brief 删除系统日志
     * @param id 日志ID
     * @return 成功返回true，失败返回false
     */
    bool deleteSystemLog(int id);
    
    /**
     * @brief 清理过期的系统日志
     * @param days 保留天数
     * @return 删除的记录数
     */
    int cleanupSystemLogs(int days = 30);
    
    // 统计信息
    
    /**
     * @brief 获取错误日志统计信息
     * @param startTime 开始时间
     * @param endTime 结束时间
     * @return 统计信息（级别->数量）
     */
    QMap<ErrorLog::LogLevel, int> getErrorLogStatistics(
        const QDateTime& startTime = QDateTime(),
        const QDateTime& endTime = QDateTime()
    );
    
    /**
     * @brief 获取系统日志统计信息
     * @param startTime 开始时间
     * @param endTime 结束时间
     * @return 统计信息（级别->数量）
     */
    QMap<SystemLog::LogLevel, int> getSystemLogStatistics(
        const QDateTime& startTime = QDateTime(),
        const QDateTime& endTime = QDateTime()
    );
    
    /**
     * @brief 获取日志数量
     * @param tableName 表名（"error_logs" 或 "system_logs"）
     * @return 日志数量
     */
    int getLogCount(const QString& tableName);
    
    // 批量操作
    
    /**
     * @brief 批量插入错误日志
     * @param errorLogs 错误日志列表
     * @return 成功插入的数量
     */
    int batchInsertErrorLogs(const QList<ErrorLog>& errorLogs);
    
    /**
     * @brief 批量插入系统日志
     * @param systemLogs 系统日志列表
     * @return 成功插入的数量
     */
    int batchInsertSystemLogs(const QList<SystemLog>& systemLogs);
    
    /**
     * @brief 清空所有错误日志
     * @return 成功返回true，失败返回false
     */
    bool clearErrorLogs();
    
    /**
     * @brief 清空所有系统日志
     * @return 成功返回true，失败返回false
     */
    bool clearSystemLogs();

signals:
    /**
     * @brief 错误日志插入完成信号
     * @param id 生成的ID
     * @param errorLog 错误日志对象
     */
    void errorLogInserted(int id, const ErrorLog& errorLog);
    
    /**
     * @brief 系统日志插入完成信号
     * @param id 生成的ID
     * @param systemLog 系统日志对象
     */
    void systemLogInserted(int id, const SystemLog& systemLog);
    
    /**
     * @brief 数据库错误信号
     * @param error 错误信息
     */
    void databaseError(const QString& error);

private:
    /**
     * @brief 获取数据库连接
     * @return 数据库连接
     */
    QSqlDatabase getDatabase();
    
    /**
     * @brief 记录错误信息
     * @param error 错误信息
     */
    void logError(const QString& error);
    
    /**
     * @brief 绑定错误日志参数
     * @param query SQL查询对象
     * @param errorLog 错误日志对象
     */
    void bindErrorLogParams(QSqlQuery& query, const ErrorLog& errorLog);
    
    /**
     * @brief 绑定系统日志参数
     * @param query SQL查询对象
     * @param systemLog 系统日志对象
     */
    void bindSystemLogParams(QSqlQuery& query, const SystemLog& systemLog);
    
    /**
     * @brief 从查询结果创建错误日志对象
     * @param query SQL查询对象
     * @return 错误日志对象
     */
    ErrorLog createErrorLogFromQuery(const QSqlQuery& query);
    
    /**
     * @brief 从查询结果创建系统日志对象
     * @param query SQL查询对象
     * @return 系统日志对象
     */
    SystemLog createSystemLogFromQuery(const QSqlQuery& query);
    
    /**
     * @brief 构建错误日志查询SQL
     * @param level 日志级别
     * @param category 分类
     * @param startTime 开始时间
     * @param endTime 结束时间
     * @param limit 数量限制
     * @param offset 偏移量
     * @return SQL查询字符串
     */
    QString buildErrorLogQuery(
        ErrorLog::LogLevel level,
        const QString& category,
        const QDateTime& startTime,
        const QDateTime& endTime,
        int limit,
        int offset
    );
    
    /**
     * @brief 构建系统日志查询SQL
     * @param level 日志级别
     * @param category 分类
     * @param component 组件
     * @param operation 操作
     * @param startTime 开始时间
     * @param endTime 结束时间
     * @param limit 数量限制
     * @param offset 偏移量
     * @return SQL查询字符串
     */
    QString buildSystemLogQuery(
        SystemLog::LogLevel level,
        const QString& category,
        const QString& component,
        const QString& operation,
        const QDateTime& startTime,
        const QDateTime& endTime,
        int limit,
        int offset
    );

private:
    QMutex m_mutex;
    
    // SQL语句常量
    static const QString INSERT_ERROR_LOG_SQL;
    static const QString INSERT_SYSTEM_LOG_SQL;
    static const QString SELECT_ERROR_LOG_BY_ID_SQL;
    static const QString SELECT_SYSTEM_LOG_BY_ID_SQL;
    static const QString DELETE_ERROR_LOG_SQL;
    static const QString DELETE_SYSTEM_LOG_SQL;
    static const QString CLEANUP_ERROR_LOGS_SQL;
    static const QString CLEANUP_SYSTEM_LOGS_SQL;
    static const QString COUNT_ERROR_LOGS_SQL;
    static const QString COUNT_SYSTEM_LOGS_SQL;
    static const QString CLEAR_ERROR_LOGS_SQL;
    static const QString CLEAR_SYSTEM_LOGS_SQL;
}; 
