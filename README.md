# 📖 HuYanReader - 智能文本阅读器与快捷浏览器

**专业！高效！轻量！**
基于Qt5的现代化多功能阅读工具，集成文本阅读、网页浏览和在线资源管理

**快速使用**：release中下载最新版本的压缩包，解压，双击运行`ProtectEye.exe`（程序默认静默启动），在托盘中找到程序（小老鼠），`右键托盘`-->`settings`，设置txt路径，`右键托盘`-->`start reading`，然后开始沉浸式阅读吧！  

**注意**：如果出现编码问题，请在`settings`尝试不同`编码`类型，或者手动调整txt格式为utf-8.  

**老板键**：  
Ctrl + Alt + M = 隐藏/显示txt阅读栏  
Ctrl + Alt + Q = 退出   


## 🎯 项目简介

HuYanReader是一个专业的桌面阅读应用程序，采用现代C++和Qt5技术栈开发。
提供专业级文本阅读体验、便捷网页浏览功能，以及智能在线小说资源管理能力。

### 🏗️ 核心架构
- **专业文本阅读引擎**：自定义渲染，支持护眼模式和个性化排版
- **嵌入式Web浏览器**：基于WebEngine，提供完整浏览体验
- **系统级集成**：全局热键、透明悬浮、系统托盘深度集成
- **多线程资源管理**：高效的在线内容获取和本地缓存机制
- **智能内容解析**：基于Lexbor的高性能HTML解析引擎

## ✨ 核心功能

### 📚 专业文本阅读器
- **护眼阅读体验**：自定义字体、行距、背景色，减少视觉疲劳
- **智能排版引擎**：自动文本断行、段落格式化、页面分割
- **无边框悬浮窗**：透明背景、拖拽调整、一键隐藏/显示
- **快捷操作**：鼠标滚轮翻页、键盘导航、进度显示
- **多格式支持**：TXT文本文件、在线内容、实时解析显示

### 🌐 快捷Web浏览器
- **轻量级浏览体验**：嵌入式WebEngine，快速启动
- **便捷操作界面**：地址栏、前进后退、刷新按钮
- **可调节透明度**：与桌面完美融合，不干扰工作流程
- **拖拽式窗口**：自由调整位置和大小
- **独立浏览环境**：与系统浏览器隔离，专注阅读场景

### 🔍 在线资源管理
- **多书源支持**：支持多个小说网站的搜索和获取
- **智能搜索引擎**：关键词搜索，支持书名和作者检索
- **并发下载机制**：多线程章节下载，提高获取效率
- **内容解析优化**：自动过滤广告、格式化文本内容
- **本地文件管理**：章节缓存、进度记录、文件组织

### ⚡ 系统集成特性
- **全局热键控制**：一键显示/隐藏、快速切换功能模块
- **系统托盘集成**：最小化到托盘，后台常驻运行
- **多窗口管理**：文本阅读器、Web浏览器、搜索界面独立控制
- **透明窗口技术**：与桌面环境无缝融合
- **会话状态保持**：自动保存阅读位置和浏览状态

## 🔧 技术特点

### 🏛️ 架构设计
```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   Core Layer    │    │  UI Layer       │    │  Network Layer  │
│                 │    │                 │    │                 │
│ • MainWindow    │◄──►│ • TextReader    │◄──►│ • HttpClient    │
│ • HotkeyMgr     │    │ • WebBrowser    │    │ • ContentParser │
│ • TrayIcon      │    │ • SearchView    │    │ • ResourceMgr   │
└─────────────────┘    └─────────────────┘    └─────────────────┘
```

### 🧵 多功能模式
- **阅读模式**：专注文本阅读，优化视觉体验
- **浏览模式**：快速网页浏览，便捷信息获取
- **搜索模式**：在线资源检索，内容获取和管理
- **集成模式**：多功能协同工作，无缝切换体验

### 🔐 技术优势
- **内存优化**：智能文本缓存，减少资源占用
- **渲染性能**：自定义绘制引擎，流畅阅读体验
- **网络安全**：SSL/TLS支持，Cookie管理，反爬虫策略
- **用户体验**：响应式界面，个性化配置，操作直观

## 🚀 快速开始

### 📋 系统要求
- **操作系统**：Windows 10/11 (64位)
- **运行时**：Visual C++ Redistributable 2019+
- **硬件要求**：最低2GB内存，支持DirectX的显卡

### 💾 安装使用
1. **获取程序**：从Release页面下载最新版本
2. **解压运行**：解压到任意目录，双击`ProtectEye.exe`
3. **系统集成**：程序自动在系统托盘创建图标
4. **功能使用**：
   - **Ctrl+热键** - 显示/隐藏文本阅读器
   - **右键托盘图标** - 打开Web浏览器或小说搜索
   - **拖拽窗口边缘** - 调整阅读器大小
   - **鼠标滚轮** - 翻页阅读

### 📖 使用场景
- **日常阅读**：打开TXT文件，享受专业阅读体验
- **资料查阅**：快速启动浏览器，查看网页内容
- **小说阅读**：搜索在线小说，下载到本地阅读
- **工作学习**：透明悬浮窗口，不干扰其他工作

## 🛠️ 开发构建

### 🔧 开发环境
- **编译器**：Visual Studio 2019+ 或 MinGW-w64
- **Qt版本**：Qt 5.14.2+ (需要WebEngine模块)
- **CMake**：3.21+
- **依赖库**：Lexbor (HTML解析), QHotkey (全局热键)

### 📦 构建步骤
```bash
# 1. 克隆项目
git clone https://github.com/dssaiy/HuYanReader.git
cd HuYanReader

# 2. 创建构建目录
mkdir build_vs
cd build_vs

# 3. 配置CMake (包含Qt WebEngine)
cmake .. -G "Visual Studio 16 2019" -A x64

# 4. 编译主程序
cmake --build . --config Release --target ProtectEye

# 5. 编译测试工具
cmake --build . --config Release --target BookSourceTest
cmake --build . --config Release --target IntegrationTest

# 6. 部署Qt依赖
cmake --build . --config Release --target DeployQt

# 7. 运行程序
./bin/ProtectEye.exe
```

### 🔍 项目结构
```
huyanreader/
├── src/                    # 源代码
│   ├── core/              # 主窗口、文档模型、管理器
│   ├── ui/                # 文本阅读器、Web视图、搜索界面
│   ├── network/           # HTTP客户端、网络通信
│   ├── novel/             # 小说业务逻辑、下载管理
│   ├── parser/            # HTML解析、内容提取
│   └── config/            # 配置管理、设置对话框
├── resources/             # 资源文件
│   ├── rules/            # 书源配置文件
│   └── mainwindow.qrc    # Qt资源文件
├── build_vs/              # 构建输出目录
└── lib/                   # 第三方库
    ├── lexbor/           # HTML解析库
    └── QHotkey/          # 全局热键库
```

## ⚙️ 配置说明

### 🎨 阅读器配置
- **字体设置**：字体族、大小、颜色自定义
- **排版选项**：行距、字距、页边距调整
- **背景主题**：护眼色彩、透明度、夜间模式
- **操作偏好**：翻页方式、快捷键、鼠标行为

### 🌐 浏览器配置
- **默认主页**：启动时打开的网址
- **透明度设置**：窗口透明程度调节
- **缓存管理**：临时文件清理策略
- **安全选项**：JavaScript、插件等开关

### 📚 书源配置
书源配置文件位于`resources/rules/`目录，采用JSON格式定义：

```json
{
  "id": 1,
  "name": "示例书源",
  "url": "https://example.com/",
  "search": {
    "url": "https://example.com/search",
    "method": "post",
    "result": "#results .book-item",
    "bookName": ".title a",
    "author": ".author"
  },
  "chapter": {
    "content": "#content",
    "title": ".chapter-title",
    "filterTag": "script div.ad"
  }
}
```

## 🤝 贡献指南

### 🐛 问题反馈
- 使用GitHub Issues报告Bug
- 提供详细的错误信息和复现步骤
- 包含系统环境和程序版本信息

### 💡 功能建议
- 阅读体验改进：字体渲染、排版算法、护眼功能
- 浏览器增强：广告屏蔽、手势操作、书签管理
- 系统集成：更多热键、开机启动、多显示器支持

### 🔧 代码贡献
1. Fork项目到个人仓库
2. 创建功能分支：`git checkout -b feature/new-feature`
3. 提交代码：`git commit -m "Add new feature"`
4. 推送分支：`git push origin feature/new-feature`
5. 创建Pull Request

### 📚 添加书源
1. 在`rules/`目录添加新的JSON配置文件
2. 按照现有格式定义搜索和解析规则
3. 测试确保搜索和下载功能正常
4. 提交PR并说明新书源的特点

## 📄 许可证

本项目采用MIT许可证，详见[LICENSE](LICENSE)文件。

## 🔗 相关链接

- **GitHub主页**：[https://github.com/dssaiy/HuYanReader](https://github.com/dssaiy/HuYanReader)
- **问题反馈**：[GitHub Issues](https://github.com/dssaiy/HuYanReader/issues)
- **版本发布**：[GitHub Releases](https://github.com/dssaiy/HuYanReader/releases)

## 🙏 致谢

- **Qt框架**：提供优秀的跨平台GUI框架
- **Lexbor库**：高性能HTML解析引擎
- **so-novel项目**：作者freeok
- **开源社区**：感谢所有贡献者的支持

---

> ⚠️ **免责声明**：本工具仅供学习和研究使用，请遵守相关网站的使用条款和版权规定。
> 🌟 **如果这个项目对你有帮助，请给个Star支持一下！**
