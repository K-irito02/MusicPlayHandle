#ifndef APPCONFIG_H
#define APPCONFIG_H

#include <QObject>
#include <QSettings>
#include <QMutex>
#include <QVariant>
#include <QString>

/**
 * @brief 应用程序配置管理器单例类
 * 
 * 负责管理应用程序的所有配置信息，包括用户设置、主题、语言等
 * 使用线程安全的单例模式，支持配置变更通知
 */
class AppConfig : public QObject
{
    Q_OBJECT
    
public:
    /**
     * @brief 获取单例实例
     * @return AppConfig实例指针
     */
    static AppConfig* instance();
    
    /**
     * @brief 获取配置值
     * @param key 配置键
     * @param defaultValue 默认值
     * @return 配置值
     */
    QVariant getValue(const QString& key, const QVariant& defaultValue = QVariant()) const;
    
    /**
     * @brief 设置配置值
     * @param key 配置键
     * @param value 配置值
     */
    void setValue(const QString& key, const QVariant& value);
    
    /**
     * @brief 获取当前主题
     * @return 主题名称
     */
    QString currentTheme() const;
    
    /**
     * @brief 设置主题
     * @param theme 主题名称
     */
    void setTheme(const QString& theme);
    
    /**
     * @brief 获取当前语言
     * @return 语言代码
     */
    QString currentLanguage() const;
    
    /**
     * @brief 设置语言
     * @param language 语言代码
     */
    void setLanguage(const QString& language);
    
    /**
     * @brief 获取数据库路径
     * @return 数据库文件路径
     */
    QString databasePath() const;
    
    /**
     * @brief 获取缓存目录
     * @return 缓存目录路径
     */
    QString cacheDirectory() const;
    
    /**
     * @brief 获取日志目录
     * @return 日志目录路径
     */
    QString logDirectory() const;
    
    /**
     * @brief 保存配置到文件
     */
    void saveConfig();
    
    /**
     * @brief 从文件加载配置
     */
    void loadConfig();
    
    /**
     * @brief 重置为默认配置
     */
    void resetToDefaults();
    
signals:
    /**
     * @brief 配置变更信号
     * @param key 配置键
     * @param value 新值
     */
    void configChanged(const QString& key, const QVariant& value);
    
    /**
     * @brief 主题变更信号
     * @param theme 新主题
     */
    void themeChanged(const QString& theme);
    
    /**
     * @brief 语言变更信号
     * @param language 新语言
     */
    void languageChanged(const QString& language);
    
private:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit AppConfig(QObject* parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~AppConfig();
    
    // 禁用拷贝构造和赋值
    AppConfig(const AppConfig&) = delete;
    AppConfig& operator=(const AppConfig&) = delete;
    
    /**
     * @brief 初始化默认配置
     */
    void initializeDefaults();
    
    /**
     * @brief 确保目录存在
     * @param path 目录路径
     */
    void ensureDirectoryExists(const QString& path);
    
    static AppConfig* m_instance;    ///< 单例实例
    static QMutex m_mutex;           ///< 线程安全互斥锁
    
    QSettings* m_settings;           ///< 配置存储
    mutable QMutex m_configMutex;    ///< 配置访问互斥锁
    
    // 配置常量
    struct ConfigKeys {
        static const QString THEME;
        static const QString LANGUAGE;
        static const QString VOLUME;
        static const QString PLAY_MODE;
        static const QString AUTO_PLAY;
        static const QString SHOW_SPECTRUM;
        static const QString SHOW_WAVEFORM;
        static const QString EQUALIZER_ENABLED;
        static const QString CROSSFADE_DURATION;
        static const QString CACHE_SIZE;
        static const QString LOG_LEVEL;
        static const QString WINDOW_GEOMETRY;
        static const QString WINDOW_STATE;
    };
};

#endif // APPCONFIG_H 