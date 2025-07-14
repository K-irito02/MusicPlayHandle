#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>
#include <QMetaType>
#include <QColor>
#include <QList>

class Playlist {
public:
    Playlist();
    Playlist(const QString& name, const QString& description = QString());
    Playlist(const Playlist& other);
    Playlist& operator=(const Playlist& other);
    ~Playlist();
    
    // 基本属性
    int id() const;
    void setId(int id);
    
    QString name() const;
    void setName(const QString& name);
    
    QString description() const;
    void setDescription(const QString& description);
    
    // 时间戳
    QDateTime createdAt() const;
    void setCreatedAt(const QDateTime& createdAt);
    
    QDateTime modifiedAt() const;
    void setModifiedAt(const QDateTime& modifiedAt);
    
    QDateTime lastPlayedAt() const;
    void setLastPlayedAt(const QDateTime& lastPlayedAt);
    
    // 统计信息
    int songCount() const;
    void setSongCount(int count);
    
    qint64 totalDuration() const;
    void setTotalDuration(qint64 duration);
    
    int playCount() const;
    void setPlayCount(int count);
    void incrementPlayCount();
    
    // 外观设置
    QColor color() const;
    void setColor(const QColor& color);
    
    QString iconPath() const;
    void setIconPath(const QString& iconPath);
    
    // 智能播放列表
    bool isSmartPlaylist() const;
    void setIsSmartPlaylist(bool isSmart);
    
    QString smartCriteria() const;
    void setSmartCriteria(const QString& criteria);
    
    // 系统属性
    bool isSystemPlaylist() const;
    void setIsSystemPlaylist(bool isSystem);
    
    bool isFavorite() const;
    void setIsFavorite(bool favorite);
    
    // 排序和布局
    int sortOrder() const;
    void setSortOrder(int order);
    
    // 验证
    bool isValid() const;
    
    // 比较操作
    bool operator==(const Playlist& other) const;
    bool operator!=(const Playlist& other) const;
    bool operator<(const Playlist& other) const;
    
    // JSON序列化
    QJsonObject toJson() const;
    static Playlist fromJson(const QJsonObject& json);
    
    // 调试输出
    QString toString() const;
    
    // 清空数据
    void clear();
    
    // 克隆
    Playlist clone() const;
    
    // 属性更新
    void updateModifiedTime();
    void updateLastPlayedTime();
    
    // 格式化方法
    QString formatDuration() const;
    QString formatCreatedTime() const;
    QString formatModifiedTime() const;
    QString formatLastPlayedTime() const;
    
private:
    int m_id;
    QString m_name;
    QString m_description;
    QDateTime m_createdAt;
    QDateTime m_modifiedAt;
    QDateTime m_lastPlayedAt;
    int m_songCount;
    qint64 m_totalDuration;
    int m_playCount;
    QColor m_color;
    QString m_iconPath;
    bool m_isSmartPlaylist;
    QString m_smartCriteria;
    bool m_isSystemPlaylist;
    bool m_isFavorite;
    int m_sortOrder;
    
    // 初始化默认值
    void initializeDefaults();
    
    // 验证方法
    bool isValidName(const QString& name) const;
    bool isValidId(int id) const;
};

// 注册元类型
Q_DECLARE_METATYPE(Playlist)

#endif // PLAYLIST_H 