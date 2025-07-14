#ifndef DATABASETRANSACTION_H
#define DATABASETRANSACTION_H

#include <QSqlDatabase>
#include <QSqlError>
#include <QString>
#include <QDebug>

/**
 * @brief 数据库事务RAII管理器
 * @details 使用RAII模式自动管理数据库事务，确保异常安全
 */
class DatabaseTransaction
{
public:
    /**
     * @brief 构造函数，自动开始事务
     * @param db 数据库连接
     * @param autoCommit 是否在析构时自动提交（默认false，需要手动提交）
     */
    explicit DatabaseTransaction(QSqlDatabase& db, bool autoCommit = false)
        : m_db(db), m_autoCommit(autoCommit), m_committed(false), m_rolledBack(false)
    {
        m_success = m_db.transaction();
        if (!m_success) {
            qWarning() << "Failed to begin transaction:" << m_db.lastError().text();
        }
    }
    
    /**
     * @brief 析构函数，自动回滚未提交的事务
     */
    ~DatabaseTransaction()
    {
        if (m_success && !m_committed && !m_rolledBack) {
            if (m_autoCommit) {
                commit();
            } else {
                rollback();
            }
        }
    }
    
    /**
     * @brief 提交事务
     * @return 是否提交成功
     */
    bool commit()
    {
        if (!m_success || m_committed || m_rolledBack) {
            return false;
        }
        
        bool result = m_db.commit();
        if (result) {
            m_committed = true;
        } else {
            qWarning() << "Failed to commit transaction:" << m_db.lastError().text();
        }
        return result;
    }
    
    /**
     * @brief 回滚事务
     * @return 是否回滚成功
     */
    bool rollback()
    {
        if (!m_success || m_committed || m_rolledBack) {
            return false;
        }
        
        bool result = m_db.rollback();
        if (result) {
            m_rolledBack = true;
        } else {
            qWarning() << "Failed to rollback transaction:" << m_db.lastError().text();
        }
        return result;
    }
    
    /**
     * @brief 检查事务是否成功开始
     * @return 是否成功
     */
    bool isValid() const
    {
        return m_success;
    }
    
    /**
     * @brief 检查事务是否已提交
     * @return 是否已提交
     */
    bool isCommitted() const
    {
        return m_committed;
    }
    
    /**
     * @brief 检查事务是否已回滚
     * @return 是否已回滚
     */
    bool isRolledBack() const
    {
        return m_rolledBack;
    }
    
    /**
     * @brief 获取最后的错误信息
     * @return 错误信息
     */
    QString lastError() const
    {
        return m_db.lastError().text();
    }
    
    // 禁用拷贝构造和赋值
    DatabaseTransaction(const DatabaseTransaction&) = delete;
    DatabaseTransaction& operator=(const DatabaseTransaction&) = delete;
    
    // 允许移动构造和赋值
    DatabaseTransaction(DatabaseTransaction&& other) noexcept
        : m_db(other.m_db)
        , m_autoCommit(other.m_autoCommit)
        , m_success(other.m_success)
        , m_committed(other.m_committed)
        , m_rolledBack(other.m_rolledBack)
    {
        other.m_committed = true; // 防止析构时回滚
    }
    
    DatabaseTransaction& operator=(DatabaseTransaction&& other) noexcept
    {
        if (this != &other) {
            // 先处理当前事务
            if (m_success && !m_committed && !m_rolledBack) {
                rollback();
            }
            
            // 移动数据
            m_autoCommit = other.m_autoCommit;
            m_success = other.m_success;
            m_committed = other.m_committed;
            m_rolledBack = other.m_rolledBack;
            
            other.m_committed = true; // 防止析构时回滚
        }
        return *this;
    }
    
private:
    QSqlDatabase& m_db;
    bool m_autoCommit;
    bool m_success;
    bool m_committed;
    bool m_rolledBack;
};

/**
 * @brief 便利宏，用于创建自动提交的事务
 */
#define AUTO_COMMIT_TRANSACTION(db) \
    DatabaseTransaction transaction(db, true); \
    if (!transaction.isValid()) { \
        qWarning() << "Failed to create transaction"; \
        return false; \
    }

/**
 * @brief 便利宏，用于创建手动提交的事务
 */
#define MANUAL_TRANSACTION(db, name) \
    DatabaseTransaction name(db, false); \
    if (!name.isValid()) { \
        qWarning() << "Failed to create transaction"; \
        return false; \
    }

#endif // DATABASETRANSACTION_H