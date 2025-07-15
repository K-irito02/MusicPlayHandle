#include "ManageTagDialog.h"
#include "ui_ManageTagDialog.h"
#include "../controllers/managetagdialogcontroller.h"
#include "../widgets/taglistitem.h"
#include "../../database/tagdao.h"
#include "../../database/songdao.h"
#include "../../models/tag.h"
#include "../../models/song.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QSqlDatabase>
#include <QTimer>
#include <QStatusBar>
#include <QApplication>
#include "../../database/databasemanager.h"

ManageTagDialog::ManageTagDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::ManageTagDialog)
{
    ui->setupUi(this);
    // 使用 UI 设计器生成的控件
    tagListSource = ui->listWidget_tag_list1;
    songList = ui->listWidget_song_list;
    tagListTarget = ui->listWidget_tag_list2;
    btnCopy = ui->pushButton_copy_transfer;
    btnMove = ui->pushButton_move_transfer;
    btnUndo = ui->pushButton_undo;
    btnExitNoSave = ui->pushButton_exit_discard;
    btnExitError = nullptr; // 若无报错按钮可置空
    // 信号槽
    connect(tagListSource, &QListWidget::itemSelectionChanged, this, &ManageTagDialog::onSourceTagSelectionChanged);
    connect(songList, &QListWidget::itemSelectionChanged, this, &ManageTagDialog::onSongSelectionChanged);
    connect(tagListTarget, &QListWidget::itemSelectionChanged, this, &ManageTagDialog::onTargetTagSelectionChanged);
    connect(btnCopy, &QPushButton::clicked, this, &ManageTagDialog::onCopySongs);
    connect(btnMove, &QPushButton::clicked, this, &ManageTagDialog::onMoveSongs);
    connect(btnUndo, &QPushButton::clicked, this, &ManageTagDialog::onUndo);
    connect(btnExitNoSave, &QPushButton::clicked, this, &ManageTagDialog::onExitNoSave);
    connect(ui->pushButton_select_all, &QPushButton::clicked, this, &ManageTagDialog::onSelectAllSongs);
    connect(ui->pushButton_deselect_all, &QPushButton::clicked, this, &ManageTagDialog::onDeselectAllSongs);
    // TODO: 若有报错按钮，补充 connect(btnExitError, ...)
    // TODO: 加载标签和歌曲数据，初始化状态
}

ManageTagDialog::~ManageTagDialog()
{
    if (m_controller) {
        m_controller->shutdown();
        delete m_controller;
    }
    delete ui;
}

void ManageTagDialog::setTagsAndSongs(const QStringList& tags, const QStringList& /*songs*/)
{
    m_originalTags = tags;
    // 设置标签和歌曲数据
    if (m_controller) {
        m_controller->loadTags();
        m_controller->loadSongs();
    }
}

QStringList ManageTagDialog::getModifiedTags() const
{
    return m_modifiedTags;
}

QStringList ManageTagDialog::getDeletedTags() const
{
    return m_deletedTags;
}

QList<ManageTagDialog::SongMove> ManageTagDialog::getSongMoves() const
{
    return m_songMoves;
}

QListWidget* ManageTagDialog::getSongListWidget()
{
    return songList;
}

void ManageTagDialog::setupConnections()
{
    // 设置信号连接
    // 这里需要根据UI文件中的控件名称来连接
}

void ManageTagDialog::setupUI()
{
    // 设置UI初始状态
    setWindowTitle("管理标签");
    setModal(true);
    
    updateButtonStates();
}

void ManageTagDialog::updateButtonStates()
{
    // 更新按钮状态
}

void ManageTagDialog::updateTagLists()
{
    // 更新标签列表
}

void ManageTagDialog::updateSongList()
{
    // 更新歌曲列表
}

void ManageTagDialog::loadSongsForTag(const QString& tag)
{
    qDebug() << "ManageTagDialog::loadSongsForTag called with tag:" << tag;
    
    if (tag.isEmpty()) {
        qDebug() << "Tag name is empty, clearing song list";
        songList->clear();
        selectedSongIds.clear();
        updateButtonStates();
        return;
    }
    
    try {
        // 清空当前歌曲列表
        songList->clear();
        selectedSongIds.clear();
        
        // 通过控制器加载指定标签的歌曲
        if (m_controller) {
            qDebug() << "Loading songs for tag through controller:" << tag;
            
            // 获取标签下的所有歌曲
            SongDao songDao;
            TagDao tagDao;
            
            // 首先获取标签ID
            Tag tagInfo = tagDao.getTagByName(tag);
            if (tagInfo.id() <= 0) {
                qDebug() << "Tag not found:" << tag;
                showStatusMessage(QString("标签 '%1' 不存在").arg(tag));
                return;
            }
            
            // 获取该标签下的所有歌曲
            QList<Song> songs = songDao.getSongsByTag(tagInfo.id());
            qDebug() << "Found" << songs.size() << "songs for tag:" << tag;
            
            // 添加歌曲到列表
            for (const Song& song : songs) {
                QListWidgetItem* item = new QListWidgetItem();
                
                // 设置显示文本
                QString displayText = QString("%1 - %2")
                    .arg(song.artist().isEmpty() ? "未知艺术家" : song.artist())
                    .arg(song.title().isEmpty() ? "未知标题" : song.title());
                
                if (!song.album().isEmpty()) {
                    displayText += QString(" [%1]").arg(song.album());
                }
                
                item->setText(displayText);
                item->setData(Qt::UserRole, song.id());
                item->setToolTip(QString("文件路径: %1\n时长: %2")
                    .arg(song.filePath())
                    .arg(formatDuration(song.duration())));
                
                songList->addItem(item);
            }
            
            // 更新状态消息
            showStatusMessage(QString("已加载标签 '%1' 下的 %2 首歌曲").arg(tag).arg(songs.size()));
            
        } else {
            qDebug() << "Controller is null, cannot load songs";
            showStatusMessage("控制器未初始化，无法加载歌曲");
        }
        
        // 更新按钮状态
        updateButtonStates();
        
    } catch (const std::exception& e) {
        qDebug() << "Exception in loadSongsForTag:" << e.what();
        showStatusMessage(QString("加载歌曲时发生错误: %1").arg(e.what()));
    } catch (...) {
        qDebug() << "Unknown exception in loadSongsForTag";
        showStatusMessage("加载歌曲时发生未知错误");
    }
}

// 辅助方法：格式化时长
QString ManageTagDialog::formatDuration(qint64 duration) const
{
    if (duration <= 0) {
        return "00:00";
    }
    
    int seconds = static_cast<int>(duration / 1000);
    int minutes = seconds / 60;
    seconds = seconds % 60;
    
    if (minutes >= 60) {
        int hours = minutes / 60;
        minutes = minutes % 60;
        return QString("%1:%2:%3")
            .arg(hours)
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'));
    } else {
        return QString("%1:%2")
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'));
    }
}

void ManageTagDialog::performTransfer(bool isCopy)
{
    qDebug() << "ManageTagDialog::performTransfer called, isCopy:" << isCopy;
    
    // 检查是否有选中的源标签、歌曲和目标标签
    if (selectedSourceTagIds.isEmpty()) {
        showStatusMessage("请先选择源标签");
        QMessageBox::warning(this, "操作无效", "请先选择源标签！");
        return;
    }
    
    if (selectedSongIds.isEmpty()) {
        showStatusMessage("请先选择要转移的歌曲");
        QMessageBox::warning(this, "操作无效", "请先选择要转移的歌曲！");
        return;
    }
    
    if (selectedTargetTagIds.isEmpty()) {
        showStatusMessage("请先选择目标标签");
        QMessageBox::warning(this, "操作无效", "请先选择目标标签！");
        return;
    }
    
    try {
        // 获取操作描述
        QString operationType = isCopy ? "复制" : "移动";
        int songCount = selectedSongIds.size();
        int sourceTagCount = selectedSourceTagIds.size();
        int targetTagCount = selectedTargetTagIds.size();
        
        qDebug() << QString("Performing %1 operation: %2 songs from %3 source tags to %4 target tags")
                    .arg(operationType).arg(songCount).arg(sourceTagCount).arg(targetTagCount);
        
        // 确认操作
        QString confirmMessage = QString("确定要%1 %2 首歌曲从 %3 个源标签到 %4 个目标标签吗？")
                                .arg(operationType)
                                .arg(songCount)
                                .arg(sourceTagCount)
                                .arg(targetTagCount);
        
        if (!isCopy) {
            confirmMessage += "\n\n注意：移动操作将从源标签中移除这些歌曲！";
        }
        
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, 
            QString("%1歌曲确认").arg(operationType),
            confirmMessage,
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
        );
        
        if (reply != QMessageBox::Yes) {
            showStatusMessage(QString("已取消%1操作").arg(operationType));
            return;
        }
        
        // 记录操作到历史栈（用于撤销）
        Operation::Type opType = isCopy ? Operation::Copy : Operation::Move;
        recordOperation(opType, selectedSongIds.values(), selectedSourceTagIds, selectedTargetTagIds);
        
        // 执行实际的转移操作
        bool success = true;
        int successCount = 0;
        int failureCount = 0;
        
        if (m_controller) {
            // 通过控制器执行转移操作
            for (int sourceTagId : selectedSourceTagIds) {
                for (int targetTagId : selectedTargetTagIds) {
                    try {
                        // 获取标签名称
                        TagDao tagDao;
                        Tag sourceTag = tagDao.getTagById(sourceTagId);
                        Tag targetTag = tagDao.getTagById(targetTagId);
                        
                        if (sourceTag.id() <= 0 || targetTag.id() <= 0) {
                            qDebug() << "Invalid tag ID found";
                            failureCount++;
                            continue;
                        }
                        
                        // 调用控制器的转移方法
                        // 需要通过公共接口调用transfer操作
m_controller->transferSongs(sourceTag.name(), targetTag.name(), isCopy);
                        successCount++;
                        
                        qDebug() << QString("Successfully %1 songs from '%2' to '%3'")
                                    .arg(isCopy ? "copied" : "moved")
                                    .arg(sourceTag.name())
                                    .arg(targetTag.name());
                        
                    } catch (const std::exception& e) {
                        qDebug() << "Exception during transfer:" << e.what();
                        failureCount++;
                        success = false;
                    }
                }
            }
        } else {
            qDebug() << "Controller is null, cannot perform transfer";
            showStatusMessage("控制器未初始化，无法执行转移操作");
            return;
        }
        
        // 更新状态消息
        if (success && failureCount == 0) {
            showStatusMessage(QString("成功%1 %2 首歌曲").arg(operationType).arg(songCount));
            QMessageBox::information(this, "操作成功", 
                QString("成功%1 %2 首歌曲到目标标签").arg(operationType).arg(songCount));
        } else {
            QString resultMessage = QString("%1操作完成：成功 %2 次，失败 %3 次")
                                   .arg(operationType).arg(successCount).arg(failureCount);
            showStatusMessage(resultMessage);
            
            if (failureCount > 0) {
                QMessageBox::warning(this, "操作部分失败", resultMessage);
            }
        }
        
        // 刷新界面
        if (m_controller) {
            m_controller->refreshData();
        }
        
        // 清空选择
        selectedSongIds.clear();
        songList->clearSelection();
        
        // 更新按钮状态
        updateButtonStates();
        
    } catch (const std::exception& e) {
        qDebug() << "Exception in performTransfer:" << e.what();
        showStatusMessage(QString("转移操作时发生错误: %1").arg(e.what()));
        QMessageBox::critical(this, "操作错误", QString("转移操作时发生错误:\n%1").arg(e.what()));
    } catch (...) {
        qDebug() << "Unknown exception in performTransfer";
        showStatusMessage("转移操作时发生未知错误");
        QMessageBox::critical(this, "操作错误", "转移操作时发生未知错误");
    }
}

void ManageTagDialog::showStatusMessage(const QString& message)
{
    qDebug() << "ManageTagDialog::showStatusMessage:" << message;
    
    if (message.isEmpty()) {
        return;
    }
    
    try {
        // 查找状态栏或状态标签
        QLabel* statusLabel = findChild<QLabel*>("label_status");
        if (!statusLabel) {
            // 如果没有专门的状态标签，尝试查找其他可用的标签
            statusLabel = findChild<QLabel*>("label_song_list");
        }
        
        if (statusLabel) {
            // 保存原始文本（如果是第一次设置状态消息）
            QString originalText = statusLabel->text();
            if (originalText.isEmpty() && !statusLabel->text().contains("状态:")) {
                originalText = statusLabel->text();
            }
            
            // 设置状态消息
            QString statusText = QString("状态: %1").arg(message);
            statusLabel->setText(statusText);
            statusLabel->setStyleSheet("QLabel { color: #00ff00; font-weight: bold; }");
            
            qDebug() << "Status message displayed in label:" << message;
            
            // 创建定时器，3秒后恢复原始文本
            QTimer* timer = new QTimer(this);
            timer->setSingleShot(true);
            timer->setInterval(3000); // 3秒
            
            connect(timer, &QTimer::timeout, [statusLabel, originalText, timer]() {
                if (statusLabel && !originalText.isEmpty()) {
                    statusLabel->setText(originalText);
                    statusLabel->setStyleSheet(""); // 恢复默认样式
                }
                timer->deleteLater();
            });
            
            timer->start();
            
        } else {
            // 如果找不到状态标签，使用窗口标题显示状态
            QString originalTitle = windowTitle();
            if (!originalTitle.contains(" - ")) {
                setWindowTitle(QString("%1 - %2").arg(originalTitle).arg(message));
                
                // 创建定时器，3秒后恢复原始标题
                QTimer* timer = new QTimer(this);
                timer->setSingleShot(true);
                timer->setInterval(3000);
                
                connect(timer, &QTimer::timeout, [this, originalTitle, timer]() {
                    setWindowTitle(originalTitle);
                    timer->deleteLater();
                });
                
                timer->start();
                
                qDebug() << "Status message displayed in window title:" << message;
            }
        }
        
        // 同时在应用程序状态栏显示消息（如果存在）
        if (QApplication::instance()) {
            QWidget* mainWindow = nullptr;
            for (QWidget* widget : QApplication::topLevelWidgets()) {
                if (widget->objectName() == "MainWindow" || 
                    widget->inherits("QMainWindow")) {
                    mainWindow = widget;
                    break;
                }
            }
            
            if (mainWindow) {
                QStatusBar* statusBar = mainWindow->findChild<QStatusBar*>();
                if (statusBar) {
                    statusBar->showMessage(message, 3000); // 显示3秒
                    qDebug() << "Status message displayed in main window status bar:" << message;
                }
            }
        }
        
        // 强制刷新界面
        QApplication::processEvents();
        
    } catch (const std::exception& e) {
        qDebug() << "Exception in showStatusMessage:" << e.what();
        // 如果出现异常，至少在调试输出中显示消息
        qDebug() << "Fallback status message:" << message;
    } catch (...) {
        qDebug() << "Unknown exception in showStatusMessage";
        qDebug() << "Fallback status message:" << message;
    }
}

QString ManageTagDialog::getSelectedTag1() const
{
    // 获取选中的标签1
    return QString();
}

QString ManageTagDialog::getSelectedTag2() const
{
    // 获取选中的标签2
    return QString();
}

QStringList ManageTagDialog::getSelectedSongs() const
{
    // 获取选中的歌曲
    return QStringList();
}

// 槽函数实现
void ManageTagDialog::onSourceTagSelectionChanged()
{
    qDebug() << "ManageTagDialog::onSourceTagSelectionChanged called";
    
    // 获取选中的源标签ID
    selectedSourceTagIds.clear();
    QList<QListWidgetItem*> selected = tagListSource->selectedItems();
    for (QListWidgetItem* item : selected) {
        selectedSourceTagIds.insert(item->data(Qt::UserRole).toInt());
        
        // 更新TagListItem的选中状态
        TagListItem* tagWidget = qobject_cast<TagListItem*>(tagListSource->itemWidget(item));
        if (tagWidget) {
            tagWidget->setSelected(true);
        }
    }
    
    // 取消未选中项的选中状态
    for (int i = 0; i < tagListSource->count(); ++i) {
        QListWidgetItem* item = tagListSource->item(i);
        if (!selected.contains(item)) {
            TagListItem* tagWidget = qobject_cast<TagListItem*>(tagListSource->itemWidget(item));
            if (tagWidget) {
                tagWidget->setSelected(false);
            }
        }
    }
    
    // 根据选中的标签加载歌曲
    if (!selected.isEmpty()) {
        // 如果选中了标签，加载第一个选中标签的歌曲
        QListWidgetItem* firstSelected = selected.first();
        TagListItem* tagWidget = qobject_cast<TagListItem*>(tagListSource->itemWidget(firstSelected));
        QString tagName;
        if (tagWidget) {
            tagName = tagWidget->getTagName();
        } else {
            // 如果没有自定义控件，使用item的文本
            tagName = firstSelected->text();
        }
        
        qDebug() << "Loading songs for selected tag:" << tagName;
        loadSongsForTag(tagName);
        
        // 显示状态消息
        showStatusMessage(QString("已选择源标签: %1").arg(tagName));
    } else {
        // 如果没有选中标签，清空歌曲列表
        qDebug() << "No source tag selected, clearing song list";
        songList->clear();
        selectedSongIds.clear();
        showStatusMessage("请选择源标签以查看歌曲");
    }
    
    // 更新按钮状态
    updateButtonStates();
}

void ManageTagDialog::onSongSelectionChanged()
{
    selectedSongIds.clear();
    QList<QListWidgetItem*> selected = songList->selectedItems();
    for (QListWidgetItem* item : selected) {
        selectedSongIds.insert(item->data(Qt::UserRole).toInt());
    }
}

void ManageTagDialog::onTargetTagSelectionChanged()
{
    selectedTargetTagIds.clear();
    QList<QListWidgetItem*> selected = tagListTarget->selectedItems();
    for (QListWidgetItem* item : selected) {
        selectedTargetTagIds.insert(item->data(Qt::UserRole).toInt());
        
        // 更新TagListItem的选中状态
        TagListItem* tagWidget = qobject_cast<TagListItem*>(tagListTarget->itemWidget(item));
        if (tagWidget) {
            tagWidget->setSelected(true);
        }
    }
    
    // 取消未选中项的选中状态
    for (int i = 0; i < tagListTarget->count(); ++i) {
        QListWidgetItem* item = tagListTarget->item(i);
        if (!selected.contains(item)) {
            TagListItem* tagWidget = qobject_cast<TagListItem*>(tagListTarget->itemWidget(item));
            if (tagWidget) {
                tagWidget->setSelected(false);
            }
        }
    }
}

// 初始化时加载标签到两个标签列表
void ManageTagDialog::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    tagListSource->clear();
    tagListTarget->clear();
    
    // 使用正确的TagDao构造方式
    TagDao tagDao;
    QList<Tag> tags = tagDao.getAllTags();
    
    // 歌曲标签列表（被转移）：包含所有标签（包括"我的歌曲"）
    for (const Tag& tag : tags) {
        // 创建自定义标签项控件
        TagListItem* tagWidget = new TagListItem(tag.name(), tag.coverPath(), true, true);
        
        // 连接编辑信号
        connect(tagWidget, &TagListItem::editRequested, this, [this, tag](const QString& tagName) {
            // TODO: 实现标签编辑功能
            Q_UNUSED(tagName)
        });
        
        // 创建列表项并设置自定义控件
        QListWidgetItem* item1 = new QListWidgetItem();
        item1->setData(Qt::UserRole, tag.id());
        item1->setSizeHint(tagWidget->sizeHint());
        tagListSource->addItem(item1);
        tagListSource->setItemWidget(item1, tagWidget);
    }
    
    // 歌曲标签列表（转移到）：只包含用户创建的标签（不包含"我的歌曲"）
    for (const Tag& tag : tags) {
        // 假设"我的歌曲"标签的名称为"我的歌曲"，过滤掉它
        if (tag.name() != "我的歌曲") {
            // 创建自定义标签项控件
            TagListItem* tagWidget = new TagListItem(tag.name(), tag.coverPath(), true, true);
            
            // 连接编辑信号
            connect(tagWidget, &TagListItem::editRequested, this, [this, tag](const QString& tagName) {
                // TODO: 实现标签编辑功能
                Q_UNUSED(tagName)
            });
            
            // 创建列表项并设置自定义控件
            QListWidgetItem* item2 = new QListWidgetItem();
            item2->setData(Qt::UserRole, tag.id());
            item2->setSizeHint(tagWidget->sizeHint());
            tagListTarget->addItem(item2);
            tagListTarget->setItemWidget(item2, tagWidget);
        }
    }
}

void ManageTagDialog::recordOperation(Operation::Type type, const QList<int>& songIds, const QSet<int>& sourceTagIds, const QSet<int>& targetTagIds)
{
    Operation op;
    op.type = type;
    op.songIds = songIds;
    op.sourceTagIds = sourceTagIds;
    op.targetTagIds = targetTagIds;
    m_operationStack.append(op);
}

void ManageTagDialog::undoLastOperation()
{
    if (m_operationStack.isEmpty()) {
        QMessageBox::information(this, tr("无可撤销操作"), tr("没有可撤销的操作。"));
        return;
    }
    Operation op = m_operationStack.takeLast();
    SongDao songDao;
    if (op.type == Operation::Copy) {
        // 撤销复制：从目标标签移除歌曲
        for (int songId : op.songIds) {
            for (int tagId : op.targetTagIds) {
                songDao.removeSongFromTag(songId, tagId);
            }
        }
    } else if (op.type == Operation::Move) {
        // 撤销移动：从目标标签移除，恢复到源标签
        for (int songId : op.songIds) {
            for (int tagId : op.targetTagIds) {
                songDao.removeSongFromTag(songId, tagId);
            }
            for (int tagId : op.sourceTagIds) {
                songDao.addSongToTag(songId, tagId);
            }
        }
    }
    onSourceTagSelectionChanged();
    QMessageBox::information(this, tr("撤销"), tr("已撤销最近一次操作。"));
}

void ManageTagDialog::commitAllOperations()
{
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.transaction()) {
        QMessageBox::critical(this, tr("数据库错误"), tr("无法开启事务，保存失败！"));
        return;
    }
    SongDao songDao;
    bool ok = true;
    for (const Operation& op : m_operationStack) {
        if (op.type == Operation::Copy) {
            for (int songId : op.songIds) {
                for (int tagId : op.targetTagIds) {
                    if (!songDao.songHasTag(songId, tagId)) {
                        if (!songDao.addSongToTag(songId, tagId)) {
                            ok = false;
                            break;
                        }
                    }
                }
                if (!ok) break;
            }
        } else if (op.type == Operation::Move) {
            for (int songId : op.songIds) {
                for (int sourceTagId : op.sourceTagIds) {
                    if (!songDao.removeSongFromTag(songId, sourceTagId)) {
                        ok = false;
                        break;
                    }
                }
                for (int targetTagId : op.targetTagIds) {
                    if (!songDao.songHasTag(songId, targetTagId)) {
                        if (!songDao.addSongToTag(songId, targetTagId)) {
                            ok = false;
                            break;
                        }
                    }
                }
                if (!ok) break;
            }
        }
        if (!ok) break;
    }
    if (ok) {
        if (!db.commit()) {
            QMessageBox::critical(this, tr("数据库错误"), tr("提交事务失败，所有更改已回滚！"));
            db.rollback();
        } else {
            m_operationStack.clear();
            QMessageBox::information(this, tr("保存成功"), tr("所有更改已保存。"));
        }
    } else {
        db.rollback();
        QMessageBox::critical(this, tr("数据库错误"), tr("保存过程中发生错误，所有更改已回滚！"));
    }
}

void ManageTagDialog::onCopySongs()
{
    qDebug() << "ManageTagDialog::onCopySongs called";
    
    // 调用通用的转移方法，isCopy=true表示复制操作
    performTransfer(true);
}

void ManageTagDialog::onMoveSongs()
{
    qDebug() << "ManageTagDialog::onMoveSongs called";
    
    // 调用通用的转移方法，isCopy=false表示移动操作
    performTransfer(false);
}

void ManageTagDialog::onUndo()
{
    undoLastOperation();
}

void ManageTagDialog::onExitNoSave()
{
    // 退出不保存：直接关闭对话框
    reject();
}

void ManageTagDialog::onExitError()
{
    // 退出报错：弹窗提示并关闭
    QMessageBox::critical(this, tr("错误"), tr("发生未处理异常，已退出。"));
    reject();
}

// 退出并保存（可绑定到"保存"按钮）
void ManageTagDialog::accept()
{
    commitAllOperations();
    QDialog::accept();
}

void ManageTagDialog::onSelectAllSongs()
{
    // 全选歌曲列表中的所有歌曲
    songList->selectAll();
}

void ManageTagDialog::onDeselectAllSongs()
{
    // 取消选中歌曲列表中的所有歌曲
    songList->clearSelection();
}