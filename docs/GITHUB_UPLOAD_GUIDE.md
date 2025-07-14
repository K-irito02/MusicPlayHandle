# MusicPlayHandle 项目上传到 GitHub 指南

## 📋 准备工作

### 1. 确保已安装 Git
```bash
git --version
```
如果没有安装，请从 [Git官网](https://git-scm.com/) 下载安装。

### 2. 配置 Git（如果还没有配置）
```bash
git config --global user.name "你的用户名"
git config --global user.email "你的邮箱"
```

## 🚀 上传步骤

### 步骤 1: 在 GitHub 上创建仓库
1. 登录 [GitHub](https://github.com)
2. 点击右上角的 "+" 按钮，选择 "New repository"
3. 填写仓库信息：
   - **Repository name**: `musicPlayHandle`
   - **Description**: `Qt6音频播放器 - 基于现代C++17和Qt6框架的高性能音频播放应用，支持标签管理、多线程音频处理和数据库持久化`
   - **Visibility**: Public（推荐）或 Private
   - **不要勾选** "Add a README file"、"Add .gitignore"、"Choose a license"（因为项目中已有这些文件）
4. 点击 "Create repository"

### 步骤 2: 初始化本地 Git 仓库
在项目根目录（`d:/QT_Learn/Projects/musicPlayHandle`）打开命令行，执行：

```bash
# 初始化 Git 仓库
git init

# 添加所有文件到暂存区
git add .

# 创建初始提交
git commit -m "Initial commit: Qt6 MusicPlayHandle 音频播放器项目

- 基于 Qt6.5.3 和 C++17 的现代音频播放器
- 支持 CMake 和 qmake 双构建系统
- 实现分层架构设计和组件化开发
- 包含音频引擎、标签管理、数据库持久化
- 支持多线程音频处理和性能优化
- 完整的单元测试和文档支持"
```

### 步骤 3: 连接远程仓库并推送
```bash
# 添加远程仓库（替换 YOUR_USERNAME 为你的 GitHub 用户名）
git remote add origin https://github.com/YOUR_USERNAME/musicPlayHandle.git

# 设置默认分支为 main
git branch -M main

# 推送到 GitHub
git push -u origin main
```

## 📁 项目结构说明

上传后的 GitHub 仓库将包含：

```
musicPlayHandle/
├── 📄 README.md                 # 项目说明文档
├── 📄 .gitignore                # Git 忽略文件配置
├── 📄 CMakeLists.txt            # CMake 构建配置
├── 📄 musicPlayHandle.pro       # qmake 构建配置
├── 📄 version.h                 # 版本信息
├── 📁 src/                      # 源代码目录
│   ├── 📁 audio/               # 音频处理模块
│   ├── 📁 core/                # 核心组件
│   ├── 📁 database/            # 数据库管理
│   ├── 📁 interfaces/          # 接口抽象层
│   ├── 📁 managers/            # 业务管理器
│   ├── 📁 models/              # 数据模型
│   ├── 📁 threading/           # 线程管理
│   └── 📁 ui/                  # 用户界面
├── 📁 docs/                     # 项目文档
├── 📁 tests/                    # 单元测试
├── 📁 examples/                 # 示例代码
├── 📁 translations/             # 国际化文件
├── 📁 images/                   # 图标资源
└── 📁 设计文档/                 # 设计文档
```

## 🔧 后续维护

### 日常提交流程
```bash
# 查看文件状态
git status

# 添加修改的文件
git add .

# 提交更改
git commit -m "描述你的更改"

# 推送到 GitHub
git push
```

### 创建分支进行功能开发
```bash
# 创建并切换到新分支
git checkout -b feature/新功能名称

# 开发完成后推送分支
git push -u origin feature/新功能名称

# 在 GitHub 上创建 Pull Request 进行代码审查
```

## 📝 注意事项

1. **敏感信息**：确保不要提交包含密码、API密钥等敏感信息的文件
2. **大文件**：避免提交大型二进制文件，使用 Git LFS 处理大文件
3. **构建产物**：`.gitignore` 已配置忽略构建产物和临时文件
4. **提交信息**：使用清晰、描述性的提交信息
5. **分支管理**：使用分支进行功能开发，保持 main 分支稳定

## 🎉 完成

按照以上步骤，你的 MusicPlayHandle 项目就成功上传到 GitHub 了！

你可以在 `https://github.com/YOUR_USERNAME/musicPlayHandle` 查看你的项目。