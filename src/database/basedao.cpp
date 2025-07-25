#include "basedao.h"
#include "databasemanager.h"
#include <QSqlDatabase>

BaseDao::BaseDao(QObject* parent)
    : QObject(parent)
    , m_dbManager(DatabaseManager::instance())
{
}

BaseDao::~BaseDao()
{
}

QSqlQuery BaseDao::executeQuery(const QString& sql)
{
    if (!m_dbManager || !m_dbManager->isInitialized()) {
        logError("executeQuery", "数据库未初始化");
        return QSqlQuery();
    }
    
    return m_dbManager->executeQuery(sql);
}

bool BaseDao::executeUpdate(const QString& sql)
{
    if (!m_dbManager || !m_dbManager->isInitialized()) {
        logError("executeUpdate", "数据库未初始化");
        return false;
    }
    
    return m_dbManager->executeUpdate(sql);
}

QSqlQuery BaseDao::prepareQuery(const QString& sql)
{
    if (!m_dbManager || !m_dbManager->isInitialized()) {
        logError("prepareQuery", "数据库未初始化");
        return QSqlQuery();
    }
    
    QSqlQuery query(m_dbManager->database());
    if (!query.prepare(sql)) {
        logError("prepareQuery", "准备查询失败: " + query.lastError().text());
    }
    return query;
}

void BaseDao::logError(const QString& operation, const QString& error)
{
    QString fullError = QString("%1 操作失败: %2").arg(operation, error);
    qCritical() << "BaseDao Error:" << fullError;
}

void BaseDao::logError(const QString& operation, const QString& error) const
{
    QString fullError = QString("%1 操作失败: %2").arg(operation, error);
    qCritical() << "BaseDao Error:" << fullError;
}

void BaseDao::logInfo(const QString& operation, const QString& message)
{
    QString fullMessage = QString("%1 操作: %2").arg(operation, message);
    qDebug() << "BaseDao Info:" << fullMessage;
}

void BaseDao::logInfo(const QString& operation, const QString& message) const
{
    QString fullMessage = QString("%1 操作: %2").arg(operation, message);
    qDebug() << "BaseDao Info:" << fullMessage;
}

DatabaseManager* BaseDao::dbManager()
{
    return m_dbManager;
}

const DatabaseManager* BaseDao::dbManager() const
{
    return m_dbManager;
}

void BaseDao::closeDatabase()
{
    if (m_dbManager) {
        m_dbManager->close();
    }
}

bool BaseDao::createTables()
{
    if (!m_dbManager || !m_dbManager->isInitialized()) {
        logError("createTables", "数据库未初始化");
        return false;
    }
    
    return m_dbManager->createTables();
}