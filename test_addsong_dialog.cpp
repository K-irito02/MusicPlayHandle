#include <QApplication>
#include <QDebug>
#include "src/ui/dialogs/AddSongDialog.h"
#include "src/core/constants.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // 测试AddSongDialog的修复
    AddSongDialog dialog;
    
    // 验证系统标签常量
    qDebug() << "系统标签:";
    qDebug() << "我的歌曲:" << Constants::SystemTags::MY_SONGS;
    qDebug() << "我的收藏:" << Constants::SystemTags::FAVORITES;
    qDebug() << "最近播放:" << Constants::SystemTags::RECENT_PLAYED;
    
    // 验证系统标签检查函数
    qDebug() << "\n系统标签检查:";
    qDebug() << "'我的歌曲' 是系统标签:" << Constants::SystemTags::isSystemTag("我的歌曲");
    qDebug() << "'我的收藏' 是系统标签:" << Constants::SystemTags::isSystemTag("我的收藏");
    qDebug() << "'用户标签' 是系统标签:" << Constants::SystemTags::isSystemTag("用户标签");
    
    // 显示对话框
    dialog.show();
    
    return app.exec();
}