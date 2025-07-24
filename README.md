## 🎵 Qt6音频播放器项目

基于Qt6和C++17开发的现代化音频播放器应用程序，采用分层架构和组件化设计，集成了先进的标签管理系统、FFmpeg音频处理技术和性能优化技术。

### 🔗 **项目信息**

- **名称**: Qt6音频播放器 (`MusicPlayHandle`)
- **版本**: 0.3.0 (主版本.次版本.补丁)
- **组织**: Qt6音频播放器开发团队
- **域名**: musicPlayHandle.qt6.org
- **描述**: Modern Qt6 Music Player with Advanced Tag Management and FFmpeg Audio Processing
- **构建系统**: qmake (主要) / CMake (兼容)
- **Qt版本**: Qt6.5.3+ (MinGW 64-bit)
- **语言标准**: C++17
- **开发环境**: Qt Creator 11.0.0+
- **最后更新**: 2025年07月24日 (FFmpeg音频处理集成)
- **主要功能**: 音频播放、标签管理、播放历史、精确进度控制、界面同步、FFmpeg音频处理
- **特色技术**: PreciseSlider、多线程音频处理、播放历史管理、UI控件冲突解决、界面同步机制、FFmpeg实时音频处理

### 🏗️ **项目架构**

本项目采用分层架构设计，包含以下核心组件：

#### 1. **应用程序管理层**
- **`ApplicationManager`** - 应用程序生命周期管理
- **`ComponentIntegration`** - 组件集成管理器
- **`ServiceContainer`** - 依赖注入容器

#### 2. **UI控制层**
- **控制器模块**
  - `MainWindowController` - 主窗口控制器
  - `AddSongDialogController` - 添加歌曲对话框控制器
  - `ManageTagDialogController` - 标签管理对话框控制器
  - `PlayInterfaceController` - 播放界面控制器
- **对话框模块**
  - `AddSongDialog` - 添加歌曲对话框
  - `CreateTagDialog` - 创建标签对话框
  - `ManageTagDialog` - 标签管理对话框
  - `PlayInterface` - 音乐播放界面
  - `SettingsDialog` - 应用程序设置对话框
- **自定义控件**
  - `MusicProgressBar` - 自定义音乐进度条（包含PreciseSlider）
  - `PreciseSlider` - 精确滑块组件（毫秒级精度控制）
  - `TagListItem` - 标签列表项控件
  - `TagListItemFactory` - 标签列表项工厂
  - `RecentPlayListItem` - 最近播放列表项组件

#### 3. **业务逻辑层**
- **音频引擎**
  - `AudioEngine` - 音频播放引擎 (支持多格式、播放控制、音效处理)
  - `FFmpegDecoder` - FFmpeg音频解码器 (实时音频处理、平衡控制、VU表)
- **管理器模块**
  - `TagManager` - 标签管理器 (标签CRUD、关联管理)
  - `PlaylistManager` - 播放列表管理器 (播放列表、历史记录、收藏)
- **接口抽象层**
  - `ITagManager` - 标签管理器接口
  - `IDatabaseManager` - 数据库管理器接口

#### 4. **线程管理层**
- **`MainThreadManager`** - 主线程管理器
- **`AudioWorkerThread`** - 音频工作线程

#### 5. **数据层**
- **数据库管理**
  - `DatabaseManager` - 数据库连接和事务管理
  - `BaseDao` - DAO基类，提供通用数据访问功能
- **数据访问对象 (DAO)**
  - `SongDao` - 歌曲数据访问
  - `TagDao` - 标签数据访问
  - `PlaylistDao` - 播放列表数据访问
  - `PlayHistoryDao` - 播放历史数据访问
  - `LogDao` - 日志数据访问
- **数据模型**
  - `Song` - 歌曲信息模型
  - `Tag` - 标签信息模型
  - `Playlist` - 播放列表模型
  - `PlayHistory` - 播放历史模型
  - `ErrorLog` - 错误日志模型
  - `SystemLog` - 系统日志模型

#### 6. **支持组件**
- **日志系统**
  - `Logger` - 基础日志系统
  - `StructuredLogger` - 结构化日志系统 (JSON格式、异步处理)
- **配置管理**
  - `AppConfig` - 应用程序配置管理
  - `TagConfiguration` - 标签配置管理
  - `TagStrings` - 标签字符串管理
- **性能优化组件**
  - `Cache` - LRU缓存系统
  - `LazyLoader` - 延迟加载器
  - `ObjectPool` - 对象池管理
- **工具类**
  - `Result<T>` - 结果处理模板 (错误处理)
  - `ServiceContainer` - 依赖注入容器
  - `Constants` - 系统常量定义
- **删除功能组件**
  - `DeleteMode` - 删除模式枚举
  - 批量删除操作支持
  - 多线程安全删除机制

### 📂 **项目文件结构**
```
MusicPlayHandle/
├── .clang-format            # 代码格式化配置
├── .gitignore               # Git忽略文件配置
├── musicPlayHandle.pro      # qmake项目文件 (主要)
├── musicPlayHandle.pro.user # Qt Creator用户配置
├── README.md                # 项目说明文档
├── version.h                # 版本信息定义
├── main.cpp                 # 程序入口
├── mainwindow.h/cpp         # 主窗口实现
├── MainWindowController_Copy.h/cpp  # 主窗口控制器备份
├── docs/                    # 项目功能实现及问题记录文档目录
├── examples/                # 示例代码
│   └── optimization_usage_example.cpp  # 优化使用示例
├── images/                  # UI图标资源
│   ├── addMusicIcon.png     # 添加音乐图标
│   ├── addToListIcon.png    # 添加到列表图标
│   ├── applicationIcon.png  # 应用程序图标
│   ├── copyIcon.png         # 复制图标
│   ├── createIcon.png       # 创建图标
│   ├── deleteIcon.png       # 删除图标
│   ├── editLabel.png        # 编辑标签图标
│   ├── exit-SaveIcon.png    # 退出保存图标
│   ├── exit-discardChangesIcon.png  # 退出丢弃更改图标
│   ├── fastForwardIcon.png  # 快进图标
│   ├── followingSongIcon.png # 下一首歌图标
│   ├── lastSongIcon.png     # 上一首歌图标
│   ├── listCycle.png        # 列表循环图标
│   ├── manageIcon.png       # 管理图标
│   ├── moveIcon.png         # 移动图标
│   ├── pauseIcon.png        # 暂停图标
│   ├── playIcon.png         # 播放图标
│   ├── playlistIcon.png     # 播放列表图标
│   ├── reverseIcon.png      # 倒退图标
│   ├── shufflePlay.png      # 随机播放图标
│   ├── singleCycle.png      # 单曲循环图标
│   └── undoIcon.png         # 撤销图标
├── src/                     # 源码主目录
│   ├── audio/               # 音频引擎模块
│   │   ├── audioengine.h/cpp  # 音频播放引擎
│   │   ├── ffmpegdecoder.h/cpp # FFmpeg音频解码器
│   │   └── audiotypes.h     # 音频类型定义
│   ├── core/                # 核心功能与优化组件
│   │   ├── applicationmanager.h/cpp  # 应用程序管理
│   │   ├── servicecontainer.h/cpp   # 依赖注入容器
│   │   ├── logger.h/cpp     # 基础日志系统
│   │   ├── structuredlogger.h/cpp   # 结构化日志
│   │   ├── appconfig.h/cpp  # 应用配置管理
│   │   ├── cache.h          # LRU缓存系统
│   │   ├── lazyloader.h/cpp # 延迟加载器
│   │   ├── objectpool.h/cpp # 对象池管理
│   │   ├── result.h         # 结果处理模板
│   │   ├── constants.h      # 系统常量定义
│   │   ├── tagconfiguration.h/cpp   # 标签配置
│   │   ├── tagstrings.h/cpp # 标签字符串管理
│   │   └── componentintegration.h  # 组件集成
│   ├── database/            # 数据库与DAO层
│   │   ├── databasemanager.h/cpp    # 数据库管理
│   │   ├── basedao.h/cpp    # DAO基类
│   │   ├── songdao.h/cpp    # 歌曲数据访问
│   │   ├── tagdao.h/cpp     # 标签数据访问
│   │   ├── playlistdao.h/cpp # 播放列表数据访问
│   │   ├── playhistorydao.h/cpp # 播放历史数据访问
│   │   └── logdao.h/cpp     # 日志数据访问
│   ├── interfaces/          # 接口抽象层
│   │   ├── idatabasemanager.h  # 数据库管理接口
│   │   └── itagmanager.h    # 标签管理接口
│   ├── managers/            # 业务管理器
│   │   ├── tagmanager.h/cpp # 标签管理器
│   │   └── playlistmanager.h/cpp # 播放列表管理器
│   ├── models/              # 数据模型
│   │   ├── song.h/cpp       # 歌曲信息模型
│   │   ├── tag.h/cpp        # 标签信息模型
│   │   ├── playlist.h/cpp   # 播放列表模型
│   │   ├── playhistory.h/cpp # 播放历史模型
│   │   ├── errorlog.h/cpp   # 错误日志模型
│   │   └── systemlog.h/cpp  # 系统日志模型
│   ├── threading/           # 多线程管理
│   │   ├── audioworkerthread.h/cpp  # 音频工作线程
│   │   └── mainthreadmanager.h/cpp  # 主线程管理
│   ├── ui/                  # 用户界面组件
│   │   ├── controllers/     # UI控制器
│   │   │   ├── MainWindowController.h/cpp
│   │   │   ├── AddSongDialogController.h/cpp
│   │   │   ├── ManageTagDialogController.h/cpp
│   │   │   └── PlayInterfaceController.h/cpp
│   │   ├── dialogs/         # 对话框实现
│   │   │   ├── AddSongDialog.h/cpp
│   │   │   ├── CreateTagDialog.h/cpp
│   │   │   ├── ManageTagDialog.h/cpp
│   │   │   ├── PlayInterface.h/cpp
│   │   │   └── SettingsDialog.h/cpp
│   │   └── widgets/         # 自定义控件
│   │       ├── musicprogressbar.h/cpp  # 音乐进度条(PreciseSlider)
│   │       ├── taglistitem.h/cpp       # 标签列表项
│   │       ├── taglistitemfactory.h/cpp # 标签列表项工厂
│   │       └── recentplaylistitem.h/cpp # 最近播放列表项
│   └── utils/               # 工具类
├── tests/                   # 测试代码
├── translations/            # 国际化文件
│   └── en_US.ts            # 英文翻译
├── *.ui                    # Qt Designer UI文件
│   ├── mainwindow.ui        # 主窗口界面
│   ├── playInterface.ui     # 播放界面
│   ├── addSongDialog.ui     # 添加歌曲对话框
│   ├── createtagdialog.ui   # 创建标签对话框
│   ├── manageTagDialog.ui   # 标签管理对话框
│   └── settingsdialog.ui    # 设置对话框
├── icon.qrc                # Qt资源文件
├── third_party/            # 第三方库
│   └── ffmpeg/             # FFmpeg库文件
│       ├── include/        # FFmpeg头文件
│       ├── lib/            # FFmpeg库文件
│       └── bin/            # FFmpeg可执行文件
├── 笔记/                    # 开发笔记
│   └── 智能体角色/          # AI助手角色定义
└── 设计文档/                # 项目设计文档
    ├── 功能设计/            # 功能设计文档 v0.1-0.2
    ├── 多线程设计/          # 多线程设计文档 v0.1-0.2
    ├── 数据库设计/          # 数据库设计文档 v0.1-0.2
    ├── 界面设计/            # UI设计文档和原型图
    │   ├── 主界面.png
    │   ├── 歌曲播放界面.png
    │   ├── 添加歌曲界面.png
    │   └── 管理标签界面.png
    └── 设计模式选择/        # 设计模式应用文档
```

### 🚀 **功能特性**

1. **音频播放系统**
   - ✅ 支持多种音频格式 (MP3, WAV, FLAC, OGG等)
   - ✅ 完整播放控制 (播放/暂停/停止/跳转/音量)
   - ✅ 多种播放模式 (列表循环/单曲循环/随机播放)
   - ✅ 播放列表管理和播放历史
   - ✅ 线程安全的音频处理
   - ✅ 实时播放状态监控
   - ✅ 自定义音乐进度条 (PreciseSlider)
   - ✅ 精确的拖拽和点击控制
   - ✅ 播放历史自动记录
   - ✅ 最近播放列表管理
   - ✅ 界面同步机制 (主界面与播放界面)
   - ✅ **FFmpeg音频解码和处理**
   - ✅ **实时音频平衡控制**
   - ✅ **VU表音频电平显示**
   - ✅ **音频元数据高级解析**
   - ✅ **封面艺术智能提取和缩放**

2. **标签管理系统**
   - ✅ 标签CRUD操作 (创建、读取、更新、删除)
   - ✅ 歌曲-标签关联管理
   - ✅ 系统标签和用户标签分离
   - ✅ 标签配置和字符串管理
   - ✅ 标签统计和搜索功能
   - ✅ 批量操作支持
   - ✅ 完整的单元测试覆盖

3. **播放列表管理**
   - ✅ 播放列表创建和管理
   - ✅ 播放历史记录
   - ✅ 收藏夹功能
   - ✅ 播放列表验证和权限控制
   - ✅ 最近播放列表
   - ✅ 多选删除操作
   - ✅ 删除模式选择
   - ✅ 批量操作支持

4. **多线程架构**
   - ✅ 音频处理独立线程 (`AudioWorkerThread`)
   - ✅ UI线程管理 (`MainThreadManager`)
   - ✅ 线程安全的数据访问
   - ✅ 异步操作和信号槽通信
   - ✅ **FFmpeg解码线程安全处理**

5. **数据持久化**
   - ✅ SQLite数据库存储
   - ✅ 完整的DAO层设计
   - ✅ 数据库事务管理
   - ✅ 歌曲、标签、播放列表数据管理
   - ✅ 错误日志和系统日志
   - ✅ **音频设置持久化存储**

6. **性能优化**
   - ✅ LRU缓存系统 (`Cache`)
   - ✅ 对象池管理 (`ObjectPool`)
   - ✅ 延迟加载机制 (`LazyLoader`)
   - ✅ 结构化日志系统 (JSON格式、异步处理)
   - ✅ 内存管理优化
   - ✅ **FFmpeg资源管理和优化**

7. **架构设计**
   - ✅ 依赖注入容器 (`ServiceContainer`)
   - ✅ 接口抽象层设计
   - ✅ 组件化架构
   - ✅ 统一错误处理 (`Result<T>`)
   - ✅ 配置管理系统

8. **用户界面**
   - ✅ MVC架构的UI控制器
   - ✅ 多个功能对话框
   - ✅ 自定义控件 (标签列表项、音乐进度条)
   - ✅ Qt Designer UI文件
   - ✅ 精确的进度条控制
   - ✅ 最近播放列表项组件
   - ✅ 删除模式选择对话框
   - ✅ 批量操作界面
   - ✅ 界面同步机制
   - ✅ **平衡控制滑块和VU表显示**
   - ✅ **播放界面UI优化和美化**

### 🔧 **构建和运行**

#### 环境要求
- **Qt环境**: Qt 6.5.3+ (推荐使用Qt 6.5.3 MinGW 64-bit)
- **编译器**: MinGW 64-bit (Windows) / MSVC 2019+ (Windows)
- **构建工具**:
  - qmake （推荐）
  - CMake 3.16+ (兼容)
  - Qt Creator 16.0.2
- **第三方库**:
  - **FFmpeg 6.x** (已集成在third_party/ffmpeg目录)

#### 构建步骤

1. **使用Qt Creator构建 (推荐)**
- 启动Qt Creator 11.0.0+
- 打开项目文件 `musicPlayHandle.pro`
- 选择构建套件：
  - Windows: Qt 6.5.3 MinGW 64-bit (推荐)
  - Windows: Qt 6.5.3 MSVC 2019+
  - Linux: Qt 6.5.3 GCC 9.0+
  - macOS: Qt 6.5.3 Clang 10+
- 配置构建模式（Debug/Release）
- 点击构建按钮或按Ctrl+B

#### 🎵 音频播放技术
- **精确进度控制**: 
  - 自定义PreciseSlider组件
  - 毫秒级精度的位置计算
  - 流畅的拖拽和点击体验
  - 拖拽预览绘制功能
- **播放历史管理**: 
  - 自动记录播放历史
  - 最近播放列表功能
  - 播放时间格式化显示
  - 重复歌曲智能去重
  - 播放记录异常更新防护
  - UI控件排序冲突解决
- **多线程音频处理**: 
  - 线程安全的音频操作
  - 后台线程数据库访问
  - UI线程安全更新机制
- **界面同步机制**: 
  - 主界面与播放界面状态同步
  - 播放控制双向同步
  - 音量控制实时同步
  - 进度条状态同步
- **FFmpeg音频处理**:
  - 实时音频解码和重采样
  - 音频平衡控制 (左右声道调节)
  - VU表音频电平实时显示
  - 音频元数据高级解析
  - 封面艺术智能提取
  - 线程安全的音频处理
  - 音频设置持久化存储

#### 🚀 性能优化
- **缓存系统**: 
  - LRU算法的智能缓存
  - 音频数据预加载机制
  - 标签系统缓存优化
- **内存管理**: 
  - 对象池复用技术
  - 智能指针资源管理
  - 内存泄漏检测
- **多线程优化**: 
  - 音频处理独立线程
  - UI响应优先级保证
  - 线程安全的数据访问
- **FFmpeg优化**:
  - FFmpeg资源智能管理
  - 音频解码性能优化
  - 实时音频处理优化

#### 🛠️ 开发工具
- **错误处理**: 
  - Result<T>模板错误处理
  - 异常捕获和恢复机制
  - 错误追踪和日志
- **日志系统**: 
  - 结构化JSON格式
  - 异步写入优化
  - 多级别日志分类
- **测试框架**: 
  - 完整单元测试覆盖
  - Mock对象和依赖注入
  - 自动化测试流程
- **调试技术**: 
  - 分层调试策略（数据库→代码→UI）
  - 详细调试信息输出
  - 问题定位和验证机制

#### 🎵 音频技术
- **格式支持**: 
  - 多格式解码(MP3/WAV/FLAC/OGG)
  - 音频格式转换
  - 元数据解析和编辑
- **播放控制**: 
  - 精确时间定位
  - 实时音量调节
  - 播放状态监控
- **高级特性**: 
  - 智能播放列表
  - 自定义进度条控制
  - 拖拽预览功能
  - 播放历史记录
  - 最近播放管理
  - 播放记录异常更新防护
  - UI控件排序冲突解决
- **删除功能增强**: 
  - 多选删除支持
  - 删除模式选择
  - 批量操作处理
  - 线程安全删除
- **界面同步**: 
  - 主界面与播放界面控制同步
  - 播放状态实时更新
  - 音量控制双向同步
  - 进度条状态同步
  - 播放模式同步
  - 静音状态同步
- **FFmpeg集成**:
  - 实时音频平衡控制
  - VU表音频电平显示
  - 音频元数据高级解析
  - 封面艺术智能提取
  - 音频设置持久化
  - 线程安全音频处理

### 📋 **开发状态**

#### ✅ 已完成功能 (核心架构)
- [x] **项目架构**: 完整的分层架构和组件化设计
- [x] **数据库系统**: SQLite集成、DAO层、事务管理、播放历史记录管理
- [x] **标签管理**: 完整的CRUD操作、系统标签、用户标签、标签配置和字符串管理、标签统计和搜索功能、批量操作支持
- [x] **播放列表**: 播放列表创建和管理、播放历史记录、收藏夹功能、播放列表验证和权限控制、最近播放列表
- [x] **音频引擎**: 多格式支持(MP3/WAV/FLAC/OGG)、完整播放控制(播放/暂停/停止/跳转/音量)、三种播放模式(列表循环/单曲循环/随机播放)、线程安全的音频处理、实时播放状态监控
- [x] **依赖注入**: ServiceContainer IoC容器、组件化架构、接口抽象层设计
- [x] **性能优化**: LRU缓存系统、对象池管理、延迟加载机制、结构化日志系统(JSON格式、异步处理)、内存管理优化
- [x] **错误处理**: Result<T>模板、统一错误管理、异常捕获和处理
- [x] **日志系统**: 基础日志系统、结构化日志(JSON格式)、异步写入、多级别日志、错误日志和系统日志
- [x] **单元测试**: TagManager完整测试覆盖、标签修复测试、调试测试、Mock对象和依赖隔离
- [x] **UI控制器**: MVC架构的控制器层(MainWindowController、AddSongDialogController、ManageTagDialogController、PlayInterfaceController)、事件处理和状态管理
- [x] **UI对话框**: 完整的对话框实现(AddSongDialog、CreateTagDialog、ManageTagDialog、PlayInterface、SettingsDialog)、用户交互优化
- [x] **自定义控件**: 标签列表项控件(TagListItem)和工厂(TagListItemFactory)、音乐进度条(MusicProgressBar)和精确滑块(PreciseSlider)
- [x] **UI资源**: 完整的图标资源集合，包含播放模式图标和控制按钮图标

#### ✅ 已完成功能 (高级特性)
- [x] **最近播放功能**: 
  - [x] 播放历史记录数据库表设计
  - [x] PlayHistoryDao数据访问层
  - [x] 播放时间格式显示("年/月-日/时-分-秒")
  - [x] 按播放时间降序排列
  - [x] 重复歌曲去重(保留最新记录)
  - [x] 最近播放列表限制(100首)
  - [x] 播放历史统计功能
  - [x] 最近播放排序问题修复
  - [x] 播放记录异常更新问题修复
  - [x] 最近播放列表去重优化
  - [x] UI控件自动排序禁用修复
- [x] **删除功能增强**: 
  - [x] 多选删除支持
  - [x] 删除模式选择(从标签移除/彻底删除/删除播放记录)
  - [x] 批量删除操作
  - [x] 后台线程安全删除
  - [x] 删除确认对话框
  - [x] 删除结果反馈
- [x] **进度条优化**: 
  - [x] 自定义PreciseSlider组件
  - [x] 精确点击跳转功能
  - [x] 流畅拖拽体验
  - [x] 拖拽预览绘制
  - [x] 毫秒级精度控制
  - [x] 线程安全的位置更新
  - [x] 事件过滤器机制
- [x] **播放控制优化**: 
  - [x] 播放/暂停状态管理
  - [x] 播放模式切换
  - [x] 音量控制
  - [x] 静音功能
  - [x] 播放历史自动记录
- [x] **多线程安全**: 
  - [x] 后台线程数据库操作
  - [x] UI线程安全更新
  - [x] 信号槽异步通信
  - [x] 线程间数据同步
- [x] **界面同步机制**: 
  - [x] 主界面与播放界面状态同步
  - [x] 播放控制双向同步
  - [x] 音量控制实时同步
  - [x] 进度条状态同步
  - [x] 播放模式同步
  - [x] 静音状态同步
- [x] **FFmpeg音频处理**: 
  - [x] FFmpeg 6.x集成和配置
  - [x] 实时音频解码和重采样
  - [x] 音频平衡控制 (左右声道调节)
  - [x] VU表音频电平实时显示
  - [x] 音频元数据高级解析
  - [x] 封面艺术智能提取和缩放
  - [x] 线程安全的音频处理
  - [x] 音频设置持久化存储
  - [x] 播放界面UI优化和美化

#### 🚧 开发中功能 (UI完善)
- [ ] **UI美化**: 
  - [x] 基础界面布局优化
  - [x] 播放控制界面美化
  - [x] 播放界面UI优化 (平衡控制、VU表)
  - [ ] 主题系统设计和实现
  - [ ] 自定义样式表支持
- [ ] **播放控制**: 
  - [x] 进度条拖动实现
  - [x] 音量控制功能
  - [x] 播放/暂停切换
  - [x] 播放模式切换
  - [x] 音频平衡控制
  - [x] VU表音频电平显示
  - [ ] 播放速度控制
  - [ ] 均衡器支持
- [ ] **播放列表**: 
  - [x] 基础播放列表管理
  - [x] 最近播放列表
  - [x] 播放历史记录
  - [ ] 拖拽排序功能
  - [x] 多选操作支持
  - [ ] 智能播放列表
- [ ] **标签管理**: 
  - [x] 基础标签CRUD
  - [x] 歌曲标签关联管理
  - [x] 批量标签操作
  - [ ] 标签拖拽功能
  - [ ] 标签颜色定制
  - [ ] 标签层级关系
- [ ] **设置界面**: 
  - [x] 基础设置选项
  - [x] 音频平衡设置
  - [ ] 音频输出设置
  - [ ] 快捷键配置
  - [ ] 主题设置
- [ ] **快捷键**: 
  - [x] 基础媒体控制快捷键
  - [ ] 全局快捷键支持
  - [ ] 自定义快捷键

#### 📅 计划功能 (高级特性)
- [ ] **音频分析**: 
  - [x] VU表音频电平显示
  - [ ] 实时频谱显示
  - [ ] 波形图绘制
  - [ ] 音频特征分析
- [ ] **歌词支持**: 
  - [ ] LRC歌词解析
  - [ ] 歌词编辑器
  - [ ] 歌词时间轴调整
- [ ] **插件系统**: 
  - [ ] 插件架构设计
  - [ ] 热插拔支持
  - [ ] 插件市场
- [ ] **网络功能**: 
  - [ ] 在线音乐库集成
  - [ ] 歌单同步功能
  - [ ] 音乐元数据获取
- [ ] **智能推荐**: 
  - [ ] 基于标签的音乐推荐
  - [ ] 播放历史分析
  - [ ] 个性化推荐算法
- [ ] **多语言**: 
  - [ ] 国际化框架集成
  - [ ] 多语言资源管理
  - [ ] 动态语言切换
- [ ] **云同步**: 
  - [ ] 用户数据云存储
  - [ ] 设置同步
  - [ ] 播放列表云备份
- [ ] **移动端**: 
  - [ ] 移动应用架构设计
  - [ ] 跨平台适配
  - [ ] 移动端特性支持

### 🤝 **开发流程**

#### 📝 代码规范
- **代码风格**: 使用项目配置的 clang-format 自动格式化
- **命名规范**: 类名使用 PascalCase，函数和变量使用 camelCase
- **注释要求**: 公共接口必须有详细的文档注释
- **架构原则**: 遵循 SOLID 原则和项目分层架构
- **错误处理**: 使用 `Result<T>` 模板进行错误处理
- **线程安全**: 多线程代码必须保证线程安全

#### 📋 提交规范
使用 [Conventional Commits](https://www.conventionalcommits.org/) 规范：
- `feat:` 新功能
- `fix:` 错误修复
- `docs:` 文档更新
- `style:` 代码格式调整
- `refactor:` 代码重构
- `test:` 测试相关
- `chore:` 构建过程或辅助工具的变动

### 📚 **项目文档**

详细的项目文档和设计资料：

#### 📖 核心文档
- **[README.md](README.md)** - 项目概述和快速开始
- **[设计文档/](设计文档/)** - 详细的架构设计和技术方案
  - `功能设计/` - 功能模块设计文档
  - `多线程设计/` - 多线程架构和实现
  - `数据库设计/` - 数据库表结构和关系
  - `界面设计/` - UI/UX 设计规范和原型
  - `设计模式选择/` - 设计模式应用文档

#### 🔧 开发文档
- **[docs/](docs/)** - 实现功能及问题记录文档目录

#### 📝 学习资料
- **[笔记/](笔记/)** - 开发笔记和学习记录
- **[examples/](examples/)** - 代码示例和使用案例
- **[tests/](tests/)** - 单元测试和测试文档

#### 🎨 资源文件
- **[images/](images/)** - 项目图标和界面截图
- **[translations/](translations/)** - 多语言翻译文件
- **[*.ui](*.ui)** - Qt Designer UI界面文件
- **[third_party/](third_party/)** - 第三方库文件 (FFmpeg等)

---

**MusicPlayHandle** - 基于现代 Qt6 技术栈的智能音频播放器 🎵✨

*让音乐播放更智能、更高效、更优雅！*