#ifndef VERSION_H
#define VERSION_H

// 应用程序版本信息
#define APP_VERSION "0.1.0"
#define APP_VERSION_MAJOR 0
#define APP_VERSION_MINOR 1
#define APP_VERSION_PATCH 0

// 应用程序信息
#define APP_NAME "Qt6音频播放器"
#define APP_ORGANIZATION "Qt6音频播放器开发团队"
#define APP_DOMAIN "musicPlayHandle.qt6.org"
#define APP_DESCRIPTION "基于Qt6的音频播放器应用程序"

// 构建信息
#define BUILD_DATE __DATE__
#define BUILD_TIME __TIME__

// 版本比较宏
#define VERSION_ENCODE(major, minor, patch) \
    (((major) << 16) | ((minor) << 8) | (patch))

#define APP_VERSION_ENCODED \
    VERSION_ENCODE(APP_VERSION_MAJOR, APP_VERSION_MINOR, APP_VERSION_PATCH)

#endif // VERSION_H

