## 🎵 Qt6音频播放器项目

基于Qt6和C++17开发的音频播放器应用程序，采用现代化的分层架构和组件化设计。

### 🔗 **项目信息**

- **版本**: 0.1.0
- **构建系统**: CMake / qmake  # 支持 CMake 和 qmake 双构建系统
- **Qt版本**: Qt6 (Qt_6_5_3_MinGW_64_bit)  # 依赖 Qt6 及其 MinGW 64位工具链
- **语言标准**: C++17  # 采用 C++17 标准
- **开发环境**: Qt Creator  # 推荐使用 Qt Creator 进行开发

### 🏗️ **项目架构**

本项目采用分层架构设计，包含以下核心组件：

#### 1. **应用程序管理层**  <!-- 管理应用生命周期、配置、国际化等 -->
- **`ApplicationManager`** - 应用程序生命周期管理
- **`ComponentIntegration`** - 组件集成管理器
- **`ServiceContainer`** - 依赖注入容器

#### 2. **UI控制层**  <!-- 负责界面事件、用户交互、UI逻辑分发 -->
- **`MainWindowController`** - 主窗口控制器
- **`AddSongDialogController`** - 添加歌曲对话框控制器
- **`ManageTagDialogController`** - 标签管理对话框控制器
- **`PlayInterfaceController`** - 播放界面控制器

#### 3. **业务逻辑层**  <!-- 负责音频播放、标签等核心业务 -->
- **`AudioEngine`** - 音频播放引擎
- **`TagManager`** - 标签管理器
- **接口层**
  - `ITagManager` - 标签管理器接口
  - `IDatabaseManager` - 数据库管理器接口

#### 4. **线程管理层**  <!-- 负责多线程任务调度与数据同步 -->
- **`MainThreadManager`** - 主线程管理器
- **`AudioWorkerThread`** - 音频工作线程

#### 5. **数据层**  <!-- 负责数据持久化、数据库访问、数据模型 -->
- **`DatabaseManager`** - 数据库管理
- **`DatabaseTransaction`** - 数据库事务管理
- **数据访问对象 (DAO)**
  - `SongDao` - 歌曲数据访问
  - `TagDao` - 标签数据访问
  - `LogDao` - 日志数据访问
- **数据模型**
  - `Song` - 歌曲信息模型
  - `Tag` - 标签信息模型
  - `Playlist` - 播放列表模型
  - `PlayHistory` - 播放历史模型
  - `ErrorLog` - 错误日志模型
  - `SystemLog` - 系统日志模型

#### 6. **支持组件**  <!-- 日志、配置等通用基础设施 -->
- **`Logger`** - 日志系统
- **`StructuredLogger`** - 结构化日志系统
- **`AppConfig`** - 配置管理
- **`Cache`** - 缓存系统
- **`LazyLoader`** - 延迟加载器
- **`ObjectPool`** - 对象池
- **`Result`** - 结果处理模板
- **`TagConfiguration`** - 标签配置管理
- **`TagStrings`** - 标签字符串管理

### 📂 **项目文件结构**

```
musicPlayHandle/  # 项目根目录
├── .cursor/  # Cursor IDE 配置目录
│   └── rules/  # 开发规则配置
│       └── Qt Project Development Rule.mdc  # Qt项目开发规范文档
├── .vscode/  # Visual Studio Code 配置目录
│   ├── c_cpp_properties.json  # C/C++ 语言配置
│   ├── settings.json  # 编辑器设置
│   └── tasks.json  # 构建任务配置
├── build/  # 构建输出目录
│   └── Desktop_Qt_6_5_3_MinGW_64_bit-Debug/  # Debug构建目录
│       ├── .qtc_clangd/  # Qt Creator clangd 配置
│       ├── debug/  # Debug 构建产物
│       └── release/  # Release 构建产物
├── src/  # 源码主目录
│   ├── core/  # 应用管理与基础设施
│   │   ├── applicationmanager.h/cpp  # 应用程序生命周期管理
│   │   ├── componentintegration.h  # 组件集成管理器
│   │   ├── servicecontainer.h/cpp  # 依赖注入容器
│   │   ├── logger.h/cpp  # 日志系统实现
│   │   ├── structuredlogger.h/cpp  # 结构化日志系统
│   │   ├── appconfig.h/cpp  # 应用配置管理
│   │   ├── cache.h  # 缓存系统
│   │   ├── lazyloader.h/cpp  # 延迟加载器
│   │   ├── objectpool.h/cpp  # 对象池
│   │   ├── result.h  # 结果处理模板
│   │   ├── constants.h  # 常量定义
│   │   ├── tagconfiguration.h/cpp  # 标签配置管理
│   │   └── tagstrings.h/cpp  # 标签字符串管理
│   ├── audio/  # 音频引擎相关
│   │   ├── audioengine.h/cpp  # 音频播放引擎核心
│   │   └── audiotypes.h  # 音频相关数据类型定义
│   ├── threading/  # 多线程相关
│   │   ├── mainthreadmanager.h  # 主线程管理器
│   │   └── audioworkerthread.h  # 音频工作线程
│   ├── interfaces/  # 接口定义
│   │   ├── itagmanager.h  # 标签管理器接口
│   │   └── idatabasemanager.h  # 数据库管理器接口
│   ├── managers/  # 业务管理器
│   │   ├── tagmanager.h/cpp  # 标签管理器实现
│   │   └── playlistmanager.h  # 播放列表管理器
│   ├── database/  # 数据库与DAO层
│   │   ├── databasemanager.h/cpp  # 数据库连接管理
│   │   ├── databasetransaction.h  # 数据库事务管理
│   │   ├── songdao.h/cpp  # 歌曲数据访问对象
│   │   ├── tagdao.h/cpp  # 标签数据访问对象
│   │   └── logdao.h/cpp  # 日志数据访问对象
│   ├── models/  # 数据模型层
│   │   ├── song.h/cpp  # 歌曲信息模型
│   │   ├── tag.h/cpp  # 标签信息模型
│   │   ├── playlist.h/cpp  # 播放列表模型
│   │   ├── playhistory.h/cpp  # 播放历史模型
│   │   ├── errorlog.h/cpp  # 错误日志模型
│   │   └── systemlog.h/cpp  # 系统日志模型
│   └── ui/  # 用户界面层
│       ├── controllers/  # UI控制器
│       │   ├── MainWindowController.h/cpp  # 主窗口控制器
│       │   ├── AddSongDialogController.h/cpp  # 添加歌曲对话框控制器
│       │   ├── PlayInterfaceController.h/cpp  # 播放界面控制器
│       │   └── ManageTagDialogController.h/cpp  # 标签管理对话框控制器
│       ├── dialogs/  # 对话框实现
│       │   ├── AddSongDialog.h/cpp  # 添加歌曲对话框
│       │   ├── CreateTagDialog.h/cpp  # 创建标签对话框
│       │   ├── ManageTagDialog.h/cpp  # 管理标签对话框
│       │   ├── PlayInterface.h/cpp  # 播放界面对话框
│       │   └── SettingsDialog.h/cpp  # 设置对话框
│       └── widgets/  # 自定义控件
│           ├── taglistitem.h/cpp  # 标签列表项自定义控件
│           └── taglistitemfactory.h  # 标签列表项工厂
├── images/  # 图标资源目录
│   ├── addMusicIcon.png  # 添加音乐图标
│   ├── addToListIcon.png  # 添加到列表图标
│   ├── applicationIcon.png  # 应用程序图标
│   ├── copyIcon.png  # 复制操作图标
│   ├── createIcon.png  # 创建操作图标
│   ├── deleteIcon.png  # 删除操作图标
│   ├── editLabel.png  # 编辑标签图标
│   ├── exit-SaveIcon.png  # 保存退出图标
│   ├── exit-discardChangesIcon.png  # 放弃更改退出图标
│   ├── fastForwardIcon.png  # 快进图标
│   ├── followingSongIcon.png  # 下一首歌图标
│   ├── lastSongIcon.png  # 上一首歌图标
│   ├── manageIcon.png  # 管理操作图标
│   ├── moveIcon.png  # 移动操作图标
│   ├── pauseIcon.png  # 暂停播放图标
│   ├── playIcon.png  # 播放图标
│   ├── playlistIcon.png  # 播放列表图标
│   ├── reverseIcon.png  # 倒退图标
│   ├── stopIcon.png  # 停止播放图标
│   └── undoIcon.png  # 撤销操作图标
├── docs/  # 项目文档
│   └── OPTIMIZATION_GUIDE.md  # 优化指南文档
├── examples/  # 示例代码
│   └── optimization_usage_example.cpp  # 优化使用示例
├── tests/  # 测试代码
│   ├── test_tagmanager.cpp  # 标签管理器测试
│   └── test_tagmanager.h  # 标签管理器测试头文件
├── translations/  # 国际化翻译文件
│   └── en_US.ts  # 英文翻译文件
├── 笔记/  # 开发笔记目录
│   └── 智能体角色/  # AI助手角色定义
│       ├── Qt 6 开发专家.md  # Qt6开发专家角色
│       └── Qt 6 报错解决专家.md  # Qt6问题解决专家角色
├── 设计文档/  # 项目设计文档
│   ├── 功能设计/  # 功能设计文档
│   │   └── 功能设计0.1.md  # 功能设计规范v0.1
│   ├── 多线程设计/  # 多线程架构设计
│   │   ├── 多线程设计0.1.md  # 多线程设计文档v0.1
│   │   └── 多线程设计0.2.md  # 多线程设计文档v0.2
│   ├── 数据库设计/  # 数据库设计文档
│   │   ├── 数据库设计0.1.md  # 数据库设计规范v0.1
│   │   └── 数据库设计0.2.md  # 数据库设计规范v0.2
│   ├── 界面设计/  # UI界面设计
│   │   ├── 主界面.png  # 主界面设计图
│   │   ├── 歌曲播放界面.png  # 播放界面设计图
│   │   ├── 添加歌曲界面.png  # 添加歌曲界面设计图
│   │   └── 管理标签界面.png  # 标签管理界面设计图
│   └── 设计模式选择/  # 设计模式文档
│       ├── 设计模式0.1.md  # 设计模式选择文档v0.1
│       └── 设计模式0.2.md  # 设计模式选择文档v0.2
├── main.cpp  # 程序入口点
├── mainwindow.h/cpp  # 主窗口类实现
├── version.h  # 版本信息定义
├── *.ui  # Qt Designer 可视化界面文件
│   ├── mainwindow.ui  # 主窗口界面设计
│   ├── addSongDialog.ui  # 添加歌曲对话框界面
│   ├── createtagdialog.ui  # 创建标签对话框界面
│   ├── manageTagDialog.ui  # 管理标签对话框界面
│   ├── playInterface.ui  # 播放界面设计
│   └── settingsdialog.ui  # 设置对话框界面
├── icon.qrc  # Qt 资源文件定义
├── CMakeLists.txt  # CMake 项目配置文件
├── musicPlayHandle.pro  # qmake 项目配置文件
├── musicPlayHandle.pro.user  # Qt Creator 用户配置
├── musicPlayHandle_resource.rc  # Windows 资源文件
├── PROJECT_OPTIMIZATION_SUMMARY.md  # 项目优化总结
├── .clang-format  # 代码格式化配置
├── .qmake.stash  # qmake 缓存文件
├── Makefile*  # 生成的构建文件
├── object_script.*  # 目标文件脚本
├── models/  # 空的模型目录（历史遗留）
└── README.md  # 项目说明文档
```

### 🚀 **功能特性**

1. **音频播放系统**  <!-- 支持多格式音频播放与控制 -->
   - 支持多种音频格式 (MP3, WAV, FLAC, OGG等)
   - 播放控制 (播放/暂停/停止/跳转)
   - 音量控制和静音
   - 音频效果处理 (均衡器/混响/平衡/交叉淡入淡出)

2. **标签管理系统**  <!-- 支持标签的增删改查与批量操作 -->
   - 标签创建、编辑、删除
   - 歌曲-标签关联管理
   - 标签配置和字符串管理
   - 标签过滤和搜索
   - 批量操作支持
   - 撤销/重做功能

3. **多线程架构**    <!-- 音频与UI分离，保证流畅体验 -->
   - 音频处理独立线程 (`AudioWorkerThread`)
   - UI线程管理 (`MainThreadManager`)
   - 线程安全的数据访问
   - 批量UI更新和延迟处理

4. **数据持久化**    <!-- SQLite数据库，数据安全可靠 -->
   - SQLite数据库存储
   - 事务管理 (`DatabaseTransaction`)
   - 歌曲信息管理
   - 播放历史和日志
   - 结构化日志系统

5. **性能优化**      <!-- 缓存、对象池、延迟加载等 -->
   - LRU缓存系统
   - 对象池管理
   - 延迟加载机制
   - 内存优化

6. **系统集成**      <!-- 托盘、自动启动、多语言等 -->
   - 系统托盘支持
   - 自动启动选项
   - 多语言支持
   - 组件集成管理
   - 依赖注入容器

### 🔧 **构建和运行**

#### 环境要求
- Qt 6.5.3 或更高版本  # 推荐使用官方 Qt 6.5.3 及以上
- MinGW 64-bit 编译器  # Windows 下推荐 MinGW 64 位
- C++17 支持           # 项目代码基于 C++17 标准
- CMake 3.16+ 或 qmake # 支持 CMake 和 qmake 双构建系统

#### 使用 CMake 构建
```bash
# 创建构建目录
mkdir build && cd build

# 配置项目
cmake ..

# 编译项目
cmake --build .

# 运行测试
cmake --build . --target test
```

#### 使用 qmake 构建
```bash
# 生成 Makefile
qmake musicPlayHandle.pro

# 编译项目
make
```

### 🎯 **技术亮点**

- **现代Qt6特性**: 充分利用Qt6的新功能和性能改进
- **C++17标准**: 使用现代C++17特性提升代码质量
- **组件化架构**: 松耦合、高内聚的模块化设计
- **依赖注入**: `ServiceContainer` 实现IoC容器模式
- **接口抽象**: `ITagManager`、`IDatabaseManager` 等接口设计
- **线程安全**: 使用QMutex、QMutexLocker和线程安全的数据结构
- **信号槽机制**: 高效的组件间通信
- **设计模式**: 单例、工厂、观察者、RAII、结果处理等模式
- **性能优化**: LRU缓存、对象池、延迟加载等优化策略
- **错误处理**: `Result<T>` 模板实现统一错误处理
- **事务管理**: `DatabaseTransaction` 实现RAII数据库事务
- **日志系统**: 多级日志、结构化日志和异步处理
- **测试支持**: 单元测试框架集成
- **代码质量**: clang-format 代码格式化配置

### 📋 **开发状态**

当前版本为 **0.1.0**，项目正在开发中。主要架构已完成，包含：
- ✅ 完整的应用程序管理框架 (`ApplicationManager`、`ComponentIntegration`)
- ✅ 依赖注入容器 (`ServiceContainer`)
- ✅ 音频播放引擎 (`AudioEngine`、`AudioWorkerThread`)
- ✅ 标签管理系统 (`TagManager`、标签配置)
- ✅ 数据库层设计 (`DatabaseManager`、DAO层、事务管理)
- ✅ 多线程架构 (`MainThreadManager`、`AudioWorkerThread`)
- ✅ 基础UI框架 (控制器、对话框、自定义控件)
- ✅ 性能优化组件 (缓存、对象池、延迟加载)
- ✅ 日志系统 (`Logger`、`StructuredLogger`)
- ✅ 测试框架集成

### 🤝 **贡献指南**

欢迎贡献代码！请遵循以下规范：
1. 使用C++17标准和Qt代码风格  # 统一风格，便于维护
2. 使用 `.clang-format` 配置进行代码格式化
3. 添加适当的注释和文档  # 便于团队协作和后期维护
4. 遵循现有的架构模式和设计原则
5. 确保线程安全和异常安全
6. 编写单元测试覆盖新功能
7. 使用 `Result<T>` 模板进行错误处理

### 📄 **许可证**

本项目采用适当的开源许可证。具体许可证信息请参考项目根目录的LICENSE文件。