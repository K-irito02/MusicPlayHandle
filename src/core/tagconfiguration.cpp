#include "tagconfiguration.h"
#include <QStandardPaths>
#include <QDir>
#include <QDebug>

// 静态成员初始化
TagConfiguration* TagConfiguration::s_instance = nullptr;

// 配置键常量
const QString TagConfiguration::KEY_SYSTEM_TAGS = QStringLiteral("tags/systemTags");
const QString TagConfiguration::KEY_TAG_COLORS = QStringLiteral("tags/colors");
const QString TagConfiguration::KEY_TAG_ICONS = QStringLiteral("tags/icons");
const QString TagConfiguration::KEY_DEFAULT_TAG_COLOR = QStringLiteral("tags/defaultColor");
const QString TagConfiguration::KEY_DEFAULT_TAG_ICON = QStringLiteral("tags/defaultIcon");
const QString TagConfiguration::KEY_SHOW_SYSTEM_TAGS = QStringLiteral("tags/showSystemTags");
const QString TagConfiguration::KEY_ALLOW_EDIT_SYSTEM_TAGS = QStringLiteral("tags/allowEditSystemTags");
const QString TagConfiguration::KEY_TAG_SORT_ORDER = QStringLiteral("tags/sortOrder");
const QString TagConfiguration::KEY_AUTO_CREATE_TAGS = QStringLiteral("tags/autoCreate");
const QString TagConfiguration::KEY_MAX_TAG_COUNT = QStringLiteral("tags/maxCount");

TagConfiguration::TagConfiguration(QObject* parent)
    : QObject(parent), m_settings(nullptr)
{
    initializeDefaults();
}

TagConfiguration::~TagConfiguration()
{
    if (m_settings) {
        saveToSettings();
        delete m_settings;
    }
}

TagConfiguration* TagConfiguration::instance()
{
    if (!s_instance) {
        s_instance = new TagConfiguration();
    }
    return s_instance;
}

void TagConfiguration::cleanup()
{
    if (s_instance) {
        delete s_instance;
        s_instance = nullptr;
    }
}

void TagConfiguration::loadFromSettings(const QString& settingsPath)
{
    QMutexLocker locker(&m_mutex);
    
    if (m_settings) {
        delete m_settings;
    }
    
    QString configPath = settingsPath;
    if (configPath.isEmpty()) {
        QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
        QDir().mkpath(configDir);
        configPath = configDir + "/" + Constants::Paths::CONFIG_FILE;
    }
    
    m_settings = new QSettings(configPath, QSettings::IniFormat);
    
    // 加载系统标签
    m_systemTags = m_settings->value(KEY_SYSTEM_TAGS, Constants::SystemTags::getAll()).toStringList();
    
    // 加载标签颜色
    m_settings->beginGroup("tagColors");
    const QStringList colorKeys = m_settings->childKeys();
    for (const QString& key : colorKeys) {
        QColor color = m_settings->value(key).value<QColor>();
        if (color.isValid()) {
            m_tagColors[key] = color;
        }
    }
    m_settings->endGroup();
    
    // 加载标签图标
    m_settings->beginGroup("tagIcons");
    const QStringList iconKeys = m_settings->childKeys();
    for (const QString& key : iconKeys) {
        m_tagIcons[key] = m_settings->value(key).toString();
    }
    m_settings->endGroup();
    
    // 加载其他配置
    m_defaultTagColor = m_settings->value(KEY_DEFAULT_TAG_COLOR, Constants::UI::PRIMARY_COLOR).value<QColor>();
    m_defaultTagIcon = m_settings->value(KEY_DEFAULT_TAG_ICON, ":/images/editLabel.png").toString();
    m_showSystemTags = m_settings->value(KEY_SHOW_SYSTEM_TAGS, true).toBool();
    m_allowEditSystemTags = m_settings->value(KEY_ALLOW_EDIT_SYSTEM_TAGS, false).toBool();
    m_tagSortOrder = m_settings->value(KEY_TAG_SORT_ORDER, 0).toInt();
    m_autoCreateTags = m_settings->value(KEY_AUTO_CREATE_TAGS, true).toBool();
    m_maxTagCount = m_settings->value(KEY_MAX_TAG_COUNT, -1).toInt();
    
    qDebug() << "TagConfiguration: Loaded configuration from" << configPath;
}

void TagConfiguration::saveToSettings()
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_settings) {
        return;
    }
    
    // 保存系统标签
    m_settings->setValue(KEY_SYSTEM_TAGS, m_systemTags);
    
    // 保存标签颜色
    m_settings->beginGroup("tagColors");
    m_settings->remove(""); // 清空组
    for (auto it = m_tagColors.begin(); it != m_tagColors.end(); ++it) {
        m_settings->setValue(it.key(), it.value());
    }
    m_settings->endGroup();
    
    // 保存标签图标
    m_settings->beginGroup("tagIcons");
    m_settings->remove(""); // 清空组
    for (auto it = m_tagIcons.begin(); it != m_tagIcons.end(); ++it) {
        m_settings->setValue(it.key(), it.value());
    }
    m_settings->endGroup();
    
    // 保存其他配置
    m_settings->setValue(KEY_DEFAULT_TAG_COLOR, m_defaultTagColor);
    m_settings->setValue(KEY_DEFAULT_TAG_ICON, m_defaultTagIcon);
    m_settings->setValue(KEY_SHOW_SYSTEM_TAGS, m_showSystemTags);
    m_settings->setValue(KEY_ALLOW_EDIT_SYSTEM_TAGS, m_allowEditSystemTags);
    m_settings->setValue(KEY_TAG_SORT_ORDER, m_tagSortOrder);
    m_settings->setValue(KEY_AUTO_CREATE_TAGS, m_autoCreateTags);
    m_settings->setValue(KEY_MAX_TAG_COUNT, m_maxTagCount);
    
    m_settings->sync();
    
    qDebug() << "TagConfiguration: Saved configuration";
}

QStringList TagConfiguration::getSystemTags() const
{
    QMutexLocker locker(&m_mutex);
    return m_systemTags;
}

void TagConfiguration::setSystemTags(const QStringList& tags)
{
    QMutexLocker locker(&m_mutex);
    if (m_systemTags != tags) {
        m_systemTags = tags;
        emitConfigurationChanged(KEY_SYSTEM_TAGS);
        emit systemTagsChanged();
    }
}

bool TagConfiguration::isSystemTag(const QString& name) const
{
    QMutexLocker locker(&m_mutex);
    return m_systemTags.contains(name);
}

bool TagConfiguration::addSystemTag(const QString& name)
{
    QMutexLocker locker(&m_mutex);
    if (!m_systemTags.contains(name)) {
        m_systemTags.append(name);
        emitConfigurationChanged(KEY_SYSTEM_TAGS);
        emit systemTagsChanged();
        return true;
    }
    return false;
}

bool TagConfiguration::removeSystemTag(const QString& name)
{
    QMutexLocker locker(&m_mutex);
    if (m_systemTags.removeOne(name)) {
        emitConfigurationChanged(KEY_SYSTEM_TAGS);
        emit systemTagsChanged();
        return true;
    }
    return false;
}

QColor TagConfiguration::getTagColor(const QString& tagName) const
{
    QMutexLocker locker(&m_mutex);
    return m_tagColors.value(tagName, m_defaultTagColor);
}

void TagConfiguration::setTagColor(const QString& tagName, const QColor& color)
{
    QMutexLocker locker(&m_mutex);
    if (m_tagColors.value(tagName) != color) {
        m_tagColors[tagName] = color;
        emitConfigurationChanged(KEY_TAG_COLORS);
        emit tagColorChanged(tagName, color);
    }
}

QColor TagConfiguration::getDefaultTagColor() const
{
    QMutexLocker locker(&m_mutex);
    return m_defaultTagColor;
}

void TagConfiguration::setDefaultTagColor(const QColor& color)
{
    QMutexLocker locker(&m_mutex);
    if (m_defaultTagColor != color) {
        m_defaultTagColor = color;
        emitConfigurationChanged(KEY_DEFAULT_TAG_COLOR);
    }
}

QString TagConfiguration::getTagIcon(const QString& tagName) const
{
    QMutexLocker locker(&m_mutex);
    return m_tagIcons.value(tagName, m_defaultTagIcon);
}

void TagConfiguration::setTagIcon(const QString& tagName, const QString& iconPath)
{
    QMutexLocker locker(&m_mutex);
    if (m_tagIcons.value(tagName) != iconPath) {
        m_tagIcons[tagName] = iconPath;
        emitConfigurationChanged(KEY_TAG_ICONS);
    }
}

QString TagConfiguration::getDefaultTagIcon() const
{
    QMutexLocker locker(&m_mutex);
    return m_defaultTagIcon;
}

void TagConfiguration::setDefaultTagIcon(const QString& iconPath)
{
    QMutexLocker locker(&m_mutex);
    if (m_defaultTagIcon != iconPath) {
        m_defaultTagIcon = iconPath;
        emitConfigurationChanged(KEY_DEFAULT_TAG_ICON);
    }
}

bool TagConfiguration::getShowSystemTags() const
{
    QMutexLocker locker(&m_mutex);
    return m_showSystemTags;
}

void TagConfiguration::setShowSystemTags(bool show)
{
    QMutexLocker locker(&m_mutex);
    if (m_showSystemTags != show) {
        m_showSystemTags = show;
        emitConfigurationChanged(KEY_SHOW_SYSTEM_TAGS);
    }
}

bool TagConfiguration::getAllowEditSystemTags() const
{
    QMutexLocker locker(&m_mutex);
    return m_allowEditSystemTags;
}

void TagConfiguration::setAllowEditSystemTags(bool allow)
{
    QMutexLocker locker(&m_mutex);
    if (m_allowEditSystemTags != allow) {
        m_allowEditSystemTags = allow;
        emitConfigurationChanged(KEY_ALLOW_EDIT_SYSTEM_TAGS);
    }
}

int TagConfiguration::getTagSortOrder() const
{
    QMutexLocker locker(&m_mutex);
    return m_tagSortOrder;
}

void TagConfiguration::setTagSortOrder(int order)
{
    QMutexLocker locker(&m_mutex);
    if (m_tagSortOrder != order) {
        m_tagSortOrder = order;
        emitConfigurationChanged(KEY_TAG_SORT_ORDER);
    }
}

bool TagConfiguration::getAutoCreateTags() const
{
    QMutexLocker locker(&m_mutex);
    return m_autoCreateTags;
}

void TagConfiguration::setAutoCreateTags(bool autoCreate)
{
    QMutexLocker locker(&m_mutex);
    if (m_autoCreateTags != autoCreate) {
        m_autoCreateTags = autoCreate;
        emitConfigurationChanged(KEY_AUTO_CREATE_TAGS);
    }
}

int TagConfiguration::getMaxTagCount() const
{
    QMutexLocker locker(&m_mutex);
    return m_maxTagCount;
}

void TagConfiguration::setMaxTagCount(int maxCount)
{
    QMutexLocker locker(&m_mutex);
    if (m_maxTagCount != maxCount) {
        m_maxTagCount = maxCount;
        emitConfigurationChanged(KEY_MAX_TAG_COUNT);
    }
}

void TagConfiguration::resetToDefaults()
{
    QMutexLocker locker(&m_mutex);
    
    initializeDefaults();
    
    if (m_settings) {
        m_settings->clear();
        saveToSettings();
    }
    
    emit configurationChanged("reset");
    emit systemTagsChanged();
    
    qDebug() << "TagConfiguration: Reset to defaults";
}

bool TagConfiguration::validateConfiguration() const
{
    QMutexLocker locker(&m_mutex);
    
    // 检查系统标签是否为空
    if (m_systemTags.isEmpty()) {
        qWarning() << "TagConfiguration: System tags list is empty";
        return false;
    }
    
    // 检查默认颜色是否有效
    if (!m_defaultTagColor.isValid()) {
        qWarning() << "TagConfiguration: Default tag color is invalid";
        return false;
    }
    
    // 检查排序方式是否在有效范围内
    if (m_tagSortOrder < 0 || m_tagSortOrder > 2) {
        qWarning() << "TagConfiguration: Invalid tag sort order:" << m_tagSortOrder;
        return false;
    }
    
    return true;
}

void TagConfiguration::initializeDefaults()
{
    m_systemTags = Constants::SystemTags::getAll();
    m_tagColors.clear();
    m_tagIcons.clear();
    m_defaultTagColor = Constants::UI::PRIMARY_COLOR;
    m_defaultTagIcon = ":/images/editLabel.png";
    m_showSystemTags = true;
    m_allowEditSystemTags = false;
    m_tagSortOrder = 0; // 按名称排序
    m_autoCreateTags = true;
    m_maxTagCount = -1; // 无限制
    
    // 设置系统标签的默认图标
    m_tagIcons[Constants::SystemTags::MY_SONGS] = ":/images/playlistIcon.png";
    m_tagIcons[Constants::SystemTags::FAVORITES] = ":/images/addToListIcon.png";
    m_tagIcons[Constants::SystemTags::RECENT_PLAYED] = ":/images/followingSongIcon.png";
    m_tagIcons[Constants::SystemTags::DEFAULT_TAG] = ":/images/createIcon.png";
    
    // 设置系统标签的默认颜色
    m_tagColors[Constants::SystemTags::MY_SONGS] = Constants::UI::PRIMARY_COLOR;
    m_tagColors[Constants::SystemTags::FAVORITES] = Constants::UI::WARNING_COLOR;
    m_tagColors[Constants::SystemTags::RECENT_PLAYED] = Constants::UI::SUCCESS_COLOR;
    m_tagColors[Constants::SystemTags::DEFAULT_TAG] = Constants::UI::SYSTEM_TAG_COLOR;
}

void TagConfiguration::emitConfigurationChanged(const QString& key)
{
    // 延迟发射信号，避免在锁定状态下发射
    QMetaObject::invokeMethod(this, [this, key]() {
        emit configurationChanged(key);
    }, Qt::QueuedConnection);
}