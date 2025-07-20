#include <QCoreApplication>
#include <QDebug>
#include <QDateTime>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    qDebug() << "开始数据库排序测试...";
    
    // 连接数据库
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("music_play_handle.db");
    
    if (!db.open()) {
        qDebug() << "无法打开数据库:" << db.lastError().text();
        return -1;
    }
    
    qDebug() << "数据库连接成功";
    
    // 测试SQL查询
    const QString sql = R"(
        SELECT s.id, s.title, s.artist, s.album, s.file_path, s.duration,
               s.file_size, s.date_added, s.last_played, s.play_count, s.rating,
               s.tags, s.created_at, s.updated_at,
               ph.played_at
        FROM songs s
        INNER JOIN (
            SELECT song_id, MAX(played_at) as played_at
            FROM play_history
            GROUP BY song_id
        ) ph ON s.id = ph.song_id
        ORDER BY ph.played_at DESC
        LIMIT 10
    )";
    
    QSqlQuery query(db);
    if (!query.exec(sql)) {
        qDebug() << "SQL查询失败:" << query.lastError().text();
        return -1;
    }
    
    qDebug() << "SQL查询成功，结果:";
    int count = 0;
    while (query.next()) {
        count++;
        QString title = query.value("title").toString();
        QString artist = query.value("artist").toString();
        QDateTime playedAt = query.value("played_at").toDateTime();
        QDateTime lastPlayed = query.value("last_played").toDateTime();
        
        qDebug() << QString("  [%1] %2 - %3")
                    .arg(count)
                    .arg(artist)
                    .arg(title);
        qDebug() << QString("      played_at: %1").arg(playedAt.toString("yyyy/MM-dd/hh-mm-ss"));
        qDebug() << QString("      last_played: %1").arg(lastPlayed.toString("yyyy/MM-dd/hh-mm-ss"));
        qDebug() << "";
    }
    
    qDebug() << QString("总共获取到 %1 条记录").arg(count);
    
    return 0;
} 