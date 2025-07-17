# MusicPlayHandle 音频引擎简化版

## 概述

本项目重写了音频播放功能的相关文件，专注于核心播放功能，并添加了详细的 `qDebug` 调试信息。实现遵循"从简单做起"的原则，去除了复杂的高级功能，保留了基本的音频播放能力。

## 重写的文件

### 核心文件
1. **`src/audio/audioengine.h`** - 音频引擎头文件（简化版）
2. **`src/audio/audioengine.cpp`** - 音频引擎实现（简化版）
3. **`src/controllers/MainWindowController.h`** - 主窗口控制器头文件（简化版）
4. **`src/controllers/MainWindowController.cpp`** - 主窗口控制器实现（简化版）

### 测试文件
5. **`test_audio.cpp`** - 独立的音频播放测试程序
6. **`CMakeLists_test.txt`** - 测试程序的CMake配置
7. **`build_test.bat`** - 自动构建脚本

## 功能特性

### AudioEngine 核心功能
- ✅ **基本播放控制**: 播放、暂停、停止
- ✅ **音量控制**: 音量调节、静音切换
- ✅ **进度控制**: 播放位置跳转
- ✅ **播放列表**: 添加、移除、切换歌曲
- ✅ **状态监控**: 播放状态、媒体状态查询
- ✅ **错误处理**: 基本错误检测和报告
- ✅ **调试信息**: 详细的 qDebug 日志输出

### 移除的复杂功能
- ❌ 均衡器设置
- ❌ 播放模式（随机、循环等）
- ❌ 音频效果处理
- ❌ 播放历史记录
- ❌ 复杂的缓存机制
- ❌ 高级错误恢复
- ❌ 多线程复杂同步

## 调试信息

所有关键操作都包含详细的 `qDebug` 输出：

```cpp
// 播放控制调试
[AudioEngine] 开始播放: "歌曲名.mp3"
[AudioEngine] 播放状态变更: StoppedState -> PlayingState
[AudioEngine] 媒体加载完成: "file:///path/to/song.mp3"

// 音量控制调试
[AudioEngine] 音量设置: 75 (之前: 50)
[AudioEngine] 静音状态: true

// 播放列表调试
[AudioEngine] 播放列表更新: 5 首歌曲
[AudioEngine] 当前歌曲索引: 2

// 错误调试
[AudioEngine] 错误: 无法加载媒体文件
[AudioEngine] 警告: 不支持的音频格式
```

## 快速开始

### 1. 编译测试程序

```bash
# 方法1: 使用自动构建脚本
.\build_test.bat

# 方法2: 手动构建
mkdir build_test
cd build_test
cmake -G "MinGW Makefiles" -f ../CMakeLists_test.txt ..
cmake --build . --config Debug
```

### 2. 运行测试程序

```bash
.\build_test\bin\MusicPlayHandleTest.exe
```

### 3. 测试功能

1. **添加歌曲**: 点击"添加歌曲"按钮选择音频文件
2. **播放控制**: 使用播放、暂停、停止按钮
3. **音量调节**: 拖动音量滑块或点击静音按钮
4. **进度控制**: 拖动进度条跳转播放位置
5. **切换歌曲**: 使用上一首、下一首按钮
6. **查看调试**: 在控制台查看详细的调试信息

## 代码架构

### AudioEngine 设计

```cpp
class AudioEngine : public QObject {
    Q_OBJECT
public:
    // 单例模式
    static AudioEngine* instance();
    
    // 核心播放控制
    void play();
    void pause();
    void stop();
    void seek(qint64 position);
    
    // 音量控制
    void setVolume(int volume);
    void setMuted(bool muted);
    
    // 播放列表管理
    void setPlaylist(const QList<Song>& songs);
    void setCurrentIndex(int index);
    
private:
    QMediaPlayer* m_mediaPlayer;
    QAudioOutput* m_audioOutput;
    QList<Song> m_playlist;
    // ... 其他成员
};
```

### 调试辅助方法

```cpp
// 统一的调试输出格式
void debugLog(const QString& message);
void debugError(const QString& error);
void debugWarning(const QString& warning);

// 状态转换调试
QString playbackStateToString(QMediaPlayer::PlaybackState state);
QString mediaStatusToString(QMediaPlayer::MediaStatus status);
```

## 支持的音频格式

- MP3
- WAV
- FLAC
- OGG
- M4A
- WMA

## 系统要求

- Qt 6.5.3 或更高版本
- C++17 编译器
- Windows 10/11 (MinGW)
- CMake 3.16 或更高版本

## 故障排除

### 常见问题

1. **编译错误**: 确保Qt路径正确设置
2. **音频无法播放**: 检查音频文件格式和路径
3. **调试信息不显示**: 确保在Debug模式下编译

### 调试技巧

```cpp
// 启用详细调试
QLoggingCategory::setFilterRules("qt.multimedia.debug=true");

// 检查音频设备
qDebug() << "可用音频设备:" << QMediaDevices::audioOutputs();

// 监控播放状态
connect(audioEngine, &AudioEngine::playbackStateChanged, 
        [](QMediaPlayer::PlaybackState state) {
    qDebug() << "播放状态:" << state;
});
```

## 下一步开发

1. **UI优化**: 改进用户界面设计
2. **功能扩展**: 逐步添加高级功能
3. **性能优化**: 优化大播放列表处理
4. **错误处理**: 增强错误恢复机制
5. **测试覆盖**: 添加单元测试和集成测试

## 贡献指南

1. 保持代码简洁性
2. 添加适当的调试信息
3. 遵循项目编码规范
4. 确保向后兼容性
5. 编写清晰的文档

---

**注意**: 这是一个简化版本，专注于核心功能和调试能力。复杂功能将在后续版本中逐步添加。