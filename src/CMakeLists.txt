# 改进架构组件的CMake构建文件

# 设置最小CMake版本
cmake_minimum_required(VERSION 3.16)

# 项目名称
project(ImprovedAudioArchitecture)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找Qt6组件
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Multimedia)

# 设置Qt MOC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/audio
    ${CMAKE_CURRENT_SOURCE_DIR}/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/threading
    ${CMAKE_CURRENT_SOURCE_DIR}/models
    ${CMAKE_CURRENT_SOURCE_DIR}/database
)

# 核心组件源文件
set(CORE_SOURCES
    core/observer.h
    core/performancemanager.h
    core/resourcemanager.h
    core/appconfig.cpp
    core/logger.cpp
    core/objectpool.cpp
    core/servicecontainer.cpp
    core/structuredlogger.cpp
    core/tagconfiguration.cpp
    core/tagstrings.cpp
)

# 音频组件源文件
set(AUDIO_SOURCES
    audio/improvedaudioengine.h
    audio/audioengine.cpp
    audio/ffmpegdecoder.cpp
    audio/audiotypes.h
)

# UI组件源文件
set(UI_SOURCES
    ui/dialogs/improvedplayinterface.h
    ui/dialogs/PlayInterface.cpp
    ui/dialogs/AddSongDialog.cpp
    ui/dialogs/CreateTagDialog.cpp
    ui/dialogs/ManageTagDialog.cpp
    ui/dialogs/SettingsDialog.cpp
)

# 线程组件源文件
set(THREADING_SOURCES
    threading/threadpoolmanager.h
    threading/audioworkerthread.cpp
    threading/mainthreadmanager.h
)

# 模型组件源文件
set(MODELS_SOURCES
    models/song.h
)

# 数据库组件源文件
set(DATABASE_SOURCES
    database/playhistorydao.h
)

# 管理器组件源文件
set(MANAGERS_SOURCES
    managers/
)

# 接口组件源文件
set(INTERFACES_SOURCES
    interfaces/idatabasemanager.h
    interfaces/itagmanager.h
)

# 所有源文件
set(ALL_SOURCES
    ${CORE_SOURCES}
    ${AUDIO_SOURCES}
    ${UI_SOURCES}
    ${THREADING_SOURCES}
    ${MODELS_SOURCES}
    ${DATABASE_SOURCES}
    ${MANAGERS_SOURCES}
    ${INTERFACES_SOURCES}
)

# 查找FFmpeg库
find_path(FFMPEG_INCLUDE_DIR
    NAMES libavcodec/avcodec.h
    PATHS
        /usr/include
        /usr/local/include
        /opt/local/include
        ${CMAKE_PREFIX_PATH}/include
)

find_library(FFMPEG_AVCODEC_LIBRARY
    NAMES avcodec
    PATHS
        /usr/lib
        /usr/local/lib
        /opt/local/lib
        ${CMAKE_PREFIX_PATH}/lib
)

find_library(FFMPEG_AVFORMAT_LIBRARY
    NAMES avformat
    PATHS
        /usr/lib
        /usr/local/lib
        /opt/local/lib
        ${CMAKE_PREFIX_PATH}/lib
)

find_library(FFMPEG_AVUTIL_LIBRARY
    NAMES avutil
    PATHS
        /usr/lib
        /usr/local/lib
        /opt/local/lib
        ${CMAKE_PREFIX_PATH}/lib
)

find_library(FFMPEG_SWRESAMPLE_LIBRARY
    NAMES swresample
    PATHS
        /usr/lib
        /usr/local/lib
        /opt/local/lib
        ${CMAKE_PREFIX_PATH}/lib
)

# 创建改进架构库
add_library(ImprovedAudioArchitecture STATIC
    ${ALL_SOURCES}
)

# 链接Qt6库
target_link_libraries(ImprovedAudioArchitecture
    Qt6::Core
    Qt6::Widgets
    Qt6::Multimedia
)

# 如果找到FFmpeg，链接FFmpeg库
if(FFMPEG_INCLUDE_DIR AND FFMPEG_AVCODEC_LIBRARY)
    target_include_directories(ImprovedAudioArchitecture PRIVATE ${FFMPEG_INCLUDE_DIR})
    target_link_libraries(ImprovedAudioArchitecture
        ${FFMPEG_AVCODEC_LIBRARY}
        ${FFMPEG_AVFORMAT_LIBRARY}
        ${FFMPEG_AVUTIL_LIBRARY}
        ${FFMPEG_SWRESAMPLE_LIBRARY}
    )
    add_definitions(-DUSE_FFMPEG)
    message(STATUS "FFmpeg found and will be used")
else()
    message(WARNING "FFmpeg not found, some features will be disabled")
endif()

# 编译选项
target_compile_definitions(ImprovedAudioArchitecture PRIVATE
    QT_NO_CAST_FROM_ASCII
    QT_NO_CAST_TO_ASCII
    QT_NO_URL_CAST_FROM_STRING
    QT_USE_QSTRINGBUILDER
)

# 编译器特定选项
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(ImprovedAudioArchitecture PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wno-unused-parameter
        -fPIC
    )
    
    # Release模式优化
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(ImprovedAudioArchitecture PRIVATE
            -O3
            -DNDEBUG
            -flto
        )
    endif()
    
    # Debug模式调试信息
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_options(ImprovedAudioArchitecture PRIVATE
            -g
            -O0
            -DDEBUG
        )
    endif()
endif()

# MSVC编译器选项
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(ImprovedAudioArchitecture PRIVATE
        /W4
        /permissive-
    )
    
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(ImprovedAudioArchitecture PRIVATE
            /O2
            /DNDEBUG
        )
    endif()
endif()

# 安装规则
install(TARGETS ImprovedAudioArchitecture
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

# 安装头文件
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/ImprovedAudioArchitecture
    FILES_MATCHING PATTERN "*.h"
)

# 演示程序
if(BUILD_EXAMPLES)
    add_subdirectory(../examples examples)
endif()

# 测试
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(../tests tests)
endif()

# 文档生成
if(BUILD_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../docs/Doxyfile.in
                       ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
        add_custom_target(docs
            ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen" VERBATIM
        )
    endif()
endif()

# 打印配置信息
message(STATUS "=== Improved Audio Architecture Configuration ===")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Qt6 version: ${Qt6_VERSION}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "FFmpeg support: ${FFMPEG_INCLUDE_DIR}")
message(STATUS "Build examples: ${BUILD_EXAMPLES}")
message(STATUS "Build tests: ${BUILD_TESTS}")
message(STATUS "Build docs: ${BUILD_DOCS}")
message(STATUS "==============================================")

# 添加自定义目标
add_custom_target(format
    COMMAND find ${CMAKE_CURRENT_SOURCE_DIR} -name "*.cpp" -o -name "*.h" | xargs clang-format -i
    COMMENT "Formatting source code"
)

add_custom_target(analyze
    COMMAND cppcheck --enable=all --std=c++17 ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running static analysis"
)