#ifndef MANAGETAGDIALOG_H
#define MANAGETAGDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QListWidgetItem>
#include <QStringList>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QInputDialog>
#include <QSet>

QT_BEGIN_NAMESPACE
namespace Ui {
class ManageTagDialog;
}
QT_END_NAMESPACE

class ManageTagDialogController;

class ManageTagDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ManageTagDialog(QWidget *parent = nullptr);
    ~ManageTagDialog();

    // 设置标签和歌曲数据
    void setTagsAndSongs(const QStringList& tags, const QStringList& songs);
    
    // 获取操作结果
    QStringList getModifiedTags() const;
    QStringList getDeletedTags() const;
    
    // 获取歌曲移动信息
    struct SongMove {
        QString song;
        QString fromTag;
        QString toTag;
        bool isCopy; // true为复制，false为移动
    };
    QList<SongMove> getSongMoves() const;

signals:
    void tagCreated(const QString& tagName);
    void tagDeleted(const QString& tagName);
    void tagModified(const QString& oldName, const QString& newName);
    void songMoved(const QString& song, const QString& fromTag, const QString& toTag, bool isCopy);
    void operationUndone();
    void dialogAccepted();
    void dialogRejected();

protected:
    void showEvent(QShowEvent* event) override;
    void accept() override;

private:
    Ui::ManageTagDialog *ui;
    ManageTagDialogController* m_controller;
    
    QStringList m_originalTags;
    QStringList m_modifiedTags;
    QStringList m_deletedTags;
    QList<SongMove> m_songMoves;
    
    // 操作历史（用于撤销）
    struct Operation {
        enum Type { Copy, Move };
        Type type;
        QList<int> songIds;
        QSet<int> sourceTagIds;
        QSet<int> targetTagIds;
    };
    QList<Operation> m_operationStack;
    void recordOperation(Operation::Type type, const QList<int>& songIds, const QSet<int>& sourceTagIds, const QSet<int>& targetTagIds);
    void undoLastOperation();
    void commitAllOperations();
    
    void setupConnections();
    void setupUI();
    void updateButtonStates();
    void updateTagLists();
    void updateSongList();
    void loadSongsForTag(const QString& tag);
    void performTransfer(bool isCopy);
    void addOperation(const Operation& op);
    void showStatusMessage(const QString& message);
    QString getSelectedTag1() const;
    QString getSelectedTag2() const;
    QStringList getSelectedSongs() const;

private:
    // 三个列表控件
    QListWidget* tagListSource;   // 标签列表1
    QListWidget* songList;        // 歌曲列表
    QListWidget* tagListTarget;   // 标签列表2
    // 五个操作按钮
    QPushButton* btnCopy;
    QPushButton* btnMove;
    QPushButton* btnUndo;
    QPushButton* btnExitNoSave;
    QPushButton* btnExitError;
    // 状态
    QSet<int> selectedSourceTagIds;
    QSet<int> selectedSongIds;
    QSet<int> selectedTargetTagIds;
private slots:
    void onSourceTagSelectionChanged();
    void onSongSelectionChanged();
    void onTargetTagSelectionChanged();
    void onCopySongs();
    void onMoveSongs();
    void onUndo();
    void onSelectAllSongs();
    void onDeselectAllSongs();
    void onExitNoSave();
    void onExitError();
};

#endif // MANAGETAGDIALOG_H