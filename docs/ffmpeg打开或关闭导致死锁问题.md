让我系统性地检查所有可能导致崩溃的代码。崩溃发生在`closeFile()`方法中，让我逐步分析：

Read file: src/audio/ffmpegdecoder.cpp
Read file: src/audio/ffmpegdecoder.cpp
Read file: src/audio/ffmpegdecoder.cpp
Read file: src/audio/ffmpegdecoder.cpp
Read file: src/audio/ffmpegdecoder.cpp
现在让我检查头文件，看看是否有互斥锁的问题：

Read file: src/audio/ffmpegdecoder.h
## �� **问题分析总结**

通过系统性地检查代码，我发现了导致崩溃的关键因素：

### �� **关键问题1：互斥锁死锁**
在`openFile()`方法中：
```cpp
try {
    QMutexLocker locker(&m_mutex);  // 获取锁
    closeFile();  // 调用closeFile()
}
```

而在`closeFile()`方法中：
```cpp
try {
    QMutexLocker locker(&m_mutex);  // 再次尝试获取同一个锁
    // ...
}
```

这会导致**死锁**，因为同一个线程试图获取已经持有的锁。

### �� **关键问题2：FFmpeg资源管理**
- `cleanupFFmpeg()`方法可能访问已释放的FFmpeg资源
- 在异常情况下，资源清理可能不完整

### �� **关键问题3：异常处理不完整**
- 某些FFmpeg API调用可能抛出异常
- 异常处理可能不够全面

## ✅ **修复方案**

让我修复这些问题：

现在让我修复`closeFile`方法，避免在已经持有锁的情况下再次获取锁：

现在让我添加`stopDecodingInternal`方法：

现在让我在头文件中添加这些新方法的声明：

让我重新读取头文件的结尾部分：

Read file: src/audio/ffmpegdecoder.h
现在让我们测试修复后的代码。请运行程序，看看是否还会崩溃。

## ✅ **死锁问题修复总结**

### �� **问题根源**：
崩溃的根本原因是**互斥锁死锁**：
1. `openFile()`方法获取锁后调用`closeFile()`
2. `closeFile()`方法又尝试获取同一个锁
3. 导致同一个线程试图获取已持有的锁，造成死锁

###️ **修复策略**：

#### 1. **重构锁管理**：
```cpp
// 修复前：死锁
void openFile() {
    QMutexLocker locker(&m_mutex);  // 获取锁
    closeFile();  // 调用closeFile()
}

void closeFile() {
    QMutexLocker locker(&m_mutex);  // 再次获取锁 → 死锁
}

// 修复后：避免死锁
void openFile() {
    closeFile();  // 先调用，不在这里获取锁
    QMutexLocker locker(&m_mutex);  // 然后获取锁
}

void closeFile() {
    bool needLock = !m_mutex.tryLock();  // 检查是否已持有锁
    if (needLock) {
        QMutexLocker locker(&m_mutex);
        closeFileInternal();
    } else {
        m_mutex.unlock();
        closeFileInternal();
    }
}
```

#### 2. **添加内部方法**：
- `closeFileInternal()`：实际执行关闭操作
- `stopDecodingInternal()`：实际执行停止操作
- 这些方法可以安全地在已持有锁的情况下调用

#### 3. **增强异常处理**：
- 在所有关键方法中添加了`try-catch`块
- 使用`tryLock()`检查锁状态，避免死锁

### **测试建议**：

现在请测试：
1. **启动程序**：确认初始化正常
2. **播放音乐**：应该不会崩溃，会看到详细的日志
3. **检查日志**：应该看到类似以下的日志：
   ```
   FFmpegDecoder: 开始关闭文件...
   FFmpegDecoder: 执行内部关闭文件操作...
   FFmpegDecoder: 停止解码...
   FFmpegDecoder: 执行内部停止解码操作...
   FFmpegDecoder: 设置解码状态为停止...
   FFmpegDecoder: 解码停止完成（模拟模式）
   FFmpegDecoder: 清理FFmpeg资源...
   FFmpegDecoder: 重置状态...
   FFmpegDecoder: 文件关闭完成
   ```

如果现在不崩溃了，我们就可以继续第三步：重新启用平衡控制功能。

请测试一下，告诉我结果！


---------------------


看起来崩溃仍然发生在`stopDecodingInternal()`方法中。让我进一步分析问题。崩溃可能发生在`QMutexLocker locker(&m_mutex)`这一行，因为我们在`closeFile()`中已经检查了锁状态，但在`stopDecodingInternal()`中又尝试获取锁。

让我修复这个问题：

现在让我也修复`closeFileInternal`方法，确保它也不会重复获取锁：

现在让我重新设计一个更简单、更安全的方案。让我完全重写`closeFile`方法，避免复杂的锁管理：

现在让我也简化`stopDecoding`方法：

现在让我从头文件中移除不再需要的方法声明：

现在让我们测试修复后的代码。请运行程序，看看是否还会崩溃。

## ✅ **简化锁管理修复总结**

### 🔍 **问题根源**：
之前的修复方案过于复杂，引入了新的死锁问题：
1. **复杂的锁检查逻辑**：`tryLock()`和锁状态检查增加了复杂性
2. **内部方法调用**：`closeFileInternal()`和`stopDecodingInternal()`仍然存在锁管理问题
3. **递归锁获取**：在已经持有锁的情况下尝试再次获取锁

###️ **修复策略**：

#### 1. **简化锁管理**：
```cpp
// 修复前：复杂的锁检查
void closeFile() {
    bool needLock = !m_mutex.tryLock();
    if (needLock) {
        QMutexLocker locker(&m_mutex);
        closeFileInternal();
    } else {
        m_mutex.unlock();
        closeFileInternal();
    }
}

// 修复后：简单的锁管理
void closeFile() {
    QMutexLocker locker(&m_mutex);
    // 直接在这里执行所有操作
    if (m_isDecoding.loadAcquire()) {
        m_isDecoding.storeRelease(0);
    }
    cleanupFFmpeg();
    resetState();
}
```

#### 2. **避免方法调用链**：
- 移除了`closeFileInternal()`和`stopDecodingInternal()`方法
- 在`closeFile()`中直接执行所有清理操作
- 避免了方法间的锁传递问题

#### 3. **保持简单性**：
- 每个方法只获取一次锁
- 避免复杂的锁状态检查
- 减少异常处理的复杂性

### **测试建议**：

现在请测试：
1. **启动程序**：确认初始化正常
2. **播放音乐**：应该不会崩溃，会看到简化的日志
3. **检查日志**：应该看到类似以下的日志：
   ```
   FFmpegDecoder: 开始关闭文件...
   FFmpegDecoder: 停止解码...
   FFmpegDecoder: 设置解码状态为停止...
   FFmpegDecoder: 解码停止完成（模拟模式）
   FFmpegDecoder: 清理FFmpeg资源...
   FFmpegDecoder: 重置状态...
   FFmpegDecoder: 文件关闭完成
   ```

如果现在不崩溃了，我们就可以继续第三步：重新启用平衡控制功能。

请测试一下，告诉我结果！

