#include "mainwindowcontroller.h"
#include "../../../mainwindow.h"
#include "../../audio/audioengine.h"
#include "../../audio/audiotypes.h"
#include "../../managers/tagmanager.h"
#include "../../managers/playlistmanager.h"
#include "../../core/componentintegration.h"
#include "../../core/constants.h"
#include "../../threading/mainthreadmanager.h"
#include "../../models/song.h"
#include "../../models/tag.h"
#include "../../models/playlist.h"
#include "../../core/logger.h"
#include "../../threading/mainthreadmanager.h"
#include "../../audio/audiotypes.h"
#include "../../database/tagdao.h"
#include "../../database/songdao.h"
#include "../../core/constants.h"
#include "addsongdialogcontroller.h"
#include "playinterfacecontroller.h"
#include "managetagdialogcontroller.h"
#include "../dialogs/managetagdialog.h"
#include "../dialogs/playinterface.h"
#include "../dialogs/settingsdialog.h"
#include "../widgets/taglistitem.h"
#include "../widgets/musicprogressbar.h"
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMessageBox>
#include <QStatusBar>
#include <QSettings>
#include <QTimer>
#include "../../database/songdao.h"
#include "../../database/tagdao.h"
#include <QListWidgetItem>
#include <QPixmap>
#include "../../ui/dialogs/createtagdialog.h"
#include <QMenu>
#include <QMessageBox>
#include <QLineEdit>
#include <QLabel>
#include "../../database/databasemanager.h"
#include <QIcon>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QUrl>
#include <QXmlStreamReader>
#include <QFormLayout>
#include <QSpinBox>
#include <QMap>
#include <QInputDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QToolTip>
#include <QtConcurrent>

MainWindowController::MainWindowController(MainWindow* mainWindow, QObject* parent)
    : QObject(parent)
    , m_isVolumeSliderPressed(false)
    , m_lastVolumeBeforeMute(50)
    , m_volumeLabel(nullptr)
    , m_volumeIconLabel(nullptr)
    , m_mainWindow(mainWindow)
    , m_audioEngine(nullptr)
    , m_playlistManager(nullptr)
    , m_tagManager(nullptr)
    , m_componentIntegration(nullptr)
    , m_lastActiveTag("")
    , m_lastPlaylist()
    , m_playlistChangedByUser(false)
    , m_shouldKeepPlaylist(false)
    , m_needsRecentPlaySortUpdate(false)
    , m_addSongController(nullptr)
    , m_playInterfaceController(nullptr)
    , m_manageTagController(nullptr)
    , m_tagListWidget(nullptr)
    , m_songListWidget(nullptr)
    , m_toolBar(nullptr)
    , m_splitter(nullptr)
    , m_tagFrame(nullptr)
    , m_songFrame(nullptr)
    , m_playbackFrame(nullptr)

    , m_musicProgressBar(nullptr)
    , m_volumeSlider(nullptr)
    , m_playButton(nullptr)
    , m_pauseButton(nullptr)
    , m_nextButton(nullptr)
    , m_previousButton(nullptr)
    , m_muteButton(nullptr)
    , m_progressBar(nullptr)
    , m_statusBar(nullptr)
    , m_state(MainWindowState::Initializing)
    , m_viewMode(ViewMode::TagView)
    , m_sortMode(SortMode::Title)
    , m_sortAscending(true)
    , m_initialized(false)
    , m_searchQuery("")
    , m_currentSearchIndex(0)
    , m_settings(nullptr)
    , m_updateTimer(nullptr)
    , m_statusTimer(nullptr)
    , m_dragDropEnabled(true)
{
    // 初始化设置
    m_settings = new QSettings(this);
    
    // 初始化定时器
    m_updateTimer = new QTimer(this);
    m_statusTimer = new QTimer(this);
    
    // 连接定时器信号
    connect(m_updateTimer, &QTimer::timeout, this, &MainWindowController::refreshUI);
    connect(m_statusTimer, &QTimer::timeout, this, &MainWindowController::updateStatusMessage);
}

MainWindowController::~MainWindowController()
{
    shutdown();
}

bool MainWindowController::initialize()
{
    if (m_initialized) {
        return true;
    }
    
    logInfo("正在初始化主窗口控制器...");
    
    try {
        // 设置UI
        setupUI();
        
        // 检查关键控件是否找到
        if (!m_tagListWidget) {
            logError("标签列表控件未找到，初始化失败");
            setState(MainWindowState::Error);
            return false;
        }
        
        if (!m_songListWidget) {
            logError("歌曲列表控件未找到，初始化失败");
            setState(MainWindowState::Error);
            return false;
        }
        
        // 设置连接
        setupConnections();
        
        // 加载设置
        loadSettings();
        
        // 设置状态
        setState(MainWindowState::Ready);
        
        // 初始化时同步播放模式按钮
        updatePlayModeButton();
        
        // 初始化数据显示
        updateTagList();
        updateSongList();
        
        m_initialized = true;
        logInfo("主窗口控制器初始化完成");
        
        return true;
    } catch (const std::exception& e) {
        logError(QString("主窗口控制器初始化失败: %1").arg(e.what()));
        setState(MainWindowState::Error);
        return false;
    }
}

void MainWindowController::shutdown()
{

    if (!m_initialized) {
        return;
    }
    
    logInfo("正在关闭主窗口控制器...");
    
    // 场景B触发条件2：用户退出应用程序
    // 如果在"最近播放"标签内有待更新的排序，现在触发更新
    if (m_needsRecentPlaySortUpdate) {
        logInfo("场景B触发条件2：用户退出应用程序，触发最近播放排序更新");
        m_needsRecentPlaySortUpdate = false;
        // 立即更新最近播放列表排序
        updateSongList();
        logInfo("最近播放列表已重新排序");
    }
    
    // 保存设置
    saveSettings();
    
    // 停止定时器
    if (m_updateTimer) {
        m_updateTimer->stop();
    }
    if (m_statusTimer) {
        m_statusTimer->stop();
    }
    
    m_initialized = false;
    logInfo("主窗口控制器已关闭");
}

bool MainWindowController::isInitialized() const
{
    return m_initialized;
}

void MainWindowController::setState(MainWindowState state)
{
    if (m_state != state) {
        m_state = state;
        emit stateChanged(state);
        updateUIState();
    }
}

MainWindowState MainWindowController::getState() const
{
    return m_state;
}

void MainWindowController::setViewMode(ViewMode mode)
{
    if (m_viewMode != mode) {
        m_viewMode = mode;
        emit viewModeChanged(mode);
        refreshUI();
    }
}

ViewMode MainWindowController::getViewMode() const
{
    return m_viewMode;
}

// 工具栏动作槽函数
void MainWindowController::onActionAddMusic()
{
    logInfo("添加音乐请求");
    emit addSongRequested();
}

void MainWindowController::onActionCreateTag()
{
    logInfo("创建标签请求");
    emit createTagRequested();
}

void MainWindowController::onActionManageTag()
{
    logInfo("管理标签请求");
    
    try {
        // 检查数据库连接 - 使用DatabaseManager而不是直接检查QSqlDatabase
        DatabaseManager* dbManager = DatabaseManager::instance();
        if (!dbManager || !dbManager->isValid()) {
            logError("数据库管理器不可用或数据库连接无效");
            showErrorDialog("数据库错误", "数据库连接不可用，无法打开标签管理对话框。");
            return;
        }
        
        // 创建并显示ManageTagDialog
        ManageTagDialog dialog(m_mainWindow);
        dialog.setWindowTitle("管理标签");
        
        // 连接对话框的信号
        connect(&dialog, &ManageTagDialog::finished, [this](int result) {
            if (result == QDialog::Accepted) {
                logInfo("标签管理对话框被接受");
                // 刷新标签列表和歌曲列表
                refreshTagList();
                refreshSongList();
            } else {
                logInfo("标签管理对话框被取消");
            }
        });
        
        // 连接歌曲移动信号
        connect(&dialog, &ManageTagDialog::songMoved, [this](const QString& song, const QString& fromTag, const QString& toTag, bool isCopy) {
            logInfo(QString("歌曲移动信号: %1 从 %2 到 %3, 复制=%4").arg(song).arg(fromTag).arg(toTag).arg(isCopy));
            // 刷新歌曲列表以确保UI同步
            refreshSongList();
        });
        
        // 显示对话框
        dialog.exec();
        
    } catch (const std::exception& e) {
        logError(QString("打开标签管理对话框时发生异常: %1").arg(e.what()));
        showErrorDialog("错误", QString("打开标签管理对话框时发生错误: %1").arg(e.what()));
    } catch (...) {
        logError("打开标签管理对话框时发生未知异常");
        showErrorDialog("错误", "打开标签管理对话框时发生未知错误");
    }
}

void MainWindowController::onActionPlayInterface()
{
    logInfo("播放界面请求");
    
    try {
        // 检查数据库连接
        DatabaseManager* dbManager = DatabaseManager::instance();
        if (!dbManager || !dbManager->isValid()) {
            logError("数据库管理器不可用或数据库连接无效");
            showErrorDialog("数据库错误", "数据库连接不可用，无法打开播放界面。");
            return;
        }
        
        // 创建并显示PlayInterface（现在是独立窗口）
        PlayInterface* dialog = new PlayInterface(m_mainWindow);
        dialog->setAttribute(Qt::WA_DeleteOnClose, true);
        
        // 确保AudioEngine已初始化
        if (!m_audioEngine) {
            m_audioEngine = AudioEngine::instance();
        }
        
        // 设置AudioEngine连接到PlayInterface
        if (dialog && m_audioEngine) {
            dialog->setAudioEngine(m_audioEngine);
            logInfo("已为播放界面设置AudioEngine连接");
            
            // 也为控制器设置连接（双重保险）
            if (dialog->getController()) {
                dialog->getController()->setAudioEngine(m_audioEngine);
                logInfo("已为播放界面控制器设置AudioEngine连接");
            }
            
            // 连接播放界面信号到主界面控制逻辑
            connect(dialog, &PlayInterface::playPauseClicked, this, &MainWindowController::onPlayButtonClicked);
            connect(dialog, &PlayInterface::nextClicked, this, &MainWindowController::onNextButtonClicked);
            connect(dialog, &PlayInterface::previousClicked, this, &MainWindowController::onPreviousButtonClicked);
            logInfo("已连接播放界面控制信号到主界面");
        }
        
        dialog->show();
        
    } catch (const std::exception& e) {
        logError(QString("打开播放界面时发生异常: %1").arg(e.what()));
        showErrorDialog("错误", QString("打开播放界面时发生错误: %1").arg(e.what()));
    } catch (...) {
        logError("打开播放界面时发生未知异常");
        showErrorDialog("错误", "打开播放界面时发生未知错误");
    }
}

void MainWindowController::onActionSettings()
{
    logInfo("设置请求");
    
    try {
        // 检查数据库连接
        DatabaseManager* dbManager = DatabaseManager::instance();
        if (!dbManager || !dbManager->isValid()) {
            logError("数据库管理器不可用或数据库连接无效");
            showErrorDialog("数据库错误", "数据库连接不可用，无法打开设置对话框。");
            return;
        }
        
        // 创建并显示SettingsDialog
        SettingsDialog dialog(m_mainWindow);
        dialog.setWindowTitle("设置");
        dialog.exec();
        
        // 移除这行，避免循环调用
        // emit settingsRequested();
        
    } catch (const std::exception& e) {
        logError(QString("打开设置对话框时发生异常: %1").arg(e.what()));
        showErrorDialog("错误", QString("打开设置对话框时发生错误: %1").arg(e.what()));
    } catch (...) {
        logError("打开设置对话框时发生未知异常");
        showErrorDialog("错误", "打开设置对话框时发生未知错误");
    }
}

void MainWindowController::onActionAbout()
{
    showInfoDialog("关于", "Qt6音频播放器 v1.0.0\n基于Qt6和C++11开发");
}

void MainWindowController::onActionExit()
{
    if (m_mainWindow) {
        m_mainWindow->close();
    }
}

// 标签列表事件
void MainWindowController::onTagListItemClicked(QListWidgetItem* item)
{
    if (item) {
        logInfo(QString("标签被点击: %1").arg(item->text()));
        
        // 清除之前选中的歌曲信息，因为切换了标签
        m_selectedSong = Song();
        logDebug("标签切换，清除选中歌曲信息");
        
        // 根据选中的标签更新歌曲列表
        updateSongList();
        updateStatusBar(QString("选择标签: %1").arg(item->text()));
    }
}

// 歌曲列表事件
void MainWindowController::onSongListItemClicked(QListWidgetItem* item)
{
    if (item) {
        logInfo(QString("歌曲被点击: %1").arg(item->text()));
        updateStatusBar(QString("选择歌曲: %1").arg(item->text()));
        
        // 更新选中的歌曲信息
        QVariant songData = item->data(Qt::UserRole);
        if (songData.isValid()) {
            Song song = songData.value<Song>();
            if (song.isValid()) {
                m_selectedSong = song;
                logDebug(QString("更新选中歌曲: ID=%1, 标题=%2, 路径=%3")
                        .arg(song.id()).arg(song.title()).arg(song.filePath()));
            }
        }
    }
}

void MainWindowController::onSongListItemDoubleClicked(QListWidgetItem* item)
{
    if (!item) {
        logWarning("双击的歌曲项为空");
        return;
    }
    
    if (!m_audioEngine) {
        logWarning("音频引擎未初始化，无法播放歌曲");
        return;
    }
    
    // 获取歌曲对象
    QVariant songData = item->data(Qt::UserRole);
    
    if (!songData.isValid()) {
        logWarning("歌曲数据无效，无法播放");
        updateStatusBar("播放失败：歌曲数据无效", 3000);
        return;
    }
    
    // 转换为Song对象
    Song song = songData.value<Song>();
    if (!song.isValid()) {
        logWarning("无法转换为有效的Song对象，播放失败");
        updateStatusBar("播放失败：歌曲对象无效", 3000);
        return;
    }
    
    // 检查文件是否存在
    if (!QFile::exists(song.filePath())) {
        logWarning(QString("歌曲文件不存在: %1").arg(song.filePath()));
        updateStatusBar("播放失败：文件不存在", 3000);
        return;
    }
    
    // 更新选中的歌曲信息
    m_selectedSong = song;
    logDebug(QString("双击更新选中歌曲: ID=%1, 标题=%2").arg(song.id()).arg(song.title()));
    
    // 调用音频引擎播放歌曲
     try {
         if (!m_songListWidget) {
             logWarning("歌曲列表控件未初始化");
             return;
         }
         
         // 构建当前标签下的播放列表
         QList<Song> playlist;
         int targetIndex = -1;
         int songCount = m_songListWidget->count();
         
         for (int i = 0; i < songCount; ++i) {
             QListWidgetItem* listItem = m_songListWidget->item(i);
             if (listItem) {
                 QVariant itemSongData = listItem->data(Qt::UserRole);
                 if (itemSongData.isValid()) {
                     Song listSong = itemSongData.value<Song>();
                     if (listSong.isValid()) {
                         // 检查文件格式是否支持
                         if (m_audioEngine->isFormatSupported(listSong.filePath())) {
                             playlist.append(listSong);
                             // 找到当前点击的歌曲在播放列表中的索引
                             if (listSong.id() == song.id()) {
                                 targetIndex = playlist.size() - 1;
                             }
                         }
                     }
                 }
             }
         }
         
         if (playlist.isEmpty()) {
             logWarning("无法构建播放列表，所有歌曲格式都不支持");
             updateStatusBar("播放失败：没有支持的音频格式", 3000);
             return;
         }
         
         if (targetIndex == -1) {
             logWarning("目标歌曲格式不支持，创建单曲播放列表");
             // 如果目标歌曲格式不支持，创建单曲播放列表
             playlist.clear();
             playlist.append(song);
             targetIndex = 0;
         }
         
         // 设置播放列表到AudioEngine
        m_audioEngine->setPlaylist(playlist);
        
        // 设置当前歌曲索引（这会触发currentSongChanged信号）
        m_audioEngine->setCurrentIndex(targetIndex);
        
        // 开始播放
        m_audioEngine->play();
         
         logInfo(QString("开始播放歌曲: %1 - %2").arg(song.artist()).arg(song.title()));
         updateStatusBar(QString("正在播放: %1").arg(item->text()), 3000);
         
     } catch (const std::exception& e) {
         logError(QString("播放歌曲时发生异常: %1").arg(e.what()));
         updateStatusBar("播放失败：发生异常", 3000);
     }
}

// 播放控制事件
void MainWindowController::onPlayButtonClicked()
{
    if (!m_audioEngine) {
        m_audioEngine = AudioEngine::instance();
        if (!m_audioEngine) {
            updateStatusBar("音频引擎未就绪", 2000);
            return;
        }
    }
    
    // 添加调试信息
    m_audioEngine->debugAudioState();
    
    AudioTypes::AudioState currentState = m_audioEngine->state();
    int playlistSize = m_audioEngine->playlist().size();
    int currentIndex = m_audioEngine->currentIndex();
    
    // 首先检查播放列表是否为空或索引无效
    if (playlistSize == 0 || currentIndex < 0) {
        // 检查当前歌曲列表是否有歌曲可以播放
        if (m_songListWidget && m_songListWidget->count() > 0) {
            startNewPlayback();
        } else {
            updateStatusBar("播放列表为空，请先添加歌曲", 3000);
        }
        return;
    }
    
    // 播放列表不为空时的播放/暂停逻辑
    switch (currentState) {
        case AudioTypes::AudioState::Playing:
            m_audioEngine->pause();
            break;
            
        case AudioTypes::AudioState::Paused:
            m_audioEngine->play();
            break;
            
        case AudioTypes::AudioState::Loading:
            updateStatusBar("正在加载媒体文件...", 2000);
            break;
            
        case AudioTypes::AudioState::Error:
            m_audioEngine->play();
            break;
            
        default:
            m_audioEngine->play();
            break;
    }
    
    // 操作后再次调试
    QTimer::singleShot(100, [this]() {
        if (m_audioEngine) {
            qDebug() << "[播放按钮] 操作后的状态:";
            m_audioEngine->debugAudioState();
        }
    });
}

void MainWindowController::startNewPlayback()
{
    if (!m_songListWidget) {
        updateStatusBar("歌曲列表未初始化", 2000);
        return;
    }
    
    // 检查当前歌曲列表是否有歌曲
    if (m_songListWidget->count() == 0) {
        // 检查是否有用户最后选中的歌曲
        if (m_selectedSong.isValid()) {
            // 根据选中歌曲所在的标签来构建播放列表
            try {
                if (m_tagManager) {
                    QList<Tag> songTags = m_tagManager->getTagsForSong(m_selectedSong.id());
                    
                    if (!songTags.isEmpty()) {
                        // 使用第一个标签
                        Tag targetTag = songTags.first();
                        
                        // 在标签列表中找到并选中该标签
                        if (m_tagListWidget) {
                            for (int i = 0; i < m_tagListWidget->count(); ++i) {
                                QListWidgetItem* tagItem = m_tagListWidget->item(i);
                                if (tagItem) {
                                    QVariant tagData = tagItem->data(Qt::UserRole);
                                    if (tagData.isValid()) {
                                        Tag tag = tagData.value<Tag>();
                                        if (tag.id() == targetTag.id()) {
                                            m_tagListWidget->setCurrentItem(tagItem);
                                            updateSongList();
                                            
                                            // 等待歌曲列表更新后开始播放
                                            QTimer::singleShot(100, [this]() {
                                                if (m_songListWidget && m_songListWidget->count() > 0) {
                                                    // 选中目标歌曲
                                                    for (int j = 0; j < m_songListWidget->count(); ++j) {
                                                        QListWidgetItem* songItem = m_songListWidget->item(j);
                                                        if (songItem) {
                                                            QVariant songData = songItem->data(Qt::UserRole);
                                                            if (songData.isValid()) {
                                                                Song song = songData.value<Song>();
                                                                if (song.id() == m_selectedSong.id()) {
                                                                    m_songListWidget->setCurrentItem(songItem);
                                                                    break;
                                                                }
                                                            }
                                                        }
                                                    }
                                                    qDebug() << "[startNewPlayback] 从选中歌曲所在标签开始播放";
                                                    startPlaybackFromCurrentList();
                                                } else {
                                                    qDebug() << "[startNewPlayback] 选中歌曲所在标签下没有歌曲";
                                                    updateStatusBar("选中歌曲所在标签下没有歌曲", 3000);
                                                }
                                            });
                                            return;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            } catch (const std::exception& e) {
                logError(QString("获取歌曲标签时发生异常: %1").arg(e.what()));
            }
        }
        
        // 如果没有选中歌曲或处理失败，尝试切换到"我的歌曲"标签
        if (m_tagListWidget) {
            for (int i = 0; i < m_tagListWidget->count(); ++i) {
                QListWidgetItem* tagItem = m_tagListWidget->item(i);
                if (tagItem && (tagItem->text() == "我的歌曲" || tagItem->text() == "全部歌曲")) {
                    m_tagListWidget->setCurrentItem(tagItem);
                    updateSongList();
                    
                    // 等待歌曲列表更新
                    QTimer::singleShot(100, [this]() {
                        if (m_songListWidget && m_songListWidget->count() > 0) {
                            startPlaybackFromCurrentList();
                        } else {
                            updateStatusBar("没有可播放的歌曲，请先添加歌曲", 3000);
                        }
                    });
                    return;
                }
            }
        }
        
        // 尝试选择第一个可用的标签
        if (m_tagListWidget && m_tagListWidget->count() > 0) {
            QListWidgetItem* firstTagItem = m_tagListWidget->item(0);
            if (firstTagItem) {
                m_tagListWidget->setCurrentItem(firstTagItem);
                updateSongList();
                
                // 等待歌曲列表更新
                QTimer::singleShot(100, [this]() {
                    if (m_songListWidget && m_songListWidget->count() > 0) {
                        startPlaybackFromCurrentList();
                    } else {
                        updateStatusBar("没有可播放的歌曲，请先添加歌曲", 3000);
                    }
                });
                return;
            }
        }
        
        updateStatusBar("没有可播放的歌曲，请先添加歌曲", 3000);
        return;
    }
    
    // 当前歌曲列表有歌曲，检查是否有选中的歌曲
    QListWidgetItem* selectedItem = m_songListWidget->currentItem();
    if (!selectedItem && m_songListWidget->count() > 0) {
        // 没有选中歌曲，选择第一首
        m_songListWidget->setCurrentItem(m_songListWidget->item(0));
    }
    
    startPlaybackFromCurrentList();
}

void MainWindowController::startPlaybackFromCurrentList()
{
    if (!m_songListWidget || !m_audioEngine) {
        logError("组件未初始化，无法开始播放");
        return;
    }
    
    // 检查选中的歌曲
    QListWidgetItem* selectedItem = m_songListWidget->currentItem();
    int targetIndex = 0; // 默认播放第一首
    
    if (selectedItem) {
        // 找到选中歌曲在列表中的索引
        for (int i = 0; i < m_songListWidget->count(); ++i) {
            if (m_songListWidget->item(i) == selectedItem) {
                targetIndex = i;
                break;
            }
        }
    } else {
        // 没有选中歌曲，选择第一首
        if (m_songListWidget->count() > 0) {
            m_songListWidget->setCurrentItem(m_songListWidget->item(0));
        }
    }
    
    // 构建播放列表
    QList<Song> playlist;
    for (int i = 0; i < m_songListWidget->count(); ++i) {
        QListWidgetItem* item = m_songListWidget->item(i);
        if (item) {
            QVariant songData = item->data(Qt::UserRole);
            if (songData.isValid()) {
                Song song = songData.value<Song>();
                if (song.isValid()) {
                    playlist.append(song);
                }
            }
        }
    }
    
    if (playlist.isEmpty()) {
        logWarning("无法构建播放列表");
        updateStatusBar("无法构建播放列表", 2000);
        return;
    }
    
    // 设置播放列表和当前索引
    m_audioEngine->setPlaylist(playlist);
    m_audioEngine->setCurrentIndex(targetIndex);
    
    // 验证设置是否成功
    if (m_audioEngine->playlist().size() != playlist.size()) {
        logError("播放列表设置失败");
        updateStatusBar("播放列表设置失败", 2000);
        return;
    }
    
    if (m_audioEngine->currentIndex() != targetIndex) {
        logError("当前索引设置失败");
        updateStatusBar("当前索引设置失败", 2000);
        return;
    }
    
    // 开始播放
    m_audioEngine->play();
    updateStatusBar(QString("开始播放: %1").arg(playlist[targetIndex].title()), 2000);
}

void MainWindowController::updatePlayButtonUI(bool isPlaying)
{
    if (!m_playButton) return;
    
    // 阻塞信号，避免UI更新时触发点击事件
    m_playButton->blockSignals(true);
    
    if (isPlaying) {
        m_playButton->setIcon(QIcon(":/new/prefix1/images/pauseIcon.png"));
        m_playButton->setText("暂停");
    } else {
        m_playButton->setIcon(QIcon(":/new/prefix1/images/playIcon.png"));
        m_playButton->setText("播放");
    }
    
    // 恢复信号
    m_playButton->blockSignals(false);
}

// 暂停按钮功能已合并到播放按钮中
void MainWindowController::onPauseButtonClicked()
{
    onPlayButtonClicked();
}



void MainWindowController::onNextButtonClicked()
{
    logInfo("下一首按钮被点击");
    if (!m_audioEngine) {
        logError("AudioEngine未初始化");
        return;
    }
    
    // 检查播放列表是否为空或当前索引无效
    int playlistSize = m_audioEngine->playlist().size();
    int currentIndex = m_audioEngine->currentIndex();
    
    // 如果播放列表为空或索引无效
    if (playlistSize == 0 || currentIndex < 0) {
        // 检查当前歌曲列表是否有歌曲可以播放
        if (m_songListWidget && m_songListWidget->count() > 0) {
            startNewPlayback();
        } else {
            qDebug() << "[下一首按钮] 播放列表为空，显示提示";
            updateStatusBar("播放列表为空，请先添加歌曲", 3000);
        }
        return;
    }
    
    // 播放列表不为空，正常切换到下一首
    emit nextRequested();
}

void MainWindowController::onPreviousButtonClicked()
{
    logInfo("上一首按钮被点击");
    if (!m_audioEngine) {
        logError("AudioEngine未初始化");
        return;
    }
    
    // 检查播放列表是否为空或当前索引无效
    int playlistSize = m_audioEngine->playlist().size();
    int currentIndex = m_audioEngine->currentIndex();
    
    qDebug() << "[上一首按钮] 当前播放列表大小:" << playlistSize;
    qDebug() << "[上一首按钮] 当前播放索引:" << currentIndex;
    
    // 如果播放列表为空或索引无效
    if (playlistSize == 0 || currentIndex < 0) {
        // 检查当前歌曲列表是否有歌曲可以播放
        if (m_songListWidget && m_songListWidget->count() > 0) {
            startNewPlayback();
        } else {
            updateStatusBar("播放列表为空，请先添加歌曲", 3000);
        }
        return;
    }
    
    // 播放列表不为空，正常切换到上一首
    emit previousRequested();
}



// 状态显示




void MainWindowController::updatePlaybackInfo(const Song& song)
{
    try {
        if (song.isValid()) {
            // 使用FFmpeg解析的元数据
            QString artist = song.artist();
            QString title = song.title();
            
            // 如果元数据为空，尝试从文件重新解析
            if (artist.isEmpty() || title.isEmpty()) {
                Song updatedSong = song;
                Song::extractAdvancedMetadata(updatedSong, song.filePath());
                artist = updatedSong.artist();
                title = updatedSong.title();
            }
            
            // 分别更新歌曲标题和艺术家标签
            QLabel* titleLabel = m_mainWindow->findChild<QLabel*>("label_song_title");
            QLabel* artistLabel = m_mainWindow->findChild<QLabel*>("label_song_artist");
            
            if (titleLabel) {
                if (!title.isEmpty()) {
                    titleLabel->setText(title);
                } else {
                    // 如果没有标题，使用文件名
                    QFileInfo fileInfo(song.filePath());
                    titleLabel->setText(fileInfo.baseName());
                }
            }
            
            if (artistLabel) {
                artistLabel->setText(artist.isEmpty() ? "" : artist);
            }
            
            // 更新合并的歌曲信息（用于状态栏和窗口标题）
            QString songInfo;
            if (!artist.isEmpty() && !title.isEmpty()) {
                songInfo = QString("%1 - %2").arg(artist, title);
            } else if (!title.isEmpty()) {
                songInfo = title;
            } else {
                QFileInfo fileInfo(song.filePath());
                songInfo = fileInfo.baseName();
            }
            
            // 更新窗口标题
            if (m_mainWindow) {
                QString windowTitle = QString("Qt6音频播放器 - %1").arg(songInfo);
                m_mainWindow->setWindowTitle(windowTitle);
            }
            
            // 更新状态栏
            QString statusMessage = QString("正在播放: %1").arg(songInfo);
            if (song.duration() > 0) {
                statusMessage += QString(" [%1]").arg(formatTime(song.duration()));
            }
            updateStatusBar(statusMessage, 3000);
            
            logInfo(QString("播放信息更新: %1").arg(songInfo));
        } else {
            // 清空播放信息
            QLabel* titleLabel = m_mainWindow->findChild<QLabel*>("label_song_title");
            QLabel* artistLabel = m_mainWindow->findChild<QLabel*>("label_song_artist");
            
            if (titleLabel) {
                titleLabel->setText("未选择歌曲");
            }
            if (artistLabel) {
                artistLabel->setText("");
            }
            
            if (m_mainWindow) {
                m_mainWindow->setWindowTitle("Qt6音频播放器");
            }
            updateStatusBar("就绪", 1000);
            logInfo("清空播放信息");
        }
    } catch (const std::exception& e) {
        logError(QString("更新播放信息时发生错误: %1").arg(e.what()));
    }
}



// 错误处理
void MainWindowController::handleError(const QString& error)
{
    logError(error);
    emit errorOccurred(error);
}

void MainWindowController::showErrorDialog(const QString& title, const QString& message)
{
    QMessageBox::critical(m_mainWindow, title, message);
}

void MainWindowController::showWarningDialog(const QString& title, const QString& message)
{
    QMessageBox::warning(m_mainWindow, title, message);
}

void MainWindowController::showInfoDialog(const QString& title, const QString& message)
{
    QMessageBox::information(m_mainWindow, title, message);
}

// 私有方法实现
void MainWindowController::setupUI()
{
    if (!m_mainWindow) {
        logError("MainWindow指针为空，无法设置UI");
        return;
    }
    
    // 获取UI控件指针
    m_tagListWidget = m_mainWindow->findChild<QListWidget*>("listWidget_my_tags");
    m_songListWidget = m_mainWindow->findChild<QListWidget*>("listWidget_songs");
    m_playButton = m_mainWindow->findChild<QPushButton*>("pushButton_play_pause");
    m_nextButton = m_mainWindow->findChild<QPushButton*>("pushButton_next");
    m_previousButton = m_mainWindow->findChild<QPushButton*>("pushButton_previous");
    m_muteButton = m_mainWindow->findChild<QPushButton*>("pushButton_mute");
    m_volumeSlider = m_mainWindow->findChild<QSlider*>("slider_volume");
    m_playModeButton = m_mainWindow->findChild<QPushButton*>("pushButton_play_mode");
    
    // 创建自定义音乐进度条组件
    m_musicProgressBar = new MusicProgressBar(m_mainWindow);
    m_musicProgressBar->setObjectName("musicProgressBar");
    
    // 查找音量相关控件
    m_volumeLabel = m_mainWindow->findChild<QLabel*>("label_volume_value");
    m_volumeIconLabel = m_mainWindow->findChild<QLabel*>("label_volume_icon");
    
    // 如果找不到音量标签，创建一个
    if (!m_volumeLabel) {
        m_volumeLabel = new QLabel("50%", m_mainWindow);
        m_volumeLabel->setObjectName("label_volume_value");
        m_volumeLabel->setStyleSheet("QLabel { color: #666; font-size: 10px; }");
        m_volumeLabel->setAlignment(Qt::AlignCenter);
        m_volumeLabel->setMinimumWidth(30);
        m_volumeLabel->setMaximumWidth(40);
        
        // 添加到音量框架中
        QFrame* volumeFrame = m_mainWindow->findChild<QFrame*>("frame_volume");
        if (volumeFrame) {
            QHBoxLayout* layout = qobject_cast<QHBoxLayout*>(volumeFrame->layout());
            if (layout) {
                layout->addWidget(m_volumeLabel);
            }
        }
    }
    
    // 将自定义音乐进度条组件添加到进度条框架中
    QFrame* progressFrame = m_mainWindow->findChild<QFrame*>("frame_progress");
    if (progressFrame) {
        // 创建垂直布局以匹配MusicProgressBar的内部结构
        QVBoxLayout* layout = qobject_cast<QVBoxLayout*>(progressFrame->layout());
        if (!layout) {
            layout = new QVBoxLayout(progressFrame);
            layout->setContentsMargins(8, 5, 8, 5);
            layout->setSpacing(2);
            progressFrame->setLayout(layout);
        }
        // 清除原有控件
        QLayoutItem* item;
        while ((item = layout->takeAt(0)) != nullptr) {
            if (item->widget()) {
                item->widget()->deleteLater();
            }
            delete item;
        }
        // 添加自定义进度条组件
        layout->addWidget(m_musicProgressBar);
        
        // 设置进度条组件的尺寸策略
        m_musicProgressBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_musicProgressBar->setMinimumHeight(50);
        m_musicProgressBar->setMaximumHeight(70);
        
        logInfo("自定义音乐进度条组件已添加到界面");
    } else {
        logWarning("未找到进度条框架，无法添加自定义音乐进度条组件");
    }
    
    // 获取AudioEngine实例
    m_audioEngine = AudioEngine::instance();
    
    // 检查关键控件是否找到
    if (!m_playButton) {
        logWarning("未找到播放按钮");
    }
    if (!m_nextButton) logInfo("未找到下一首按钮");
    if (!m_previousButton) logInfo("未找到上一首按钮");
    // 进度条功能已集成到自定义组件中
    if (!m_volumeSlider) logInfo("未找到音量滑块");
    if (!m_tagListWidget) {
        logError("未找到标签列表控件 - 这是关键错误！");
        // 尝试从UI中查找
        QList<QListWidget*> listWidgets = m_mainWindow->findChildren<QListWidget*>();
        logInfo(QString("找到%1个QListWidget控件").arg(listWidgets.size()));
        for (QListWidget* widget : listWidgets) {
            qDebug() << "[MainWindowController] setupUI: QListWidget对象名:" << widget->objectName();
        }
    }
    if (!m_songListWidget) {
        logError("未找到歌曲列表控件 - 这是关键错误！");
        qDebug() << "[MainWindowController] setupUI: 错误 - 未找到歌曲列表控件";
    }
    if (!m_playModeButton) logInfo("未找到播放模式按钮");
    
    // 设置窗口标题
    updateWindowTitle();
    
    // 设置初始状态
    updateUIState();
    
    logInfo("UI控件初始化完成");
}

void MainWindowController::setupConnections()
{
    if (!m_mainWindow) return;
    
    // 播放控制按钮连接已在MainWindow::setupConnections中完成，这里不再重复连接
    // 避免重复连接导致的多次触发问题
    
    // 播放模式按钮连接
    if (m_playModeButton) {
        connect(m_playModeButton, &QPushButton::clicked, this, &MainWindowController::onPlayModeButtonClicked);
        logDebug("播放模式按钮信号连接完成");
    }
    
    // 自定义音乐进度条组件连接
    if (m_musicProgressBar) {
        connect(m_musicProgressBar, &MusicProgressBar::seekRequested, [this](qint64 position) {
            if (m_audioEngine) {
                m_audioEngine->seek(position);
                logInfo(QString("音频跳转到 %1 ms").arg(position));
            } else {
                logError("AudioEngine为空，无法执行跳转");
            }
        });
        logDebug("自定义音乐进度条组件信号连接完成");
    }
    
    // 音量滑块连接 - 添加拖动相关信号
    if (m_volumeSlider) {
        connect(m_volumeSlider, &QSlider::sliderPressed, this, &MainWindowController::onVolumeSliderPressed);
        connect(m_volumeSlider, &QSlider::sliderReleased, this, &MainWindowController::onVolumeSliderReleased);
        connect(m_volumeSlider, &QSlider::valueChanged, this, &MainWindowController::onVolumeSliderChanged);
        logDebug("音量滑块信号连接完成");
    }
    
    // 静音按钮连接
    if (m_muteButton) {
        connect(m_muteButton, &QPushButton::clicked, this, &MainWindowController::onMuteButtonClicked);
        logDebug("静音按钮信号连接完成");
    }
    
    // 音量图标点击连接 - 使用事件过滤器
    if (m_volumeIconLabel) {
        m_volumeIconLabel->installEventFilter(this);
        logDebug("音量图标事件过滤器安装完成");
    }
    
    // 音量标签双击连接 - 使用事件过滤器
    if (m_volumeLabel) {
        m_volumeLabel->installEventFilter(this);
        logDebug("音量标签事件过滤器安装完成");
    }
    
    // 列表控件连接
    if (m_tagListWidget) {
        connect(m_tagListWidget, &QListWidget::itemClicked, this, &MainWindowController::onTagListItemClicked);
        connect(m_tagListWidget, &QListWidget::itemDoubleClicked, this, &MainWindowController::onTagListItemDoubleClicked);
        logDebug("标签列表信号连接完成");
    }
    if (m_songListWidget) {
        connect(m_songListWidget, &QListWidget::itemClicked, this, &MainWindowController::onSongListItemClicked);
        connect(m_songListWidget, &QListWidget::itemDoubleClicked, this, &MainWindowController::onSongListItemDoubleClicked);
        logDebug("歌曲列表信号连接完成");
    }
    
    // 连接PlayInterfaceController信号
    if (m_playInterfaceController) {
        connect(m_playInterfaceController.get(), &PlayInterfaceController::playModeChangeRequested, this, &MainWindowController::cyclePlayMode);
        logDebug("播放界面控制器播放模式信号连接完成");
    }
    
    // 连接AudioEngine信号
    if (m_audioEngine) {
        connect(m_audioEngine, &AudioEngine::stateChanged, this, &MainWindowController::onAudioStateChanged);
        connect(m_audioEngine, &AudioEngine::positionChanged, this, &MainWindowController::onPositionChanged);
        connect(m_audioEngine, &AudioEngine::durationChanged, this, &MainWindowController::onDurationChanged);
        connect(m_audioEngine, &AudioEngine::volumeChanged, this, &MainWindowController::onVolumeChanged);
        connect(m_audioEngine, &AudioEngine::balanceChanged, this, &MainWindowController::onBalanceChanged);
        connect(m_audioEngine, &AudioEngine::mutedChanged, this, &MainWindowController::onMutedChanged);
        connect(m_audioEngine, &AudioEngine::currentSongChanged, this, &MainWindowController::onCurrentSongChanged);
        connect(m_audioEngine, &AudioEngine::playModeChanged, this, &MainWindowController::onPlayModeChanged);
        connect(m_audioEngine, &AudioEngine::errorOccurred, this, &MainWindowController::onAudioError);
        
        // 注意：MusicProgressBar的更新通过MainWindowController的onPositionChanged和onDurationChanged方法处理
        
        logDebug("AudioEngine信号连接完成");
    }
    
    // 连接控制器信号到AudioEngine
    connect(this, &MainWindowController::playRequested, [this](const Song& song) {
        logInfo(QString("收到播放请求：%1").arg(song.filePath()));
        if (m_audioEngine) {
            if (song.isValid()) {
                Tag currentTag = getSelectedTag();
                if (currentTag.isValid()) {
                    // 使用QTimer::singleShot将数据库操作移到下一个事件循环，避免阻塞主线程
                    QTimer::singleShot(0, [this, song, currentTag]() {
                        try {
                            SongDao songDao;
                            QList<Song> playlist = songDao.getSongsByTag(currentTag.id());
                            logInfo(QString("当前标签下歌曲数量：%1").arg(playlist.size()));
                            
                            // 回到主线程执行AudioEngine操作
                            QMetaObject::invokeMethod(this, [this, song, playlist]() {
                                if (!m_audioEngine) return;
                                
                                // 检查是否需要保持播放列表
                                if (m_shouldKeepPlaylist && !m_lastPlaylist.isEmpty() && !m_playlistChangedByUser) {
                                    // 在保存的播放列表中查找目标歌曲
                                    int targetIndex = -1;
                                    for (int i = 0; i < m_lastPlaylist.size(); ++i) {
                                        if (m_lastPlaylist[i].id() == song.id()) {
                                            targetIndex = i;
                                            break;
                                        }
                                    }
                                    
                                    if (targetIndex >= 0) {
                                        // 使用保存的播放列表
                                        m_audioEngine->setPlaylist(m_lastPlaylist);
                                        m_audioEngine->setCurrentIndex(targetIndex);
                                        logInfo(QString("使用保存的播放列表，共%1首歌曲，当前索引: %2").arg(m_lastPlaylist.size()).arg(targetIndex));
                                    } else {
                                        // 目标歌曲不在保存的播放列表中，使用当前标签的播放列表
                                        if (!playlist.isEmpty()) {
                                            int targetIndex = -1;
                                            for (int i = 0; i < playlist.size(); ++i) {
                                                if (playlist[i].id() == song.id()) {
                                                    targetIndex = i;
                                                    break;
                                                }
                                            }
                                            if (targetIndex >= 0) {
                                                m_audioEngine->setPlaylist(playlist);
                                                m_audioEngine->setCurrentIndex(targetIndex);
                                                logInfo(QString("设置当前标签播放列表，共%1首歌曲，当前索引: %2").arg(playlist.size()).arg(targetIndex));
                                            } else {
                                                QList<Song> singleSongPlaylist = {song};
                                                m_audioEngine->setPlaylist(singleSongPlaylist);
                                                m_audioEngine->setCurrentIndex(0);
                                                logInfo("歌曲不在当前标签中，创建单曲播放列表");
                                            }
                                        } else {
                                            QList<Song> singleSongPlaylist = {song};
                                            m_audioEngine->setPlaylist(singleSongPlaylist);
                                            m_audioEngine->setCurrentIndex(0);
                                            logInfo("当前标签无歌曲，创建单曲播放列表");
                                        }
                                    }
                                } else {
                                    // 正常播放逻辑
                                    if (!playlist.isEmpty()) {
                                        int targetIndex = -1;
                                        for (int i = 0; i < playlist.size(); ++i) {
                                            if (playlist[i].id() == song.id()) {
                                                targetIndex = i;
                                                break;
                                            }
                                        }
                                        logInfo(QString("目标歌曲索引：%1").arg(targetIndex));
                                        if (targetIndex >= 0) {
                                            m_audioEngine->setPlaylist(playlist);
                                            m_audioEngine->setCurrentIndex(targetIndex);
                                            logInfo(QString("设置播放列表，共%1首歌曲，当前索引: %2").arg(playlist.size()).arg(targetIndex));
                                        } else {
                                            QList<Song> singleSongPlaylist = {song};
                                            m_audioEngine->setPlaylist(singleSongPlaylist);
                                            m_audioEngine->setCurrentIndex(0);
                                            logInfo("歌曲不在当前标签中，创建单曲播放列表");
                                        }
                                    } else {
                                        // 如果当前标签没有歌曲，创建单曲播放列表
                                        QList<Song> singleSongPlaylist = {song};
                                        m_audioEngine->setPlaylist(singleSongPlaylist);
                                        m_audioEngine->setCurrentIndex(0);
                                        logInfo("当前标签无歌曲，创建单曲播放列表");
                                    }
                                }
                                
                                // 标记用户已主动播放歌曲，重置播放列表保持
                                m_playlistChangedByUser = true;
                                m_shouldKeepPlaylist = false;
                                
                                qDebug() << "[排查] 调用m_audioEngine->play()前，currentIndex:" << m_audioEngine->currentIndex();
                                m_audioEngine->play();
                                logInfo("发送播放请求到AudioEngine");
                            }, Qt::QueuedConnection);
                            
                        } catch (const std::exception& e) {
                            logError(QString("获取播放列表失败: %1").arg(e.what()));
                            // 异常情况下也要回到主线程
                            QMetaObject::invokeMethod(this, [this, song]() {
                                if (!m_audioEngine) return;
                                QList<Song> singleSongPlaylist = {song};
                                m_audioEngine->setPlaylist(singleSongPlaylist);
                                m_audioEngine->setCurrentIndex(0);
                                m_audioEngine->play();
                            }, Qt::QueuedConnection);
                        }
                    });
                } else {
                    QList<Song> singleSongPlaylist = {song};
                    m_audioEngine->setPlaylist(singleSongPlaylist);
                    m_audioEngine->setCurrentIndex(0);
                    logInfo("没有选中标签，创建单曲播放列表");
                    m_audioEngine->play();
                    logInfo("发送播放请求到AudioEngine");
                }
            } else {
                // 无效歌曲，直接播放
                m_audioEngine->play();
                logInfo("发送播放请求到AudioEngine");
            }
        } else {
            logError("AudioEngine为空，无法播放");
        }
    });
    
    connect(this, &MainWindowController::pauseRequested, [this]() {
        if (m_audioEngine) {
            m_audioEngine->pause();
            logInfo("发送暂停请求到AudioEngine");
        }
    });
    
    connect(this, &MainWindowController::nextRequested, [this]() {
        if (m_audioEngine) {
            m_audioEngine->playNext();
            logInfo("发送下一首请求到AudioEngine");
        }
    });
    
    connect(this, &MainWindowController::previousRequested, [this]() {
        if (m_audioEngine) {
            m_audioEngine->playPrevious();
            logInfo("发送上一首请求到AudioEngine");
        }
    });
    
    connect(this, &MainWindowController::volumeChangeRequested, [this](int volume) {
        if (m_audioEngine) {
            m_audioEngine->setVolume(volume);
            logInfo(QString("发送音量变更请求到AudioEngine: %1").arg(volume));
        }
    });
    
    // 移除重复的seekRequested连接，因为MusicProgressBar已经直接连接到AudioEngine
    // connect(this, &MainWindowController::seekRequested, [this](qint64 position) {
    //     if (m_audioEngine) {
    //         m_audioEngine->seek(position);
    //         logInfo(QString("发送跳转请求到AudioEngine: %1ms").arg(position));
    //     }
    // });
    
    connect(this, &MainWindowController::muteToggleRequested, [this]() {
        if (m_audioEngine) {
            m_audioEngine->toggleMute();
            logInfo("发送静音切换请求到AudioEngine");
        }
    });
    
    logInfo("所有信号槽连接完成");
}

// AudioEngine信号处理槽函数
void MainWindowController::onAudioStateChanged(AudioTypes::AudioState state)
{
    logInfo(QString("收到AudioEngine状态变化信号: %1").arg(static_cast<int>(state)));
    
    // 更新播放按钮状态
    updatePlayButtonUI(state == AudioTypes::AudioState::Playing);
    
    // 更新状态栏
    QString stateText;
    switch (state) {
    case AudioTypes::AudioState::Playing:
        stateText = "正在播放";
        break;
    case AudioTypes::AudioState::Paused:
        stateText = "已暂停";
        break;
    case AudioTypes::AudioState::Loading:
        stateText = "正在加载媒体文件...";
        break;
    case AudioTypes::AudioState::Error:
        stateText = "播放出错";
        break;
    default:
        stateText = "未知状态";
        break;
    }
    updateStatusBar(stateText, 2000);
    logInfo(QString("状态栏已更新为: %1").arg(stateText));
}

void MainWindowController::onCurrentSongChanged(const Song& song)
{
    logInfo(QString("当前歌曲变化: %1 - %2").arg(song.artist(), song.title()));
    
    // 检查是否在"最近播放"标签下
    QString currentTag = "";
    if (m_tagListWidget && m_tagListWidget->currentItem()) {
        currentTag = m_tagListWidget->currentItem()->text();
    }
    
    // 检查AudioEngine的播放状态，只有在实际播放时才更新播放记录
    bool isActuallyPlaying = false;
    if (m_audioEngine) {
        isActuallyPlaying = (m_audioEngine->state() == AudioTypes::AudioState::Playing);
    }
    
    logInfo(QString("当前播放状态: %1").arg(isActuallyPlaying ? "播放中" : "未播放"));
    
    // 只有在实际播放时才更新播放记录
    if (isActuallyPlaying && song.isValid()) {
        // 场景A：在"最近播放"标签外播放歌曲时
        if (currentTag != "最近播放") {
            // 立即更新时间戳
            PlayHistoryDao playHistoryDao;
            if (playHistoryDao.addPlayRecord(song.id())) {
                logInfo(QString("场景A：在标签'%1'外播放歌曲 %2，立即更新播放时间").arg(currentTag).arg(song.title()));
            }
        }
        // 场景B：在"最近播放"标签内播放歌曲时
        else if (currentTag == "最近播放") {
            // 立即更新时间戳，但不立即更新排序
            PlayHistoryDao playHistoryDao;
            if (playHistoryDao.addPlayRecord(song.id())) {
                logInfo(QString("场景B：在'最近播放'标签内播放歌曲 %1，更新播放时间但不立即排序").arg(song.title()));
                
                // 标记需要延迟排序更新
                m_needsRecentPlaySortUpdate = true;
            }
        }
    } else {
        logInfo("歌曲未实际播放，跳过播放记录更新");
    }
    
    // 更新当前歌曲信息
    updateCurrentSongInfo();
    
    // 高亮当前播放的歌曲
    if (m_songListWidget) {
        logInfo(QString("开始高亮当前播放歌曲，列表中共有 %1 首歌曲").arg(m_songListWidget->count()));
        for (int i = 0; i < m_songListWidget->count(); ++i) {
            QListWidgetItem* item = m_songListWidget->item(i);
            if (item) {
                Song itemSong = item->data(Qt::UserRole).value<Song>();
                if (itemSong.id() == song.id()) {
                    logInfo(QString("找到匹配歌曲，设置高亮，索引: %1").arg(i));
                    m_songListWidget->setCurrentItem(item);
                    item->setBackground(QColor(100, 149, 237, 100)); // 浅蓝色背景
                } else {
                    item->setBackground(QColor()); // 恢复默认背景
                }
            }
        }
    } else {
        logWarning("歌曲列表控件为空");
    }
    
    logInfo("当前歌曲变化处理完成");
}

void MainWindowController::onPlayModeChanged(AudioTypes::PlayMode mode)
{
    QString modeText;
    switch (mode) {
    case AudioTypes::PlayMode::Loop:
        modeText = "列表循环";
        break;
    case AudioTypes::PlayMode::Random:
        modeText = "随机播放";
        break;
    case AudioTypes::PlayMode::RepeatOne:
        modeText = "单曲循环";
        break;
    default:
        modeText = "未知模式";
        break;
    }
    
    // 统一更新播放模式按钮
    updatePlayModeButton();
    
    updateStatusBar(QString("播放模式: %1").arg(modeText), 2000);
    logInfo(QString("播放模式变化: %1").arg(modeText));
}

void MainWindowController::onAudioError(const QString& error)
{
    logError(QString("音频错误: %1").arg(error));
    showErrorDialog("音频播放错误", error);
    
    // 重置播放按钮状态
    if (m_playButton) {
        m_playButton->setIcon(QIcon(":/new/prefix1/images/playIcon.png"));
    }
    
    updateStatusBar("播放出错", 5000);
}

// 滑块变化处理已在前面定义

// 列表项点击处理
// 重复定义已删除 - onTagListItemClicked

void MainWindowController::onTagListItemDoubleClicked(QListWidgetItem* item)
{
    if (!item) return;
    
    QString tagName = item->text();
    logInfo(QString("双击标签: %1").arg(tagName));
    
    // 可以在这里添加标签编辑功能
}



// 工具函数
QString MainWindowController::formatTime(qint64 milliseconds) const
{
    qint64 seconds = milliseconds / 1000;
    qint64 minutes = seconds / 60;
    seconds %= 60;
    
    return QString("%1:%2")
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0'));
}

void MainWindowController::updateStatusBar(const QString& message, int timeout)
{
    if (m_mainWindow && m_mainWindow->statusBar()) {
        m_mainWindow->statusBar()->showMessage(message, timeout);
    }
}

void MainWindowController::loadSettings()
{
    // 加载默认设置
    loadDefaultSettings();
    
    // 应用设置到UI
    applySettingsToUI();
}

void MainWindowController::saveSettings()
{
    if (m_settings) {
        m_settings->setValue("MainWindow/ViewMode", static_cast<int>(m_viewMode));
        m_settings->setValue("MainWindow/SortMode", static_cast<int>(m_sortMode));
        m_settings->setValue("MainWindow/SortAscending", m_sortAscending);
        m_settings->sync();
    }
}

void MainWindowController::loadDefaultSettings()
{
    // 设置默认值
    m_viewMode = ViewMode::TagView;
    m_sortMode = SortMode::Title;
    m_sortAscending = true;
}

void MainWindowController::applySettingsToUI()
{
    // 应用设置到UI
    updateUIState();
}

void MainWindowController::updateUIState()
{
    // 更新UI状态
    updateWindowTitle();
    updateStatusMessage();
    updatePlaybackControls(); // 添加对播放控制按钮的更新
}

void MainWindowController::updateWindowTitle()
{
    if (m_mainWindow) {
        QString title = "Qt6音频播放器 - v1.0.0";
        if (m_state == MainWindowState::Playing) {
            title += " - 播放中";
        } else if (m_state == MainWindowState::Paused) {
            title += " - 暂停";
        }
        m_mainWindow->setWindowTitle(title);
    }
}

void MainWindowController::updateStatusMessage()
{
    QString message;
    switch (m_state) {
        case MainWindowState::Initializing:
            message = "正在初始化...";
            break;
        case MainWindowState::Ready:
            message = "就绪";
            break;
        case MainWindowState::Playing:
            message = "播放中";
            break;
        case MainWindowState::Paused:
            message = "暂停";
            break;
        case MainWindowState::Loading:
            message = "正在加载...";
            break;
        case MainWindowState::Error:
            message = "错误";
            break;
    }
    updateStatusBar(message);
}

void MainWindowController::refreshUI()
{
    updateUIState();
}

void MainWindowController::logError(const QString& error)
{
    Logger::instance()->error(error, "MainWindowController");
}

void MainWindowController::logInfo(const QString& message)
{
    Logger::instance()->info(message, "MainWindowController");
}

void MainWindowController::logDebug(const QString& message)
{
    Logger::instance()->debug(message, "MainWindowController");
}

void MainWindowController::logWarning(const QString& message)
{
    Logger::instance()->warning(message, "MainWindowController");
}

// 主窗口事件槽函数
void MainWindowController::onMainWindowShow()
{
    logInfo("主窗口显示");
    updateUIState();
}

void MainWindowController::onMainWindowClose()
{
    logInfo("主窗口关闭");
    shutdown();
}

void MainWindowController::onMainWindowResize()
{
    logInfo("主窗口大小调整");
    saveLayout();
}

void MainWindowController::onMainWindowMove()
{
    logInfo("主窗口移动");
    saveLayout();
}

// 标签列表事件槽函数
void MainWindowController::onTagListContextMenuRequested(const QPoint& position)
{
    logInfo("标签列表右键菜单请求");
    showTagContextMenu(position);
}

void MainWindowController::onTagListSelectionChanged()
{
    logInfo("标签列表选择变化");
    handleTagSelectionChange();
}

// 歌曲列表事件槽函数
void MainWindowController::onSongListContextMenuRequested(const QPoint& position)
{
    logInfo("歌曲列表右键菜单请求");
    showSongContextMenu(position);
}

void MainWindowController::onSongListSelectionChanged()
{
    logInfo("歌曲列表选择变化");
    handleSongSelectionChange();
}



// 标签管理事件槽函数
void MainWindowController::onTagCreated(const Tag& tag)
{
    logInfo("标签创建: " + tag.name());
    refreshTagList();
}

void MainWindowController::onTagUpdated(const Tag& tag)
{
    logInfo("标签更新: " + tag.name());
    refreshTagList();
}

void MainWindowController::onTagDeleted(int tagId, const QString& name)
{
    Q_UNUSED(tagId)
    logInfo("标签删除: " + name);
    refreshTagList();
}

void MainWindowController::onSongAddedToTag(int songId, int tagId)
{
    Q_UNUSED(songId)
    Q_UNUSED(tagId)
    logInfo("歌曲添加到标签");
    refreshSongList();
}

void MainWindowController::onSongRemovedFromTag(int songId, int tagId)
{
    Q_UNUSED(songId)
    Q_UNUSED(tagId)
    logInfo("歌曲从标签移除");
    refreshSongList();
}

// 播放列表事件槽函数
void MainWindowController::onPlaylistCreated(const Playlist& playlist)
{
    logInfo("播放列表创建: " + playlist.name());
    // 更新播放列表UI
}

void MainWindowController::onPlaylistUpdated(const Playlist& playlist)
{
    logInfo("播放列表更新: " + playlist.name());
    // 更新播放列表UI
}

void MainWindowController::onPlaylistDeleted(int playlistId, const QString& name)
{
    Q_UNUSED(playlistId)
    logInfo("播放列表删除: " + name);
    // 更新播放列表UI
}

void MainWindowController::onPlaybackStarted(const Song& song)
{
    logInfo("播放开始: " + song.title());
    setState(MainWindowState::Playing);
}

void MainWindowController::onPlaybackPaused()
{
    logInfo("播放暂停");
    setState(MainWindowState::Paused);
}

void MainWindowController::onPlaybackStopped()
{
    logInfo("播放停止");
    setState(MainWindowState::Ready);
}

// 拖放事件槽函数
void MainWindowController::onDragEnterEvent(QDragEnterEvent* event)
{
    if (!event) {
        logWarning("拖拽进入事件为空");
        return;
    }
    
    try {
        logDebug("处理拖拽进入事件");
        
        // 检查是否启用拖拽功能
        if (!m_dragDropEnabled) {
            logDebug("拖拽功能已禁用");
            event->ignore();
            return;
        }
        
        // 检查是否包含文件URL
        if (event->mimeData()->hasUrls()) {
            QList<QUrl> urls = event->mimeData()->urls();
            bool hasAudioFiles = false;
            
            // 检查是否包含支持的音频文件
            for (const QUrl& url : urls) {
                if (url.isLocalFile()) {
                    QString filePath = url.toLocalFile();
                    QString suffix = QFileInfo(filePath).suffix().toLower();
                    
                    // 支持的音频格式
                    QStringList supportedFormats = {"mp3", "wav", "flac", "ogg", "m4a", "aac", "wma"};
                    
                    if (supportedFormats.contains(suffix)) {
                        hasAudioFiles = true;
                        break;
                    }
                }
            }
            
            if (hasAudioFiles) {
                logInfo(QString("检测到 %1 个拖拽文件，包含支持的音频格式").arg(urls.size()));
                event->acceptProposedAction();
                return;
            }
        }
        
        logDebug("拖拽内容不包含支持的音频文件");
        event->ignore();
        
    } catch (const std::exception& e) {
        logError(QString("处理拖拽进入事件时发生异常: %1").arg(e.what()));
        event->ignore();
    }
}

void MainWindowController::onDropEvent(QDropEvent* event)
{
    if (!event) {
        logWarning("拖拽放下事件为空");
        return;
    }
    
    try {
        logInfo("处理拖拽放下事件");
        
        // 检查是否启用拖拽功能
        if (!m_dragDropEnabled) {
            logDebug("拖拽功能已禁用");
            event->ignore();
            return;
        }
        
        // 检查是否包含文件URL
        if (event->mimeData()->hasUrls()) {
            QList<QUrl> urls = event->mimeData()->urls();
            QStringList audioFiles;
            
            // 筛选出支持的音频文件
            for (const QUrl& url : urls) {
                if (url.isLocalFile()) {
                    QString filePath = url.toLocalFile();
                    QString suffix = QFileInfo(filePath).suffix().toLower();
                    
                    // 支持的音频格式
                    QStringList supportedFormats = {"mp3", "wav", "flac", "ogg", "m4a", "aac", "wma"};
                    
                    if (supportedFormats.contains(suffix)) {
                        audioFiles.append(filePath);
                        logDebug(QString("添加音频文件: %1").arg(filePath));
                    } else {
                        logDebug(QString("跳过不支持的文件: %1").arg(filePath));
                    }
                }
            }
            
            if (!audioFiles.isEmpty()) {
                logInfo(QString("准备添加 %1 个音频文件到音乐库").arg(audioFiles.size()));
                
                // 调用添加歌曲方法
                addSongs(audioFiles);
                
                // 更新状态栏
                updateStatusBar(QString("成功添加 %1 个音频文件").arg(audioFiles.size()), 3000);
                
                event->acceptProposedAction();
                return;
            } else {
                logWarning("拖拽的文件中没有支持的音频格式");
                updateStatusBar("没有找到支持的音频文件", 2000);
            }
        }
        
        event->ignore();
        
    } catch (const std::exception& e) {
        logError(QString("处理拖拽放下事件时发生异常: %1").arg(e.what()));
        updateStatusBar("添加文件时发生错误", 2000);
        event->ignore();
    }
}

// 缺失的私有方法实现
void MainWindowController::refreshTagList()
{
    logInfo("刷新标签列表");
    updateTagList();
}

void MainWindowController::refreshSongList()
{
    logInfo("刷新歌曲列表");
    updateSongList();
}

void MainWindowController::saveLayout()
{
    if (!m_settings) {
        logWarning("设置对象未初始化，无法保存布局");
        return;
    }
    
    try {
        // 保存主窗口几何信息
        if (m_mainWindow) {
            m_settings->setValue("MainWindow/geometry", m_mainWindow->saveGeometry());
            m_settings->setValue("MainWindow/windowState", m_mainWindow->saveState());
        }
        
        // 保存分割器状态
        if (m_splitter) {
            m_settings->setValue("MainWindow/splitterState", m_splitter->saveState());
        }
        
        // 保存音量设置
        if (m_volumeSlider) {
            m_settings->setValue("Audio/volume", m_volumeSlider->value());
        }
        
        // 保存播放模式
        if (m_audioEngine) {
            m_settings->setValue("Audio/playMode", static_cast<int>(m_audioEngine->playMode()));
        }
        
        logInfo("布局保存完成");
    } catch (const std::exception& e) {
        logError(QString("保存布局时发生错误: %1").arg(e.what()));
    }
}

void MainWindowController::restoreLayout()
{
    if (!m_settings) {
        logWarning("设置对象未初始化，无法恢复布局");
        return;
    }
    
    try {
        // 恢复主窗口几何信息
        if (m_mainWindow) {
            QByteArray geometry = m_settings->value("MainWindow/geometry").toByteArray();
            if (!geometry.isEmpty()) {
                m_mainWindow->restoreGeometry(geometry);
            }
            
            QByteArray windowState = m_settings->value("MainWindow/windowState").toByteArray();
            if (!windowState.isEmpty()) {
                m_mainWindow->restoreState(windowState);
            }
        }
        
        // 恢复分割器状态
        if (m_splitter) {
            QByteArray splitterState = m_settings->value("MainWindow/splitterState").toByteArray();
            if (!splitterState.isEmpty()) {
                m_splitter->restoreState(splitterState);
            }
        }
        
        // 恢复音量设置
        if (m_volumeSlider) {
            int volume = m_settings->value("Audio/volume", 50).toInt();
            m_volumeSlider->setValue(volume);
            if (m_audioEngine) {
                m_audioEngine->setVolume(volume);
            }
        }
        
        // 恢复播放模式
        if (m_audioEngine) {
            int playMode = m_settings->value("Audio/playMode", static_cast<int>(AudioTypes::PlayMode::Loop)).toInt();
            m_audioEngine->setPlayMode(static_cast<AudioTypes::PlayMode>(playMode));
            updatePlayModeButton();
        }
        
        logInfo("布局恢复完成");
    } catch (const std::exception& e) {
        logError(QString("恢复布局时发生错误: %1").arg(e.what()));
    }
}

void MainWindowController::resetLayout()
{
    try {
        // 重置主窗口大小和位置
        if (m_mainWindow) {
            m_mainWindow->resize(1200, 800);  // 默认大小
            m_mainWindow->move(100, 100);     // 默认位置
        }
        
        // 重置分割器比例
        if (m_splitter) {
            QList<int> sizes;
            sizes << 300 << 900;  // 标签区域:歌曲区域 = 1:3
            m_splitter->setSizes(sizes);
        }
        
        // 重置音量
        if (m_volumeSlider) {
            m_volumeSlider->setValue(50);
            if (m_audioEngine) {
                m_audioEngine->setVolume(50);
            }
        }
        
        // 重置播放模式
        if (m_audioEngine) {
            m_audioEngine->setPlayMode(AudioTypes::PlayMode::Loop);
            updatePlayModeButton();
        }
        
        // 清除设置中的布局信息
        if (m_settings) {
            m_settings->remove("MainWindow/geometry");
            m_settings->remove("MainWindow/windowState");
            m_settings->remove("MainWindow/splitterState");
        }
        
        logInfo("布局重置完成");
    } catch (const std::exception& e) {
        logError(QString("重置布局时发生错误: %1").arg(e.what()));
    }
}

void MainWindowController::showTagContextMenu(const QPoint& position)
{
    if (!m_tagListWidget) return;
    QListWidgetItem* item = m_tagListWidget->itemAt(position);
    if (!item) return;
    QString tagName = item->text();
    QMenu menu;
    QAction* editAction = menu.addAction("编辑标签");
    QAction* deleteAction = menu.addAction("删除标签");
    QAction* selected = menu.exec(m_tagListWidget->viewport()->mapToGlobal(position));
    if (selected == editAction) {
        // 弹出编辑对话框
        CreateTagDialog dialog(m_mainWindow);
        dialog.setWindowTitle("编辑标签");
        dialog.findChild<QLineEdit*>("lineEditTagName")->setText(tagName);
        // 预填图片（如有）
        TagDao tagDao;
        Tag tag = tagDao.getTagByName(tagName);
        if (!tag.coverPath().isEmpty()) {
            dialog.findChild<QLabel*>("labelImagePreview")->setPixmap(QPixmap(tag.coverPath()).scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            dialog.findChild<CreateTagDialog*>()->setProperty("m_imagePath", tag.coverPath());
        }
        if (dialog.exec() == QDialog::Accepted) {
            QString newName = dialog.getTagName();
            QString imagePath = dialog.getTagImagePath();
            if (!newName.isEmpty()) {
                editTag(tagName, newName, imagePath);
            }
        }
    } else if (selected == deleteAction) {
        if (QMessageBox::question(m_mainWindow, "删除标签", QString("确定要删除标签 '%1' 吗？").arg(tagName)) == QMessageBox::Yes) {
            deleteTag(tagName);
        }
    }
}

void MainWindowController::editTag(const QString& oldName, const QString& newName, const QString& imagePath)
{
    TagDao tagDao;
    Tag tag = tagDao.getTagByName(oldName);
    if (!tag.isValid()) {
        showErrorDialog("编辑失败", "标签不存在");
        return;
    }
    tag.setName(newName);
    tag.setCoverPath(imagePath);
    if (!tagDao.updateTag(tag)) {
        showErrorDialog("编辑失败", "数据库更新失败");
    } else {
        updateStatusBar("标签编辑成功");
        refreshTagList();
    }
}

void MainWindowController::deleteTag(const QString& name)
{
    TagDao tagDao;
    Tag tag = tagDao.getTagByName(name);
    if (!tag.isValid()) {
        showErrorDialog("删除失败", "标签不存在");
        return;
    }
    if (tag.isSystem()) {
        showErrorDialog("删除失败", "系统标签不可删除");
        return;
    }
    if (!tagDao.deleteTag(tag.id())) {
        showErrorDialog("删除失败", "数据库删除失败");
    } else {
        updateStatusBar("标签删除成功");
        refreshTagList();
    }
}

void MainWindowController::showSongContextMenu(const QPoint& position)
{
    if (!m_songListWidget) {
        logWarning("歌曲列表控件未初始化");
        return;
    }
    
    try {
        QListWidgetItem* item = m_songListWidget->itemAt(position);
        if (!item) {
            logInfo("右键点击位置没有歌曲项");
            return;
        }
        
        // 获取歌曲信息
        QVariant songData = item->data(Qt::UserRole);
        Song song = songData.value<Song>();
        int songId = song.id();
        QString songTitle = item->text();
        
        // 检查是否为"最近播放"标签下的歌曲
        bool isRecentPlayItem = (item->data(Qt::UserRole + 1).toString() == "recent_play");
        
        logInfo(QString("显示歌曲右键菜单: %1 (ID: %2, 最近播放: %3)").arg(songTitle).arg(songId).arg(isRecentPlayItem));
        
        // 创建右键菜单
        QMenu contextMenu(m_songListWidget);
        
        // 播放歌曲
        QAction* playAction = contextMenu.addAction(QIcon(":/icons/play.png"), "播放");
        connect(playAction, &QAction::triggered, [this, songId]() {
            logInfo(QString("从右键菜单播放歌曲 ID: %1").arg(songId));
            if (m_audioEngine) {
                // 根据歌曲ID获取歌曲信息并播放
                SongDao songDao;
                Song song = songDao.getSongById(songId);
                if (song.isValid()) {
                    // m_audioEngine->playSong(song);
                }
            }
        });
        
        contextMenu.addSeparator();
        
        // 添加到标签
        QAction* addToTagAction = contextMenu.addAction(QIcon(":/icons/tag_add.png"), "添加到标签...");
        connect(addToTagAction, &QAction::triggered, [this, songId, songTitle]() {
            logInfo(QString("为歌曲 %1 添加标签").arg(songTitle));
            showAddToTagDialog(songId, songTitle);
        });
        
        // 从标签移除
        QAction* removeFromTagAction = contextMenu.addAction(QIcon(":/icons/tag_remove.png"), "从当前标签移除");
        connect(removeFromTagAction, &QAction::triggered, [this, songId, songTitle]() {
            logInfo(QString("从当前标签移除歌曲 %1").arg(songTitle));
            removeFromCurrentTag(songId, songTitle);
        });
        
        contextMenu.addSeparator();
        
        // 编辑歌曲信息
        QAction* editInfoAction = contextMenu.addAction(QIcon(":/icons/edit.png"), "编辑信息...");
        connect(editInfoAction, &QAction::triggered, [this, songId, songTitle]() {
            logInfo(QString("编辑歌曲信息: %1").arg(songTitle));
            showEditSongDialog(songId, songTitle);
        });
        
        // 在文件夹中显示
        QAction* showInFolderAction = contextMenu.addAction(QIcon(":/icons/folder.png"), "在文件夹中显示");
        connect(showInFolderAction, &QAction::triggered, [this, songId, songTitle]() {
            logInfo(QString("在文件夹中显示歌曲: %1").arg(songTitle));
            showInFileExplorer(songId, songTitle);
        });
        
        contextMenu.addSeparator();
        
        // 根据是否为"最近播放"标签下的歌曲，提供不同的删除选项
        if (isRecentPlayItem) {
            // 删除播放记录（仅从最近播放列表中移除）
            QAction* deletePlayHistoryAction = contextMenu.addAction(QIcon(":/icons/delete.png"), "删除播放记录");
            connect(deletePlayHistoryAction, &QAction::triggered, [this, songId, songTitle]() {
                logInfo(QString("删除播放记录: %1").arg(songTitle));
                
                // 确认删除播放记录
                QMessageBox::StandardButton reply = QMessageBox::question(
                    m_mainWindow,
                    "确认删除播放记录",
                    QString("确定要从最近播放列表中删除 \"%1\" 的播放记录吗？\n\n注意：这只会删除播放历史记录，不会删除歌曲文件。")
                        .arg(songTitle),
                    QMessageBox::Yes | QMessageBox::No,
                    QMessageBox::No
                );
                
                if (reply == QMessageBox::Yes) {
                    deletePlayHistoryRecord(songId, songTitle);
                    logInfo(QString("用户确认删除播放记录: %1").arg(songTitle));
                }
            });
        } else {
            // 删除歌曲（从数据库中删除歌曲记录）
            QAction* deleteAction = contextMenu.addAction(QIcon(":/icons/delete.png"), "删除");
            connect(deleteAction, &QAction::triggered, [this, songId, songTitle]() {
                logInfo(QString("删除歌曲: %1").arg(songTitle));
                
                // 确认删除
                QMessageBox::StandardButton reply = QMessageBox::question(
                    m_mainWindow,
                    "确认删除",
                    QString("确定要删除歌曲 \"%1\" 吗？\n\n注意：这将从数据库中删除歌曲记录，但不会删除实际文件。")
                        .arg(songTitle),
                    QMessageBox::Yes | QMessageBox::No,
                    QMessageBox::No
                );
                
                if (reply == QMessageBox::Yes) {
                    deleteSongFromDatabase(songId, songTitle);
                    logInfo(QString("用户确认删除歌曲: %1").arg(songTitle));
                }
            });
        }
        
        // 显示菜单
        contextMenu.exec(m_songListWidget->mapToGlobal(position));
        
    } catch (const std::exception& e) {
        logError(QString("显示歌曲右键菜单时发生异常: %1").arg(e.what()));
    }
}

void MainWindowController::showPlaylistContextMenu(const QPoint& position)
{
    Q_UNUSED(position)
    logInfo("显示播放列表右键菜单");
    
    try {
        // 创建播放列表右键菜单
        QMenu contextMenu(m_mainWindow);
        
        // 创建新播放列表
        QAction* createPlaylistAction = contextMenu.addAction(QIcon(":/icons/playlist_add.png"), "创建播放列表...");
        connect(createPlaylistAction, &QAction::triggered, [this]() {
            logInfo("创建新播放列表");
            showCreatePlaylistDialog();
        });
        
        // 导入播放列表
        QAction* importPlaylistAction = contextMenu.addAction(QIcon(":/icons/import.png"), "导入播放列表...");
        connect(importPlaylistAction, &QAction::triggered, [this]() {
            logInfo("导入播放列表");
            importPlaylistFromFile();
        });
        
        contextMenu.addSeparator();
        
        // 刷新播放列表
        QAction* refreshAction = contextMenu.addAction(QIcon(":/icons/refresh.png"), "刷新");
        connect(refreshAction, &QAction::triggered, [this]() {
            logInfo("刷新播放列表");
            refreshPlaylistView();
        });
        
        // 显示菜单
        contextMenu.exec(m_mainWindow->mapToGlobal(position));
        
    } catch (const std::exception& e) {
        logError(QString("显示播放列表右键菜单时发生异常: %1").arg(e.what()));
    }
}

void MainWindowController::updateTagList()
{
    logInfo("开始更新标签列表");
    
    if (!m_tagListWidget) {
        logError("标签列表控件未初始化，无法更新标签列表");
        return;
    }
    
    try {
        logDebug("清空当前标签列表");
        m_tagListWidget->clear();
        
        // 定义系统标签信息
        struct SystemTagInfo {
            QString name;
            int id;
            QColor color;
        };
        
        QList<SystemTagInfo> systemTags = {
            {"我的歌曲", 1, QColor("#2196F3")},
            {"我的收藏", 2, QColor("#FF9800")},
            {"最近播放", 3, QColor("#4CAF50")}
        };
        
        // 添加系统标签到列表
        for (const auto& tagInfo : systemTags) {
            QListWidgetItem* item = new QListWidgetItem(tagInfo.name);
            item->setData(Qt::UserRole, tagInfo.id);
            item->setForeground(tagInfo.color);
            item->setToolTip(QString("系统标签: %1").arg(tagInfo.name));
            m_tagListWidget->addItem(item);
            
            logDebug(QString("添加系统标签: %1 (ID: %2)").arg(tagInfo.name).arg(tagInfo.id));
        }
        
        // 获取并添加用户创建的标签
        logDebug("开始获取用户标签");
        TagDao tagDao;
        auto allTags = tagDao.getAllTags();
        
        QStringList systemTagNames = {"我的歌曲", "我的收藏", "最近播放"};
        int userTagCount = 0;
        
        for (const Tag& tag : allTags) {
            // 跳过系统标签（根据标签名称判断）
            if (systemTagNames.contains(tag.name())) {
                logDebug(QString("跳过系统标签: %1").arg(tag.name()));
                continue;
            }
            
            QListWidgetItem* item = new QListWidgetItem(tag.name());
            item->setData(Qt::UserRole, tag.id());
            item->setForeground(QColor("#9C27B0")); // 紫色表示用户标签
            item->setToolTip(QString("用户标签: %1").arg(tag.name()));
            m_tagListWidget->addItem(item);
            
            userTagCount++;
            logDebug(QString("添加用户标签: %1 (ID: %2)").arg(tag.name()).arg(tag.id()));
        }
        
        // 默认选中第一个标签
        if (m_tagListWidget->count() > 0) {
            m_tagListWidget->setCurrentRow(0);
            logDebug("默认选中第一个标签");
        }
        
        logInfo(QString("标签列表更新完成，共 %1 个系统标签，%2 个用户标签").arg(systemTags.size()).arg(userTagCount));
        
    } catch (const std::exception& e) {
        logError(QString("更新标签列表时发生异常: %1").arg(e.what()));
    } catch (...) {
        logError("更新标签列表时发生未知异常");
    }
}

void MainWindowController::updateSongList()
{
    logInfo("开始更新歌曲列表");
    
    if (!m_songListWidget) {
        logError("歌曲列表控件未初始化");
        return;
    }
    
    try {
        // 获取当前选中的标签
        QString selectedTag;
        if (m_tagListWidget && m_tagListWidget->currentItem()) {
            selectedTag = m_tagListWidget->currentItem()->text();
            logInfo(QString("当前选中标签: %1").arg(selectedTag));
        } else {
            logInfo("没有选中标签");
        }
        
        // 清空当前列表
        m_songListWidget->clear();
        logDebug("已清空歌曲列表控件");
        
        // 获取歌曲列表
        QList<Song> songs;
        if (selectedTag.isEmpty() || selectedTag == "全部歌曲") {
            // 显示所有歌曲
            logInfo("获取所有歌曲");
            SongDao songDao;
            songs = songDao.getAllSongs();
            logInfo(QString("从数据库获取到 %1 首歌曲").arg(songs.size()));
        } else if (selectedTag == "最近播放") {
            // 特殊处理"最近播放"标签
            logInfo("获取最近播放的歌曲");
            PlayHistoryDao playHistoryDao;
            songs = playHistoryDao.getRecentPlayedSongs(100);
            logInfo(QString("从播放历史获取到 %1 首歌曲").arg(songs.size()));
            
            // 调试：打印获取到的歌曲列表
            logDebug("获取到的歌曲列表:");
            for (int i = 0; i < songs.size(); ++i) {
                const Song& song = songs[i];
                QDateTime playTime = song.lastPlayedTime();
                QString timeStr = playTime.toString("yyyy/MM-dd/hh-mm-ss");
                logDebug(QString("  [%1] %2 - %3  %4")
                            .arg(i + 1)
                            .arg(song.artist())
                            .arg(song.title())
                            .arg(timeStr));
            }
        } else {
            // 显示特定标签的歌曲
            logInfo(QString("获取标签'%1'的歌曲").arg(selectedTag));
            SongDao songDao;
            TagDao tagDao;
            Tag tag = tagDao.getTagByName(selectedTag);
            if (tag.isValid()) {
                logDebug(QString("找到标签，ID: %1").arg(tag.id()));
                songs = songDao.getSongsByTag(tag.id());
                logInfo(QString("标签'%1'下有 %2 首歌曲").arg(selectedTag).arg(songs.size()));
            } else {
                logWarning(QString("标签'%1'不存在").arg(selectedTag));
            }
        }
        
        // 添加歌曲到列表
        logInfo("开始添加歌曲到列表控件");
        
        if (selectedTag == "最近播放") {
            // 对于"最近播放"标签，歌曲列表已经按时间排序，直接使用
            logInfo("开始添加最近播放歌曲到UI");
            for (int i = 0; i < songs.size(); ++i) {
                const auto& song = songs[i];
                logDebug(QString("添加第%1首歌曲: ID=%2, 标题=%3, 艺术家=%4")
                            .arg(i + 1)
                            .arg(song.id())
                            .arg(song.title())
                            .arg(song.artist()));
                
                // 从歌曲数据中获取播放时间（如果可用）
                QString displayText;
                QDateTime lastPlayTime = song.lastPlayedTime();
                if (lastPlayTime.isValid()) {
                    // 格式："年/月-日/时-分-秒"
                    QString timeStr = lastPlayTime.toString("yyyy/MM-dd/hh-mm-ss");
                    displayText = QString("%1 - %2  %3").arg(song.artist(), song.title(), timeStr);
                    logDebug(QString("使用歌曲对象中的时间: %1").arg(timeStr));
                } else {
                    // 如果歌曲数据中没有播放时间，则查询数据库
                    PlayHistoryDao playHistoryDao;
                    lastPlayTime = playHistoryDao.getLastPlayTime(song.id());
                    if (lastPlayTime.isValid()) {
                        QString timeStr = lastPlayTime.toString("yyyy/MM-dd/hh-mm-ss");
                        displayText = QString("%1 - %2  %3").arg(song.artist(), song.title(), timeStr);
                        logDebug(QString("从数据库查询时间: %1").arg(timeStr));
                    } else {
                        displayText = QString("%1 - %2").arg(song.artist(), song.title());
                        logWarning("无法获取播放时间");
                    }
                }
                
                logDebug(QString("显示文本: %1").arg(displayText));
                
                QListWidgetItem* item = new QListWidgetItem();
                item->setText(displayText);
                item->setData(Qt::UserRole, QVariant::fromValue(song));
                item->setData(Qt::UserRole + 1, "recent_play"); // 标识为最近播放项
                item->setToolTip(QString("文件: %1\n时长: %2")
                               .arg(song.filePath())
                               .arg(QString::number(song.duration())));
                
                m_songListWidget->addItem(item);
            }
            logInfo(QString("最近播放歌曲添加完成，UI列表共有 %1 项").arg(m_songListWidget->count()));
        } else {
            // 对于其他标签，正常处理
            for (const auto& song : songs) {
                logDebug(QString("添加歌曲: ID=%1, 标题=%2, 艺术家=%3")
                         .arg(song.id())
                         .arg(song.title())
                         .arg(song.artist()));
                
                QString displayText = QString("%1 - %2").arg(song.artist(), song.title());
                
                QListWidgetItem* item = new QListWidgetItem();
                item->setText(displayText);
                item->setData(Qt::UserRole, QVariant::fromValue(song));
                item->setToolTip(QString("文件: %1\n时长: %2")
                               .arg(song.filePath())
                               .arg(QString::number(song.duration())));
                
                m_songListWidget->addItem(item);
            }
        }
        
        logInfo(QString("歌曲列表控件现在有 %1 个项目").arg(m_songListWidget->count()));
        
        // 更新状态栏
        updateStatusBar(QString("共 %1 首歌曲").arg(songs.size()), 3000);
        
        logInfo(QString("歌曲列表更新完成，共 %1 首歌曲").arg(songs.size()));
        
    } catch (const std::exception& e) {
        logError(QString("更新歌曲列表时发生异常: %1").arg(e.what()));
        showErrorDialog("更新歌曲列表失败", QString("发生异常: %1").arg(e.what()));
    }
    
    logInfo("歌曲列表更新完成");
}

void MainWindowController::updatePlaybackControls()
{
    if (!m_audioEngine || !m_mainWindow) return;
    
    // 更新播放/暂停按钮状态
    if (m_playButton) {
        bool isPlaying = (m_audioEngine->state() == AudioTypes::AudioState::Playing);
        m_playButton->setIcon(QIcon(isPlaying ? ":/new/prefix1/images/pauseIcon.png" : ":/new/prefix1/images/playIcon.png"));
    }
    
    // 进度条更新由自定义MusicProgressBar组件处理
    
    // 更新音量滑块
    if (m_volumeSlider) {
        m_volumeSlider->setValue(m_audioEngine->volume());
    }
    
    logInfo("播放控件状态更新完成");
}

void MainWindowController::updateVolumeControls()
{
    if (!m_audioEngine || !m_volumeSlider) return;
    
    // 更新音量滑块
    int volume = m_audioEngine->volume();
    m_volumeSlider->blockSignals(true);
    m_volumeSlider->setValue(volume);
    m_volumeSlider->blockSignals(false);
    
    // 更新静音按钮状态
    if (m_muteButton) {
        bool isMuted = m_audioEngine->isMuted();
        m_muteButton->setText(isMuted ? "取消静音" : "静音");
        m_muteButton->setIcon(QIcon(isMuted ? ":/images/volume_muted.png" : ":/images/volume.png"));
    }
    
    logInfo(QString("音量控件更新完成，音量: %1").arg(volume));
}

void MainWindowController::updateProgressControls()
{
    // 进度条更新由自定义MusicProgressBar组件处理
    logDebug("进度控件更新已由MusicProgressBar组件接管");
}

void MainWindowController::updateCurrentSongInfo()
{
    logInfo("开始更新当前歌曲信息");
    
    if (!m_audioEngine) {
        logError("音频引擎为空");
        return;
    }
    
    logDebug("获取当前歌曲");
    Song currentSong = m_audioEngine->currentSong();
    logDebug(QString("当前歌曲是否有效: %1").arg(currentSong.isValid()));
    
    if (currentSong.isValid()) {
        // 使用FFmpeg解析的元数据
        QString artist = currentSong.artist();
        QString title = currentSong.title();
        
        // 如果元数据为空，尝试从文件重新解析
        if (artist.isEmpty() || title.isEmpty()) {
            Song updatedSong = currentSong;
            Song::extractAdvancedMetadata(updatedSong, currentSong.filePath());
            artist = updatedSong.artist();
            title = updatedSong.title();
        }
        
        // 分别更新歌曲标题和艺术家标签
        QLabel* titleLabel = m_mainWindow->findChild<QLabel*>("label_song_title");
        QLabel* artistLabel = m_mainWindow->findChild<QLabel*>("label_song_artist");
        
        if (titleLabel) {
            if (!title.isEmpty()) {
                titleLabel->setText(title);
            } else {
                QFileInfo fileInfo(currentSong.filePath());
                titleLabel->setText(fileInfo.baseName());
            }
        }
        
        if (artistLabel) {
            artistLabel->setText(artist.isEmpty() ? "" : artist);
        }
        
        // 更新合并的歌曲信息（用于状态栏和窗口标题）
        QString songInfo;
        if (!artist.isEmpty() && !title.isEmpty()) {
            songInfo = QString("%1 - %2").arg(artist, title);
        } else if (!title.isEmpty()) {
            songInfo = title;
        } else {
            QFileInfo fileInfo(currentSong.filePath());
            songInfo = fileInfo.baseName();
        }
        
        logInfo(QString("歌曲信息: %1").arg(songInfo));
        
        // 更新窗口标题
        logDebug("更新窗口标题");
        if (m_mainWindow) {
            QString title = QString("Qt6音频播放器 - %1").arg(songInfo);
            logDebug(QString("设置窗口标题: %1").arg(title));
            m_mainWindow->setWindowTitle(title);
            logDebug("窗口标题设置完成");
        } else {
            logWarning("主窗口为空");
        }
        
        logInfo(QString("当前歌曲信息更新: %1").arg(songInfo));
    } else {
        qDebug() << "[MainWindowController::updateCurrentSongInfo] 没有有效歌曲，设置默认信息";
        
        // 清空播放信息
        QLabel* titleLabel = m_mainWindow->findChild<QLabel*>("label_song_title");
        QLabel* artistLabel = m_mainWindow->findChild<QLabel*>("label_song_artist");
        
        if (titleLabel) {
            titleLabel->setText("未选择歌曲");
        }
        if (artistLabel) {
            artistLabel->setText("");
        }
        
        if (m_mainWindow) {
            qDebug() << "[MainWindowController::updateCurrentSongInfo] 设置默认窗口标题";
            m_mainWindow->setWindowTitle("Qt6音频播放器");
        }
        logInfo("清空当前歌曲信息");
    }
    
    qDebug() << "[MainWindowController::updateCurrentSongInfo] 当前歌曲信息更新完成";
}

void MainWindowController::updatePlayModeButton()
{
    if (!m_audioEngine) return;
    
    AudioTypes::PlayMode mode = m_audioEngine->playMode();
    QString text;
    QIcon icon;
    switch (mode) {
    case AudioTypes::PlayMode::Loop:
        text = tr("列表循环");
        icon = QIcon(":/new/prefix1/images/listCycle.png");
        break;
    case AudioTypes::PlayMode::Random:
        text = tr("随机播放");
        icon = QIcon(":/new/prefix1/images/shufflePlay.png");
        break;
    case AudioTypes::PlayMode::RepeatOne:
        text = tr("单曲循环");
        icon = QIcon(":/new/prefix1/images/singleCycle.png");
        break;
    default:
        text = tr("列表循环");
        icon = QIcon(":/new/prefix1/images/listCycle.png");
        break;
    }
    
    QString tooltip = QString("播放模式：%1").arg(text);
    
    // 更新主窗口底部的播放模式按钮
    if (m_playModeButton) {
        m_playModeButton->setText(""); // 不显示文字，只显示图标
        m_playModeButton->setIcon(icon);
        m_playModeButton->setToolTip(tooltip);
    }
    
    // 注意：pushButton_shuffle已被删除，不再更新
    
    // 同时更新PlayInterfaceController的播放模式按钮
    // 注意：m_playInterfaceController可能为空，因为它是在PlayInterface创建时才初始化的
    // 而不是在MainWindowController初始化时
    if (m_playInterfaceController && m_playInterfaceController.get()) {
        int modeIndex = 0; // 默认列表循环
        switch (mode) {
            case AudioTypes::PlayMode::Loop:
                modeIndex = 0; // 列表循环
                break;
            case AudioTypes::PlayMode::Random:
                modeIndex = 1; // 随机播放
                break;
            case AudioTypes::PlayMode::RepeatOne:
                modeIndex = 2; // 单曲循环
                break;
            default:
                modeIndex = 0; // 默认为列表循环
                break;
        }
        try {
            m_playInterfaceController->updatePlayModeButton(modeIndex);
        } catch (const std::exception& e) {
            logError(QString("更新播放界面播放模式按钮失败: %1").arg(e.what()));
        }
    }
}

void MainWindowController::handleTagSelectionChange()
{
    if (!m_tagListWidget) {
        logWarning("标签列表控件未初始化");
        return;
    }
    
    try {
        QListWidgetItem* currentItem = m_tagListWidget->currentItem();
        if (currentItem) {
            // 获取选中的标签信息
            QVariant tagData = currentItem->data(Qt::UserRole);
            int tagId = tagData.toInt();
            QString tagName = currentItem->text();
            
            // 检查是否是标签切换
            bool isTagSwitch = (m_lastActiveTag != tagName);
            
            if (isTagSwitch) {
                // 保存当前播放列表状态
                if (m_audioEngine && !m_audioEngine->playlist().isEmpty()) {
                    m_lastPlaylist = m_audioEngine->playlist();
                    m_shouldKeepPlaylist = true;
                    logInfo(QString("标签切换，保存播放列表: %1 首歌曲").arg(m_lastPlaylist.size()));
                }
                
                // 场景B触发条件1：用户切换到其他标签并播放歌曲
                // 如果在"最近播放"标签内有待更新的排序，现在触发更新
                if (m_needsRecentPlaySortUpdate && m_lastActiveTag == "最近播放") {
                    logInfo("场景B触发条件1：用户切换到其他标签，触发最近播放排序更新");
                    m_needsRecentPlaySortUpdate = false;
                    // 延迟一点时间后重新加载歌曲列表，确保数据库更新完成
                    QTimer::singleShot(100, [this]() {
                        updateSongList();
                        logInfo("最近播放列表已重新排序");
                    });
                }
                
                // 更新歌曲列表显示对应标签的歌曲
                updateSongList();
                
                // 重置播放列表保持标志（用户切换标签时）
                m_playlistChangedByUser = false;
            }
            
            // 更新状态栏
            if (tagId == -1) {
                updateStatusBar("显示所有歌曲", 2000);
            } else {
                updateStatusBar(QString("显示标签 '%1' 的歌曲").arg(tagName), 2000);
            }
            
            // 发送标签选择变化信号
            if (tagId != -1) {
                Tag selectedTag;
                selectedTag.setId(tagId);
                selectedTag.setName(tagName);
                emit tagSelectionChanged(selectedTag);
            }
            
            // 更新最后活跃的标签
            m_lastActiveTag = tagName;
            
            logInfo(QString("标签选择变化: %1 (ID: %2), 标签切换: %3").arg(tagName).arg(tagId).arg(isTagSwitch));
        } else {
            // 没有选中任何标签
            updateStatusBar("未选择标签", 1000);
            logInfo("清空标签选择");
        }
    } catch (const std::exception& e) {
        logError(QString("处理标签选择变化时发生错误: %1").arg(e.what()));
    }
}

void MainWindowController::handleSongSelectionChange()
{
    if (!m_songListWidget) {
        logWarning("歌曲列表控件未初始化");
        return;
    }
    
    try {
        QListWidgetItem* currentItem = m_songListWidget->currentItem();
        if (currentItem) {
            // 获取选中的歌曲信息
            QVariant songData = currentItem->data(Qt::UserRole);
            int songId = songData.toInt();
            QString songTitle = currentItem->text();
            
            // 更新状态栏显示选中的歌曲信息
            updateStatusBar(QString("选中歌曲: %1").arg(songTitle), 2000);
            
            // 发送歌曲选择变化信号
            if (songId > 0) {
                Song selectedSong;
                selectedSong.setId(songId);
                selectedSong.setTitle(songTitle);
                emit songSelectionChanged(selectedSong);
            }
            
            logInfo(QString("歌曲选择变化: %1 (ID: %2)").arg(songTitle).arg(songId));
        } else {
            // 没有选中任何歌曲
            updateStatusBar("未选择歌曲", 1000);
            logInfo("清空歌曲选择");
        }
    } catch (const std::exception& e) {
        logError(QString("处理歌曲选择变化时发生错误: %1").arg(e.what()));
    }
}

void MainWindowController::handlePlaybackStateChange()
{
    if (!m_audioEngine) {
        logWarning("音频引擎未初始化");
        return;
    }
    
    try {
        AudioTypes::AudioState currentState = m_audioEngine->state();
        
        // 更新播放控制按钮状态
        updatePlaybackControls();
        
        // 根据播放状态更新UI
        switch (currentState) {
        case AudioTypes::AudioState::Playing:
            setState(MainWindowState::Playing);
            updateStatusBar("正在播放", 1000);
            break;
        case AudioTypes::AudioState::Paused:
            setState(MainWindowState::Paused);
            updateStatusBar("已暂停", 1000);
            break;
        case AudioTypes::AudioState::Loading:
            setState(MainWindowState::Loading);
            updateStatusBar("正在加载...", 1000);
            break;
        case AudioTypes::AudioState::Error:
            setState(MainWindowState::Error);
            updateStatusBar("播放错误", 3000);
            break;
        default:
            setState(MainWindowState::Ready);
            updateStatusBar("就绪", 1000);
            break;
        }
        
        logInfo(QString("播放状态变化: %1").arg(static_cast<int>(currentState)));
    } catch (const std::exception& e) {
        logError(QString("处理播放状态变化时发生错误: %1").arg(e.what()));
    }
}

void MainWindowController::handleAudioEngineError(const QString& error)
{
    logError("处理音频引擎错误: " + error);
    handleError(error);
}

void MainWindowController::addSongs(const QStringList& filePaths)
{
    addSongs(filePaths, QMap<QString, QStringList>());
}

void MainWindowController::addSongs(const QStringList& filePaths, const QMap<QString, QStringList>& fileTagAssignments)
{
    logInfo(QString("批量添加音乐: %1 个文件").arg(filePaths.size()));
    if (filePaths.isEmpty()) return;
    
    QList<Song> songs;
    for (const QString& path : filePaths) {
        Song song = Song::fromFile(path);
        if (song.isValid()) {
            songs.append(song);
        } else {
            logInfo(QString("无效文件: %1").arg(path));
        }
    }
    
    if (songs.isEmpty()) {
        showErrorDialog("添加失败", "没有有效的音频文件。");
        return;
    }
    
    // 先将歌曲插入数据库
    SongDao songDao;
    int inserted = songDao.insertSongs(songs);
    
    if (inserted > 0) {
        // 处理标签关联
        if (!fileTagAssignments.isEmpty()) {
            TagManager* tagManager = TagManager::instance();
            if (tagManager) {
                for (auto it = fileTagAssignments.constBegin(); it != fileTagAssignments.constEnd(); ++it) {
                    const QString& filePath = it.key();
                    const QStringList& tags = it.value();
                    
                    // 根据文件路径查找歌曲ID
                    Song song = songDao.getSongByPath(filePath);
                    if (song.isValid()) {
                        for (const QString& tagName : tags) {
                            if (!tagName.isEmpty()) {
                                // 确保标签存在
                                if (!tagManager->tagExists(tagName)) {
                                    auto result = tagManager->createTag(tagName);
                                    if (!result.success) {
                                        logWarning(QString("创建标签失败: %1 - %2").arg(tagName).arg(result.message));
                                        continue;
                                    }
                                }
                                
                                // 将歌曲添加到标签
                                Tag tag = tagManager->getTagByName(tagName);
                                if (!tag.isValid()) {
                                    logWarning(QString("标签不存在: %1").arg(tagName));
                                    continue;
                                }
                                auto result = tagManager->addSongToTag(song.id(), tag.id());
                                if (!result.success) {
                                    logWarning(QString("添加歌曲到标签失败: %1 - %2").arg(tagName).arg(result.message));
                                }
                            }
                        }
                    }
                }
            }
        }
        
        updateStatusBar(QString("成功添加 %1 首歌曲。" ).arg(inserted));
        refreshSongList();
        refreshTagList(); // 刷新标签列表以显示新的标签关联
    } else {
        showErrorDialog("添加失败", "歌曲添加到数据库失败。");
    }
}

void MainWindowController::addTag(const QString& name, const QString& imagePath)
{
    qDebug() << "[MainWindowController] addTag: 创建标签:" << name << ", 图片:" << imagePath;
    
    TagManager* tagManager = TagManager::instance();
    if (tagManager->tagExists(name)) {
        showErrorDialog("标签已存在", "该标签名已存在，请更换。");
        return;
    }
    
    Tag tag;
    tag.setName(name);
    tag.setCoverPath(imagePath);
    tag.setTagType(Tag::UserTag);
    tag.setCreatedAt(QDateTime::currentDateTime());
    tag.setUpdatedAt(QDateTime::currentDateTime());
    
    auto result = tagManager->createTag(name, QString(), QColor(), QPixmap(imagePath));
    if (result.success) {
        updateStatusBar("标签创建成功");
        qDebug() << "[MainWindowController] addTag: 标签创建成功，刷新标签列表";
        // 刷新标签列表以显示新创建的标签
        updateTagList();
    } else {
        showErrorDialog("创建失败", result.message);
    }
}

void MainWindowController::togglePlayMode()
{
    if (!m_audioEngine) return;
    AudioTypes::PlayMode mode = m_audioEngine->playMode();
    using AudioTypes::PlayMode;
    switch (mode) {
    case PlayMode::Loop:
        m_audioEngine->setPlayMode(PlayMode::RepeatOne);
        break;
    case PlayMode::RepeatOne:
        m_audioEngine->setPlayMode(PlayMode::Random);
        break;
    case PlayMode::Random:
    default:
        m_audioEngine->setPlayMode(PlayMode::Loop);
        break;
    }
    updatePlayModeButton();
}

Song MainWindowController::getCurrentSong() const
{
    return m_audioEngine ? m_audioEngine->currentSong() : Song();
}
int MainWindowController::getCurrentVolume() const
{
    return m_audioEngine ? m_audioEngine->volume() : 0;
}
qint64 MainWindowController::getCurrentPosition() const
{
    return m_audioEngine ? m_audioEngine->position() : 0;
}
qint64 MainWindowController::getCurrentDuration() const
{
    return m_audioEngine ? m_audioEngine->duration() : 0;
}
void MainWindowController::setCurrentVolume(int volume)
{
    if (m_audioEngine) m_audioEngine->setVolume(volume);
}
void MainWindowController::refreshWindowTitle()
{
    updateWindowTitle();
}

void MainWindowController::refreshTagListPublic()
{
    updateTagList();
}

void MainWindowController::editTagFromMainWindow(const QString& tagName)
{
    // 检查是否为系统标签
    if (tagName == "我的歌曲" || tagName == "我的收藏" || tagName == "最近播放") {
        QMessageBox::warning(m_mainWindow, "警告", "系统标签不能编辑！");
        return;
    }
    
    // 获取标签信息
    TagDao tagDao;
    Tag tag = tagDao.getTagByName(tagName);
    if (tag.id() == -1) {
        QMessageBox::warning(m_mainWindow, "错误", "标签不存在！");
        return;
    }
    
    // 创建编辑对话框
    CreateTagDialog dialog(m_mainWindow);
    dialog.setWindowTitle("编辑标签");
    dialog.setTagName(tag.name());
    dialog.setImagePath(tag.coverPath());
    
    if (dialog.exec() == QDialog::Accepted) {
        QString newName = dialog.getTagName().trimmed();
        QString newImagePath = dialog.getTagImagePath();
        
        // 验证新标签名
        if (newName.isEmpty()) {
            QMessageBox::warning(m_mainWindow, "错误", "标签名不能为空！");
            return;
        }
        
        // 检查是否与其他标签重名（除了自己）
        if (newName != tagName && tagDao.getTagByName(newName).id() != -1) {
            QMessageBox::warning(m_mainWindow, "错误", "标签名已存在！");
            return;
        }
        
        // 调用编辑方法
        editTag(tagName, newName, newImagePath);
    }
}

// 歌曲操作方法实现
void MainWindowController::showAddToTagDialog(int songId, const QString& songTitle)
{
    logInfo(QString("显示添加到标签对话框: 歌曲ID=%1, 标题=%2").arg(songId).arg(songTitle));
    
    try {
        // 获取所有可用标签
        TagDao tagDao;
        SongDao songDao;
        QList<Tag> allTags = tagDao.getAllTags();
        
        if (allTags.isEmpty()) {
            QMessageBox::information(m_mainWindow, "提示", "没有可用的标签，请先创建标签");
            return;
        }
        
        // 获取歌曲当前所属的标签
        QList<Tag> currentTags = m_tagManager->getTagsForSong(songId);
        QSet<int> currentTagIds;
        for (const Tag& tag : currentTags) {
            currentTagIds.insert(tag.id());
        }
        
        // 创建标签选择对话框
        QDialog dialog(m_mainWindow);
        dialog.setWindowTitle(QString("为歌曲 '%1' 添加标签").arg(songTitle));
        dialog.setModal(true);
        dialog.resize(400, 300);
        
        QVBoxLayout* layout = new QVBoxLayout(&dialog);
        
        QLabel* label = new QLabel(QString("选择要添加的标签:"));
        layout->addWidget(label);
        
        QListWidget* tagList = new QListWidget();
        tagList->setSelectionMode(QAbstractItemView::MultiSelection);
        
        // 添加标签到列表（排除已有的标签）
        for (const Tag& tag : allTags) {
            if (!currentTagIds.contains(tag.id())) {
                QListWidgetItem* item = new QListWidgetItem(tag.name());
                item->setData(Qt::UserRole, tag.id());
                tagList->addItem(item);
            }
        }
        
        layout->addWidget(tagList);
        
        // 按钮
        QHBoxLayout* buttonLayout = new QHBoxLayout();
        QPushButton* okButton = new QPushButton("确定");
        QPushButton* cancelButton = new QPushButton("取消");
        
        connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
        connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);
        
        buttonLayout->addStretch();
        buttonLayout->addWidget(okButton);
        buttonLayout->addWidget(cancelButton);
        layout->addLayout(buttonLayout);
        
        if (dialog.exec() == QDialog::Accepted) {
            QList<QListWidgetItem*> selectedItems = tagList->selectedItems();
            if (selectedItems.isEmpty()) {
                QMessageBox::information(m_mainWindow, "提示", "请选择至少一个标签");
                return;
            }
            
            // 添加歌曲到选中的标签
            int successCount = 0;
            for (QListWidgetItem* item : selectedItems) {
                int tagId = item->data(Qt::UserRole).toInt();
                if (songDao.addSongToTag(songId, tagId)) {
                    successCount++;
                    logInfo(QString("歌曲 %1 已添加到标签 %2").arg(songId).arg(tagId));
                }
            }
            
            if (successCount > 0) {
                updateStatusBar(QString("歌曲已添加到 %1 个标签").arg(successCount), 3000);
                refreshSongList(); // 刷新歌曲列表
            } else {
                QMessageBox::warning(m_mainWindow, "错误", "添加标签失败");
            }
        }
        
    } catch (const std::exception& e) {
        logError(QString("显示添加到标签对话框时发生异常: %1").arg(e.what()));
        QMessageBox::critical(m_mainWindow, "错误", "显示对话框时发生错误");
    }
}

bool MainWindowController::removeFromCurrentTag(int songId, const QString& songTitle)
{
    logInfo(QString("从当前标签移除歌曲: 歌曲ID=%1, 标题=%2").arg(songId).arg(songTitle));
    
    try {
        // 获取当前选中的标签
        Tag currentTag = getSelectedTag();
        if (currentTag.id() == -1) {
            QMessageBox::information(m_mainWindow, "提示", "请先选择一个标签");
            return false;
        }
        
        // 确认移除操作
        int ret = QMessageBox::question(m_mainWindow, "确认移除", 
            QString("确定要从标签 '%1' 中移除歌曲 '%2' 吗？")
            .arg(currentTag.name()).arg(songTitle),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            
        if (ret == QMessageBox::Yes) {
            SongDao songDao;
            if (songDao.removeSongFromTag(songId, currentTag.id())) {
                logInfo(QString("歌曲 %1 已从标签 %2 移除").arg(songId).arg(currentTag.id()));
                updateStatusBar(QString("歌曲已从标签 '%1' 移除").arg(currentTag.name()), 3000);
                refreshSongList(); // 刷新歌曲列表
                return true;
            } else {
                logError(QString("移除歌曲失败: 歌曲ID=%1, 标签ID=%2").arg(songId).arg(currentTag.id()));
                QMessageBox::critical(m_mainWindow, "错误", "移除歌曲失败");
                return false;
            }
        }
        
        return false;
        
    } catch (const std::exception& e) {
        logError(QString("移除歌曲时发生异常: %1").arg(e.what()));
        QMessageBox::critical(m_mainWindow, "错误", "移除歌曲时发生错误");
        return false;
    }
}

void MainWindowController::showEditSongDialog(int songId, const QString& songTitle)
{
    logInfo(QString("显示编辑歌曲对话框: 歌曲ID=%1, 标题=%2").arg(songId).arg(songTitle));
    
    try {
        // 获取歌曲信息
        SongDao songDao;
        Song song = songDao.getSongById(songId);
        
        if (song.id() == -1) {
            QMessageBox::warning(m_mainWindow, "错误", "歌曲不存在");
            return;
        }
        
        // 创建编辑对话框
        QDialog dialog(m_mainWindow);
        dialog.setWindowTitle(QString("编辑歌曲信息: %1").arg(songTitle));
        dialog.setModal(true);
        dialog.resize(500, 400);
        
        QVBoxLayout* layout = new QVBoxLayout(&dialog);
        
        // 创建表单
        QFormLayout* formLayout = new QFormLayout();
        
        QLineEdit* titleEdit = new QLineEdit(song.title());
        QLineEdit* artistEdit = new QLineEdit(song.artist());
        QLineEdit* albumEdit = new QLineEdit(song.album());
        QLineEdit* genreEdit = new QLineEdit(song.genre());
        QSpinBox* yearSpin = new QSpinBox();
        yearSpin->setRange(1900, 2100);
        yearSpin->setValue(song.year());
        
        formLayout->addRow("标题:", titleEdit);
        formLayout->addRow("艺术家:", artistEdit);
        formLayout->addRow("专辑:", albumEdit);
        formLayout->addRow("流派:", genreEdit);
        formLayout->addRow("年份:", yearSpin);
        
        layout->addLayout(formLayout);
        
        // 按钮
        QHBoxLayout* buttonLayout = new QHBoxLayout();
        QPushButton* okButton = new QPushButton("保存");
        QPushButton* cancelButton = new QPushButton("取消");
        
        connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
        connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);
        
        buttonLayout->addStretch();
        buttonLayout->addWidget(okButton);
        buttonLayout->addWidget(cancelButton);
        layout->addLayout(buttonLayout);
        
        if (dialog.exec() == QDialog::Accepted) {
            // 更新歌曲信息
            song.setTitle(titleEdit->text().trimmed());
            song.setArtist(artistEdit->text().trimmed());
            song.setAlbum(albumEdit->text().trimmed());
            song.setGenre(genreEdit->text().trimmed());
            song.setYear(yearSpin->value());
            
            if (songDao.updateSong(song)) {
                logInfo(QString("歌曲信息更新成功: %1").arg(songId));
                updateStatusBar("歌曲信息已更新", 3000);
                refreshSongList(); // 刷新歌曲列表
            } else {
                logError(QString("更新歌曲信息失败: %1").arg(songId));
                QMessageBox::critical(m_mainWindow, "错误", "更新歌曲信息失败");
            }
        }
        
    } catch (const std::exception& e) {
        logError(QString("显示编辑歌曲对话框时发生异常: %1").arg(e.what()));
        QMessageBox::critical(m_mainWindow, "错误", "显示对话框时发生错误");
    }
}

void MainWindowController::showInFileExplorer(int songId, const QString& songTitle)
{
    logInfo(QString("在文件夹中显示歌曲: 歌曲ID=%1, 标题=%2").arg(songId).arg(songTitle));
    
    try {
        // 获取歌曲文件路径
        SongDao songDao;
        Song song = songDao.getSongById(songId);
        
        if (song.id() == -1) {
            QMessageBox::warning(m_mainWindow, "错误", "歌曲不存在");
            return;
        }
        
        QString filePath = song.filePath();
        if (filePath.isEmpty()) {
            QMessageBox::warning(m_mainWindow, "错误", "歌曲文件路径为空");
            return;
        }
        
        // 检查文件是否存在
        QFileInfo fileInfo(filePath);
        if (!fileInfo.exists()) {
            QMessageBox::warning(m_mainWindow, "文件不存在", 
                QString("文件 '%1' 不存在，可能已被移动或删除").arg(filePath));
            return;
        }
        
        // 在文件管理器中显示文件
#ifdef Q_OS_WIN
        QStringList args;
        args << "/select," << QDir::toNativeSeparators(filePath);
        QProcess::startDetached("explorer", args);
#elif defined(Q_OS_MAC)
        QStringList args;
        args << "-e" << "tell application \"Finder\" to reveal POSIX file \"" + filePath + "\"";
        QProcess::startDetached("osascript", args);
#else
        // Linux - 打开包含文件的目录
        QDesktopServices::openUrl(QUrl::fromLocalFile(fileInfo.absolutePath()));
#endif
        
        logInfo(QString("已在文件管理器中显示文件: %1").arg(filePath));
        updateStatusBar("已在文件管理器中显示文件", 3000);
        
    } catch (const std::exception& e) {
        logError(QString("在文件管理器中显示文件时发生异常: %1").arg(e.what()));
        QMessageBox::critical(m_mainWindow, "错误", "显示文件时发生错误");
    }
}

void MainWindowController::deleteSongFromDatabase(int songId, const QString& songTitle)
{
    logInfo(QString("从数据库删除歌曲: 歌曲ID=%1, 标题=%2").arg(songId).arg(songTitle));
    
    try {
        // 确认删除操作
        int ret = QMessageBox::question(m_mainWindow, "确认删除", 
            QString("确定要从数据库中删除歌曲 '%1' 吗？\n\n注意：这将删除歌曲记录及其所有标签关联，但不会删除实际文件。")
            .arg(songTitle),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            
        if (ret == QMessageBox::Yes) {
            SongDao songDao;
            
            // 开始事务
            QSqlDatabase db = DatabaseManager::instance()->database();
            db.transaction();
            
            // 先删除歌曲与标签的关联
            if (!songDao.removeAllTagsFromSong(songId)) {
                db.rollback();
                logError(QString("删除歌曲标签关联失败: %1").arg(songId));
                QMessageBox::critical(m_mainWindow, "错误", "删除歌曲失败：无法移除标签关联");
                return;
            }
            
            // 删除歌曲记录
            if (!songDao.deleteSong(songId)) {
                db.rollback();
                logError(QString("删除歌曲记录失败: %1").arg(songId));
                QMessageBox::critical(m_mainWindow, "错误", "删除歌曲失败：无法删除歌曲记录");
                return;
            }
            
            // 提交事务
            if (!db.commit()) {
                db.rollback();
                logError(QString("提交删除歌曲事务失败: %1").arg(songId));
                QMessageBox::critical(m_mainWindow, "错误", "删除歌曲失败：事务提交失败");
                return;
            }
            
            logInfo(QString("歌曲删除成功: %1").arg(songId));
            updateStatusBar(QString("歌曲 '%1' 已删除").arg(songTitle), 3000);
            
            // 刷新歌曲列表
            refreshSongList();
        }
        
    } catch (const std::exception& e) {
        // 回滚事务
        QSqlDatabase db = DatabaseManager::instance()->database();
        if (db.isOpen()) {
            db.rollback();
        }
        
        logError(QString("删除歌曲时发生异常: %1").arg(e.what()));
        QMessageBox::critical(m_mainWindow, "错误", "删除歌曲时发生错误");
    }
}

bool MainWindowController::deletePlayHistoryRecord(int songId, const QString& songTitle)
{
    logInfo(QString("删除播放记录: 歌曲ID=%1, 标题=%2").arg(songId).arg(songTitle));
    
    try {
        PlayHistoryDao playHistoryDao;
        
        // 删除播放记录
        if (playHistoryDao.deleteSongPlayHistory(songId)) {
            logInfo(QString("播放记录删除成功: %1").arg(songId));
            updateStatusBar(QString("已删除 '%1' 的播放记录").arg(songTitle), 3000);
            
            // 刷新歌曲列表（如果当前在"最近播放"标签下）
            if (m_tagListWidget && m_tagListWidget->currentItem() && 
                m_tagListWidget->currentItem()->text() == "最近播放") {
                updateSongList();
            }
            return true;
        } else {
            logError(QString("删除播放记录失败: %1").arg(songId));
            QMessageBox::warning(m_mainWindow, "警告", "删除播放记录失败");
            return false;
        }
        
    } catch (const std::exception& e) {
        logError(QString("删除播放记录时发生异常: %1").arg(e.what()));
        QMessageBox::critical(m_mainWindow, "错误", "删除播放记录时发生错误");
        return false;
    }
}

void MainWindowController::deleteSelectedPlayHistoryRecords(const QList<QListWidgetItem*>& items)
{
    logInfo(QString("批量删除播放记录，共 %1 首歌曲").arg(items.size()));
    
    int successCount = 0;
    int failureCount = 0;
    
    // 直接使用PlayHistoryDao进行批量删除，不显示确认对话框
    PlayHistoryDao playHistoryDao;
    
    for (auto item : items) {
        QVariant songData = item->data(Qt::UserRole);
        Song song = songData.value<Song>();
        
        if (song.isValid()) {
            try {
                if (playHistoryDao.deleteSongPlayHistory(song.id())) {
                    successCount++;
                    logInfo(QString("播放记录删除成功: 歌曲ID=%1").arg(song.id()));
                } else {
                    failureCount++;
                    logError(QString("删除播放记录失败: 歌曲ID=%1").arg(song.id()));
                }
            } catch (const std::exception& e) {
                failureCount++;
                logError(QString("删除播放记录时发生异常: 歌曲ID=%1, 错误=%2").arg(song.id()).arg(e.what()));
            }
        } else {
            failureCount++;
            logWarning(QString("歌曲数据无效，跳过删除"));
        }
    }
    
    // 刷新歌曲列表（如果当前在"最近播放"标签下）
    if (m_tagListWidget && m_tagListWidget->currentItem() && 
        m_tagListWidget->currentItem()->text() == "最近播放") {
        updateSongList();
    }
    
    QString resultMessage;
    if (failureCount == 0) {
        resultMessage = QString("成功删除 %1 首歌曲的播放记录").arg(successCount);
    } else {
        resultMessage = QString("删除播放记录完成：成功 %1 首，失败 %2 首").arg(successCount).arg(failureCount);
    }
    updateStatusBar(resultMessage, 3000);
}

void MainWindowController::deleteSelectedPlayHistoryRecordsBySongs(const QList<Song>& songs)
{
    logInfo(QString("批量删除播放记录，共 %1 首歌曲").arg(songs.size()));
    
    int successCount = 0;
    int failureCount = 0;
    
    // 直接使用PlayHistoryDao进行批量删除，不显示确认对话框
    PlayHistoryDao playHistoryDao;
    
    for (const Song& song : songs) {
        if (song.isValid()) {
            try {
                if (playHistoryDao.deleteSongPlayHistory(song.id())) {
                    successCount++;
                    logInfo(QString("播放记录删除成功: 歌曲ID=%1").arg(song.id()));
                } else {
                    failureCount++;
                    logError(QString("删除播放记录失败: 歌曲ID=%1").arg(song.id()));
                }
            } catch (const std::exception& e) {
                failureCount++;
                logError(QString("删除播放记录时发生异常: 歌曲ID=%1, 错误=%2").arg(song.id()).arg(e.what()));
            }
        } else {
            failureCount++;
            logWarning(QString("歌曲数据无效，跳过删除"));
        }
    }
    
    // 刷新歌曲列表（如果当前在"最近播放"标签下）
    if (m_tagListWidget && m_tagListWidget->currentItem() && 
        m_tagListWidget->currentItem()->text() == "最近播放") {
        updateSongList();
    }
    
    QString resultMessage;
    if (failureCount == 0) {
        resultMessage = QString("成功删除 %1 首歌曲的播放记录").arg(successCount);
    } else {
        resultMessage = QString("删除播放记录完成：成功 %1 首，失败 %2 首").arg(successCount).arg(failureCount);
    }
    updateStatusBar(resultMessage, 3000);
}

void MainWindowController::showDeleteModeDialog(const QList<QListWidgetItem*>& items, DeleteMode mode)
{
    QString songTitles;
    for (auto item : items) {
        songTitles += item->text() + "\n";
    }
    
    QString title = "选择删除模式";
    QString message;
    QMessageBox::StandardButtons buttons;
    
    if (mode == DeleteMode::FromDatabase) {
        // 在"全部歌曲"标签下
        message = QString("请选择删除模式：\n\n%1").arg(songTitles);
        buttons = QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel;
        
        QMessageBox msgBox(m_mainWindow);
        msgBox.setWindowTitle(title);
        msgBox.setText(message);
        msgBox.setInformativeText("选择删除模式：");
        msgBox.setStandardButtons(buttons);
        msgBox.setButtonText(QMessageBox::Yes, "彻底删除歌曲");
        msgBox.setButtonText(QMessageBox::No, "仅删除播放记录");
        msgBox.setButtonText(QMessageBox::Cancel, "取消");
        msgBox.setDefaultButton(QMessageBox::Cancel);
        
        int result = msgBox.exec();
        
        if (result == QMessageBox::Yes) {
            executeDeleteOperation(items, DeleteMode::FromDatabase);
        } else if (result == QMessageBox::No) {
            executeDeleteOperation(items, DeleteMode::FromPlayHistory);
        }
    } else if (mode == DeleteMode::FromTag) {
        // 在特定标签下
        message = QString("请选择删除模式：\n\n%1").arg(songTitles);
        buttons = QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel;
        
        QMessageBox msgBox(m_mainWindow);
        msgBox.setWindowTitle(title);
        msgBox.setText(message);
        msgBox.setInformativeText("选择删除模式：");
        msgBox.setStandardButtons(buttons);
        msgBox.setButtonText(QMessageBox::Yes, "从当前标签移除");
        msgBox.setButtonText(QMessageBox::No, "彻底删除歌曲");
        msgBox.setButtonText(QMessageBox::Cancel, "取消");
        msgBox.setDefaultButton(QMessageBox::Cancel);
        
        int result = msgBox.exec();
        
        if (result == QMessageBox::Yes) {
            executeDeleteOperation(items, DeleteMode::FromTag);
        } else if (result == QMessageBox::No) {
            executeDeleteOperation(items, DeleteMode::FromDatabase);
        }
    }
}

void MainWindowController::showDeleteModeDialogBySongs(const QList<Song>& songs, DeleteMode mode)
{
    QString songTitles;
    for (const Song& song : songs) {
        songTitles += song.title() + "\n";
    }
    
    QString title = "选择删除模式";
    QString message;
    QMessageBox::StandardButtons buttons;
    
    if (mode == DeleteMode::FromDatabase) {
        // 在"全部歌曲"标签下
        message = QString("请选择删除模式：\n\n%1").arg(songTitles);
        buttons = QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel;
        
        QMessageBox msgBox(m_mainWindow);
        msgBox.setWindowTitle(title);
        msgBox.setText(message);
        msgBox.setInformativeText("选择删除模式：");
        msgBox.setStandardButtons(buttons);
        msgBox.setButtonText(QMessageBox::Yes, "彻底删除歌曲");
        msgBox.setButtonText(QMessageBox::No, "仅删除播放记录");
        msgBox.setButtonText(QMessageBox::Cancel, "取消");
        msgBox.setDefaultButton(QMessageBox::Cancel);
        
        int result = msgBox.exec();
        
        if (result == QMessageBox::Yes) {
            executeDeleteOperationBySongs(songs, DeleteMode::FromDatabase);
        } else if (result == QMessageBox::No) {
            executeDeleteOperationBySongs(songs, DeleteMode::FromPlayHistory);
        }
    } else if (mode == DeleteMode::FromTag) {
        // 在特定标签下
        message = QString("请选择删除模式：\n\n%1").arg(songTitles);
        buttons = QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel;
        
        QMessageBox msgBox(m_mainWindow);
        msgBox.setWindowTitle(title);
        msgBox.setText(message);
        msgBox.setInformativeText("选择删除模式：");
        msgBox.setStandardButtons(buttons);
        msgBox.setButtonText(QMessageBox::Yes, "从当前标签移除");
        msgBox.setButtonText(QMessageBox::No, "彻底删除歌曲");
        msgBox.setButtonText(QMessageBox::Cancel, "取消");
        msgBox.setDefaultButton(QMessageBox::Cancel);
        
        int result = msgBox.exec();
        
        if (result == QMessageBox::Yes) {
            executeDeleteOperationBySongs(songs, DeleteMode::FromTag);
        } else if (result == QMessageBox::No) {
            executeDeleteOperationBySongs(songs, DeleteMode::FromDatabase);
        }
    }
}

void MainWindowController::executeDeleteOperation(const QList<QListWidgetItem*>& items, DeleteMode mode)
{
    switch (mode) {
        case DeleteMode::FromTag:
            removeSelectedSongsFromCurrentTag(items);
            break;
        case DeleteMode::FromDatabase:
            deleteSelectedSongsFromDatabase(items);
            break;
        case DeleteMode::FromPlayHistory:
            deleteSelectedPlayHistoryRecords(items);
            break;
    }
}

void MainWindowController::executeDeleteOperationBySongs(const QList<Song>& songs, DeleteMode mode)
{
    switch (mode) {
        case DeleteMode::FromTag:
            removeSelectedSongsFromCurrentTagBySongs(songs);
            break;
        case DeleteMode::FromDatabase:
            deleteSelectedSongsFromDatabaseBySongs(songs);
            break;
        case DeleteMode::FromPlayHistory:
            deleteSelectedPlayHistoryRecordsBySongs(songs);
            break;
    }
}

void MainWindowController::removeSelectedSongsFromCurrentTag(const QList<QListWidgetItem*>& items)
{
    // 获取当前标签
    QString currentTagName = "";
    if (m_tagListWidget && m_tagListWidget->currentItem()) {
        currentTagName = m_tagListWidget->currentItem()->text();
    }
    
    if (currentTagName.isEmpty() || currentTagName == "全部歌曲" || currentTagName == "最近播放") {
        logWarning("无法从系统标签中移除歌曲");
        QMessageBox::warning(m_mainWindow, "警告", "无法从系统标签中移除歌曲");
        return;
    }
    
    // 获取标签ID
    TagDao tagDao;
    Tag tag = tagDao.getTagByName(currentTagName);
    if (!tag.isValid()) {
        logError(QString("标签不存在: %1").arg(currentTagName));
        QMessageBox::warning(m_mainWindow, "警告", "标签不存在");
        return;
    }
    
    int successCount = 0;
    int failureCount = 0;
    
    for (auto item : items) {
        QVariant songData = item->data(Qt::UserRole);
        Song song = songData.value<Song>();
        
        if (song.isValid()) {
            // 直接调用SongDao移除歌曲，不显示确认对话框
            SongDao songDao;
            if (songDao.removeSongFromTag(song.id(), tag.id())) {
                successCount++;
                logInfo(QString("歌曲 %1 已从标签 %2 移除").arg(song.id()).arg(tag.id()));
            } else {
                failureCount++;
                logError(QString("移除歌曲失败: 歌曲ID=%1, 标签ID=%2").arg(song.id()).arg(tag.id()));
            }
        } else {
            failureCount++;
        }
    }
    
    // 刷新歌曲列表
    refreshSongList();
    
    QString resultMessage;
    if (failureCount == 0) {
        resultMessage = QString("成功从标签 '%1' 移除 %2 首歌曲").arg(currentTagName).arg(successCount);
    } else {
        resultMessage = QString("移除歌曲完成：成功 %1 首，失败 %2 首").arg(successCount).arg(failureCount);
    }
    updateStatusBar(resultMessage, 3000);
}

void MainWindowController::deleteSelectedSongsFromDatabase(const QList<QListWidgetItem*>& items)
{
    // 收集歌曲信息
    QList<int> songIds;
    QStringList songTitles;
    
    for (auto item : items) {
        QVariant songData = item->data(Qt::UserRole);
        Song song = songData.value<Song>();
        
        if (song.isValid()) {
            songIds.append(song.id());
            songTitles.append(song.title());
        }
    }
    
    if (songIds.isEmpty()) {
        logWarning("无法获取选中歌曲的ID信息");
        QMessageBox::warning(m_mainWindow, "警告", "无法获取选中歌曲的信息");
        return;
    }
    
    // 直接在主线程中执行删除操作，避免数据库连接问题
    qDebug() << "[DeleteSongs] 开始删除歌曲";
    int successCount = 0;
    int failureCount = 0;
    
    SongDao songDao;
    for (int i = 0; i < songIds.size(); ++i) {
        int songId = songIds[i];
        QString songTitle = songTitles[i];
        
        try {
            qDebug() << "[DeleteSongs] 删除歌曲 ID:" << songId << "标题:" << songTitle;
            bool deleteResult = songDao.deleteSong(songId);
            if (deleteResult) {
                successCount++;
            } else {
                failureCount++;
            }
        } catch (const std::exception& e) {
            failureCount++;
            qDebug() << "[DeleteSongs] 删除歌曲异常:" << e.what();
        }
    }
    
    // 更新UI
    onSongDeletionCompleted(successCount, failureCount);
    
    updateStatusBar("删除歌曲完成", 3000);
}

// 播放列表操作方法实现
void MainWindowController::showCreatePlaylistDialog()
{
    logInfo("显示创建播放列表对话框");
    
    try {
        // 创建输入对话框
        bool ok;
        QString playlistName = QInputDialog::getText(m_mainWindow, "创建播放列表", 
            "请输入播放列表名称:", QLineEdit::Normal, "", &ok);
            
        if (ok && !playlistName.trimmed().isEmpty()) {
            playlistName = playlistName.trimmed();
            
            // 检查播放列表名称是否已存在
            if (!m_playlistManager) {
                logWarning("PlaylistManager 未初始化，无法创建播放列表");
                QMessageBox::warning(m_mainWindow, "警告", "播放列表管理器未初始化");
                return;
            }
            
            if (m_playlistManager->playlistExists(playlistName)) {
                logWarning(QString("播放列表名称已存在: %1").arg(playlistName));
                QMessageBox::warning(m_mainWindow, "警告", 
                    QString("播放列表 '%1' 已存在，请使用其他名称").arg(playlistName));
                return;
            }
            
            // 创建播放列表
            Playlist newPlaylist;
            newPlaylist.setName(playlistName);
            newPlaylist.setDescription(QString("用户创建的播放列表"));
            newPlaylist.setType(PlaylistType::User);
            newPlaylist.setIsSystem(false);
            newPlaylist.setCreatedAt(QDateTime::currentDateTime());
            newPlaylist.setUpdatedAt(QDateTime::currentDateTime());
            
            auto result = m_playlistManager->createPlaylist(newPlaylist.name(), newPlaylist.description());
            if (!result.success) {
                logError(QString("创建播放列表失败: %1").arg(result.message));
                QMessageBox::critical(m_mainWindow, "错误", "创建播放列表失败");
                return;
            }
            
            logInfo(QString("创建播放列表: %1").arg(playlistName));
            updateStatusBar(QString("播放列表 '%1' 已创建").arg(playlistName), 3000);
            
            // 刷新播放列表视图
            refreshPlaylistView();
        }
        
    } catch (const std::exception& e) {
        logError(QString("创建播放列表时发生异常: %1").arg(e.what()));
        QMessageBox::critical(m_mainWindow, "错误", "创建播放列表时发生错误");
    }
}

void MainWindowController::importPlaylistFromFile()
{
    logInfo("导入播放列表");
    
    try {
        // 选择播放列表文件
        QString fileName = QFileDialog::getOpenFileName(m_mainWindow, 
            "导入播放列表", 
            QStandardPaths::writableLocation(QStandardPaths::MusicLocation),
            "播放列表文件 (*.m3u *.m3u8 *.pls *.xspf);;所有文件 (*.*)");
            
        if (!fileName.isEmpty()) {
            // 检查PlaylistManager是否已初始化
            if (!m_playlistManager) {
                logError("PlaylistManager未初始化，无法导入播放列表");
                QMessageBox::warning(m_mainWindow, "警告", "播放列表管理器未初始化");
                return;
            }
            
            // 解析播放列表文件
            QFileInfo fileInfo(fileName);
            QString extension = fileInfo.suffix().toLower();
            
            QStringList songPaths;
            QString playlistName = fileInfo.baseName();
            
            if (extension == "m3u" || extension == "m3u8") {
                songPaths = parseM3UPlaylist(fileName);
            } else if (extension == "pls") {
                songPaths = parsePLSPlaylist(fileName);
            } else if (extension == "xspf") {
                songPaths = parseXSPFPlaylist(fileName);
            } else {
                logError(QString("不支持的播放列表格式: %1").arg(extension));
                QMessageBox::warning(m_mainWindow, "警告", "不支持的播放列表格式");
                return;
            }
            
            if (songPaths.isEmpty()) {
                logWarning("播放列表文件中没有找到有效的歌曲路径");
                QMessageBox::information(m_mainWindow, "信息", "播放列表文件中没有找到有效的歌曲");
                return;
            }
            
            // 创建播放列表
            auto result = m_playlistManager->createPlaylist(playlistName, "从文件导入的播放列表");
            if (!result.success) {
                logError(QString("创建播放列表失败: %1").arg(result.message));
                QMessageBox::critical(m_mainWindow, "错误", "创建播放列表失败");
                return;
            }
            
            Playlist playlist = result.data.value<Playlist>();
            int successCount = 0;
            int totalCount = songPaths.size();
            
            // 添加歌曲到播放列表
            for (const QString& songPath : songPaths) {
                QFileInfo songFileInfo(songPath);
                if (songFileInfo.exists() && songFileInfo.isFile()) {
                    // 这里需要先将歌曲添加到数据库，然后再添加到播放列表
                    // 由于当前架构限制，暂时记录日志
                    logInfo(QString("找到歌曲文件: %1").arg(songPath));
                    successCount++;
                } else {
                    logWarning(QString("歌曲文件不存在: %1").arg(songPath));
                }
            }
            
            logInfo(QString("导入播放列表文件: %1，成功解析 %2/%3 首歌曲")
                   .arg(fileName).arg(successCount).arg(totalCount));
            updateStatusBar(QString("播放列表导入完成，解析了 %1/%2 首歌曲")
                           .arg(successCount).arg(totalCount), 3000);
            
            // 刷新播放列表视图
            refreshPlaylistView();
        }
        
    } catch (const std::exception& e) {
        logError(QString("导入播放列表时发生异常: %1").arg(e.what()));
        QMessageBox::critical(m_mainWindow, "错误", "导入播放列表时发生错误");
    }
}

void MainWindowController::refreshPlaylistView()
{
    logInfo("刷新播放列表视图");
    
    if (!m_playlistManager) {
        logWarning("PlaylistManager 未初始化，无法刷新播放列表视图");
        updateStatusBar("播放列表管理器未初始化", 3000);
        return;
    }
    
    try {
        // 获取所有播放列表
        QList<Playlist> playlists = m_playlistManager->getAllPlaylists();
        logDebug(QString("获取到 %1 个播放列表").arg(playlists.size()));
        
        // 由于当前UI设计中没有专门的播放列表控件，
        // 这里通过状态栏显示播放列表信息并记录到日志
        if (playlists.isEmpty()) {
            updateStatusBar("暂无播放列表", 2000);
            logInfo("当前没有播放列表");
        } else {
            QString message = QString("共有 %1 个播放列表").arg(playlists.size());
            updateStatusBar(message, 2000);
            
            // 记录播放列表详细信息到日志
            for (const auto& playlist : playlists) {
                logDebug(QString("播放列表: %1 (ID: %2, 歌曲数: %3, 类型: %4)")
                        .arg(playlist.name())
                        .arg(QString::number(playlist.id()))
                        .arg(QString::number(playlist.songCount()))
                        .arg(QString::number(static_cast<int>(playlist.type()))));
            }
        }
        
        // 如果当前视图模式是播放列表视图，进行特殊处理
        if (m_viewMode == ViewMode::PlaylistView) {
            logDebug("当前视图模式为播放列表视图，执行相应更新");
            // 这里可以添加播放列表视图特有的更新逻辑
            // 例如：更新播放列表相关的UI控件
        }
        
        logInfo("播放列表视图刷新完成");
        
    } catch (const std::exception& e) {
        QString errorMsg = QString("刷新播放列表视图时发生异常: %1").arg(e.what());
        logError(errorMsg);
        updateStatusBar(errorMsg, 5000);
        handleError(errorMsg);
    }
}

// 辅助方法：获取当前选中的标签
Tag MainWindowController::getSelectedTag() const
{
    if (!m_tagListWidget) {
        return Tag();
    }
    
    QListWidgetItem* currentItem = m_tagListWidget->currentItem();
    if (!currentItem) {
        return Tag();
    }
    
    // 从item中获取标签信息
    QString tagName = currentItem->text();
    TagDao tagDao;
    return tagDao.getTagByName(tagName);
}

// 播放列表文件解析方法实现
QStringList MainWindowController::parseM3UPlaylist(const QString& filePath) const
{
    QStringList songPaths;
    QFile file(filePath);
    
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const_cast<MainWindowController*>(this)->logError(QString("无法打开M3U播放列表文件: %1").arg(filePath));
        return songPaths;
    }
    
    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    
    QDir playlistDir = QFileInfo(filePath).absoluteDir();
    
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        
        // 跳过注释行和空行
        if (line.isEmpty() || line.startsWith("#")) {
            continue;
        }
        
        // 处理相对路径
        if (QFileInfo(line).isRelative()) {
            line = playlistDir.absoluteFilePath(line);
        }
        
        // 检查文件是否存在
        if (QFileInfo::exists(line)) {
            songPaths.append(QDir::toNativeSeparators(line));
            const_cast<MainWindowController*>(this)->logDebug(QString("M3U: 找到歌曲文件: %1").arg(line));
        } else {
            const_cast<MainWindowController*>(this)->logWarning(QString("M3U: 歌曲文件不存在: %1").arg(line));
        }
    }
    
    file.close();
    const_cast<MainWindowController*>(this)->logInfo(QString("M3U播放列表解析完成，共找到 %1 首歌曲").arg(songPaths.size()));
    return songPaths;
}

QStringList MainWindowController::parsePLSPlaylist(const QString& filePath) const
{
    QStringList songPaths;
    QFile file(filePath);
    
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const_cast<MainWindowController*>(this)->logError(QString("无法打开PLS播放列表文件: %1").arg(filePath));
        return songPaths;
    }
    
    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    
    QDir playlistDir = QFileInfo(filePath).absoluteDir();
    
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        
        // 查找File条目
        if (line.startsWith("File", Qt::CaseInsensitive)) {
            int equalPos = line.indexOf('=');
            if (equalPos > 0) {
                QString songPath = line.mid(equalPos + 1).trimmed();
                
                // 处理相对路径
                if (QFileInfo(songPath).isRelative()) {
                    songPath = playlistDir.absoluteFilePath(songPath);
                }
                
                // 检查文件是否存在
                if (QFileInfo::exists(songPath)) {
                    songPaths.append(QDir::toNativeSeparators(songPath));
                    const_cast<MainWindowController*>(this)->logDebug(QString("PLS: 找到歌曲文件: %1").arg(songPath));
                } else {
                    const_cast<MainWindowController*>(this)->logWarning(QString("PLS: 歌曲文件不存在: %1").arg(songPath));
                }
            }
        }
    }
    
    file.close();
    const_cast<MainWindowController*>(this)->logInfo(QString("PLS播放列表解析完成，共找到 %1 首歌曲").arg(songPaths.size()));
    return songPaths;
}

QStringList MainWindowController::parseXSPFPlaylist(const QString& filePath) const
{
    QStringList songPaths;
    QFile file(filePath);
    
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << QString("无法打开XSPF播放列表文件: %1").arg(filePath);
        return songPaths;
    }
    
    QXmlStreamReader xml(&file);
    QDir playlistDir = QFileInfo(filePath).absoluteDir();
    
    while (!xml.atEnd()) {
        xml.readNext();
        
        if (xml.isStartElement() && xml.name() == QStringLiteral("location")) {
            QString location = xml.readElementText().trimmed();
            
            // 处理file://协议
            if (location.startsWith("file://")) {
                location = QUrl(location).toLocalFile();
            }
            
            // 处理相对路径
            if (QFileInfo(location).isRelative()) {
                location = playlistDir.absoluteFilePath(location);
            }
            
            // 检查文件是否存在
            if (QFileInfo::exists(location)) {
                songPaths.append(QDir::toNativeSeparators(location));
                const_cast<MainWindowController*>(this)->logDebug(QString("XSPF: 找到歌曲文件: %1").arg(location));
            } else {
                const_cast<MainWindowController*>(this)->logWarning(QString("XSPF: 歌曲文件不存在: %1").arg(location));
            }
        }
    }
    
    if (xml.hasError()) {
        const_cast<MainWindowController*>(this)->logError(QString("XSPF解析错误: %1").arg(xml.errorString()));
    }
    
    file.close();
    const_cast<MainWindowController*>(this)->logInfo(QString("XSPF播放列表解析完成，共找到 %1 首歌曲").arg(songPaths.size()));
    return songPaths;
}

// 播放/暂停切换功能
void MainWindowController::togglePlayPause()
{
    qDebug() << "MainWindowController::togglePlayPause() - 切换播放/暂停状态";
    
    if (!m_audioEngine) {
        logWarning("AudioEngine 未初始化，无法切换播放状态");
        return;
    }
    
    AudioTypes::AudioState currentState = m_audioEngine->state();
    
    switch (currentState) {
        case AudioTypes::AudioState::Playing:
            m_audioEngine->pause();
            logInfo("音频播放已暂停");
            updateStatusBar("播放已暂停", 2000);
            break;
            
        case AudioTypes::AudioState::Paused:
            m_audioEngine->play();
            logInfo("音频播放已开始");
            updateStatusBar("开始播放", 2000);
            break;
            
        default:
            logWarning(QString("当前音频状态不支持播放/暂停切换: %1").arg(static_cast<int>(currentState)));
            break;
    }
}

// 循环切换播放模式
void MainWindowController::cyclePlayMode()
{
    qDebug() << "MainWindowController::cyclePlayMode() - 循环切换播放模式";
    
    if (!m_audioEngine) {
        logWarning("AudioEngine 未初始化，无法切换播放模式");
        return;
    }
    
    AudioTypes::PlayMode currentMode = m_audioEngine->playMode();
    AudioTypes::PlayMode nextMode;
    QString modeText;
    
    switch (currentMode) {
        case AudioTypes::PlayMode::Loop:
            nextMode = AudioTypes::PlayMode::Random;
            modeText = "随机播放";
            break;
            
        case AudioTypes::PlayMode::Random:
            nextMode = AudioTypes::PlayMode::RepeatOne;
            modeText = "单曲循环";
            break;
            
        case AudioTypes::PlayMode::RepeatOne:
        default:
            nextMode = AudioTypes::PlayMode::Loop;
            modeText = "列表循环";
            break;
    }
    
    m_audioEngine->setPlayMode(nextMode);
    logInfo(QString("播放模式已切换为: %1").arg(modeText));
    updateStatusBar(QString("播放模式: %1").arg(modeText), 2000);
    
    // 通知PlayInterfaceController更新播放模式按钮
    // 注意：m_playInterfaceController可能为空，因为它是在PlayInterface创建时才初始化的
    // 而不是在MainWindowController初始化时
    if (m_playInterfaceController && m_playInterfaceController.get()) {
        int modeIndex = 0; // 默认列表循环
        switch (nextMode) {
            case AudioTypes::PlayMode::Loop:
                modeIndex = 0; // 列表循环
                break;
            case AudioTypes::PlayMode::Random:
                modeIndex = 1; // 随机播放
                break;
            case AudioTypes::PlayMode::RepeatOne:
                modeIndex = 2; // 单曲循环
                break;
            default:
                modeIndex = 0; // 默认列表循环
                break;
        }
        try {
            m_playInterfaceController->updatePlayModeButton(modeIndex);
        } catch (const std::exception& e) {
            logError(QString("更新播放模式按钮失败: %1").arg(e.what()));
        }
    }
    // 无论PlayInterfaceController是否存在，都更新主窗口的播放模式按钮
    updatePlayModeButton();
}

// 全选当前标签下的所有歌曲
void MainWindowController::selectAllSongs()
{
    qDebug() << "MainWindowController::selectAllSongs() - 全选当前标签下的所有歌曲";
    
    if (!m_songListWidget) {
        logWarning("歌曲列表控件未初始化");
        return;
    }
    
    int songCount = m_songListWidget->count();
    if (songCount == 0) {
        logInfo("当前标签下没有歌曲可选择");
        updateStatusBar("当前标签下没有歌曲", 2000);
        return;
    }
    
    // 全选所有歌曲项
    for (int i = 0; i < songCount; ++i) {
        QListWidgetItem* item = m_songListWidget->item(i);
        if (item) {
            item->setSelected(true);
        }
    }
    
    logInfo(QString("已全选 %1 首歌曲").arg(songCount));
    updateStatusBar(QString("已全选 %1 首歌曲").arg(songCount), 2000);
}

// 取消选中状态
void MainWindowController::clearSongSelection()
{
    qDebug() << "MainWindowController::clearSongSelection() - 取消所有歌曲的选中状态";
    
    if (!m_songListWidget) {
        logWarning("歌曲列表控件未初始化");
        return;
    }
    
    QList<QListWidgetItem*> selectedItems = m_songListWidget->selectedItems();
    int selectedCount = selectedItems.count();
    
    if (selectedCount == 0) {
        logInfo("当前没有选中的歌曲");
        updateStatusBar("当前没有选中的歌曲", 2000);
        return;
    }
    
    // 清除所有选中状态
    m_songListWidget->clearSelection();
    
    logInfo(QString("已取消 %1 首歌曲的选中状态").arg(selectedCount));
    updateStatusBar(QString("已取消 %1 首歌曲的选中状态").arg(selectedCount), 2000);
}

// 删除选中的歌曲
void MainWindowController::deleteSelectedSongs()
{
    qDebug() << "MainWindowController::deleteSelectedSongs() - 删除选中的歌曲";
    
    if (!m_songListWidget) {
        logWarning("歌曲列表控件未初始化");
        return;
    }
    
    QList<QListWidgetItem*> selectedItems = m_songListWidget->selectedItems();
    int selectedCount = selectedItems.count();
    
    if (selectedCount == 0) {
        logInfo("当前没有选中的歌曲可删除");
        updateStatusBar("请先选择要删除的歌曲", 2000);
        return;
    }
    
    // 确认删除对话框
    QMessageBox::StandardButton reply = QMessageBox::question(
        m_mainWindow,
        "确认删除",
        QString("确定要删除选中的 %1 首歌曲吗？\n\n注意：这将从数据库中永久删除这些歌曲记录。").arg(selectedCount),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );
    
    if (reply != QMessageBox::Yes) {
        logInfo("用户取消了删除操作");
        return;
    }
    
    // 收集要删除的歌曲ID
    QList<int> songIdsToDelete;
    QStringList songTitlesToDelete;
    
    for (QListWidgetItem* item : selectedItems) {
        if (item && item->data(Qt::UserRole).isValid()) {
            QVariant songData = item->data(Qt::UserRole);
            Song song = songData.value<Song>();
            
            logInfo(QString("从列表项获取歌曲: ID=%1, 标题=%2, 有效性=%3")
                   .arg(song.id()).arg(song.title()).arg(song.isValid()));
            
            if (song.isValid() && song.id() > 0) {
                songIdsToDelete.append(song.id());
                songTitlesToDelete.append(song.title());
            } else {
                logWarning(QString("歌曲数据无效或ID为0: 显示文本=%1").arg(item->text()));
            }
        }
    }
    
    if (songIdsToDelete.isEmpty()) {
        logWarning("无法获取选中歌曲的ID信息");
        QMessageBox::warning(m_mainWindow, "警告", "无法获取选中歌曲的信息");
        return;
    }
    
    // 使用QtConcurrent运行删除操作
    QFuture<void> future = QtConcurrent::run([this, songIdsToDelete, songTitlesToDelete]() {
        qDebug() << "[DeleteSongs] 后台线程开始删除歌曲";
        int successCount = 0;
        int failureCount = 0;
        
        // 在后台线程中创建数据库连接
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "DeleteSongsThread");
        db.setDatabaseName(DatabaseManager::instance()->database().databaseName());
        
        if (!db.open()) {
            qDebug() << "[DeleteSongs] 后台线程数据库连接失败:" << db.lastError().text();
            QMetaObject::invokeMethod(this, "onSongDeletionCompleted", Qt::QueuedConnection,
                                      Q_ARG(int, 0), Q_ARG(int, songIdsToDelete.size()));
            return;
        }
        
        SongDao songDao;
        for (int i = 0; i < songIdsToDelete.size(); ++i) {
            int songId = songIdsToDelete[i];
            QString songTitle = songTitlesToDelete[i];
            try {
                qDebug() << "[DeleteSongs] 删除歌曲 ID:" << songId << "标题:" << songTitle;
                bool deleteResult = songDao.deleteSong(songId);
                if (deleteResult) {
                    successCount++;
                } else {
                    failureCount++;
                }
            } catch (const std::exception& e) {
                failureCount++;
                qDebug() << "[DeleteSongs] 删除歌曲异常:" << e.what();
            }
        }
        
        // 关闭后台线程的数据库连接
        db.close();
        QSqlDatabase::removeDatabase("DeleteSongsThread");
        
        // 使用invokeMethod在主线程更新UI
        QMetaObject::invokeMethod(this, "onSongDeletionCompleted", Qt::QueuedConnection,
                                  Q_ARG(int, successCount), Q_ARG(int, failureCount));
    });
    
    updateStatusBar("正在删除歌曲...", 0);
}

void MainWindowController::onSongDeletionCompleted(int successCount, int failureCount)
{
    qDebug() << "[DeleteSongs] 删除完成: 成功" << successCount << "失败" << failureCount;
    
    // 刷新歌曲列表
    refreshSongList();
    
    // 如果当前在"最近播放"标签下，也需要刷新，因为彻底删除的歌曲应该从最近播放中移除
    if (m_tagListWidget && m_tagListWidget->currentItem() && 
        m_tagListWidget->currentItem()->text() == "最近播放") {
        updateSongList();
    }
    
    // 更新播放列表，移除已删除的歌曲
    updatePlaylistAfterDeletion();
    
    QString resultMessage;
    if (failureCount == 0) {
        resultMessage = QString("成功删除 %1 首歌曲").arg(successCount);
    } else {
        resultMessage = QString("删除完成：成功 %1 首，失败 %2 首").arg(successCount).arg(failureCount);
    }
    updateStatusBar(resultMessage, 3000);
}

// 歌曲列表控制按钮的槽函数实现
void MainWindowController::onPlayAllButtonClicked()
{
    qDebug() << "[播放控制] 开始播放全部";
    qDebug() << "[排查] m_audioEngine指针:" << m_audioEngine;
    qDebug() << "[排查] m_songListWidget指针:" << m_songListWidget;
    if (!m_audioEngine || !m_songListWidget) {
        qWarning() << "播放组件未初始化";
        return;
    }
    if (m_songListWidget->count() == 0) {
        qDebug() << "[播放控制] 歌曲列表为空";
        updateStatusBar("当前无可用歌曲", 2000);
        return;
    }
    QList<Song> playlist;
    for (int i = 0; i < m_songListWidget->count(); ++i) {
        if (auto item = m_songListWidget->item(i)) {
            auto song = item->data(Qt::UserRole).value<Song>();
            qDebug() << "[排查] Song.isValid():" << song.isValid();
            qDebug() << "[排查] Song.filePath():" << song.filePath();
            qDebug() << "[排查] 文件是否存在:" << QFile::exists(song.filePath());
            if (song.isValid()) {
                playlist.append(song);
                qDebug() << "[播放列表] 添加歌曲:" << song.title();
            }
        }
    }
    if (playlist.isEmpty()) {
        qWarning() << "无法构建有效播放列表";
        return;
    }
    qDebug() << "[播放控制] 设置播放列表，共" << playlist.size() << "首歌曲";
    m_audioEngine->setPlaylist(playlist);
    m_audioEngine->setCurrentIndex(0);
    qDebug() << "[排查] 调用m_audioEngine->play()前，currentIndex:" << m_audioEngine->currentIndex();
    m_audioEngine->play();
    qDebug() << "[播放控制] 已开始播放首曲:" << playlist.first().title();
}

void MainWindowController::onPlayModeButtonClicked()
{
    qDebug() << "MainWindowController::onPlayModeButtonClicked() - 播放模式按钮被点击";
    cyclePlayMode();
}

void MainWindowController::onSelectAllButtonClicked()
{
    qDebug() << "MainWindowController::onSelectAllButtonClicked() - 全选按钮被点击";
    selectAllSongs();
}

void MainWindowController::onClearSelectionButtonClicked()
{
    qDebug() << "MainWindowController::onClearSelectionButtonClicked() - 取消全选按钮被点击";
    clearSongSelection();
}

void MainWindowController::onDeleteSelectedButtonClicked()
{
    qDebug() << "MainWindowController::onDeleteSelectedButtonClicked() - 删除选中按钮被点击";
    
    // 获取当前选中的标签
    QString currentTag = "";
    if (m_tagListWidget && m_tagListWidget->currentItem()) {
        currentTag = m_tagListWidget->currentItem()->text();
    }
    
    // 获取选中的歌曲并创建安全的数据副本
    QList<QListWidgetItem*> selectedItems = m_songListWidget->selectedItems();
    if (selectedItems.isEmpty()) {
        QMessageBox::information(m_mainWindow, "提示", "请先选择要删除的歌曲");
        return;
    }
    
    // 创建歌曲数据的安全副本，避免指针失效问题
    QList<Song> songsToDelete;
    QStringList songTitles;
    
    for (auto item : selectedItems) {
        if (item) {
            QVariant songData = item->data(Qt::UserRole);
            Song song = songData.value<Song>();
            if (song.isValid()) {
                songsToDelete.append(song);
                songTitles.append(song.title());
            }
        }
    }
    
    if (songsToDelete.isEmpty()) {
        QMessageBox::warning(m_mainWindow, "警告", "无法获取选中歌曲的有效数据");
        return;
    }
    
    // 根据当前标签提供不同的删除选项
    if (currentTag == "最近播放") {
        // 在"最近播放"标签下，只提供删除播放记录的选项
        QString songTitlesText;
        for (const QString& title : songTitles) {
            songTitlesText += title + "\n";
        }
        
        QMessageBox::StandardButton reply = QMessageBox::question(
            m_mainWindow,
            "确认删除播放记录",
            QString("确定要从最近播放列表中删除以下歌曲的播放记录吗？\n\n%1\n注意：这只会删除播放历史记录，不会删除歌曲文件。")
                .arg(songTitlesText),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
        );
        
        if (reply == QMessageBox::Yes) {
            deleteSelectedPlayHistoryRecordsBySongs(songsToDelete);
        }
    } else if (currentTag == "我的歌曲") {
        // 在"我的歌曲"标签下，只提供彻底删除选项（包括删除实际文件）
        QString songTitlesText;
        for (const QString& title : songTitles) {
            songTitlesText += title + "\n";
        }
        
        QMessageBox::StandardButton reply = QMessageBox::question(
            m_mainWindow,
            "确认彻底删除",
            QString("确定要彻底删除以下歌曲吗？\n\n%1\n注意：这将删除歌曲记录、所有标签关联以及实际文件。此操作不可恢复！")
                .arg(songTitlesText),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
        );
        
        if (reply == QMessageBox::Yes) {
            deleteSelectedSongsCompletelyBySongs(songsToDelete);
        }
    } else if (currentTag == "全部歌曲") {
        // 在"全部歌曲"标签下，提供两种删除选项
        showDeleteModeDialogBySongs(songsToDelete, DeleteMode::FromDatabase);
    } else {
        // 在特定标签下，提供三种删除选项
        showDeleteModeDialogBySongs(songsToDelete, DeleteMode::FromTag);
    }
}

// 实现新的进度条相关方法
// 旧的进度条相关槽函数已删除，现在使用自定义音乐进度条组件

// 实现新的音量条相关方法
void MainWindowController::onVolumeSliderPressed()
{
    qDebug() << "[音量条] 滑块被按下";
    m_isVolumeSliderPressed = true;
}

void MainWindowController::onVolumeSliderReleased()
{
    qDebug() << "[音量条] 滑块被释放";
    m_isVolumeSliderPressed = false;
    
    // 发送音量变更请求
    if (m_volumeSlider) {
        int volume = m_volumeSlider->value();
        emit volumeChangeRequested(volume);
        qDebug() << "[音量条] 发送音量变更请求:" << volume;
    }
}

void MainWindowController::onVolumeSliderChanged(int value)
{
    // 更新音量显示
    updateVolumeDisplay(value);
    
    // 发送音量变更请求到AudioEngine
    emit volumeChangeRequested(value);
    
    logInfo(QString("音量变化: %1").arg(value));
}

void MainWindowController::onMuteButtonClicked()
{
    logInfo("静音按钮被点击");
    
    if (!m_audioEngine) {
        logWarning("AudioEngine未初始化");
        return;
    }
    
    // 使用AudioEngine的toggleMute方法
    m_audioEngine->toggleMute();
    
    // 更新静音按钮状态
    updateMuteButtonState();
}

void MainWindowController::onVolumeLabelDoubleClicked()
{
    qDebug() << "[音量标签] 双击事件触发";
    showVolumeEditDialog();
}

void MainWindowController::showVolumeEditDialog()
{
    if (!m_volumeSlider) {
        logWarning("音量滑块未初始化");
        return;
    }
    
    int currentVolume = m_volumeSlider->value();
    bool ok;
    int newVolume = QInputDialog::getInt(m_mainWindow, 
                                        "设置音量", 
                                        "请输入音量大小 (0-100):", 
                                        currentVolume, 
                                        0, 
                                        100, 
                                        1, 
                                        &ok);
    
    if (ok) {
        qDebug() << "[音量编辑] 用户输入音量:" << newVolume;
        
        // 更新音量滑块
        m_volumeSlider->setValue(newVolume);
        
        // 发送音量变更请求
        emit volumeChangeRequested(newVolume);
        
        // 更新显示
        updateVolumeDisplay(newVolume);
        
        logInfo(QString("音量已设置为: %1").arg(newVolume));
    }
}

// 更新音量显示方法
void MainWindowController::updateVolumeDisplay(int volume)
{
    // 更新音量标签
    if (m_volumeLabel) {
        m_volumeLabel->setText(QString("%1%").arg(volume));
    }
    
    // 更新音量图标
    if (m_volumeIconLabel) {
        if (volume == 0) {
            m_volumeIconLabel->setText("🔇"); // 静音
        } else if (volume < 30) {
            m_volumeIconLabel->setText("🔈"); // 低音量
        } else if (volume < 70) {
            m_volumeIconLabel->setText("🔉"); // 中音量
        } else {
            m_volumeIconLabel->setText("🔊"); // 高音量
        }
    }
    
    // 更新状态栏
    updateStatusBar(QString("音量: %1%").arg(volume), 1000);
}

// 更新静音按钮状态
void MainWindowController::updateMuteButtonState()
{
    if (!m_audioEngine || !m_muteButton) return;
    
    bool isMuted = m_audioEngine->isMuted();
    
    if (isMuted) {
        m_muteButton->setText("🔇");
        m_muteButton->setToolTip("取消静音");
    } else {
        m_muteButton->setText("🔊");
        m_muteButton->setToolTip("静音");
    }
}

// 更新进度条显示
void MainWindowController::updateProgressBar(int value, int maximum)
{
    // 进度条更新由自定义MusicProgressBar组件处理
    logDebug(QString("进度条更新请求: %1/%2").arg(formatTime(static_cast<qint64>(value))).arg(formatTime(static_cast<qint64>(maximum))));
}

// 在onPositionChanged方法中更新进度条
void MainWindowController::onPositionChanged(qint64 position)
{
    // 更新自定义音乐进度条组件的位置
    if (m_musicProgressBar) {
        m_musicProgressBar->updatePosition(position);
    }
    
    // 同步到音频可视化界面
    if (m_playInterfaceController) {
        qint64 duration = m_musicProgressBar ? m_musicProgressBar->duration() : 0;
        m_playInterfaceController->syncProgressBar(position, duration);
    }
}

// 在onDurationChanged方法中更新进度条最大值
void MainWindowController::onDurationChanged(qint64 duration)
{
    // 更新自定义音乐进度条组件的时长
    if (m_musicProgressBar) {
        m_musicProgressBar->updateDuration(duration);
        m_musicProgressBar->setRange(0, duration);
    }
    qDebug() << "[进度条] 设置时长范围: 0 -" << formatTime(duration);
    qDebug() << "[进度条] 更新总时长显示:" << formatTime(duration);
    
    // 同步到音频可视化界面
    if (m_playInterfaceController) {
        qint64 position = m_musicProgressBar ? m_musicProgressBar->position() : 0;
        m_playInterfaceController->syncProgressBar(position, duration);
    }
    
    logInfo(QString("歌曲时长: %1").arg(formatTime(duration)));
}

// 在onVolumeChanged方法中更新音量显示
void MainWindowController::onVolumeChanged(int volume)
{
    // 更新音量滑块（不触发信号）
    if (m_volumeSlider) {
        m_volumeSlider->blockSignals(true);
        m_volumeSlider->setValue(volume);
        m_volumeSlider->blockSignals(false);
    }
    
    // 更新音量显示
    updateVolumeDisplay(volume);
    
    // 同步到音频可视化界面
    if (m_playInterfaceController && m_audioEngine) {
        bool muted = m_audioEngine->isMuted();
        m_playInterfaceController->syncVolumeControls(volume, muted);
    }
    
    // 保存音量设置
    if (m_settings) {
        m_settings->setValue("Audio/volume", volume);
        m_settings->sync();
    }
    
    logInfo(QString("音量已更新: %1").arg(volume));
}

// 在onMutedChanged方法中更新静音状态
void MainWindowController::onMutedChanged(bool muted)
{
    // 更新静音按钮状态
    updateMuteButtonState();
    
    // 更新音量显示
    if (m_volumeSlider) {
        int volume = muted ? 0 : m_volumeSlider->value();
        updateVolumeDisplay(volume);
    }
    
    // 同步到音频可视化界面
    if (m_playInterfaceController && m_audioEngine) {
        int currentVolume = m_audioEngine->volume();
        m_playInterfaceController->syncVolumeControls(currentVolume, muted);
    }
    
    logInfo(QString("静音状态已更新: %1").arg(muted ? "静音" : "取消静音"));
}

void MainWindowController::setBalance(double balance)
{
    if (m_audioEngine) {
        m_audioEngine->setBalance(balance);
    }
}

void MainWindowController::onBalanceChanged(double balance)
{
    // 更新状态栏显示
    QString balanceText;
    if (balance < 0) {
        balanceText = QString("平衡: 左 %1%").arg(qAbs(static_cast<int>(balance * 100)));
    } else if (balance > 0) {
        balanceText = QString("平衡: 右 %1%").arg(static_cast<int>(balance * 100));
    } else {
        balanceText = "平衡: 中央";
    }
    updateStatusBar(balanceText, 2000);
}

// 添加事件过滤器方法来处理进度条鼠标悬停
bool MainWindowController::eventFilter(QObject* obj, QEvent* event)
{
    // 进度条事件处理由自定义MusicProgressBar组件处理
    if (obj == m_volumeIconLabel) {
        switch (event->type()) {
            case QEvent::MouseButtonPress:
                // 音量图标被点击
                onVolumeIconClicked();
                return true; // 事件已处理
                
            default:
                break;
        }
    }
    else if (obj == m_volumeLabel) {
        switch (event->type()) {
            case QEvent::MouseButtonDblClick:
                // 音量标签被双击
                onVolumeLabelDoubleClicked();
                return true; // 事件已处理
                
            default:
                break;
        }
    }
    
    // 调用基类的事件过滤器
    return QObject::eventFilter(obj, event);
}

// 添加音量图标点击处理方法
void MainWindowController::onVolumeIconClicked()
{
    qDebug() << "[音量图标] 点击事件触发";
    
    if (!m_audioEngine) {
        logWarning("AudioEngine未初始化");
        return;
    }
    
    bool isMuted = m_audioEngine->isMuted();
    
    if (isMuted) {
        // 取消静音，恢复之前的音量
        if (m_volumeSlider) {
            m_volumeSlider->setValue(m_lastVolumeBeforeMute);
        }
        m_audioEngine->setMuted(false);
        logInfo("点击音量图标取消静音，恢复音量");
    } else {
        // 静音，保存当前音量
        if (m_volumeSlider) {
            m_lastVolumeBeforeMute = m_volumeSlider->value();
            m_volumeSlider->setValue(0);
        }
        m_audioEngine->setMuted(true);
        logInfo("点击音量图标启用静音");
    }
    
    // 更新静音按钮状态
    updateMuteButtonState();
}

void MainWindowController::removeSelectedSongsFromCurrentTagBySongs(const QList<Song>& songs)
{
    // 获取当前标签
    QString currentTagName = "";
    if (m_tagListWidget && m_tagListWidget->currentItem()) {
        currentTagName = m_tagListWidget->currentItem()->text();
    }
    
    if (currentTagName.isEmpty() || currentTagName == "全部歌曲" || currentTagName == "最近播放") {
        logWarning("无法从系统标签中移除歌曲");
        QMessageBox::warning(m_mainWindow, "警告", "无法从系统标签中移除歌曲");
        return;
    }
    
    // 获取标签ID
    TagDao tagDao;
    Tag tag = tagDao.getTagByName(currentTagName);
    if (!tag.isValid()) {
        logError(QString("标签不存在: %1").arg(currentTagName));
        QMessageBox::warning(m_mainWindow, "警告", "标签不存在");
        return;
    }
    
    int successCount = 0;
    int failureCount = 0;
    
    for (const Song& song : songs) {
        if (song.isValid()) {
            // 直接调用SongDao移除歌曲，不显示确认对话框
            SongDao songDao;
            if (songDao.removeSongFromTag(song.id(), tag.id())) {
                successCount++;
                logInfo(QString("歌曲 %1 已从标签 %2 移除").arg(song.id()).arg(tag.id()));
            } else {
                failureCount++;
                logError(QString("移除歌曲失败: 歌曲ID=%1, 标签ID=%2").arg(song.id()).arg(tag.id()));
            }
        } else {
            failureCount++;
        }
    }
    
    // 刷新歌曲列表
    refreshSongList();
    
    QString resultMessage;
    if (failureCount == 0) {
        resultMessage = QString("成功从标签 '%1' 移除 %2 首歌曲").arg(currentTagName).arg(successCount);
    } else {
        resultMessage = QString("移除歌曲完成：成功 %1 首，失败 %2 首").arg(successCount).arg(failureCount);
    }
    updateStatusBar(resultMessage, 3000);
}

void MainWindowController::deleteSelectedSongsFromDatabaseBySongs(const QList<Song>& songs)
{
    // 收集歌曲信息
    QList<int> songIds;
    QStringList songTitles;
    
    for (const Song& song : songs) {
        if (song.isValid()) {
            songIds.append(song.id());
            songTitles.append(song.title());
        }
    }
    
    if (songIds.isEmpty()) {
        logWarning("无法获取选中歌曲的ID信息");
        QMessageBox::warning(m_mainWindow, "警告", "无法获取选中歌曲的信息");
        return;
    }
    
    // 直接在主线程中执行删除操作，避免数据库连接问题
    qDebug() << "[DeleteSongs] 开始删除歌曲";
    int successCount = 0;
    int failureCount = 0;
    
    SongDao songDao;
    for (int i = 0; i < songIds.size(); ++i) {
        int songId = songIds[i];
        QString songTitle = songTitles[i];
        
        try {
            qDebug() << "[DeleteSongs] 删除歌曲 ID:" << songId << "标题:" << songTitle;
            bool deleteResult = songDao.deleteSong(songId);
            if (deleteResult) {
                successCount++;
            } else {
                failureCount++;
            }
        } catch (const std::exception& e) {
            failureCount++;
            qDebug() << "[DeleteSongs] 删除歌曲异常:" << e.what();
        }
    }
    
    // 更新UI
    onSongDeletionCompleted(successCount, failureCount);
    
    updateStatusBar("删除歌曲完成", 3000);
}

void MainWindowController::deleteSelectedSongsCompletelyBySongs(const QList<Song>& songs)
{
    // 收集歌曲信息
    QList<int> songIds;
    QStringList songTitles;
    QStringList songPaths;
    
    for (const Song& song : songs) {
        if (song.isValid()) {
            songIds.append(song.id());
            songTitles.append(song.title());
            songPaths.append(song.filePath());
        }
    }
    
    if (songIds.isEmpty()) {
        logWarning("无法获取选中歌曲的ID信息");
        QMessageBox::warning(m_mainWindow, "警告", "无法获取选中歌曲的信息");
        return;
    }
    
    // 检查是否有正在播放的歌曲在删除列表中
    bool needToStopPlayback = false;
    Song currentPlayingSong;
    if (m_audioEngine) {
        currentPlayingSong = m_audioEngine->currentSong();
        for (const Song& song : songs) {
            if (song.id() == currentPlayingSong.id()) {
                needToStopPlayback = true;
                qDebug() << "[DeleteSongsCompletely] 检测到正在播放的歌曲将被删除:" << song.title();
                break;
            }
        }
    }
    
    // 如果有正在播放的歌曲将被删除，先停止播放
    if (needToStopPlayback && m_audioEngine) {
        qDebug() << "[DeleteSongsCompletely] 停止当前播放以释放文件锁";
        m_audioEngine->stop();
        // 等待一小段时间确保文件锁被释放
        QThread::msleep(100);
    }
    
    // 直接在主线程中执行删除操作，避免数据库连接问题
    qDebug() << "[DeleteSongsCompletely] 开始彻底删除歌曲";
    int successCount = 0;
    int failureCount = 0;
    QStringList failedFiles;
    
    SongDao songDao;
    for (int i = 0; i < songIds.size(); ++i) {
        int songId = songIds[i];
        QString songTitle = songTitles[i];
        QString songPath = songPaths[i];
        
        try {
            qDebug() << "[DeleteSongsCompletely] 彻底删除歌曲 ID:" << songId 
                     << "标题:" << songTitle << "路径:" << songPath;
            
            bool deleteResult = true;
            
            // 1. 先删除实际文件（避免文件被占用的问题）
            QFileInfo fileInfo(songPath);
            if (fileInfo.exists()) {
                QFile file(songPath);
                if (file.remove()) {
                    qDebug() << "[DeleteSongsCompletely] 文件删除成功:" << songPath;
                } else {
                    qDebug() << "[DeleteSongsCompletely] 文件删除失败:" << songPath 
                             << "错误:" << file.errorString();
                    failedFiles.append(songPath);
                    // 文件删除失败，但继续删除数据库记录
                }
            } else {
                qDebug() << "[DeleteSongsCompletely] 文件不存在:" << songPath;
            }
            
            // 2. 删除数据库记录（包括标签关联）
            deleteResult = songDao.deleteSong(songId);
            
            if (deleteResult) {
                successCount++;
                qDebug() << "[DeleteSongsCompletely] 数据库记录删除成功:" << songId;
            } else {
                failureCount++;
                qDebug() << "[DeleteSongsCompletely] 数据库删除失败:" << songId;
            }
            
        } catch (const std::exception& e) {
            failureCount++;
            qDebug() << "[DeleteSongsCompletely] 删除歌曲异常:" << e.what();
        }
    }
    
    // 更新UI
    onSongDeletionCompleted(successCount, failureCount);
    
    // 如果有文件删除失败，显示详细信息
    if (!failedFiles.isEmpty()) {
        QString errorMessage = "以下文件删除失败：\n";
        for (const QString& file : failedFiles) {
            errorMessage += file + "\n";
        }
        QMessageBox::warning(m_mainWindow, "文件删除失败", errorMessage);
    }
    
    updateStatusBar("彻底删除歌曲完成", 3000);
}

void MainWindowController::updatePlaylistAfterDeletion()
{
    qDebug() << "[updatePlaylistAfterDeletion] 开始更新播放列表";
    
    if (!m_audioEngine) {
        qDebug() << "[updatePlaylistAfterDeletion] AudioEngine未初始化";
        return;
    }
    
    // 获取当前播放列表和当前歌曲
    QList<Song> currentPlaylist = m_audioEngine->playlist();
    Song currentSong = m_audioEngine->currentSong();
    int currentIndex = m_audioEngine->currentIndex();
    
    qDebug() << "[updatePlaylistAfterDeletion] 当前播放列表大小:" << currentPlaylist.size();
    qDebug() << "[updatePlaylistAfterDeletion] 当前歌曲索引:" << currentIndex;
    qDebug() << "[updatePlaylistAfterDeletion] 当前歌曲:" << (currentSong.isValid() ? currentSong.title() : "无");
    
    // 检查当前播放列表是否为空
    if (currentPlaylist.isEmpty()) {
        qDebug() << "[updatePlaylistAfterDeletion] 播放列表为空，重置播放器状态";
        resetPlayerToEmptyState();
        return;
    }
    
    // 检查当前播放的歌曲是否仍然存在于数据库中
    bool currentSongStillExists = false;
    if (currentSong.isValid()) {
        SongDao songDao;
        Song existingSong = songDao.getSongById(currentSong.id());
        currentSongStillExists = existingSong.isValid();
        qDebug() << "[updatePlaylistAfterDeletion] 当前歌曲是否仍存在:" << currentSongStillExists;
    }
    
    // 如果当前播放的歌曲已被删除
    if (!currentSongStillExists && currentSong.isValid()) {
        qDebug() << "[updatePlaylistAfterDeletion] 当前播放的歌曲已被删除，停止播放";
        
        // 停止当前播放
        m_audioEngine->stop();
        
        // 从播放列表中移除已删除的歌曲
        QList<Song> updatedPlaylist;
        for (const Song& song : currentPlaylist) {
            SongDao songDao;
            Song existingSong = songDao.getSongById(song.id());
            if (existingSong.isValid()) {
                updatedPlaylist.append(song);
            } else {
                qDebug() << "[updatePlaylistAfterDeletion] 从播放列表中移除已删除的歌曲:" << song.title();
            }
        }
        
        // 更新播放列表
        if (!updatedPlaylist.isEmpty()) {
            qDebug() << "[updatePlaylistAfterDeletion] 更新播放列表，剩余歌曲数量:" << updatedPlaylist.size();
            m_audioEngine->setPlaylist(updatedPlaylist);
            
            // 自动播放第一首歌曲
            m_audioEngine->setCurrentIndex(0);
            m_audioEngine->play();
            qDebug() << "[updatePlaylistAfterDeletion] 自动播放第一首歌曲:" << updatedPlaylist.first().title();
        } else {
            qDebug() << "[updatePlaylistAfterDeletion] 播放列表为空，重置播放器状态";
            resetPlayerToEmptyState();
        }
    } else {
        // 当前歌曲仍然存在，但需要检查播放列表中是否有其他歌曲被删除
        QList<Song> updatedPlaylist;
        for (const Song& song : currentPlaylist) {
            SongDao songDao;
            Song existingSong = songDao.getSongById(song.id());
            if (existingSong.isValid()) {
                updatedPlaylist.append(song);
            } else {
                qDebug() << "[updatePlaylistAfterDeletion] 从播放列表中移除已删除的歌曲:" << song.title();
            }
        }
        
        // 如果播放列表有变化，更新播放列表
        if (updatedPlaylist.size() != currentPlaylist.size()) {
            qDebug() << "[updatePlaylistAfterDeletion] 播放列表有变化，更新播放列表";
            qDebug() << "[updatePlaylistAfterDeletion] 原播放列表大小:" << currentPlaylist.size() 
                     << "，新播放列表大小:" << updatedPlaylist.size();
            
            if (!updatedPlaylist.isEmpty()) {
                // 找到当前歌曲在新播放列表中的位置
                int newIndex = -1;
                for (int i = 0; i < updatedPlaylist.size(); ++i) {
                    if (updatedPlaylist[i].id() == currentSong.id()) {
                        newIndex = i;
                        break;
                    }
                }
                
                if (newIndex >= 0) {
                    m_audioEngine->setPlaylist(updatedPlaylist);
                    m_audioEngine->setCurrentIndex(newIndex);
                    qDebug() << "[updatePlaylistAfterDeletion] 更新播放列表，当前歌曲新索引:" << newIndex;
                } else {
                    // 当前歌曲不在新播放列表中，播放第一首
                    m_audioEngine->setPlaylist(updatedPlaylist);
                    m_audioEngine->setCurrentIndex(0);
                    m_audioEngine->play();
                    qDebug() << "[updatePlaylistAfterDeletion] 当前歌曲不在新播放列表中，播放第一首";
                }
            } else {
                qDebug() << "[updatePlaylistAfterDeletion] 更新后的播放列表为空，重置播放器状态";
                resetPlayerToEmptyState();
            }
        } else {
            qDebug() << "[updatePlaylistAfterDeletion] 播放列表无变化";
        }
    }
    
    qDebug() << "[updatePlaylistAfterDeletion] 播放列表更新完成";
}

void MainWindowController::resetPlayerToEmptyState()
{
    qDebug() << "[resetPlayerToEmptyState] 重置播放器到空状态";
    
    if (!m_audioEngine) {
        qDebug() << "[resetPlayerToEmptyState] AudioEngine未初始化";
        return;
    }
    
    // 停止播放
    m_audioEngine->stop();
    
    // 清空播放列表
    m_audioEngine->setPlaylist(QList<Song>());
    
    // 重置当前索引
    m_audioEngine->setCurrentIndex(-1);
    
    // 更新UI状态
    updatePlayButtonUI(false);
    updateCurrentSongInfo();
    
    // 清除播放进度
    if (m_musicProgressBar) {
        m_musicProgressBar->setPosition(0);
        m_musicProgressBar->setDuration(0);
    }
    
    // 更新状态栏
    updateStatusBar("播放列表为空", 3000);
    
    qDebug() << "[resetPlayerToEmptyState] 播放器已重置到空状态";
}

void MainWindowController::triggerRecentPlaySortUpdate()
{
    qDebug() << "[triggerRecentPlaySortUpdate] 触发最近播放排序更新";
    
    // 检查是否在"最近播放"标签下
    QString currentTag = "";
    if (m_tagListWidget && m_tagListWidget->currentItem()) {
        currentTag = m_tagListWidget->currentItem()->text();
    }
    
    if (currentTag == "最近播放" && m_needsRecentPlaySortUpdate) {
        logInfo("手动触发最近播放排序更新");
        m_needsRecentPlaySortUpdate = false;
        
        // 延迟一点时间后重新加载歌曲列表，确保数据库更新完成
        QTimer::singleShot(100, [this]() {
            updateSongList();
            logInfo("最近播放列表已重新排序");
        });
    } else {
        logInfo("当前不在最近播放标签下或无需更新排序");
    }
}