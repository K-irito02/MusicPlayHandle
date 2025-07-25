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
#include <QMap>
#include <QSqlDatabase>
#include "src/database/databasemanager.h" // Added for DatabaseManager

namespace {
QTranslator* g_translator = nullptr;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_controller(nullptr)
    , m_isPlaying(false)
    , m_currentPlayMode(AudioTypes::PlayMode::Loop)
{
    qDebug() << "MainWindow::MainWindow() - 开始构造主窗口";
    
    qDebug() << "MainWindow::MainWindow() - 设置UI";
    ui->setupUi(this);
    
    // 设置UI
    qDebug() << "MainWindow::MainWindow() - 调用setupUI()";
    setupUI();
    
    // 确保UI完全设置后再创建控制器
    qDebug() << "MainWindow::MainWindow() - 创建MainWindowController";
    m_controller = new MainWindowController(this, this);  // 修复：第一个参数是MainWindow*，第二个参数是QObject*
    
    // 设置信号槽连接
    qDebug() << "MainWindow::MainWindow() - 设置信号槽连接";
    setupConnections();
    
    // 初始化控制器（controller会负责标签列表的初始化）
    if (m_controller) {
        qDebug() << "MainWindow::MainWindow() - 初始化控制器";
        if (!m_controller->initialize()) {
            qWarning() << "MainWindowController初始化失败";
        }
    }
    
    // 注释掉populateDefaultTags()，让controller完全负责标签列表管理
    // qDebug() << "MainWindow::MainWindow() - 填充默认标签";
    // populateDefaultTags();
    
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
    
    // 歌曲列表控制按钮信号连接 - 连接到MainWindowController的新方法
    if (m_controller) {
        connect(ui->pushButton_play_all, &QPushButton::clicked, m_controller, &MainWindowController::onPlayAllButtonClicked);
        connect(ui->pushButton_repeat, &QPushButton::clicked, m_controller, &MainWindowController::onSelectAllButtonClicked);
        connect(ui->pushButton_sort, &QPushButton::clicked, m_controller, &MainWindowController::onClearSelectionButtonClicked);
        connect(ui->pushButton_delete, &QPushButton::clicked, m_controller, &MainWindowController::onDeleteSelectedButtonClicked);
    } else {
        // 如果控制器未初始化，暂时连接到MainWindow的方法
        connect(ui->pushButton_play_all, &QPushButton::clicked, this, &MainWindow::onPlayAllClicked);
        connect(ui->pushButton_repeat, &QPushButton::clicked, this, &MainWindow::onRepeatClicked);
        connect(ui->pushButton_sort, &QPushButton::clicked, this, &MainWindow::onSortClicked);
        connect(ui->pushButton_delete, &QPushButton::clicked, this, &MainWindow::onDeleteClicked);
    }
    
    // 播放控制按钮信号连接
    connect(ui->pushButton_previous, &QPushButton::clicked, this, &MainWindow::onPreviousClicked);
    connect(ui->pushButton_play_pause, &QPushButton::clicked, this, &MainWindow::onPlayPauseClicked);
    connect(ui->pushButton_next, &QPushButton::clicked, this, &MainWindow::onNextClicked);
    
    // 列表项点击信号连接
    connect(ui->listWidget_my_tags, &QListWidget::itemClicked, this, &MainWindow::onTagListItemClicked);
    connect(ui->listWidget_songs, &QListWidget::itemClicked, this, &MainWindow::onSongListItemClicked);
    connect(ui->listWidget_my_tags, &QListWidget::itemDoubleClicked, this, &MainWindow::onTagListItemDoubleClicked);
    connect(ui->listWidget_songs, &QListWidget::itemDoubleClicked, this, &MainWindow::onSongListItemDoubleClicked);
    
    // 滑块信号连接 - 移除这些连接，让控制器直接处理
    // connect(ui->slider_progress, &QSlider::valueChanged, this, &MainWindow::onProgressSliderChanged);
    // connect(ui->slider_volume, &QSlider::valueChanged, this, &MainWindow::onVolumeSliderChanged);
    
    // 如果控制器存在，连接控制器的信号
    if (m_controller) {
        // 连接音频状态变化信号
        connect(m_controller, &MainWindowController::stateChanged, this, &MainWindow::onAudioStateChanged);
        
        // 连接其他信号...
        connect(m_controller, &MainWindowController::addSongRequested, this, &MainWindow::showAddSongDialog);
        connect(m_controller, &MainWindowController::createTagRequested, this, &MainWindow::onActionCreateTag);
        // 移除这行，避免循环调用
        // connect(m_controller, &MainWindowController::manageTagRequested, this, &MainWindow::onActionManageTag);
        // 移除这行，避免循环调用
        // connect(m_controller, &MainWindowController::playInterfaceRequested, this, &MainWindow::onActionPlayInterface);
        // 移除这行，避免循环调用
        // connect(m_controller, &MainWindowController::settingsRequested, this, &MainWindow::onActionSettings);
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
    ui->label_song_title->setText("未选择歌曲");
                ui->label_song_artist->setText("");
    
    // 设置播放模式按钮初始状态（Loop模式）
    ui->pushButton_play_pause->setText("播放");
    
    // 设置音量滑块初始值
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
    if (m_controller && m_controller->isInitialized()) {
        m_controller->onActionManageTag();
    } else {
        // 检查数据库连接 - 使用DatabaseManager而不是直接检查QSqlDatabase
        DatabaseManager* dbManager = DatabaseManager::instance();
        if (!dbManager || !dbManager->isValid()) {
            QMessageBox::critical(this, "数据库错误", "数据库连接不可用，无法打开标签管理对话框。");
            return;
        }
        
        try {
            ManageTagDialog dialog(this);
            dialog.setWindowTitle("管理标签");
            dialog.exec(); // 使用exec()而不是show()，确保对话框模态显示并正确管理生命周期
            if (m_controller && m_controller->isInitialized()) {
                m_controller->refreshTagListPublic();
            }
        } catch (const std::exception& e) {
            QMessageBox::critical(this, "错误", QString("打开标签管理对话框时发生错误: %1").arg(e.what()));
        } catch (...) {
            QMessageBox::critical(this, "错误", "打开标签管理对话框时发生未知错误");
        }
    }
}

void MainWindow::onActionPlayInterface()
{
    if (m_controller && m_controller->isInitialized()) {
        m_controller->onActionPlayInterface();
    } else {
        // 检查数据库连接
        DatabaseManager* dbManager = DatabaseManager::instance();
        if (!dbManager || !dbManager->isValid()) {
            QMessageBox::critical(this, "数据库错误", "数据库连接不可用，无法打开播放界面。");
            return;
        }
        
        try {
            PlayInterface* dialog = new PlayInterface(this);
            dialog->setAttribute(Qt::WA_DeleteOnClose, true);
            dialog->show();
            refreshPlaybackStatus();
        } catch (const std::exception& e) {
            QMessageBox::critical(this, "错误", QString("打开播放界面时发生错误: %1").arg(e.what()));
        } catch (...) {
            QMessageBox::critical(this, "错误", "打开播放界面时发生未知错误");
        }
    }
}

void MainWindow::onActionSettings()
{
    if (m_controller && m_controller->isInitialized()) {
        m_controller->onActionSettings();
    } else {
        // 检查数据库连接
        DatabaseManager* dbManager = DatabaseManager::instance();
        if (!dbManager || !dbManager->isValid()) {
            QMessageBox::critical(this, "数据库错误", "数据库连接不可用，无法打开设置对话框。");
            return;
        }
        
        try {
            SettingsDialog dialog(this);
            dialog.setWindowTitle("设置");
            connect(&dialog, &SettingsDialog::settingsChanged, this, &MainWindow::refreshSettings);
            dialog.exec();
        } catch (const std::exception& e) {
            QMessageBox::critical(this, "错误", QString("打开设置对话框时发生错误: %1").arg(e.what()));
        } catch (...) {
            QMessageBox::critical(this, "错误", "打开设置对话框时发生未知错误");
        }
    }
}

// 歌曲列表控制按钮槽函数实现
void MainWindow::onPlayAllClicked()
{
    // 切换播放/暂停状态
    if (m_isPlaying) {
        // 当前正在播放，执行暂停
        m_isPlaying = false;
        ui->pushButton_play_all->setText("开始播放");
        showStatusMessage("歌曲已暂停");
        qDebug() << "歌曲暂停播放";
        
        // 如果有控制器，调用暂停功能
        if (m_controller) {
            // TODO: 调用控制器的暂停方法
        }
    } else {
        // 当前未播放，执行播放
        m_isPlaying = true;
        ui->pushButton_play_all->setText("暂停播放");
        showStatusMessage("开始播放歌曲");
        qDebug() << "开始播放歌曲";
        
        // 如果有控制器，调用播放功能
        if (m_controller) {
            // TODO: 调用控制器的播放方法
        }
    }
}



void MainWindow::onRepeatClicked()
{
    // 全选当前标签下的所有歌曲
    if (ui->listWidget_songs->count() == 0) {
        showStatusMessage("当前列表为空，无歌曲可选择");
        qDebug() << "全选操作：当前列表为空";
        return;
    }
    
    // 选择所有歌曲
    ui->listWidget_songs->selectAll();
    int songCount = ui->listWidget_songs->count();
    showStatusMessage(QString("已全选 %1 首歌曲").arg(songCount));
    qDebug() << QString("全选操作完成：选中 %1 首歌曲").arg(songCount);
}

void MainWindow::onSortClicked()
{
    // 取消选中状态
    int selectedCount = ui->listWidget_songs->selectedItems().count();
    if (selectedCount == 0) {
        showStatusMessage("当前没有选中的歌曲");
        qDebug() << "取消全选操作：当前没有选中的歌曲";
        return;
    }
    
    ui->listWidget_songs->clearSelection();
    showStatusMessage(QString("已取消选中 %1 首歌曲").arg(selectedCount));
    qDebug() << QString("取消全选操作完成：取消选中 %1 首歌曲").arg(selectedCount);
}

void MainWindow::onDeleteClicked()
{
    // 删除选中状态的歌曲
    QList<QListWidgetItem*> selectedItems = ui->listWidget_songs->selectedItems();
    if (selectedItems.isEmpty()) {
        QMessageBox::information(this, "提示", "请先选择要删除的歌曲");
        qDebug() << "删除操作：没有选中的歌曲";
        return;
    }
    
    int selectedCount = selectedItems.size();
    int ret = QMessageBox::question(this, "确认删除", 
                                    QString("确定要删除选中的 %1 首歌曲吗？\n\n注意：这将从数据库中删除歌曲记录，但不会删除实际文件。").arg(selectedCount),
                                    QMessageBox::Yes | QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        qDebug() << QString("开始删除 %1 首选中的歌曲").arg(selectedCount);
        
        // 如果有控制器，调用控制器的删除方法
        if (m_controller) {
            // TODO: 调用控制器的批量删除歌曲方法
            // m_controller->deleteSelectedSongs(selectedItems);
        }
        
        // 临时实现：直接从UI中移除
        for (QListWidgetItem* item : selectedItems) {
            delete item;
        }
        
        showStatusMessage(QString("已删除 %1 首歌曲").arg(selectedCount));
        qDebug() << QString("删除操作完成：删除了 %1 首歌曲").arg(selectedCount);
    } else {
        qDebug() << "删除操作：用户取消删除";
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
    qDebug() << "[MainWindow::onPlayPauseClicked] 播放/暂停按钮被点击";
    qDebug() << "[MainWindow::onPlayPauseClicked] 当前按钮文本:" << ui->pushButton_play_pause->text();
    
    if (m_controller) {
        qDebug() << "[MainWindow::onPlayPauseClicked] 调用控制器方法";
        m_controller->onPlayButtonClicked();
    } else {
        qDebug() << "[MainWindow::onPlayPauseClicked] 控制器为空，使用简单逻辑";
        // 如果没有控制器，使用简单的切换逻辑
        QString currentText = ui->pushButton_play_pause->text();
        if (currentText == "播放") {
            ui->pushButton_play_pause->setText("暂停");
            showStatusMessage("开始播放");
        } else {
            ui->pushButton_play_pause->setText("播放");
            showStatusMessage("暂停播放");
        }
        qDebug() << "播放/暂停切换:" << ui->pushButton_play_pause->text();
    }
    
    qDebug() << "[MainWindow::onPlayPauseClicked] 按钮点击处理完成";
}

void MainWindow::onAudioStateChanged(MainWindowState state)
{
    qDebug() << "[MainWindow::onAudioStateChanged] 收到状态变化信号:" << static_cast<int>(state);
    
    // 根据音频状态更新播放按钮文本
    switch (state) {
    case MainWindowState::Playing:
        ui->pushButton_play_pause->setText("暂停");
        qDebug() << "[MainWindow::onAudioStateChanged] 设置播放按钮文本为'暂停'";
        break;
    case MainWindowState::Paused:
        ui->pushButton_play_pause->setText("播放");
        qDebug() << "[MainWindow::onAudioStateChanged] 设置播放按钮文本为'播放'";
        break;
    default:
        qDebug() << "[MainWindow::onAudioStateChanged] 不更新播放按钮文本，状态:" << static_cast<int>(state);
        break;
    }
    
    qDebug() << "[MainWindow::onAudioStateChanged] 状态变化处理完成";
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
            ui->label_song_artist->setText("");
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

// 进度条相关功能已由自定义MusicProgressBar组件实现

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
        
        // 连接标签创建信号到主界面刷新
        connect(dialog.getController(), &AddSongDialogController::tagCreated,
                this, [this](const QString& tagName, bool isSystemTag) {
                    Q_UNUSED(isSystemTag)
                    qDebug() << "[MainWindow] 接收到标签创建信号:" << tagName;
                    if (m_controller) {
                        m_controller->refreshTagList();
                    }
                });
    }
    
    // 可选：设置可用标签等
    if (dialog.exec() == QDialog::Accepted) {
        QStringList files = dialog.getAllFiles(); // 获取所有添加的文件，而不是只获取选中的文件
        QMap<QString, QStringList> fileTagAssignments = dialog.getFileTagAssignments(); // 获取文件标签关联信息
        
        if (!files.isEmpty()) {
            showStatusMessage(QString("成功添加 %1 个音频文件").arg(files.size()));
            if (m_controller) {
                m_controller->addSongs(files, fileTagAssignments); // 传递标签关联信息
            }
        } else {
            showStatusMessage("未添加音频文件");
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
