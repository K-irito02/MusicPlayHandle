/**
 * @file optimization_usage_example.cpp
 * @brief 代码优化使用示例
 * @details 展示如何在实际项目中应用各种优化技术
 * @author QT6/C++11 专家
 * @date 2024
 */

#include "../src/core/servicecontainer.h"
#include "../src/core/itagmanager.h"
#include "../src/core/idatabasemanager.h"
#include "../src/core/result.h"
#include "../src/core/databasetransaction.h"
#include "../src/core/constants.h"
#include "../src/core/cache.h"
#include "../src/core/lazyloader.h"
#include "../src/core/tagconfiguration.h"
#include "../src/core/structuredlogger.h"
#include "../src/core/tagstrings.h"
#include "../src/core/objectpool.h"
#include "../src/ui/widgets/taglistitemfactory.h"
#include "../src/models/tag.h"

#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QMessageBox>
#include <QTimer>
#include <QElapsedTimer>
#include <memory>

/**
 * @brief 优化示例主窗口
 * @details 展示各种优化技术的实际应用
 */
class OptimizationExampleWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit OptimizationExampleWindow(QWidget* parent = nullptr)
        : QMainWindow(parent)
        , m_serviceContainer(ServiceContainer::instance())
        , m_tagManager(nullptr)
        , m_databaseManager(nullptr)
        , m_tagCache(std::make_unique<Cache<int, std::shared_ptr<Tag>>>(100))
        , m_lazyTagLoader(std::make_unique<LazyTagList>())
    {
        setupUI();
        setupServices();
        setupLogging();
        setupInternationalization();
        
        // 演示各种优化技术
        demonstrateOptimizations();
    }
    
private slots:
    /**
     * @brief 演示依赖注入
     */
    void demonstrateDependencyInjection()
    {
        LOG_UI_ACTION("demonstrateDependencyInjection", "OptimizationExampleWindow");
        
        // 1. 从服务容器获取服务
        m_tagManager = m_serviceContainer->getService<ITagManager>();
        m_databaseManager = m_serviceContainer->getService<IDatabaseManager>();
        
        if (!m_tagManager || !m_databaseManager) {
            QMessageBox::warning(this, TagStrings::tr("错误"), 
                               TagStrings::tr("服务初始化失败"));
            return;
        }
        
        // 2. 使用服务
        auto allTags = m_tagManager->getAllTags();
        
        QString message = TagStrings::tr("成功获取 %1 个标签").arg(allTags.size());
        QMessageBox::information(this, TagStrings::tr("依赖注入示例"), message);
        
        LOG_TAG_OPERATION("getAllTags", QString::number(allTags.size()));
    }
    
    /**
     * @brief 演示Result类型模式
     */
    void demonstrateResultPattern()
    {
        LOG_UI_ACTION("demonstrateResultPattern", "OptimizationExampleWindow");
        
        // 1. 创建标签的Result示例
        auto createResult = createTagWithResult("示例标签");
        
        if (createResult.isSuccess()) {
            auto tag = createResult.value();
            QString message = TagStrings::tr("标签创建成功：%1").arg(tag->getName());
            QMessageBox::information(this, TagStrings::tr("Result模式示例"), message);
            
            LOG_TAG_OPERATION("createTag", tag->getName());
        } else {
            QString message = TagStrings::tr("标签创建失败：%1").arg(createResult.errorMessage());
            QMessageBox::warning(this, TagStrings::tr("Result模式示例"), message);
            
            LOG_ERROR("tag", createResult.errorMessage(), "TAG_CREATE_FAILED");
        }
    }
    
    /**
     * @brief 演示数据库事务
     */
    void demonstrateDatabaseTransaction()
    {
        LOG_UI_ACTION("demonstrateDatabaseTransaction", "OptimizationExampleWindow");
        
        if (!m_databaseManager) {
            QMessageBox::warning(this, TagStrings::tr("错误"), 
                               TagStrings::tr("数据库管理器未初始化"));
            return;
        }
        
        QElapsedTimer timer;
        timer.start();
        
        // 使用RAII事务管理
        {
            DatabaseTransaction transaction(m_databaseManager->getDatabase());
            
            try {
                // 执行多个数据库操作
                auto tag1 = std::make_shared<Tag>();
                tag1->setName("事务标签1");
                
                auto tag2 = std::make_shared<Tag>();
                tag2->setName("事务标签2");
                
                if (m_tagManager->createTag(tag1) && m_tagManager->createTag(tag2)) {
                    transaction.commit();
                    
                    QString message = TagStrings::tr("事务提交成功，创建了2个标签");
                    QMessageBox::information(this, TagStrings::tr("事务示例"), message);
                    
                    LOG_DATABASE_QUERY("CREATE_TAGS_TRANSACTION", timer.elapsed());
                } else {
                    // 事务会自动回滚
                    throw std::runtime_error("标签创建失败");
                }
            } catch (const std::exception& e) {
                // 事务自动回滚
                QString message = TagStrings::tr("事务回滚：%1").arg(e.what());
                QMessageBox::warning(this, TagStrings::tr("事务示例"), message);
                
                LOG_ERROR("database", e.what(), "TRANSACTION_FAILED");
            }
        } // 事务对象析构，自动处理提交或回滚
    }
    
    /**
     * @brief 演示缓存策略
     */
    void demonstrateCaching()
    {
        LOG_UI_ACTION("demonstrateCaching", "OptimizationExampleWindow");
        
        if (!m_tagManager) {
            return;
        }
        
        QElapsedTimer timer;
        
        // 第一次访问（从数据库加载）
        timer.start();
        auto tag = getTagWithCache(1);
        qint64 firstAccess = timer.elapsed();
        
        // 第二次访问（从缓存获取）
        timer.restart();
        tag = getTagWithCache(1);
        qint64 secondAccess = timer.elapsed();
        
        // 显示性能对比
        QString message = TagStrings::tr(
            "缓存性能对比：\n"
            "首次访问：%1ms\n"
            "缓存访问：%2ms\n"
            "性能提升：%3x"
        ).arg(firstAccess)
         .arg(secondAccess)
         .arg(firstAccess > 0 ? (double)firstAccess / secondAccess : 1.0, 0, 'f', 1);
        
        QMessageBox::information(this, TagStrings::tr("缓存示例"), message);
        
        // 显示缓存统计
        auto stats = m_tagCache->getStatistics();
        QString statsMessage = TagStrings::tr(
            "缓存统计：\n"
            "命中率：%1%\n"
            "总访问：%2\n"
            "缓存大小：%3"
        ).arg(stats.hitRate)
         .arg(stats.totalAccess)
         .arg(stats.currentSize);
        
        QMessageBox::information(this, TagStrings::tr("缓存统计"), statsMessage);
        
        LOG_PERFORMANCE("cache_demo", firstAccess + secondAccess, 
                       QJsonObject{{"firstAccess", firstAccess}, 
                                  {"secondAccess", secondAccess}});
    }
    
    /**
     * @brief 演示延迟加载
     */
    void demonstrateLazyLoading()
    {
        LOG_UI_ACTION("demonstrateLazyLoading", "OptimizationExampleWindow");
        
        QElapsedTimer timer;
        timer.start();
        
        // 延迟加载标签列表
        const auto& tags = m_lazyTagLoader->getData();
        qint64 loadTime = timer.elapsed();
        
        // 第二次访问（已加载）
        timer.restart();
        const auto& cachedTags = m_lazyTagLoader->getData();
        qint64 cachedTime = timer.elapsed();
        
        QString message = TagStrings::tr(
            "延迟加载示例：\n"
            "加载了 %1 个标签\n"
            "首次加载：%2ms\n"
            "缓存访问：%3ms"
        ).arg(tags.size())
         .arg(loadTime)
         .arg(cachedTime);
        
        QMessageBox::information(this, TagStrings::tr("延迟加载示例"), message);
        
        LOG_PERFORMANCE("lazy_loading_demo", loadTime, 
                       QJsonObject{{"tagCount", tags.size()}, 
                                  {"cachedTime", cachedTime}});
    }
    
    /**
     * @brief 演示对象池
     */
    void demonstrateObjectPool()
    {
        LOG_UI_ACTION("demonstrateObjectPool", "OptimizationExampleWindow");
        
        auto pool = getTagListItemPool();
        if (!pool) {
            QMessageBox::warning(this, TagStrings::tr("错误"), 
                               TagStrings::tr("对象池未初始化"));
            return;
        }
        
        QElapsedTimer timer;
        timer.start();
        
        // 获取和释放多个对象
        QList<std::unique_ptr<TagListItem>> items;
        for (int i = 0; i < 10; ++i) {
            items.append(pool->acquire());
        }
        
        // 释放对象回池
        for (auto& item : items) {
            pool->release(std::move(item));
        }
        items.clear();
        
        qint64 poolTime = timer.elapsed();
        
        // 直接创建对象对比
        timer.restart();
        QList<std::unique_ptr<TagListItem>> directItems;
        for (int i = 0; i < 10; ++i) {
            directItems.append(std::make_unique<TagListItem>());
        }
        directItems.clear();
        
        qint64 directTime = timer.elapsed();
        
        // 显示性能对比
        auto stats = pool->getStatistics();
        QString message = TagStrings::tr(
            "对象池性能对比：\n"
            "对象池方式：%1ms\n"
            "直接创建：%2ms\n"
            "命中率：%3%\n"
            "池大小：%4"
        ).arg(poolTime)
         .arg(directTime)
         .arg(stats.hitRate)
         .arg(stats.currentPoolSize);
        
        QMessageBox::information(this, TagStrings::tr("对象池示例"), message);
        
        LOG_PERFORMANCE("object_pool_demo", poolTime, 
                       QJsonObject{{"directTime", directTime}, 
                                  {"hitRate", stats.hitRate}});
    }
    
    /**
     * @brief 演示工厂模式
     */
    void demonstrateFactoryPattern()
    {
        LOG_UI_ACTION("demonstrateFactoryPattern", "OptimizationExampleWindow");
        
        // 使用工厂创建不同类型的标签项
        auto systemTag = TagListItemFactory::createSystemTag(Constants::SystemTags::MY_SONGS);
        auto userTag = TagListItemFactory::createUserTag("用户自定义标签");
        auto readOnlyTag = TagListItemFactory::createReadOnlyTag("只读标签");
        
        // 显示创建的标签信息
        QString message = TagStrings::tr(
            "工厂模式示例：\n"
            "系统标签：%1 (可编辑:%2, 可删除:%3)\n"
            "用户标签：%4 (可编辑:%5, 可删除:%6)\n"
            "只读标签：%7 (可编辑:%8, 可删除:%9)"
        ).arg(systemTag->getTagName())
         .arg(systemTag->isEditable() ? TagStrings::tr("是") : TagStrings::tr("否"))
         .arg(systemTag->isDeletable() ? TagStrings::tr("是") : TagStrings::tr("否"))
         .arg(userTag->getTagName())
         .arg(userTag->isEditable() ? TagStrings::tr("是") : TagStrings::tr("否"))
         .arg(userTag->isDeletable() ? TagStrings::tr("是") : TagStrings::tr("否"))
         .arg(readOnlyTag->getTagName())
         .arg(readOnlyTag->isEditable() ? TagStrings::tr("是") : TagStrings::tr("否"))
         .arg(readOnlyTag->isDeletable() ? TagStrings::tr("是") : TagStrings::tr("否"));
        
        QMessageBox::information(this, TagStrings::tr("工厂模式示例"), message);
        
        LOG_TAG_OPERATION("factory_demo", "created_3_different_tag_types");
    }
    
    /**
     * @brief 演示配置管理
     */
    void demonstrateConfigurationManagement()
    {
        LOG_UI_ACTION("demonstrateConfigurationManagement", "OptimizationExampleWindow");
        
        auto config = TagConfiguration::instance();
        
        // 获取配置信息
        auto systemTags = config->getSystemTags();
        auto defaultColor = config->getDefaultTagColor();
        auto maxNameLength = config->getMaxTagNameLength();
        bool showIcons = config->getShowTagIcons();
        
        QString message = TagStrings::tr(
            "配置管理示例：\n"
            "系统标签数量：%1\n"
            "默认标签颜色：%2\n"
            "最大名称长度：%3\n"
            "显示图标：%4"
        ).arg(systemTags.size())
         .arg(defaultColor)
         .arg(maxNameLength)
         .arg(showIcons ? TagStrings::tr("是") : TagStrings::tr("否"));
        
        QMessageBox::information(this, TagStrings::tr("配置管理示例"), message);
        
        LOG_TAG_OPERATION("config_demo", "displayed_configuration_info");
    }
    
private:
    /**
     * @brief 设置用户界面
     */
    void setupUI()
    {
        auto centralWidget = new QWidget(this);
        setCentralWidget(centralWidget);
        
        auto layout = new QVBoxLayout(centralWidget);
        
        // 创建演示按钮
        auto diButton = new QPushButton(TagStrings::tr("演示依赖注入"), this);
        auto resultButton = new QPushButton(TagStrings::tr("演示Result模式"), this);
        auto transactionButton = new QPushButton(TagStrings::tr("演示数据库事务"), this);
        auto cacheButton = new QPushButton(TagStrings::tr("演示缓存策略"), this);
        auto lazyButton = new QPushButton(TagStrings::tr("演示延迟加载"), this);
        auto poolButton = new QPushButton(TagStrings::tr("演示对象池"), this);
        auto factoryButton = new QPushButton(TagStrings::tr("演示工厂模式"), this);
        auto configButton = new QPushButton(TagStrings::tr("演示配置管理"), this);
        
        layout->addWidget(diButton);
        layout->addWidget(resultButton);
        layout->addWidget(transactionButton);
        layout->addWidget(cacheButton);
        layout->addWidget(lazyButton);
        layout->addWidget(poolButton);
        layout->addWidget(factoryButton);
        layout->addWidget(configButton);
        
        // 连接信号槽
        connect(diButton, &QPushButton::clicked, this, &OptimizationExampleWindow::demonstrateDependencyInjection);
        connect(resultButton, &QPushButton::clicked, this, &OptimizationExampleWindow::demonstrateResultPattern);
        connect(transactionButton, &QPushButton::clicked, this, &OptimizationExampleWindow::demonstrateDatabaseTransaction);
        connect(cacheButton, &QPushButton::clicked, this, &OptimizationExampleWindow::demonstrateCaching);
        connect(lazyButton, &QPushButton::clicked, this, &OptimizationExampleWindow::demonstrateLazyLoading);
        connect(poolButton, &QPushButton::clicked, this, &OptimizationExampleWindow::demonstrateObjectPool);
        connect(factoryButton, &QPushButton::clicked, this, &OptimizationExampleWindow::demonstrateFactoryPattern);
        connect(configButton, &QPushButton::clicked, this, &OptimizationExampleWindow::demonstrateConfigurationManagement);
        
        setWindowTitle(TagStrings::tr("代码优化示例"));
        resize(400, 300);
    }
    
    /**
     * @brief 设置服务
     */
    void setupServices()
    {
        // 这里应该注册实际的服务实现
        // 为了示例，我们假设服务已经注册
    }
    
    /**
     * @brief 设置日志系统
     */
    void setupLogging()
    {
        auto logger = StructuredLogger::instance();
        logger->setConsoleOutput(true);
        logger->setFileOutput(true);
        logger->setJsonFormat(false);
        
        LOG_TAG_OPERATION("application_start", "OptimizationExampleWindow");
    }
    
    /**
     * @brief 设置国际化
     */
    void setupInternationalization()
    {
        auto strings = TagStrings::instance();
        strings->initialize();
    }
    
    /**
     * @brief 演示所有优化技术
     */
    void demonstrateOptimizations()
    {
        // 可以在这里添加自动演示逻辑
        LOG_TAG_OPERATION("optimization_demo_ready", "all_systems_initialized");
    }
    
    /**
     * @brief 使用Result模式创建标签
     * @param name 标签名称
     * @return 创建结果
     */
    Result<std::shared_ptr<Tag>> createTagWithResult(const QString& name)
    {
        if (name.isEmpty()) {
            return Result<std::shared_ptr<Tag>>::error(TagStrings::tagNameCannotBeEmpty());
        }
        
        if (name.length() > Constants::Tag::MAX_NAME_LENGTH) {
            return Result<std::shared_ptr<Tag>>::error(
                TagStrings::tagNameTooLong(Constants::Tag::MAX_NAME_LENGTH));
        }
        
        auto tag = std::make_shared<Tag>();
        tag->setName(name);
        tag->setDescription(TagStrings::tr("通过Result模式创建的标签"));
        
        if (m_tagManager && m_tagManager->createTag(tag)) {
            return Result<std::shared_ptr<Tag>>::success(tag);
        } else {
            return Result<std::shared_ptr<Tag>>::error(TagStrings::tagCreationFailed());
        }
    }
    
    /**
     * @brief 使用缓存获取标签
     * @param tagId 标签ID
     * @return 标签对象
     */
    std::shared_ptr<Tag> getTagWithCache(int tagId)
    {
        // 先从缓存查找
        auto cached = m_tagCache->get(tagId);
        if (cached.has_value()) {
            return cached.value();
        }
        
        // 从数据库加载
        if (m_tagManager) {
            auto tag = m_tagManager->getTag(tagId);
            if (tag) {
                // 放入缓存
                m_tagCache->put(tagId, tag);
            }
            return tag;
        }
        
        return nullptr;
    }
    
private:
    ServiceContainer* m_serviceContainer;
    std::shared_ptr<ITagManager> m_tagManager;
    std::shared_ptr<IDatabaseManager> m_databaseManager;
    
    std::unique_ptr<Cache<int, std::shared_ptr<Tag>>> m_tagCache;
    std::unique_ptr<LazyTagList> m_lazyTagLoader;
};

/**
 * @brief 主函数
 * @param argc 参数数量
 * @param argv 参数数组
 * @return 程序退出码
 */
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // 设置应用程序信息
    app.setApplicationName(Constants::Application::NAME);
    app.setApplicationVersion(Constants::Application::VERSION);
    app.setOrganizationName(Constants::Application::ORGANIZATION);
    
    // 创建并显示示例窗口
    OptimizationExampleWindow window;
    window.show();
    
    int result = app.exec();
    
    // 清理资源
    ServiceContainer::cleanup();
    StructuredLogger::cleanup();
    TagStrings::cleanup();
    ObjectPoolManager::cleanup();
    
    return result;
}

#include "optimization_usage_example.moc"