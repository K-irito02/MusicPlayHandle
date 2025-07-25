#include "mainthreadmanager.h"
#include <QDebug>
#include <QApplication>
#include <QThread>
#include <QElapsedTimer>
#include <algorithm>

// 静态成员初始化
MainThreadManager* MainThreadManager::m_instance = nullptr;
QMutex MainThreadManager::m_instanceMutex;

MainThreadManager* MainThreadManager::instance()
{
    if (!m_instance) {
        QMutexLocker locker(&m_instanceMutex);
        if (!m_instance) {
            m_instance = new MainThreadManager();
        }
    }
    return m_instance;
}

void MainThreadManager::cleanup()
{
    QMutexLocker locker(&m_instanceMutex);
    if (m_instance) {
        delete m_instance;
        m_instance = nullptr;
    }
}

MainThreadManager::MainThreadManager(QObject* parent)
    : QObject(parent)
    , m_updateTimer(new QTimer(this))
    , m_delayedTimer(new QTimer(this))
    , m_batchTimer(new QTimer(this))
    , m_statisticsTimer(new QTimer(this))
    , m_updateInterval(DEFAULT_UPDATE_INTERVAL)
    , m_defaultPriority(0)
    , m_batchSize(DEFAULT_BATCH_SIZE)
    , m_batchTimeout(DEFAULT_BATCH_TIMEOUT)
    , m_updatesPaused(false)
    , m_debugMode(false)
    , m_pendingUpdateCount(0)
    , m_processedUpdateCount(0)
    , m_failedUpdateCount(0)
    , m_totalProcessingTime(0)
    , m_maxProcessingTime(0)
    , m_minProcessingTime(0)
{
    initializeTimers();
    resetStatistics();
    
    if (m_debugMode) {
        qDebug() << "MainThreadManager: 初始化完成";
    }
}

MainThreadManager::~MainThreadManager()
{
    cleanupTimers();
    
    if (m_debugMode) {
        qDebug() << "MainThreadManager: 已销毁";
    }
}

void MainThreadManager::scheduleUIUpdate(std::function<void()> updateFunction, 
                                       const QString& description, int priority)
{
    if (!updateFunction) {
        return;
    }
    
    UIUpdateTask task(updateFunction, description, priority);
    
    QMutexLocker locker(&m_updateMutex);
    
    if (m_updateQueue.size() >= MAX_QUEUE_SIZE) {
        if (m_errorHandler) {
            m_errorHandler("更新队列已满，丢弃任务: " + description);
        }
        return;
    }
    
    insertByPriority(task);
    m_pendingUpdateCount++;
    
    emit uiUpdateScheduled(description);
    
    if (m_debugMode) {
        qDebug() << "MainThreadManager: 调度UI更新:" << description;
    }
}

void MainThreadManager::scheduleUIUpdateDelayed(std::function<void()> updateFunction, int delayMs,
                                               const QString& description, int priority)
{
    if (!updateFunction) {
        return;
    }
    
    UIUpdateTask task(updateFunction, description, priority, true, delayMs);
    
    QMutexLocker locker(&m_delayedMutex);
    
    if (m_delayedQueue.size() >= MAX_QUEUE_SIZE) {
        if (m_errorHandler) {
            m_errorHandler("延迟更新队列已满，丢弃任务: " + description);
        }
        return;
    }
    
    m_delayedQueue.enqueue(task);
    m_pendingUpdateCount++;
    
    emit uiUpdateScheduled("延迟: " + description);
    
    if (m_debugMode) {
        qDebug() << "MainThreadManager: 调度延迟UI更新:" << description << "延迟:" << delayMs << "ms";
    }
}

void MainThreadManager::batchUIUpdates(const QList<std::function<void()>>& updates, 
                                      const QString& batchDescription)
{
    if (updates.isEmpty()) {
        return;
    }
    
    QMutexLocker locker(&m_batchMutex);
    
    for (const auto& updateFunc : updates) {
        if (updateFunc) {
            UIUpdateTask task(updateFunc, batchDescription, m_defaultPriority);
            m_batchQueue.enqueue(task);
        }
    }
    
    m_pendingUpdateCount += updates.size();
    
    emit batchUpdateStarted(updates.size());
    
    if (m_debugMode) {
        qDebug() << "MainThreadManager: 调度批量UI更新:" << batchDescription << "任务数:" << updates.size();
    }
}

void MainThreadManager::handleUIEvent(const UIEvent& event)
{
    if (m_debugMode) {
        qDebug() << "MainThreadManager: 处理UI事件:" << formatUIEvent(event);
    }
    
    processUIEvent(event);
}

void MainThreadManager::handlePlaybackEvent(const QVariant& data)
{
    UIEvent event(UIEventType::PlaybackUpdate, "播放事件", data);
    handleUIEvent(event);
}

void MainThreadManager::handleDatabaseEvent(const QVariant& data)
{
    UIEvent event(UIEventType::DatabaseUpdate, "数据库事件", data);
    handleUIEvent(event);
}

void MainThreadManager::handleFileEvent(const QVariant& data)
{
    UIEvent event(UIEventType::FileUpdate, "文件事件", data);
    handleUIEvent(event);
}

void MainThreadManager::handleAudioEvent(const QVariant& data)
{
    UIEvent event(UIEventType::AudioUpdate, "音频事件", data);
    handleUIEvent(event);
}

void MainThreadManager::handleTagEvent(const QVariant& data)
{
    UIEvent event(UIEventType::TagUpdate, "标签事件", data);
    handleUIEvent(event);
}

void MainThreadManager::handlePlaylistEvent(const QVariant& data)
{
    UIEvent event(UIEventType::PlaylistUpdate, "播放列表事件", data);
    handleUIEvent(event);
}

void MainThreadManager::handleErrorEvent(const QVariant& data)
{
    UIEvent event(UIEventType::ErrorUpdate, "错误事件", data);
    handleUIEvent(event);
}

bool MainThreadManager::isMainThread() const
{
    return QThread::currentThread() == QApplication::instance()->thread();
}

bool MainThreadManager::isCurrentThreadMainThread() const
{
    return isMainThread();
}

void MainThreadManager::setUpdatePriority(int priority)
{
    m_defaultPriority = priority;
}

int MainThreadManager::getUpdatePriority() const
{
    return m_defaultPriority;
}

void MainThreadManager::setUpdateInterval(int intervalMs)
{
    m_updateInterval = intervalMs;
    configureUpdateTimer();
}

int MainThreadManager::getUpdateInterval() const
{
    return m_updateInterval;
}

void MainThreadManager::pauseUpdates()
{
    m_updatesPaused = true;
    emit updatesPaused();
    
    if (m_debugMode) {
        qDebug() << "MainThreadManager: 暂停更新";
    }
}

void MainThreadManager::resumeUpdates()
{
    m_updatesPaused = false;
    emit updatesResumed();
    
    if (m_debugMode) {
        qDebug() << "MainThreadManager: 恢复更新";
    }
}

bool MainThreadManager::isUpdatesPaused() const
{
    return m_updatesPaused;
}

int MainThreadManager::getPendingUpdateCount() const
{
    QMutexLocker locker(&m_statisticsMutex);
    return m_pendingUpdateCount;
}

int MainThreadManager::getProcessedUpdateCount() const
{
    QMutexLocker locker(&m_statisticsMutex);
    return m_processedUpdateCount;
}

int MainThreadManager::getFailedUpdateCount() const
{
    QMutexLocker locker(&m_statisticsMutex);
    return m_failedUpdateCount;
}

qint64 MainThreadManager::getAverageProcessingTime() const
{
    QMutexLocker locker(&m_statisticsMutex);
    if (m_processedUpdateCount > 0) {
        return m_totalProcessingTime / m_processedUpdateCount;
    }
    return 0;
}

void MainThreadManager::setBatchSize(int size)
{
    m_batchSize = size;
}

int MainThreadManager::getBatchSize() const
{
    return m_batchSize;
}

void MainThreadManager::setBatchTimeout(int timeoutMs)
{
    m_batchTimeout = timeoutMs;
    configureBatchTimer();
}

int MainThreadManager::getBatchTimeout() const
{
    return m_batchTimeout;
}

void MainThreadManager::setErrorHandler(std::function<void(const QString&)> handler)
{
    m_errorHandler = handler;
}

void MainThreadManager::enableDebugMode(bool enabled)
{
    m_debugMode = enabled;
}

bool MainThreadManager::isDebugModeEnabled() const
{
    return m_debugMode;
}

void MainThreadManager::dumpPendingUpdates() const
{
    if (!m_debugMode) {
        return;
    }
    
    QMutexLocker updateLocker(&m_updateMutex);
    QMutexLocker delayedLocker(&m_delayedMutex);
    QMutexLocker batchLocker(&m_batchMutex);
    
    qDebug() << "MainThreadManager: 待处理更新统计:";
    qDebug() << "  更新队列:" << m_updateQueue.size();
    qDebug() << "  延迟队列:" << m_delayedQueue.size();
    qDebug() << "  批量队列:" << m_batchQueue.size();
    qDebug() << "  总待处理:" << m_pendingUpdateCount;
}

void MainThreadManager::processUIUpdateQueue()
{
    if (m_updatesPaused) {
        return;
    }
    
    QMutexLocker locker(&m_updateMutex);
    
    if (m_updateQueue.isEmpty()) {
        return;
    }
    
    UIUpdateTask task = m_updateQueue.dequeue();
    m_pendingUpdateCount--;
    
    locker.unlock();
    
    executeUIUpdateTask(task);
}

void MainThreadManager::processDelayedUpdates()
{
    if (m_updatesPaused) {
        return;
    }
    
    QMutexLocker locker(&m_delayedMutex);
    
    if (m_delayedQueue.isEmpty()) {
        return;
    }
    
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    QList<UIUpdateTask> readyTasks;
    
    while (!m_delayedQueue.isEmpty()) {
        UIUpdateTask task = m_delayedQueue.head();
        if (currentTime - task.timestamp >= task.delayMs) {
            m_delayedQueue.dequeue();
            readyTasks.append(task);
            m_pendingUpdateCount--;
        } else {
            break;
        }
    }
    
    locker.unlock();
    
    for (const auto& task : readyTasks) {
        executeUIUpdateTask(task);
    }
}

void MainThreadManager::processBatchUpdates()
{
    if (m_updatesPaused) {
        return;
    }
    
    QMutexLocker locker(&m_batchMutex);
    
    if (m_batchQueue.isEmpty()) {
        return;
    }
    
    QList<UIUpdateTask> batchTasks;
    int count = qMin(m_batchSize, m_batchQueue.size());
    
    for (int i = 0; i < count; ++i) {
        batchTasks.append(m_batchQueue.dequeue());
        m_pendingUpdateCount--;
    }
    
    locker.unlock();
    
    executeUIUpdateBatch(batchTasks);
}

void MainThreadManager::updateStatistics()
{
    QMutexLocker locker(&m_statisticsMutex);
    
    emit statisticsUpdated(m_pendingUpdateCount, m_processedUpdateCount, m_failedUpdateCount);
    
    if (m_debugMode) {
        qDebug() << "MainThreadManager: 统计更新 - 待处理:" << m_pendingUpdateCount
                 << "已处理:" << m_processedUpdateCount
                 << "失败:" << m_failedUpdateCount;
    }
}

void MainThreadManager::handleTimeout()
{
    if (m_debugMode) {
        qDebug() << "MainThreadManager: 处理超时";
    }
    
    processBatchUpdates();
}

void MainThreadManager::initializeTimers()
{
    configureUpdateTimer();
    configureDelayedTimer();
    configureBatchTimer();
    configureStatisticsTimer();
}

void MainThreadManager::cleanupTimers()
{
    if (m_updateTimer) {
        m_updateTimer->stop();
    }
    if (m_delayedTimer) {
        m_delayedTimer->stop();
    }
    if (m_batchTimer) {
        m_batchTimer->stop();
    }
    if (m_statisticsTimer) {
        m_statisticsTimer->stop();
    }
}

void MainThreadManager::configureUpdateTimer()
{
    if (m_updateTimer) {
        m_updateTimer->setInterval(m_updateInterval);
        connect(m_updateTimer, &QTimer::timeout, this, &MainThreadManager::processUIUpdateQueue);
        m_updateTimer->start();
    }
}

void MainThreadManager::configureDelayedTimer()
{
    if (m_delayedTimer) {
        m_delayedTimer->setInterval(100); // 每100ms检查一次延迟任务
        connect(m_delayedTimer, &QTimer::timeout, this, &MainThreadManager::processDelayedUpdates);
        m_delayedTimer->start();
    }
}

void MainThreadManager::configureBatchTimer()
{
    if (m_batchTimer) {
        m_batchTimer->setInterval(m_batchTimeout);
        connect(m_batchTimer, &QTimer::timeout, this, &MainThreadManager::processBatchUpdates);
        m_batchTimer->start();
    }
}

void MainThreadManager::configureStatisticsTimer()
{
    if (m_statisticsTimer) {
        m_statisticsTimer->setInterval(STATISTICS_UPDATE_INTERVAL);
        connect(m_statisticsTimer, &QTimer::timeout, this, &MainThreadManager::updateStatistics);
        m_statisticsTimer->start();
    }
}

void MainThreadManager::executeUIUpdateTask(const UIUpdateTask& task)
{
    if (!task.function) {
        return;
    }
    
    QElapsedTimer timer;
    timer.start();
    
    try {
        task.function();
        
        qint64 processingTime = timer.elapsed();
        updateProcessingStatistics(processingTime);
        
        m_processedUpdateCount++;
        emit uiUpdateProcessed(task.description);
        
        if (m_debugMode) {
            qDebug() << "MainThreadManager: 执行UI更新成功:" << task.description
                     << "耗时:" << processingTime << "ms";
        }
    } catch (const std::exception& e) {
        QString error = QString::fromStdString(e.what());
        handleUpdateError(error, task.description);
    } catch (...) {
        handleUpdateError("未知异常", task.description);
    }
}

void MainThreadManager::executeUIUpdateBatch(const QList<UIUpdateTask>& tasks)
{
    if (tasks.isEmpty()) {
        return;
    }
    
    int processed = 0;
    int failed = 0;
    
    for (const auto& task : tasks) {
        if (!task.function) {
            failed++;
            continue;
        }
        
        QElapsedTimer timer;
        timer.start();
        
        try {
            task.function();
            
            qint64 processingTime = timer.elapsed();
            updateProcessingStatistics(processingTime);
            
            processed++;
            
            if (m_debugMode) {
                qDebug() << "MainThreadManager: 批量更新成功:" << task.description;
            }
        } catch (const std::exception& e) {
            QString error = QString::fromStdString(e.what());
            handleUpdateError(error, task.description);
            failed++;
        } catch (...) {
            handleUpdateError("未知异常", task.description);
            failed++;
        }
    }
    
    m_processedUpdateCount += processed;
    m_failedUpdateCount += failed;
    
    emit batchUpdateProgress(processed, tasks.size());
    emit batchUpdateFinished(processed, failed);
    
    if (m_debugMode) {
        qDebug() << "MainThreadManager: 批量更新完成 - 成功:" << processed << "失败:" << failed;
    }
}

void MainThreadManager::clearUpdateQueue()
{
    QMutexLocker locker(&m_updateMutex);
    m_updateQueue.clear();
    m_pendingUpdateCount = 0;
    emit queueCleared();
}

void MainThreadManager::clearDelayedQueue()
{
    QMutexLocker locker(&m_delayedMutex);
    m_delayedQueue.clear();
    m_pendingUpdateCount = 0;
}

void MainThreadManager::clearBatchQueue()
{
    QMutexLocker locker(&m_batchMutex);
    m_batchQueue.clear();
    m_pendingUpdateCount = 0;
}

void MainThreadManager::sortUpdateQueue()
{
    // 按优先级排序（高优先级在前）
    std::sort(m_updateQueue.begin(), m_updateQueue.end(), 
              [](const UIUpdateTask& a, const UIUpdateTask& b) {
                  return a.priority > b.priority;
              });
}

void MainThreadManager::insertByPriority(UIUpdateTask& task)
{
    // 简单的优先级插入（高优先级在前）
    auto it = m_updateQueue.begin();
    while (it != m_updateQueue.end() && it->priority >= task.priority) {
        ++it;
    }
    m_updateQueue.insert(it, task);
}

void MainThreadManager::processUIEvent(const UIEvent& event)
{
    handleEventInternal(event.type, event.data);
    emit eventProcessed(event.type, event.description);
}

void MainThreadManager::handleEventInternal(UIEventType type, const QVariant& data)
{
    Q_UNUSED(data) // 暂时未使用data参数
    
    switch (type) {
    case UIEventType::PlaybackUpdate:
        // 处理播放更新事件
        break;
    case UIEventType::DatabaseUpdate:
        // 处理数据库更新事件
        break;
    case UIEventType::FileUpdate:
        // 处理文件更新事件
        break;
    case UIEventType::AudioUpdate:
        // 处理音频更新事件
        break;
    case UIEventType::TagUpdate:
        // 处理标签更新事件
        break;
    case UIEventType::PlaylistUpdate:
        // 处理播放列表更新事件
        break;
    case UIEventType::ErrorUpdate:
        // 处理错误更新事件
        break;
    case UIEventType::StatusUpdate:
        // 处理状态更新事件
        break;
    case UIEventType::ProgressUpdate:
        // 处理进度更新事件
        break;
    case UIEventType::Generic:
        // 处理通用事件
        break;
    }
}

void MainThreadManager::handleUpdateError(const QString& error, const QString& taskDescription)
{
    m_failedUpdateCount++;
    
    if (m_errorHandler) {
        m_errorHandler(QString("UI更新失败: %1 - %2").arg(taskDescription, error));
    }
    
    emit uiUpdateFailed(taskDescription, error);
    
    if (m_debugMode) {
        qDebug() << "MainThreadManager: UI更新失败:" << taskDescription << "错误:" << error;
    }
}

void MainThreadManager::logError(const QString& error)
{
    if (m_errorHandler) {
        m_errorHandler(error);
    }
    
    if (m_debugMode) {
        qDebug() << "MainThreadManager: 错误:" << error;
    }
}

void MainThreadManager::logInfo(const QString& message)
{
    if (m_debugMode) {
        qDebug() << "MainThreadManager: 信息:" << message;
    }
}

void MainThreadManager::logDebug(const QString& message)
{
    if (m_debugMode) {
        qDebug() << "MainThreadManager: 调试:" << message;
    }
}

void MainThreadManager::updateProcessingStatistics(qint64 processingTime)
{
    QMutexLocker locker(&m_statisticsMutex);
    
    m_totalProcessingTime += processingTime;
    
    if (processingTime > m_maxProcessingTime) {
        m_maxProcessingTime = processingTime;
    }
    
    if (m_minProcessingTime == 0 || processingTime < m_minProcessingTime) {
        m_minProcessingTime = processingTime;
    }
}

void MainThreadManager::resetStatistics()
{
    QMutexLocker locker(&m_statisticsMutex);
    
    m_pendingUpdateCount = 0;
    m_processedUpdateCount = 0;
    m_failedUpdateCount = 0;
    m_totalProcessingTime = 0;
    m_maxProcessingTime = 0;
    m_minProcessingTime = 0;
}

bool MainThreadManager::validateMainThread() const
{
    return isMainThread();
}

void MainThreadManager::ensureMainThread() const
{
    if (!isMainThread()) {
        throw std::runtime_error("操作必须在主线程中执行");
    }
}

QString MainThreadManager::formatUpdateTask(const UIUpdateTask& task) const
{
    return QString("任务[%1] 优先级:%2 延迟:%3ms 描述:%4")
           .arg(task.timestamp)
           .arg(task.priority)
           .arg(task.delayMs)
           .arg(task.description);
}

QString MainThreadManager::formatUIEvent(const UIEvent& event) const
{
    return QString("事件[%1] 类型:%2 优先级:%3 描述:%4")
           .arg(event.timestamp)
           .arg(static_cast<int>(event.type))
           .arg(event.priority)
           .arg(event.description);
}

void MainThreadManager::dumpQueueState() const
{
    if (!m_debugMode) {
        return;
    }
    
    QMutexLocker updateLocker(&m_updateMutex);
    QMutexLocker delayedLocker(&m_delayedMutex);
    QMutexLocker batchLocker(&m_batchMutex);
    
    qDebug() << "MainThreadManager: 队列状态:";
    qDebug() << "  更新队列大小:" << m_updateQueue.size();
    qDebug() << "  延迟队列大小:" << m_delayedQueue.size();
    qDebug() << "  批量队列大小:" << m_batchQueue.size();
}

// ThreadSafeUIUpdater 实现
void ThreadSafeUIUpdater::updateProgressBar(QProgressBar* progressBar, int value)
{
    if (!progressBar) return;
    
    MainThreadManager::instance()->scheduleUIUpdate([progressBar, value]() {
        if (progressBar) {
            progressBar->setValue(value);
        }
    }, QString("Update ProgressBar %1").arg(progressBar->objectName()));
}

void ThreadSafeUIUpdater::updateLabel(QLabel* label, const QString& text)
{
    if (!label) return;
    
    MainThreadManager::instance()->scheduleUIUpdate([label, text]() {
        if (label) {
            label->setText(text);
        }
    }, QString("Update Label %1").arg(label->objectName()));
}

void ThreadSafeUIUpdater::updateListWidget(QListWidget* listWidget, const QStringList& items)
{
    if (!listWidget) return;
    
    MainThreadManager::instance()->scheduleUIUpdate([listWidget, items]() {
        if (listWidget) {
            listWidget->clear();
            listWidget->addItems(items);
        }
    }, QString("Update ListWidget %1").arg(listWidget->objectName()));
}

void ThreadSafeUIUpdater::updateStatusBar(QStatusBar* statusBar, const QString& message, int timeout)
{
    if (!statusBar) return;
    
    MainThreadManager::instance()->scheduleUIUpdate([statusBar, message, timeout]() {
        if (statusBar) {
            statusBar->showMessage(message, timeout);
        }
    }, QString("Update StatusBar %1").arg(statusBar->objectName()));
}

void ThreadSafeUIUpdater::updateButton(QPushButton* button, const QString& text, bool enabled)
{
    if (!button) return;
    
    MainThreadManager::instance()->scheduleUIUpdate([button, text, enabled]() {
        if (button) {
            button->setText(text);
            button->setEnabled(enabled);
        }
    }, QString("Update Button %1").arg(button->objectName()));
}

void ThreadSafeUIUpdater::updateSlider(QSlider* slider, int value)
{
    if (!slider) return;
    
    MainThreadManager::instance()->scheduleUIUpdate([slider, value]() {
        if (slider) {
            slider->setValue(value);
        }
    }, QString("Update Slider %1").arg(slider->objectName()));
}

void ThreadSafeUIUpdater::updateTextEdit(QTextEdit* textEdit, const QString& text)
{
    if (!textEdit) return;
    
    MainThreadManager::instance()->scheduleUIUpdate([textEdit, text]() {
        if (textEdit) {
            textEdit->setText(text);
        }
    }, QString("Update TextEdit %1").arg(textEdit->objectName()));
}

void ThreadSafeUIUpdater::updateTableWidget(QTableWidget* table, int row, int column, const QString& text)
{
    if (!table) return;
    
    MainThreadManager::instance()->scheduleUIUpdate([table, row, column, text]() {
        if (table) {
            QTableWidgetItem* item = table->item(row, column);
            if (!item) {
                item = new QTableWidgetItem();
                table->setItem(row, column, item);
            }
            item->setText(text);
        }
    }, QString("Update TableWidget %1").arg(table->objectName()));
}

void ThreadSafeUIUpdater::updateTreeWidget(QTreeWidget* tree, QTreeWidgetItem* item, int column, const QString& text)
{
    if (!tree) return;
    
    MainThreadManager::instance()->scheduleUIUpdate([tree, item, column, text]() {
        if (tree && item) {
            item->setText(column, text);
        }
    }, QString("Update TreeWidget %1").arg(tree->objectName()));
}

void ThreadSafeUIUpdater::updateComboBox(QComboBox* comboBox, const QStringList& items)
{
    if (!comboBox) return;
    
    MainThreadManager::instance()->scheduleUIUpdate([comboBox, items]() {
        if (comboBox) {
            comboBox->clear();
            comboBox->addItems(items);
        }
    }, QString("Update ComboBox %1").arg(comboBox->objectName()));
}

void ThreadSafeUIUpdater::updateGroupBox(QGroupBox* groupBox, const QString& title, bool enabled)
{
    if (!groupBox) return;
    
    MainThreadManager::instance()->scheduleUIUpdate([groupBox, title, enabled]() {
        if (groupBox) {
            groupBox->setTitle(title);
            groupBox->setEnabled(enabled);
        }
    }, QString("Update GroupBox %1").arg(groupBox->objectName()));
}

void ThreadSafeUIUpdater::updateCheckBox(QCheckBox* checkBox, bool checked)
{
    if (!checkBox) return;
    
    MainThreadManager::instance()->scheduleUIUpdate([checkBox, checked]() {
        if (checkBox) {
            checkBox->setChecked(checked);
        }
    }, QString("Update CheckBox %1").arg(checkBox->objectName()));
}

void ThreadSafeUIUpdater::updateRadioButton(QRadioButton* radioButton, bool checked)
{
    if (!radioButton) return;
    
    MainThreadManager::instance()->scheduleUIUpdate([radioButton, checked]() {
        if (radioButton) {
            radioButton->setChecked(checked);
        }
    }, QString("Update RadioButton %1").arg(radioButton->objectName()));
}

void ThreadSafeUIUpdater::updateImageLabel(QLabel* label, const QPixmap& pixmap)
{
    if (!label) return;
    
    MainThreadManager::instance()->scheduleUIUpdate([label, pixmap]() {
        if (label) {
            label->setPixmap(pixmap);
        }
    }, QString("Update ImageLabel %1").arg(label->objectName()));
}

void ThreadSafeUIUpdater::updateToolBar(QToolBar* toolBar, bool visible)
{
    if (!toolBar) return;
    
    MainThreadManager::instance()->scheduleUIUpdate([toolBar, visible]() {
        if (toolBar) {
            toolBar->setVisible(visible);
        }
    }, QString("Update ToolBar %1").arg(toolBar->objectName()));
}

void ThreadSafeUIUpdater::updateMenu(QMenu* menu, bool enabled)
{
    if (!menu) return;
    
    MainThreadManager::instance()->scheduleUIUpdate([menu, enabled]() {
        if (menu) {
            menu->setEnabled(enabled);
        }
    }, QString("Update Menu %1").arg(menu->objectName()));
}

void ThreadSafeUIUpdater::updateAction(QAction* action, bool enabled, const QString& text)
{
    if (!action) return;
    
    MainThreadManager::instance()->scheduleUIUpdate([action, enabled, text]() {
        action->setEnabled(enabled);
        if (!text.isEmpty()) {
            action->setText(text);
        }
    }, QString("更新动作 %1").arg(action->objectName()));
}

void ThreadSafeUIUpdater::updateWindow(QWidget* window, const QString& title)
{
    if (!window) return;
    
    MainThreadManager::instance()->scheduleUIUpdate([window, title]() {
        if (window) {
            window->setWindowTitle(title);
        }
    }, QString("Update Window %1").arg(window->objectName()));
}

void ThreadSafeUIUpdater::setWidgetFocus(QWidget* widget)
{
    if (!widget) return;
    
    MainThreadManager::instance()->scheduleUIUpdate([widget]() {
        if (widget) {
            widget->setFocus();
        }
    }, QString("Set Focus %1").arg(widget->objectName()));
}

void ThreadSafeUIUpdater::updateWidgetStyle(QWidget* widget, const QString& styleSheet)
{
    if (!widget) return;
    
    MainThreadManager::instance()->scheduleUIUpdate([widget, styleSheet]() {
        if (widget) {
            widget->setStyleSheet(styleSheet);
        }
    }, QString("Update Style %1").arg(widget->objectName()));
}

void ThreadSafeUIUpdater::batchUpdateWidgets(const QList<std::function<void()>>& updates)
{
    MainThreadManager::instance()->batchUIUpdates(updates, "批量控件更新");
} 