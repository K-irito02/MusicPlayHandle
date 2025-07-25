#include "song.h"
#include <QFileInfo>
#include <QDateTime>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QBuffer>
#include <QImage>
#include <QImageReader>
#include <QDir>
#include <QStandardPaths>
#include <QApplication>

// FFmpeg相关头文件
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/dict.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

Song::Song()
    : m_id(0)
    , m_duration(0)
    , m_fileSize(0)
    , m_bitRate(0)
    , m_sampleRate(0)
    , m_channels(2)
    , m_hasLyrics(false)
    , m_playCount(0)
    , m_isFavorite(false)
    , m_isAvailable(true)
    , m_createdAt(QDateTime::currentDateTime())
    , m_updatedAt(QDateTime::currentDateTime())
    , m_rating(0)
{
}

Song::Song(const QString& filePath, const QString& title, 
           const QString& artist, const QString& album)
    : m_id(0)
    , m_filePath(filePath)
    , m_fileName(extractFileName(filePath))
    , m_title(title)
    , m_artist(artist)
    , m_album(album)
    , m_duration(0)
    , m_fileSize(0)
    , m_bitRate(0)
    , m_sampleRate(0)
    , m_channels(2)
    , m_hasLyrics(false)
    , m_playCount(0)
    , m_isFavorite(false)
    , m_isAvailable(true)
    , m_createdAt(QDateTime::currentDateTime())
    , m_updatedAt(QDateTime::currentDateTime())
    , m_rating(0)
{
    // 从文件路径获取文件格式
    QFileInfo fileInfo(filePath);
    m_fileFormat = fileInfo.suffix().toLower();
    
    // 获取文件大小
    m_fileSize = fileInfo.size();
    
    // 获取文件修改时间
    m_dateModified = fileInfo.lastModified();
    
    // 设置添加时间
    m_dateAdded = QDateTime::currentDateTime();
}

Song::Song(const Song& other)
    : m_id(other.m_id)
    , m_filePath(other.m_filePath)
    , m_fileName(other.m_fileName)
    , m_title(other.m_title)
    , m_artist(other.m_artist)
    , m_album(other.m_album)
    , m_duration(other.m_duration)
    , m_fileSize(other.m_fileSize)
    , m_bitRate(other.m_bitRate)
    , m_sampleRate(other.m_sampleRate)
    , m_channels(other.m_channels)
    , m_fileFormat(other.m_fileFormat)
    , m_coverPath(other.m_coverPath)
    , m_hasLyrics(other.m_hasLyrics)
    , m_lyricsPath(other.m_lyricsPath)
    , m_playCount(other.m_playCount)
    , m_lastPlayedTime(other.m_lastPlayedTime)
    , m_dateAdded(other.m_dateAdded)
    , m_dateModified(other.m_dateModified)
    , m_isFavorite(other.m_isFavorite)
    , m_isAvailable(other.m_isAvailable)
    , m_createdAt(other.m_createdAt)
    , m_updatedAt(other.m_updatedAt)
    , m_tags(other.m_tags)
    , m_rating(other.m_rating)
{
}

Song& Song::operator=(const Song& other)
{
    if (this != &other) {
        m_id = other.m_id;
        m_filePath = other.m_filePath;
        m_fileName = other.m_fileName;
        m_title = other.m_title;
        m_artist = other.m_artist;
        m_album = other.m_album;
        m_duration = other.m_duration;
        m_fileSize = other.m_fileSize;
        m_bitRate = other.m_bitRate;
        m_sampleRate = other.m_sampleRate;
        m_channels = other.m_channels;
        m_fileFormat = other.m_fileFormat;
        m_coverPath = other.m_coverPath;
        m_hasLyrics = other.m_hasLyrics;
        m_lyricsPath = other.m_lyricsPath;
        m_playCount = other.m_playCount;
        m_lastPlayedTime = other.m_lastPlayedTime;
        m_dateAdded = other.m_dateAdded;
        m_dateModified = other.m_dateModified;
        m_isFavorite = other.m_isFavorite;
        m_isAvailable = other.m_isAvailable;
        m_createdAt = other.m_createdAt;
        m_updatedAt = other.m_updatedAt;
        m_tags = other.m_tags;
        m_rating = other.m_rating;
    }
    return *this;
}

bool Song::operator==(const Song& other) const
{
    return m_id == other.m_id && m_filePath == other.m_filePath;
}

bool Song::operator!=(const Song& other) const
{
    return !(*this == other);
}

void Song::setFilePath(const QString& filePath)
{
    m_filePath = filePath;
    m_fileName = extractFileName(filePath);
    
    // 更新文件格式
    QFileInfo fileInfo(filePath);
    m_fileFormat = fileInfo.suffix().toLower();
    
    // 更新文件大小
    m_fileSize = fileInfo.size();
    
    // 更新修改时间
    m_dateModified = fileInfo.lastModified();
    
    // 更新可用性
    m_isAvailable = fileInfo.exists();
}

bool Song::isValid() const
{
    return !m_filePath.isEmpty() && m_isAvailable;
}

QString Song::displayName() const
{
    // 直接返回文件名，不显示艺术家和标题信息
    return m_fileName;
}

QString Song::formattedDuration() const
{
    return formatTime(m_duration);
}

QString Song::formattedFileSize() const
{
    return formatFileSize(m_fileSize);
}

void Song::updatePlayCount()
{
    m_playCount++;
    m_lastPlayedTime = QDateTime::currentDateTime();
    m_updatedAt = QDateTime::currentDateTime();
}

void Song::setFavorite()
{
    m_isFavorite = true;
    m_updatedAt = QDateTime::currentDateTime();
}

void Song::unsetFavorite()
{
    m_isFavorite = false;
    m_updatedAt = QDateTime::currentDateTime();
}

void Song::toggleFavorite()
{
    m_isFavorite = !m_isFavorite;
    m_updatedAt = QDateTime::currentDateTime();
}

Song Song::fromJson(const QJsonObject& json)
{
    Song song;
    song.m_id = json["id"].toInt();
    song.m_filePath = json["filePath"].toString();
    song.m_fileName = json["fileName"].toString();
    song.m_title = json["title"].toString();
    song.m_artist = json["artist"].toString();
    song.m_album = json["album"].toString();
    song.m_duration = json["duration"].toDouble();
    song.m_fileSize = json["fileSize"].toDouble();
    song.m_bitRate = json["bitRate"].toInt();
    song.m_sampleRate = json["sampleRate"].toInt();
    song.m_channels = json["channels"].toInt();
    song.m_fileFormat = json["fileFormat"].toString();
    song.m_coverPath = json["coverPath"].toString();
    song.m_hasLyrics = json["hasLyrics"].toBool();
    song.m_lyricsPath = json["lyricsPath"].toString();
    song.m_playCount = json["playCount"].toInt();
    song.m_lastPlayedTime = QDateTime::fromString(json["lastPlayedTime"].toString(), Qt::ISODate);
    song.m_dateAdded = QDateTime::fromString(json["dateAdded"].toString(), Qt::ISODate);
    song.m_dateModified = QDateTime::fromString(json["dateModified"].toString(), Qt::ISODate);
    song.m_isFavorite = json["isFavorite"].toBool();
    song.m_isAvailable = json["isAvailable"].toBool();
    song.m_createdAt = QDateTime::fromString(json["createdAt"].toString(), Qt::ISODate);
    song.m_updatedAt = QDateTime::fromString(json["updatedAt"].toString(), Qt::ISODate);
    
    return song;
}

QJsonObject Song::toJson() const
{
    QJsonObject json;
    json["id"] = m_id;
    json["filePath"] = m_filePath;
    json["fileName"] = m_fileName;
    json["title"] = m_title;
    json["artist"] = m_artist;
    json["album"] = m_album;
    json["duration"] = static_cast<double>(m_duration);
    json["fileSize"] = static_cast<double>(m_fileSize);
    json["bitRate"] = m_bitRate;
    json["sampleRate"] = m_sampleRate;
    json["channels"] = m_channels;
    json["fileFormat"] = m_fileFormat;
    json["coverPath"] = m_coverPath;
    json["hasLyrics"] = m_hasLyrics;
    json["lyricsPath"] = m_lyricsPath;
    json["playCount"] = m_playCount;
    json["lastPlayedTime"] = m_lastPlayedTime.toString(Qt::ISODate);
    json["dateAdded"] = m_dateAdded.toString(Qt::ISODate);
    json["dateModified"] = m_dateModified.toString(Qt::ISODate);
    json["isFavorite"] = m_isFavorite;
    json["isAvailable"] = m_isAvailable;
    json["createdAt"] = m_createdAt.toString(Qt::ISODate);
    json["updatedAt"] = m_updatedAt.toString(Qt::ISODate);
    
    return json;
}

Song Song::fromVariantMap(const QVariantMap& map)
{
    Song song;
    song.m_id = map["id"].toInt();
    song.m_filePath = map["filePath"].toString();
    song.m_fileName = map["fileName"].toString();
    song.m_title = map["title"].toString();
    song.m_artist = map["artist"].toString();
    song.m_album = map["album"].toString();
    song.m_duration = map["duration"].toLongLong();
    song.m_fileSize = map["fileSize"].toLongLong();
    song.m_bitRate = map["bitRate"].toInt();
    song.m_sampleRate = map["sampleRate"].toInt();
    song.m_channels = map["channels"].toInt();
    song.m_fileFormat = map["fileFormat"].toString();
    song.m_coverPath = map["coverPath"].toString();
    song.m_hasLyrics = map["hasLyrics"].toBool();
    song.m_lyricsPath = map["lyricsPath"].toString();
    song.m_playCount = map["playCount"].toInt();
    song.m_lastPlayedTime = map["lastPlayedTime"].toDateTime();
    song.m_dateAdded = map["dateAdded"].toDateTime();
    song.m_dateModified = map["dateModified"].toDateTime();
    song.m_isFavorite = map["isFavorite"].toBool();
    song.m_isAvailable = map["isAvailable"].toBool();
    song.m_createdAt = map["createdAt"].toDateTime();
    song.m_updatedAt = map["updatedAt"].toDateTime();
    
    return song;
}

QVariantMap Song::toVariantMap() const
{
    QVariantMap map;
    map["id"] = m_id;
    map["filePath"] = m_filePath;
    map["fileName"] = m_fileName;
    map["title"] = m_title;
    map["artist"] = m_artist;
    map["album"] = m_album;
    map["duration"] = m_duration;
    map["fileSize"] = m_fileSize;
    map["bitRate"] = m_bitRate;
    map["sampleRate"] = m_sampleRate;
    map["channels"] = m_channels;
    map["fileFormat"] = m_fileFormat;
    map["coverPath"] = m_coverPath;
    map["hasLyrics"] = m_hasLyrics;
    map["lyricsPath"] = m_lyricsPath;
    map["playCount"] = m_playCount;
    map["lastPlayedTime"] = m_lastPlayedTime;
    map["dateAdded"] = m_dateAdded;
    map["dateModified"] = m_dateModified;
    map["isFavorite"] = m_isFavorite;
    map["isAvailable"] = m_isAvailable;
    map["createdAt"] = m_createdAt;
    map["updatedAt"] = m_updatedAt;
    
    return map;
}

void Song::clear()
{
    m_id = 0;
    m_filePath.clear();
    m_fileName.clear();
    m_title.clear();
    m_artist.clear();
    m_album.clear();
    m_duration = 0;
    m_fileSize = 0;
    m_bitRate = 0;
    m_sampleRate = 0;
    m_channels = 2;
    m_fileFormat.clear();
    m_coverPath.clear();
    m_hasLyrics = false;
    m_lyricsPath.clear();
    m_playCount = 0;
    m_lastPlayedTime = QDateTime();
    m_dateAdded = QDateTime();
    m_dateModified = QDateTime();
    m_isFavorite = false;
    m_isAvailable = true;
    m_createdAt = QDateTime::currentDateTime();
    m_updatedAt = QDateTime::currentDateTime();
    m_tags.clear();
    m_rating = 0;
}

bool Song::isEmpty() const
{
    return m_filePath.isEmpty();
}

QString Song::toString() const
{
    return QString("Song(id=%1, title=%2, artist=%3, filePath=%4)")
           .arg(m_id)
           .arg(m_title)
           .arg(m_artist)
           .arg(m_filePath);
}

QString Song::extractFileName(const QString& filePath) const
{
    QFileInfo fileInfo(filePath);
    return fileInfo.fileName();
}

QString Song::formatTime(qint64 milliseconds) const
{
    if (milliseconds <= 0) {
        return "00:00";
    }
    
    int totalSeconds = milliseconds / 1000;
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int seconds = totalSeconds % 60;
    
    if (hours > 0) {
        return QString("%1:%2:%3")
               .arg(hours)
               .arg(minutes, 2, 10, QChar('0'))
               .arg(seconds, 2, 10, QChar('0'));
    } else {
        return QString("%1:%2")
               .arg(minutes)
               .arg(seconds, 2, 10, QChar('0'));
    }
}

QString Song::formatFileSize(qint64 bytes) const
{
    if (bytes <= 0) {
        return "0 B";
    }
    
    const QStringList units = {"B", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    double size = bytes;
    
    while (size >= 1024 && unitIndex < units.size() - 1) {
        size /= 1024;
        unitIndex++;
    }
    
    return QString("%1 %2")
           .arg(size, 0, 'f', unitIndex == 0 ? 0 : 1)
           .arg(units[unitIndex]);
}

void Song::extractBasicMetadata(Song& song, const QString& filePath)
{
    QFileInfo fileInfo(filePath);
    QString baseName = fileInfo.baseName(); // 不包含扩展名的文件名
    
    // 尝试从文件名解析 "艺术家 - 标题" 格式
    if (baseName.contains(" - ")) {
        QStringList parts = baseName.split(" - ", Qt::SkipEmptyParts);
        if (parts.size() >= 2) {
            song.setArtist(parts[0].trimmed());
            song.setTitle(parts[1].trimmed());
        } else {
            // 如果分割失败，使用文件名作为标题
            song.setTitle(baseName);
            song.setArtist("");
        }
    } else {
        // 没有分隔符，使用整个文件名作为标题
        song.setTitle(baseName);
        song.setArtist("");
    }
    
    // 设置默认专辑名
    if (song.album().isEmpty()) {
        song.setAlbum("未知专辑");
    }
    
    // 确保title不为空（数据库约束要求）
    if (song.title().isEmpty()) {
        song.setTitle(fileInfo.fileName()); // 使用完整文件名作为后备
    }
}

Song Song::fromFile(const QString& filePath)
{
    Song song;
    song.setFilePath(filePath);
    QFileInfo fileInfo(filePath);
    song.setFileName(fileInfo.fileName());
    song.setFileFormat(fileInfo.suffix().toLower());
    song.setFileSize(fileInfo.size());
    song.setDateAdded(QDateTime::currentDateTime());
    song.setDateModified(fileInfo.lastModified());
    song.setIsAvailable(fileInfo.exists());
    
    // 使用FFmpeg高级元数据提取
    extractAdvancedMetadata(song, filePath);
    
    return song;
}

void Song::extractAdvancedMetadata(Song& song, const QString& filePath)
{
    // 首先尝试使用FFmpeg解析元数据
    if (extractFFmpegMetadata(song, filePath)) {
        return; // 如果FFmpeg解析成功，直接返回
    }
    
    // 如果FFmpeg解析失败，回退到基本解析
    extractBasicMetadata(song, filePath);
}

bool Song::extractFFmpegMetadata(Song& song, const QString& filePath)
{
    AVFormatContext* formatContext = nullptr;
    bool success = false;
    
    try {
        // 打开文件
        if (avformat_open_input(&formatContext, filePath.toUtf8().constData(), nullptr, nullptr) < 0) {
            qWarning() << "无法打开音频文件:" << filePath;
            return false;
        }
        
        // 查找流信息
        if (avformat_find_stream_info(formatContext, nullptr) < 0) {
            qWarning() << "无法获取流信息:" << filePath;
            return false;
        }
        
        // 解析元数据
        AVDictionary* metadata = formatContext->metadata;
        AVDictionaryEntry* entry = nullptr;
        
        while ((entry = av_dict_get(metadata, "", entry, AV_DICT_IGNORE_SUFFIX))) {
            QString key = QString::fromUtf8(entry->key);
            QString value = QString::fromUtf8(entry->value);
            
            if (key.toLower() == "title" && !value.isEmpty()) {
                song.setTitle(value);
            } else if (key.toLower() == "artist" && !value.isEmpty()) {
                song.setArtist(value);
            } else if (key.toLower() == "album" && !value.isEmpty()) {
                song.setAlbum(value);
            } else if (key.toLower() == "date" && !value.isEmpty()) {
                song.setYear(value);
            } else if (key.toLower() == "genre" && !value.isEmpty()) {
                song.setGenre(value);
            }
        }
        
        // 获取时长
        if (formatContext->duration > 0) {
            song.setDuration(formatContext->duration / 1000); // 转换为毫秒
        }
        
        success = true;
        
    } catch (const std::exception& e) {
        qWarning() << "FFmpeg元数据解析异常:" << e.what();
    } catch (...) {
        qWarning() << "FFmpeg元数据解析未知异常";
    }
    
    // 清理资源
    if (formatContext) {
        avformat_close_input(&formatContext);
    }
    
    return success;
}

QPixmap Song::extractCoverArt(const QString& filePath, const QSize& size)
{
    AVFormatContext* formatContext = nullptr;
    QPixmap coverPixmap;
    
    try {
        // 打开文件
        if (avformat_open_input(&formatContext, filePath.toUtf8().constData(), nullptr, nullptr) < 0) {
            return coverPixmap;
        }
        
        // 查找流信息
        if (avformat_find_stream_info(formatContext, nullptr) < 0) {
            return coverPixmap;
        }
        
        // 查找封面流
        for (unsigned int i = 0; i < formatContext->nb_streams; i++) {
            AVStream* stream = formatContext->streams[i];
            if (stream->disposition & AV_DISPOSITION_ATTACHED_PIC) {
                AVPacket* packet = &stream->attached_pic;
                
                // 解码封面数据
                QImage coverImage;
                if (coverImage.loadFromData(packet->data, packet->size)) {
                    coverPixmap = QPixmap::fromImage(coverImage);
                    
                    // 智能缩放，保持宽高比
                    if (!coverPixmap.isNull() && size.isValid()) {
                        coverPixmap = coverPixmap.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                    }
                    
                    break;
                }
            }
        }
        
        // 如果没有找到封面流，尝试从元数据中获取
        if (coverPixmap.isNull()) {
            AVDictionary* metadata = formatContext->metadata;
            AVDictionaryEntry* entry = av_dict_get(metadata, "metadata_block_picture", nullptr, 0);
            
            if (entry) {
                // 这里可以解析Base64编码的封面数据
                // 由于实现复杂，暂时跳过
            }
        }
        
    } catch (const std::exception& e) {
        qWarning() << "封面提取异常:" << e.what();
    } catch (...) {
        qWarning() << "封面提取未知异常";
    }
    
    // 清理资源
    if (formatContext) {
        avformat_close_input(&formatContext);
    }
    
    return coverPixmap;
}

QString Song::getTitleFromMetadata(const QString& filePath)
{
    Song song;
    extractAdvancedMetadata(song, filePath);
    return song.title();
}

QString Song::getArtistFromMetadata(const QString& filePath)
{
    Song song;
    extractAdvancedMetadata(song, filePath);
    return song.artist();
}

QString Song::getAlbumFromMetadata(const QString& filePath)
{
    Song song;
    extractAdvancedMetadata(song, filePath);
    return song.album();
}

QString Song::getYearFromMetadata(const QString& filePath)
{
    Song song;
    extractAdvancedMetadata(song, filePath);
    return QString::number(song.year());
}

QString Song::getGenreFromMetadata(const QString& filePath)
{
    Song song;
    extractAdvancedMetadata(song, filePath);
    return song.genre();
}