#ifndef ADDSONGDIALOGCONTROLLER_H
#define ADDSONGDIALOGCONTROLLER_H

#include <QObject>
#include <QStringList>
#include <QFileDialog>
#include <QDir>
#include <QFileInfo>
#include <QMimeData>
#include <QUrl>
#include <QTimer>
#include <QProgressBar>
#include <QLabel>
#include <QMessageBox>
#include <QInputDialog>
#include <QColorDialog>
#include <QPixmap>
#include <QStandardPaths>
#include <QMutex>
#include <QSettings>

// 前向声明
class AddSongDialog;
class TagManager;
class AudioEngine;
class DatabaseManager;
class Logger;

#include "../../models/song.h"
#include "../../models/tag.h"
#include "../../core/logger.h"

// 文件状态枚举
enum class FileStatus {
    Pending,
    Processing,
    Completed,
    Failed,
    Skipped
};

// 文件信息结构
struct FileInfo {
    QString filePath;
    QString fileName;
    QString displayName;
    QString title;
    QString artist;
    QString album;
    FileStatus status;
    QString errorMessage;
    qint64 fileSize;
    QString format;
    qint64 duration;
    QString tagAssignment;
    bool isValid;
    
    FileInfo() : status(FileStatus::Pending), fileSize(0), duration(0), isValid(false) {}
};

// 标签信息结构
struct TagInfo {
    QString name;
    QString displayName;
    QString color;
    QString iconPath;
    QString description;
    int songCount;
    bool isDefault;
    bool isEditable;
    
    TagInfo() : songCount(0), isDefault(false), isEditable(true) {}
};

// 操作记录结构，用于撤销功能
struct Operation {
    QString type;      // 操作类型："assign"、"unassign"等
    QString filePath;  // 相关文件路径
    QString tagName;   // 相关标签名称
    QVariant extraData; // 额外数据
};

class AddSongDialogController : public QObject
{
    Q_OBJECT

public:
    explicit AddSongDialogController(AddSongDialog* dialog, QObject* parent = nullptr);
    ~AddSongDialogController();

    // 初始化和清理
    bool initialize();
    void shutdown();
    bool isInitialized() const;

    // 文件操作
    bool addFiles(const QStringList& filePaths); // 返回值修正为bool
    void removeFiles(const QStringList& filePaths);
    void clearFiles();
    QStringList getSelectedFiles() const;
    QStringList getAllFiles() const;
    
    // 标签操作
    void loadAvailableTags();
    void createTag(const QString& name, const QString& color = "", const QString& iconPath = "");
    void deleteTag(const QString& name);
    void editTag(const QString& oldName, const QString& newName, const QString& color = "", const QString& iconPath = "");
    void assignTagToFiles(const QString& tagName, const QStringList& files);
    void unassignTagFromFiles(const QString& tagName, const QStringList& files);
    
    // 批量操作
    void assignTagToAllFiles(const QString& tagName);
    void clearAllTagAssignments();
    void processSelectedFiles();
    
    // 文件分析
    void analyzeFiles(const QStringList& filePaths);
    void validateFiles(const QStringList& filePaths);
    
    // 设置
    void setAutoAssignToDefault(bool enabled);
    void setDuplicateHandling(int mode); // 0: 跳过, 1: 覆盖, 2: 询问
    void setProgressCallback(std::function<void(int, const QString&)> callback);
    
    // 获取信息
    QList<FileInfo> getFileInfoList() const;
    QList<FileInfo> getFileList() const; // 获取文件列表
    QList<TagInfo> getTagInfoList() const;
    int getProcessedFileCount() const;
    int getFailedFileCount() const;
    bool canUndo() const; // 检查是否可以撤销操作
    QStringList getSelectedTags() const; // 补全声明
    
    // 新增/修正声明
    bool isValidAudioFormat(const QString& format) const;
    QString getFileExtension(const QString& filePath) const;
    bool hasValidExtension(const QString& filePath) const;
    void selectAllFiles();
    void clearSelection();
    void batchAssignTags(const QStringList& files, const QStringList& tags);
    void batchUnassignTags(const QStringList& files, const QStringList& tags);
    void analyzeFile(const QString& filePath);
    FileInfo getFileInfo(const QString& filePath) const;
    TagInfo getTagInfo(const QString& tagName) const;
    
    // 标签数据库操作
    void loadTagsFromDatabase();
    void updateTagList();
    
    // 日志相关
    void logWarning(const QString& message);
    
    // 常量
    static const QStringList SUPPORTED_FORMATS;
    static const int MAX_FILE_SIZE = 500 * 1024 * 1024; // 500MB
    static const int BATCH_SIZE = 10;
    static const int PROGRESS_UPDATE_INTERVAL = 100; // ms

signals:
    // 文件操作信号
    void filesAdded(const QStringList& files);
    void filesRemoved(const QStringList& files);
    void filesCleared();
    void fileProcessed(const QString& file, bool success);
    void fileAnalyzed(const QString& file, const FileInfo& info);
    
    // 标签相关信号
    void tagCreated(const QString& name, bool isSystemTag);
    void tagDeleted(const QString& name);
    void tagEdited(const QString& oldName, const QString& newName);
    void tagAssigned(const QString& tagName, const QStringList& files);
    void tagUnassigned(const QString& tagName, const QStringList& files);
    void tagListChanged(); // 通知主界面刷新标签列表
    
    // 进度信号
    void progressUpdated(int value, const QString& message);
    void operationStarted(const QString& operation);
    void operationCompleted(const QString& operation, bool success);
    
    // 错误信号
    void errorOccurred(const QString& error);
    void warningOccurred(const QString& warning);
    void dialogAccepted();
    void dialogRejected();
    
    // 选择变化信号
    void filesSelectionChanged(const QStringList& selectedFiles);
    void tagsSelectionChanged(const QStringList& selectedTags);

public slots:
    // 文件操作槽
    void onAddFilesRequested();
    void onRemoveFilesRequested();
    void onClearFilesRequested();
    void onFileSelectionChanged();
    
    // 标签操作槽
    void onCreateTagRequested();
    void onDeleteTagRequested();
    void editTagFromMenu(const QString& tagName); // 从操作菜单编辑标签
    void onAssignTagRequested();
    void onUnassignTagRequested();
    void onTagSelectionChanged();
    
    // 对话框操作槽
    void onAcceptRequested();
    void onRejectRequested();
    void onUndoRequested();
    void onExitWithoutSavingRequested();
    void onSaveAndExitRequested();
    
    // 拖放操作槽
    void onFilesDropped(const QStringList& files);

private slots:
    void onFileProcessingTimer();
    void onProgressUpdateTimer();
    void onFileAnalysisCompleted();

private:
    AddSongDialog* m_dialog;
    TagManager* m_tagManager;
    AudioEngine* m_audioEngine;
    DatabaseManager* m_databaseManager;
    Logger* m_logger;
    
    // 数据存储
    QList<FileInfo> m_fileInfoList;
    QList<TagInfo> m_tagInfoList;
    QStringList m_selectedFiles;
    QStringList m_selectedTags;
    QList<Operation> m_recentOperations; // 最近的操作记录，用于撤销功能
    
    // 处理状态
    bool m_initialized;
    bool m_processing;
    int m_processedCount;
    int m_failedCount;
    int m_totalCount;
    
    // 设置
    bool m_autoAssignToDefault;
    int m_duplicateHandling;
    std::function<void(int, const QString&)> m_progressCallback;
    
    // 定时器
    QTimer* m_processingTimer;
    QTimer* m_progressTimer;
    
    // 线程安全
    QMutex m_mutex;
    
    // 设置
    QSettings* m_settings;
    QString m_lastDirectory; // 最后使用的目录路径
    QString m_lastUsedDirectory; // 最后使用的目录路径（备用）
    
    // 内部方法
    void setupConnections();
    void loadSettings();
    void saveSettings();
    
    // 文件处理
    void processFileInternal(const QString& filePath);
    void analyzeFileInternal(const QString& filePath);
    bool validateFileInternal(const QString& filePath) const;
    FileInfo extractFileInfo(const QString& filePath);
    
    // 标签处理
    void saveTagToDatabase(const TagInfo& tagInfo);
    void deleteTagFromDatabase(const QString& tagName);
    TagInfo createDefaultTagInfo(const QString& name);
    
    // UI更新
    void updateFileList();
    void updateProgressBar();
    void updateStatusBar();
    void updateButtonStates();
    void refreshUI();
    
    // 工具方法
    QString formatFileSize(qint64 size) const;
    QString formatDuration(qint64 duration) const;
    QString getFileFormat(const QString& filePath) const;
    bool isAudioFile(const QString& filePath) const;
    bool isDuplicateFile(const QString& filePath) const;
    QString generateUniqueFileName(const QString& filePath) const;
    
    // 错误处理
    void handleError(const QString& error);
    void handleWarning(const QString& warning);
    void logInfo(const QString& message) const;
    void logError(const QString& error) const;
    void logDebug(const QString& message) const;
    
    // 内部操作方法
    void assignTag(const QString& filePath, const QString& tagName);
    void unassignTag(const QString& filePath, const QString& tagName);
    void processFiles();
    void undoOperation();
};

#endif // ADDSONGDIALOGCONTROLLER_H