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
#include "interfaces/idatabasemanager.h"

/**
 * @brief 数据库管理器 - 简化版本
 * 
 * 提供基础的数据库连接、初始化和查询功能
 * 移除了复杂的事务管理和信号槽机制，专注于稳定性
 */
class DatabaseManager : public QObject, public IDatabaseManager
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
    // IDatabaseManager interface implementation
    bool initialize(const QString& dbPath) override;
    void close() override;
    bool isConnected() const override { return isValid(); }
    QSqlDatabase getDatabase() const override { return database(); }
    bool createTables() override;
    bool tableExists(const QString& tableName) const override;
    QSqlQuery executeQuery(const QString& sql) override;
    QSqlQuery prepareQuery(const QString& sql) override;
    bool optimizeDatabase() override;
    bool backupDatabase(const QString& backupPath) override;
    QString getDatabaseVersion() const override;
    QString getLastError() const override { return m_lastError; }
    
    // Additional public methods
    bool isInitialized() const { return m_initialized; }
    bool isValid() const;
    QSqlDatabase database() const;
    bool executeUpdate(const QString& queryStr);
    void closeDatabase();

private:
    explicit DatabaseManager(QObject* parent = nullptr);
    ~DatabaseManager();
    
    // 禁用拷贝构造和赋值
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;
    
    /**
     * @brief 创建歌曲表
     */
    bool createSongsTable();
    
    /**
     * @brief 创建标签表
     */
    bool createTagsTable();
    
    /**
     * @brief 创建歌曲-标签关联表
     */
    bool createSongTagsTable();
    
    /**
     * @brief 创建播放历史表
     */
    bool createPlayHistoryTable();
    
    /**
     * @brief 创建日志表
     */
    bool createLogsTable();
    
    /**
     * @brief 插入初始数据
     */
    bool insertInitialData();
    
    /**
     * @brief 清理多余标签，只保留系统标签
     */
    bool cleanupExtraTags();
    
    /**
     * @brief 检查并修复系统标签
     */
    bool checkAndFixSystemTags();
    
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