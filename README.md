## 🎵 Qt6音频播放器项目

基于Qt6和C++17开发的现代化音频播放器应用程序，采用分层架构和组件化设计，集成了先进的标签管理系统和性能优化技术。

### 🔗 **项目信息**

- **名称**: Qt6音频播放器 (`MusicPlayHandle`)
- **版本**: 0.1.0 (主版本.次版本.补丁)
- **组织**: Qt6音频播放器开发团队
- **域名**: musicPlayHandle.qt6.org
- **描述**: Modern Qt6 Music Player with Advanced Tag Management
- **构建系统**: qmake (主要) / CMake (兼容)
- **Qt版本**: Qt6.5.3+ (MinGW 64-bit)
- **语言标准**: C++17
- **开发环境**: Qt Creator

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
  - `TagListItem` - 标签列表项控件
  - `TagListItemFactory` - 标签列表项工厂

#### 3. **业务逻辑层**
- **音频引擎**
  - `AudioEngine` - 音频播放引擎 (支持多格式、播放控制、音效处理)
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

### 📂 **项目文件结构**
```
MusicPlayHandle/
├── .clang-format             # 代码格式化配置
├── .gitignore               # Git忽略文件配置
├── musicPlayHandle.pro      # qmake项目文件 (主要)
├── CMakeLists.txt           # CMake构建配置 (兼容)
├── README.md                # 项目说明文档
├── version.h                # 版本信息定义
├── main.cpp                 # 程序入口
├── mainwindow.h/cpp         # 主窗口实现
├── docs/                    # 项目文档
│   ├── 拖拽与点击进度条-最终解决及优化方案.md  # 进度条优化方案
│   ├── 播放与暂停不可控问题.md              # 播放控制问题解决
│   ├── PROJECT_OPTIMIZATION_SUMMARY.md      # 项目优化总结
│   ├── OPTIMIZATION_GUIDE.md                # 详细优化指南
│   ├── GITHUB_UPLOAD_GUIDE.md              # GitHub上传指南
│   └── 其他技术文档...
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
│   └── ui/                  # 用户界面组件
│       ├── controllers/     # UI控制器
│       │   ├── MainWindowController.h/cpp
│       │   ├── AddSongDialogController.h/cpp
│       │   ├── ManageTagDialogController.h/cpp
│       │   └── PlayInterfaceController.h/cpp
│       ├── dialogs/         # 对话框实现
│       │   ├── AddSongDialog.h/cpp
│       │   ├── CreateTagDialog.h/cpp
│       │   ├── ManageTagDialog.h/cpp
│       │   ├── PlayInterface.h/cpp
│       │   └── SettingsDialog.h/cpp
│       └── widgets/         # 自定义控件
│           ├── musicprogressbar.h/cpp  # 音乐进度条
│           ├── taglistitem.h/cpp
│           └── taglistitemfactory.h/cpp
├── tests/                   # 测试代码
│   ├── test_tagmanager.h/cpp  # 标签管理完整测试套件
│   ├── test_tag_fix.cpp     # 标签修复测试
│   ├── test_play_pause.cpp  # 播放暂停测试
│   ├── test_progress_bar_fix.cpp # 进度条修复测试
│   └── debug_test.cpp       # 调试测试
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
├── 笔记/                    # 开发笔记
│   └── 智能体角色/          # AI助手角色定义
│       ├── Qt 6 MusicPlayHandle 开发助手.md
│       ├── Qt 6 开发专家.md
│       └── Qt 6 报错解决专家.md
└── 设计文档/                # 项目设计文档
    ├── 功能设计/            # 功能设计文档 v0.1-0.2
    │   ├── 功能设计0.1.md
    │   └── 功能设计0.2.md
    ├── 多线程设计/          # 多线程设计文档 v0.1-0.2
    │   ├── 多线程设计0.1.md
    │   └── 多线程设计0.2.md
    ├── 数据库设计/          # 数据库设计文档 v0.1-0.2
    │   ├── 数据库设计0.1.md
    │   └── 数据库设计0.2.md
    ├── 界面设计/            # UI设计文档和原型图
    │   ├── 主界面.png
    │   ├── 歌曲播放界面.png
    │   ├── 添加歌曲界面.png
    │   └── 管理标签界面.png
    └── 设计模式选择/        # 设计模式应用文档
        ├── 设计模式0.1.md
        └── 设计模式0.2.md
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

4. **多线程架构**
   - ✅ 音频处理独立线程 (`AudioWorkerThread`)
   - ✅ UI线程管理 (`MainThreadManager`)
   - ✅ 线程安全的数据访问
   - ✅ 异步操作和信号槽通信

5. **数据持久化**
   - ✅ SQLite数据库存储
   - ✅ 完整的DAO层设计
   - ✅ 数据库事务管理
   - ✅ 歌曲、标签、播放列表数据管理
   - ✅ 错误日志和系统日志

6. **性能优化**
   - ✅ LRU缓存系统 (`Cache`)
   - ✅ 对象池管理 (`ObjectPool`)
   - ✅ 延迟加载机制 (`LazyLoader`)
   - ✅ 结构化日志系统 (JSON格式、异步处理)
   - ✅ 内存管理优化

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

### 🔧 **构建和运行**

#### 环境要求
- **Qt环境**:
  - Qt 6.5.3+ (推荐使用Qt 6.5.3 MinGW 64-bit)
  - Qt Creator 11.0.0+
  - Qt多媒体模块和SQL模块
- **编译器**:
  - MinGW 64-bit (Windows)
  - MSVC 2019+ (Windows)
  - GCC 9.0+ (Linux)
  - Clang 10+ (macOS)
- **构建工具**:
  - qmake （推荐）
  - CMake 3.16+ (兼容)
  - Qt Creator 16.0.2

#### 构建步骤

1. **获取源码**
```bash
# 克隆仓库
git clone https://github.com/K-irito02/musicPlayHandle.git
cd musicPlayHandle

# 更新子模块（如果有）
git submodule update --init --recursive
```

2. **使用Qt Creator构建**
- 启动Qt Creator
- 打开项目文件 `musicPlayHandle.pro` (推荐) 或 `CMakeLists.txt`
- 选择构建套件：
  - Windows: MinGW 64-bit 或 MSVC 2019+
  - Linux: GCC 9.0+
  - macOS: Clang 10+
- 配置构建模式（Debug/Release）
- 点击构建按钮或按Ctrl+B

3. **命令行构建**

**使用 qmake (推荐):**
```bash
# Windows (MinGW)
qmake
mingw32-make -j4

# Windows (MSVC)
qmake
nmake

# Linux/macOS
qmake
make -j4
```

**使用 CMake:**
```bash
# 创建并进入构建目录
mkdir build && cd build

# 配置项目
cmake ..

# 编译项目
cmake --build . --config Release

# 运行程序
.\MusicPlayHandle.exe  # Windows
./MusicPlayHandle      # Linux/macOS
```

#### 依赖检查
- 确保Qt多媒体模块已安装
- 检查系统音频驱动
- 验证SQLite支持
- 确认编解码器支持（MP3/WAV/FLAC/OGG）

#### 常见问题
- 如果遇到找不到Qt库的错误，检查环境变量设置
- 编解码器缺失问题，安装相应的Qt多媒体编解码器
- 数据库连接错误，检查SQLite驱动
- 音频播放问题，验证系统音频设置

### 🎯 **技术亮点**

#### 🏗️ 架构设计
- **现代Qt6特性**: 
  - 充分利用Qt6的新功能和性能改进
  - 基于Qt6.5.3+的最新特性
  - 现代化UI组件和控件
- **C++17标准**: 
  - 智能指针自动内存管理
  - Lambda表达式和函数式编程
  - 模板元编程优化
- **分层架构**: 
  - UI控制层、业务逻辑层、数据层分离
  - MVC模式的控制器设计
  - 插件化架构支持
- **组件化设计**: 
  - 松耦合、高内聚的模块化架构
  - 可复用的自定义控件
  - 标准化的组件接口

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

### 📋 **开发状态**

#### ✅ 已完成功能 (核心架构)
- [x] **项目架构**: 完整的分层架构和组件化设计
- [x] **数据库系统**: SQLite集成、DAO层、事务管理
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

#### 🚧 开发中功能 (UI完善)
- [ ] **UI美化**: 
  - [x] 基础界面布局优化
  - [x] 播放控制界面美化
  - [ ] 主题系统设计和实现
  - [ ] 自定义样式表支持
- [ ] **播放控制**: 
  - [x] 进度条拖动实现
  - [x] 音量控制功能
  - [ ] 播放速度控制
  - [ ] 均衡器支持
- [ ] **播放列表**: 
  - [x] 基础播放列表管理
  - [ ] 拖拽排序功能
  - [ ] 多选操作支持
  - [ ] 智能播放列表
- [ ] **标签管理**: 
  - [x] 基础标签CRUD
  - [ ] 标签拖拽功能
  - [ ] 标签颜色定制
  - [ ] 标签层级关系
- [ ] **设置界面**: 
  - [x] 基础设置选项
  - [ ] 音频输出设置
  - [ ] 快捷键配置
  - [ ] 主题设置
- [ ] **快捷键**: 
  - [x] 基础媒体控制快捷键
  - [ ] 全局快捷键支持
  - [ ] 自定义快捷键

#### 📅 计划功能 (高级特性)
- [ ] **音频分析**: 
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

### 🤝 **贡献指南**

欢迎参与 MusicPlayHandle 项目的开发！请遵循以下步骤：

#### 🔧 开发流程
1. **Fork 仓库**: 点击右上角 Fork 按钮
2. **克隆项目**: `git clone https://github.com/your-username/musicPlayHandle.git`
3. **创建分支**: `git checkout -b feature/your-feature-name`
4. **开发功能**: 按照项目规范编写代码
5. **测试验证**: 确保所有测试通过
6. **提交代码**: `git commit -m "feat: add your feature description"`
7. **推送分支**: `git push origin feature/your-feature-name`
8. **创建 PR**: 在 GitHub 上创建 Pull Request

#### 📝 代码规范
- **代码风格**: 使用项目配置的 clang-format 自动格式化
- **命名规范**: 类名使用 PascalCase，函数和变量使用 camelCase
- **注释要求**: 公共接口必须有详细的文档注释
- **架构原则**: 遵循 SOLID 原则和项目分层架构
- **错误处理**: 使用 `Result<T>` 模板进行错误处理
- **线程安全**: 多线程代码必须保证线程安全

#### 🧪 测试要求
- 新功能必须包含相应的单元测试
- 确保所有现有测试通过
- 测试覆盖率应保持在合理水平
- 使用 Mock 对象进行依赖隔离

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
- **[docs/](docs/)** - 技术文档目录
  - `拖拽与点击进度条-最终解决及优化方案.md` - 进度条优化方案
  - `播放与暂停不可控问题.md` - 播放控制问题解决
  - `PROJECT_OPTIMIZATION_SUMMARY.md` - 项目优化总结
  - `OPTIMIZATION_GUIDE.md` - 详细优化指南
  - `GITHUB_UPLOAD_GUIDE.md` - GitHub上传指南

#### 📝 学习资料
- **[笔记/](笔记/)** - 开发笔记和学习记录
  - `智能体角色/` - AI助手角色定义
- **[examples/](examples/)** - 代码示例和使用案例
  - `optimization_usage_example.cpp` - 优化使用示例
- **[tests/](tests/)** - 单元测试和测试文档
  - `test_tagmanager.h/cpp` - 标签管理完整测试套件
  - `test_play_pause.cpp` - 播放暂停测试
  - `test_progress_bar_fix.cpp` - 进度条修复测试

#### 🎨 资源文件
- **[images/](images/)** - 项目图标和界面截图
- **[translations/](translations/)** - 多语言翻译文件
- **[*.ui](*.ui)** - Qt Designer UI界面文件

### 📄 **许可证**

本项目采用 **MIT 许可证** - 查看 [LICENSE](LICENSE) 文件了解详情。

### 📞 **联系方式**

- **项目仓库**: [GitHub - MusicPlayHandle](https://github.com/K-irito02/musicPlayHandle)
- **问题反馈**: [Issues](https://github.com/K-irito02/musicPlayHandle/issues)
- **功能建议**: [Discussions](https://github.com/K-irito02/musicPlayHandle/discussions)
- **技术文档**: 查看 `docs/` 目录下的详细文档

### 🙏 **致谢**

感谢所有为 MusicPlayHandle 项目做出贡献的开发者！

特别感谢以下技术和项目的支持：
- [Qt Framework](https://www.qt.io/) - 强大的跨平台应用开发框架
- [SQLite](https://www.sqlite.org/) - 轻量级数据库引擎
- [CMake](https://cmake.org/) - 跨平台构建系统

---

**MusicPlayHandle** - 基于现代 Qt6 技术栈的智能音频播放器 🎵✨

*让音乐播放更智能、更高效、更优雅！*