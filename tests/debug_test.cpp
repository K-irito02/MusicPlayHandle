#include <QApplication>
#include <QDebug>
#include "src/database/databasemanager.h"
#include "src/database/tagdao.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    qDebug() << "=== 数据库调试测试开始 ===";
    
    // 初始化数据库管理器
    DatabaseManager* dbManager = DatabaseManager::instance();
    if (!dbManager) {
        qDebug() << "ERROR: 无法获取DatabaseManager实例";
        return -1;
    }
    
    qDebug() << "DatabaseManager实例获取成功";
    
    // 初始化数据库
    if (!dbManager->initialize()) {
        qDebug() << "ERROR: 数据库初始化失败";
        return -1;
    }
    
    qDebug() << "数据库初始化成功";
    
    // 测试标签查询
    TagDao tagDao(dbManager);
    QList<Tag> tags = tagDao.getAllTags();
    
    qDebug() << QString("查询到 %1 个标签:").arg(tags.size());
    for (const Tag& tag : tags) {
        qDebug() << QString("- 标签: %1, 系统标签: %2")
                    .arg(tag.getName())
                    .arg(tag.getIsSystem() ? "是" : "否");
    }
    
    qDebug() << "=== 数据库调试测试完成 ===";
    
    return 0;
}