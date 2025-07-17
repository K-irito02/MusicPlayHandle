@echo off
echo [构建脚本] 开始构建音频引擎测试程序
echo.

:: 设置Qt环境变量（根据实际Qt安装路径调整）
set QT_DIR=C:\Qt\6.5.3\mingw_64
set PATH=%QT_DIR%\bin;%PATH%
set CMAKE_PREFIX_PATH=%QT_DIR%

:: 创建构建目录
if not exist "build_test" (
    echo [构建脚本] 创建构建目录
    mkdir build_test
)

:: 进入构建目录
cd build_test

echo [构建脚本] 配置CMake项目
:: 配置CMake项目
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug -f ../CMakeLists_test.txt ..

if %ERRORLEVEL% neq 0 (
    echo [构建脚本] CMake配置失败
    pause
    exit /b 1
)

echo [构建脚本] 开始编译
:: 编译项目
cmake --build . --config Debug

if %ERRORLEVEL% neq 0 (
    echo [构建脚本] 编译失败
    pause
    exit /b 1
)

echo [构建脚本] 编译成功
echo.
echo [构建脚本] 可执行文件位置: build_test\bin\MusicPlayHandleTest.exe
echo.
echo [构建脚本] 是否立即运行测试程序？(Y/N)
set /p choice=

if /i "%choice%"=="Y" (
    echo [构建脚本] 启动测试程序
    echo.
    .\bin\MusicPlayHandleTest.exe
) else (
    echo [构建脚本] 构建完成，可手动运行测试程序
)

echo.
echo [构建脚本] 按任意键退出
pause