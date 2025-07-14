#include "appconfig.h"
#include <QStandardPaths>
#include <QDir>
#include <QMutexLocker>
#include <QCoreApplication>
#include <QDebug>

// 静态成员初始化
AppConfig* AppConfig::m_instance = nullptr;
QMutex AppConfig::m_mutex;

// 配置键定义
const QString AppConfig::ConfigKeys::THEME = "appearance/theme";
const QString AppConfig::ConfigKeys::LANGUAGE = "general/language";
const QString AppConfig::ConfigKeys::VOLUME = "audio/volume";
const QString AppConfig::ConfigKeys::PLAY_MODE = "playback/play_mode";
const QString AppConfig::ConfigKeys::AUTO_PLAY = "playback/auto_play";
const QString AppConfig::ConfigKeys::SHOW_SPECTRUM = "visualization/show_spectrum";
const QString AppConfig::ConfigKeys::SHOW_WAVEFORM = "visualization/show_waveform";
const QString AppConfig::ConfigKeys::EQUALIZER_ENABLED = "audio/equalizer_enabled";
const QString AppConfig::ConfigKeys::CROSSFADE_DURATION = "audio/crossfade_duration";
const QString AppConfig::ConfigKeys::CACHE_SIZE = "performance/cache_size";
const QString AppConfig::ConfigKeys::LOG_LEVEL = "debug/log_level";
const QString AppConfig::ConfigKeys::WINDOW_GEOMETRY = "ui/window_geometry";
const QString AppConfig::ConfigKeys::WINDOW_STATE = "ui/window_state";

AppConfig* AppConfig::instance()
{
    if (m_instance == nullptr) {
        QMutexLocker locker(&m_mutex);
        if (m_instance == nullptr) {
            m_instance = new AppConfig();
        }
    }
    return m_instance;
}

AppConfig::AppConfig(QObject* parent)
    : QObject(parent)
    , m_settings(nullptr)
{
    // 设置应用程序信息
    QCoreApplication::setOrganizationName("Qt6音频播放器开发团队");
    QCoreApplication::setOrganizationDomain("musicPlayHandle.qt6.org");
    QCoreApplication::setApplicationName("Qt6音频播放器");
    
    // 初始化设置
    m_settings = new QSettings(this);
    
    // 确保必要的目录存在
    ensureDirectoryExists(cacheDirectory());
    ensureDirectoryExists(logDirectory());
    
    // 初始化默认配置
    initializeDefaults();
    
    // 加载配置
    loadConfig();
}

AppConfig::~AppConfig()
{
    saveConfig();
}

QVariant AppConfig::getValue(const QString& key, const QVariant& defaultValue) const
{
    QMutexLocker locker(&m_configMutex);
    return m_settings->value(key, defaultValue);
}

void AppConfig::setValue(const QString& key, const QVariant& value)
{
    QMutexLocker locker(&m_configMutex);
    
    QVariant oldValue = m_settings->value(key);
    if (oldValue != value) {
        m_settings->setValue(key, value);
        
        // 发出配置变更信号
        emit configChanged(key, value);
        
        // 发出特定的变更信号
        if (key == ConfigKeys::THEME) {
            emit themeChanged(value.toString());
        } else if (key == ConfigKeys::LANGUAGE) {
            emit languageChanged(value.toString());
        }
    }
}

QString AppConfig::currentTheme() const
{
    return getValue(ConfigKeys::THEME, "default").toString();
}

void AppConfig::setTheme(const QString& theme)
{
    setValue(ConfigKeys::THEME, theme);
}

QString AppConfig::currentLanguage() const
{
    return getValue(ConfigKeys::LANGUAGE, "zh_CN").toString();
}

void AppConfig::setLanguage(const QString& language)
{
    setValue(ConfigKeys::LANGUAGE, language);
}

QString AppConfig::databasePath() const
{
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return QDir(dataDir).filePath("musicPlayer.db");
}

QString AppConfig::cacheDirectory() const
{
    return QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
}

QString AppConfig::logDirectory() const
{
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return QDir(dataDir).filePath("logs");
}

void AppConfig::saveConfig()
{
    QMutexLocker locker(&m_configMutex);
    m_settings->sync();
}

void AppConfig::loadConfig()
{
    QMutexLocker locker(&m_configMutex);
    
    // 检查是否首次运行
    if (!m_settings->contains(ConfigKeys::THEME)) {
        initializeDefaults();
    }
}

void AppConfig::resetToDefaults()
{
    QMutexLocker locker(&m_configMutex);
    
    m_settings->clear();
    initializeDefaults();
    
    // 发出配置重置信号
    emit configChanged("", QVariant());
}

void AppConfig::initializeDefaults()
{
    // 外观设置
    if (!m_settings->contains(ConfigKeys::THEME)) {
        m_settings->setValue(ConfigKeys::THEME, "default");
    }
    
    // 语言设置
    if (!m_settings->contains(ConfigKeys::LANGUAGE)) {
        m_settings->setValue(ConfigKeys::LANGUAGE, "zh_CN");
    }
    
    // 音频设置
    if (!m_settings->contains(ConfigKeys::VOLUME)) {
        m_settings->setValue(ConfigKeys::VOLUME, 50);
    }
    
    if (!m_settings->contains(ConfigKeys::PLAY_MODE)) {
        m_settings->setValue(ConfigKeys::PLAY_MODE, 0); // 顺序播放
    }
    
    if (!m_settings->contains(ConfigKeys::AUTO_PLAY)) {
        m_settings->setValue(ConfigKeys::AUTO_PLAY, true);
    }
    
    if (!m_settings->contains(ConfigKeys::EQUALIZER_ENABLED)) {
        m_settings->setValue(ConfigKeys::EQUALIZER_ENABLED, false);
    }
    
    if (!m_settings->contains(ConfigKeys::CROSSFADE_DURATION)) {
        m_settings->setValue(ConfigKeys::CROSSFADE_DURATION, 0);
    }
    
    // 可视化设置
    if (!m_settings->contains(ConfigKeys::SHOW_SPECTRUM)) {
        m_settings->setValue(ConfigKeys::SHOW_SPECTRUM, true);
    }
    
    if (!m_settings->contains(ConfigKeys::SHOW_WAVEFORM)) {
        m_settings->setValue(ConfigKeys::SHOW_WAVEFORM, true);
    }
    
    // 性能设置
    if (!m_settings->contains(ConfigKeys::CACHE_SIZE)) {
        m_settings->setValue(ConfigKeys::CACHE_SIZE, 100); // 100MB
    }
    
    // 调试设置
    if (!m_settings->contains(ConfigKeys::LOG_LEVEL)) {
        m_settings->setValue(ConfigKeys::LOG_LEVEL, "INFO");
    }
}

void AppConfig::ensureDirectoryExists(const QString& path)
{
    QDir dir;
    if (!dir.exists(path)) {
        if (!dir.mkpath(path)) {
            qWarning() << "无法创建目录:" << path;
        }
    }
} 