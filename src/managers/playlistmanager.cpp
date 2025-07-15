#include "playlistmanager.h"
#include "../database/playlistdao.h"
#include "../database/songdao.h"
#include "../core/constants.h"
#include <QDebug>
#include <QFileInfo>
#include <QTextStream>
#include <QStringConverter>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QMutex>
#include <QMutexLocker>

// 静态成员变量定义
PlaylistManager* PlaylistManager::m_instance = nullptr;
QMutex PlaylistManager::m_instanceMutex;

PlaylistManager::PlaylistManager(QObject* parent)
    : QObject(parent)
    , m_playlistDao(nullptr)
    , m_songDao(nullptr)
    , m_currentPlaylistId(-1)
    , m_currentIndex(-1)
    , m_currentSongIndex(-1)
    , m_playMode(PlayMode::Sequential)
    , m_state(PlaylistState::Stopped)
    , m_repeatMode(RepeatMode::NoRepeat)
    , m_shuffleMode(false)
    , m_shuffleIndex(-1)
    , m_maxHistorySize(100)
    , m_cacheEnabled(true)
    , m_undoRedoEnabled(true)
    , m_maxUndoRedoSize(50)
    , m_nextPlaylistId(1)
{
    qDebug() << "PlaylistManager 构造函数";
    initializeDao();
}

PlaylistManager::~PlaylistManager()
{
    qDebug() << "PlaylistManager 析构函数";
    cleanup();
}

bool PlaylistManager::initialize()
{
    // Logger::instance().logInfo("PlaylistManager::initialize", "初始化播放列表管理器");
    qDebug() << "PlaylistManager::initialize: 初始化播放列表管理器";
    
    if (!initializeDao()) {
        // Logger::instance().logError("PlaylistManager::initialize", "初始化DAO失败");
        qDebug() << "PlaylistManager::initialize: 初始化DAO失败";
        return false;
    }
    
    // 创建默认播放列表
    createDefaultPlaylists();
    
    // Logger::instance().logInfo("PlaylistManager::initialize", "播放列表管理器初始化完成");
    qDebug() << "PlaylistManager::initialize: 播放列表管理器初始化完成";
    return true;
}

void PlaylistManager::shutdown()
{
    // Logger::instance().logInfo("PlaylistManager::shutdown", "清理播放列表管理器");
    qDebug() << "PlaylistManager::shutdown: 清理播放列表管理器";
    cleanup();
}

PlaylistManager* PlaylistManager::instance()
{
    QMutexLocker locker(&m_instanceMutex);
    if (!m_instance) {
        m_instance = new PlaylistManager();
    }
    return m_instance;
}

void PlaylistManager::cleanup()
{
    QMutexLocker locker(&m_instanceMutex);
    if (m_instance) {
        delete m_instance;
        m_instance = nullptr;
    }
}

PlaylistOperationResult PlaylistManager::createPlaylist(const QString& name, const QString& description)
{
    PlaylistOperationResult result;
    
    if (name.trimmed().isEmpty()) {
        result.success = false;
        result.message = "播放列表名称不能为空";
        // Logger::instance().logError("PlaylistManager::createPlaylist", "播放列表名称不能为空");
        qDebug() << "PlaylistManager::createPlaylist: 播放列表名称不能为空";
        return result;
    }
    
    if (!m_playlistDao) {
        result.success = false;
        result.message = "PlaylistDao未初始化";
        // Logger::instance().logError("PlaylistManager::createPlaylist", "PlaylistDao未初始化");
        qDebug() << "PlaylistManager::createPlaylist: PlaylistDao未初始化";
        return result;
    }
    
    // 检查名称是否已存在
    if (m_playlistDao->playlistExists(name)) {
        // Logger::instance().logError("PlaylistManager::createPlaylist", 
        //     QString("播放列表名称已存在: %1").arg(name));
        qDebug() << "PlaylistManager::createPlaylist: 播放列表名称已存在:" << name;
        return PlaylistOperationResult(false, QString("播放列表名称已存在: %1").arg(name));
    }
    
    Playlist playlist;
    playlist.setName(name.trimmed());
    playlist.setDescription(description);
    playlist.setCreatedAt(QDateTime::currentDateTime());
    playlist.setModifiedAt(QDateTime::currentDateTime());
    playlist.setSongCount(0);
    playlist.setTotalDuration(0);
    playlist.setPlayCount(0);
    playlist.setColor(QColor("#3498db")); // 默认蓝色
    playlist.setIsSmartPlaylist(false);
    playlist.setIsSystemPlaylist(false);
    playlist.setIsFavorite(false);
    playlist.setSortOrder(getNextSortOrder());
    
    int playlistId = m_playlistDao->addPlaylist(playlist);
    if (playlistId > 0) {
        // Logger::instance().logInfo("PlaylistManager::createPlaylist", 
        //     QString("成功创建播放列表: %1, ID: %2").arg(name).arg(playlistId));
        qDebug() << "PlaylistManager::createPlaylist: 成功创建播放列表:" << name << "ID:" << playlistId;
        
        // 发射信号
        Playlist createdPlaylist = m_playlistDao->getPlaylistById(playlistId);
        emit playlistCreated(createdPlaylist);
        
        return PlaylistOperationResult(true, "播放列表创建成功", QVariant::fromValue(createdPlaylist));
    }
    
    return PlaylistOperationResult(false, "创建播放列表失败");
}

PlaylistOperationResult PlaylistManager::updatePlaylist(int playlistId, const QString& name, const QString& description)
{
    PlaylistOperationResult result;
    
    if (!m_playlistDao) {
        result.success = false;
        result.message = "PlaylistDao未初始化";
        // Logger::instance().logError("PlaylistManager::updatePlaylist", "PlaylistDao未初始化");
        qDebug() << "PlaylistManager::updatePlaylist: PlaylistDao未初始化";
        return result;
    }
    
    if (playlistId <= 0) {
        result.success = false;
        result.message = "无效的播放列表ID";
        // Logger::instance().logError("PlaylistManager::updatePlaylist", "无效的播放列表ID");
        qDebug() << "PlaylistManager::updatePlaylist: 无效的播放列表ID";
        return result;
    }
    
    if (name.trimmed().isEmpty()) {
        result.success = false;
        result.errorMessage = "播放列表名称不能为空";
        // Logger::instance().logError("PlaylistManager::updatePlaylist", "播放列表名称不能为空");
        qDebug() << "PlaylistManager::updatePlaylist: 播放列表名称不能为空";
        return result;
    }
    
    // 获取现有播放列表
    Playlist playlist = m_playlistDao->getPlaylistById(playlistId);
    if (playlist.id() <= 0) {
        return PlaylistOperationResult(false, "播放列表不存在");
    }
    
    // 更新播放列表信息
    playlist.setName(name.trimmed());
    playlist.setDescription(description);
    playlist.setModifiedAt(QDateTime::currentDateTime());
    
    bool success = m_playlistDao->updatePlaylist(playlist);
    if (success) {
        // Logger::instance().logInfo("PlaylistManager::updatePlaylist", 
        //     QString("成功更新播放列表: %1").arg(name));
        qDebug() << "PlaylistManager::updatePlaylist: 成功更新播放列表:" << name;
        
        // 如果更新的是当前播放列表，刷新当前播放列表
        if (m_currentPlaylistId == playlistId) {
            loadPlaylist(playlistId);
        }
        
        // 发射信号
        emit playlistUpdated(playlist);
        
        return PlaylistOperationResult(true, "播放列表更新成功", QVariant::fromValue(playlist));
    }
    
    return PlaylistOperationResult(false, "更新播放列表失败");
}

PlaylistOperationResult PlaylistManager::deletePlaylist(int playlistId)
{
    PlaylistOperationResult result;
    
    if (!m_playlistDao) {
        result.success = false;
        result.message = "PlaylistDao未初始化";
        // Logger::instance().logError("PlaylistManager::deletePlaylist", "PlaylistDao未初始化");
        qDebug() << "PlaylistManager::deletePlaylist: PlaylistDao未初始化";
        return result;
    }
    
    if (playlistId <= 0) {
        result.success = false;
        result.message = "无效的播放列表ID";
        // Logger::instance().logError("PlaylistManager::deletePlaylist", "无效的播放列表ID");
        qDebug() << "PlaylistManager::deletePlaylist: 无效的播放列表ID";
        return result;
    }
    
    // 获取播放列表信息用于信号发射
    Playlist playlist = m_playlistDao->getPlaylistById(playlistId);
    if (playlist.id() <= 0) {
        return PlaylistOperationResult(false, "播放列表不存在");
    }
    
    // 检查是否为系统播放列表
    if (playlist.isSystemPlaylist()) {
        // Logger::instance().logError("PlaylistManager::deletePlaylist", 
        //     QString("不能删除系统播放列表: %1").arg(playlist.name()));
        qDebug() << "PlaylistManager::deletePlaylist: 不能删除系统播放列表:" << playlist.name();
        return PlaylistOperationResult(false, QString("不能删除系统播放列表: %1").arg(playlist.name()));
    }
    
    bool success = m_playlistDao->deletePlaylist(playlistId);
    if (success) {
        // Logger::instance().logInfo("PlaylistManager::deletePlaylist", 
        //     QString("成功删除播放列表: %1").arg(playlist.name()));
        qDebug() << "PlaylistManager::deletePlaylist: 成功删除播放列表:" << playlist.name();
        
        // 如果删除的是当前播放列表，清空当前播放列表
        if (m_currentPlaylistId == playlistId) {
            clearCurrentPlaylist();
        }
        
        // 发射信号
        emit playlistDeleted(playlistId, playlist.name());
        
        return PlaylistOperationResult(true, "播放列表删除成功");
    }
    
    return PlaylistOperationResult(false, "删除播放列表失败");
}

Playlist PlaylistManager::getPlaylist(int playlistId) const
{
    if (!m_playlistDao) {
        // Logger::instance().logError("PlaylistManager::getPlaylist", "PlaylistDao未初始化");
        qDebug() << "PlaylistManager::getPlaylist: PlaylistDao未初始化";
        return Playlist();
    }
    
    return m_playlistDao->getPlaylistById(playlistId);
}

Playlist PlaylistManager::getPlaylistByName(const QString& name) const
{
    if (!m_playlistDao) {
        // Logger::instance().logError("PlaylistManager::getPlaylistByName", "PlaylistDao未初始化");
        qDebug() << "PlaylistManager::getPlaylistByName: PlaylistDao未初始化";
        return Playlist();
    }
    
    return m_playlistDao->getPlaylistByName(name);
}

QList<Playlist> PlaylistManager::getAllPlaylists() const
{
    if (!m_playlistDao) {
        // Logger::instance().logError("PlaylistManager::getAllPlaylists", "PlaylistDao未初始化");
        qDebug() << "PlaylistManager::getAllPlaylists: PlaylistDao未初始化";
        return QList<Playlist>();
    }
    
    return m_playlistDao->getAllPlaylists();
}

// 这些方法的实现已移动到文件末尾，避免重复定义

QList<Playlist> PlaylistManager::getRecentPlaylists(int count) const
{
    if (!m_playlistDao) {
        // Logger::instance().logError("PlaylistManager::getRecentPlaylists", "PlaylistDao未初始化");
    qDebug() << "PlaylistManager::getRecentPlaylists: PlaylistDao未初始化";
        return QList<Playlist>();
    }
    
    return m_playlistDao->getRecentPlaylists(count);
}

QList<Playlist> PlaylistManager::getFavoritePlaylists() const
{
    if (!m_playlistDao) {
        // Logger::instance().logError("PlaylistManager::getFavoritePlaylists", "PlaylistDao未初始化");
    qDebug() << "PlaylistManager::getFavoritePlaylists: PlaylistDao未初始化";
        return QList<Playlist>();
    }
    
    return m_playlistDao->getFavoritePlaylists();
}

PlaylistOperationResult PlaylistManager::addSongToPlaylist(int playlistId, const Song& song)
{
    if (!m_playlistDao) {
        // Logger::instance().logError("PlaylistManager::addSongToPlaylist", "PlaylistDao未初始化");
    qDebug() << "PlaylistManager::addSongToPlaylist: PlaylistDao未初始化";
        return PlaylistOperationResult(false, "PlaylistDao未初始化");
    }
    
    if (playlistId <= 0) {
        return PlaylistOperationResult(false, "无效的播放列表ID");
    }
    
    if (song.id() <= 0) {
        return PlaylistOperationResult(false, "无效的歌曲");
    }
    
    bool success = m_playlistDao->addSongToPlaylist(playlistId, song.id());
    if (success) {
        // Logger::instance().logInfo("PlaylistManager::addSongToPlaylist",
        //     QString("成功添加歌曲到播放列表: 播放列表ID=%1, 歌曲ID=%2").arg(playlistId).arg(song.id()));
        qDebug() << "PlaylistManager::addSongToPlaylist: 成功添加歌曲到播放列表, 播放列表ID=" << playlistId << ", 歌曲ID=" << song.id();
        
        // 如果添加到当前播放列表，刷新当前播放列表
        if (m_currentPlaylistId == playlistId) {
            loadPlaylist(playlistId);
        }
        
        // 发射信号
        Playlist playlist = m_playlistDao->getPlaylistById(playlistId);
        emit playlistUpdated(playlist);
        emit songAddedToPlaylist(playlistId, song, playlist.songCount());
        
        return PlaylistOperationResult(true, "歌曲添加成功");
    }
    
    return PlaylistOperationResult(false, "添加歌曲失败");
}

PlaylistOperationResult PlaylistManager::addSongsToPlaylist(int playlistId, const QList<Song>& songs)
{
    if (!m_playlistDao) {
        // Logger::instance().logError("PlaylistManager::addSongsToPlaylist", "PlaylistDao未初始化");
    qDebug() << "PlaylistManager::addSongsToPlaylist: PlaylistDao未初始化";
        return PlaylistOperationResult(false, "PlaylistDao未初始化");
    }
    
    if (playlistId <= 0) {
        return PlaylistOperationResult(false, "无效的播放列表ID");
    }
    
    if (songs.isEmpty()) {
        // Logger::instance().logWarning("PlaylistManager::addSongsToPlaylist", "歌曲列表为空");
        qDebug() << "PlaylistManager::addSongsToPlaylist: 歌曲列表为空";
        return PlaylistOperationResult(true, "歌曲列表为空，无需添加");
    }
    
    int successCount = 0;
    int totalCount = songs.size();
    
    for (const Song& song : songs) {
        if (song.id() > 0 && m_playlistDao->addSongToPlaylist(playlistId, song.id())) {
            successCount++;
        } else {
            // Logger::instance().logError("PlaylistManager::addSongsToPlaylist", 
            //     QString("添加歌曲失败: 播放列表ID=%1, 歌曲ID=%2").arg(playlistId).arg(song.id()));
            qDebug() << "PlaylistManager::addSongsToPlaylist: 添加歌曲失败, 播放列表ID=" << playlistId << ", 歌曲ID=" << song.id();
        }
    }
    
    if (successCount > 0) {
        // Logger::instance().logInfo("PlaylistManager::addSongsToPlaylist", 
        //     QString("成功添加 %1/%2 首歌曲到播放列表: ID=%3").arg(successCount).arg(totalCount).arg(playlistId));
        qDebug() << "PlaylistManager::addSongsToPlaylist: 成功添加" << successCount << "/" << totalCount << "首歌曲到播放列表, ID=" << playlistId;
        
        // 如果添加到当前播放列表，刷新当前播放列表
        if (m_currentPlaylistId == playlistId) {
            loadPlaylist(playlistId);
        }
        
        // 发射信号
        Playlist playlist = m_playlistDao->getPlaylistById(playlistId);
        emit playlistUpdated(playlist);
        
        if (successCount == totalCount) {
            return PlaylistOperationResult(true, QString("成功添加 %1 首歌曲").arg(successCount));
        } else {
            return PlaylistOperationResult(true, QString("部分成功：添加了 %1/%2 首歌曲").arg(successCount).arg(totalCount));
        }
    }
    
    return PlaylistOperationResult(false, "添加歌曲失败");
}

PlaylistOperationResult PlaylistManager::removeSongFromPlaylist(int playlistId, int songIndex)
{
    if (!m_playlistDao) {
        // Logger::instance().logError("PlaylistManager::removeSongFromPlaylist", "PlaylistDao未初始化");
        qDebug() << "PlaylistManager::removeSongFromPlaylist: PlaylistDao未初始化";
        return PlaylistOperationResult(false, "PlaylistDao未初始化");
    }
    
    if (playlistId <= 0) {
        return PlaylistOperationResult(false, "无效的播放列表ID");
    }
    
    if (songIndex < 0) {
        return PlaylistOperationResult(false, "无效的歌曲索引");
    }
    
    // 获取播放列表中的歌曲列表
    QList<Song> songs = getPlaylistSongs(playlistId);
    if (songIndex >= songs.size()) {
        return PlaylistOperationResult(false, "歌曲索引超出范围");
    }
    
    // 获取要移除的歌曲ID
    int songId = songs[songIndex].id();
    
    bool success = m_playlistDao->removeSongFromPlaylist(playlistId, songId);
    if (success) {
        // Logger::instance().logInfo("PlaylistManager::removeSongFromPlaylist", 
        //     QString("成功从播放列表移除歌曲: 播放列表ID=%1, 歌曲索引=%2, 歌曲ID=%3").arg(playlistId).arg(songIndex).arg(songId));
        qDebug() << "PlaylistManager::removeSongFromPlaylist: 成功从播放列表移除歌曲, 播放列表ID=" << playlistId << ", 歌曲索引=" << songIndex << ", 歌曲ID=" << songId;
        
        // 如果从当前播放列表移除，刷新当前播放列表
        if (m_currentPlaylistId == playlistId) {
            loadPlaylist(playlistId);
        }
        
        // 发射信号
        Playlist playlist = m_playlistDao->getPlaylistById(playlistId);
        emit playlistUpdated(playlist);
        
        emit songRemovedFromPlaylist(playlistId, songIndex);
        
        return PlaylistOperationResult(true, "成功移除歌曲");
    }
    
    return PlaylistOperationResult(false, "移除歌曲失败");
}

QList<Song> PlaylistManager::getPlaylistSongs(int playlistId) const
{
    if (!m_playlistDao) {
        // Logger::instance().logError("PlaylistManager::getPlaylistSongs", "PlaylistDao未初始化");
        qDebug() << "PlaylistManager::getPlaylistSongs: PlaylistDao未初始化";
        return QList<Song>();
    }
    
    return m_playlistDao->getPlaylistSongs(playlistId);
}

int PlaylistManager::getPlaylistSongCount(int playlistId) const
{
    if (!m_playlistDao) {
        // Logger::instance().logError("PlaylistManager::getPlaylistSongCount", "PlaylistDao未初始化");
        qDebug() << "PlaylistManager::getPlaylistSongCount: PlaylistDao未初始化";
        return 0;
    }
    
    return m_playlistDao->getPlaylistSongCount(playlistId);
}

PlaylistOperationResult PlaylistManager::clearPlaylist(int playlistId)
{
    if (!m_playlistDao) {
        // Logger::instance().logError("PlaylistManager::clearPlaylist", "PlaylistDao未初始化");
        qDebug() << "PlaylistManager::clearPlaylist: PlaylistDao未初始化";
        return PlaylistOperationResult(false, "PlaylistDao未初始化");
    }
    
    if (playlistId <= 0) {
        return PlaylistOperationResult(false, "无效的播放列表ID");
    }
    
    bool success = m_playlistDao->clearPlaylist(playlistId);
    if (success) {
        // Logger::instance().logInfo("PlaylistManager::clearPlaylist", 
        //     QString("成功清空播放列表: ID=%1").arg(playlistId));
        qDebug() << "PlaylistManager::clearPlaylist: 成功清空播放列表, ID=" << playlistId;
        
        // 如果清空的是当前播放列表，清空当前播放列表
        if (m_currentPlaylistId == playlistId) {
            clearCurrentPlaylist();
        }
        
        // 发射信号
        Playlist playlist = m_playlistDao->getPlaylistById(playlistId);
        emit playlistUpdated(playlist);
        emit playlistCleared(playlistId);
        
        return PlaylistOperationResult(true, "播放列表清空成功");
    }
    
    return PlaylistOperationResult(false, "清空播放列表失败");
}

bool PlaylistManager::loadPlaylist(int playlistId)
{
    if (!m_playlistDao) {
        // Logger::instance().logError("PlaylistManager::loadPlaylist", "PlaylistDao未初始化");
        qDebug() << "PlaylistManager::loadPlaylist: PlaylistDao未初始化";
        return false;
    }
    
    if (playlistId <= 0) {
        // Logger::instance().logError("PlaylistManager::loadPlaylist", "无效的播放列表ID");
        qDebug() << "PlaylistManager::loadPlaylist: 无效的播放列表ID";
        return false;
    }
    
    // 获取播放列表歌曲
    QList<Song> songs = m_playlistDao->getPlaylistSongs(playlistId);
    
    m_currentPlaylistId = playlistId;
    m_currentPlaylist = getPlaylist(playlistId);
    m_currentPlaylistSongs = songs;
    m_currentSongIndex = -1;
    
    // 重新生成随机播放索引
    generateShuffledIndices();
    
    // Logger::instance().logInfo("PlaylistManager::loadPlaylist", 
    //     QString("成功加载播放列表: ID=%1, 歌曲数量=%2").arg(playlistId).arg(songs.size()));
    qDebug() << "PlaylistManager::loadPlaylist: 成功加载播放列表, ID=" << playlistId << ", 歌曲数量=" << songs.size();
    
    // 发射信号
    emit currentPlaylistChanged(playlistId);
    
    return true;
}

void PlaylistManager::clearCurrentPlaylist()
{
    m_currentPlaylistId = -1;
    m_currentPlaylist.clear();
    m_currentPlaylistSongs.clear();
    m_currentSongIndex = -1;
    m_shuffledIndices.clear();
    
    // Logger::instance().logInfo("PlaylistManager::clearCurrentPlaylist", "清空当前播放列表");
    qDebug() << "PlaylistManager::clearCurrentPlaylist: 清空当前播放列表";
    
    // 发射信号
    emit currentPlaylistChanged(-1);
}

void PlaylistManager::setCurrentPlaylist(int playlistId)
{
    if (m_currentPlaylistId != playlistId) {
        m_currentPlaylistId = playlistId;
        m_currentPlaylist = getPlaylist(playlistId);
        m_currentPlaylistSongs = getPlaylistSongs(playlistId);
        m_currentIndex = -1;
        m_currentSongIndex = -1;
        emit currentPlaylistChanged(playlistId);
        qDebug() << "PlaylistManager::setCurrentPlaylist: 设置当前播放列表ID:" << playlistId;
    }
}

void PlaylistManager::setCurrentPlaylist(const Playlist& playlist)
{
    m_currentPlaylist = playlist;
    setCurrentPlaylist(playlist.id());
}

int PlaylistManager::getCurrentPlaylistId() const
{
    return m_currentPlaylistId;
}

bool PlaylistManager::hasCurrentPlaylist() const
{
    return m_currentPlaylistId != -1;
}

Playlist PlaylistManager::getCurrentPlaylist() const
{
    return m_currentPlaylist;
}

int PlaylistManager::getCurrentSongIndex() const
{
    return m_currentSongIndex;
}

Song PlaylistManager::getCurrentSong() const
{
    if (m_currentPlaylistId > 0 && m_currentSongIndex >= 0) {
        QList<Song> songs = getPlaylistSongs(m_currentPlaylistId);
        if (m_currentSongIndex < songs.size()) {
            return songs.at(m_currentSongIndex);
        }
    }
    return Song();
}

bool PlaylistManager::hasCurrentSong() const
{
    return m_currentPlaylistId != -1 && m_currentIndex != -1;
}

void PlaylistManager::play()
{
    // TODO: 实现播放逻辑
    qDebug() << "PlaylistManager::play: 开始播放";
}

void PlaylistManager::pause()
{
    // TODO: 实现暂停逻辑
    qDebug() << "PlaylistManager::pause: 暂停播放";
}

void PlaylistManager::stop()
{
    // TODO: 实现停止逻辑
    qDebug() << "PlaylistManager::stop: 停止播放";
}

void PlaylistManager::next()
{
    // TODO: 实现下一首逻辑
    qDebug() << "PlaylistManager::next: 下一首";
}

void PlaylistManager::previous()
{
    // TODO: 实现上一首逻辑
    qDebug() << "PlaylistManager::previous: 上一首";
}

void PlaylistManager::playAt(int index)
{
    setCurrentIndex(index);
    play();
}

void PlaylistManager::playPlaylist(int playlistId)
{
    setCurrentPlaylist(playlistId);
    setCurrentIndex(0);
    play();
}

void PlaylistManager::playSong(const Song& song)
{
    // TODO: 实现播放指定歌曲逻辑
    qDebug() << "PlaylistManager::playSong: 播放歌曲" << song.title();
}

void PlaylistManager::setCurrentIndex(int index)
{
    if (m_currentIndex != index) {
        m_currentIndex = index;
        emit currentIndexChanged(index);
        qDebug() << "PlaylistManager::setCurrentIndex: 设置当前索引:" << index;
    }
}

int PlaylistManager::getCurrentIndex() const
{
    return m_currentIndex;
}

void PlaylistManager::setPlayMode(PlayMode mode)
{
    if (m_playMode != mode) {
        m_playMode = mode;
        emit playModeChanged(mode);
        qDebug() << "PlaylistManager::setPlayMode: 设置播放模式:" << static_cast<int>(mode);
    }
}

PlayMode PlaylistManager::getPlayMode() const
{
    return m_playMode;
}

QString PlaylistManager::getPlayModeString() const
{
    switch (m_playMode) {
    case PlayMode::Sequential: return "顺序播放";
    case PlayMode::Loop: return "列表循环";
    case PlayMode::SingleLoop: return "单曲循环";
    case PlayMode::Random: return "随机播放";
    case PlayMode::Shuffle: return "洗牌播放";
    default: return "未知模式";
    }
}

PlaylistState PlaylistManager::getState() const
{
    return m_state;
}

bool PlaylistManager::isPlaying() const
{
    return m_state == PlaylistState::Playing;
}

bool PlaylistManager::isPaused() const
{
    return m_state == PlaylistState::Paused;
}

bool PlaylistManager::isStopped() const
{
    return m_state == PlaylistState::Stopped;
}

Song PlaylistManager::getNextSong()
{
    if (m_currentPlaylistSongs.isEmpty()) {
        return Song();
    }
    
    int nextIndex = getNextSongIndex();
    if (nextIndex >= 0) {
        m_currentSongIndex = nextIndex;
        return m_currentPlaylistSongs.at(nextIndex);
    }
    
    return Song();
}

Song PlaylistManager::getPreviousSong()
{
    if (m_currentPlaylistSongs.isEmpty()) {
        return Song();
    }
    
    int prevIndex = getPreviousSongIndex();
    if (prevIndex >= 0) {
        m_currentSongIndex = prevIndex;
        return m_currentPlaylistSongs.at(prevIndex);
    }
    
    return Song();
}

bool PlaylistManager::setCurrentSongIndex(int index)
{
    if (index < 0 || index >= m_currentPlaylistSongs.size()) {
        // Logger::instance().logError("PlaylistManager::setCurrentSongIndex", 
        //     QString("无效的歌曲索引: %1").arg(index));
        qDebug() << "PlaylistManager::setCurrentSongIndex: 无效的歌曲索引:" << index;
        return false;
    }
    
    m_currentSongIndex = index;
    // Logger::instance().logInfo("PlaylistManager::setCurrentSongIndex", 
    //     QString("设置当前歌曲索引: %1").arg(index));
    qDebug() << "PlaylistManager::setCurrentSongIndex: 设置当前歌曲索引:" << index;
    
    return true;
}

bool PlaylistManager::isShuffleMode() const
{
    return m_shuffleMode;
}

void PlaylistManager::setShuffleMode(bool enabled)
{
    if (m_shuffleMode != enabled) {
        m_shuffleMode = enabled;
        
        if (enabled) {
            generateShuffledIndices();
        }
        
        // Logger::instance().logInfo("PlaylistManager::setShuffleMode", 
        //     QString("设置随机播放模式: %1").arg(enabled ? "开启" : "关闭"));
        qDebug() << "PlaylistManager::setShuffleMode: 设置随机播放模式:" << (enabled ? "开启" : "关闭");
        
        emit shuffleModeChanged(enabled);
    }
}

RepeatMode PlaylistManager::getRepeatMode() const
{
    return m_repeatMode;
}

void PlaylistManager::setRepeatMode(RepeatMode mode)
{
    if (m_repeatMode != mode) {
        m_repeatMode = mode;
        
        QString modeStr;
        switch (mode) {
        case RepeatMode::NoRepeat:
            modeStr = "不重复";
            break;
        case RepeatMode::RepeatOne:
            modeStr = "单曲循环";
            break;
        case RepeatMode::RepeatAll:
            modeStr = "列表循环";
            break;
        }
        
        // Logger::instance().logInfo("PlaylistManager::setRepeatMode", 
        //     QString("设置重复播放模式: %1").arg(modeStr));
        qDebug() << "PlaylistManager::setRepeatMode: 设置重复播放模式:" << modeStr;
        
        emit repeatModeChanged(mode);
    }
}

bool PlaylistManager::exportPlaylist(int playlistId, const QString& filePath, ExportFormat format)
{
    if (!m_playlistDao) {
        // Logger::instance().logError("PlaylistManager::exportPlaylist", "PlaylistDao未初始化");
        qDebug() << "PlaylistManager::exportPlaylist: PlaylistDao未初始化";
        return false;
    }
    
    Playlist playlist = m_playlistDao->getPlaylistById(playlistId);
    if (!playlist.isValid()) {
        // Logger::instance().logError("PlaylistManager::exportPlaylist", 
        //     QString("播放列表不存在: ID=%1").arg(playlistId));
        qDebug() << "PlaylistManager::exportPlaylist: 播放列表不存在, ID=" << playlistId;
        return false;
    }
    
    QList<Song> songs = m_playlistDao->getPlaylistSongs(playlistId);
    
    bool success = false;
    switch (format) {
    case ExportFormat::M3U:
        success = exportToM3U(playlist, songs, filePath);
        break;
    case ExportFormat::PLS:
        success = exportToPLS(playlist, songs, filePath);
        break;
    case ExportFormat::JSON:
        success = exportToJSON(playlist, songs, filePath);
        break;
    }
    
    if (success) {
        // Logger::instance().logInfo("PlaylistManager::exportPlaylist", 
        //     QString("成功导出播放列表: %1 到 %2").arg(playlist.name()).arg(filePath));
        qDebug() << "PlaylistManager::exportPlaylist: 成功导出播放列表:" << playlist.name() << "到" << filePath;
    }
    
    return success;
}

bool PlaylistManager::importPlaylist(const QString& filePath, const QString& playlistName)
{
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        // Logger::instance().logError("PlaylistManager::importPlaylist", 
        //     QString("文件不存在: %1").arg(filePath));
        qDebug() << "PlaylistManager::importPlaylist: 文件不存在:" << filePath;
        return false;
    }
    
    QString suffix = fileInfo.suffix().toLower();
    bool success = false;
    
    if (suffix == "m3u" || suffix == "m3u8") {
        success = importFromM3U(filePath, playlistName);
    } else if (suffix == "pls") {
        success = importFromPLS(filePath, playlistName);
    } else if (suffix == "json") {
        success = importFromJSON(filePath, playlistName);
    } else {
        // Logger::instance().logError("PlaylistManager::importPlaylist", 
        //     QString("不支持的文件格式: %1").arg(suffix));
        qDebug() << "PlaylistManager::importPlaylist: 不支持的文件格式:" << suffix;
        return false;
    }
    
    if (success) {
        // Logger::instance().logInfo("PlaylistManager::importPlaylist", 
        //     QString("成功导入播放列表: %1").arg(playlistName));
        qDebug() << "PlaylistManager::importPlaylist: 成功导入播放列表:" << playlistName;
    }
    
    return success;
}

bool PlaylistManager::initializeDao()
{
    try {
        if (!m_playlistDao) {
            m_playlistDao = new PlaylistDao(this);
        }
        
        if (!m_songDao) {
            m_songDao = new SongDao(this);
        }
        
        return true;
        
    } catch (const std::exception& e) {
        // Logger::instance().logError("PlaylistManager::initializeDao", 
        //     QString("初始化DAO时发生异常: %1").arg(e.what()));
        qDebug() << "PlaylistManager::initializeDao: 初始化DAO时发生异常:" << e.what();
        return false;
    }
}

void PlaylistManager::createDefaultPlaylists()
{
    if (!m_playlistDao) {
        return;
    }
    
    // 创建"我喜欢的音乐"播放列表
    if (!m_playlistDao->playlistExists("我喜欢的音乐")) {
        Playlist favorites;
        favorites.setName("我喜欢的音乐");
        favorites.setDescription("收藏的音乐");
        favorites.setCreatedAt(QDateTime::currentDateTime());
        favorites.setModifiedAt(QDateTime::currentDateTime());
        favorites.setColor(QColor("#e74c3c")); // 红色
        favorites.setIconPath(":/icons/heart.svg");
        favorites.setIsSystemPlaylist(true);
        favorites.setIsFavorite(true);
        favorites.setSortOrder(0);
        
        int favoritesId = m_playlistDao->addPlaylist(favorites);
        if (favoritesId > 0) {
            // Logger::instance().logInfo("PlaylistManager::createDefaultPlaylists", 
            //     "创建默认播放列表: 我喜欢的音乐");
            qDebug() << "PlaylistManager::createDefaultPlaylists: 创建默认播放列表: 我喜欢的音乐";
        }
    }
    
    // 创建"最近播放"播放列表
    if (!m_playlistDao->playlistExists("最近播放")) {
        Playlist recent;
        recent.setName("最近播放");
        recent.setDescription("最近播放的音乐");
        recent.setCreatedAt(QDateTime::currentDateTime());
        recent.setModifiedAt(QDateTime::currentDateTime());
        recent.setColor(QColor("#9b59b6")); // 紫色
        recent.setIconPath(":/icons/clock.svg");
        recent.setIsSystemPlaylist(true);
        recent.setSortOrder(1);
        
        int recentId = m_playlistDao->addPlaylist(recent);
        if (recentId > 0) {
            // Logger::instance().logInfo("PlaylistManager::createDefaultPlaylists", 
            //     "创建默认播放列表: 最近播放");
            qDebug() << "PlaylistManager::createDefaultPlaylists: 创建默认播放列表: 最近播放";
        }
    }
}

int PlaylistManager::getNextSortOrder() const
{
    if (!m_playlistDao) {
        return 0;
    }
    
    QList<Playlist> playlists = m_playlistDao->getAllPlaylists();
    int maxSortOrder = -1;
    
    for (const Playlist& playlist : playlists) {
        if (playlist.sortOrder() > maxSortOrder) {
            maxSortOrder = playlist.sortOrder();
        }
    }
    
    return maxSortOrder + 1;
}

void PlaylistManager::generateShuffledIndices()
{
    m_shuffledIndices.clear();
    
    if (m_currentPlaylistSongs.isEmpty()) {
        return;
    }
    
    // 生成索引列表
    for (int i = 0; i < m_currentPlaylistSongs.size(); ++i) {
        m_shuffledIndices.append(i);
    }
    
    // 随机打乱
    std::random_shuffle(m_shuffledIndices.begin(), m_shuffledIndices.end());
    
    // Logger::instance().logInfo("PlaylistManager::generateShuffledIndices", 
    //     QString("生成随机播放索引，歌曲数量: %1").arg(m_shuffledIndices.size()));
    qDebug() << "PlaylistManager::generateShuffledIndices: 生成随机播放索引，歌曲数量:" << m_shuffledIndices.size();
}

int PlaylistManager::getNextSongIndex() const
{
    if (m_currentPlaylistSongs.isEmpty()) {
        return -1;
    }
    
    if (m_repeatMode == RepeatMode::RepeatOne) {
        return m_currentSongIndex;
    }
    
    int nextIndex = -1;
    
    if (m_shuffleMode) {
        // 随机播放模式
        if (!m_shuffledIndices.isEmpty()) {
            int currentShuffledIndex = m_shuffledIndices.indexOf(m_currentSongIndex);
            if (currentShuffledIndex >= 0 && currentShuffledIndex < m_shuffledIndices.size() - 1) {
                nextIndex = m_shuffledIndices.at(currentShuffledIndex + 1);
            } else if (m_repeatMode == RepeatMode::RepeatAll) {
                nextIndex = m_shuffledIndices.first();
            }
        }
    } else {
        // 顺序播放模式
        if (m_currentSongIndex < m_currentPlaylistSongs.size() - 1) {
            nextIndex = m_currentSongIndex + 1;
        } else if (m_repeatMode == RepeatMode::RepeatAll) {
            nextIndex = 0;
        }
    }
    
    return nextIndex;
}

int PlaylistManager::getPreviousSongIndex() const
{
    if (m_currentPlaylistSongs.isEmpty()) {
        return -1;
    }
    
    if (m_repeatMode == RepeatMode::RepeatOne) {
        return m_currentSongIndex;
    }
    
    int prevIndex = -1;
    
    if (m_shuffleMode) {
        // 随机播放模式
        if (!m_shuffledIndices.isEmpty()) {
            int currentShuffledIndex = m_shuffledIndices.indexOf(m_currentSongIndex);
            if (currentShuffledIndex > 0) {
                prevIndex = m_shuffledIndices.at(currentShuffledIndex - 1);
            } else if (m_repeatMode == RepeatMode::RepeatAll) {
                prevIndex = m_shuffledIndices.last();
            }
        }
    } else {
        // 顺序播放模式
        if (m_currentSongIndex > 0) {
            prevIndex = m_currentSongIndex - 1;
        } else if (m_repeatMode == RepeatMode::RepeatAll) {
            prevIndex = m_currentPlaylistSongs.size() - 1;
        }
    }
    
    return prevIndex;
}

bool PlaylistManager::exportToM3U(const Playlist& playlist, const QList<Song>& songs, const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        // Logger::instance().logError("PlaylistManager::exportToM3U", 
        //     QString("无法创建文件: %1").arg(filePath));
        qDebug() << "PlaylistManager::exportToM3U: 无法创建文件:" << filePath;
        return false;
    }
    
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    
    // 写入M3U头部
    out << "#EXTM3U" << Qt::endl;
    out << QString("#PLAYLIST:%1").arg(playlist.name()) << Qt::endl;
    
    // 写入歌曲信息
    for (const Song& song : songs) {
        out << QString("#EXTINF:%1,%2 - %3")
               .arg(song.duration() / 1000)
               .arg(song.artist())
               .arg(song.title()) << Qt::endl;
        out << song.filePath() << Qt::endl;
    }
    
    file.close();
    return true;
}

bool PlaylistManager::exportToPLS(const Playlist& playlist, const QList<Song>& songs, const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        // Logger::instance().logError("PlaylistManager::exportToPLS", 
        //     QString("无法创建文件: %1").arg(filePath));
        qDebug() << "PlaylistManager::exportToPLS: 无法创建文件:" << filePath;
        return false;
    }
    
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    
    // 写入PLS头部
    out << "[playlist]" << Qt::endl;
    out << QString("PlaylistName=%1").arg(playlist.name()) << Qt::endl;
    out << QString("NumberOfEntries=%1").arg(songs.size()) << Qt::endl;
    
    // 写入歌曲信息
    for (int i = 0; i < songs.size(); ++i) {
        const Song& song = songs.at(i);
        int index = i + 1;
        
        out << QString("File%1=%2").arg(index).arg(song.filePath()) << Qt::endl;
        out << QString("Title%1=%2 - %3").arg(index).arg(song.artist()).arg(song.title()) << Qt::endl;
        out << QString("Length%1=%2").arg(index).arg(song.duration() / 1000) << Qt::endl;
    }
    
    out << "Version=2" << Qt::endl;
    
    file.close();
    return true;
}

bool PlaylistManager::exportToJSON(const Playlist& playlist, const QList<Song>& songs, const QString& filePath)
{
    QJsonObject playlistObj;
    playlistObj["name"] = playlist.name();
    playlistObj["description"] = playlist.description();
    playlistObj["created_at"] = playlist.createdAt().toString(Qt::ISODate);
    playlistObj["song_count"] = songs.size();
    
    QJsonArray songsArray;
    for (const Song& song : songs) {
        QJsonObject songObj;
        songObj["title"] = song.title();
        songObj["artist"] = song.artist();
        songObj["album"] = song.album();
        songObj["duration"] = static_cast<qint64>(song.duration());
        songObj["file_path"] = song.filePath();
        songsArray.append(songObj);
    }
    
    playlistObj["songs"] = songsArray;
    
    QJsonDocument doc(playlistObj);
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        // Logger::instance().logError("PlaylistManager::exportToJSON", 
        //     QString("无法创建文件: %1").arg(filePath));
        qDebug() << "PlaylistManager::exportToJSON: 无法创建文件:" << filePath;
        return false;
    }
    
    file.write(doc.toJson());
    file.close();
    
    return true;
}

bool PlaylistManager::importFromM3U(const QString& filePath, const QString& playlistName)
{
    // M3U导入实现
    // 这里简化实现，实际应该解析M3U文件格式
    Q_UNUSED(playlistName)
    // Logger::instance().logInfo("PlaylistManager::importFromM3U", 
    //     QString("导入M3U播放列表: %1").arg(filePath));
    qDebug() << "PlaylistManager::importFromM3U: 导入M3U播放列表:" << filePath;
    return true;
}

bool PlaylistManager::importFromPLS(const QString& filePath, const QString& playlistName)
{
    // PLS导入实现
    // 这里简化实现，实际应该解析PLS文件格式
    Q_UNUSED(playlistName)
    // Logger::instance().logInfo("PlaylistManager::importFromPLS", 
    //     QString("导入PLS播放列表: %1").arg(filePath));
    qDebug() << "PlaylistManager::importFromPLS: 导入PLS播放列表:" << filePath;
    return true;
}

bool PlaylistManager::importFromJSON(const QString& filePath, const QString& playlistName)
{
    // JSON导入实现
    // 这里简化实现，实际应该解析JSON文件格式
    Q_UNUSED(playlistName)
    // Logger::instance().logInfo("PlaylistManager::importFromJSON", 
    //     QString("导入JSON播放列表: %1").arg(filePath));
    qDebug() << "PlaylistManager::importFromJSON: 导入JSON播放列表:" << filePath;
    return true;
}

// 私有槽函数实现
void PlaylistManager::onPlaylistChanged(int playlistId)
{
    qDebug() << "PlaylistManager::onPlaylistChanged: 播放列表变化:" << playlistId;
    
    // 如果是当前播放列表发生变化，重新加载
    if (playlistId == m_currentPlaylistId) {
        loadPlaylist(playlistId);
    }
    
    // 清除相关缓存
    if (m_cacheEnabled) {
        QMutexLocker locker(&m_cacheMutex);
        m_playlistCache.remove(playlistId);
        m_songCache.remove(playlistId);
    }
    
    // 更新统计信息
    updateStatistics();
}

void PlaylistManager::onSongChanged(int songId)
{
    qDebug() << "PlaylistManager::onSongChanged: 歌曲变化:" << songId;
    
    // 如果当前播放的歌曲发生变化，更新当前歌曲信息
    if (hasCurrentSong()) {
        Song currentSong = getCurrentSong();
        if (currentSong.id() == songId) {
            // 重新加载当前播放列表以获取最新的歌曲信息
            if (hasCurrentPlaylist()) {
                loadPlaylist(m_currentPlaylistId);
            }
        }
    }
    
    // 清除相关缓存
    if (m_cacheEnabled) {
        QMutexLocker locker(&m_cacheMutex);
        // 清除所有包含该歌曲的播放列表缓存
        for (auto it = m_songCache.begin(); it != m_songCache.end(); ++it) {
            const QList<Song>& songs = it.value();
            for (const Song& song : songs) {
                if (song.id() == songId) {
                    m_songCache.remove(it.key());
                    break;
                }
            }
        }
    }
}

void PlaylistManager::updateStatistics()
{
    qDebug() << "PlaylistManager::updateStatistics: 更新统计信息";
    
    try {
        QMutexLocker locker(&m_mutex);
        
        // 重置统计信息
        m_statistics = PlayStatistics();
        
        // 计算播放列表总数
        m_statistics.totalPlaylists = m_playlists.size();
        
        // 计算歌曲总数和播放时间
        int totalSongs = 0;
        int totalPlayTime = 0;
        int longestPlaylist = 0;
        int shortestPlaylist = INT_MAX;
        
        for (const Playlist& playlist : m_playlists) {
            QList<Song> songs = getPlaylistSongs(playlist.id());
            int songCount = songs.size();
            totalSongs += songCount;
            
            // 计算播放列表长度统计
            if (songCount > longestPlaylist) {
                longestPlaylist = songCount;
                m_statistics.mostPlayedPlaylist = playlist.name();
            }
            if (songCount < shortestPlaylist && songCount > 0) {
                shortestPlaylist = songCount;
            }
            
            // 计算播放时间
            for (const Song& song : songs) {
                totalPlayTime += song.duration();
            }
        }
        
        m_statistics.totalSongs = totalSongs;
        m_statistics.totalPlayTime = totalPlayTime;
        m_statistics.longestPlaylist = longestPlaylist;
        m_statistics.shortestPlaylist = (shortestPlaylist == INT_MAX) ? 0 : shortestPlaylist;
        m_statistics.averagePlaylistLength = m_statistics.totalPlaylists > 0 ? 
            totalSongs / m_statistics.totalPlaylists : 0;
        
        // 设置最近播放的播放列表
        if (hasCurrentPlaylist()) {
            m_statistics.recentPlaylist = m_currentPlaylist.name();
        }
        
        // 发射统计信息更新信号
        emit statisticsUpdated(m_statistics);
        
    } catch (const std::exception& e) {
        qCritical() << "PlaylistManager::updateStatistics: 异常:" << e.what();
    } catch (...) {
        qCritical() << "PlaylistManager::updateStatistics: 未知异常";
    }
}

void PlaylistManager::cleanupEmptyPlaylists()
{
    qDebug() << "PlaylistManager::cleanupEmptyPlaylists: 清理空播放列表";
    
    try {
        QMutexLocker locker(&m_mutex);
        
        QList<int> emptyPlaylistIds;
        
        // 查找空的播放列表
        for (const Playlist& playlist : m_playlists) {
            if (getPlaylistSongCount(playlist.id()) == 0) {
                // 不删除当前播放列表和收藏的播放列表
                if (playlist.id() != m_currentPlaylistId && !isFavorite(playlist.id())) {
                    emptyPlaylistIds.append(playlist.id());
                }
            }
        }
        
        // 删除空的播放列表
        for (int playlistId : emptyPlaylistIds) {
            qDebug() << "PlaylistManager::cleanupEmptyPlaylists: 删除空播放列表:" << playlistId;
            deletePlaylist(playlistId);
        }
        
        if (!emptyPlaylistIds.isEmpty()) {
            qDebug() << "PlaylistManager::cleanupEmptyPlaylists: 清理了" << emptyPlaylistIds.size() << "个空播放列表";
        }
        
    } catch (const std::exception& e) {
        qCritical() << "PlaylistManager::cleanupEmptyPlaylists: 异常:" << e.what();
    } catch (...) {
        qCritical() << "PlaylistManager::cleanupEmptyPlaylists: 未知异常";
    }
}

void PlaylistManager::handlePlaybackFinished()
{
    qDebug() << "PlaylistManager::handlePlaybackFinished: 播放完成";
    
    try {
        // 根据重复模式决定下一步操作
        switch (m_repeatMode) {
        case RepeatMode::RepeatOne:
            // 单曲循环，重新播放当前歌曲
            if (hasCurrentSong()) {
                qDebug() << "PlaylistManager::handlePlaybackFinished: 单曲循环，重新播放";
                play();
            }
            break;
            
        case RepeatMode::RepeatAll:
            // 列表循环，播放下一首
            qDebug() << "PlaylistManager::handlePlaybackFinished: 列表循环，播放下一首";
            next();
            break;
            
        case RepeatMode::NoRepeat:
        default:
            // 不重复，检查是否还有下一首
            if (m_currentIndex < m_currentPlaylistSongs.size() - 1) {
                qDebug() << "PlaylistManager::handlePlaybackFinished: 播放下一首";
                next();
            } else {
                // 播放列表结束
                qDebug() << "PlaylistManager::handlePlaybackFinished: 播放列表结束";
                stop();
                emit playbackStopped();
            }
            break;
        }
        
    } catch (const std::exception& e) {
        qCritical() << "PlaylistManager::handlePlaybackFinished: 异常:" << e.what();
    } catch (...) {
        qCritical() << "PlaylistManager::handlePlaybackFinished: 未知异常";
    }
}

void PlaylistManager::handleQueueNext()
{
    qDebug() << "PlaylistManager::handleQueueNext: 处理队列中的下一首";
    
    try {
        if (!m_playQueue.isEmpty()) {
            QueueItem nextItem = m_playQueue.dequeue();
            qDebug() << "PlaylistManager::handleQueueNext: 播放队列中的歌曲:" << nextItem.song.title();
            
            // 播放队列中的歌曲
            playSong(nextItem.song);
            
            // 添加到播放历史
            addToHistory(nextItem.song);
            
            // 发射队列变化信号
            emit queueChanged();
        } else {
            qDebug() << "PlaylistManager::handleQueueNext: 队列为空，继续正常播放";
            // 队列为空，继续正常的播放流程
            handlePlaybackFinished();
        }
        
    } catch (const std::exception& e) {
        qCritical() << "PlaylistManager::handleQueueNext: 异常:" << e.what();
    } catch (...) {
        qCritical() << "PlaylistManager::handleQueueNext: 未知异常";
    }
}

void PlaylistManager::addToHistory(const Song& song)
{
    qDebug() << "PlaylistManager::addToHistory called with song:" << song.title();
    
    QMutexLocker locker(&m_mutex);
    
    // 移除历史中已存在的相同歌曲
    m_playHistory.removeAll(song);
    
    // 添加到历史开头
    m_playHistory.prepend(song);
    
    // 限制历史大小
    while (m_playHistory.size() > m_maxHistorySize) {
        m_playHistory.removeLast();
    }
    
    emit historyChanged();
    emit songAddedToHistory(song);
}

bool PlaylistManager::isFavorite(int playlistId) const
{
    qDebug() << "PlaylistManager::isFavorite called with playlistId:" << playlistId;
    
    QMutexLocker locker(&m_mutex);
    return m_favoritePlaylistIds.contains(playlistId);
}

QList<Song> PlaylistManager::getHistory() const
{
    qDebug() << "PlaylistManager::getHistory called";
    
    QMutexLocker locker(&m_mutex);
    return m_playHistory;
}

void PlaylistManager::clearHistory()
{
    qDebug() << "PlaylistManager::clearHistory called";
    
    QMutexLocker locker(&m_mutex);
    m_playHistory.clear();
    
    emit historyCleared();
    emit historyChanged();
}

void PlaylistManager::setHistorySize(int size)
{
    qDebug() << "PlaylistManager::setHistorySize called with size:" << size;
    
    QMutexLocker locker(&m_mutex);
    m_maxHistorySize = qMax(1, size); // 确保至少为1
    
    // 如果当前历史超过新的大小限制，则截断
    while (m_playHistory.size() > m_maxHistorySize) {
        m_playHistory.removeLast();
    }
    
    emit historyChanged();
}

int PlaylistManager::getHistorySize() const
{
    qDebug() << "PlaylistManager::getHistorySize called";
    
    QMutexLocker locker(&m_mutex);
    return m_maxHistorySize;
}

void PlaylistManager::addToFavorites(int playlistId)
{
    qDebug() << "PlaylistManager::addToFavorites called with playlistId:" << playlistId;
    
    QMutexLocker locker(&m_mutex);
    
    if (!validatePlaylistId(playlistId)) {
        qWarning() << "PlaylistManager::addToFavorites: Invalid playlist ID:" << playlistId;
        return;
    }
    
    m_favoritePlaylistIds.insert(playlistId);
    
    // TODO: 保存到数据库
    qDebug() << "Playlist" << playlistId << "added to favorites";
}

void PlaylistManager::removeFromFavorites(int playlistId)
{
    qDebug() << "PlaylistManager::removeFromFavorites called with playlistId:" << playlistId;
    
    QMutexLocker locker(&m_mutex);
    
    if (m_favoritePlaylistIds.remove(playlistId)) {
        // TODO: 从数据库中移除
        qDebug() << "Playlist" << playlistId << "removed from favorites";
    } else {
        qWarning() << "Playlist" << playlistId << "was not in favorites";
    }
}

QList<int> PlaylistManager::getFavoritePlaylistIds() const
{
    qDebug() << "PlaylistManager::getFavoritePlaylistIds called";
    
    QMutexLocker locker(&m_mutex);
    return m_favoritePlaylistIds.values();
}

bool PlaylistManager::playlistExists(const QString& name) const
{
    qDebug() << "PlaylistManager::playlistExists called with name:" << name;
    
    QMutexLocker locker(&m_mutex);
    
    for (const auto& playlist : m_playlists) {
        if (playlist.name() == name) {
            return true;
        }
    }
    
    return false;
}

bool PlaylistManager::playlistExists(int playlistId) const
{
    qDebug() << "PlaylistManager::playlistExists called with playlistId:" << playlistId;
    
    QMutexLocker locker(&m_mutex);
    
    for (const auto& playlist : m_playlists) {
        if (playlist.id() == playlistId) {
            return true;
        }
    }
    
    return false;
}

bool PlaylistManager::canDeletePlaylist(int playlistId) const
{
    qDebug() << "PlaylistManager::canDeletePlaylist called with playlistId:" << playlistId;
    
    QMutexLocker locker(&m_mutex);
    
    // 检查播放列表是否存在
    if (!playlistExists(playlistId)) {
        return false;
    }
    
    // 检查是否为当前播放列表（可能需要特殊处理）
    if (playlistId == m_currentPlaylistId) {
        qDebug() << "Cannot delete current playlist";
        return false;
    }
    
    // 其他业务逻辑检查可以在这里添加
    return true;
}

bool PlaylistManager::canUpdatePlaylist(int playlistId) const
{
    qDebug() << "PlaylistManager::canUpdatePlaylist called with playlistId:" << playlistId;
    
    QMutexLocker locker(&m_mutex);
    
    // 检查播放列表是否存在
    return playlistExists(playlistId);
}

bool PlaylistManager::validatePlaylistId(int playlistId) const
{
    qDebug() << "PlaylistManager::validatePlaylistId called with playlistId:" << playlistId;
    
    // 检查ID是否有效
    if (playlistId <= 0) {
        qWarning() << "PlaylistManager::validatePlaylistId: Invalid playlist ID:" << playlistId;
        return false;
    }
    
    // 检查播放列表是否存在
    return playlistExists(playlistId);
}

bool PlaylistManager::validatePlaylistName(const QString& name) const
{
    qDebug() << "PlaylistManager::validatePlaylistName called with name:" << name;
    
    // 检查名称是否为空
    if (name.trimmed().isEmpty()) {
        qWarning() << "PlaylistManager::validatePlaylistName: Empty playlist name";
        return false;
    }
    
    // 检查名称长度
    if (name.length() > 255) {
        qWarning() << "PlaylistManager::validatePlaylistName: Playlist name too long:" << name.length();
        return false;
    }
    
    // 检查是否包含非法字符
    const QString invalidChars = "<>:\"/|?*";
    for (const QChar& ch : invalidChars) {
        if (name.contains(ch)) {
            qWarning() << "PlaylistManager::validatePlaylistName: Invalid character in name:" << ch;
            return false;
        }
    }
    
    return true;
}

bool PlaylistManager::validateSongIndex(int playlistId, int index) const
{
    qDebug() << "PlaylistManager::validateSongIndex called with playlistId:" << playlistId << "index:" << index;
    
    // 检查播放列表是否有效
    if (!validatePlaylistId(playlistId)) {
        return false;
    }
    
    // 检查索引是否有效
    if (index < 0) {
        qWarning() << "PlaylistManager::validateSongIndex: Negative index:" << index;
        return false;
    }
    
    // 获取播放列表歌曲数量
    int songCount = getPlaylistSongCount(playlistId);
    if (index >= songCount) {
        qWarning() << "PlaylistManager::validateSongIndex: Index out of range:" << index << "(max:" << songCount - 1 << ")";
        return false;
    }
    
    return true;
}