#ifndef ADDSONGDIALOG_H
#define ADDSONGDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QListWidgetItem>
#include <QStringList>
#include <QFileDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QLabel>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>

QT_BEGIN_NAMESPACE
namespace Ui {
class AddSongDialog;
}
QT_END_NAMESPACE

class AddSongDialogController;

class AddSongDialog : public QDialog
{
    Q_OBJECT

public:
    AddSongDialog(QWidget *parent = nullptr);
    ~AddSongDialog();

    // 获取选择的音频文件
    QStringList getSelectedFiles() const;
    
    // 获取选择的标签
    QStringList getSelectedTags() const;
    
    // 设置可用的标签列表
    void setAvailableTags(const QStringList& tags);
    
    // 添加音频文件到列表
    void addAudioFiles(const QStringList& files);
    
    // 清除音频文件列表
    void clearAudioFiles();
    
    // 拖放事件处理
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    
    // 获取控制器实例
    AddSongDialogController* getController() const;

signals:
    void filesAdded(const QStringList& files);
    void tagAssigned(const QString& file, const QString& tag);
    void dialogAccepted();
    void dialogRejected();

private slots:
    void onAddFilesClicked();
    void onCreateTagClicked();
    void onDeleteTagClicked();
    void onEditTagClicked();
    void onAssignTagClicked();
    void onUndoAssignClicked();
    void onSelectAllClicked();
    void onDeselectAllClicked();
    void onExitSaveClicked();
    void onExitDiscardClicked();
    void onFileListSelectionChanged();
    void onTagListSelectionChanged();
    void onFilesAdded(const QStringList& filePaths);
    void onTagCreated(const QString& tagName, bool isSystemTag);

private:
    Ui::AddSongDialog *ui;
    AddSongDialogController* m_controller;
    
    QStringList m_selectedFiles;
    QStringList m_selectedTags;
    
    void setupConnections();
    void setupUI();
    void showStatusMessage(const QString& message);
    void initializeDefaultTags();

public:
    void updateButtonStates();
};

#endif // ADDSONGDIALOG_H