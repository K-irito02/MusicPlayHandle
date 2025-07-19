#include <QApplication>
#include <QTest>
#include <QSignalSpy>
#include <QMessageBox>
#include "../src/ui/dialogs/ManageTagDialog.h"
#include "../src/database/databasemanager.h"

class TestManageTagDialog : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        // 初始化数据库
        DatabaseManager::instance().initialize();
    }

    void testManageTagDialogCreation()
    {
        // 测试ManageTagDialog的创建
        ManageTagDialog dialog;
        QVERIFY(dialog.isVisible() == false);
        
        // 测试对话框可以正常显示
        dialog.show();
        QVERIFY(dialog.isVisible() == true);
        
        // 测试对话框可以正常关闭
        dialog.close();
        QVERIFY(dialog.isVisible() == false);
    }

    void testManageTagDialogModal()
    {
        // 测试模态对话框
        ManageTagDialog dialog;
        dialog.setModal(true);
        
        // 显示对话框但不阻塞
        dialog.show();
        QVERIFY(dialog.isVisible() == true);
        
        dialog.close();
    }

    void testManageTagDialogController()
    {
        // 测试控制器是否正确初始化
        ManageTagDialog dialog;
        
        // 控制器应该在构造函数中初始化
        QVERIFY(dialog.getSongListWidget() != nullptr);
    }

    void cleanupTestCase()
    {
        // 清理数据库连接
        DatabaseManager::instance().shutdown();
    }
};

QTEST_MAIN(TestManageTagDialog)
#include "test_manage_tag_dialog.moc" 