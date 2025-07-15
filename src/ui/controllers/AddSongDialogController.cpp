#include "addsongdialogcontroller.h"
#include "../dialogs/addsongdialog.h"
#include "../dialogs/CreateTagDialog.h"
#include "../widgets/taglistitem.h"
#include "../../managers/tagmanager.h"
#include "../../audio/audioengine.h"
#include "../../database/databasemanager.h"
#include "../../database/tagdao.h"
#include "../../core/logger.h"
#include "../../core/constants.h"

#include <QDebug>
#include <QFile>
#include <QIODevice>
#include <QInputDialog>
#include <QColorDialog>
#include <QMessageBox>
#include <QListWidget>
#include <QListWidgetItem>
#include <QSettings>
#include <QFileDialog>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QSqlQuery>
#include <QSqlError>
#include <QTimer>
#include <QApplication>
#include <QThread>

const QStringList AddSongDialogController::SUPPORTED_FORMATS = {
    "mp3", "wav", "flac", "ogg", "aac", "wma", "m4a", "mp4", "opus", "ape", "aiff"
};

AddSongDialogController::AddSongDialogController(AddSongDialog* dialog, QObject* parent)
    : QObject(parent)
    , m_dialog(dialog)
    , m_tagManager(nullptr)
    , m_audioEngine(nullptr)
    , m_databaseManager(nullptr)
    , m_logger(nullptr)
    , m_initialized(false)
    , m_processing(false)
    , m_processedCount(0)
    , m_failedCount(0)
    , m_totalCount(0)
    , m_autoAssignToDefault(true)
    , m_duplicateHandling(0)
    , m_processingTimer(nullptr)
    , m_progressTimer(nullptr)
    , m_settings(nullptr)
{
    // 初始化设置
    m_settings = new QSettings(this);
    
    // 初始化定时器
    m_processingTimer = new QTimer(this);
    m_progressTimer = new QTimer(this);
    
    // 连接定时器信号
    connect(m_processingTimer, &QTimer::timeout, this, &AddSongDialogController::onFileProcessingTimer);
    connect(m_progressTimer, &QTimer::timeout, this, &AddSongDialogController::onProgressUpdateTimer);
    
    // 加载设置
    loadSettings();
}

AddSongDialogController::~AddSongDialogController()
{
    shutdown();
}

bool AddSongDialogController::initialize()
{
    logInfo("Initializing AddSongDialogController");
    
    if (m_initialized) {
        logInfo("Already initialized");
        return true;
    }
    
    try {
        // 检查必要的组件
        if (!m_dialog) {
            logError("Dialog is null, cannot initialize");
            return false;
        }
        
        // 获取数据库管理器实例
        m_databaseManager = DatabaseManager::instance();
        if (!m_databaseManager) {
            logError("Failed to get DatabaseManager instance");
            return false;
        }
        
        // 检查数据库连接状态
        if (!m_databaseManager->isValid()) {
            logError("Database is not connected");
            return false;
        }
        
        // 设置连接
        setupConnections();
        
        // 加载设置
        loadSettings();
        
        // 初始化数据
        m_fileInfoList.clear();
        m_recentOperations.clear();
        
        // 加载标签
        loadAvailableTags();
        
        // 设置定时器
        if (m_processingTimer) {
            m_processingTimer->setSingleShot(false);
            m_processingTimer->setInterval(100); // 100ms间隔
        }
        
        if (m_progressTimer) {
            m_progressTimer->setSingleShot(false);
            m_progressTimer->setInterval(500); // 500ms间隔
        }
        
        // 初始化UI状态
        updateFileList();
        updateTagList();
        updateButtonStates();
        
        m_initialized = true;
        logInfo("AddSongDialogController initialized successfully");
        
        emit operationCompleted("控制器初始化完成", true);
        
        return true;
    } catch (const std::exception& e) {
        logError(QString("AddSongDialogController initialization failed: %1").arg(e.what()));
        return false;
    }
}

void AddSongDialogController::shutdown()
{
    if (!m_initialized) {
        return;
    }
    
    // 保存设置
    saveSettings();
    
    // 停止定时器
    if (m_processingTimer) {
        m_processingTimer->stop();
    }
    if (m_progressTimer) {
        m_progressTimer->stop();
    }
    
    m_initialized = false;
    logInfo("AddSongDialogController shut down");
}

bool AddSongDialogController::isInitialized() const
{
    return m_initialized;
}

bool AddSongDialogController::addFiles(const QStringList& filePaths)
{
    logInfo(QString("Adding %1 files").arg(filePaths.size()));
    
    if (filePaths.isEmpty()) {
        emit warningOccurred("没有选择文件");
        return false;
    }
    
    emit operationStarted(QString("正在添加 %1 个文件...").arg(filePaths.size()));
    
    QStringList validFiles;
    QStringList invalidFiles;
    QStringList duplicateFiles;
    
    for (const QString& filePath : filePaths) {
        if (!isAudioFile(filePath)) {
            invalidFiles.append(filePath);
            continue;
        }
        
        if (!validateFileInternal(filePath)) {
            invalidFiles.append(filePath);
            continue;
        }
        
        // 检查是否已存在
        bool exists = false;
        for (const FileInfo& existing : m_fileInfoList) {
            if (existing.filePath == filePath) {
                exists = true;
                duplicateFiles.append(filePath);
                break;
            }
        }
        
        if (!exists) {
            FileInfo fileInfo = extractFileInfo(filePath);
            m_fileInfoList.append(fileInfo);
            validFiles.append(filePath);
        }
    }
    
    // 更新文件列表显示
    updateFileList();
    
    // 发送信号通知UI
    if (!validFiles.isEmpty()) {
        emit filesAdded(validFiles);
    }
    
    // 构建结果消息
    QStringList messages;
    if (!validFiles.isEmpty()) {
        messages << QString("成功添加 %1 个文件").arg(validFiles.size());
    }
    if (!duplicateFiles.isEmpty()) {
        messages << QString("跳过 %1 个重复文件").arg(duplicateFiles.size());
    }
    if (!invalidFiles.isEmpty()) {
        messages << QString("跳过 %1 个无效文件").arg(invalidFiles.size());
    }
    
    QString resultMessage = messages.join(", ");
    if (!resultMessage.isEmpty()) {
        emit operationCompleted(resultMessage, !validFiles.isEmpty());
    }
    
    // 自动为新添加的文件分配"我的歌曲"标签
    if (!validFiles.isEmpty() && m_autoAssignToDefault) {
        for (const QString& filePath : validFiles) {
            assignTag(filePath, "我的歌曲");
        }
    }
    
    return !validFiles.isEmpty();
}

void AddSongDialogController::removeFiles(const QStringList& filePaths)
{
    logInfo(QString("Removing %1 files").arg(filePaths.size()));
    
    for (const QString& filePath : filePaths) {
        m_fileInfoList.removeIf([filePath](const FileInfo& info) {
            return info.filePath == filePath;
        });
    }
    
    emit filesRemoved(filePaths);
    updateFileList();
}

void AddSongDialogController::clearFiles()
{
    logInfo("Clearing all files");
    
    m_fileInfoList.clear();
    emit filesCleared();
    updateFileList();
}

void AddSongDialogController::loadAvailableTags()
{
    logInfo("Loading available tags");
    
    emit operationStarted("正在加载标签列表...");
    
    // 从数据库加载标签
    loadTagsFromDatabase();
    
    // 更新标签列表显示
    updateTagList();
    
    emit operationCompleted(QString("已加载 %1 个标签").arg(m_tagInfoList.size()), true);
    
    // 通知对话框更新标签列表
    if (m_dialog) {
        QStringList tagNames;
        for (const TagInfo& tag : m_tagInfoList) {
            tagNames << tag.name;
        }
        // m_dialog->setAvailableTags(tagNames); // 如果对话框有此方法
    }
}

void AddSongDialogController::createTag(const QString& name, const QString& color, const QString& iconPath)
{
    logInfo(QString("Creating tag: %1").arg(name));
    
    // 检查标签名称是否有效
    if (name.isEmpty()) {
        emit warningOccurred("标签名称不能为空");
        return;
    }
    
    // 检查标签是否已存在
    bool tagExists = false;
    for (const TagInfo& tag : m_tagInfoList) {
        if (tag.name == name) {
            tagExists = true;
            break;
        }
    }
    
    if (tagExists) {
        emit warningOccurred(QString("标签 '%1' 已存在").arg(name));
        return;
    }
    
    TagInfo tagInfo;
    tagInfo.name = name;
    tagInfo.displayName = name;
    tagInfo.color = color.isEmpty() ? "#0078d4" : color;
    tagInfo.iconPath = iconPath;
    tagInfo.isDefault = false;
    tagInfo.isEditable = true;
    
    m_tagInfoList.append(tagInfo);
    
    emit tagCreated(name, true); // 修改为发送正确的参数
    emit tagListChanged(); // 通知主界面刷新标签列表
    emit operationCompleted(QString("已创建标签: %1").arg(name), true);
    updateTagList();
    
    // 更新按钮状态
    updateButtonStates();
}

// 槽函数实现
void AddSongDialogController::onAddFilesRequested()
{
    logInfo("Add files requested");
    
    if (!m_dialog) {
        emit errorOccurred("对话框不存在");
        return;
    }
    
    QStringList filters;
    filters << "音频文件 (*.mp3 *.wav *.flac *.ogg *.aac *.wma *.m4a)"
            << "所有文件 (*.*)";
    
    QFileDialog fileDialog(m_dialog);
    fileDialog.setWindowTitle("选择音乐文件");
    fileDialog.setFileMode(QFileDialog::ExistingFiles);
    fileDialog.setNameFilters(filters);
    
    // 设置默认目录
    QString defaultDir = QDir::currentPath();
    if (!m_lastDirectory.isEmpty()) {
        defaultDir = m_lastDirectory;
    }
    fileDialog.setDirectory(defaultDir);
    
    if (fileDialog.exec() == QDialog::Accepted) {
        QStringList filePaths = fileDialog.selectedFiles();
        if (!filePaths.isEmpty()) {
            addFiles(filePaths);
            m_lastDirectory = QFileInfo(filePaths.first()).absolutePath();
        }
    }
}

void AddSongDialogController::onFileProcessingTimer()
{
    // 处理文件的定时器
    logDebug("File processing timer triggered");
}

void AddSongDialogController::onProgressUpdateTimer()
{
    // 更新进度的定时器
    logDebug("Progress update timer triggered");
}

void AddSongDialogController::onFileAnalysisCompleted()
{
    // 文件分析完成
    logInfo("File analysis completed");
}

// 私有方法实现
void AddSongDialogController::setupConnections()
{
    // 设置信号连接
    if (m_dialog) {
        // 这里应该连接对话框的信号
        // 目前为空
    }
}

void AddSongDialogController::loadSettings()
{
    if (m_settings) {
        m_autoAssignToDefault = m_settings->value("AutoAssignToDefault", true).toBool();
        m_duplicateHandling = m_settings->value("DuplicateHandling", 0).toInt();
    }
}

void AddSongDialogController::saveSettings()
{
    if (m_settings) {
        m_settings->setValue("AutoAssignToDefault", m_autoAssignToDefault);
        m_settings->setValue("DuplicateHandling", m_duplicateHandling);
        
        // 保存最后使用的目录
        if (!m_lastDirectory.isEmpty()) {
            m_settings->setValue("LastDirectory", m_lastDirectory);
        }
        
        // 保存窗口大小和位置
        if (m_dialog) {
            m_settings->beginGroup("AddSongDialog");
            m_settings->setValue("Size", m_dialog->size());
            m_settings->setValue("Position", m_dialog->pos());
            m_settings->endGroup();
        }
        
        m_settings->sync();
    }
}

FileInfo AddSongDialogController::extractFileInfo(const QString& filePath)
{
    FileInfo info;
    info.filePath = filePath;
    info.fileName = QFileInfo(filePath).fileName();
    info.displayName = QFileInfo(filePath).baseName();
    info.fileSize = QFileInfo(filePath).size();
    info.format = getFileFormat(filePath);
    info.isValid = validateFileInternal(filePath);
    info.status = FileStatus::Pending;
    
    return info;
}

bool AddSongDialogController::isAudioFile(const QString& filePath) const
{
    QString extension = QFileInfo(filePath).suffix().toLower();
    return SUPPORTED_FORMATS.contains(extension);
}

bool AddSongDialogController::validateFileInternal(const QString& filePath) const
{
    // 检查文件是否存在
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        logError(QString("File does not exist or is not a file: %1").arg(filePath));
        return false;
    }
    
    // 检查文件大小
    if (fileInfo.size() <= 0) {
        logError(QString("File is empty: %1").arg(filePath));
        return false;
    }
    
    // 检查文件格式
    if (!isAudioFile(filePath)) {
        logError(QString("File is not a supported audio format: %1").arg(filePath));
        return false;
    }
    
    // 检查文件是否可读
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        logError(QString("Cannot read file: %1").arg(filePath));
        return false;
    }
    file.close();
    
    return true;
}

QString AddSongDialogController::getFileFormat(const QString& filePath) const
{
    return QFileInfo(filePath).suffix().toUpper();
}

void AddSongDialogController::updateFileList()
{
    // 更新文件列表显示
    logDebug("Updating file list");
    
    if (!m_dialog) {
        return;
    }
    
    // 获取文件列表控件
    QListWidget* fileListWidget = m_dialog->findChild<QListWidget*>("listWidget_added_songs");
    if (!fileListWidget) {
        logError("找不到文件列表控件");
        return;
    }
    
    // 清空现有列表
    fileListWidget->clear();
    
    // 添加所有文件到列表
    for (const FileInfo& fileInfo : m_fileInfoList) {
        QListWidgetItem* item = new QListWidgetItem(fileInfo.fileName);
        item->setData(Qt::UserRole, fileInfo.filePath);
        item->setToolTip(QString("%1\n大小: %2 KB\n格式: %3")
                        .arg(fileInfo.filePath)
                        .arg(fileInfo.fileSize / 1024)
                        .arg(fileInfo.format));
        
        // 根据文件状态设置图标或颜色
        switch (fileInfo.status) {
            case FileStatus::Pending:
                item->setForeground(QColor(Qt::blue));
                break;
            case FileStatus::Processing:
                item->setForeground(QColor(Qt::yellow));
                break;
            case FileStatus::Completed:
                item->setForeground(QColor(Qt::green));
                break;
            case FileStatus::Failed:
                item->setForeground(QColor(Qt::red));
                item->setToolTip(item->toolTip() + "\n错误: " + fileInfo.errorMessage);
                break;
            case FileStatus::Skipped:
                item->setForeground(QColor(Qt::gray));
                break;
        }
        
        fileListWidget->addItem(item);
    }
    
    // 更新按钮状态
    updateButtonStates();
}

void AddSongDialogController::updateTagList()
{
    // 更新标签列表显示
    logDebug("Updating tag list");
    
    if (!m_dialog) {
        logError("Dialog is null in updateTagList");
        return;
    }
    
    // 获取标签列表控件
    QListWidget* tagListWidget = m_dialog->findChild<QListWidget*>("listWidget_system_tags");
    if (!tagListWidget) {
        logError("找不到标签列表控件");
        return;
    }
    
    // 检查标签列表是否为空
    if (m_tagInfoList.isEmpty()) {
        logWarning("Tag list is empty, skipping update");
        return;
    }
    
    // 保存当前选中的标签
    QStringList selectedTags;
    for (int i = 0; i < tagListWidget->count(); ++i) {
        QListWidgetItem* item = tagListWidget->item(i);
        if (item && item->isSelected()) {
            selectedTags << item->data(Qt::UserRole).toString();
        }
    }
    
    // 清空现有列表
    tagListWidget->clear();
    
    // 添加所有标签到列表
    for (const TagInfo& tagInfo : m_tagInfoList) {
        // 使用Constants中定义的系统标签判断
        bool isSystemTag = Constants::SystemTags::isSystemTag(tagInfo.name);
        
        // 创建TagListItem自定义控件
        TagListItem* tagWidget = new TagListItem(tagInfo.name, tagInfo.iconPath, !isSystemTag, !isSystemTag);
        
        // 连接编辑信号
        connect(tagWidget, &TagListItem::editRequested, this, [this, tagInfo](const QString& tagName) {
            // 直接编辑指定标签，无需模拟选中
            QStringList systemTags = {"我的歌曲", "我的收藏", "最近播放"};
            if (systemTags.contains(tagName)) {
                QMessageBox::information(m_dialog, tr("提示"), tr("系统标签不能编辑"));
                return;
            }
            
            // 创建编辑对话框
            CreateTagDialog createDialog(m_dialog);
            createDialog.setWindowTitle(tr("编辑标签"));
            createDialog.setTagName(tagInfo.name);
            createDialog.setImagePath(tagInfo.iconPath);
            
            if (createDialog.exec() == QDialog::Accepted) {
                QString newTagName = createDialog.getTagName().trimmed();
                QString newIconPath = createDialog.getTagImagePath();
                
                if (newTagName.isEmpty()) {
                    QMessageBox::warning(m_dialog, tr("错误"), tr("标签名称不能为空"));
                    return;
                }
                
                // 检查标签名称是否重复（排除自身）
                if (newTagName != tagInfo.name) {
                    for (const TagInfo& existingTag : m_tagInfoList) {
                        if (existingTag.name == newTagName) {
                            QMessageBox::warning(m_dialog, tr("错误"), tr("标签名称已存在"));
                            return;
                        }
                    }
                }
                
                // 执行编辑操作
                editTag(tagInfo.name, newTagName, tagInfo.color, newIconPath);
            }
        });
        
        // 创建QListWidgetItem并设置自定义控件
        QListWidgetItem* item = new QListWidgetItem();
        item->setData(Qt::UserRole, tagInfo.name);
        item->setSizeHint(tagWidget->sizeHint());
        
        tagListWidget->addItem(item);
        tagListWidget->setItemWidget(item, tagWidget);
        
        // 恢复选中状态
        if (selectedTags.contains(tagInfo.name)) {
            item->setSelected(true);
        }
    }
    
    // 更新按钮状态
    updateButtonStates();
}

void AddSongDialogController::handleError(const QString& error)
{
    logError(error);
    emit errorOccurred(error);
}

void AddSongDialogController::logInfo(const QString& message) const
{
    qDebug() << "[AddSongDialogController][INFO]" << message;
    
    // 可以在这里添加更复杂的日志记录逻辑
    // 例如写入日志文件、发送到日志服务等
}

void AddSongDialogController::logError(const QString& error) const
{
    qCritical() << "AddSongDialogController Error:" << error;
}

void AddSongDialogController::logDebug(const QString& message) const
{
    qDebug() << "AddSongDialogController Debug:" << message;
}

QString AddSongDialogController::formatFileSize(qint64 bytes) const
{
    if (bytes < 1024) {
        return QString("%1 B").arg(bytes);
    } else if (bytes < 1024 * 1024) {
        return QString("%1 KB").arg(bytes / 1024.0, 0, 'f', 1);
    } else if (bytes < 1024 * 1024 * 1024) {
        return QString("%1 MB").arg(bytes / (1024.0 * 1024.0), 0, 'f', 1);
    } else {
        return QString("%1 GB").arg(bytes / (1024.0 * 1024.0 * 1024.0), 0, 'f', 1);
    }
}

QString AddSongDialogController::formatDuration(qint64 duration) const
{
    if (duration < 0) {
        return "--:--";
    }
    
    int seconds = static_cast<int>(duration);
    int hours = seconds / 3600;
    int minutes = (seconds % 3600) / 60;
    int secs = seconds % 60;
    
    if (hours > 0) {
        return QString("%1:%2:%3")
            .arg(hours)
            .arg(minutes, 2, 10, QChar('0'))
            .arg(secs, 2, 10, QChar('0'));
    } else {
        return QString("%1:%2")
            .arg(minutes)
            .arg(secs, 2, 10, QChar('0'));
    }
}

bool AddSongDialogController::isValidAudioFormat(const QString& format) const
{
    return SUPPORTED_FORMATS.contains(format.toLower());
}

QString AddSongDialogController::getFileExtension(const QString& filePath) const
{
    return QFileInfo(filePath).suffix().toLower();
}

bool AddSongDialogController::hasValidExtension(const QString& filePath) const
{
    QString extension = getFileExtension(filePath);
    return isValidAudioFormat(extension);
}



bool AddSongDialogController::isDuplicateFile(const QString& filePath) const
{
    for (const auto& fileInfo : m_fileInfoList) {
        if (fileInfo.filePath == filePath) {
            return true;
        }
    }
    return false;
}

QString AddSongDialogController::generateUniqueFileName(const QString& filePath) const
{
    QFileInfo fileInfo(filePath);
    QString baseName = fileInfo.completeBaseName();
    QString extension = fileInfo.suffix();
    QString dir = fileInfo.absolutePath();
    
    int counter = 1;
    QString newFilePath;
    
    do {
        newFilePath = QString("%1/%2_%3.%4")
            .arg(dir)
            .arg(baseName)
            .arg(counter)
            .arg(extension);
        counter++;
    } while (QFile::exists(newFilePath) && counter < 1000);
    
    return newFilePath;
}



// 缺失的槽函数实现
void AddSongDialogController::onRemoveFilesRequested()
{
    logInfo("Remove files requested");
    
    QStringList selectedFiles = getSelectedFiles();
    if (selectedFiles.isEmpty()) {
        emit warningOccurred("请先选择要移除的文件");
        return;
    }
    
    removeFiles(selectedFiles);
}

void AddSongDialogController::onClearFilesRequested()
{
    logInfo("Clear files requested");
    clearFiles();
}

void AddSongDialogController::onFileSelectionChanged()
{
    updateButtonStates();
}

void AddSongDialogController::onTagSelectionChanged()
{
    updateButtonStates();
}

void AddSongDialogController::onCreateTagRequested()
{
    logInfo("Create tag requested");
    
    CreateTagDialog dialog(m_dialog);
    if (dialog.exec() == QDialog::Accepted) {
        QString tagName = dialog.getTagName();
        QString imagePath = dialog.getTagImagePath();
        
        if (tagName.isEmpty()) {
            emit warningOccurred("标签名称不能为空");
            return;
        }
        
        // 检查标签是否已存在
        for (const auto& tagInfo : m_tagInfoList) {
            if (tagInfo.name == tagName) {
                emit warningOccurred(QString("标签 '%1' 已存在").arg(tagName));
                return;
            }
        }
        
        // 使用默认颜色，图片路径从对话框获取
        createTag(tagName, "#3498db", imagePath);
    }
}

void AddSongDialogController::onDeleteTagRequested()
{
    logInfo("Delete tag requested");
    
    QStringList selectedTags = m_dialog->getSelectedTags();
    if (selectedTags.isEmpty()) {
        emit warningOccurred("请先选择要删除的标签");
        return;
    }
    
    // 检查是否包含系统标签
    QStringList systemTags = {"我的歌曲", "默认标签", "收藏", "最近播放"};
    QStringList validTags;
    for (const QString& tagName : selectedTags) {
        if (!systemTags.contains(tagName)) {
            validTags.append(tagName);
        }
    }
    
    if (validTags.isEmpty()) {
        emit warningOccurred("系统标签不能删除");
        return;
    }
    
    // 检查标签下是否有歌曲
    int totalSongCount = 0;
    QStringList tagsWithSongs;
    for (const QString& tagName : validTags) {
        for (const auto& tagInfo : m_tagInfoList) {
            if (tagInfo.name == tagName && tagInfo.songCount > 0) {
                totalSongCount += tagInfo.songCount;
                tagsWithSongs.append(QString("%1(%2首)").arg(tagName).arg(tagInfo.songCount));
                break;
            }
        }
    }
    
    QString confirmMessage;
    if (validTags.size() == 1) {
        confirmMessage = QString("确定要删除标签 '%1' 吗？").arg(validTags.first());
        if (totalSongCount > 0) {
            confirmMessage = QString("标签 '%1' 下有 %2 首歌曲，删除标签将同时移除这些歌曲的标签关联。\n\n确定要删除吗？")
                .arg(validTags.first()).arg(totalSongCount);
        }
    } else {
        confirmMessage = QString("确定要删除选中的 %1 个标签吗？").arg(validTags.size());
        if (!tagsWithSongs.isEmpty()) {
            confirmMessage = QString("以下标签下有歌曲：\n%1\n\n删除这些标签将同时移除相关歌曲的标签关联。确定要删除吗？")
                .arg(tagsWithSongs.join("\n"));
        }
    }
    
    int ret = QMessageBox::question(m_dialog, "确认删除", confirmMessage,
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    
    if (ret != QMessageBox::Yes) {
        return;
    }
    
    // 批量删除标签
    for (const QString& tagName : validTags) {
        deleteTag(tagName);
    }
}

void AddSongDialogController::editTagFromMenu(const QString& tagName)
{
    logInfo(QString("Edit tag from menu requested: %1").arg(tagName));
    
    // 检查是否是系统标签
    QStringList systemTags = {"我的歌曲", "默认标签", "收藏", "最近播放"};
    if (systemTags.contains(tagName)) {
        emit warningOccurred(QString("系统标签'%1'不可编辑").arg(tagName));
        return;
    }
    
    // 获取标签信息
    TagInfo tagInfo;
    bool found = false;
    for (const auto& info : m_tagInfoList) {
        if (info.name == tagName) {
            tagInfo = info;
            found = true;
            break;
        }
    }
    
    if (!found) {
        emit warningOccurred(QString("标签 '%1' 不存在").arg(tagName));
        return;
    }
    
    // 创建编辑对话框
    CreateTagDialog dialog(m_dialog);
    dialog.setWindowTitle("编辑标签");
    dialog.setTagName(tagInfo.name);
    dialog.setImagePath(tagInfo.iconPath);
    
    if (dialog.exec() == QDialog::Accepted) {
        QString newName = dialog.getTagName().trimmed();
        QString newImagePath = dialog.getTagImagePath();
        
        // 验证新标签名
        if (newName.isEmpty()) {
            emit warningOccurred("标签名称不能为空");
            return;
        }
        
        // 检查名称是否重复（除了当前标签）
        if (newName != tagName) {
            for (const auto& info : m_tagInfoList) {
                if (info.name == newName) {
                    emit warningOccurred(QString("标签名称 '%1' 已存在").arg(newName));
                    return;
                }
            }
        }
        
        // 调用编辑标签方法
        editTag(tagName, newName, tagInfo.color, newImagePath);
        emit tagListChanged(); // 通知主界面刷新标签列表
    }
}

void AddSongDialogController::onAssignTagRequested()
{
    logInfo("Assign tag requested");
    
    QStringList selectedFiles = getSelectedFiles();
    QStringList selectedTags = m_dialog->getSelectedTags();
    
    if (selectedFiles.isEmpty()) {
        emit warningOccurred("请先选择要添加标签的歌曲");
        return;
    }
    
    if (selectedTags.isEmpty()) {
        emit warningOccurred("请先选择要添加的标签");
        return;
    }
    
    if (selectedTags.size() > 1) {
        emit warningOccurred("一次只能添加一个标签");
        return;
    }
    
    QString tagName = selectedTags.first();
    assignTagToFiles(tagName, selectedFiles);
}

void AddSongDialogController::onUnassignTagRequested()
{
    logInfo("Unassign tag requested");
    
    QStringList selectedFiles = getSelectedFiles();
    QStringList selectedTags = m_dialog->getSelectedTags();
    
    if (selectedFiles.isEmpty()) {
        emit warningOccurred("请先选择要移除标签的歌曲");
        return;
    }
    
    if (selectedTags.isEmpty()) {
        emit warningOccurred("请先选择要移除的标签");
        return;
    }
    
    if (selectedTags.size() > 1) {
        emit warningOccurred("一次只能移除一个标签");
        return;
    }
    
    QString tagName = selectedTags.first();
    unassignTagFromFiles(tagName, selectedFiles);
}

void AddSongDialogController::onAcceptRequested()
{
    logInfo("Accept requested");
    
    // 处理接受操作
    processFiles();
    
    // 发出对话框接受信号
    emit dialogAccepted();
    
    // 关闭对话框窗口
    if (m_dialog) {
        m_dialog->accept();
    }
}

void AddSongDialogController::onRejectRequested()
{
    logInfo("Reject requested");
    
    // 发出对话框拒绝信号
    emit dialogRejected();
    
    // 关闭对话框窗口
    if (m_dialog) {
        m_dialog->reject();
    }
}

void AddSongDialogController::onExitWithoutSavingRequested()
{
    logInfo("Exit without saving requested");
    
    // 检查是否有未保存的更改
    if (!m_fileInfoList.isEmpty()) {
        int ret = QMessageBox::question(m_dialog, "确认退出", 
            "有未保存的更改，确定要退出而不保存吗？",
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        
        if (ret != QMessageBox::Yes) {
            return;
        }
    }
    
    emit dialogRejected();
    
    // 关闭对话框窗口
    if (m_dialog) {
        m_dialog->reject();
    }
}

void AddSongDialogController::onUndoRequested()
{
    logInfo("Undo requested");
    undoOperation();
}

void AddSongDialogController::onFilesDropped(const QStringList& filePaths)
{
    logInfo(QString("文件拖放: %1个文件").arg(filePaths.size()));
    
    QStringList validFiles;
    for (const QString& filePath : filePaths) {
        if (isAudioFile(filePath)) {
            validFiles << filePath;
        }
    }
    
    if (!validFiles.isEmpty()) {
        addFiles(validFiles);
    } else {
        handleError("没有有效的音频文件");
    }
}

// 批量操作方法实现
void AddSongDialogController::assignTagToFiles(const QString& tagName, const QStringList& files)
{
    logInfo(QString("Assigning tag '%1' to %2 files").arg(tagName).arg(files.size()));
    
    if (files.isEmpty()) {
        emit warningOccurred("没有选择文件");
        return;
    }
    
    emit operationStarted(QString("正在为 %1 个文件添加标签 '%2'...").arg(files.size()).arg(tagName));
    
    int successCount = 0;
    for (const QString& filePath : files) {
        // 检查文件是否存在于列表中
        bool fileFound = false;
        for (auto& fileInfo : m_fileInfoList) {
            if (fileInfo.filePath == filePath) {
                fileFound = true;
                
                // 检查是否已经分配了该标签
                QStringList currentTags = fileInfo.tagAssignment.split(",", Qt::SkipEmptyParts);
                if (!currentTags.contains(tagName)) {
                    currentTags.append(tagName);
                    fileInfo.tagAssignment = currentTags.join(",");
                    
                    // 记录操作用于撤销
                    Operation op;
                    op.type = "assign";
                    op.filePath = filePath;
                    op.tagName = tagName;
                    m_recentOperations.append(op);
                    
                    successCount++;
                }
                break;
            }
        }
        
        if (!fileFound) {
            logError(QString("File not found in list: %1").arg(filePath));
        }
    }
    
    // 更新标签的歌曲数量
    for (auto& tagInfo : m_tagInfoList) {
        if (tagInfo.name == tagName) {
            tagInfo.songCount += successCount;
            break;
        }
    }
    
    // 更新UI
    updateFileList();
    updateTagList();
    updateButtonStates();
    
    emit tagAssigned(tagName, files);
    emit operationCompleted(QString("成功为 %1 个文件添加了标签 '%2'").arg(successCount).arg(tagName), successCount > 0);
}

void AddSongDialogController::unassignTagFromFiles(const QString& tagName, const QStringList& files)
{
    logInfo(QString("Unassigning tag '%1' from %2 files").arg(tagName).arg(files.size()));
    
    if (files.isEmpty()) {
        emit warningOccurred("没有选择文件");
        return;
    }
    
    emit operationStarted(QString("正在从 %1 个文件移除标签 '%2'...").arg(files.size()).arg(tagName));
    
    int successCount = 0;
    for (const QString& filePath : files) {
        // 检查文件是否存在于列表中
        bool fileFound = false;
        for (auto& fileInfo : m_fileInfoList) {
            if (fileInfo.filePath == filePath) {
                fileFound = true;
                
                // 检查是否已经分配了该标签
                QStringList currentTags = fileInfo.tagAssignment.split(",", Qt::SkipEmptyParts);
                if (currentTags.contains(tagName)) {
                    currentTags.removeAll(tagName);
                    fileInfo.tagAssignment = currentTags.join(",");
                    
                    // 记录操作用于撤销
                    Operation op;
                    op.type = "unassign";
                    op.filePath = filePath;
                    op.tagName = tagName;
                    m_recentOperations.append(op);
                    
                    successCount++;
                }
                break;
            }
        }
        
        if (!fileFound) {
            logError(QString("File not found in list: %1").arg(filePath));
        }
    }
    
    // 更新标签的歌曲数量
    for (auto& tagInfo : m_tagInfoList) {
        if (tagInfo.name == tagName) {
            tagInfo.songCount = qMax(0, tagInfo.songCount - successCount);
            break;
        }
    }
    
    // 更新UI
    updateFileList();
    updateTagList();
    updateButtonStates();
    
    emit tagUnassigned(tagName, files);
    emit operationCompleted(QString("成功从 %1 个文件移除了标签 '%2'").arg(successCount).arg(tagName), successCount > 0);
}

void AddSongDialogController::onSaveAndExitRequested()
{
    logInfo("Save and exit requested");
    
    // 处理文件
    processFiles();
    
    // 保存设置
    saveSettings();
    
    emit dialogAccepted();
    
    // 关闭对话框窗口
    if (m_dialog) {
        m_dialog->accept();
    }
}
void AddSongDialogController::assignTag(const QString& filePath, const QString& tagName)
{
    logInfo(QString("Assigning tag '%1' to file: %2").arg(tagName, filePath));
    
    // 检查文件是否存在于列表中
    bool fileFound = false;
    for (auto& fileInfo : m_fileInfoList) {
        if (fileInfo.filePath == filePath) {
            fileFound = true;
            
            // 检查是否已经分配了该标签
            QStringList currentTags = fileInfo.tagAssignment.split(",", Qt::SkipEmptyParts);
            if (!currentTags.contains(tagName)) {
                currentTags.append(tagName);
                fileInfo.tagAssignment = currentTags.join(",");
                
                // 记录操作用于撤销
                Operation op;
                op.type = "assign";
                op.filePath = filePath;
                op.tagName = tagName;
                m_recentOperations.append(op);
                
                // 更新标签的歌曲数量
                for (auto& tagInfo : m_tagInfoList) {
                    if (tagInfo.name == tagName) {
                        tagInfo.songCount++;
                        break;
                    }
                }
                
                logInfo(QString("Tag '%1' assigned to file: %2").arg(tagName, filePath));
                emit tagAssigned(tagName, QStringList() << filePath);
            } else {
                logInfo(QString("File already has tag '%1': %2").arg(tagName, filePath));
            }
            break;
        }
    }
    
    if (!fileFound) {
        logError(QString("File not found in list: %1").arg(filePath));
    }
    
    // 更新UI
    updateFileList();
    updateTagList();
    updateButtonStates();
}
void AddSongDialogController::unassignTag(const QString& filePath, const QString& tagName)
{
    logInfo(QString("Unassigning tag '%1' from file: %2").arg(tagName, filePath));
    
    // 检查文件是否存在于列表中
    bool fileFound = false;
    for (auto& fileInfo : m_fileInfoList) {
        if (fileInfo.filePath == filePath) {
            fileFound = true;
            
            // 检查是否已经分配了该标签
            QStringList currentTags = fileInfo.tagAssignment.split(",", Qt::SkipEmptyParts);
            if (currentTags.contains(tagName)) {
                currentTags.removeAll(tagName);
                fileInfo.tagAssignment = currentTags.join(",");
                
                // 记录操作用于撤销
                Operation op;
                op.type = "unassign";
                op.filePath = filePath;
                op.tagName = tagName;
                m_recentOperations.append(op);
                
                // 更新标签的歌曲数量
                for (auto& tagInfo : m_tagInfoList) {
                    if (tagInfo.name == tagName) {
                        tagInfo.songCount = qMax(0, tagInfo.songCount - 1);
                        break;
                    }
                }
                
                logInfo(QString("Tag '%1' unassigned from file: %2").arg(tagName, filePath));
                emit tagUnassigned(tagName, QStringList() << filePath);
            } else {
                logInfo(QString("File does not have tag '%1': %2").arg(tagName, filePath));
            }
            break;
        }
    }
    
    if (!fileFound) {
        logError(QString("File not found in list: %1").arg(filePath));
    }
    
    // 更新UI
    updateFileList();
    updateTagList();
    updateButtonStates();
}
void AddSongDialogController::processFiles() {
    logInfo("Processing files");
    
    if (m_fileInfoList.isEmpty()) {
        emit warningOccurred("没有文件需要处理");
        return;
    }
    
    // 开始处理文件
    emit operationStarted(QString("正在处理 %1 个文件...").arg(m_fileInfoList.size()));
    
    int totalFiles = m_fileInfoList.size();
    int processedFiles = 0;
    int successFiles = 0;
    int failedFiles = 0;
    
    for (FileInfo& fileInfo : m_fileInfoList) {
        if (fileInfo.status == FileStatus::Pending) {
            // 更新状态为处理中
            fileInfo.status = FileStatus::Processing;
            
            try {
                // 验证文件
                if (!validateFileInternal(fileInfo.filePath)) {
                    fileInfo.status = FileStatus::Failed;
                    fileInfo.errorMessage = "文件验证失败";
                    failedFiles++;
                    logError(QString("File validation failed: %1").arg(fileInfo.filePath));
                } else {
                    // 处理成功
                    fileInfo.status = FileStatus::Completed;
                    successFiles++;
                    logInfo(QString("File processed successfully: %1").arg(fileInfo.filePath));
                }
                
                emit fileProcessed(fileInfo.filePath, fileInfo.status == FileStatus::Completed);
            } catch (const std::exception& e) {
                fileInfo.status = FileStatus::Failed;
                fileInfo.errorMessage = QString("处理异常: %1").arg(e.what());
                failedFiles++;
                logError(QString("Exception processing file %1: %2").arg(fileInfo.filePath, e.what()));
            }
        }
        
        processedFiles++;
        
        // 发送进度更新
        int progress = (processedFiles * 100) / totalFiles;
        emit progressUpdated(progress, QString("处理文件 %1/%2").arg(processedFiles).arg(totalFiles));
    }
    
    // 保存最后使用的目录
    if (!m_fileInfoList.isEmpty()) {
        QString lastFilePath = m_fileInfoList.last().filePath;
        m_lastDirectory = QFileInfo(lastFilePath).absolutePath();
    }
    
    // 构建结果消息
    QString resultMessage;
    if (successFiles > 0 && failedFiles == 0) {
        resultMessage = QString("成功处理了 %1 个文件").arg(successFiles);
    } else if (successFiles > 0 && failedFiles > 0) {
        resultMessage = QString("成功处理了 %1 个文件，%2 个文件失败").arg(successFiles).arg(failedFiles);
    } else if (failedFiles > 0) {
        resultMessage = QString("处理失败，%1 个文件出错").arg(failedFiles);
    } else {
        resultMessage = "没有文件需要处理";
    }
    
    emit operationCompleted(resultMessage, successFiles > 0);
    
    // 更新UI
    updateFileList();
    updateButtonStates();
}
void AddSongDialogController::undoOperation() {
    logInfo("Undoing last operation");
    
    // 检查是否有可撤销的操作
    if (m_recentOperations.isEmpty()) {
        emit warningOccurred("没有可撤销的操作");
        return;
    }
    
    // 获取最近的操作
    Operation lastOp = m_recentOperations.takeLast();
    
    emit operationStarted("正在撤销操作...");
    
    try {
        // 根据操作类型执行撤销
        if (lastOp.type == "assign") {
            // 撤销分配标签操作 - 直接操作数据，不记录新的撤销操作
            logInfo(QString("Undoing assign operation: %1 -> %2").arg(lastOp.filePath, lastOp.tagName));
            
            for (auto& fileInfo : m_fileInfoList) {
                if (fileInfo.filePath == lastOp.filePath) {
                    QStringList currentTags = fileInfo.tagAssignment.split(",", Qt::SkipEmptyParts);
                    currentTags.removeAll(lastOp.tagName);
                    fileInfo.tagAssignment = currentTags.join(",");
                    
                    // 更新标签的歌曲数量
                    for (auto& tagInfo : m_tagInfoList) {
                        if (tagInfo.name == lastOp.tagName) {
                            tagInfo.songCount = qMax(0, tagInfo.songCount - 1);
                            break;
                        }
                    }
                    break;
                }
            }
            
            emit tagUnassigned(lastOp.tagName, QStringList() << lastOp.filePath);
            
        } else if (lastOp.type == "unassign") {
            // 撤销取消分配标签操作 - 直接操作数据，不记录新的撤销操作
            logInfo(QString("Undoing unassign operation: %1 -> %2").arg(lastOp.filePath, lastOp.tagName));
            
            for (auto& fileInfo : m_fileInfoList) {
                if (fileInfo.filePath == lastOp.filePath) {
                    QStringList currentTags = fileInfo.tagAssignment.split(",", Qt::SkipEmptyParts);
                    if (!currentTags.contains(lastOp.tagName)) {
                        currentTags.append(lastOp.tagName);
                        fileInfo.tagAssignment = currentTags.join(",");
                        
                        // 更新标签的歌曲数量
                        for (auto& tagInfo : m_tagInfoList) {
                            if (tagInfo.name == lastOp.tagName) {
                                tagInfo.songCount++;
                                break;
                            }
                        }
                    }
                    break;
                }
            }
            
            emit tagAssigned(lastOp.tagName, QStringList() << lastOp.filePath);
            
        } else {
            emit warningOccurred(QString("未知的操作类型: %1").arg(lastOp.type));
            return;
        }
        
        emit operationCompleted(QString("已撤销操作: %1 %2").arg(lastOp.type, lastOp.tagName), true);
        
    } catch (const std::exception& e) {
        logError(QString("Error during undo operation: %1").arg(e.what()));
        emit errorOccurred(QString("撤销操作失败: %1").arg(e.what()));
    }
    
    // 更新UI
    updateFileList();
    updateTagList();
    updateButtonStates();
}

void AddSongDialogController::deleteTag(const QString& name)
{
    logDebug(QString("deleteTag: %1").arg(name));
    
    // 检查标签名称是否有效
    if (name.isEmpty()) {
        emit warningOccurred("标签名称不能为空");
        return;
    }
    
    // 检查标签是否存在
    bool tagExists = false;
    int tagIndex = -1;
    
    for (int i = 0; i < m_tagInfoList.size(); ++i) {
        if (m_tagInfoList[i].name == name) {
            tagExists = true;
            tagIndex = i;
            break;
        }
    }
    
    if (!tagExists) {
        emit warningOccurred(QString("标签 '%1' 不存在").arg(name));
        return;
    }
    
    // 检查是否是系统标签
    if (m_tagInfoList[tagIndex].isDefault) {
        emit warningOccurred(QString("无法删除系统标签: %1").arg(name));
        return;
    }
    
    // 从列表中移除标签
    m_tagInfoList.removeAt(tagIndex);
    
    // 从数据库或其他存储中删除标签
    // 这里应该调用数据库操作方法
    
    // 发出信号通知UI更新
    emit tagDeleted(name);
    emit operationCompleted(QString("已删除标签: %1").arg(name), true);
    
    // 更新标签列表
    updateTagList();
    
    // 更新按钮状态
    updateButtonStates();
}
void AddSongDialogController::editTag(const QString& oldName, const QString& newName, const QString& color, const QString& iconPath) {
    logDebug(QString("editTag: %1 -> %2, color=%3, icon=%4").arg(oldName, newName, color, iconPath));
    
    // 检查标签名称是否有效
    if (oldName.isEmpty()) {
        emit warningOccurred("原标签名称不能为空");
        return;
    }
    
    if (newName.isEmpty()) {
        emit warningOccurred("新标签名称不能为空");
        return;
    }
    
    // 检查原标签是否存在
    bool oldTagExists = false;
    int tagIndex = -1;
    
    for (int i = 0; i < m_tagInfoList.size(); ++i) {
        if (m_tagInfoList[i].name == oldName) {
            oldTagExists = true;
            tagIndex = i;
            break;
        }
    }
    
    if (!oldTagExists) {
        emit warningOccurred(QString("标签 '%1' 不存在").arg(oldName));
        return;
    }
    
    // 检查是否是系统标签
    if (m_tagInfoList[tagIndex].isDefault) {
        emit warningOccurred(QString("无法编辑系统标签: %1").arg(oldName));
        return;
    }
    
    // 检查新标签名是否已存在（如果新名称与旧名称不同）
    if (oldName != newName) {
        for (const TagInfo& tag : m_tagInfoList) {
            if (tag.name == newName) {
                emit warningOccurred(QString("标签名称 '%1' 已存在").arg(newName));
                return;
            }
        }
    }
    
    // 更新标签信息
    m_tagInfoList[tagIndex].name = newName;
    
    if (!color.isEmpty()) {
        m_tagInfoList[tagIndex].color = color;
    }
    
    if (!iconPath.isEmpty()) {
        m_tagInfoList[tagIndex].iconPath = iconPath;
    }
    
    // 更新数据库或其他存储中的标签
    // 这里应该调用数据库操作方法
    
    // 发出信号通知UI更新
    emit tagEdited(oldName, newName);
    emit tagListChanged(); // 通知主界面刷新标签列表
    emit operationCompleted(QString("已编辑标签: %1 -> %2").arg(oldName, newName), true);
    
    // 更新标签列表
    updateTagList();
    
    // 更新按钮状态
    updateButtonStates();
}

bool AddSongDialogController::canUndo() const
{
    // 检查是否有可撤销的操作
    return !m_recentOperations.isEmpty();
}

QList<FileInfo> AddSongDialogController::getFileList() const
{
    return m_fileInfoList;
}

QList<FileInfo> AddSongDialogController::getFileInfoList() const
{
    return m_fileInfoList;
}

void AddSongDialogController::updateButtonStates()
{
    logDebug("updateButtonStates");
    
    // 检查对话框是否存在
    if (!m_dialog) {
        return;
    }
    
    // 不需要在这里获取状态，对话框的updateButtonStates方法会自行处理
    
    // 更新按钮状态
    // 注意：不应直接访问对话框的UI元素
    // 应该通过对话框的公共方法来更新按钮状态
    
    // 通知对话框更新按钮状态
    // 对话框内部会根据当前状态更新各个按钮
    if (m_dialog) {
        m_dialog->updateButtonStates();
    }
    
    // 注意：所有按钮状态更新都应该在对话框的updateButtonStates方法中处理
    // 不应在这里直接访问UI元素
}

// 获取选中文件的方法实现
QStringList AddSongDialogController::getSelectedFiles() const
{
    QStringList selectedFiles;
    
    if (!m_dialog) {
        logError("Dialog is null in getSelectedFiles");
        return selectedFiles;
    }
    
    // 获取文件列表控件
    QListWidget* fileListWidget = m_dialog->findChild<QListWidget*>("listWidget_added_songs");
    if (!fileListWidget) {
        logError("找不到文件列表控件 listWidget_added_songs");
        return selectedFiles;
    }
    
    // 获取选中的项目
    QList<QListWidgetItem*> selectedItems = fileListWidget->selectedItems();
    for (QListWidgetItem* item : selectedItems) {
        QString filePath = item->data(Qt::UserRole).toString();
        if (!filePath.isEmpty()) {
            selectedFiles.append(filePath);
        }
    }
    
    logDebug(QString("获取到 %1 个选中文件").arg(selectedFiles.size()));
    return selectedFiles;
}









void AddSongDialogController::logWarning(const QString& message)
{
    qWarning() << "[AddSongDialogController][WARNING]" << message;
    
    // 发出警告信号
    emit warningOccurred(message);
}





void AddSongDialogController::handleWarning(const QString& warning)
{
    logWarning(warning);
    emit warningOccurred(warning);
}

// 数据库相关方法
void AddSongDialogController::loadTagsFromDatabase()
{
    logInfo("Loading tags from database");
    
    if (!m_databaseManager || !m_databaseManager->isValid()) {
        logError("Database not available, using default tags");
        // 使用默认标签
        if (m_tagInfoList.isEmpty()) {
            TagInfo defaultTag = createDefaultTagInfo("我的歌曲");
            defaultTag.isDefault = true;
            defaultTag.isEditable = false;
            m_tagInfoList.append(defaultTag);
        }
        return;
    }
    
    try {
        // 从数据库加载标签
        QSqlQuery query(m_databaseManager->database());
        query.prepare("SELECT name, color, icon_path, is_default, is_editable FROM tags ORDER BY sort_order, name");
        
        if (!query.exec()) {
            logError(QString("Failed to load tags from database: %1").arg(query.lastError().text()));
            // 使用默认标签作为后备
            if (m_tagInfoList.isEmpty()) {
                TagInfo defaultTag = createDefaultTagInfo("我的歌曲");
                defaultTag.isDefault = true;
                defaultTag.isEditable = false;
                m_tagInfoList.append(defaultTag);
            }
            return;
        }
        
        m_tagInfoList.clear();
        
        while (query.next()) {
            TagInfo tagInfo;
            tagInfo.name = query.value("name").toString();
            tagInfo.displayName = tagInfo.name;
            tagInfo.color = query.value("color").toString();
            tagInfo.iconPath = query.value("icon_path").toString();
            tagInfo.isDefault = query.value("is_default").toBool();
            tagInfo.isEditable = query.value("is_editable").toBool();
            tagInfo.songCount = 0; // 可以后续查询获取
            
            m_tagInfoList.append(tagInfo);
        }
        
        // 如果数据库中没有标签，添加默认标签
        if (m_tagInfoList.isEmpty()) {
            TagInfo defaultTag = createDefaultTagInfo("我的歌曲");
            defaultTag.isDefault = true;
            defaultTag.isEditable = false;
            m_tagInfoList.append(defaultTag);
            
            // 将默认标签保存到数据库
            saveTagToDatabase(defaultTag);
        }
        
        logInfo(QString("Loaded %1 tags from database").arg(m_tagInfoList.size()));
        
    } catch (const std::exception& e) {
        logError(QString("Exception while loading tags: %1").arg(e.what()));
        // 使用默认标签作为后备
        if (m_tagInfoList.isEmpty()) {
            TagInfo defaultTag = createDefaultTagInfo("我的歌曲");
            defaultTag.isDefault = true;
            defaultTag.isEditable = false;
            m_tagInfoList.append(defaultTag);
        }
    }
}

void AddSongDialogController::saveTagToDatabase(const TagInfo& tagInfo)
{
    logInfo(QString("Saving tag to database: %1").arg(tagInfo.name));
    
    if (!m_databaseManager || !m_databaseManager->isValid()) {
        logError("Database not available, cannot save tag");
        return;
    }
    
    try {
        QSqlQuery query(m_databaseManager->database());
        
        // 检查标签是否已存在
        query.prepare("SELECT id FROM tags WHERE name = ?");
        query.addBindValue(tagInfo.name);
        
        if (!query.exec()) {
            logError(QString("Failed to check existing tag: %1").arg(query.lastError().text()));
            return;
        }
        
        if (query.next()) {
            // 标签已存在，更新
            int tagId = query.value("id").toInt();
            query.prepare("UPDATE tags SET color = ?, icon_path = ?, is_default = ?, is_editable = ? WHERE id = ?");
            query.addBindValue(tagInfo.color);
            query.addBindValue(tagInfo.iconPath);
            query.addBindValue(tagInfo.isDefault);
            query.addBindValue(tagInfo.isEditable);
            query.addBindValue(tagId);
        } else {
            // 标签不存在，插入新记录
            query.prepare("INSERT INTO tags (name, color, icon_path, is_default, is_editable, sort_order) VALUES (?, ?, ?, ?, ?, ?)");
            query.addBindValue(tagInfo.name);
            query.addBindValue(tagInfo.color);
            query.addBindValue(tagInfo.iconPath);
            query.addBindValue(tagInfo.isDefault);
            query.addBindValue(tagInfo.isEditable);
            query.addBindValue(0); // 默认排序顺序
        }
        
        if (!query.exec()) {
            logError(QString("Failed to save tag to database: %1").arg(query.lastError().text()));
        } else {
            logInfo(QString("Tag '%1' saved to database successfully").arg(tagInfo.name));
        }
        
    } catch (const std::exception& e) {
        logError(QString("Exception while saving tag: %1").arg(e.what()));
    }
}

void AddSongDialogController::deleteTagFromDatabase(const QString& tagName)
{
    logInfo(QString("Deleting tag from database: %1").arg(tagName));
    
    if (!m_databaseManager || !m_databaseManager->isValid()) {
        logError("Database manager not available, cannot delete tag");
        emit errorOccurred("数据库不可用，无法删除标签");
        return;
    }
    
    if (tagName.trimmed().isEmpty()) {
        logError("Tag name cannot be empty");
        emit warningOccurred("标签名不能为空");
        return;
    }
    
    try {
        // 使用TagDao删除标签
        TagDao tagDao;
        
        // 先获取标签信息
        Tag tag = tagDao.getTagByName(tagName);
        if (!tag.isValid()) {
            logWarning(QString("Tag not found: %1").arg(tagName));
            emit warningOccurred(QString("标签 '%1' 不存在").arg(tagName));
            return;
        }
        
        // 检查是否为系统标签
        if (tag.isSystem()) {
            logError(QString("Cannot delete system tag: %1").arg(tagName));
            emit errorOccurred("不能删除系统标签");
            return;
        }
        
        // 确认删除
        if (m_dialog) {
            int ret = QMessageBox::question(m_dialog, "确认删除", 
                QString("确定要删除标签 '%1' 吗？\n\n删除后该标签的所有关联将被移除。")
                .arg(tagName),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            
            if (ret != QMessageBox::Yes) {
                logInfo("Tag deletion cancelled by user");
                return;
            }
        }
        
        // 执行删除
        if (tagDao.deleteTag(tag.id())) {
            logInfo(QString("Tag deleted successfully: %1").arg(tagName));
            
            // 从本地标签列表中移除
            for (int i = 0; i < m_tagInfoList.size(); ++i) {
                if (m_tagInfoList[i].name == tagName) {
                    m_tagInfoList.removeAt(i);
                    break;
                }
            }
            
            // 刷新标签列表
            loadTagsFromDatabase();
            
            emit tagDeleted(tagName);
            emit operationCompleted(QString("标签 '%1' 删除成功").arg(tagName), true);
        } else {
            logError(QString("Failed to delete tag: %1").arg(tagName));
            emit errorOccurred(QString("删除标签 '%1' 失败").arg(tagName));
        }
        
    } catch (const std::exception& e) {
        logError(QString("Exception in deleteTagFromDatabase: %1").arg(e.what()));
        emit errorOccurred(QString("删除标签时发生错误: %1").arg(e.what()));
    } catch (...) {
        logError("Unknown exception in deleteTagFromDatabase");
        emit errorOccurred("删除标签时发生未知错误");
    }
}

TagInfo AddSongDialogController::createDefaultTagInfo(const QString& name)
{
    TagInfo tagInfo;
    tagInfo.name = name;
    tagInfo.displayName = name;
    tagInfo.color = "#4CAF50"; // 默认绿色
    tagInfo.iconPath = "";
    tagInfo.songCount = 0;
    tagInfo.isDefault = false;
    tagInfo.isEditable = true;
    
    return tagInfo;
}

// 文件处理方法
void AddSongDialogController::processFileInternal(const QString& filePath)
{
    logInfo(QString("Processing file internally: %1").arg(filePath));
    
    // 查找文件信息
    for (auto& fileInfo : m_fileInfoList) {
        if (fileInfo.filePath == filePath) {
            fileInfo.status = FileStatus::Processing;
            
            // 模拟文件处理
            QThread::msleep(100); // 模拟处理时间
            
            if (validateFileInternal(filePath)) {
                fileInfo.status = FileStatus::Completed;
                emit fileProcessed(filePath, true);
            } else {
                fileInfo.status = FileStatus::Failed;
                fileInfo.errorMessage = "文件验证失败";
                emit fileProcessed(filePath, false);
            }
            
            break;
        }
    }
    
    updateFileList();
}

void AddSongDialogController::analyzeFileInternal(const QString& filePath)
{
    logInfo(QString("Analyzing file internally: %1").arg(filePath));
    
    FileInfo info = extractFileInfo(filePath);
    
    // 更新文件信息列表中的对应项
    for (auto& fileInfo : m_fileInfoList) {
        if (fileInfo.filePath == filePath) {
            fileInfo.fileName = info.fileName;
            fileInfo.fileSize = info.fileSize;
            fileInfo.format = info.format;
            fileInfo.isValid = info.isValid;
            fileInfo.status = info.isValid ? FileStatus::Completed : FileStatus::Failed;
            
            if (!info.isValid) {
                fileInfo.errorMessage = "文件格式不支持或文件损坏";
            }
            
            break;
        }
    }
    
    emit fileAnalyzed(filePath, info);
    updateFileList();
}

// UI更新方法
void AddSongDialogController::updateProgressBar()
{
    if (!m_dialog) {
        return;
    }
    
    // 获取进度条控件
    QProgressBar* progressBar = m_dialog->findChild<QProgressBar*>("progressBar");
    if (progressBar) {
        int progress = 0;
        if (m_totalCount > 0) {
            progress = (m_processedCount * 100) / m_totalCount;
        }
        progressBar->setValue(progress);
    }
}

void AddSongDialogController::updateStatusBar()
{
    if (!m_dialog) {
        return;
    }
    
    // 获取状态标签控件
    QLabel* statusLabel = m_dialog->findChild<QLabel*>("label_status");
    if (statusLabel) {
        QString statusText;
        if (m_processing) {
            statusText = QString("正在处理... (%1/%2)").arg(m_processedCount).arg(m_totalCount);
        } else {
            statusText = QString("就绪 - 共 %1 个文件").arg(m_fileInfoList.size());
        }
        statusLabel->setText(statusText);
    }
}

// 获取选中标签的方法实现
QStringList AddSongDialogController::getSelectedTags() const
{
    QStringList selectedTags;
    
    if (!m_dialog) {
        return selectedTags;
    }
    
    // 获取标签列表控件
    QListWidget* tagListWidget = m_dialog->findChild<QListWidget*>("listWidget_system_tags");
    if (!tagListWidget) {
        logError("找不到标签列表控件");
        return selectedTags;
    }
    
    // 获取选中的项目
    QList<QListWidgetItem*> selectedItems = tagListWidget->selectedItems();
    for (QListWidgetItem* item : selectedItems) {
        QString tagName = item->data(Qt::UserRole).toString();
        if (!tagName.isEmpty()) {
            selectedTags << tagName;
        }
    }
    
    return selectedTags;
}

// 全选文件方法实现
void AddSongDialogController::selectAllFiles()
{
    logInfo("Selecting all files");
    
    if (!m_dialog) {
        logError("Dialog is null");
        return;
    }
    
    // 获取文件列表控件
    QListWidget* fileListWidget = m_dialog->findChild<QListWidget*>("listWidget_added_songs");
    if (!fileListWidget) {
        logError("找不到文件列表控件");
        return;
    }
    
    // 选中所有项目
    fileListWidget->selectAll();
    
    // 更新按钮状态
    updateButtonStates();
    
    emit filesSelectionChanged(getSelectedFiles());
    logInfo(QString("Selected all %1 files").arg(fileListWidget->count()));
}

// 清除选择方法实现
void AddSongDialogController::clearSelection()
{
    logInfo("Clearing all selections");
    
    if (!m_dialog) {
        logError("Dialog is null");
        return;
    }
    
    // 获取文件列表控件
    QListWidget* fileListWidget = m_dialog->findChild<QListWidget*>("listWidget_added_songs");
    if (fileListWidget) {
        fileListWidget->clearSelection();
    }
    
    // 获取标签列表控件
    QListWidget* tagListWidget = m_dialog->findChild<QListWidget*>("listWidget_system_tags");
    if (tagListWidget) {
        tagListWidget->clearSelection();
    }
    
    // 更新按钮状态
    updateButtonStates();
    
    emit filesSelectionChanged(QStringList());
    emit tagsSelectionChanged(QStringList());
    logInfo("Cleared all selections");
}

// 批量分配标签方法实现
void AddSongDialogController::batchAssignTags(const QStringList& files, const QStringList& tags)
{
    logInfo(QString("Batch assigning %1 tags to %2 files").arg(tags.size()).arg(files.size()));
    
    if (files.isEmpty() || tags.isEmpty()) {
        emit warningOccurred("请选择文件和标签");
        return;
    }
    
    emit operationStarted(QString("正在为 %1 个文件批量添加 %2 个标签...").arg(files.size()).arg(tags.size()));
    
    int totalSuccess = 0;
    for (const QString& tagName : tags) {
        // 跳过"我的歌曲"标签，因为所有歌曲都会自动添加到这个标签
        if (tagName == "我的歌曲") {
            continue;
        }
        
        int successCount = 0;
        for (const QString& filePath : files) {
            // 检查文件是否存在于列表中
            for (auto& fileInfo : m_fileInfoList) {
                if (fileInfo.filePath == filePath) {
                    // 检查是否已经分配了该标签
                    QStringList currentTags = fileInfo.tagAssignment.split(",", Qt::SkipEmptyParts);
                    if (!currentTags.contains(tagName)) {
                        currentTags.append(tagName);
                        fileInfo.tagAssignment = currentTags.join(",");
                        
                        // 记录操作用于撤销
                        Operation op;
                        op.type = "assign";
                        op.filePath = filePath;
                        op.tagName = tagName;
                        m_recentOperations.append(op);
                        
                        successCount++;
                        totalSuccess++;
                    }
                    break;
                }
            }
        }
        
        // 更新标签的歌曲数量
        for (auto& tagInfo : m_tagInfoList) {
            if (tagInfo.name == tagName) {
                tagInfo.songCount += successCount;
                break;
            }
        }
    }
    
    // 更新UI
    updateFileList();
    updateTagList();
    updateButtonStates();
    
    emit operationCompleted(QString("批量操作完成，成功添加了 %1 个标签分配").arg(totalSuccess), totalSuccess > 0);
}

// 批量取消分配标签方法实现
void AddSongDialogController::batchUnassignTags(const QStringList& files, const QStringList& tags)
{
    logInfo(QString("Batch unassigning %1 tags from %2 files").arg(tags.size()).arg(files.size()));
    
    if (files.isEmpty() || tags.isEmpty()) {
        emit warningOccurred("请选择文件和标签");
        return;
    }
    
    emit operationStarted(QString("正在从 %1 个文件批量移除 %2 个标签...").arg(files.size()).arg(tags.size()));
    
    int totalSuccess = 0;
    for (const QString& tagName : tags) {
        // 不允许从"我的歌曲"标签移除歌曲
        if (tagName == "我的歌曲") {
            continue;
        }
        
        int successCount = 0;
        for (const QString& filePath : files) {
            // 检查文件是否存在于列表中
            for (auto& fileInfo : m_fileInfoList) {
                if (fileInfo.filePath == filePath) {
                    // 检查是否已经分配了该标签
                    QStringList currentTags = fileInfo.tagAssignment.split(",", Qt::SkipEmptyParts);
                    if (currentTags.contains(tagName)) {
                        currentTags.removeAll(tagName);
                        fileInfo.tagAssignment = currentTags.join(",");
                        
                        // 记录操作用于撤销
                        Operation op;
                        op.type = "unassign";
                        op.filePath = filePath;
                        op.tagName = tagName;
                        m_recentOperations.append(op);
                        
                        successCount++;
                        totalSuccess++;
                    }
                    break;
                }
            }
        }
        
        // 更新标签的歌曲数量
        for (auto& tagInfo : m_tagInfoList) {
            if (tagInfo.name == tagName) {
                tagInfo.songCount = qMax(0, tagInfo.songCount - successCount);
                break;
            }
        }
    }
    
    // 更新UI
    updateFileList();
    updateTagList();
    updateButtonStates();
    
    emit operationCompleted(QString("批量操作完成，成功移除了 %1 个标签分配").arg(totalSuccess), totalSuccess > 0);
 }

// 文件分析方法实现
void AddSongDialogController::analyzeFile(const QString& filePath)
{
    logInfo(QString("Analyzing file: %1").arg(filePath));
    
    if (!QFile::exists(filePath)) {
        logError(QString("File does not exist: %1").arg(filePath));
        return;
    }
    
    emit operationStarted(QString("正在分析文件: %1").arg(QFileInfo(filePath).fileName()));
    
    // 查找文件信息
    FileInfo* fileInfo = nullptr;
    for (auto& info : m_fileInfoList) {
        if (info.filePath == filePath) {
            fileInfo = &info;
            break;
        }
    }
    
    if (!fileInfo) {
        logError(QString("File not found in list: %1").arg(filePath));
        emit operationCompleted("文件分析失败：文件不在列表中", false);
        return;
    }
    
    try {
        // 更新文件基本信息
        QFileInfo qFileInfo(filePath);
        fileInfo->fileName = qFileInfo.fileName();
        fileInfo->fileSize = qFileInfo.size();
        fileInfo->format = getFileFormat(filePath);
        fileInfo->status = FileStatus::Completed;
        
        // 这里可以添加更多的音频文件分析逻辑
        // 例如使用第三方库读取音频元数据（标题、艺术家、专辑等）
        
        // 模拟分析过程
        QThread::msleep(100); // 模拟分析时间
        
        // 更新UI
        updateFileList();
        
        emit fileAnalyzed(filePath, *fileInfo);
        emit operationCompleted(QString("文件分析完成: %1").arg(qFileInfo.fileName()), true);
        
        logInfo(QString("File analysis completed: %1").arg(filePath));
    }
    catch (const std::exception& e) {
        logError(QString("Error analyzing file %1: %2").arg(filePath).arg(e.what()));
        fileInfo->status = FileStatus::Failed;
        updateFileList();
        emit operationCompleted("文件分析失败", false);
    }
}

// 获取文件信息方法实现
FileInfo AddSongDialogController::getFileInfo(const QString& filePath) const
{
    for (const auto& fileInfo : m_fileInfoList) {
        if (fileInfo.filePath == filePath) {
            return fileInfo;
        }
    }
    
    // 返回空的文件信息
    FileInfo emptyInfo;
    emptyInfo.filePath = filePath;
    emptyInfo.fileName = QFileInfo(filePath).fileName();
    emptyInfo.status = FileStatus::Failed;
    return emptyInfo;
}

// 获取标签信息方法实现
TagInfo AddSongDialogController::getTagInfo(const QString& tagName) const
{
    for (const auto& tagInfo : m_tagInfoList) {
        if (tagInfo.name == tagName) {
            return tagInfo;
        }
    }
    
    // 返回空的标签信息
    TagInfo emptyInfo;
    emptyInfo.name = tagName;
    emptyInfo.songCount = 0;
    emptyInfo.color = "#808080"; // 默认灰色
    return emptyInfo;
}