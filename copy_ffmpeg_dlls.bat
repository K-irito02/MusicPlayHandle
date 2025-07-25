@echo off
chcp 65001 >nul
echo ========================================
echo FFmpeg DLL文件复制工具
echo ========================================
echo.

:: 设置路径变量
set "FFMPEG_BIN_DIR=%~dp0third_party\ffmpeg\bin"
set "DEBUG_DIR=%~dp0build\Desktop_Qt_6_5_3_MinGW_64_bit-Debug\debug"
set "RELEASE_DIR=%~dp0build\Desktop_Qt_6_5_3_MinGW_64_bit-Debug\release"

:: 检查FFmpeg bin目录是否存在
if not exist "%FFMPEG_BIN_DIR%" (
    echo 错误: 找不到FFmpeg bin目录: %FFMPEG_BIN_DIR%
    echo 请确保FFmpeg库已正确放置在third_party\ffmpeg\bin目录中
    pause
    exit /b 1
)

echo 源目录: %FFMPEG_BIN_DIR%
echo.

:: 定义需要复制的DLL文件列表
set "DLL_FILES=avcodec-61.dll avdevice-61.dll avfilter-10.dll avformat-61.dll avutil-59.dll postproc-58.dll swresample-5.dll swscale-8.dll"

:: 复制到Debug目录
echo 正在复制到Debug目录...
if not exist "%DEBUG_DIR%" (
    echo 创建Debug目录: %DEBUG_DIR%
    mkdir "%DEBUG_DIR%"
)

for %%f in (%DLL_FILES%) do (
    if exist "%FFMPEG_BIN_DIR%\%%f" (
        echo 复制: %%f
        copy /Y "%FFMPEG_BIN_DIR%\%%f" "%DEBUG_DIR%\" >nul
        if errorlevel 1 (
            echo 错误: 复制 %%f 失败
        ) else (
            echo 成功: %%f
        )
    ) else (
        echo 警告: 找不到文件 %%f
    )
)

echo.

:: 复制到Release目录
echo 正在复制到Release目录...
if not exist "%RELEASE_DIR%" (
    echo 创建Release目录: %RELEASE_DIR%
    mkdir "%RELEASE_DIR%"
)

for %%f in (%DLL_FILES%) do (
    if exist "%FFMPEG_BIN_DIR%\%%f" (
        echo 复制: %%f
        copy /Y "%FFMPEG_BIN_DIR%\%%f" "%RELEASE_DIR%\" >nul
        if errorlevel 1 (
            echo 错误: 复制 %%f 失败
        ) else (
            echo 成功: %%f
        )
    ) else (
        echo 警告: 找不到文件 %%f
    )
)

echo.
echo ========================================
echo 复制完成！
echo ========================================
echo.
echo 已复制的DLL文件:
for %%f in (%DLL_FILES%) do (
    echo   - %%f
)
echo.
echo Debug目录: %DEBUG_DIR%
echo Release目录: %RELEASE_DIR%
echo.
echo 注意: 如果您的项目使用不同的构建配置，请相应调整目标目录路径
echo.
pause 