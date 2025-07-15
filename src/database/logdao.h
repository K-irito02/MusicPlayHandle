#ifndef LOGDAO_H
#define LOGDAO_H

#include "basedao.h"
#include "../models/errorlog.h"
#include "../models/systemlog.h"
#include <QDateTime>
#include <QList>
#include <QStringList>

/**
 * @brief 日志条目结构
 */
struct LogEntry
{
    int id = 0;
    QString level;
    QString message;
    QString category;
    QDateTime timestamp;
    
    LogEntry() = default;
    LogEntry(const QString& lvl, const QString& msg, const QString& cat = QString())
        : level(lvl), message(msg), category(cat), timestamp(QDateTime::currentDateTime()) {}
};

/**
 * @brief 日志数据访问对象
 * 
 * 提供日志相关的数据库操作
 */
class LogDao : public BaseDao
{
    Q_OBJECT

public:
    explicit LogDao(QObject* parent = nullptr);
    
    /**
     * @brief 添加日志条目
     * @param entry 日志条目
     * @return 添加成功返回日志ID，失败返回-1
     */
    int addLog(const LogEntry& entry);
    
    /**
     * @brief 添加日志条目（便捷方法）
     * @param level 日志级别
     * @param message 日志消息
     * @param category 日志分类
     * @return 添加成功返回日志ID，失败返回-1
     */
    int addLog(const QString& level, const QString& message, const QString& category = QString());
    
    /**
     * @brief 根据ID获取日志
     * @param id 日志ID
     * @return 日志条目
     */
    LogEntry getLogById(int id);
    
    /**
     * @brief 获取所有日志
     * @param limit 限制数量，-1表示不限制
     * @return 日志列表
     */
    QList<LogEntry> getAllLogs(int limit = -1);
    
    /**
     * @brief 根据级别获取日志
     * @param level 日志级别
     * @param limit 限制数量，-1表示不限制
     * @return 日志列表
     */
    QList<LogEntry> getLogsByLevel(const QString& level, int limit = -1);
    
    /**
     * @brief 根据分类获取日志
     * @param category 日志分类
     * @param limit 限制数量，-1表示不限制
     * @return 日志列表
     */
    QList<LogEntry> getLogsByCategory(const QString& category, int limit = -1);
    
    /**
     * @brief 根据时间范围获取日志
     * @param startTime 开始时间
     * @param endTime 结束时间
     * @param limit 限制数量，-1表示不限制
     * @return 日志列表
     */
    QList<LogEntry> getLogsByTimeRange(const QDateTime& startTime, const QDateTime& endTime, int limit = -1);
    
    /**
     * @brief 搜索日志
     * @param keyword 关键词
     * @param limit 限制数量，-1表示不限制
     * @return 日志列表
     */
    QList<LogEntry> searchLogs(const QString& keyword, int limit = -1);
    
    /**
     * @brief 删除指定时间之前的日志
     * @param beforeTime 时间点
     * @return 删除的日志数量
     */
    int deleteLogsBefore(const QDateTime& beforeTime);
    
    /**
     * @brief 删除指定级别的日志
     * @param level 日志级别
     * @return 删除的日志数量
     */
    int deleteLogsByLevel(const QString& level);
    
    /**
     * @brief 清空所有日志
     * @return 删除的日志数量
     */
    int clearAllLogs();
    
    /**
     * @brief 获取日志总数
     * @return 日志总数
     */
    int getLogCount();
    
    /**
     * @brief 获取指定级别的日志数量
     * @param level 日志级别
     * @return 日志数量
     */
    int getLogCountByLevel(const QString& level);
    
    /**
     * @brief 获取所有日志级别
     * @return 日志级别列表
     */
    QStringList getAllLogLevels();
    
    /**
     * @brief 获取所有日志分类
     * @return 分类列表
     */
    QStringList getAllLogCategories();
    
    /**
     * @brief 插入系统日志
     * @param systemLog 系统日志对象
     * @return 添加成功返回日志ID，失败返回-1
     */
    int insertSystemLog(const SystemLog& systemLog);
    
    /**
     * @brief 插入错误日志
     * @param errorLog 错误日志对象
     * @return 添加成功返回日志ID，失败返回-1
     */
    int insertErrorLog(const ErrorLog& errorLog);

    /**
     * @brief 数据库错误信号
     * @param error 错误信息
     */
    Q_SIGNAL void databaseError(const QString& error);

private:
    /**
     * @brief 从查询结果创建日志条目
     * @param query 查询结果
     * @return 日志条目
     */
    LogEntry createLogEntryFromQuery(const QSqlQuery& query);
};

#endif // LOGDAO_H