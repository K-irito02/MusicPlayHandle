#ifndef BASEDAO_H
#define BASEDAO_H

#include <QObject>
#include <QSqlQuery>
#include <QSqlError>
#include <QString>
#include <QVariant>
#include <QDebug>

class DatabaseManager;

/**
 * @brief DAO基类
 * 
 * 提供通用的数据库操作方法
 */
class BaseDao : public QObject
{
    Q_OBJECT

public:
    explicit BaseDao(QObject* parent = nullptr);
    virtual ~BaseDao();

protected:
    /**
     * @brief 执行查询
     * @param sql SQL语句
     * @return 查询结果
     */
    virtual QSqlQuery executeQuery(const QString& sql);
    
    /**
     * @brief 执行更新操作
     * @param sql SQL语句
     * @return 操作是否成功
     */
    virtual bool executeUpdate(const QString& sql);
    
    /**
     * @brief 准备查询语句
     * @param sql SQL语句
     * @return 准备好的查询对象
     */
    virtual QSqlQuery prepareQuery(const QString& sql);
    
    /**
     * @brief 记录错误
     * @param operation 操作描述
     * @param error 错误信息
     */
    virtual void logError(const QString& operation, const QString& error);
    
    /**
     * @brief 记录错误 (const版本)
     * @param operation 操作描述
     * @param error 错误信息
     */
    virtual void logError(const QString& operation, const QString& error) const;
    
    /**
     * @brief 获取数据库管理器实例
     */
    virtual DatabaseManager* dbManager();
    
    /**
     * @brief 获取数据库管理器实例 (const版本)
     */
    virtual const DatabaseManager* dbManager() const;

private:
    DatabaseManager* m_dbManager;
};

#endif // BASEDAO_H