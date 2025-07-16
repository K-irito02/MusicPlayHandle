#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <QFileDialog>
#include <QListWidgetItem>
#include "src/ui/widgets/taglistitem.h"

#include "src/audio/audiotypes.h"

// 使用AudioTypes命名空间中的PlayMode枚举

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

// 前向声明
class MainWindowController;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    // TagListItem相关槽函数
    void onTagItemClicked(const QString& tagName);
    void onTagItemDoubleClicked(const QString& tagName);
    void onTagEditRequested(const QString& tagName);

private slots:
    // 工具栏按钮槽函数
    void onActionAddMusic();
    void onActionCreateTag();
    void onActionManageTag();
    void onActionPlayInterface();
    void onActionSettings();
    
    // 歌曲列表控制按钮槽函数
    void onPlayAllClicked();
    void onShuffleClicked();
    void onRepeatClicked();
    void onSortClicked();
    void onDeleteClicked();
    
    // 播放控制按钮槽函数
    void onPreviousClicked();
    void onPlayPauseClicked();
    void onNextClicked();
    
    // 列表项点击事件
    void onTagListItemClicked(QListWidgetItem* item);
    void onSongListItemClicked(QListWidgetItem* item);
    void onTagListItemDoubleClicked(QListWidgetItem* item);
    void onSongListItemDoubleClicked(QListWidgetItem* item);
    
    // 滑块事件
    void onProgressSliderChanged(int value);
    void onVolumeSliderChanged(int value);
    void showAddSongDialog();
    void addSongs(const QStringList& filePaths);
    void refreshPlaybackStatus();
    void refreshSettings();
    
    void showCreateTagDialog();
    void showManageTagDialog();
    void showPlayInterfaceDialog();
    void showSettingsDialog();

private:
    Ui::MainWindow *ui;
    MainWindowController* m_controller;
    
    // 播放状态相关
    bool m_isPlaying;           // 当前是否正在播放
    AudioTypes::PlayMode m_currentPlayMode; // 当前播放模式
    
    // 初始化方法
    void setupConnections();
    void setupUI();
    void populateDefaultTags();
    void showStatusMessage(const QString& message);
    void applyLanguage(int languageIndex);
};
#endif // MAINWINDOW_H
