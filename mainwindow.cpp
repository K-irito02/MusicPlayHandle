#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "src/ui/controllers/mainwindowcontroller.h"
#include "src/ui/widgets/taglistitem.h"
#include <QListWidgetItem>
#include <QDebug>
#include "src/ui/dialogs/AddSongDialog.h"
#include "src/ui/controllers/AddSongDialogController.h"
#include "src/ui/dialogs/CreateTagDialog.h"
#include "src/ui/dialogs/ManageTagDialog.h"
#include "src/ui/dialogs/PlayInterface.h"
#include "src/ui/dialogs/SettingsDialog.h"
#include <QSettings>
#include <QApplication>
#include <QTranslator>
#include <QLocale>
#include "src/audio/audioengine.h"

namespace {
QTranslator* g_translator = nullptr;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_controller(nullptr)
{
    qDebug() << "MainWindow::MainWindow() - 开始构造主窗口";
    
    qDebug() << "MainWindow::MainWindow() - 设置UI";
    ui->setupUi(this);
    
    // 设置UI
    qDebug() << "MainWindow::MainWindow() - 调用setupUI()";
    setupUI();
    
    // 创建控制器
    qDebug() << "MainWindow::MainWindow() - 创建MainWindowController";
    m_controller = new MainWindowController(this, this);
    
    // 设置信号槽连接
    qDebug() << "MainWindow::MainWindow() - 设置信号槽连接";
    setupConnections();
    
    // 填充默认标签
    qDebug() << "MainWindow::MainWindow() - 填充默认标签";
    populateDefaultTags();
    
    // 初始化控制器
    if (m_controller) {
        qDebug() << "MainWindow::MainWindow() - 初始化控制器";
        m_controller->initialize();
    }
    
    // 显示状态消息
    qDebug() << "MainWindow::MainWindow() - 显示状态消息";
    showStatusMessage("应用程序已启动");
    
    qDebug() << "MainWindow::MainWindow() - 主窗口构造完成";
}

MainWindow::~MainWindow()
{

    if (m_controller) {
        m_controller->shutdown();
        delete m_controller;
    }
    delete ui;
}

void MainWindow::setupConnections()
{
    // 工具栏按钮信号连接
    connect(ui->actionAddMusic, &QAction::triggered, this, &MainWindow::onActionAddMusic);
    connect(ui->actionCreateTag, &QAction::triggered, this, &MainWindow::onActionCreateTag);
    connect(ui->actionManageTag, &QAction::triggered, this, &MainWindow::onActionManageTag);
    connect(ui->actionPlayInterface, &QAction::triggered, this, &MainWindow::onActionPlayInterface);
    connect(ui->actionSettings, &QAction::triggered, this, &MainWindow::onActionSettings);
    
    // 歌曲列表控制按钮信号连接
    connect(ui->pushButton_play_all, &QPushButton::clicked, this, &MainWindow::onPlayAllClicked);
    connect(ui->pushButton_shuffle, &QPushButton::clicked, this, &MainWindow::onShuffleClicked);
    connect(ui->pushButton_repeat, &QPushButton::clicked, this, &MainWindow::onRepeatClicked);
    connect(ui->pushButton_sort, &QPushButton::clicked, this, &MainWindow::onSortClicked);
    connect(ui->pushButton_delete, &QPushButton::clicked, this, &MainWindow::onDeleteClicked);
    
    // 播放控制按钮信号连接
    connect(ui->pushButton_previous, &QPushButton::clicked, this, &MainWindow::onPreviousClicked);
    connect(ui->pushButton_play_pause, &QPushButton::clicked, this, &MainWindow::onPlayPauseClicked);
    connect(ui->pushButton_stop, &QPushButton::clicked, this, &MainWindow::onStopClicked);
    connect(ui->pushButton_next, &QPushButton::clicked, this, &MainWindow::onNextClicked);
    
    // 列表项点击信号连接
    connect(ui->listWidget_my_tags, &QListWidget::itemClicked, this, &MainWindow::onTagListItemClicked);
    connect(ui->listWidget_songs, &QListWidget::itemClicked, this, &MainWindow::onSongListItemClicked);
    connect(ui->listWidget_my_tags, &QListWidget::itemDoubleClicked, this, &MainWindow::onTagListItemDoubleClicked);
    connect(ui->listWidget_songs, &QListWidget::itemDoubleClicked, this, &MainWindow::onSongListItemDoubleClicked);
    
    // 注意：TagListItem的信号连接在populateDefaultTags()方法中进行
    
    // 滑块信号连接
    connect(ui->slider_progress, &QSlider::valueChanged, this, &MainWindow::onProgressSliderChanged);
    connect(ui->slider_volume, &QSlider::valueChanged, this, &MainWindow::onVolumeSliderChanged);
    
    // 如果控制器存在，连接控制器的信号
    if (m_controller) {
        connect(m_controller, &MainWindowController::addSongRequested, this, &MainWindow::showAddSongDialog);
        connect(m_controller, &MainWindowController::createTagRequested, this, &MainWindow::showCreateTagDialog);
        connect(m_controller, &MainWindowController::manageTagRequested, this, &MainWindow::showManageTagDialog);
        connect(m_controller, &MainWindowController::playInterfaceRequested, this, &MainWindow::showPlayInterfaceDialog);
        connect(m_controller, &MainWindowController::settingsRequested, this, &MainWindow::showSettingsDialog);
        connect(m_controller, &MainWindowController::errorOccurred, this, [this](const QString& error) {
            QMessageBox::critical(this, "错误", error);
        });
    }
}

void MainWindow::setupUI()
{
    // 设置窗口标题
    setWindowTitle("Qt6音频播放器 - v1.0.0");
    
    // 设置初始状态
    ui->pushButton_play_pause->setText("播放");
    ui->label_current_time->setText("00:00");
    ui->label_total_time->setText("00:00");
    ui->label_song_title->setText("未选择歌曲");
    ui->label_song_artist->setText("未知艺术家");
    
    // 设置滑块初始值
    ui->slider_progress->setRange(0, 100);
    ui->slider_progress->setValue(0);
    ui->slider_volume->setRange(0, 100);
    ui->slider_volume->setValue(50);
}

void MainWindow::populateDefaultTags()
{
    // 清空现有项目
    ui->listWidget_my_tags->clear();
    
    // 创建系统标签（不可删除，不可编辑）
    QStringList coreTagNames = {"我的歌曲", "我的收藏", "最近播放"};
    
    for (const QString& tagName : coreTagNames) {
        // 创建自定义标签项控件（系统标签不可编辑，不可删除）
        TagListItem* tagItem = new TagListItem(tagName, QString(), false, false);
        
        // 连接信号
        connect(tagItem, &TagListItem::tagClicked, this, &MainWindow::onTagItemClicked);
        connect(tagItem, &TagListItem::tagDoubleClicked, this, &MainWindow::onTagItemDoubleClicked);
        connect(tagItem, &TagListItem::editRequested, this, &MainWindow::onTagEditRequested);
        
        // 创建列表项并设置自定义控件
        QListWidgetItem* listItem = new QListWidgetItem();
        listItem->setSizeHint(tagItem->sizeHint());
        
        // 添加到列表
        ui->listWidget_my_tags->addItem(listItem);
        ui->listWidget_my_tags->setItemWidget(listItem, tagItem);
    }
}

void MainWindow::showStatusMessage(const QString& message)
{
    ui->statusbar->showMessage(message, 3000);
}

// 工具栏按钮槽函数实现 - 委托给控制器
void MainWindow::onActionAddMusic()
{
    if (m_controller) {
        m_controller->onActionAddMusic();
    } else {
        // 如果控制器不存在，使用原来的实现
        QStringList filters;
        filters << "音频文件 (*.mp3 *.wav *.flac *.ogg *.aac *.wma)"
                << "MP3 文件 (*.mp3)"
                << "WAV 文件 (*.wav)"
                << "FLAC 文件 (*.flac)"
                << "OGG 文件 (*.ogg)"
                << "所有文件 (*.*)";
        
        QStringList files = QFileDialog::getOpenFileNames(
            this,
            "选择音频文件",
            QDir::homePath(),
            filters.join(";;")
        );
        
        if (!files.isEmpty()) {
            showStatusMessage(QString("选择了 %1 个音频文件").arg(files.size()));
            
            // 这里应该调用音频引擎或控制器来处理文件
            // 目前只是显示选择的文件数量
            for (const QString& file : files) {
                qDebug() << "添加音频文件:" << file;
            }
        }
    }
}

void MainWindow::onActionCreateTag()
{
    CreateTagDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QString tagName = dialog.getTagName();
        QString imagePath = dialog.getTagImagePath();
        if (!tagName.isEmpty()) {
            if (m_controller) {
                m_controller->addTag(tagName, imagePath);
                showStatusMessage("标签创建请求已提交");
            } else {
                showStatusMessage("控制器未初始化");
            }
        } else {
            showStatusMessage("标签名不能为空");
        }
    } else {
        showStatusMessage("已取消创建标签");
    }
}

void MainWindow::onActionManageTag()
{
    if (m_controller) {
        m_controller->onActionManageTag();
    } else {
        ManageTagDialog dialog(this);
        dialog.setWindowTitle("管理标签");
        dialog.show();
        if (m_controller) {
            m_controller->refreshTagListPublic();
        }
    }
}

void MainWindow::onActionPlayInterface()
{
    if (m_controller) {
        m_controller->onActionPlayInterface();
    } else {
        PlayInterface dialog(this);
        dialog.setWindowTitle("播放界面");
        dialog.show();
        refreshPlaybackStatus();
    }
}

void MainWindow::onActionSettings()
{
    if (m_controller) {
        m_controller->onActionSettings();
    } else {
        SettingsDialog dialog(this);
        dialog.setWindowTitle("设置");
        connect(&dialog, &SettingsDialog::settingsChanged, this, &MainWindow::refreshSettings);
        dialog.exec();
        // 可选：关闭后刷新主界面设置
        // refreshSettings();
    }
}

// 歌曲列表控制按钮槽函数实现
void MainWindow::onPlayAllClicked()
{
    showStatusMessage("开始播放所有歌曲");
    ui->pushButton_play_pause->setText("暂停");
    qDebug() << "开始播放所有歌曲";
}

void MainWindow::onShuffleClicked()
{
    showStatusMessage("随机播放模式");
    qDebug() << "随机播放模式";
}

void MainWindow::onRepeatClicked()
{
    showStatusMessage("重复播放模式");
    qDebug() << "重复播放模式";
}

void MainWindow::onSortClicked()
{
    showStatusMessage("取消全选");
    ui->listWidget_songs->clearSelection();
    qDebug() << "取消全选";
}

void MainWindow::onDeleteClicked()
{
    QList<QListWidgetItem*> selectedItems = ui->listWidget_songs->selectedItems();
    if (selectedItems.isEmpty()) {
        QMessageBox::information(this, "提示", "请先选择要删除的歌曲");
        return;
    }
    
    int ret = QMessageBox::question(this, "确认删除", 
                                    QString("确定要删除选中的 %1 首歌曲吗？").arg(selectedItems.size()),
                                    QMessageBox::Yes | QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        for (QListWidgetItem* item : selectedItems) {
            delete item;
        }
        showStatusMessage(QString("已删除 %1 首歌曲").arg(selectedItems.size()));
    }
}

// 播放控制按钮槽函数实现
void MainWindow::onPreviousClicked()
{
    if (m_controller) {
        m_controller->onPreviousButtonClicked();
    } else {
        showStatusMessage("上一首");
        qDebug() << "上一首";
    }
}

void MainWindow::onPlayPauseClicked()
{
    QString currentText = ui->pushButton_play_pause->text();
    if (currentText == "播放") {
        ui->pushButton_play_pause->setText("暂停");
        showStatusMessage("开始播放");
        if (m_controller) {
            m_controller->onPlayButtonClicked();
        }
    } else {
        ui->pushButton_play_pause->setText("播放");
        showStatusMessage("暂停播放");
        if (m_controller) {
            m_controller->onPauseButtonClicked();
        }
    }
    qDebug() << "播放/暂停切换:" << ui->pushButton_play_pause->text();
}

void MainWindow::onStopClicked()
{
    ui->pushButton_play_pause->setText("播放");
    ui->slider_progress->setValue(0);
    ui->label_current_time->setText("00:00");
    showStatusMessage("停止播放");
    if (m_controller) {
        m_controller->onStopButtonClicked();
    }
    qDebug() << "停止播放";
}

void MainWindow::onNextClicked()
{
    if (m_controller) {
        m_controller->onNextButtonClicked();
    } else {
        showStatusMessage("下一首");
        qDebug() << "下一首";
    }
}

// 列表项点击事件实现
void MainWindow::onTagListItemClicked(QListWidgetItem* item)
{
    if (m_controller) {
        m_controller->onTagListItemClicked(item);
    } else {
        if (item) {
            showStatusMessage(QString("选择标签: %1").arg(item->text()));
            qDebug() << "选择标签:" << item->text();
            
            // 这里应该根据选择的标签加载相应的歌曲
            // 目前只是清空歌曲列表并显示占位符
            ui->listWidget_songs->clear();
            ui->listWidget_songs->addItem(QString("正在加载 '%1' 标签下的歌曲...").arg(item->text()));
        }
    }
}

void MainWindow::onSongListItemClicked(QListWidgetItem* item)
{
    if (m_controller) {
        m_controller->onSongListItemClicked(item);
    } else {
        if (item) {
            showStatusMessage(QString("选择歌曲: %1").arg(item->text()));
            ui->label_song_title->setText(item->text());
            ui->label_song_artist->setText("未知艺术家");
            qDebug() << "选择歌曲:" << item->text();
        }
    }
}

void MainWindow::onTagListItemDoubleClicked(QListWidgetItem* item)
{
    if (m_controller) {
        m_controller->onTagListItemDoubleClicked(item);
    } else {
        if (item) {
            showStatusMessage(QString("编辑标签: %1").arg(item->text()));
            qDebug() << "双击标签:" << item->text();
        }
    }
}

void MainWindow::onSongListItemDoubleClicked(QListWidgetItem* item)
{
    if (m_controller) {
        m_controller->onSongListItemDoubleClicked(item);
    } else {
        if (item) {
            showStatusMessage(QString("播放歌曲: %1").arg(item->text()));
            ui->label_song_title->setText(item->text());
            ui->pushButton_play_pause->setText("暂停");
            qDebug() << "双击播放歌曲:" << item->text();
        }
    }
}

// 滑块事件实现
void MainWindow::onProgressSliderChanged(int value)
{
    if (m_controller) {
        m_controller->onProgressSliderChanged(value);
    } else {
        // 这里应该控制音频播放进度
        qDebug() << "进度条变化:" << value;
    }
}

void MainWindow::onVolumeSliderChanged(int value)
{
    if (m_controller) {
        m_controller->onVolumeSliderChanged(value);
    } else {
        showStatusMessage(QString("音量: %1%").arg(value));
        qDebug() << "音量变化:" << value;
    }
}

void MainWindow::showAddSongDialog()
{
    AddSongDialog dialog(this);
    
    // 连接标签列表变化信号到主界面刷新
    if (dialog.getController()) {
        connect(dialog.getController(), &AddSongDialogController::tagListChanged,
                m_controller, &MainWindowController::refreshTagList);
    }
    
    // 可选：设置可用标签等
    if (dialog.exec() == QDialog::Accepted) {
        QStringList files = dialog.getSelectedFiles();
        if (!files.isEmpty()) {
            showStatusMessage(QString("成功添加 %1 个音频文件").arg(files.size()));
            if (m_controller) {
                m_controller->addSongs(files);
            }
        } else {
            showStatusMessage("未选择音频文件");
        }
    } else {
        showStatusMessage("已取消添加音乐");
    }
}

void MainWindow::refreshPlaybackStatus()
{
    if (m_controller) {
        // 通过 public 方法获取播放状态
        Song currentSong = m_controller->getCurrentSong();
        int volume = m_controller->getCurrentVolume();
        qint64 position = m_controller->getCurrentPosition();
        qint64 duration = m_controller->getCurrentDuration();
        m_controller->updatePlaybackInfo(currentSong);
        m_controller->updateProgressBar(static_cast<int>(position), static_cast<int>(duration));
        m_controller->updateVolumeDisplay(volume);
        m_controller->refreshWindowTitle();
    }
    showStatusMessage(tr("播放状态已刷新"));
}

void MainWindow::applyLanguage(int languageIndex)
{
    static const QStringList languageFiles = {":/translations/en_US.qm", ":/translations/zh_CN.qm"};
    if (g_translator) {
        qApp->removeTranslator(g_translator);
        delete g_translator;
        g_translator = nullptr;
    }
    if (languageIndex == 1) { // English
        g_translator = new QTranslator;
        if (g_translator->load(languageFiles[0])) {
            qApp->installTranslator(g_translator);
        }
    } else if (languageIndex == 0) { // 简体中文
        g_translator = new QTranslator;
        if (g_translator->load(languageFiles[1])) {
            qApp->installTranslator(g_translator);
        }
    }
    // 重新翻译界面（如需刷新UI）
    // ...
}

void MainWindow::refreshSettings()
{
    QSettings settings;
    int theme = settings.value("theme", 0).toInt();
    int language = settings.value("language", 0).toInt();
    int volume = settings.value("defaultVolume", 50).toInt();
    // 应用主题
    if (theme == 0) {
        qApp->setStyleSheet("");
    } else {
        qApp->setStyleSheet("QWidget { background-color: #232323; color: #fff; }");
    }
    // 应用语言
    applyLanguage(language);
    // 应用音量
    if (m_controller) {
        m_controller->setCurrentVolume(volume);
    }
    showStatusMessage(tr("设置已应用"));
}

void MainWindow::addSongs(const QStringList& filePaths)
{
    if (m_controller) {
        m_controller->addSongs(filePaths);
    } else {
        showStatusMessage("未能添加歌曲：控制器不存在");
    }
}

// TagListItem相关槽函数实现
void MainWindow::onTagItemClicked(const QString& tagName)
{
    showStatusMessage(QString("选择标签: %1").arg(tagName));
    qDebug() << "TagListItem点击:" << tagName;
    
    // 清空歌曲列表并显示占位符
    ui->listWidget_songs->clear();
    ui->listWidget_songs->addItem(QString("正在加载 '%1' 标签下的歌曲...").arg(tagName));
    
    // 更新选中状态
    for (int i = 0; i < ui->listWidget_my_tags->count(); ++i) {
        QListWidgetItem* item = ui->listWidget_my_tags->item(i);
        TagListItem* tagItem = qobject_cast<TagListItem*>(ui->listWidget_my_tags->itemWidget(item));
        if (tagItem) {
            tagItem->setSelected(tagItem->getTagName() == tagName);
        }
    }
    
    // 如果控制器存在，委托给控制器处理
    if (m_controller) {
        // 这里可以调用控制器的相关方法来加载标签下的歌曲
        // m_controller->loadSongsByTag(tagName);
    }
}

void MainWindow::onTagItemDoubleClicked(const QString& tagName)
{
    showStatusMessage(QString("双击标签: %1").arg(tagName));
    qDebug() << "TagListItem双击:" << tagName;
    
    // 双击可以触发编辑或其他操作
    if (m_controller) {
        // m_controller->onTagDoubleClicked(tagName);
    }
}

void MainWindow::onTagEditRequested(const QString& tagName)
{
    showStatusMessage(QString("编辑标签: %1").arg(tagName));
    qDebug() << "TagListItem编辑请求:" << tagName;
    
    // 检查是否是系统标签
    if (tagName == "我的歌曲" || tagName == "我的收藏" || tagName == "最近播放") {
        QMessageBox::information(this, "提示", "系统标签不可编辑");
        return;
    }
    
    if (m_controller) {
        m_controller->editTagFromMainWindow(tagName);
    }
}

// 主界面弹窗槽函数实现
void MainWindow::showCreateTagDialog() {
    CreateTagDialog dlg(this);
    dlg.exec();
}
void MainWindow::showManageTagDialog() {
    ManageTagDialog dlg(this);
    dlg.exec();
}
void MainWindow::showPlayInterfaceDialog() {
    PlayInterface* dlg = new PlayInterface(this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->show();
}
void MainWindow::showSettingsDialog() {
    SettingsDialog dlg(this);
    dlg.exec();
}
