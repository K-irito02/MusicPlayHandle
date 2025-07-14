#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <QString>
#include <QStringList>
#include <QColor>

/**
 * @brief 应用程序常量定义
 * @details 集中管理所有常量，便于维护和修改
 */
namespace Constants {

    /**
     * @brief 系统标签相关常量
     */
    namespace SystemTags {
        const QString MY_SONGS = QStringLiteral("我的歌曲");
        const QString DEFAULT_TAG = QStringLiteral("默认标签");
        const QString FAVORITES = QStringLiteral("收藏");
        const QString RECENT_PLAYED = QStringLiteral("最近播放");
        
        /**
         * @brief 获取所有系统标签
         * @return 系统标签列表
         */
        inline QStringList getAll() {
            return {MY_SONGS, DEFAULT_TAG, FAVORITES, RECENT_PLAYED};
        }
        
        /**
         * @brief 检查是否为系统标签
         * @param name 标签名称
         * @return 是否为系统标签
         */
        inline bool isSystemTag(const QString& name) {
            return getAll().contains(name);
        }
    }
    
    /**
     * @brief 数据库相关常量
     */
    namespace Database {
        const QString DEFAULT_DB_NAME = QStringLiteral("musicplayer.db");
        const QString CONNECTION_NAME = QStringLiteral("main_connection");
        const int CURRENT_VERSION = 1;
        
        // 表名
        const QString TABLE_SONGS = QStringLiteral("songs");
        const QString TABLE_TAGS = QStringLiteral("tags");
        const QString TABLE_SONG_TAGS = QStringLiteral("song_tags");
        const QString TABLE_PLAYLISTS = QStringLiteral("playlists");
        const QString TABLE_PLAY_HISTORY = QStringLiteral("play_history");
        const QString TABLE_LOGS = QStringLiteral("logs");
        const QString TABLE_ERROR_LOGS = QStringLiteral("error_logs");
    }
    
    /**
     * @brief UI相关常量
     */
    namespace UI {
        // 颜色
        const QColor PRIMARY_COLOR = QColor("#2196F3");
        const QColor SECONDARY_COLOR = QColor("#FFC107");
        const QColor SUCCESS_COLOR = QColor("#4CAF50");
        const QColor WARNING_COLOR = QColor("#FF9800");
        const QColor ERROR_COLOR = QColor("#F44336");
        const QColor SYSTEM_TAG_COLOR = QColor("#9E9E9E");
        
        // 尺寸
        const int DEFAULT_ICON_SIZE = 16;
        const int LARGE_ICON_SIZE = 24;
        const int LIST_ITEM_HEIGHT = 32;
        const int DIALOG_MIN_WIDTH = 400;
        const int DIALOG_MIN_HEIGHT = 300;
        
        // 间距
        const int DEFAULT_MARGIN = 8;
        const int DEFAULT_SPACING = 6;
        const int LARGE_SPACING = 12;
    }
    
    /**
     * @brief 文件和路径相关常量
     */
    namespace Paths {
        const QString CONFIG_DIR = QStringLiteral("config");
        const QString DATA_DIR = QStringLiteral("data");
        const QString LOG_DIR = QStringLiteral("logs");
        const QString TEMP_DIR = QStringLiteral("temp");
        const QString BACKUP_DIR = QStringLiteral("backup");
        
        const QString CONFIG_FILE = QStringLiteral("config.ini");
        const QString LOG_FILE = QStringLiteral("application.log");
        const QString ERROR_LOG_FILE = QStringLiteral("error.log");
    }
    
    /**
     * @brief 音频相关常量
     */
    namespace Audio {
        const QStringList SUPPORTED_FORMATS = {
            QStringLiteral("mp3"),
            QStringLiteral("wav"),
            QStringLiteral("flac"),
            QStringLiteral("ogg"),
            QStringLiteral("m4a"),
            QStringLiteral("aac")
        };
        
        const int DEFAULT_VOLUME = 50;
        const int MAX_VOLUME = 100;
        const int MIN_VOLUME = 0;
        
        const int DEFAULT_BUFFER_SIZE = 4096;
        const int DEFAULT_SAMPLE_RATE = 44100;
    }
    
    /**
     * @brief 应用程序信息常量
     */
    namespace App {
        const QString NAME = QStringLiteral("Music Player");
        const QString VERSION = QStringLiteral("1.0.0");
        const QString ORGANIZATION = QStringLiteral("Qt Music Player");
        const QString DOMAIN = QStringLiteral("qtmusicplayer.com");
        const QString DESCRIPTION = QStringLiteral("A Qt6-based music player application");
    }
    
    /**
     * @brief 性能相关常量
     */
    namespace Performance {
        const int CACHE_SIZE_LIMIT = 100; // 缓存项目数量限制
        const int LAZY_LOAD_THRESHOLD = 50; // 延迟加载阈值
        const int BATCH_SIZE = 20; // 批处理大小
        const int CLEANUP_INTERVAL_MS = 300000; // 清理间隔（5分钟）
    }
    
    /**
     * @brief 错误代码常量
     */
    namespace ErrorCodes {
        const int SUCCESS = 0;
        const int GENERAL_ERROR = -1;
        const int DATABASE_ERROR = -100;
        const int FILE_ERROR = -200;
        const int NETWORK_ERROR = -300;
        const int AUDIO_ERROR = -400;
        const int PERMISSION_ERROR = -500;
    }
    
    /**
     * @brief 日志相关常量
     */
    namespace Logging {
        const QString CATEGORY_GENERAL = QStringLiteral("general");
        const QString CATEGORY_DATABASE = QStringLiteral("database");
        const QString CATEGORY_AUDIO = QStringLiteral("audio");
        const QString CATEGORY_UI = QStringLiteral("ui");
        const QString CATEGORY_NETWORK = QStringLiteral("network");
        
        const int MAX_LOG_FILE_SIZE = 10 * 1024 * 1024; // 10MB
        const int MAX_LOG_FILES = 5;
    }
}

#endif // CONSTANTS_H