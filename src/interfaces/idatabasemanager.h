#ifndef IDATABASEMANAGER_H
#define IDATABASEMANAGER_H

#include <QString>
#include <QSqlDatabase>
#include <QSqlQuery>

/**
 * @brief 数据库管理器抽象接口
 * @details 定义数据库管理的核心功能接口，便于测试和扩展
 */
class IDatabaseManager
{
public:
    virtual ~IDatabaseManager() = default;
    
    // 数据库连接管理
    /**
     * @brief 初始化数据库
     * @param dbPath 数据库文件路径
     * @return 是否初始化成功
     */
    virtual bool initialize(const QString& dbPath) = 0;
    
    /**
     * @brief 关闭数据库连接
     */
    virtual void close() = 0;
    
    /**
     * @brief 检查数据库是否已连接
     * @return 是否已连接
     */
    virtual bool isConnected() const = 0;
    
    /**
     * @brief 获取数据库连接
     * @return 数据库连接对象
     */
    virtual QSqlDatabase getDatabase() const = 0;
    
    // 表管理
    /**
     * @brief 创建所有必要的表
     * @return 是否创建成功
     */
    virtual bool createTables() = 0;
    
    /**
     * @brief 检查表是否存在
     * @param tableName 表名
     * @return 是否存在
     */
    virtual bool tableExists(const QString& tableName) const = 0;
    
    // 事务管理
    /**
     * @brief 开始事务
     * @return 是否成功
     */
    virtual bool beginTransaction() = 0;
    
    /**
     * @brief 提交事务
     * @return 是否成功
     */
    virtual bool commitTransaction() = 0;
    
    /**
     * @brief 回滚事务
     * @return 是否成功
     */
    virtual bool rollbackTransaction() = 0;
    
    // 查询执行
    /**
     * @brief 执行SQL查询
     * @param sql SQL语句
     * @return 查询对象
     */
    virtual QSqlQuery executeQuery(const QString& sql) = 0;
    
    /**
     * @brief 执行预处理查询
     * @param sql SQL语句
     * @return 查询对象
     */
    virtual QSqlQuery prepareQuery(const QString& sql) = 0;
    
    // 数据库维护
    /**
     * @brief 优化数据库
     * @return 是否成功
     */
    virtual bool optimizeDatabase() = 0;
    
    /**
     * @brief 备份数据库
     * @param backupPath 备份文件路径
     * @return 是否成功
     */
    virtual bool backupDatabase(const QString& backupPath) = 0;
    
    /**
     * @brief 获取数据库版本
     * @return 版本号
     */
    virtual QString getDatabaseVersion() const = 0;
    
    /**
     * @brief 获取最后的错误信息
     * @return 错误信息
     */
    virtual QString getLastError() const = 0;
};

#endif // IDATABASEMANAGER_H