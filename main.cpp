#include "mainwindow.h"
#include "src/core/applicationmanager.h"
#include "src/core/logger.h"
#include "version.h"

#include <QApplication>
#include <QMessageBox>
#include <QDebug>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QStyleFactory>
#include <QDir>
#include <QStandardPaths>
#include <QTranslator>
#include <QLocale>
#include <QFontDatabase>
#include <QSplashScreen>
#include <QPixmap>
#include <QTimer>
#include <QEventLoop>

// 所有初始化和清理工作现在由ApplicationManager处理

int main(int argc, char *argv[])
{
    // Qt6中高DPI支持已默认启用，无需手动设置
    QApplication app(argc, argv);
    
    // 设置应用程序基本信息
    app.setApplicationName("Qt6音频播放器");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Qt6音频播放器开发团队");
    app.setOrganizationDomain("musicplayer.qt6.com");
    
    // 设置应用程序图标
    app.setWindowIcon(QIcon(":/new/prefix1/images/applicationIcon.png"));
    
    // 命令行解析
    QCommandLineParser parser;
    parser.setApplicationDescription("基于Qt6的音频播放器");
    parser.addHelpOption();
    parser.addVersionOption();
    
    // 添加命令行选项
    QCommandLineOption debugOption(QStringList() << "d" << "debug", "启用调试模式");
    parser.addOption(debugOption);
    
    QCommandLineOption testOption(QStringList() << "t" << "test", "运行测试");
    parser.addOption(testOption);
    
    QCommandLineOption configOption(QStringList() << "c" << "config", "配置文件路径", "path");
    parser.addOption(configOption);
    
    QCommandLineOption logLevelOption(QStringList() << "l" << "log-level", "日志级别 (0-4)", "level");
    parser.addOption(logLevelOption);
    
    QCommandLineOption noGuiOption("no-gui", "无GUI模式");
    parser.addOption(noGuiOption);
    
    parser.process(app);
    
    qDebug() << "main() - 程序开始执行";
    
    // 初始化应用程序管理器
    qDebug() << "main() - 获取ApplicationManager实例";
    ApplicationManager* appManager = ApplicationManager::instance();
    
    // 设置调试模式
    if (parser.isSet(debugOption)) {
        qDebug() << "main() - 启用调试模式";
        appManager->enableDebugMode(true);
        appManager->enableDeveloperMode(true);
    }
    
    // 初始化应用程序
    qDebug() << "main() - 开始初始化应用程序";
    if (!appManager->initialize(&app, argc, argv)) {
        qDebug() << "main() - 应用程序初始化失败";
        QMessageBox::critical(nullptr, "错误", "应用程序初始化失败");
        return -1;
    }
    qDebug() << "main() - 应用程序初始化成功";
    
    // 如果是测试模式，运行测试
    if (parser.isSet(testOption)) {
        qDebug() << "测试模式已被禁用";
        qDebug() << "测试功能已从应用程序中移除";
        return 0;
    }
    
    // 启动应用程序
    qDebug() << "main() - 开始启动应用程序";
    if (!appManager->start()) {
        qDebug() << "main() - 应用程序启动失败";
        QMessageBox::critical(nullptr, "错误", "应用程序启动失败");
        return -1;
    }
    qDebug() << "main() - 应用程序启动成功";
    
    // 运行主事件循环
    int result = app.exec();
    
    // 关闭应用程序
    appManager->shutdown();
    
    // 清理资源
    ApplicationManager::cleanup();
    
    qDebug() << "Application terminated with code:" << result;
    return result;
}
