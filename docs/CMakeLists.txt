cmake_minimum_required(VERSION 3.16)

# 项目信息
project(MusicPlayHandle VERSION 1.0.0 LANGUAGES CXX)

# C++标准设置
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 构建类型设置
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Qt6配置
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Multimedia Sql)

# 启用Qt的MOC、UIC、RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 设置UIC搜索路径
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR})

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/audio
    ${CMAKE_CURRENT_SOURCE_DIR}/src/database
    ${CMAKE_CURRENT_SOURCE_DIR}/src/interfaces
    ${CMAKE_CURRENT_SOURCE_DIR}/src/managers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/models
    ${CMAKE_CURRENT_SOURCE_DIR}/src/threading
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/controllers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/dialogs
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/widgets
)

# 源文件列表
set(SOURCES
    # 主程序文件
    main.cpp
    mainwindow.cpp
    
    # 音频模块
    src/audio/audioengine.cpp
    src/threading/audioworkerthread.cpp
    
    # 核心模块
    src/core/appconfig.cpp
    src/core/applicationmanager.cpp
    src/core/lazyloader.cpp
    src/core/logger.cpp
    src/core/objectpool.cpp
    src/core/servicecontainer.cpp
    src/core/structuredlogger.cpp
    src/core/tagconfiguration.cpp
    src/core/tagstrings.cpp
    
    # 数据库模块
    src/database/basedao.cpp
    src/database/databasemanager.cpp
    src/database/logdao.cpp
    src/database/playlistdao.cpp
    src/database/songdao.cpp
    src/database/tagdao.cpp
    
    # 管理器模块
    src/managers/playlistmanager.cpp
    src/managers/tagmanager.cpp
    
    # 数据模型
    src/models/errorlog.cpp
    src/models/playhistory.cpp
    src/models/playlist.cpp
    src/models/song.cpp
    src/models/systemlog.cpp
    src/models/tag.cpp
    
    # UI控制器
    src/ui/controllers/MainWindowController.cpp
    src/ui/controllers/AddSongDialogController.cpp
    src/ui/controllers/ManageTagDialogController.cpp
    src/ui/controllers/PlayInterfaceController.cpp
    
    # UI对话框
    src/ui/dialogs/AddSongDialog.cpp
    src/ui/dialogs/CreateTagDialog.cpp
    src/ui/dialogs/ManageTagDialog.cpp
    src/ui/dialogs/PlayInterface.cpp
    src/ui/dialogs/SettingsDialog.cpp
    
    # UI控件
    src/ui/widgets/taglistitem.cpp
    
)

# 头文件列表
set(HEADERS
    mainwindow.h
    version.h
    
    # 音频模块
    src/audio/audioengine.h
    src/audio/audiotypes.h
    
    # 核心模块
    src/core/appconfig.h
    src/core/applicationmanager.h
    src/core/audiotypes.h
    src/core/cache.h
    src/core/componentintegration.h
    src/core/constants.h
    src/core/lazyloader.h
    src/core/logger.h
    src/core/objectpool.h
    src/core/result.h
    src/core/servicecontainer.h
    src/core/structuredlogger.h
    src/core/tagconfiguration.h
    src/core/tagstrings.h
    
    # 数据库模块
    src/database/basedao.h
    src/database/databasemanager.h
    src/database/logdao.h
    src/database/playlistdao.h
    src/database/songdao.h
    src/database/tagdao.h
    
    # 接口
    src/interfaces/idatabasemanager.h
    src/interfaces/itagmanager.h
    
    # 管理器模块
    src/managers/playlistmanager.h
    src/managers/tagmanager.h
    
    # 数据模型
    src/models/errorlog.h
    src/models/playhistory.h
    src/models/playlist.h
    src/models/song.h
    src/models/systemlog.h
    src/models/tag.h
    
    # 线程管理
    src/threading/audioworkerthread.h
    src/threading/mainthreadmanager.h
    
    # UI控制器
    src/ui/controllers/MainWindowController.h
    src/ui/controllers/AddSongDialogController.h
    src/ui/controllers/ManageTagDialogController.h
    src/ui/controllers/PlayInterfaceController.h
    
    # UI对话框
    src/ui/dialogs/AddSongDialog.h
    src/ui/dialogs/CreateTagDialog.h
    src/ui/dialogs/ManageTagDialog.h
    src/ui/dialogs/PlayInterface.h
    src/ui/dialogs/SettingsDialog.h
    
    # UI控件
    src/ui/widgets/taglistitem.h
    src/ui/widgets/taglistitemfactory.h
    
)

# UI文件列表
set(UI_FILES
    mainwindow.ui
    addSongDialog.ui
    createtagdialog.ui
    manageTagDialog.ui
    playInterface.ui
    settingsdialog.ui
)

# 资源文件
set(RESOURCE_FILES
    icon.qrc
)

# 翻译文件
set(TS_FILES
    translations/en_US.ts
)

# 创建可执行文件
add_executable(MusicPlayHandle
    ${SOURCES}
    ${HEADERS}
    ${UI_FILES}
    ${RESOURCE_FILES}
)

# 链接Qt6库
target_link_libraries(MusicPlayHandle
    Qt6::Core
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::Sql
)

# 编译器特定设置
if(MSVC)
    target_compile_options(MusicPlayHandle PRIVATE /W4)
else()
    target_compile_options(MusicPlayHandle PRIVATE -Wall -Wextra -Wpedantic)
endif()

# 设置输出目录
set_target_properties(MusicPlayHandle PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin
)

# 处理翻译文件
if(Qt6LinguistTools_FOUND)
    qt6_add_translations(MusicPlayHandle TS_FILES ${TS_FILES})
endif()

# 安装规则（可选）
install(TARGETS MusicPlayHandle
    RUNTIME DESTINATION bin
)

# 复制Qt6运行时库（Windows）
if(WIN32)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS ${Qt6_DIR}/../../../bin)
    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET MusicPlayHandle POST_BUILD
            COMMAND ${WINDEPLOYQT_EXECUTABLE} $<TARGET_FILE:MusicPlayHandle>
            COMMENT "Deploying Qt libraries")
    endif()
endif()

# 打印配置信息
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Qt6 version: ${Qt6_VERSION}")