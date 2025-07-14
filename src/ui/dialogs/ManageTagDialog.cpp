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
    // 加载指定标签的歌曲
    Q_UNUSED(tag)
}

void ManageTagDialog::performTransfer(bool isCopy)
{
    // 执行歌曲传输
    Q_UNUSED(isCopy)
}

void ManageTagDialog::showStatusMessage(const QString& message)
{
    // 显示状态消息
    Q_UNUSED(message)
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
    
    // 只显示第一个选中标签的歌曲
    songList->clear();
    if (!selectedSourceTagIds.isEmpty()) {
        int tagId = *selectedSourceTagIds.begin();
        SongDao songDao;
        QList<Song> songs = songDao.getSongsByTag(tagId);
        for (const Song& song : songs) {
            QListWidgetItem* songItem = new QListWidgetItem(song.title());
            songItem->setData(Qt::UserRole, song.id());
            songList->addItem(songItem);
        }
    }
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
    
    // 使用正确的TagDAO构造方式
    TagDAO tagDao(DatabaseManager::instance());
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
    if (selectedSourceTagIds.isEmpty() || selectedSongIds.isEmpty() || selectedTargetTagIds.isEmpty()) {
        QMessageBox::warning(this, tr("操作无效"), tr("请先选择源标签、歌曲和目标标签！"));
        return;
    }
    // 仅记录操作，不直接写数据库
    recordOperation(Operation::Copy, selectedSongIds.values(), selectedSourceTagIds, selectedTargetTagIds);
    QMessageBox::information(this, tr("操作已暂存"), tr("复制操作已加入待保存队列，点击保存后生效。"));
}

void ManageTagDialog::onMoveSongs()
{
    if (selectedSourceTagIds.isEmpty() || selectedSongIds.isEmpty() || selectedTargetTagIds.isEmpty()) {
        QMessageBox::warning(this, tr("操作无效"), tr("请先选择源标签、歌曲和目标标签！"));
        return;
    }
    // 仅记录操作，不直接写数据库
    recordOperation(Operation::Move, selectedSongIds.values(), selectedSourceTagIds, selectedTargetTagIds);
    QMessageBox::information(this, tr("操作已暂存"), tr("移动操作已加入待保存队列，点击保存后生效。"));
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