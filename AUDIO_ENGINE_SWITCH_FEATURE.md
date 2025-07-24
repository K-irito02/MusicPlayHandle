# 音频引擎切换功能

## 功能概述

在播放界面的音乐控制按钮最左边新增了音频引擎选择按钮，用户可以在两种音频播放引擎之间切换：

1. **QMediaPlayer** - 默认引擎，纯净音质播放
2. **FFmpeg** - 高级引擎，支持实时音效处理

## 功能特点

### QMediaPlayer引擎
- ✅ **纯净音质**：不受任何音效处理影响，保持原始音频质量
- ✅ **系统兼容性好**：使用Qt框架内置的媒体播放器
- ✅ **稳定可靠**：经过广泛测试，兼容性强
- ❌ **平衡控制无效**：左右声道平衡调节不会生效
- ❌ **音效功能受限**：不支持实时音频处理

### FFmpeg引擎
- ✅ **实时音效处理**：支持平衡控制等音频效果
- ✅ **高级音频功能**：基于FFmpeg解码，功能强大
- ✅ **自定义音频处理**：可以实现复杂的音频算法
- ⚠️ **计算资源消耗**：相比QMediaPlayer消耗更多CPU
- ⚠️ **复杂度高**：内部实现较为复杂

## 界面设计

### 音频引擎切换按钮
- **位置**：播放控制按钮组的最左边，靠近边框
- **尺寸**：100x40像素
- **样式**：深色主题，可切换状态
- **文本显示**：
  - QMediaPlayer模式：显示"QMediaPlayer"
  - FFmpeg模式：显示"FFmpeg"

### 按钮状态指示
```
QMediaPlayer模式：
┌─────────────┐
│QMediaPlayer │  ← 未选中状态（默认色彩）
└─────────────┘

FFmpeg模式：
┌─────────────┐
│   FFmpeg    │  ← 选中状态（高亮色彩）
└─────────────┘
```

### 工具提示
- **QMediaPlayer模式**：
  ```
  当前使用QMediaPlayer播放
  不受音效处理影响，音质纯净
  点击切换到FFmpeg
  ```

- **FFmpeg模式**：
  ```
  当前使用FFmpeg引擎播放
  支持实时音效处理（平衡控制等）
  点击切换到QMediaPlayer
  ```

## 平衡控制集成

### 视觉反馈
平衡控制标签会根据当前音频引擎显示不同的状态：

#### QMediaPlayer模式
```
平衡: 中央 (切换到FFmpeg生效)
```
- 文字颜色：橙色 (#D08770)
- 提示用户需要切换到FFmpeg引擎才能使平衡控制生效

#### FFmpeg模式
```
平衡: 中央 (已生效)
```
- 文字颜色：蓝色 (#81A1C1)
- 表示平衡控制当前已生效

## 技术实现

### 核心类结构

#### AudioTypes::AudioEngineType 枚举
```cpp
enum class AudioEngineType {
    QMediaPlayer,   // 使用QMediaPlayer播放
    FFmpeg          // 使用FFmpeg解码+QAudioSink播放
};
```

#### AudioEngine 新增方法
```cpp
// 设置音频引擎类型
void setAudioEngineType(AudioTypes::AudioEngineType type);

// 获取当前音频引擎类型
AudioTypes::AudioEngineType getAudioEngineType() const;
QString getAudioEngineTypeString() const;

// 音频引擎类型变化信号
signal void audioEngineTypeChanged(AudioTypes::AudioEngineType type);
```

#### PlayInterface 新增功能
```cpp
// 音频引擎按钮点击处理
void onAudioEngineButtonClicked();

// 音频引擎类型变化处理
void onAudioEngineTypeChanged(AudioTypes::AudioEngineType type);

// 更新平衡显示（包含引擎状态提示）
void updateBalanceDisplay();
```

### 播放逻辑分离

#### QMediaPlayer模式
```cpp
void AudioEngine::playWithQMediaPlayer(const Song& song) {
    // 停止FFmpeg解码器
    if (m_ffmpegDecoder && m_ffmpegDecoder->isDecoding()) {
        m_ffmpegDecoder->stop();
    }
    
    // 使用QMediaPlayer播放
    m_player->play();
}
```

#### FFmpeg模式
```cpp
void AudioEngine::playWithFFmpeg(const Song& song) {
    // 停止QMediaPlayer
    if (m_player && m_player->playbackState() != QMediaPlayer::StoppedState) {
        m_player->stop();
    }
    
    // 使用FFmpeg解码器播放
    m_ffmpegDecoder->openFile(song.filePath());
    m_ffmpegDecoder->setBalance(m_balance);  // 应用平衡设置
    m_ffmpegDecoder->startDecoding();
}
```

### Seek操作适配

```cpp
void AudioEngine::seek(qint64 position) {
    if (m_audioEngineType == AudioTypes::AudioEngineType::FFmpeg) {
        // FFmpeg模式：使用FFmpeg解码器跳转
        m_ffmpegDecoder->seekTo(position);
    } else {
        // QMediaPlayer模式：使用QMediaPlayer跳转
        m_player->setPosition(position);
    }
}
```

## 配置持久化

### 设置保存
- 音频引擎类型设置保存到配置文件：`audio/engine_type`
- 默认值：0 (QMediaPlayer)
- FFmpeg：1

### 启动时恢复
```cpp
// 加载音频引擎类型设置
int engineTypeInt = config->getValue("audio/engine_type", 0).toInt();
m_audioEngineType = static_cast<AudioTypes::AudioEngineType>(engineTypeInt);
```

## 使用说明

### 基本操作
1. **查看当前引擎**：查看按钮上的文字显示
2. **切换引擎**：点击音频引擎按钮
3. **验证切换**：观察按钮文字和颜色变化

### 平衡控制测试
1. **QMediaPlayer模式测试**：
   - 调节平衡滑块
   - 观察到标签显示"(切换到FFmpeg生效)"
   - 实际音频输出不受影响

2. **FFmpeg模式测试**：
   - 切换到FFmpeg引擎
   - 调节平衡滑块
   - 观察到标签显示"(已生效)"
   - 实际音频输出会有左右声道平衡变化

### 最佳实践
- **音质优先**：使用QMediaPlayer模式获得最佳音质
- **音效需求**：需要平衡控制等音效时切换到FFmpeg模式
- **性能考虑**：FFmpeg模式会消耗更多系统资源

## 测试验证

### 功能测试脚本
项目包含测试脚本 `test_audio_engine_switch.cpp`，可验证：
- 引擎切换功能
- 平衡控制在不同引擎下的效果
- 进度条拖拽兼容性
- 播放状态管理

### 测试步骤
1. 编译并运行测试程序
2. 点击"切换引擎"按钮测试切换功能
3. 在不同引擎下调节平衡控制
4. 测试播放、暂停、跳转功能
5. 观察音频质量和效果差异

## 注意事项

### 兼容性
- Qt6.0+ 版本支持
- FFmpeg库需要正确编译和链接
- 某些音频格式可能只支持特定引擎

### 性能影响
- QMediaPlayer：低CPU占用，推荐用于长时间播放
- FFmpeg：较高CPU占用，建议在需要音效时使用

### 异常处理
- FFmpeg初始化失败时自动回退到QMediaPlayer
- 播放错误时会尝试切换引擎重新播放
- 配置文件损坏时使用默认设置

## 未来扩展

### 可能的增强功能
1. **音效预设**：为不同音乐类型提供预设音效
2. **可视化增强**：在FFmpeg模式下提供更丰富的可视化效果
3. **音频分析**：利用FFmpeg的强大功能进行实时音频分析
4. **插件系统**：支持第三方音频处理插件

### 技术优化
1. **性能优化**：优化FFmpeg解码器的资源使用
2. **音质改进**：实现更高质量的音频重采样
3. **延迟优化**：减少FFmpeg模式的音频延迟
4. **内存管理**：优化音频缓冲区管理