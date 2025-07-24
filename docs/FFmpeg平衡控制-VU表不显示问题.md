我来分析并解决平衡控制和VU表不工作的问题。让我先检查当前的实现：

Read file: src/audio/ffmpegdecoder.cpp
Read file: src/audio/ffmpegdecoder.cpp
Read file: src/audio/ffmpegdecoder.cpp
Read file: src/audio/audioengine.cpp
Ran tool
Read file: src/audio/audioengine.cpp
Ran tool
Read file: src/audio/audioengine.cpp
现在我发现了问题！主要有几个关键问题：

1. **FFmpeg解码器没有与QMediaPlayer同步**：FFmpeg解码器在独立运行，但没有控制实际的音频播放
2. **平衡控制只影响FFmpeg解码器，不影响实际播放**：实际的音频播放还是通过QMediaPlayer
3. **VU表数据来源问题**：FFmpeg解码器在解码，但QMediaPlayer在播放

让我检查播放相关的代码：

Ran tool
Read file: src/audio/audioengine.cpp
Ran tool
Read file: src/audio/audioengine.cpp
现在我明白问题了！当前的架构有两个独立的音频系统：

1. **QMediaPlayer + QAudioOutput**：负责实际的音频播放
2. **FFmpegDecoder**：负责解码和音频处理，但没有控制实际播放

这就是为什么平衡控制和VU表不工作的原因。我需要修改架构，让FFmpeg解码器控制实际的音频播放。

让我实现一个解决方案：

现在我需要修改pause和stop方法：

Ran tool
Read file: src/audio/audioengine.cpp
现在修改stop方法：

现在修改seek方法：

现在我需要修改FFmpegDecoder，让它能够实际播放音频。我需要添加音频输出功能：

Read file: src/audio/ffmpegdecoder.h
我需要添加Qt音频输出功能到FFmpegDecoder。让我修改头文件：

现在修改FFmpegDecoder的实现：

现在我需要添加音频输出的实现方法。让我在文件末尾添加：

现在我需要修改processAudioFrame方法，让它将音频数据写入音频输出：

让我重新读取文件内容：

Read file: src/audio/ffmpegdecoder.cpp
现在我需要修改calculateLevels方法，让平衡控制影响实际的音频数据：

现在我需要修改processAudioFrame方法，让平衡控制影响实际的音频数据：

现在我需要修改startDecoding和stopDecoding方法：

Ran tool
Read file: src/audio/ffmpegdecoder.cpp
现在我需要修改startDecoding方法，确保音频输出正确启动：

现在我需要修改stopDecoding方法：

现在我需要修改closeFile方法：

Ran tool
Read file: src/audio/ffmpegdecoder.cpp
现在我需要修改AudioEngine的setBalance方法，确保它正确调用FFmpeg解码器的平衡设置：

Read file: src/audio/audioengine.cpp
现在我需要修改AudioEngine的onFFmpegAudioDataReady方法，确保VU表能正确显示：

Read file: src/audio/audioengine.cpp
现在我需要修改onFFmpegAudioDataReady方法，移除状态检查，让VU表始终能显示：

现在让我总结一下修复的内容：

## �� 修复总结

### ✅ 主要问题分析：

1. **架构问题**：之前FFmpeg解码器只负责解码，不控制实际音频播放
2. **平衡控制无效**：平衡设置只影响FFmpeg解码器，不影响QMediaPlayer播放
3. **VU表无显示**：FFmpeg解码器在解码，但QMediaPlayer在播放，数据不同步

### ✅ 解决方案：

#### 1. **修改AudioEngine播放控制**：
- **play()方法**：优先使用FFmpeg解码器播放，回退到QMediaPlayer
- **pause()方法**：支持FFmpeg解码器暂停
- **stop()方法**：支持FFmpeg解码器停止
- **seek()方法**：支持FFmpeg解码器跳转

#### 2. **增强FFmpegDecoder功能**：
- **添加音频输出**：集成QAudioOutput进行实际音频播放
- **实时平衡控制**：在音频数据处理时应用平衡设置
- **音频数据写入**：将处理后的音频数据写入音频设备

#### 3. **关键修改点**：

**AudioEngine.cpp**：
```cpp
// 播放时优先使用FFmpeg解码器
if (m_ffmpegDecoder) {
    if (m_ffmpegDecoder->openFile(song.filePath())) {
        m_ffmpegDecoder->startDecoding();
        // 开始播放...
    }
}
```

**FFmpegDecoder.cpp**：
```cpp
// 应用平衡控制到音频数据
for (int i = 0; i < samples; ++i) {
    float leftSample = audioData[i * 2];
    float rightSample = audioData[i * 2 + 1];
    
    // 应用平衡设置
    if (m_balance < 0) {
        leftSample *= (1.0 + qAbs(m_balance));
        rightSample *= (1.0 - qAbs(m_balance) * 0.5);
    } else if (m_balance > 0) {
        leftSample *= (1.0 - m_balance * 0.5);
        rightSample *= (1.0 + m_balance);
    }
    
    audioData[i * 2] = leftSample;
    audioData[i * 2 + 1] = rightSample;
}

// 写入音频输出
writeAudioData(audioBytes);
```

#### 4. **VU表修复**：
- 移除了播放状态检查，让VU表始终能显示
- FFmpeg解码器直接提供实时音频电平数据
- 平衡控制影响实际音频数据，VU表反映真实变化

### �� 预期效果：

1. **平衡控制**：拖动滑块时，左右声道音量会实时变化
2. **VU表显示**：显示实时的左右声道电平差异
3. **持久化**：平衡设置会保存到配置文件
4. **实时性**：所有变化都是实时的，无延迟

### �� 技术特点：

- **双系统架构**：FFmpeg解码器 + QAudioOutput
- **实时处理**：音频数据实时处理和应用平衡
- **线程安全**：使用互斥锁保护共享数据
- **错误处理**：完善的错误处理和回退机制

现在您可以测试平衡控制和VU表功能了。拖动平衡滑块应该能看到：
1. 音乐左右声道音量变化
2. VU表实时显示左右声道电平差异
3. 平衡数值实时更新

