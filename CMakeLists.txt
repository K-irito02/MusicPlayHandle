cmake_minimum_required(VERSION 3.16)

# 项目信息
project(MusicPlayHandle
    VERSION 1.0.0
    DESCRIPTION "Modern Qt6 Music Player with Advanced Tag Management"
    LANGUAGES CXX
)

# C++标准设置
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Qt6配置
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Widgets
    Sql
    Network
    Multimedia
    Test
    LinguistTools
)

# 编译器特定设置
if(MSVC)
    add_compile_options(/W4 /permissive-)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 调试/发布配置
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(QT_QML_DEBUG)
    add_compile_options(-g -O0)
else()
    add_compile_definitions(QT_NO_DEBUG_OUTPUT)
    add_compile_options(-O2)
endif()

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/widgets
    ${CMAKE_CURRENT_SOURCE_DIR}/src/models
    ${CMAKE_CURRENT_SOURCE_DIR}/src/dao
    ${CMAKE_CURRENT_SOURCE_DIR}/src/managers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/controllers
)

# 核心库源文件
set(CORE_SOURCES
    # 核心优化组件
    src/core/servicecontainer.cpp
    src/core/lazyloader.cpp
    src/core/tagconfiguration.cpp
    src/core/structuredlogger.cpp
    src/core/tagstrings.cpp
    src/core/objectpool.cpp
    
    # 现有核心组件
    src/core/applicationmanager.cpp
    src/core/databasemanager.cpp
    src/core/tagmanager.cpp
    src/managers/playlistmanager.cpp
)

set(CORE_HEADERS
    # 核心优化组件
    src/core/servicecontainer.h
    src/core/itagmanager.h
    src/core/idatabasemanager.h
    src/core/result.h
    src/core/databasetransaction.h
    src/core/constants.h
    src/core/cache.h
    src/core/lazyloader.h
    src/core/tagconfiguration.h
    src/core/structuredlogger.h
    src/core/tagstrings.h
    src/core/objectpool.h
    
    # 现有核心组件
    src/core/applicationmanager.h
    src/core/databasemanager.h
    src/core/tagmanager.h
    src/managers/playlistmanager.h
)

# UI组件源文件
set(UI_SOURCES
    src/ui/mainwindow.cpp
    src/ui/widgets/taglistitem.cpp
    src/ui/widgets/taglistitemfactory.h  # 仅头文件实现
    src/controllers/mainwindowcontroller.cpp
)

set(UI_HEADERS
    src/ui/mainwindow.h
    src/ui/widgets/taglistitem.h
    src/ui/widgets/taglistitemfactory.h
    src/controllers/mainwindowcontroller.h
)

# 模型源文件
set(MODEL_SOURCES
    src/models/tag.cpp
    src/models/song.cpp
)

set(MODEL_HEADERS
    src/models/tag.h
    src/models/song.h
)

# DAO源文件
set(DAO_SOURCES
    src/database/basedao.cpp
    src/database/tagdao.cpp
    src/database/songdao.cpp
    src/database/playlistdao.cpp
)

set(DAO_HEADERS
    src/database/basedao.h
    src/database/tagdao.h
    src/database/songdao.h
    src/database/playlistdao.h
)

# UI文件
set(UI_FILES
    src/ui/mainwindow.ui
)

# 资源文件
set(RESOURCE_FILES
    resources/resources.qrc
)

# 翻译文件
set(TS_FILES
    translations/musicplayhandle_zh_CN.ts
    translations/musicplayhandle_en_US.ts
)

# 处理UI文件
qt6_wrap_ui(UI_GENERATED_HEADERS ${UI_FILES})

# 处理资源文件
qt6_add_resources(RESOURCE_GENERATED ${RESOURCE_FILES})

# 处理翻译文件
qt6_add_lrelease(${TS_FILES})
qt6_add_lupdate(
    SOURCES ${CORE_SOURCES} ${UI_SOURCES} ${MODEL_SOURCES} ${DAO_SOURCES}
    TS_FILES ${TS_FILES}
)

# 创建核心库
add_library(MusicPlayHandleCore STATIC
    ${CORE_SOURCES}
    ${CORE_HEADERS}
    ${MODEL_SOURCES}
    ${MODEL_HEADERS}
    ${DAO_SOURCES}
    ${DAO_HEADERS}
)

# 核心库链接
target_link_libraries(MusicPlayHandleCore
    Qt6::Core
    Qt6::Sql
    Qt6::Network
)

# 设置核心库属性
set_target_properties(MusicPlayHandleCore PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    AUTOUIC ON
)

# 主应用程序
add_executable(MusicPlayHandle
    src/main.cpp
    ${UI_SOURCES}
    ${UI_HEADERS}
    ${UI_GENERATED_HEADERS}
    ${RESOURCE_GENERATED}
)

# 主应用程序链接
target_link_libraries(MusicPlayHandle
    MusicPlayHandleCore
    Qt6::Core
    Qt6::Widgets
    Qt6::Sql
    Qt6::Network
    Qt6::Multimedia
)

# 设置主应用程序属性
set_target_properties(MusicPlayHandle PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    AUTOUIC ON
    WIN32_EXECUTABLE ON
)

# 测试配置
enable_testing()

# 测试源文件
set(TEST_SOURCES
    src/tests/test_tagmanager.cpp
    src/tests/test_main.cpp
)

set(TEST_HEADERS
    src/tests/test_tagmanager.h
)

# 创建测试可执行文件
add_executable(MusicPlayHandleTests
    ${TEST_SOURCES}
    ${TEST_HEADERS}
)

# 测试链接
target_link_libraries(MusicPlayHandleTests
    MusicPlayHandleCore
    Qt6::Core
    Qt6::Test
    Qt6::Sql
)

# 设置测试属性
set_target_properties(MusicPlayHandleTests PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    AUTOUIC ON
)

# 添加测试
add_test(NAME TagManagerTests COMMAND MusicPlayHandleTests)

# 示例应用程序
add_executable(OptimizationExample
    examples/optimization_usage_example.cpp
)

# 示例应用程序链接
target_link_libraries(OptimizationExample
    MusicPlayHandleCore
    Qt6::Core
    Qt6::Widgets
    Qt6::Sql
)

# 设置示例应用程序属性
set_target_properties(OptimizationExample PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    AUTOUIC ON
    WIN32_EXECUTABLE ON
)

# 安装配置
install(TARGETS MusicPlayHandle
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)

# 安装Qt6库（Windows）
if(WIN32)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS ${Qt6_DIR}/../../../bin)
    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET MusicPlayHandle POST_BUILD
            COMMAND ${WINDEPLOYQT_EXECUTABLE} $<TARGET_FILE:MusicPlayHandle>
            COMMENT "Deploying Qt libraries"
        )
    endif()
endif()

# 代码格式化目标
find_program(CLANG_FORMAT_EXECUTABLE clang-format)
if(CLANG_FORMAT_EXECUTABLE)
    file(GLOB_RECURSE ALL_SOURCE_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/examples/*.cpp
    )
    
    add_custom_target(format
        COMMAND ${CLANG_FORMAT_EXECUTABLE} -i ${ALL_SOURCE_FILES}
        COMMENT "Formatting source code"
    )
endif()

# 静态分析目标
find_program(CLANG_TIDY_EXECUTABLE clang-tidy)
if(CLANG_TIDY_EXECUTABLE)
    set_target_properties(MusicPlayHandleCore PROPERTIES
        CXX_CLANG_TIDY "${CLANG_TIDY_EXECUTABLE};-checks=*,-fuchsia-*,-google-*,-zircon-*,-abseil-*,-llvm-*"
    )
endif()

# 文档生成
find_package(Doxygen)
if(DOXYGEN_FOUND)
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
    
    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
    
    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )
endif()

# 性能分析目标
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    find_program(VALGRIND_EXECUTABLE valgrind)
    if(VALGRIND_EXECUTABLE)
        add_custom_target(memcheck
            COMMAND ${VALGRIND_EXECUTABLE} --tool=memcheck --leak-check=full --show-leak-kinds=all $<TARGET_FILE:MusicPlayHandle>
            DEPENDS MusicPlayHandle
            COMMENT "Running memory check with Valgrind"
        )
    endif()
endif()

# 打包配置
set(CPACK_PACKAGE_NAME "MusicPlayHandle")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_VENDOR "Qt6/C++11 Expert Team")
set(CPACK_PACKAGE_CONTACT "expert@musicplayhandle.com")

if(WIN32)
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_NSIS_DISPLAY_NAME "Music Play Handle")
    set(CPACK_NSIS_PACKAGE_NAME "MusicPlayHandle")
    set(CPACK_NSIS_CONTACT "expert@musicplayhandle.com")
    set(CPACK_NSIS_HELP_LINK "https://github.com/musicplayhandle/musicplayhandle")
    set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/musicplayhandle/musicplayhandle")
else()
    set(CPACK_GENERATOR "DEB;RPM")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt6core6, libqt6widgets6, libqt6sql6")
    set(CPACK_RPM_PACKAGE_REQUIRES "qt6-qtbase, qt6-qtmultimedia")
endif()

include(CPack)

# 输出构建信息
message(STATUS "=== Build Configuration ===")
message(STATUS "Project: ${PROJECT_NAME} v${PROJECT_VERSION}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Qt6 Version: ${Qt6_VERSION}")
message(STATUS "Install Prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "=============================")