#include <QApplication>
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include "src/database/databasemanager.h"
#include "src/database/tagdao.h"
#include "src/ui/controllers/AddSongDialogController.h"
#include "src/ui/controllers/MainWindowController.h"

/**
 * 标签功能修复测试
 * 
 * 测试内容：
 * 1. 数据库连接和表创建
 * 2. 系统标签初始化
 * 3. 用户标签创建
 * 4. 标签列表显示
 * 5. 标签类型判断
 */

void testDatabaseConnection()
{
    qDebug() << "=== 测试数据库连接 ===";
    
    DatabaseManager* dbManager = DatabaseManager::instance();
    if (!dbManager->initialize(":memory:")) {
        qDebug() << "数据库初始化失败";
        return;
    }
    
    qDebug() << "数据库连接成功";
    qDebug() << "数据库有效性:" << dbManager->isValid();
}

void testSystemTagsInitialization()
{
    qDebug() << "\n=== 测试系统标签初始化 ===";
    
    DatabaseManager* dbManager = DatabaseManager::instance();
    
    // 检查系统标签是否存在
    QSqlQuery query = dbManager->executeQuery("SELECT name, is_system FROM tags WHERE is_system = 1");
    
    qDebug() << "系统标签列表:";
    while (query.next()) {
        QString name = query.value("name").toString();
        int isSystem = query.value("is_system").toInt();
        qDebug() << "- 标签:" << name << "系统标签:" << (isSystem == 1 ? "是" : "否");
    }
}

void testUserTagCreation()
{
    qDebug() << "\n=== 测试用户标签创建 ===";
    
    DatabaseManager* dbManager = DatabaseManager::instance();
    
    // 创建测试用户标签
    QStringList testTags = {"摇滚", "流行", "古典", "山海"};
    
    for (const QString& tagName : testTags) {
        QSqlQuery query(dbManager->database());
        query.prepare("INSERT INTO tags (name, color, description, is_system) VALUES (?, ?, ?, ?)");
        query.addBindValue(tagName);
        query.addBindValue("#9C27B0");
        query.addBindValue(QString("用户创建的标签: %1").arg(tagName));
        query.addBindValue(0);
        
        if (query.exec()) {
            qDebug() << "用户标签创建成功:" << tagName;
        } else {
            qDebug() << "用户标签创建失败:" << tagName << query.lastError().text();
        }
    }
}

void testTagListRetrieval()
{
    qDebug() << "\n=== 测试标签列表获取 ===";
    
    TagDao tagDao;
    auto allTags = tagDao.getAllTags();
    
    qDebug() << "总标签数量:" << allTags.size();
    
    QStringList systemTagNames = {"我的歌曲", "我的收藏", "最近播放"};
    int systemCount = 0;
    int userCount = 0;
    
    for (const Tag& tag : allTags) {
        if (systemTagNames.contains(tag.name())) {
            systemCount++;
            qDebug() << "系统标签:" << tag.name() << "ID:" << tag.id();
        } else {
            userCount++;
            qDebug() << "用户标签:" << tag.name() << "ID:" << tag.id();
        }
    }
    
    qDebug() << "系统标签数量:" << systemCount;
    qDebug() << "用户标签数量:" << userCount;
}

void testTagInfoStructure()
{
    qDebug() << "\n=== 测试TagInfo结构 ===";
    
    // 模拟AddSongDialogController的TagInfo创建
    struct TagInfo {
        QString name;
        QString displayName;
        QString color;
        QString iconPath;
        QString description;
        int songCount;
        bool isDefault;
        bool isEditable;
        
        TagInfo() : songCount(0), isDefault(false), isEditable(true) {}
    };
    
    TagInfo testTag;
    testTag.name = "测试标签";
    testTag.color = "#FF5722";
    testTag.description = "这是一个测试标签";
    
    qDebug() << "TagInfo测试:";
    qDebug() << "- 名称:" << testTag.name;
    qDebug() << "- 颜色:" << testTag.color;
    qDebug() << "- 描述:" << testTag.description;
    qDebug() << "- 可编辑:" << testTag.isEditable;
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    qDebug() << "开始标签功能修复测试...";
    
    testDatabaseConnection();
    testSystemTagsInitialization();
    testUserTagCreation();
    testTagListRetrieval();
    testTagInfoStructure();
    
    qDebug() << "\n=== 测试完成 ===";
    qDebug() << "\n修复要点总结:";
    qDebug() << "1. 修复了saveTagToDatabase方法的SQL参数不匹配问题";
    qDebug() << "2. 修复了标签类型判断逻辑，使用标签名称而非tagType()";
    qDebug() << "3. 统一了AddSongDialogController和MainWindowController的标签处理逻辑";
    qDebug() << "4. 增加了详细的调试信息输出";
    qDebug() << "5. 确保用户标签能够正确显示在界面中";
    
    return 0;
}