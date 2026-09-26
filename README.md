<p align="center">
  <img width="120" src="./themes/default/public/logo.png" alt="Stickify Logo">
</p>

<h1 align="center">Stickify | 现代化桌面贴纸便签与多维管理工作台</h1>

<p align="center">
  一款现代、通透、低开销且功能完备的 Windows 桌面便签与集中式多维管理工作台软件。<br>
  采用 <b>现代 C++17 (Win32 / DirectComposition / WebView2) + 现代前端 (Vue 3 / TypeScript / Pinia / Vite / Element Plus)</b> 六边形架构全栈构建。
</p>

<p align="center">
  <a href="LICENSE"><img src="https://img.shields.io/badge/License-Apache%202.0-blue.svg" alt="License"></a>
  <img src="https://img.shields.io/badge/Platform-Windows%2010%20%7C%2011%20(x64)-0078D6.svg?logo=windows" alt="Platform">
  <img src="https://img.shields.io/badge/Native%20Host-C%2B%2B17%20%2F%20MFC-00599C.svg?logo=c%2B%2B" alt="C++">
  <img src="https://img.shields.io/badge/Frontend-Vue%203%20%7C%20TypeScript%20%7C%20Pinia-4FC08D.svg?logo=vuedotjs" alt="Vue 3">
  <img src="https://img.shields.io/badge/Database-SQLite%20(WAL)-003B57.svg?logo=sqlite" alt="SQLite">
  <img src="https://img.shields.io/badge/Tests-231%20Passed-brightgreen.svg?logo=checkmarx" alt="Tests">
  <img src="https://img.shields.io/badge/i18n-zh--CN%20%7C%20en--US-orange.svg" alt="i18n">
  <a href="https://github.com/EchoCreek/sticky_notes/releases"><img src="https://img.shields.io/badge/Release-v1.0.0-success.svg" alt="Release"></a>
</p>

<p align="center">
  <a href="#-为什么选择-stickify双核形态的破局之道">💡 设计理念</a> •
  <a href="#-核心特性矩阵">✨ 核心特性</a> •
  <a href="#️-快捷键速查指南">⌨️ 快捷键</a> •
  <a href="#-下载与运行">📥 下载运行</a> •
  <a href="#️-现代六边形架构-hexagonal-architecture">🏛️ 架构设计</a> •
  <a href="#-衍生作品与致谢说明-fork--attribution">📄 开源协议</a>
</p>

---

## 💡 为什么选择 Stickify？双核形态的破局之道

> **传统便签软件的终极困境**：  
> 刚开始只记一两张便利贴，桌面清爽随手；但随着时间推移，桌面贴满花花绿绿的便签，杂乱无章；而一旦随手关掉便签，历史记录彻底丢失，无法全局搜索与归纳。

**Stickify 创新采用「桌面随手贴纸」+「多维管理工作台」的双核设计**，完美平衡了“随手轻盈”与“严肃管理”：

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                            Stickify 双核生产力闭环                           │
├──────────────────────────────────────┬──────────────────────────────────────┤
│       【桌面端】轻盈通透贴纸 (Widget)        │       【管理端】多维管理工作台 (Workbench)    │
├──────────────────────────────────────┼──────────────────────────────────────┤
│ • 半透明毛玻璃、8 款预设底色与自由拾色     │ • 卡片瀑布流 / 紧凑高密度表格双视图   │
│ • 无感鼠标穿透：化身桌面壁纸级挂件   │ • 全局待办进度仪表盘（完成率与进度条）│
│ • 贴边 100% 吸附隐藏，零像素残留悬停拉手 │ • 毫秒级全文模糊搜索 + 关键字荧光高亮 │
│ • 1:1 原地就地修改、Markdown 排版支持  │ • 侧边抽屉原地属性/待办微调与多维筛选 │
│ • 拖拽排序、完成沉底、系统日历(.ics)联动│ • 软删除回收站防误删、历史归档封存    │
│ • 8 方向无级自由缩放，彻底消除拉伸黑框 │ • Notion 级浮动批量操作条、MD/JSON导出│
└──────────────────────────────────────┴──────────────────────────────────────┘
```

无论你是想在桌面上安安静静挂几张待办便签，还是需要系统化整理和回溯数百张项目待办与备忘，Stickify 都能丝滑胜任。

---

## ✨ 核心特性矩阵

### 🎨 1. 沉浸式桌面贴纸 (Sticky Notes UX)
- **现代美学与智能配色**：半透明细腻毛玻璃质感，内置 8 款现代扁平高级预设色盘（曜石黑、石墨灰、极简白、暖阳米、抹茶绿、冰川蓝、丁香紫、柔粉桃），集成原生自由取色器。
- **亮度自适应算法 (Luminance Auto-Adaptation)**：底色切换时自动计算明度阀值，毫秒级切换黑/白前景色，确保任何配色方案下文字清晰锐利。
- **无感鼠标穿透（Widget 挂件模式）**：一键开启鼠标穿透，便签瞬间化身为桌面壁纸级挂件。日常点击、移动、框选桌面图标完全无阻隔、不误触；按下全局热键一秒唤醒编辑。
- **贴边 100% 自动隐藏 (Edge Docking)**：拖拽便签至屏幕左侧、右侧或顶部，窗口自动 100% 滑出桌面边界隐藏，零像素边缘残留；鼠标滑向边缘拉手平滑呼出，支持一键锁定边缘停靠状态。
- **8 方向无级自由缩放与零黑框**：支持四边与四角 8 方向鼠标拖拽缩放，结合内容基准尺寸计算与 Win32 `SWP_NOCOPYBITS` 机制，彻底根除传统窗口拖拽时的撕裂与黑框残留。

### 📋 2. 极简待办与高效排版 (Todo & Content)
- **闪速录入与拖拽重排**：`Enter` 键快速新建待办条目，配合淡入式拖拽把手自由调整上下优先级。
- **就地直接编辑 (In-Place Edit)**：双击文本就地进入内联编辑，字体、字号与行间距与浏览状态 1:1 绝对重合，无视觉跳动；`Shift + Enter` 换行，`Esc` 取消。
- **状态流转与一键清空**：勾选事项后平滑沉底；工具栏支持一键清除所有已完成事项，保持视野专注。
- **轻量 Markdown 排版支持**：支持加粗、斜体、行内代码、独立代码块与外部超链接。
- **系统日历日程联动**：点击待办项导出按钮，一键将事项转化为通用日历标准格式 (`.ics`)，直接唤起系统默认日历软件建立日程提醒。

### 🗂️ 3. 现代化多维便签管理工作台 (Management Workbench)
- **卡片瀑布流 / 紧凑表格双视图**：卡片视图适合直观浏览内容与色彩分类，紧凑表格视图适合高密度查看元数据（状态、颜色、修改时间、待办数）。
- **全局待办完成率仪表盘 (StatSidebar)**：工作台侧边栏实时汇总所有便签的待办总数、已完成项与**全局完成度百分比**，并以动态进度条可视化呈现。
- **全文检索与关键字实时荧光高亮**：毫秒级模糊检索，匹配项在卡片和列表中以醒目荧光黄即时高亮展示。
- **多维组合筛选与灵活排序**：支持按活动中、已隐藏、有待办、已完成、已归档、回收站及 8 种主题色系一键过滤；支持按最近更新时间、标题等规则排序。
- **全功能侧边抽屉 (NoteDetailDrawer)**：在工作台内直接滑出侧边抽屉，可无缝修改便签标题、切换主题色、调整置顶状态、通过滑块精细调节透明度（20%~100%）、切换桌面显示，并直接在抽屉中增删改待办子项。
- **桌面呼吸高亮定位脉冲 (Locate Pulse)**：在管理中心点击定位按钮，对应的桌面便签窗口立即激活置顶，并触发 1400ms 的呼吸光晕脉冲动效。

### 🛡️ 4. 数据安全与全生命周期管理 (Trash & Lifecycle)
- **防误删软删除回收站 (Trash Bin)**：删除操作自动移入系统回收站，支持随时单项还原、批量还原、单项彻底销毁或一键清空回收站，杜绝误操作导致的数据丢失。
- **历史便签归档封存 (Archive)**：将已完成的项目便签归档封存，既释放管理视野，又保障历史资产随时可查，随时可解档恢复至工作台。
- **Notion 级浮动批量操作栏 (Batch Floating Bar)**：勾选多张便签时底部平滑浮现操作胶囊，支持批量显隐、批量置顶/取消置顶、批量修改底色、批量归档、批量删除以及批量导出。
- **标准格式导出与备份**：支持单张或批量便签一键导出为 Markdown (`.md`) 或结构化 JSON (`.json`) 备份文件，内置代码高亮预览、一键复制到剪贴板与直接下载保存。

### 🎨 5. 双主题生态与端到端全栈国际化 (Themes & i18n)
- **Default 与 Simple 双套便签主题**：
  - `Default` 主题：圆角、细腻毛玻璃阴影与现代扁平化视觉；
  - `Simple` 主题：去繁就简，极简无边框设计，专为追求低视觉干扰的专注者打造。
- **端到端中英双语国际化 (Full-Stack i18n)**：系统托盘右键菜单、Win32 原生对话框、设置面板、管理工作台、两套便签主题全部支持 `zh-CN` ↔ `en-US` 实时无缝切换；前端基于 TypeScript 泛型提供 100% 编译期键名防漏检，并动态联动 Element Plus 语言包。

### 💾 6. 企业级 SQLite 持久化底座
- **高性能 SQLite 引擎**：开启 WAL（Write-Ahead Logging）模式、预编译语句（PreparedStatement）缓存与覆盖索引，吞吐量提升 20 倍以上，具备极强的异常抗掉电损坏能力。
- **平滑无损迁移**：初次启动自动探测旧版 INI / 单 JSON 历史数据，并自动平滑升级迁移至 SQLite 数据库中。

---

## ⌨️ 全局快捷键速查指南

| 默认全局快捷键 | 功能说明 | 支持自定义录制 |
| :--- | :--- | :---: |
| <kbd>Win</kbd> + <kbd>Alt</kbd> + <kbd>F8</kbd> | **快速新建便签**（自动聚焦至新便签输入框） | ✅ 是 |
| <kbd>Win</kbd> + <kbd>Alt</kbd> + <kbd>F7</kbd> | **穿透模式下编辑**（解除当前鼠标下便签的穿透锁定） | ✅ 是 |
| <kbd>Win</kbd> + <kbd>Alt</kbd> + <kbd>F9</kbd> | **隐藏全部便签**（一键快速隐藏所有桌面便签） | ✅ 是 |
| <kbd>Win</kbd> + <kbd>Alt</kbd> + <kbd>F10</kbd> | **显示全部便签**（一键恢复所有桌面便签的显示） | ✅ 是 |
| <kbd>Enter</kbd> | 便签输入框中快速添加待办事项 | - |
| <kbd>Shift</kbd> + <kbd>Enter</kbd> | 便签输入框内换行 | - |
| <kbd>Esc</kbd> | 退出就地编辑状态 / 取消关闭弹窗 | - |

> 快捷键可在便签管理中心的 **“系统设置” ➔ “全局快捷键”** 分区中通过按键录制组件自由设定。

---

## 📥 下载与运行

Stickify 为原生 Windows 桌面应用，系统资源占用极低，提供两种开箱即用的分发形态：

### 1. 绿色免安装便携版（推荐 🌟）
- 从 [Releases 页面](https://github.com/EchoCreek/sticky_notes/releases) 下载 `Stickify.1.0.0.x64.portable.zip`。
- 解压至任意目录（如 `D:\Tools\Stickify\`），双击 `Notes.exe` 即可直接运行。
- **真正的便携化**：配置与 SQLite 数据库全量保存在程序所在目录下，U 盘即插即用，重装系统数据不丢失。

### 2. 标准安装包版
- 下载并运行 `Stickify.1.0.0.x64.exe`，根据向导完成安装，自动建立开始菜单与桌面快捷方式。

> **系统要求**：
> - Windows 10 / Windows 11 (64-bit)。
> - 运行依赖：Microsoft Edge WebView2 Runtime（绝大多数现代 Windows 系统已内置，无需额外安装）。

---

## 🏛️ 现代六边形架构 (Hexagonal Architecture)

Stickify 坚持**高内聚、低耦合、深模块、窄接口**的设计原则，将业务核心与平台底层彻底解耦：

```
Stickify/
├── Notes/                               # C++17 桌面原生宿主工程
│   ├── core/                            # 纯业务领域层（Domain & Ports），零平台依赖
│   │   ├── domain/                      # Note, NoteItem, AppSetting 纯实体与值对象
│   │   ├── ports/                       # INoteRepository 等纯虚接口（接缝）
│   │   └── services/                    # NoteService 业务用例协调层
│   │
│   ├── infra/                           # 基础设施适配器层（Adapters）
│   │   ├── SqliteNoteRepository.h/.cpp  # SQLite3 事务型持久化仓储（WAL/索引）
│   │   ├── JsonNoteRepository.h/.cpp    # 轻量 JSON 仓储支持
│   │   ├── AppSettingStore.h/.cpp       # 全局配置 INI 存取适配器
│   │   ├── SqliteMigrator.h/.cpp        # SQLite 数据库版本迁移器
│   │   └── DataMigrator.h/.cpp          # 旧版数据兼容迁移工具
│   │
│   ├── host/                            # 原生宿主层（Host Layer）
│   │   ├── NoteHostWindow.h/.cpp        # DirectComposition + WebView2 单便签宿主窗口
│   │   ├── NoteManagerWindow.h/.cpp     # Vue 3 桌面管理工作台独立宿主窗口
│   │   ├── WebMessageDispatcher.h/.cpp  # 函数表驱动强类型 IPC 消息路由分发器
│   │   ├── NoteManager.h/.cpp           # unique_ptr 窗口生命周期安全管理器
│   │   ├── NoteDto.h/.cpp               # 跨进程/跨语言传输 DTO 序列化
│   │   └── MainControlPanel.h/.cpp      # Win32 系统托盘、全局热键与控制中心
│   │
│   ├── control/                         # 自定义原生控件（HotKeyEdit 等）
│   └── ref/                             # 通用工具库（Path, Ini, Log, Cvt, HotKey, Sqlite3）
│
├── themes/                              # 前端工程（Vue 3 + TypeScript + Vite + Pinia）
│   ├── manager/                         # 桌面管理工作台（Pinia 状态管理 + Element Plus + i18n）
│   ├── default/                         # 默认便签主题（现代扁平毛玻璃）
│   └── simple/                          # 简约便签主题（极简通透无边框）
│
└── tests/                               # C++ 自动化测试工程（231 项用例全绿通过）
    └── Notes.Tests.cpp                  # 包含领域规则、SQLite CRUD、IPC 路由与边界条件全量回归
```

---

## 🛠️ 本地编译与构建指南

<details>
<summary><b>点击展开开发者编译与测试说明</b></summary>

### 1. 环境准备
- **操作系统**：Windows 10 / Windows 11 (x64)
- **编译工具链**：Visual Studio 2022 / Build Tools（需勾选 `C++ 桌面开发`、`MFC 和 ATL 支持`）
- **前端工具链**：Node.js (>= 18.0.0) 与 `pnpm` (>= 8.0.0)
- **安装包打包工具（可选）**：Inno Setup 6 (`iscc.exe`)

---

### 2. 前端主题构建
在根目录下分别构建 3 个前端工程：
```powershell
# 构建便签管理中心
pnpm --dir themes/manager install && pnpm --dir themes/manager build

# 构建 Default 便签主题
pnpm --dir themes/default install && pnpm --dir themes/default build

# 构建 Simple 便签主题
pnpm --dir themes/simple  install && pnpm --dir themes/simple  build
```

---

### 3. 运行 231 项 C++ 自动化单元测试
```powershell
cmd /c "`"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat`" && cl /nologo /EHsc /MD /D_AFXDLL /DUNICODE /D_UNICODE /utf-8 /I Notes /I packages\rapidjson.1.0.2\build\native\include /I packages\Microsoft.Windows.ImplementationLibrary.1.0.230411.1\include /I packages\Microsoft.Web.WebView2.1.0.1774.30\build\native\include tests\Notes.Tests.cpp Notes\core\domain\Note.cpp Notes\core\domain\NoteItem.cpp Notes\core\domain\AppSetting.cpp Notes\core\services\NoteService.cpp Notes\infra\JsonNoteRepository.cpp Notes\infra\SqliteNoteRepository.cpp Notes\infra\SqliteMigrator.cpp Notes\infra\AppSettingStore.cpp Notes\infra\DataMigrator.cpp Notes\host\WebMessageDispatcher.cpp Notes\host\NoteHostProtocol.cpp Notes\host\NoteManagerProtocol.cpp Notes\host\NoteDto.cpp Notes\ref\Path.cpp Notes\ref\Ini.cpp Notes\ref\Log.cpp Notes\ref\Cvt.cpp Notes\ref\Shell.cpp Notes\ref\Registry.cpp Notes\ref\Utility.cpp Notes\ref\XFile.cpp Notes\ref\RawInput.cpp Notes\ref\HotKey.cpp Notes\ref\sqlite\sqlite3.c /Fe:x64\Release\NotesTests.exe /link /LIBPATH:packages\Microsoft.Web.WebView2.1.0.1774.30\build\native\x64"

.\x64\Release\NotesTests.exe
```

---

### 4. 编译 Release 主程序
使用 MSBuild 编译主程序，编译后会自动执行 `DeployThemesToOutput` 与 `SyncToWorkspaceOutput`：
```powershell
$msbuild = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\amd64\MSBuild.exe"
& $msbuild "Notes\Notes.vcxproj" /p:Configuration=Release /p:Platform=x64 /t:Rebuild /m
```
编译产物位于 `x64\Release\Notes.exe`。

---

### 5. 生成便携版与安装包
```powershell
# 制作绿色免安装 ZIP 包
& "scripts\package_portable.ps1"

# 制作 Inno Setup 安装包
& "C:\Program Files (x86)\Inno Setup 6\iscc.exe" setupx64.iss
```
</details>

---

## 📌 衍生作品与致谢说明 (Fork & Attribution)

本项目是基于 [imlinhanchao/sticky_notes](https://github.com/imlinhanchao/sticky_notes) 开源项目的**全面深度架构重构与能力扩展衍生版本**，遵循 **Apache License 2.0** 协议开源。

- **原工程作者**：[imlinhanchao](https://github.com/imlinhanchao)（原仓库：[imlinhanchao/sticky_notes](https://github.com/imlinhanchao/sticky_notes)）
- **重构与维护者**：[EchoCreek](https://github.com/EchoCreek) & Stickify Contributors
- **主要重构成果**：
  1. 将原有单体 God Object 解耦为纯粹的**六边形架构（Ports & Adapters）**；
  2. 搭建函数表驱动的强类型 IPC 消息分发体系；
  3. 引入支持事务与覆盖索引的高性能 SQLite 存储引擎，实现数据无损平滑迁移；
  4. 打造基于 Vue 3 + Pinia + Element Plus 的现代化多维便签管理工作台；
  5. 补齐端到端全场景中英双语国际化与 231 项自动化回归单元测试。

详细技术更新历史请查阅 [NOTICE](NOTICE) 与 [CHANGELOG.md](CHANGELOG.md)。

---

## 📄 开源许可证与协议声明

- 本项目采用 **[Apache License 2.0](LICENSE)** 协议开源。
- 完整版权、原始作者致谢与第三方开源组件引用声明请参阅 **[NOTICE](NOTICE)** 文件。
