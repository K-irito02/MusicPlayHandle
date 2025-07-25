#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QSlider>
#include <QProgressBar>
#include <QTextEdit>
#include <QTimer>
#include <QDebug>
#include <memory>

// 包含改进的组件
#include "../src/audio/improvedaudioengine.h"
#include "../src/ui/dialogs/improvedplayinterface.h"
#include "../src/core/performancemanager.h"
#include "../src/core/resourcemanager.h"
#include "../src/threading/threadpoolmanager.h"

/**
 * @brief 演示应用程序
 * 展示新架构的使用方法和优势
 */
class ImprovedArchitectureDemo : public QWidget {
    Q_OBJECT

public:
    explicit ImprovedArchitectureDemo(QWidget *parent = nullptr)
        : QWidget(parent)
        , m_performanceLabel(nullptr)
        , m_resourceLabel(nullptr)
        , m_statusTextEdit(nullptr)
        , m_statusTimer(new QTimer(this))
    {
        setupUI();
        initializeComponents();
        connectSignals();
        startDemo();
    }

    ~ImprovedArchitectureDemo() {
        cleanup();
    }

private slots:
    void onCreateEngine1() {
        // 创建第一个音频引擎实例（性能优化模式）
        m_audioEngine1 = AudioEngineFactory::createPerformanceOptimizedEngine("Engine1");
        
        if (m_audioEngine1 && m_audioEngine1->isInitialized()) {
            appendStatus("✅ 音频引擎1创建成功（性能优化模式）");
            
            // 创建对应的播放界面
            createPlayInterface1();
        } else {
            appendStatus("❌ 音频引擎1创建失败");
        }
    }

    void onCreateEngine2() {
        // 创建第二个音频引擎实例（省电模式）
        m_audioEngine2 = AudioEngineFactory::createPowerSaverEngine("Engine2");
        
        if (m_audioEngine2 && m_audioEngine2->isInitialized()) {
            appendStatus("✅ 音频引擎2创建成功（省电模式）");
            
            // 创建对应的播放界面
            createPlayInterface2();
        } else {
            appendStatus("❌ 音频引擎2创建失败");
        }
    }

    void onTestResourceLocking() {
        // 演示资源独占机制
        appendStatus("🔒 测试音频资源独占机制...");
        
        // 尝试获取资源锁
        {
            SCOPED_AUDIO_LOCK("TestLock", "DemoApp");
            appendStatus("✅ 成功获取资源锁");
            
            // 模拟音频操作
            QThread::msleep(1000);
            
            appendStatus("🔓 资源锁自动释放");
        }
        
        // 测试资源冲突
        auto lock1 = ResourceManager::instance().createScopedLock("ConflictTest", "User1");
        if (lock1 && *lock1) {
            appendStatus("👤 用户1获取锁成功");
            
            // 另一个用户尝试获取同一锁
            auto lock2 = ResourceManager::instance().createScopedLock("ConflictTest", "User2", 1000);
            if (!lock2 || !*lock2) {
                appendStatus("⚠️ 用户2获取锁失败（预期行为）");
            }
        }
    }

    void onTestPerformanceMonitoring() {
        appendStatus("📊 启动性能监控测试...");
        
        if (m_audioEngine1) {
            auto* perfManager = m_audioEngine1->getPerformanceManager();
            if (perfManager) {
                perfManager->startMonitoring();
                perfManager->setPerformanceProfile(PerformanceManager::PerformanceProfile::Performance);
                appendStatus("✅ 性能监控已启动");
            }
        }
    }

    void onTestThreadPoolManager() {
        appendStatus("🧵 测试线程池管理器...");
        
        auto& threadManager = ThreadPoolManager::instance();
        
        // 提交一些测试任务
        for (int i = 0; i < 5; ++i) {
            auto* task = new TestTask(QString("Task_%1").arg(i));
            threadManager.submitGeneralTask(task);
        }
        
        appendStatus("📤 已提交5个测试任务到线程池");
        
        // 显示线程池统计
        auto stats = threadManager.getGlobalStatistics();
        appendStatus(QString("📈 当前活跃线程: %1, 待处理任务: %2")
                    .arg(stats.totalActiveThreads)
                    .arg(stats.totalPendingTasks));
    }

    void onTestObserverPattern() {
        appendStatus("👁️ 测试观察者模式...");
        
        if (m_audioEngine1 && m_playInterface1) {
            // 创建测试观察者
            auto testObserver = std::make_shared<TestObserver>("TestObserver");
            
            // 注册观察者
            m_audioEngine1->addObserver(testObserver);
            
            // 触发状态变化
            m_audioEngine1->setVolume(75);
            m_audioEngine1->setMuted(true);
            m_audioEngine1->setMuted(false);
            
            appendStatus("🔄 已触发状态变化，观察者应该收到通知");
        }
    }

    void onCleanupResources() {
        appendStatus("🧹 清理资源...");
        cleanup();
        appendStatus("✅ 资源清理完成");
    }

    void updateStatus() {
        // 更新性能信息
        updatePerformanceInfo();
        
        // 更新资源信息
        updateResourceInfo();
    }

private:
    void setupUI() {
        setWindowTitle("改进架构演示 - 观察者模式 & 资源管理");
        setMinimumSize(800, 600);
        
        auto* mainLayout = new QVBoxLayout(this);
        
        // 控制按钮
        auto* buttonLayout = new QHBoxLayout();
        
        auto* createEngine1Btn = new QPushButton("创建音频引擎1（性能模式）");
        auto* createEngine2Btn = new QPushButton("创建音频引擎2（省电模式）");
        auto* testResourceBtn = new QPushButton("测试资源锁");
        auto* testPerfBtn = new QPushButton("测试性能监控");
        auto* testThreadBtn = new QPushButton("测试线程池");
        auto* testObserverBtn = new QPushButton("测试观察者模式");
        auto* cleanupBtn = new QPushButton("清理资源");
        
        buttonLayout->addWidget(createEngine1Btn);
        buttonLayout->addWidget(createEngine2Btn);
        buttonLayout->addWidget(testResourceBtn);
        buttonLayout->addWidget(testPerfBtn);
        buttonLayout->addWidget(testThreadBtn);
        buttonLayout->addWidget(testObserverBtn);
        buttonLayout->addWidget(cleanupBtn);
        
        // 状态显示
        auto* statusLayout = new QHBoxLayout();
        
        m_performanceLabel = new QLabel("性能: 等待中...");
        m_resourceLabel = new QLabel("资源: 等待中...");
        
        statusLayout->addWidget(m_performanceLabel);
        statusLayout->addWidget(m_resourceLabel);
        
        // 日志显示
        m_statusTextEdit = new QTextEdit();
        m_statusTextEdit->setMaximumHeight(200);
        m_statusTextEdit->setReadOnly(true);
        
        mainLayout->addLayout(buttonLayout);
        mainLayout->addLayout(statusLayout);
        mainLayout->addWidget(new QLabel("事件日志:"));
        mainLayout->addWidget(m_statusTextEdit);
        
        // 连接按钮信号
        connect(createEngine1Btn, &QPushButton::clicked, this, &ImprovedArchitectureDemo::onCreateEngine1);
        connect(createEngine2Btn, &QPushButton::clicked, this, &ImprovedArchitectureDemo::onCreateEngine2);
        connect(testResourceBtn, &QPushButton::clicked, this, &ImprovedArchitectureDemo::onTestResourceLocking);
        connect(testPerfBtn, &QPushButton::clicked, this, &ImprovedArchitectureDemo::onTestPerformanceMonitoring);
        connect(testThreadBtn, &QPushButton::clicked, this, &ImprovedArchitectureDemo::onTestThreadPoolManager);
        connect(testObserverBtn, &QPushButton::clicked, this, &ImprovedArchitectureDemo::onTestObserverPattern);
        connect(cleanupBtn, &QPushButton::clicked, this, &ImprovedArchitectureDemo::onCleanupResources);
    }

    void initializeComponents() {
        // 初始化资源管理器
        ResourceManager::instance().startResourceMonitoring();
        
        // 初始化线程池管理器
        ThreadPoolManager::instance().enableAdaptiveManagement(true);
        
        appendStatus("🚀 组件初始化完成");
    }

    void connectSignals() {
        // 连接状态更新定时器
        connect(m_statusTimer, &QTimer::timeout, this, &ImprovedArchitectureDemo::updateStatus);
        m_statusTimer->start(1000); // 每秒更新一次
    }

    void startDemo() {
        appendStatus("🎵 改进架构演示启动");
        appendStatus("📝 架构改进点:");
        appendStatus("  • 使用观察者模式替代单例模式");
        appendStatus("  • 实现音频资源独占机制");
        appendStatus("  • 动态解码频率调整");
        appendStatus("  • RAII资源管理");
        appendStatus("  • 线程池优化");
        appendStatus("  • 性能监控系统");
        appendStatus("");
    }

    void createPlayInterface1() {
        // 创建性能优化的播放界面
        ImprovedPlayInterface::InterfaceConfig config;
        config.interfaceName = "PlayInterface1";
        config.enablePerformanceMonitoring = true;
        config.updateInterval = 30; // 高刷新率
        
        m_playInterface1 = PlayInterfaceFactory::createInterface(this, config);
        if (m_playInterface1) {
            m_playInterface1->setAudioEngine(m_audioEngine1);
            m_playInterface1->show();
            appendStatus("🎮 播放界面1创建成功");
        }
    }

    void createPlayInterface2() {
        // 创建简化的播放界面
        ImprovedPlayInterface::InterfaceConfig config;
        config.interfaceName = "PlayInterface2";
        config.enablePerformanceMonitoring = false;
        config.enableVisualization = false;
        config.updateInterval = 100; // 低刷新率
        
        m_playInterface2 = PlayInterfaceFactory::createInterface(this, config);
        if (m_playInterface2) {
            m_playInterface2->setAudioEngine(m_audioEngine2);
            m_playInterface2->show();
            appendStatus("🎮 播放界面2创建成功");
        }
    }

    void updatePerformanceInfo() {
        if (m_audioEngine1) {
            auto* perfManager = m_audioEngine1->getPerformanceManager();
            if (perfManager && perfManager->isMonitoring()) {
                auto stats = perfManager->getPerformanceStats();
                m_performanceLabel->setText(QString("性能: CPU %.1f%% | 内存 %1 MB | 响应 %.1f ms")
                                          .arg(stats.avgCpuUsage)
                                          .arg(stats.avgMemoryUsage / 1024 / 1024)
                                          .arg(stats.avgResponseTime));
            }
        }
    }

    void updateResourceInfo() {
        auto& resourceManager = ResourceManager::instance();
        auto stats = resourceManager.getResourceStats();
        
        m_resourceLabel->setText(QString("资源: 活跃锁 %1 | 内存使用 %2 MB | 命中率 %3%")
                               .arg(stats.activeLocks)
                               .arg(stats.totalMemoryUsage / 1024 / 1024)
                               .arg(stats.memoryPoolHitRate));
    }

    void appendStatus(const QString& message) {
        if (m_statusTextEdit) {
            QString timestamp = QTime::currentTime().toString("hh:mm:ss");
            m_statusTextEdit->append(QString("[%1] %2").arg(timestamp, message));
        }
    }

    void cleanup() {
        // 清理播放界面
        if (m_playInterface1) {
            m_playInterface1->close();
            m_playInterface1.reset();
        }
        
        if (m_playInterface2) {
            m_playInterface2->close();
            m_playInterface2.reset();
        }
        
        // 清理音频引擎
        m_audioEngine1.reset();
        m_audioEngine2.reset();
        
        // 清理线程池
        ThreadPoolManager::instance().shutdownAllPools();
        
        // 停止资源监控
        ResourceManager::instance().stopResourceMonitoring();
    }

    /**
     * @brief 测试任务类
     */
    class TestTask : public CancellableTask {
    public:
        explicit TestTask(const QString& taskId) {
            setTaskId(taskId);
            setTaskType("TestTask");
        }

    protected:
        void execute() override {
            // 模拟一些工作
            for (int i = 0; i < 10 && shouldContinue(); ++i) {
                QThread::msleep(100);
            }
        }
    };

    /**
     * @brief 测试观察者
     */
    class TestObserver : public AudioVolumeObserver {
    public:
        explicit TestObserver(const QString& name) : m_name(name) {}

        void onNotify(const AudioEvents::VolumeChanged& event) override {
            qDebug() << m_name << "收到音量变化通知: 音量=" << event.volume 
                     << "静音=" << event.muted << "平衡=" << event.balance;
        }

        QString getObserverName() const override {
            return m_name;
        }

    private:
        QString m_name;
    };

private:
    // UI组件
    QLabel* m_performanceLabel;
    QLabel* m_resourceLabel;
    QTextEdit* m_statusTextEdit;
    QTimer* m_statusTimer;

    // 音频引擎实例
    std::unique_ptr<ImprovedAudioEngine> m_audioEngine1;
    std::unique_ptr<ImprovedAudioEngine> m_audioEngine2;

    // 播放界面实例
    std::unique_ptr<ImprovedPlayInterface> m_playInterface1;
    std::unique_ptr<ImprovedPlayInterface> m_playInterface2;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // 创建演示窗口
    ImprovedArchitectureDemo demo;
    demo.show();

    return app.exec();
}

#include "improved_architecture_demo.moc"