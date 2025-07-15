#include "tagmanager.h"
#include <QMutexLocker>

static TagManager* s_instance = nullptr;
static QMutex s_mutex;

TagManager::TagManager(QObject* parent)
    : QObject(parent)
{
}

TagManager::~TagManager()
{
}

TagManager* TagManager::instance()
{
    QMutexLocker locker(&s_mutex);
    if (!s_instance) {
        s_instance = new TagManager();
    }
    return s_instance;
}

void TagManager::cleanup()
{
    QMutexLocker locker(&s_mutex);
    delete s_instance;
    s_instance = nullptr;
}

bool TagManager::initialize() { return true; }
void TagManager::shutdown() {}

TagOperationResult TagManager::createTag(const QString& name, const QString& description, const QColor& color, const QPixmap& icon)
{
    Q_UNUSED(name)
    Q_UNUSED(description)
    Q_UNUSED(color)
    Q_UNUSED(icon)
    return TagOperationResult(true, "stub createTag");
}

bool TagManager::tagExists(const QString& name) const
{
    Q_UNUSED(name)
    return false;
}

bool TagManager::tagExists(int tagId) const
{
    Q_UNUSED(tagId)
    return false;
}

void TagManager::onTagChanged(int tagId) { Q_UNUSED(tagId); }
void TagManager::onSongChanged(int songId) { Q_UNUSED(songId); }
void TagManager::updateStatistics() {}
void TagManager::cleanupOrphanedTags() {}

QList<Tag> TagManager::getTagsForSong(int songId) const
{
    Q_UNUSED(songId)
    return QList<Tag>();
}

// ... 你可以根据需要继续补充其它被调用的成员函数 stub 实现 ...