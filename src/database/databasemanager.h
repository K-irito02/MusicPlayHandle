#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QMutex>
#include <QString>
#include <QStringList>
#include <memory>

/**
 * @brief 数据库管理器 - 简化版本
 * 
 * 提供基础的数据库连接、初始化和查询功能
 * 移除了复杂的事务管理和信号槽机制，专注于稳定性
 */
class DatabaseManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 获取单例实例
     */
    static DatabaseManager* instance();
    
    /**
     * @brief 初始化数据库
     * @param dbPath 数据库文件路径
     * @return 初始化是否成功
     */
    bool initialize(const QString& dbPath);
    
    /**
     * @brief 检查数据库是否已初始化且有效
     */
    bool isInitialized() const { return m_initialized; }
    
    /**
     * @brief 检查数据库连接是否有效
     */
    bool isValid() const;
    
    /**
     * @brief 获取数据库连接
     */
    QSqlDatabase database() const;
    
    /**
     * @brief 执行查询
     * @param queryStr SQL查询语句
     * @return 查询结果
     */
    QSqlQuery executeQuery(const QString& queryStr);
    
    /**
     * @brief 执行更新操作（INSERT/UPDATE/DELETE）
     * @param queryStr SQL语句
     * @return 操作是否成功
     */
    bool executeUpdate(const QString& queryStr);
    
    /**
     * @brief 获取最后的错误信息
     */
    QString lastError() const { return m_lastError; }
    
    /**
     * @brief 关闭数据库连接
     */
    void closeDatabase();

private:
    explicit DatabaseManager(QObject* parent = nullptr);
    ~DatabaseManager();
    
    // 禁用拷贝构造和赋值
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;
    
    /**
     * @brief 创建数据库表结构
     */
    bool createTables();
    
    /**
     * @brief 创建歌曲表
     */
    bool createSongsTable();
    
    /**
     * @brief 创建标签表
     */
    bool createTagsTable();
    
    /**
     * @brief 创建日志表
     */
    bool createLogsTable();
    
    /**
     * @brief 插入初始数据
     */
    bool insertInitialData();
    
    /**
     * @brief 记录错误信息
     */
    void logError(const QString& error);

private:
    static DatabaseManager* m_instance;
    static QMutex m_mutex;
    
    QSqlDatabase m_database;
    bool m_initialized;
    QString m_lastError;
    
    // 数据库连接名称
    static const QString CONNECTION_NAME;
};

#endif // DATABASEMANAGER_H