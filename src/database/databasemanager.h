#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QMutex>
#include <QString>
#include <QStringList>

/**
 * @brief 数据库管理器单例类
 * 
 * 负责管理SQLite数据库连接、表结构创建、事务处理等核心功能
 * 使用线程安全的单例模式，支持连接池管理
 */
class DatabaseManager : public QObject
{
    Q_OBJECT
    
public:
    /**
     * @brief 获取单例实例
     * @return DatabaseManager实例指针
     */
    static DatabaseManager* instance();
    
    /**
     * @brief 初始化数据库
     * @param dbPath 数据库文件路径
     * @return 初始化是否成功
     */
    bool initialize(const QString& dbPath);
    
    /**
     * @brief 获取数据库连接
     * @return 数据库连接对象
     */
    QSqlDatabase database();
    
    /**
     * @brief 获取指定名称的数据库连接
     * @param connectionName 连接名称
     * @return 数据库连接对象
     */
    QSqlDatabase database(const QString& connectionName);
    
    /**
     * @brief 开始事务
     * @return 是否成功开始事务
     */
    bool beginTransaction();
    
    /**
     * @brief 提交事务
     * @return 是否成功提交事务
     */
    bool commitTransaction();
    
    /**
     * @brief 回滚事务
     * @return 是否成功回滚事务
     */
    bool rollbackTransaction();
    
    /**
     * @brief 执行SQL查询
     * @param query SQL查询语句
     * @param params 查询参数
     * @return 查询结果
     */
    QSqlQuery executeQuery(const QString& query, const QVariantList& params = QVariantList());
    
    /**
     * @brief 执行SQL语句
     * @param sql SQL语句
     * @param params 参数
     * @return 是否执行成功
     */
    bool executeSQL(const QString& sql, const QVariantList& params = QVariantList());
    
    /**
     * @brief 数据库维护 - 清理
     * @return 是否成功
     */
    bool vacuum();
    
    /**
     * @brief 数据库完整性检查
     * @return 是否通过检查
     */
    bool checkIntegrity();
    
    /**
     * @brief 备份数据库
     * @param backupPath 备份文件路径
     * @return 是否备份成功
     */
    bool backupDatabase(const QString& backupPath);
    
    /**
     * @brief 恢复数据库
     * @param backupPath 备份文件路径
     * @return 是否恢复成功
     */
    bool restoreDatabase(const QString& backupPath);
    
    /**
     * @brief 获取数据库版本
     * @return 数据库版本号
     */
    int getDatabaseVersion();
    
    /**
     * @brief 更新数据库版本
     * @param version 新版本号
     * @return 是否更新成功
     */
    bool updateDatabaseVersion(int version);
    
    /**
     * @brief 关闭数据库连接
     */
    void closeDatabase();
    
    /**
     * @brief 检查数据库连接是否有效
     * @return 是否有效
     */
    bool isValid() const;
    
    /**
     * @brief 获取最后一次错误信息
     * @return 错误信息
     */
    QString lastError() const;
    
signals:
    /**
     * @brief 数据库连接成功信号
     */
    void databaseConnected();
    
    /**
     * @brief 数据库连接失败信号
     * @param error 错误信息
     */
    void databaseConnectionFailed(const QString& error);
    
    /**
     * @brief 数据库错误信号
     * @param error 错误信息
     */
    void databaseError(const QString& error);
    
    /**
     * @brief 事务开始信号
     */
    void transactionStarted();
    
    /**
     * @brief 事务提交信号
     */
    void transactionCommitted();
    
    /**
     * @brief 事务回滚信号
     */
    void transactionRolledBack();
    
private:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit DatabaseManager(QObject* parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~DatabaseManager();
    
    // 禁用拷贝构造和赋值
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;
    
    /**
     * @brief 创建数据库表结构
     * @return 操作成功返回true，否则返回false
     */
    bool createTables();

    /**
     * @brief 创建错误日志表
     * @return 操作成功返回true，否则返回false
     */
    bool createErrorLogTable();

    /**
     * @brief 创建系统日志表
     * @return 操作成功返回true，否则返回false
     */
    bool createSystemLogTable();

    /**
     * @brief 创建歌曲表
     * @return 操作成功返回true，否则返回false
     */
    bool createSongsTable();

    /**
     * @brief 创建标签表
     * @return 操作成功返回true，否则返回false
     */
    bool createTagsTable();
    
    /**
     * @brief 创建索引
     * @return 是否创建成功
     */
    bool createIndexes();
    
    /**
     * @brief 创建触发器
     * @return 是否创建成功
     */
    bool createTriggers();
    
    /**
     * @brief 插入初始数据
     * @return 是否插入成功
     */
    bool insertInitialData();
    
    /**
     * @brief 数据库升级
     * @param oldVersion 旧版本号
     * @param newVersion 新版本号
     * @return 是否升级成功
     */
    bool upgradeDatabaseSchema(int oldVersion, int newVersion);
    
    /**
     * @brief 验证数据库连接
     * @return 是否连接有效
     */
    bool validateConnection();
    
    /**
     * @brief 记录错误信息
     * @param error 错误信息
     */
    void logError(const QString& error);
    
    static DatabaseManager* m_instance;      ///< 单例实例
    static QMutex m_mutex;                   ///< 线程安全互斥锁
    
    QSqlDatabase m_database;                 ///< 默认数据库连接
    QString m_databasePath;                  ///< 数据库文件路径
    QString m_lastError;                     ///< 最后一次错误信息
    bool m_initialized;                      ///< 是否已初始化
    bool m_inTransaction;                    ///< 是否在事务中
    
    mutable QMutex m_dbMutex;                ///< 数据库访问互斥锁
    
    static const int DATABASE_VERSION = 1;   ///< 当前数据库版本
    static const QString DEFAULT_CONNECTION_NAME; ///< 默认连接名称
    
    // SQL语句常量
    struct SQLStatements {
        // 表创建语句
        static const QString CREATE_SONGS_TABLE;
        static const QString CREATE_TAGS_TABLE;
        static const QString CREATE_SONG_TAG_RELATIONS_TABLE;
        static const QString CREATE_PLAYLISTS_TABLE;
        static const QString CREATE_PLAYLIST_SONGS_TABLE;
        static const QString CREATE_PLAY_HISTORY_TABLE;
        static const QString CREATE_SETTINGS_TABLE;
        static const QString CREATE_EQUALIZER_PRESETS_TABLE;
        
        // 索引创建语句
        static const QString CREATE_SONGS_INDEXES;
        static const QString CREATE_TAGS_INDEXES;
        static const QString CREATE_RELATIONS_INDEXES;
        static const QString CREATE_PLAYLISTS_INDEXES;
        static const QString CREATE_HISTORY_INDEXES;
        static const QString CREATE_SETTINGS_INDEXES;
        
        // 触发器创建语句
        static const QString CREATE_SYSTEM_TAG_PROTECTION_TRIGGER;
        static const QString CREATE_TAG_SONG_COUNT_TRIGGERS;
        static const QString CREATE_PLAY_COUNT_TRIGGER;
        static const QString CREATE_VALIDATION_TRIGGERS;
        
        // 初始数据插入语句
        static const QString INSERT_SYSTEM_TAGS;
        static const QString INSERT_DEFAULT_SETTINGS;
        static const QString INSERT_EQUALIZER_PRESETS;
    };
};

#endif // DATABASEMANAGER_H 