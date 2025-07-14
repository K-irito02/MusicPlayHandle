#include "AddSongDialog.h"
#include "ui_AddSongDialog.h"
#include "../controllers/addsongdialogcontroller.h"
#include "../widgets/taglistitem.h"

AddSongDialog::AddSongDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AddSongDialog)
    , m_controller(nullptr)
{
    ui->setupUi(this);
    
    // 创建控制器
    m_controller = new AddSongDialogController(this, this);
    
    // 设置连接
    setupConnections();
    
    // 设置UI
    setupUI();
    
    // 初始化控制器
    if (m_controller) {
        m_controller->initialize();
    }
}

AddSongDialog::~AddSongDialog()
{
    if (m_controller) {
        m_controller->shutdown();
        delete m_controller;
    }
    delete ui;
}

void AddSongDialog::dragEnterEvent(QDragEnterEvent* event)
{
    // 拖拽进入事件
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void AddSongDialog::dropEvent(QDropEvent* event)
{
    // 拖拽放下事件
    if (event->mimeData()->hasUrls() && m_controller) {
        QStringList filePaths;
        for (const QUrl& url : event->mimeData()->urls()) {
            if (url.isLocalFile()) {
                filePaths.append(url.toLocalFile());
            }
        }
        
        if (!filePaths.isEmpty()) {
            m_controller->addFiles(filePaths);
            event->acceptProposedAction();
        }
    }
}

QStringList AddSongDialog::getSelectedFiles() const
{
    // 获取选中的文件列表
    QStringList selectedFiles;
    
    QList<QListWidgetItem*> selectedItems = ui->listWidget_added_songs->selectedItems();
    for (QListWidgetItem* item : selectedItems) {
        QString filePath = item->data(Qt::UserRole).toString();
        if (!filePath.isEmpty()) {
            selectedFiles.append(filePath);
        }
    }
    
    return selectedFiles;
}

QStringList AddSongDialog::getSelectedTags() const
{
    // 获取选中的标签列表
    QStringList selectedTags;
    
    // 获取系统标签列表中选中的项
    QList<QListWidgetItem*> systemSelectedItems = ui->listWidget_system_tags->selectedItems();
    for (QListWidgetItem* item : systemSelectedItems) {
        QString tagName = item->data(Qt::UserRole).toString();
        if (!tagName.isEmpty()) {
            selectedTags.append(tagName);
        }
    }
    
    return selectedTags;
}

void AddSongDialog::setAvailableTags(const QStringList& tags)
{
    // 清空现有标签
    ui->listWidget_system_tags->clear();
    
    // 重新初始化默认标签
    initializeDefaultTags();
    
    // 添加所有标签到system_tags列表
    QStringList systemTags = {"我的歌曲", "我的收藏", "最近播放"};
    for (const QString& tag : tags) {
        if (!systemTags.contains(tag)) { // 避免重复添加系统标签
            // 创建自定义标签项控件
            TagListItem* tagItem = new TagListItem(tag, QString(), true, true);
            
            // 创建QListWidgetItem并设置自定义控件
            QListWidgetItem* item = new QListWidgetItem();
            item->setData(Qt::UserRole, tag);
            item->setSizeHint(tagItem->sizeHint());
            
            ui->listWidget_system_tags->addItem(item);
            ui->listWidget_system_tags->setItemWidget(item, tagItem);
        }
    }
    
    updateButtonStates();
}

void AddSongDialog::addAudioFiles(const QStringList& files)
{
    // 添加音频文件
    m_selectedFiles = files;
    if (m_controller) {
        m_controller->addFiles(files);
    }
}

void AddSongDialog::clearAudioFiles()
{
    // 清除音频文件
    m_selectedFiles.clear();
    if (m_controller) {
        m_controller->clearFiles();
    }
}

void AddSongDialog::setupConnections()
{
    // 设置信号连接
    // 按钮点击事件
    connect(ui->pushButton_add_songs, &QPushButton::clicked, this, &AddSongDialog::onAddFilesClicked);
    connect(ui->pushButton_create_tag, &QPushButton::clicked, this, &AddSongDialog::onCreateTagClicked);
    connect(ui->pushButton_delete_tag, &QPushButton::clicked, this, &AddSongDialog::onDeleteTagClicked);
    connect(ui->pushButton_edit_tag, &QPushButton::clicked, this, &AddSongDialog::onEditTagClicked);
    connect(ui->pushButton_add_to_tag, &QPushButton::clicked, this, &AddSongDialog::onAssignTagClicked);
    connect(ui->pushButton_undo_add, &QPushButton::clicked, this, &AddSongDialog::onUndoAssignClicked);
    connect(ui->pushButton_select_all, &QPushButton::clicked, this, &AddSongDialog::onSelectAllClicked);
    connect(ui->pushButton_deselect_all, &QPushButton::clicked, this, &AddSongDialog::onDeselectAllClicked);
    connect(ui->pushButton_exit_save, &QPushButton::clicked, this, &AddSongDialog::onExitSaveClicked);
    connect(ui->pushButton_exit_discard, &QPushButton::clicked, this, &AddSongDialog::onExitDiscardClicked);
    
    // 列表选择变化事件
    connect(ui->listWidget_added_songs, &QListWidget::itemSelectionChanged, this, &AddSongDialog::onFileListSelectionChanged);
    connect(ui->listWidget_system_tags, &QListWidget::itemSelectionChanged, this, &AddSongDialog::onTagListSelectionChanged);
    
    // 控制器信号连接
    if (m_controller) {
        // 文件操作信号
        connect(m_controller, &AddSongDialogController::filesAdded, this, &AddSongDialog::onFilesAdded);
        
        // 标签操作信号
        connect(m_controller, &AddSongDialogController::tagCreated, this, &AddSongDialog::onTagCreated);
    }
}

void AddSongDialog::setupUI()
{
    // 设置UI初始状态
    setWindowTitle("添加歌曲");
    setModal(true);
    
    // 设置接受拖放
    setAcceptDrops(true);
    ui->listWidget_added_songs->setAcceptDrops(true);
    ui->listWidget_added_songs->setDragDropMode(QAbstractItemView::DragDrop);
    
    // 初始化按钮状态
    updateButtonStates();
    
    // 设置列表控件的工具提示
    ui->listWidget_added_songs->setToolTip("显示已添加的音乐文件列表，可以拖放文件到此处");
    ui->listWidget_system_tags->setToolTip("歌曲标签列表，支持拖拽调整位置");
    
    // 初始化"我的歌曲"标签
    initializeDefaultTags();
}

void AddSongDialog::updateButtonStates()
{
    // 更新按钮状态
    bool hasSongs = ui->listWidget_added_songs->count() > 0;
    bool hasSongSelected = !ui->listWidget_added_songs->selectedItems().isEmpty();
    bool hasTagSelected = !ui->listWidget_system_tags->selectedItems().isEmpty();
    bool canUndoOperation = m_controller && m_controller->canUndo();
    
    // 全选歌曲按钮 - 有歌曲时可用
    ui->pushButton_select_all->setEnabled(hasSongs);
    
    // 取消选中按钮 - 有选中的歌曲时可用
    ui->pushButton_deselect_all->setEnabled(hasSongSelected);
    
    // 添加歌曲按钮 - 始终可用
    ui->pushButton_add_songs->setEnabled(true);
    
    // 创建标签按钮 - 始终可用
    ui->pushButton_create_tag->setEnabled(true);
    
    // 删除标签按钮 - 选中非"我的歌曲"标签时可用
    bool canDeleteTag = false;
    if (hasTagSelected) {
        QList<QListWidgetItem*> selectedItems = ui->listWidget_system_tags->selectedItems();
        for (QListWidgetItem* item : selectedItems) {
            if (item && item->text() != "我的歌曲") {
                canDeleteTag = true;
                break;
            }
        }
    }
    ui->pushButton_delete_tag->setEnabled(canDeleteTag);
    
    // 编辑标签按钮 - 有选中标签时可用
    ui->pushButton_edit_tag->setEnabled(hasTagSelected);
    
    // 加入标签按钮 - 需要同时选中歌曲和标签
    ui->pushButton_add_to_tag->setEnabled(hasSongSelected && hasTagSelected);
    
    // 撤回加入按钮 - 需要有可撤销的操作
    ui->pushButton_undo_add->setEnabled(canUndoOperation);
    
    // 退出按钮 - 始终可用
    ui->pushButton_exit_discard->setEnabled(true);
    ui->pushButton_exit_save->setEnabled(true);
}

void AddSongDialog::showStatusMessage(const QString& message)
{
    // 显示状态消息
    if (!message.isEmpty()) {
        // 这里可以使用状态栏或临时标签显示消息
        // 如果对话框没有状态栏，可以考虑添加一个临时的标签来显示消息
        qDebug() << "AddSongDialog: " << message;
    }
}

// 槽函数实现
void AddSongDialog::onAddFilesClicked()
{
    // 添加文件按钮点击
    if (m_controller) {
        // 打开文件选择对话框
        QStringList filters;
        filters << "音频文件 (*.mp3 *.wav *.flac *.ogg *.aac *.wma *.m4a)";
        filters << "所有文件 (*.*)";
        
        QFileDialog fileDialog(this);
        fileDialog.setWindowTitle("选择音乐文件");
        fileDialog.setFileMode(QFileDialog::ExistingFiles);
        fileDialog.setNameFilters(filters);
        
        // 设置默认目录为项目本地地址
        QString defaultDir = QDir::currentPath();
        fileDialog.setDirectory(defaultDir);
        
        if (fileDialog.exec() == QDialog::Accepted) {
            QStringList filePaths = fileDialog.selectedFiles();
            if (!filePaths.isEmpty()) {
                m_controller->addFiles(filePaths);
                showStatusMessage(QString("已选择 %1 个音频文件").arg(filePaths.size()));
            }
        }
    }
}

void AddSongDialog::onCreateTagClicked()
{
    // 创建标签按钮点击
    if (m_controller) {
        m_controller->onCreateTagRequested();
    }
}

void AddSongDialog::onDeleteTagClicked()
{
    // 删除标签按钮点击
    QStringList selectedTags = getSelectedTags();
    
    if (selectedTags.isEmpty()) {
        showStatusMessage("请先选择要删除的标签");
        return;
    }
    
    // 检查是否包含"我的歌曲"标签
    if (selectedTags.contains("我的歌曲")) {
        showStatusMessage("'我的歌曲'标签不可删除");
        return;
    }
    
    if (m_controller) {
        m_controller->onDeleteTagRequested();
    }
}

void AddSongDialog::onEditTagClicked()
{
    // 编辑标签按钮点击
    QStringList selectedTags = getSelectedTags();
    
    if (selectedTags.isEmpty()) {
        showStatusMessage("请先选择要编辑的标签");
        return;
    }
    
    // 检查是否包含系统标签
    QStringList systemTags = {"我的歌曲", "我的收藏", "最近播放"};
    for (const QString& systemTag : systemTags) {
        if (selectedTags.contains(systemTag)) {
            showStatusMessage(QString("'%1'标签不可编辑").arg(systemTag));
            return;
        }
    }
    
    if (selectedTags.size() > 1) {
        showStatusMessage("一次只能编辑一个标签，请选择单个标签");
        return;
    }
    
    // 调用controller处理编辑标签
    if (m_controller) {
        QString tagName = selectedTags.first();
        m_controller->editTagFromMenu(tagName);
    }
}

void AddSongDialog::onAssignTagClicked()
{
    // 加入标签按钮点击
    QStringList selectedFiles = getSelectedFiles();
    QStringList selectedTags = getSelectedTags();
    
    if (selectedFiles.isEmpty()) {
        showStatusMessage("请先选择要添加标签的歌曲");
        return;
    }
    
    if (selectedTags.isEmpty()) {
        showStatusMessage("请先选择要添加的标签");
        return;
    }
    
    if (m_controller) {
        m_controller->onAssignTagRequested();
        showStatusMessage(QString("正在为 %1 首歌曲添加 %2 个标签...").arg(selectedFiles.size()).arg(selectedTags.size()));
    }
}

void AddSongDialog::onUndoAssignClicked()
{
    // 撤回加入按钮点击
    if (m_controller && m_controller->canUndo()) {
        m_controller->onUndoRequested();
        showStatusMessage("正在撤回上一步操作...");
    } else {
        showStatusMessage("没有可撤回的操作");
    }
}

void AddSongDialog::onExitSaveClicked()
{
    // 退出并保存按钮点击
    if (m_controller) {
        // 确认保存操作
        int fileCount = ui->listWidget_added_songs->count();
        if (fileCount > 0) {
            QMessageBox::StandardButton reply = QMessageBox::question(this, 
                                                                   "确认保存", 
                                                                   QString("确定要保存 %1 首歌曲和相关标签设置吗？").arg(fileCount),
                                                                   QMessageBox::Yes | QMessageBox::No);
            
            if (reply == QMessageBox::Yes) {
                showStatusMessage("正在保存并退出...");
                m_controller->onSaveAndExitRequested();
            }
        } else {
            // 没有歌曲时直接退出
            accept();
        }
    }
}

void AddSongDialog::onExitDiscardClicked()
{
    // 退出不保存按钮点击
    int fileCount = ui->listWidget_added_songs->count();
    
    if (fileCount > 0) {
        // 有未保存的内容时确认
        QMessageBox::StandardButton reply = QMessageBox::question(this, 
                                                               "确认退出", 
                                                               QString("您有 %1 首歌曲未保存，确定要放弃这些更改并退出吗？").arg(fileCount),
                                                               QMessageBox::Yes | QMessageBox::No);
        
        if (reply == QMessageBox::Yes) {
            showStatusMessage("正在退出，不保存更改...");
            if (m_controller) {
                m_controller->onExitWithoutSavingRequested();
            } else {
                reject();
            }
        }
    } else {
        // 没有内容时直接退出
        reject();
    }
}

void AddSongDialog::onFileListSelectionChanged()
{
    // 文件列表选择改变
    updateButtonStates();
}

void AddSongDialog::onTagListSelectionChanged()
{
    // 标签列表选择变化时更新按钮状态
    updateButtonStates();
    
    // 获取当前选中的标签信息
    if (!ui->listWidget_system_tags->selectedItems().isEmpty()) {
        QString selectedTag = ui->listWidget_system_tags->currentItem()->text();
        showStatusMessage(QString("已选择标签: %1").arg(selectedTag));
    }
}

void AddSongDialog::onFilesAdded(const QStringList& filePaths)
{
    // 文件添加后的处理
    if (filePaths.isEmpty()) {
        return;
    }
    
    // 更新文件列表
    ui->listWidget_added_songs->clear();
    
    // 从控制器获取最新的文件列表
    if (m_controller) {
        QList<FileInfo> files = m_controller->getFileInfoList();
        for (const FileInfo& file : files) {
            QListWidgetItem* item = new QListWidgetItem(file.fileName);
            item->setData(Qt::UserRole, file.filePath);
            item->setToolTip(file.filePath);
            ui->listWidget_added_songs->addItem(item);
        }
    }
    
    // 更新按钮状态
    updateButtonStates();
    
    // 显示状态消息
    showStatusMessage(QString("已添加 %1 个文件").arg(filePaths.size()));
}

void AddSongDialog::onTagCreated(const QString& tagName, bool isSystemTag)
{
    Q_UNUSED(isSystemTag)  // 标记参数为未使用，避免编译警告
    
    // 添加新创建的标签到标签列表
    // 创建自定义标签项控件
    TagListItem* tagItem = new TagListItem(tagName, QString(), true, true);
    
    // 创建QListWidgetItem并设置自定义控件
    QListWidgetItem* item = new QListWidgetItem();
    item->setData(Qt::UserRole, tagName);
    item->setSizeHint(tagItem->sizeHint());
    
    ui->listWidget_system_tags->addItem(item);
    ui->listWidget_system_tags->setItemWidget(item, tagItem);
    
    updateButtonStates();
    showStatusMessage(QString("标签 '%1' 创建成功").arg(tagName));
}

void AddSongDialog::onSelectAllClicked()
{
    // 全选所有歌曲
    if (ui->listWidget_added_songs->count() > 0) {
        ui->listWidget_added_songs->selectAll();
        updateButtonStates();
        showStatusMessage(QString("已全选 %1 首歌曲").arg(ui->listWidget_added_songs->count()));
    } else {
        showStatusMessage("没有歌曲可以选择");
    }
}

void AddSongDialog::onDeselectAllClicked()
{
    // 取消选中所有歌曲
    int selectedCount = ui->listWidget_added_songs->selectedItems().count();
    if (selectedCount > 0) {
        ui->listWidget_added_songs->clearSelection();
        updateButtonStates();
        showStatusMessage(QString("已取消选中 %1 首歌曲").arg(selectedCount));
    } else {
        showStatusMessage("没有选中的歌曲");
    }
}

void AddSongDialog::initializeDefaultTags()
{
    // 添加默认的系统标签
    QStringList systemTags = {"我的歌曲", "我的收藏", "最近播放"};
    
    for (const QString& tagName : systemTags) {
        // 创建自定义标签项控件
        TagListItem* tagItem = new TagListItem(tagName, QString(), false, false);
        
        // 创建QListWidgetItem并设置自定义控件
        QListWidgetItem* item = new QListWidgetItem();
        item->setData(Qt::UserRole, tagName);
        item->setSizeHint(tagItem->sizeHint());
        
        ui->listWidget_system_tags->addItem(item);
        ui->listWidget_system_tags->setItemWidget(item, tagItem);
    }
}

AddSongDialogController* AddSongDialog::getController() const
{
    return m_controller;
}