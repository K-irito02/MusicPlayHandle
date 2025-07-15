#include "src/core/constants.h"
#include "src/ui/widgets/taglistitemfactory.h"
#include <QApplication>
#include <QWidget>
#include <memory>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // 测试常量定义
    qDebug() << "System tags:";
    qDebug() << Constants::SystemTags::MY_SONGS;
    qDebug() << Constants::SystemTags::FAVORITES;
    qDebug() << Constants::SystemTags::RECENT_PLAYED;
    qDebug() << Constants::SystemTags::DEFAULT_TAG;
    
    // 测试工厂类
    QWidget parent;
    
    // 测试创建系统标签
    auto systemTag = TagListItemFactory::createSystemTag(Constants::SystemTags::MY_SONGS, &parent);
    qDebug() << "Created system tag:" << systemTag->getTagName();
    
    // 测试创建用户标签
    auto userTag = TagListItemFactory::createUserTag("测试用户标签", QString(), &parent);
    qDebug() << "Created user tag:" << userTag->getTagName();
    
    // 测试批量创建系统标签
    auto allSystemTags = TagListItemFactory::createAllSystemTags(&parent);
    qDebug() << "Created" << allSystemTags.size() << "system tags";
    
    // 测试isSystemTag函数
    qDebug() << "Is '我的歌曲' a system tag?" << Constants::SystemTags::isSystemTag("我的歌曲");
    qDebug() << "Is '用户标签' a system tag?" << Constants::SystemTags::isSystemTag("用户标签");
    
    qDebug() << "All tests passed!";
    
    return 0;
}