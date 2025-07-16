#include "lazyloader.h"
#include "../models/tag.h"
#include "../models/song.h"
#include "../managers/tagmanager.h"
#include "../database/songdao.h"
#include "../database/databasemanager.h"
#include <QDebug>

// LazyTagList 实现
LazyTagList::LazyTagList(QObject* parent)
    : LazyLoader<Tag>(parent), m_systemOnly(false), m_userOnly(false)
{
}

void LazyTagList::setFilter(bool systemOnly, bool userOnly)
{
    QMutexLocker locker(&m_mutex);
    
    if (m_systemOnly != systemOnly || m_userOnly != userOnly) {
        m_systemOnly = systemOnly;
        m_userOnly = userOnly;
        
        // 如果已加载，清空数据以触发重新加载
        if (m_loaded) {
            clear();
        }
    }
}

QList<Tag> LazyTagList::doLoadData()
{
    QList<Tag> tags;
    
    try {
        TagManager* tagManager = TagManager::instance();
        if (!tagManager) {
            qWarning() << "LazyTagList: TagManager instance not available";
            return tags;
        }
        
        // 获取所有标签
        QList<Tag> allTags; // = tagManager->getAllTags();
        
        // 应用过滤条件
        for (const Tag& tag : allTags) {
            bool isSystem = Constants::SystemTags::isSystemTag(tag.name());
            
            if (m_systemOnly && !isSystem) {
                continue;
            }
            if (m_userOnly && isSystem) {
                continue;
            }
            
            tags.append(tag);
        }
        
        qDebug() << "LazyTagList: Loaded" << tags.size() << "tags";
        
    } catch (const std::exception& e) {
        qWarning() << "LazyTagList: Exception during data loading:" << e.what();
        throw;
    }
    
    return tags;
}

// LazySongList 实现
LazySongList::LazySongList(int tagId, QObject* parent)
    : LazyLoader<Song>(parent), m_tagId(tagId)
{
}

void LazySongList::setTagFilter(int tagId)
{
    QMutexLocker locker(&m_mutex);
    
    if (m_tagId != tagId) {
        m_tagId = tagId;
        
        // 如果已加载，清空数据以触发重新加载
        if (m_loaded) {
            clear();
        }
    }
}

QList<Song> LazySongList::doLoadData()
{
    QList<Song> songs;
    
    try {
        DatabaseManager* dbManager = DatabaseManager::instance();
        if (!dbManager || !dbManager->isConnected()) {
            qWarning() << "LazySongList: Database not available";
            return songs;
        }
        
        SongDao songDao;
        
        if (m_tagId == -1) {
            // 加载所有歌曲
            songs = songDao.getAllSongs();
        } else {
            // 加载特定标签下的歌曲
            songs = songDao.getSongsByTag(m_tagId);
        }
        
        qDebug() << "LazySongList: Loaded" << songs.size() << "songs for tag" << m_tagId;
        
    } catch (const std::exception& e) {
        qWarning() << "LazySongList: Exception during data loading:" << e.what();
        throw;
    }
    
    return songs;
}

// 移除 moc 文件包含，因为 LazyLoader 是模板类