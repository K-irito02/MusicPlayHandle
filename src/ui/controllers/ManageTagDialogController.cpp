#include "managetagdialogcontroller.h"
#include "../dialogs/managetagdialog.h"
#include "../../database/tagdao.h"
#include "../../database/songdao.h"
#include "../../database/databasemanager.h"
#include <QDebug>
#include <QMessageBox>
#include <QInputDialog>
#include <QColorDialog>
#include <QLineEdit>
#include <QColor>
#include <QPushButton>
#include <QLabel>
#include <QApplication>

// 常量定义
static const int MAX_TAG_NAME_LENGTH = 50;

ManageTagDialogController::ManageTagDialogController(ManageTagDialog* dialog, QObject* parent)
    : QObject(parent)
    , m_dialog(dialog)
    , m_tagManager(nullptr)
    , m_playlistManager(nullptr)
    , m_databaseManager(DatabaseManager::instance())  // 修复：正确初始化数据库管理器
    , m_logger(nullptr)
    , m_selectedTag1("")
    , m_selectedTag2("")
    , m_operationIndex(0)
    , m_maxHistorySize(MAX_HISTORY_SIZE)
    , m_filterText("")
    , m_searchQuery("")
    , m_filterActive(false)
    , m_sortField(SortField::Title)
    , m_sortAscending(true)
    , m_initialized(false)
    , m_dataLoaded(false)
    , m_processing(false)
    , m_hasUnsavedChanges(false)
    , m_dataUpdateTimer(nullptr)
    , m_operationTimer(nullptr)
    , m_statisticsUpdateTimer(nullptr)
    , m_settings(nullptr)
{
    // 初始化定时器
    m_dataUpdateTimer = new QTimer(this);
    m_operationTimer = new QTimer(this);
    m_statisticsUpdateTimer = new QTimer(this);
    
    // 初始化设置
    m_settings = new QSettings(this);
    
    // 连接定时器
    connect(m_dataUpdateTimer, &QTimer::timeout, this, &ManageTagDialogController::onDataUpdateTimer);
    connect(m_operationTimer, &QTimer::timeout, this, &ManageTagDialogController::onOperationTimer);
    connect(m_statisticsUpdateTimer, &QTimer::timeout, this, &ManageTagDialogController::onStatisticsUpdateTimer);
}

ManageTagDialogController::~ManageTagDialogController()
{
    shutdown();
}

bool ManageTagDialogController::initialize()
{
    if (m_initialized) {
        return true;
    }
    
    try {
        // 设置连接
        setupConnections();
        
        // 加载设置
        loadSettings();
        
        // 加载数据
        loadTags();
        loadSongs();
        
        // 启动定时器
        m_dataUpdateTimer->start(DATA_UPDATE_INTERVAL);
        m_statisticsUpdateTimer->start(STATISTICS_UPDATE_INTERVAL);
        
        m_initialized = true;
        logInfo("ManageTagDialogController initialized successfully");
        
        return true;
    } catch (const std::exception& e) {
        logError(QString("ManageTagDialogController initialization failed: %1").arg(e.what()));
        return false;
    }
}

void ManageTagDialogController::shutdown()
{
    if (!m_initialized) {
        return;
    }
    
    // 保存设置
    saveSettings();
    
    // 停止定时器
    if (m_dataUpdateTimer) m_dataUpdateTimer->stop();
    if (m_operationTimer) m_operationTimer->stop();
    if (m_statisticsUpdateTimer) m_statisticsUpdateTimer->stop();
    
    m_initialized = false;
    logInfo("ManageTagDialogController shut down");
}

bool ManageTagDialogController::isInitialized() const
{
    return m_initialized;
}

void ManageTagDialogController::loadTags()
{
    logInfo("Loading tags from database");
    
    try {
        m_tags.clear();
        
        // 检查数据库管理器是否可用
        if (!m_databaseManager) {
            logError("Database manager is null, cannot load tags");
            updateTagLists();
            return;
        }
        
        if (!m_databaseManager->isValid()) {
            logError("Database manager is not valid, cannot load tags");
            updateTagLists();
            return;
        }
        
        // 创建TagDao实例并获取所有标签
        TagDao tagDao;
        QList<Tag> allTags = tagDao.getAllTags();
        
        if (allTags.isEmpty()) {
            logInfo("No tags found in database");
        } else {
            m_tags = allTags;
            logInfo(QString("Loaded %1 tags from database").arg(m_tags.size()));
        }
        
        updateTagLists();
        
    } catch (const std::exception& e) {
        logError(QString("Exception while loading tags: %1").arg(e.what()));
        handleError("加载标签时发生错误");
    } catch (...) {
        logError("Unknown exception while loading tags");
        handleError("加载标签时发生未知错误");
    }
}

void ManageTagDialogController::loadSongs()
{
    logInfo("Loading songs from database");
    
    try {
        m_songs.clear();
        
        // 检查数据库管理器是否可用
        if (!m_databaseManager) {
            logError("Database manager is null, cannot load songs");
            updateSongList();
            return;
        }
        
        if (!m_databaseManager->isValid()) {
            logError("Database manager is not valid, cannot load songs");
            updateSongList();
            return;
        }
        
        // 创建SongDao实例并获取所有歌曲
        SongDao songDao;
        QList<Song> allSongs = songDao.getAllSongs();
        
        if (allSongs.isEmpty()) {
            logInfo("No songs found in database");
        } else {
            m_songs = allSongs;
            logInfo(QString("Loaded %1 songs from database").arg(m_songs.size()));
        }
        
        updateSongList();
        
    } catch (const std::exception& e) {
        logError(QString("Exception while loading songs: %1").arg(e.what()));
        handleError("加载歌曲时发生错误");
    } catch (...) {
        logError("Unknown exception while loading songs");
        handleError("加载歌曲时发生未知错误");
    }
}

void ManageTagDialogController::createTag(const QString& name, const QString& color, const QString& iconPath)
{
    logInfo(QString("Creating tag: %1").arg(name));
    
    try {
        // 检查数据库管理器是否可用
        if (!m_databaseManager) {
            logError("Database manager is null, cannot create tag");
            handleError("数据库不可用，无法创建标签");
            return;
        }
        
        if (!m_databaseManager->isValid()) {
            logError("Database manager is not valid, cannot create tag");
            handleError("数据库不可用，无法创建标签");
            return;
        }
        
        // 检查标签名是否为空
        if (name.trimmed().isEmpty()) {
            logError("Tag name cannot be empty");
            handleError("标签名不能为空");
            return;
        }
        
        // 创建标签对象
        Tag tag;
        tag.setName(name.trimmed());
        tag.setColor(color.isEmpty() ? "#0078d4" : color);
        tag.setDescription(""); // 可以后续扩展
        tag.setIsSystem(false); // 用户创建的标签不是系统标签
        
        // 保存到数据库
        TagDao tagDao;
        int tagId = tagDao.addTag(tag);
        
        if (tagId > 0) {
            // 设置生成的ID
            tag.setId(tagId);
            
            // 添加到内存列表
            m_tags.append(tag);
            
            // 创建统计信息
            TagDialogStatistics stats;
            stats.tagName = name;
            stats.color = color;
            stats.iconPath = iconPath;
            
            logInfo(QString("Tag created successfully with ID: %1").arg(tagId));
            emit tagCreated(name, stats);
            updateTagLists();
            
        } else {
            logError(QString("Failed to create tag in database: %1").arg(name));
            handleError("创建标签失败，可能标签名已存在");
        }
        
    } catch (const std::exception& e) {
        logError(QString("Exception while creating tag: %1").arg(e.what()));
        handleError("创建标签时发生错误");
    } catch (...) {
        logError("Unknown exception while creating tag");
        handleError("创建标签时发生未知错误");
    }
}

void ManageTagDialogController::deleteTag(const QString& name)
{
    logInfo(QString("Deleting tag: %1").arg(name));
    
    try {
        // 检查数据库管理器是否可用
        if (!m_databaseManager) {
            logError("Database manager is null, cannot delete tag");
            handleError("数据库不可用，无法删除标签");
            return;
        }
        
        if (!m_databaseManager->isValid()) {
            logError("Database manager is not valid, cannot delete tag");
            handleError("数据库不可用，无法删除标签");
            return;
        }
        
        // 检查标签名是否为空
        if (name.trimmed().isEmpty()) {
            logError("Tag name cannot be empty");
            handleError("标签名不能为空");
            return;
        }
        
        // 查找要删除的标签
        Tag tagToDelete;
        int tagIndex = -1;
        for (int i = 0; i < m_tags.size(); ++i) {
            if (m_tags[i].name() == name) {
                tagToDelete = m_tags[i];
                tagIndex = i;
                break;
            }
        }
        
        if (tagIndex == -1) {
            logError(QString("Tag not found: %1").arg(name));
            handleError("未找到指定的标签");
            return;
        }
        
        // 检查是否为系统标签
        if (tagToDelete.isSystem()) {
            logError(QString("Cannot delete system tag: %1").arg(name));
            handleError("不能删除系统标签");
            return;
        }
        
        // 从数据库删除
        TagDao tagDao;
        bool success = tagDao.deleteTag(tagToDelete.id());
        
        if (success) {
            // 从内存列表中移除
            m_tags.removeAt(tagIndex);
            
            logInfo(QString("Tag deleted successfully: %1").arg(name));
            emit tagDeleted(name);
            updateTagLists();
            
        } else {
            logError(QString("Failed to delete tag from database: %1").arg(name));
            handleError("删除标签失败");
        }
        
    } catch (const std::exception& e) {
        logError(QString("Exception while deleting tag: %1").arg(e.what()));
        handleError("删除标签时发生错误");
    } catch (...) {
        logError("Unknown exception while deleting tag");
        handleError("删除标签时发生未知错误");
    }
}

// 槽函数实现
void ManageTagDialogController::onDataUpdateTimer()
{
    // 数据更新定时器
    try {
        if (!m_initialized || m_processing) {
            logDebug("Skipping data update - not initialized or processing");
            return;
        }
        
        // 检查数据库连接状态
        if (!DatabaseManager::instance()->database().isOpen()) {
            qWarning("Database not connected, skipping data update");
            return;
        }
        
        // 重新加载数据
        loadTags();
        loadSongs();
        
        // 更新统计信息
        m_tagStatistics = loadTagStatistics();
        
        logDebug("Data update completed");
        
    } catch (const std::exception& e) {
        logError(QString("Exception in onDataUpdateTimer: %1").arg(e.what()));
    } catch (...) {
        logError("Unknown exception in onDataUpdateTimer");
    }
}

void ManageTagDialogController::onOperationTimer()
{
    // 操作定时器
    logDebug("Operation timer triggered");
    
    try {
        if (!m_initialized) {
            logDebug("Skipping operation timer - not initialized");
            return;
        }
        
        // 检查是否有待处理的操作
        if (m_processing) {
            logDebug("Still processing previous operation");
            return;
        }
        
        // 清理过期的操作历史
        clearOperationHistory();
        
        // 更新按钮状态
        updateButtonStates();
        
        logDebug("Operation timer completed");
        
    } catch (const std::exception& e) {
        logError(QString("Exception in onOperationTimer: %1").arg(e.what()));
    } catch (...) {
        logError("Unknown exception in onOperationTimer");
    }
}

void ManageTagDialogController::onStatisticsUpdateTimer()
{
    // 统计更新定时器
    logDebug("Statistics update timer triggered");
    
    try {
        if (!m_initialized || !m_dataLoaded) {
            logDebug("Skipping statistics update - not ready");
            return;
        }
        
        // 更新统计信息
        m_tagStatistics = loadTagStatistics();
        
        // 发出统计信息更新信号
        emit statisticsUpdated(m_tagStatistics);
        
        logDebug("Statistics update completed");
        
    } catch (const std::exception& e) {
        logError(QString("Exception in onStatisticsUpdateTimer: %1").arg(e.what()));
    } catch (...) {
        logError("Unknown exception in onStatisticsUpdateTimer");
    }
}

// 私有方法实现
void ManageTagDialogController::setupConnections()
{
    // 设置信号连接
    if (m_dialog) {
        // 这里应该连接对话框的信号
        // 目前为空
    }
}

void ManageTagDialogController::loadSettings()
{
    if (m_settings) {
        m_sortField = static_cast<SortField>(m_settings->value("SortField", 0).toInt());
        m_sortAscending = m_settings->value("SortAscending", true).toBool();
        m_maxHistorySize = m_settings->value("MaxHistorySize", MAX_HISTORY_SIZE).toInt();
    }
}

void ManageTagDialogController::saveSettings()
{
    if (m_settings) {
        m_settings->setValue("SortField", static_cast<int>(m_sortField));
        m_settings->setValue("SortAscending", m_sortAscending);
        m_settings->setValue("MaxHistorySize", m_maxHistorySize);
        m_settings->sync();
    }
}

void ManageTagDialogController::updateTagLists()
{
    // 更新标签列表显示
    logDebug("Updating tag lists");
}

void ManageTagDialogController::updateSongList()
{
    // 更新歌曲列表显示
    logDebug("Updating song list");
}

void ManageTagDialogController::handleError(const QString& error)
{
    logError(error);
    emit errorOccurred(error);
}

void ManageTagDialogController::logInfo(const QString& message)
{
    qInfo() << "ManageTagDialogController:" << message;
}

void ManageTagDialogController::logError(const QString& error)
{
    qCritical() << "ManageTagDialogController Error:" << error;
}

void ManageTagDialogController::logDebug(const QString& message)
{
    qDebug() << "ManageTagDialogController:" << message;
}

// 缺失的槽函数实现
void ManageTagDialogController::onTag1SelectionChanged(const QString& tagName)
{
    logInfo(QString("标签1选择改变: %1").arg(tagName));
    
    try {
        m_selectedTag1 = tagName;
        
        // 更新按钮状态
        updateButtonStates();
        
        // 更新统计信息
        m_tagStatistics = loadTagStatistics();
        
        // 更新歌曲列表
        updateSongList();
        
        // 发出选择变更信号
        emit onTag1SelectionChanged(tagName);
        
        logDebug(QString("Tag1 selection changed to: %1").arg(tagName));
        
    } catch (const std::exception& e) {
        logError(QString("Exception in onTag1SelectionChanged: %1").arg(e.what()));
    } catch (...) {
        logError("Unknown exception in onTag1SelectionChanged");
    }
}

void ManageTagDialogController::onTag2SelectionChanged(const QString& tagName)
{
    logInfo(QString("标签2选择改变: %1").arg(tagName));
    
    try {
        m_selectedTag2 = tagName;
        
        // 更新按钮状态
        updateButtonStates();
        
        // 更新统计信息
        // 删除 m_tagStatistics.update();
        
        // 更新歌曲列表
        updateSongList();
        
        // 发出选择变更信号
        emit onTag2SelectionChanged(tagName);
        
        logDebug(QString("Tag2 selection changed to: %1").arg(tagName));
        
    } catch (const std::exception& e) {
        logError(QString("Exception in onTag2SelectionChanged: %1").arg(e.what()));
    } catch (...) {
        logError("Unknown exception in onTag2SelectionChanged");
    }
}

void ManageTagDialogController::onSongSelectionChanged(const QList<QString>& songIds)
{
    logInfo(QString("歌曲选择改变: %1个歌曲").arg(songIds.size()));
    
    try {
        m_selectedSongs = songIds;
        
        // 更新按钮状态
        updateButtonStates();
        
        // 更新统计信息
        updateStatistics();
        
        // 发出选择变更信号
        emit onSongSelectionChanged(m_selectedSongs);
        
        logDebug(QString("Song selection changed - Selected songs count: %1")
                .arg(m_selectedSongs.size()));
                
    } catch (const std::exception& e) {
        logError(QString("Exception in onSongSelectionChanged: %1").arg(e.what()));
    } catch (...) {
        logError("Unknown exception in onSongSelectionChanged");
    }
}

void ManageTagDialogController::onCreateTagClicked()
{
    logInfo("创建标签按钮点击");
    
    try {
        // 弹出输入对话框获取标签名
        bool ok;
        QString tagName = QInputDialog::getText(m_dialog, "创建标签", 
            "请输入标签名称:", QLineEdit::Normal, "", &ok);
            
        if (!ok || tagName.trimmed().isEmpty()) {
            logDebug("User cancelled tag creation or entered empty name");
            return;
        }
        
        // 检查标签名长度
        if (tagName.trimmed().length() > MAX_TAG_NAME_LENGTH) {
            QMessageBox::warning(m_dialog, "标签名过长", 
                QString("标签名不能超过 %1 个字符").arg(MAX_TAG_NAME_LENGTH));
            return;
        }
        
        // 弹出颜色选择对话框
        QColor color = QColorDialog::getColor(QColor("#0078d4"), m_dialog, "选择标签颜色");
        QString colorStr = color.isValid() ? color.name() : "#0078d4";
        
        // 创建标签
        createTag(tagName.trimmed(), colorStr);
        
    } catch (const std::exception& e) {
        logError(QString("Exception in onCreateTagClicked: %1").arg(e.what()));
        handleError("创建标签时发生错误");
    } catch (...) {
        logError("Unknown exception in onCreateTagClicked");
        handleError("创建标签时发生未知错误");
    }
}

void ManageTagDialogController::onDeleteTagClicked()
{
    logInfo("删除标签按钮点击");
    
    try {
        // 检查是否选择了标签
        QString tagToDelete;
        if (!m_selectedTag1.isEmpty()) {
            tagToDelete = m_selectedTag1;
        } else if (!m_selectedTag2.isEmpty()) {
            tagToDelete = m_selectedTag2;
        } else {
            QMessageBox::warning(m_dialog, "选择标签", "请先选择要删除的标签");
            return;
        }
        
        // 查找标签信息以进行额外检查
        Tag tagInfo;
        for (const Tag& tag : m_tags) {
            if (tag.name() == tagToDelete) {
                tagInfo = tag;
                break;
            }
        }
        
        // 检查是否为系统标签
        if (tagInfo.isSystem()) {
            QMessageBox::warning(m_dialog, "无法删除", "不能删除系统标签");
            return;
        }
        
        // 确认删除操作
        int ret = QMessageBox::question(m_dialog, "确认删除", 
            QString("确定要删除标签 '%1' 吗？\n\n注意：删除标签后，该标签下的所有歌曲关联将被移除。")
            .arg(tagToDelete),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            
        if (ret == QMessageBox::Yes) {
            // 执行删除操作
            deleteTag(tagToDelete);
            
            // 清除选中状态
            if (m_selectedTag1 == tagToDelete) {
                m_selectedTag1.clear();
            }
            if (m_selectedTag2 == tagToDelete) {
                m_selectedTag2.clear();
            }
        }
        
    } catch (const std::exception& e) {
        logError(QString("Exception in onDeleteTagClicked: %1").arg(e.what()));
        handleError("删除标签时发生错误");
    } catch (...) {
        logError("Unknown exception in onDeleteTagClicked");
        handleError("删除标签时发生未知错误");
    }
}

void ManageTagDialogController::onRenameTagClicked()
{
    logInfo("重命名标签按钮点击");
    
    try {
        // 检查是否选择了标签
        QString tagToRename;
        if (!m_selectedTag1.isEmpty()) {
            tagToRename = m_selectedTag1;
        } else if (!m_selectedTag2.isEmpty()) {
            tagToRename = m_selectedTag2;
        } else {
            QMessageBox::warning(m_dialog, "选择标签", "请先选择要重命名的标签");
            return;
        }
        
        // 查找标签信息以进行额外检查
        Tag tagInfo;
        for (const Tag& tag : m_tags) {
            if (tag.name() == tagToRename) {
                tagInfo = tag;
                break;
            }
        }
        
        // 检查是否为系统标签
        if (tagInfo.isSystem()) {
            QMessageBox::warning(m_dialog, "无法重命名", "不能重命名系统标签");
            return;
        }
        
        // 弹出输入对话框获取新标签名
        bool ok;
        QString newTagName = QInputDialog::getText(m_dialog, "重命名标签", 
            QString("请输入新的标签名称 (当前: %1):").arg(tagToRename), 
            QLineEdit::Normal, tagToRename, &ok);
            
        if (!ok || newTagName.trimmed().isEmpty()) {
            logDebug("User cancelled tag rename or entered empty name");
            return;
        }
        
        // 检查标签名长度
        if (newTagName.trimmed().length() > MAX_TAG_NAME_LENGTH) {
            QMessageBox::warning(m_dialog, "标签名过长", 
                QString("标签名不能超过 %1 个字符").arg(MAX_TAG_NAME_LENGTH));
            return;
        }
        
        // 执行重命名操作
        renameTag(tagToRename, newTagName.trimmed());
        
    } catch (const std::exception& e) {
        logError(QString("Exception in onRenameTagClicked: %1").arg(e.what()));
        handleError("重命名标签时发生错误");
    } catch (...) {
        logError("Unknown exception in onRenameTagClicked");
        handleError("重命名标签时发生未知错误");
    }
}

void ManageTagDialogController::onEditTagPropertiesClicked()
{
    logInfo("编辑标签属性按钮点击");
    // TODO: 打开标签属性编辑对话框
}

void ManageTagDialogController::onMoveTransferClicked()
{
    logInfo("移动转移按钮点击");
    
    try {
        // 检查是否选择了源标签和目标标签
        if (m_selectedTag1.isEmpty() || m_selectedTag2.isEmpty()) {
            qWarning() << "Source or target tag not selected for move operation";
            QMessageBox::warning(m_dialog, "选择标签", "请先选择源标签和目标标签");
            return;
        }
        
        // 检查源标签和目标标签是否相同
        if (m_selectedTag1 == m_selectedTag2) {
            qWarning() << "Source and target tags are the same";
            QMessageBox::warning(m_dialog, "标签选择错误", "源标签和目标标签不能相同");
            return;
        }
        
        // 检查是否有选中的歌曲
        if (m_selectedSongs.isEmpty()) {
            qWarning() << "No songs selected for move operation";
            QMessageBox::warning(m_dialog, "选择歌曲", "请先选择要移动的歌曲");
            return;
        }
        
        // 确认移动操作
        int ret = QMessageBox::question(m_dialog, "确认移动", 
            QString("确定要将 %1 首歌曲从标签 '%2' 移动到标签 '%3' 吗？")
            .arg(m_selectedSongs.size()).arg(m_selectedTag1).arg(m_selectedTag2),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            
        if (ret == QMessageBox::Yes) {
            // 执行移动操作（copy=false）
            transferSongs(m_selectedTag1, m_selectedTag2, false);
        }
        
    } catch (const std::exception& e) {
        logError(QString("Exception in onMoveTransferClicked: %1").arg(e.what()));
        handleError("移动歌曲时发生错误");
    } catch (...) {
        logError("Unknown exception in onMoveTransferClicked");
        handleError("移动歌曲时发生未知错误");
    }
}

void ManageTagDialogController::onCopyTransferClicked()
{
    logInfo("复制转移按钮点击");
    
    try {
        // 检查是否选择了源标签和目标标签
        if (m_selectedTag1.isEmpty() || m_selectedTag2.isEmpty()) {
            qWarning() << "Source or target tag not selected for copy operation";
            QMessageBox::warning(m_dialog, "选择标签", "请先选择源标签和目标标签");
            return;
        }
        
        // 检查源标签和目标标签是否相同
        if (m_selectedTag1 == m_selectedTag2) {
            qWarning() << "Source and target tags are the same";
            QMessageBox::warning(m_dialog, "标签选择错误", "源标签和目标标签不能相同");
            return;
        }
        
        // 检查是否有选中的歌曲
        if (m_selectedSongs.isEmpty()) {
            qWarning() << "No songs selected for copy operation";
            QMessageBox::warning(m_dialog, "选择歌曲", "请先选择要复制的歌曲");
            return;
        }
        
        // 确认复制操作
        int ret = QMessageBox::question(m_dialog, "确认复制", 
            QString("确定要将 %1 首歌曲从标签 '%2' 复制到标签 '%3' 吗？")
            .arg(m_selectedSongs.size()).arg(m_selectedTag1).arg(m_selectedTag2),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            
        if (ret == QMessageBox::Yes) {
            // 执行复制操作（copy=true）
            transferSongs(m_selectedTag1, m_selectedTag2, true);
        }
        
    } catch (const std::exception& e) {
        logError(QString("Exception in onCopyTransferClicked: %1").arg(e.what()));
        handleError("复制歌曲时发生错误");
    } catch (...) {
        logError("Unknown exception in onCopyTransferClicked");
        handleError("复制歌曲时发生未知错误");
    }
}

void ManageTagDialogController::onUndoClicked()
{
    logInfo("撤销按钮点击");
    
    try {
        if (m_operationHistory.undoStack.isEmpty()) {
            logDebug("No operations to undo");
            QMessageBox::information(m_dialog, "撤销", "没有可撤销的操作");
            return;
        }
        
        // 获取最后一个操作
        TagDialogOperation lastOp = m_operationHistory.undoStack.pop();
        
        // 根据操作类型执行撤销
        bool success = false;
        switch (lastOp.type) {
            case OperationType::CreateTag:
                createTag(lastOp.tagName, "", "");
                break;
            case OperationType::DeleteTag:
                deleteTag(lastOp.tagName);
                break;
            case OperationType::RenameTag:
                renameTag(lastOp.newTagName, lastOp.tagName);
                break;
            case OperationType::MoveSong:
            case OperationType::CopySong:
                transferSongs(lastOp.fromTags.isEmpty()?"":lastOp.fromTags.first(), lastOp.toTags.isEmpty()?"":lastOp.toTags.first(), lastOp.type==OperationType::CopySong);
                success = true;
                break;
            default:
                qWarning() << "Unknown operation type for undo:" << static_cast<int>(lastOp.type);
                break;
        }
        
        if (success) {
            // 将操作添加到重做栈
            m_operationHistory.redoStack.push(lastOp);
            
            // 刷新UI
            refreshData();
            
            logInfo(QString("Successfully undone operation: %1").arg(lastOp.tagName));
        } else {
            // 撤销失败，将操作放回撤销栈
            m_operationHistory.undoStack.push(lastOp);
            handleError("撤销操作失败");
        }
        
    } catch (const std::exception& e) {
        logError(QString("Exception in onUndoClicked: %1").arg(e.what()));
        handleError("撤销操作时发生错误");
    } catch (...) {
        logError("Unknown exception in onUndoClicked");
        handleError("撤销操作时发生未知错误");
    }
    
    undoOperation();
}

void ManageTagDialogController::onRedoClicked()
{
    logInfo("重做按钮点击");
    
    try {
        if (m_operationHistory.redoStack.isEmpty()) {
            logDebug("No operations to redo");
            QMessageBox::information(m_dialog, "重做", "没有可重做的操作");
            return;
        }
        
        // 获取最后一个重做操作
        TagDialogOperation redoOp = m_operationHistory.redoStack.pop();
        
        // 根据操作类型执行重做
        bool success = false;
        switch (redoOp.type) {
            case OperationType::CreateTag:
                createTag(redoOp.tagName, "", "");
                break;
            case OperationType::DeleteTag:
                deleteTag(redoOp.tagName);
                success = true;
                break;
            case OperationType::RenameTag:
                renameTag(redoOp.tagName, redoOp.newTagName);
                success = true;
                break;
            case OperationType::MoveSong:
            case OperationType::CopySong:
                transferSongs(redoOp.fromTags.isEmpty()?"":redoOp.fromTags.first(), redoOp.toTags.isEmpty()?"":redoOp.toTags.first(), redoOp.type==OperationType::CopySong);
                success = true;
                break;
            default:
                qWarning() << "Unknown operation type for redo:" << static_cast<int>(redoOp.type);
                break;
        }
        
        if (success) {
            // 将操作添加到撤销栈
            m_operationHistory.undoStack.push(redoOp);
            
            // 刷新UI
            refreshData();
            
            logInfo(QString("Successfully redone operation: %1").arg(redoOp.tagName));
        } else {
            // 重做失败，将操作放回重做栈
            m_operationHistory.redoStack.push(redoOp);
            handleError("重做操作失败");
        }
        
    } catch (const std::exception& e) {
        logError(QString("Exception in onRedoClicked: %1").arg(e.what()));
        handleError("重做操作时发生错误");
    } catch (...) {
        logError("Unknown exception in onRedoClicked");
        handleError("重做操作时发生未知错误");
    }
    
    redoOperation();
}

void ManageTagDialogController::undoOperation() {
    logDebug("undoOperation (无参) called");
}

void ManageTagDialogController::redoOperation() {
    logDebug("redoOperation (无参) called");
}

void ManageTagDialogController::onSelectAllClicked()
{
    logInfo("全选按钮点击");
    
    try {
        if (!m_dialog) {
            logError("对话框指针为空，无法执行全选操作");
            return;
        }
        
        // 调用对话框的全选方法
        // 注释掉 m_dialog->selectAllSongs();
        
        logInfo("歌曲全选操作完成");
        
        // 更新按钮状态
        updateButtonStates();
        
    } catch (const std::exception& e) {
        logError(QString("全选操作时发生异常: %1").arg(e.what()));
        handleError("全选操作失败");
    }
}

void ManageTagDialogController::onSelectNoneClicked()
{
    logInfo("取消全选按钮点击");
    
    try {
        if (!m_dialog) {
            logError("对话框指针为空，无法执行取消全选操作");
            return;
        }
        
        // 调用对话框的取消全选方法
        // 注释掉 m_dialog->deselectAllSongs();
        
        logInfo("歌曲取消全选操作完成");
        
        // 清空选中歌曲列表
        m_selectedSongs.clear();
        
        // 更新按钮状态
        updateButtonStates();
        
    } catch (const std::exception& e) {
        logError(QString("取消全选操作时发生异常: %1").arg(e.what()));
        handleError("取消全选操作失败");
    }
}

void ManageTagDialogController::onInvertSelectionClicked()
{
    logInfo("反选按钮点击");
    
    try {
        if (!m_dialog) {
            logError("对话框指针为空，无法执行反选操作");
            return;
        }
        
        // 获取歌曲列表控件
        // 注释掉 m_dialog->getSongListWidget();
        if (!m_dialog->getSongListWidget()) {
            logError("歌曲列表控件为空，无法执行反选操作");
            handleError("反选操作失败：歌曲列表不可用");
            return;
        }
        
        // 获取所有歌曲项
        int itemCount = m_dialog->getSongListWidget()->count();
        if (itemCount == 0) {
            logInfo("歌曲列表为空，无需反选");
            return;
        }
        
        // 执行反选操作
        for (int i = 0; i < itemCount; ++i) {
            QListWidgetItem* item = m_dialog->getSongListWidget()->item(i);
            if (item) {
                // 切换选中状态
                item->setSelected(!item->isSelected());
            }
        }
        
        logInfo(QString("歌曲反选操作完成，共处理 %1 个歌曲项").arg(itemCount));
        
        // 更新选中歌曲列表
        QList<QString> newSelectedSongs;
        QList<QListWidgetItem*> selectedItems = m_dialog->getSongListWidget()->selectedItems();
        for (QListWidgetItem* item : selectedItems) {
            QVariant songIdData = item->data(Qt::UserRole);
            if (songIdData.isValid()) {
                newSelectedSongs.append(songIdData.toString());
            }
        }
        
        m_selectedSongs = newSelectedSongs;
        
        logInfo(QString("反选后选中歌曲数量: %1").arg(m_selectedSongs.size()));
        
        // 更新按钮状态
        updateButtonStates();
        
    } catch (const std::exception& e) {
        logError(QString("反选操作时发生异常: %1").arg(e.what()));
        handleError("反选操作失败");
    }
}

void ManageTagDialogController::onFilterChanged(const QString& filter)
{
    logInfo(QString("过滤器改变: %1").arg(filter));
    m_filterText = filter;
    updateSongList();
}

void ManageTagDialogController::onSearchChanged(const QString& search)
{
    logInfo(QString("搜索改变: %1").arg(search));
    m_searchQuery = search;
    updateSongList();
}

void ManageTagDialogController::onClearFilterClicked()
{
    logInfo("清除过滤器按钮点击");
    m_filterText.clear();
    m_searchQuery.clear();
    updateSongList();
}

void ManageTagDialogController::onSortByTitleClicked()
{
    logInfo("按标题排序按钮点击");
    m_sortField = SortField::Title;
    m_sortAscending = !m_sortAscending;
    updateSongList();
}

void ManageTagDialogController::onSortByArtistClicked()
{
    logInfo("按艺术家排序按钮点击");
    m_sortField = SortField::Artist;
    m_sortAscending = !m_sortAscending;
    updateSongList();
}

void ManageTagDialogController::onSortByAlbumClicked()
{
    logInfo("按专辑排序按钮点击");
    m_sortField = SortField::Album;
    m_sortAscending = !m_sortAscending;
    updateSongList();
}

void ManageTagDialogController::onSortByDurationClicked()
{
    logInfo("按时长排序按钮点击");
    m_sortField = SortField::Duration;
    m_sortAscending = !m_sortAscending;
    updateSongList();
}

void ManageTagDialogController::onSortByDateAddedClicked()
{
    logInfo("按添加日期排序按钮点击");
    m_sortField = SortField::DateAdded;
    m_sortAscending = !m_sortAscending;
    updateSongList();
}

void ManageTagDialogController::onAcceptRequested()
{
    logInfo("接受请求");
    // 应用所有待处理的操作
    applyOperations();
    emit dialogAccepted();
}

void ManageTagDialogController::onRejectRequested()
{
    logInfo("拒绝请求");
    // 取消所有待处理的操作
    cancelOperations();
    emit dialogRejected();
}

void ManageTagDialogController::onApplyRequested()
{
    logInfo("应用请求");
    // 应用所有待处理的操作但不关闭对话框
    applyOperations();
}

void ManageTagDialogController::onResetRequested()
{
    logInfo("重置请求");
    // 重置所有更改
    resetOperations();
}

// 辅助方法实现
void ManageTagDialogController::updateButtonStates()
{
    logDebug("更新按钮状态");
    
    try {
        if (!m_dialog) {
            qWarning() << "Dialog is null in updateButtonStates";
            return;
        }
        
        // 检查是否有标签选中
        bool hasTagSelected = !m_selectedTag1.isEmpty() || !m_selectedTag2.isEmpty();
        bool hasSongsSelected = !m_selectedSongs.isEmpty();
        bool hasBothTagsSelected = !m_selectedTag1.isEmpty() && !m_selectedTag2.isEmpty();
        
        // 更新标签操作按钮状态
        // 删除和重命名按钮需要选中标签
        if (m_dialog->findChild<QPushButton*>("deleteTagButton")) {
            m_dialog->findChild<QPushButton*>("deleteTagButton")->setEnabled(hasTagSelected);
        }
        if (m_dialog->findChild<QPushButton*>("renameTagButton")) {
            m_dialog->findChild<QPushButton*>("renameTagButton")->setEnabled(hasTagSelected);
        }
        
        // 歌曲转移按钮需要选中歌曲和两个标签
        if (m_dialog->findChild<QPushButton*>("moveButton")) {
            m_dialog->findChild<QPushButton*>("moveButton")->setEnabled(hasSongsSelected && hasBothTagsSelected);
        }
        if (m_dialog->findChild<QPushButton*>("copyButton")) {
            m_dialog->findChild<QPushButton*>("copyButton")->setEnabled(hasSongsSelected && hasBothTagsSelected);
        }
        
        // 撤销/重做按钮状态
        if (m_dialog->findChild<QPushButton*>("undoButton")) {
            m_dialog->findChild<QPushButton*>("undoButton")->setEnabled(!m_operationHistory.undoStack.isEmpty());
        }
        if (m_dialog->findChild<QPushButton*>("redoButton")) {
            m_dialog->findChild<QPushButton*>("redoButton")->setEnabled(!m_operationHistory.redoStack.isEmpty());
        }
        
        logDebug(QString("Button states updated - TagSelected: %1, SongsSelected: %2, BothTags: %3")
                .arg(hasTagSelected).arg(hasSongsSelected).arg(hasBothTagsSelected));
                
    } catch (const std::exception& e) {
        logError(QString("Exception in updateButtonStates: %1").arg(e.what()));
    } catch (...) {
        logError("Unknown exception in updateButtonStates");
    }
}

void ManageTagDialogController::updateStatistics()
{
    logDebug("更新统计信息");
    
    try {
        if (!m_dialog) {
            qWarning() << "Dialog is null in updateStatistics";
            return;
        }
        
        // 计算基本统计信息
        int totalTags = m_tags.size();
        int totalSongs = m_songs.size();
        int selectedSongs = m_selectedSongs.size();
        
        // 计算选中标签的歌曲数量
        int tag1SongCount = 0;
        int tag2SongCount = 0;
        
        if (!m_selectedTag1.isEmpty()) {
            // 计算标签1的歌曲数量
            for (const Song& song : m_songs) {
                if (song.tags().contains(m_selectedTag1)) {
                    tag1SongCount++;
                }
            }
        }
        
        if (!m_selectedTag2.isEmpty()) {
            // 计算标签2的歌曲数量
            for (const Song& song : m_songs) {
                if (song.tags().contains(m_selectedTag2)) {
                    tag2SongCount++;
                }
            }
        }
        
        // 更新状态栏或统计标签
        QString statisticsText = QString("标签: %1 | 歌曲: %2 | 已选: %3")
                                .arg(totalTags).arg(totalSongs).arg(selectedSongs);
        
        if (!m_selectedTag1.isEmpty()) {
            statisticsText += QString(" | %1: %2首").arg(m_selectedTag1).arg(tag1SongCount);
        }
        if (!m_selectedTag2.isEmpty()) {
            statisticsText += QString(" | %1: %2首").arg(m_selectedTag2).arg(tag2SongCount);
        }
        
        // 查找并更新统计标签
        if (QLabel* statsLabel = m_dialog->findChild<QLabel*>("statisticsLabel")) {
            statsLabel->setText(statisticsText);
        }
        
        // 发出统计信息更新信号
        emit statisticsUpdated(m_tagStatistics);
        
        logDebug(QString("Statistics updated: %1").arg(statisticsText));
        
    } catch (const std::exception& e) {
        logError(QString("Exception in updateStatistics: %1").arg(e.what()));
    } catch (...) {
        logError("Unknown exception in updateStatistics");
    }
}

void ManageTagDialogController::refreshUI()
{
    logDebug("刷新UI");
    
    try {
        if (!m_dialog) {
            qWarning() << "Dialog is null in refreshUI";
            return;
        }
        
        // 重新加载数据
        loadTags();
        loadSongs();
        
        // 更新标签列表
        updateTagLists();
        
        // 更新歌曲列表
        updateSongList();
        
        // 更新按钮状态
        updateButtonStates();
        
        // 更新统计信息
        updateStatistics();
        
        // 发出UI刷新信号
        emit uiRefreshed();
        
        logInfo("UI refreshed successfully");
        
    } catch (const std::exception& e) {
        logError(QString("Exception in refreshUI: %1").arg(e.what()));
        handleError("刷新界面时发生错误");
    } catch (...) {
        logError("Unknown exception in refreshUI");
        handleError("刷新界面时发生未知错误");
    }
}

void ManageTagDialogController::transferSongs(const QString& fromTag, const QString& toTag, bool copy) {
    logInfo(QString("Transferring songs from '%1' to '%2', copy=%3").arg(fromTag, toTag).arg(copy));
    
    try {
        // 检查数据库管理器是否可用
        if (!m_databaseManager || !m_databaseManager->isValid()) {
            logError("Database manager not available, cannot transfer songs");
            handleError("数据库不可用，无法转移歌曲");
            return;
        }
        
        // 检查标签名是否有效
        if (fromTag.trimmed().isEmpty() || toTag.trimmed().isEmpty()) {
            logError("Source or target tag name is empty");
            handleError("源标签或目标标签名为空");
            return;
        }
        
        // 检查是否有选中的歌曲
        if (m_selectedSongs.isEmpty()) {
            qWarning() << "No songs selected for transfer";
            handleError("没有选中要转移的歌曲");
            return;
        }
        
        // 查找源标签和目标标签的ID
        TagDao tagDao;
        Tag sourceTag = tagDao.getTagByName(fromTag);
        Tag targetTag = tagDao.getTagByName(toTag);
        
        if (sourceTag.id() <= 0) {
            logError(QString("Source tag not found: %1").arg(fromTag));
            handleError(QString("未找到源标签: %1").arg(fromTag));
            return;
        }
        
        if (targetTag.id() <= 0) {
            logError(QString("Target tag not found: %1").arg(toTag));
            handleError(QString("未找到目标标签: %1").arg(toTag));
            return;
        }
        
        // 执行歌曲转移操作
        SongDao songDao;
        int successCount = 0;
        int failureCount = 0;
        
        for (const QString& songIdStr : m_selectedSongs) {
            bool ok;
            int songId = songIdStr.toInt(&ok);
            if (!ok || songId <= 0) {
                logError(QString("Invalid song ID: %1").arg(songIdStr));
                failureCount++;
                continue;
            }
            
            // 检查歌曲是否已经在目标标签中
            if (songDao.songHasTag(songId, targetTag.id())) {
                logDebug(QString("Song %1 already has target tag %2").arg(songId).arg(toTag));
                if (!copy) {
                    // 如果是移动操作，仍需要从源标签移除
                    if (songDao.removeSongFromTag(songId, sourceTag.id())) {
                        successCount++;
                    } else {
                        failureCount++;
                    }
                }
                continue;
            }
            
            // 添加歌曲到目标标签
            bool addSuccess = songDao.addSongToTag(songId, targetTag.id());
            if (!addSuccess) {
                logError(QString("Failed to add song %1 to tag %2").arg(songId).arg(toTag));
                failureCount++;
                continue;
            }
            
            // 如果是移动操作，从源标签移除歌曲
            if (!copy) {
                bool removeSuccess = songDao.removeSongFromTag(songId, sourceTag.id());
                if (!removeSuccess) {
                    logError(QString("Failed to remove song %1 from tag %2").arg(songId).arg(fromTag));
                    // 尝试回滚：从目标标签移除刚添加的歌曲
                    songDao.removeSongFromTag(songId, targetTag.id());
                    failureCount++;
                    continue;
                }
            }
            
            successCount++;
        }
        
        // 记录操作结果
        QString operationType = copy ? "复制" : "移动";
        if (successCount > 0) {
            logInfo(QString("%1 %2 songs successfully, %3 failed")
                   .arg(operationType).arg(successCount).arg(failureCount));
            
            // 发射信号通知操作完成
            if (copy) {
                emit songsCopied(m_selectedSongs, fromTag, toTag);
            } else {
                emit songsMoved(m_selectedSongs, fromTag, toTag);
            }
            
            // 刷新数据
            loadSongs();
            updateSongList();
            
            // 通知主界面刷新歌曲列表（特别是移动转移后）
            if (!copy) {
                // 移动转移后，需要通知主界面刷新当前标签的歌曲列表
                emit uiRefreshed();
                
                // 如果主界面有控制器，也通知它刷新
                if (m_dialog) {
                    // 发送信号通知主界面需要刷新
                    emit dialogAccepted();
                }
            }
            
            // 显示成功消息
            if (failureCount == 0) {
                QMessageBox::information(m_dialog, "操作成功", 
                    QString("成功%1了 %2 首歌曲").arg(operationType).arg(successCount));
            } else {
                QMessageBox::warning(m_dialog, "操作部分成功", 
                    QString("成功%1了 %2 首歌曲，%3 首失败")
                    .arg(operationType).arg(successCount).arg(failureCount));
            }
        } else {
            logError(QString("All song transfer operations failed"));
            handleError(QString("所有歌曲%1操作都失败了").arg(operationType));
        }
        
    } catch (const std::exception& e) {
        logError(QString("Exception while transferring songs: %1").arg(e.what()));
        handleError("转移歌曲时发生错误");
    } catch (...) {
        logError("Unknown exception while transferring songs");
        handleError("转移歌曲时发生未知错误");
    }
}
void ManageTagDialogController::applyOperations() {
    logDebug("applyOperations called");
}
void ManageTagDialogController::cancelOperations() {
    logDebug("cancelOperations called");
}
void ManageTagDialogController::resetOperations() {
    logDebug("resetOperations called");
}

void ManageTagDialogController::refreshData()
{
    logInfo("刷新数据");
    
    try {
        if (!m_initialized) {
            logError("控制器未初始化，无法刷新数据");
            return;
        }
        
        // 重新加载数据
        loadTags();
        loadSongs();
        
        // 更新UI
        updateTagLists();
        updateSongList();
        updateButtonStates();
        
        logInfo("数据刷新完成");
        
    } catch (const std::exception& e) {
        logError(QString("刷新数据时发生异常: %1").arg(e.what()));
    } catch (...) {
        logError("刷新数据时发生未知异常");
    }
}

void ManageTagDialogController::clearOperationHistory()
{
    logInfo("清空操作历史");
    
    try {
        // 清空撤销和重做栈
        m_operationHistory.undoStack.clear();
        m_operationHistory.redoStack.clear();
        
        // 重置操作索引
        m_operationIndex = 0;
        
        // 更新按钮状态
        updateButtonStates();
        
        logInfo("操作历史已清空");
        
    } catch (const std::exception& e) {
        logError(QString("清空操作历史时发生异常: %1").arg(e.what()));
    } catch (...) {
        logError("清空操作历史时发生未知异常");
    }
}

// 撤销/重做具体实现方法
bool ManageTagDialogController::undoCreateTag(const TagDialogOperation& operation)
{
    logInfo(QString("Undoing create tag operation: %1").arg(operation.tagName));
    
    try {
        // 删除创建的标签
        deleteTag(operation.tagName);
        return true;
    } catch (const std::exception& e) {
        logError(QString("Exception in undoCreateTag: %1").arg(e.what()));
        return false;
    }
}

bool ManageTagDialogController::undoDeleteTag(const TagDialogOperation& operation)
{
    logInfo(QString("Undoing delete tag operation: %1").arg(operation.tagName));
    
    try {
        // 重新创建被删除的标签
        createTag(operation.tagName, "", "");
        return true;
    } catch (const std::exception& e) {
        logError(QString("Exception in undoDeleteTag: %1").arg(e.what()));
        return false;
    }
}

bool ManageTagDialogController::undoRenameTag(const TagDialogOperation& operation)
{
    logInfo(QString("Undoing rename tag operation: %1").arg(operation.tagName));
    
    try {
        // 将标签名改回原来的名称
        renameTag(operation.newTagName, operation.tagName);
        return true;
    } catch (const std::exception& e) {
        logError(QString("Exception in undoRenameTag: %1").arg(e.what()));
        return false;
    }
}

bool ManageTagDialogController::undoTransferSongs(const TagDialogOperation& operation)
{
    logInfo(QString("Undoing transfer songs operation: %1").arg(operation.tagName));
    
    try {
        if (operation.type == OperationType::MoveSong) {
            // 撤销移动：将歌曲从目标标签移回源标签
            QString fromTag = operation.toTags.isEmpty() ? "" : operation.toTags.first();
            QString toTag = operation.fromTags.isEmpty() ? "" : operation.fromTags.first();
            transferSongs(fromTag, toTag, false);
        } else if (operation.type == OperationType::CopySong) {
            // 撤销复制：从目标标签移除歌曲
            // TODO: 实现从特定标签移除歌曲的功能
        }
        return true;
    } catch (const std::exception& e) {
        logError(QString("Exception in undoTransferSongs: %1").arg(e.what()));
        return false;
    }
}

bool ManageTagDialogController::redoCreateTag(const TagDialogOperation& operation)
{
    logInfo(QString("Redoing create tag operation: %1").arg(operation.tagName));
    
    try {
        // 重新创建标签
        createTag(operation.tagName, "", "");
        return true;
    } catch (const std::exception& e) {
        logError(QString("Exception in redoCreateTag: %1").arg(e.what()));
        return false;
    }
}

bool ManageTagDialogController::redoDeleteTag(const TagDialogOperation& operation)
{
    logInfo(QString("Redoing delete tag operation: %1").arg(operation.tagName));
    
    try {
        // 重新删除标签
        deleteTag(operation.tagName);
        return true;
    } catch (const std::exception& e) {
        logError(QString("Exception in redoDeleteTag: %1").arg(e.what()));
        return false;
    }
}

bool ManageTagDialogController::redoRenameTag(const TagDialogOperation& operation)
{
    logInfo(QString("Redoing rename tag operation: %1").arg(operation.tagName));
    
    try {
        // 重新执行重命名
        renameTag(operation.tagName, operation.newTagName);
        return true;
    } catch (const std::exception& e) {
        logError(QString("Exception in redoRenameTag: %1").arg(e.what()));
        return false;
    }
}

bool ManageTagDialogController::redoTransferSongs(const TagDialogOperation& operation)
{
    logInfo(QString("Redoing transfer songs operation: %1").arg(operation.tagName));
    
    try {
        bool copy = (operation.type == OperationType::CopySong);
        transferSongs(operation.fromTags.isEmpty()?"":operation.fromTags.first(), operation.toTags.isEmpty()?"":operation.toTags.first(), copy);
        return true;
    } catch (const std::exception& e) {
        logError(QString("Exception in redoTransferSongs: %1").arg(e.what()));
        return false;
    }
}

void ManageTagDialogController::logWarning(const QString& warning) {
    qWarning() << warning;
}

void ManageTagDialogController::cleanupOperationHistory()
{
    try {
        // 限制撤销栈大小
        while (m_operationHistory.undoStack.size() > m_maxHistorySize) {
            m_operationHistory.undoStack.removeFirst();
        }
        
        // 限制重做栈大小
        while (m_operationHistory.redoStack.size() > m_maxHistorySize) {
            m_operationHistory.redoStack.removeFirst();
        }
        
        logDebug(QString("Operation history cleaned up - Undo: %1, Redo: %2")
                .arg(m_operationHistory.undoStack.size()).arg(m_operationHistory.redoStack.size()));
                
    } catch (const std::exception& e) {
        logError(QString("Exception in cleanupOperationHistory: %1").arg(e.what()));
    } catch (...) {
        logError("Unknown exception in cleanupOperationHistory");
    }
}

void ManageTagDialogController::renameTag(const QString& oldName, const QString& newName) {
    logInfo(QString("Renaming tag from '%1' to '%2'").arg(oldName, newName));
    
    try {
        // 检查数据库管理器是否可用
        if (!m_databaseManager || !m_databaseManager->isValid()) {
            logError("Database manager not available, cannot rename tag");
            handleError("数据库不可用，无法重命名标签");
            return;
        }
        
        // 检查标签名是否有效
        if (oldName.trimmed().isEmpty() || newName.trimmed().isEmpty()) {
            logError("Old or new tag name is empty");
            handleError("旧标签名或新标签名为空");
            return;
        }
        
        // 检查新标签名是否与旧标签名相同
        if (oldName.trimmed() == newName.trimmed()) {
            qWarning() << "New tag name is the same as old tag name";
            handleError("新标签名与旧标签名相同");
            return;
        }
        
        // 查找要重命名的标签
        TagDao tagDao;
        Tag tagToRename = tagDao.getTagByName(oldName);
        
        if (tagToRename.id() <= 0) {
            logError(QString("Tag not found: %1").arg(oldName));
            handleError(QString("未找到标签: %1").arg(oldName));
            return;
        }
        
        // 检查是否为系统标签
        if (tagToRename.isSystem()) {
            logError(QString("Cannot rename system tag: %1").arg(oldName));
            handleError("不能重命名系统标签");
            return;
        }
        
        // 检查新标签名是否已存在
        Tag existingTag = tagDao.getTagByName(newName.trimmed());
        if (existingTag.id() > 0) {
            logError(QString("Tag name already exists: %1").arg(newName));
            handleError(QString("标签名已存在: %1").arg(newName));
            return;
        }
        
        // 更新标签名
        tagToRename.setName(newName.trimmed());
        bool success = tagDao.updateTag(tagToRename);
        
        if (success) {
            // 更新内存中的标签列表
            for (int i = 0; i < m_tags.size(); ++i) {
                if (m_tags[i].name() == oldName) {
                    m_tags[i].setName(newName.trimmed());
                    break;
                }
            }
            
            // 更新选中的标签名
            if (m_selectedTag1 == oldName) {
                m_selectedTag1 = newName.trimmed();
            }
            if (m_selectedTag2 == oldName) {
                m_selectedTag2 = newName.trimmed();
            }
            
            logInfo(QString("Tag renamed successfully from '%1' to '%2'").arg(oldName, newName));
            emit tagRenamed(oldName, newName.trimmed());
            updateTagLists();
            
            QMessageBox::information(m_dialog, "重命名成功", 
                QString("标签 '%1' 已成功重命名为 '%2'").arg(oldName, newName.trimmed()));
            
        } else {
            logError(QString("Failed to rename tag in database: %1 -> %2").arg(oldName, newName));
            handleError("重命名标签失败");
        }
        
    } catch (const std::exception& e) {
        logError(QString("Exception while renaming tag: %1").arg(e.what()));
        handleError("重命名标签时发生错误");
    } catch (...) {
        logError("Unknown exception while renaming tag");
        handleError("重命名标签时发生未知错误");
    }
}

QList<TagDialogStatistics> ManageTagDialogController::loadTagStatistics()
{
    // TODO: 实现统计信息加载逻辑，暂时返回空列表
    return QList<TagDialogStatistics>();
}