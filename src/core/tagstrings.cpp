#include "tagstrings.h"
#include <QDir>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QLibraryInfo>
#include <QDebug>
#include <QFileInfo>
#include <cmath>

// 静态成员初始化
TagStrings* TagStrings::s_instance = nullptr;

TagStrings::TagStrings(QObject* parent)
    : QObject(parent)
    , m_currentLocale(QLocale::system())
    , m_translator(nullptr)
{
    // 初始化支持的语言列表
    m_supportedLocales << QLocale(QLocale::Chinese, QLocale::China)        // 简体中文
                      << QLocale(QLocale::Chinese, QLocale::Taiwan)       // 繁体中文
                      << QLocale(QLocale::English, QLocale::UnitedStates) // 英语
                      << QLocale(QLocale::Japanese, QLocale::Japan)       // 日语
                      << QLocale(QLocale::Korean, QLocale::SouthKorea);   // 韩语
}

TagStrings::~TagStrings()
{
    if (m_translator) {
        QCoreApplication::removeTranslator(m_translator);
        delete m_translator;
    }
}

TagStrings* TagStrings::instance()
{
    static QMutex mutex;
    QMutexLocker locker(&mutex);
    
    if (!s_instance) {
        s_instance = new TagStrings();
        // 默认初始化
        s_instance->initialize();
    }
    return s_instance;
}

void TagStrings::cleanup()
{
    if (s_instance) {
        delete s_instance;
        s_instance = nullptr;
    }
}

void TagStrings::initialize(const QLocale& locale, const QString& translationPath)
{
    QMutexLocker locker(&m_mutex);
    
    m_currentLocale = locale;
    
    // 确定翻译文件路径
    if (translationPath.isEmpty()) {
        // 默认路径：应用程序目录下的translations文件夹
        m_translationPath = QCoreApplication::applicationDirPath() + "/translations";
        
        // 如果默认路径不存在，尝试资源路径
        if (!QDir(m_translationPath).exists()) {
            m_translationPath = ":/translations";
        }
    } else {
        m_translationPath = translationPath;
    }
    
    // 加载翻译
    loadTranslation(m_currentLocale);
    
    qDebug() << "TagStrings initialized with locale:" << m_currentLocale.name()
             << "Translation path:" << m_translationPath;
}

bool TagStrings::switchLanguage(const QLocale& locale)
{
    QMutexLocker locker(&m_mutex);
    
    if (m_currentLocale == locale) {
        return true; // 已经是当前语言
    }
    
    // 移除当前翻译器
    if (m_translator) {
        QCoreApplication::removeTranslator(m_translator);
        delete m_translator;
        m_translator = nullptr;
    }
    
    // 加载新语言
    if (loadTranslation(locale)) {
        QLocale oldLocale = m_currentLocale;
        m_currentLocale = locale;
        
        // 发送语言改变信号
        emit languageChanged(m_currentLocale);
        
        qDebug() << "Language switched from" << oldLocale.name() << "to" << m_currentLocale.name();
        return true;
    }
    
    // 如果加载失败，恢复原来的翻译器
    loadTranslation(m_currentLocale);
    qWarning() << "Failed to switch language to" << locale.name();
    return false;
}

QLocale TagStrings::currentLocale() const
{
    QMutexLocker locker(&m_mutex);
    return m_currentLocale;
}

QList<QLocale> TagStrings::supportedLocales() const
{
    QMutexLocker locker(&m_mutex);
    return m_supportedLocales;
}

void TagStrings::reloadTranslations()
{
    QMutexLocker locker(&m_mutex);
    
    // 重新加载当前语言的翻译
    if (m_translator) {
        QCoreApplication::removeTranslator(m_translator);
        delete m_translator;
        m_translator = nullptr;
    }
    
    loadTranslation(m_currentLocale);
    qDebug() << "Translations reloaded for locale:" << m_currentLocale.name();
}

bool TagStrings::loadTranslation(const QLocale& locale)
{
    QString translationFile = getTranslationFilePath(locale);
    
    if (translationFile.isEmpty()) {
        qWarning() << "No translation file found for locale:" << locale.name();
        return false;
    }
    
    m_translator = new QTranslator(this);
    
    if (m_translator->load(translationFile)) {
        QCoreApplication::installTranslator(m_translator);
        qDebug() << "Translation loaded:" << translationFile;
        return true;
    } else {
        delete m_translator;
        m_translator = nullptr;
        qWarning() << "Failed to load translation:" << translationFile;
        return false;
    }
}

QString TagStrings::getTranslationFilePath(const QLocale& locale) const
{
    // 构建可能的翻译文件名
    QStringList possibleNames;
    possibleNames << QString("musicPlayHandle_%1").arg(locale.name())           // musicPlayHandle_zh_CN
                  << QString("musicPlayHandle_%1").arg(locale.name().left(2))   // musicPlayHandle_zh
                  << QString("tags_%1").arg(locale.name())                      // tags_zh_CN
                  << QString("tags_%1").arg(locale.name().left(2));             // tags_zh
    
    // 在翻译路径中查找文件
    for (const QString& baseName : possibleNames) {
        QString filePath = m_translationPath + "/" + baseName + ".qm";
        
        if (QFileInfo::exists(filePath)) {
            return filePath;
        }
        
        // 如果是资源路径，直接检查
        if (m_translationPath.startsWith("::")) {
            return filePath; // 资源文件存在性由QTranslator::load检查
        }
    }
    
    return QString();
}

// ========== 格式化方法实现 ==========

QString TagStrings::formatDuration(int seconds)
{
    if (seconds < 0) {
        return "--:--";
    }
    
    int hours = seconds / 3600;
    int minutes = (seconds % 3600) / 60;
    int secs = seconds % 60;
    
    if (hours > 0) {
        return QString("%1:%2:%3")
               .arg(hours)
               .arg(minutes, 2, 10, QChar('0'))
               .arg(secs, 2, 10, QChar('0'));
    } else {
        return QString("%1:%2")
               .arg(minutes)
               .arg(secs, 2, 10, QChar('0'));
    }
}

QString TagStrings::formatFileSize(qint64 bytes)
{
    if (bytes < 0) {
        return tr("未知大小");
    }
    
    const qint64 KB = 1024;
    const qint64 MB = KB * 1024;
    const qint64 GB = MB * 1024;
    const qint64 TB = GB * 1024;
    
    if (bytes >= TB) {
        return tr("%1 TB").arg(QString::number(static_cast<double>(bytes) / TB, 'f', 2));
    } else if (bytes >= GB) {
        return tr("%1 GB").arg(QString::number(static_cast<double>(bytes) / GB, 'f', 2));
    } else if (bytes >= MB) {
        return tr("%1 MB").arg(QString::number(static_cast<double>(bytes) / MB, 'f', 2));
    } else if (bytes >= KB) {
        return tr("%1 KB").arg(QString::number(static_cast<double>(bytes) / KB, 'f', 1));
    } else {
        return tr("%1 字节").arg(bytes);
    }
}

QString TagStrings::formatDateTime(const QDateTime& dateTime)
{
    if (!dateTime.isValid()) {
        return tr("无效日期");
    }
    
    QLocale locale = instance()->currentLocale();
    return locale.toString(dateTime, QLocale::ShortFormat);
}

QString TagStrings::formatRelativeTime(const QDateTime& dateTime)
{
    if (!dateTime.isValid()) {
        return tr("无效日期");
    }
    
    QDateTime now = QDateTime::currentDateTime();
    qint64 secondsAgo = dateTime.secsTo(now);
    
    if (secondsAgo < 0) {
        return tr("未来时间");
    }
    
    if (secondsAgo < 60) {
        return tr("刚刚");
    } else if (secondsAgo < 3600) {
        int minutes = secondsAgo / 60;
        return tr("%1分钟前").arg(minutes);
    } else if (secondsAgo < 86400) {
        int hours = secondsAgo / 3600;
        return tr("%1小时前").arg(hours);
    } else if (secondsAgo < 2592000) { // 30天
        int days = secondsAgo / 86400;
        return tr("%1天前").arg(days);
    } else if (secondsAgo < 31536000) { // 365天
        int months = secondsAgo / 2592000;
        return tr("%1个月前").arg(months);
    } else {
        int years = secondsAgo / 31536000;
        return tr("%1年前").arg(years);
    }
}