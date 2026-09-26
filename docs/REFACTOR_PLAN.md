# Sticky Notes 全面重构规划

> **基准工程**：[imlinhanchao/sticky_notes](https://github.com/imlinhanchao/sticky_notes)  
> **授权协议**：Apache License 2.0  
> **规划日期**：2026-08-17  
> **分析方法**：codebase-design（深度模块 · 接缝 · 适配器）+ domain-modeling（领域术语规范化）

---

## 一、领域词汇表（Ubiquitous Language）

> 所有重构代码、注释、文档必须统一使用以下术语，禁止混用同义词。

| 术语（中文） | 英文标准名 | 含义说明 |
| :--- | :--- | :--- |
| **便签** | `Note` | 一个独立的便签窗口，包含多个条目和全部外观设置 |
| **便签条目** | `NoteItem` | 便签中的一行待办文本，含完成状态 |
| **应用设置** | `AppSetting` | 全局配置（快捷键、主题、存储目录等） |
| **便签仓库** | `NoteRepository` | 负责 Note 的持久化读写（抽象接口） |
| **主机窗口** | `NoteHostWindow` | 托管 WebView2 的原生 Win32 窗口 |
| **消息分发器** | `WebMessageDispatcher` | 解析并路由 Web→Native IPC 消息 |
| **便签管理器** | `NoteManager` | 管理所有运行中的 Note 实例的生命周期 |
| **主控制面板** | `MainControlPanel` | 托盘图标、快捷键、设置对话框的入口 |

---

## 二、原始工程架构问题汇总

### 2.1 God Dialog 问题（`CNoteDlg`）

`CNoteDlg` 同时承载了以下 **5 类职责**，违反单一职责原则：
- MFC 窗口生命周期（OnInitDialog / OnSize / OnPaint / OnEraseBkgnd）
- WebView2 COM 环境初始化与事件绑定
- IPC 消息路由（18 个 if-else 分支的字符串分发）
- Win32 鼠标穿透、置顶、透明度控制
- NoteItem 的业务逻辑修改（add / update / remove / task）

### 2.2 持久化强耦合（`CNoteConfig` / `CConfig`）

- 全静态方法 + 全局单例，无法 Mock
- 同时存在两套存储格式（旧 INI 格式 `CConfig`，新 JSON 格式 `CNoteConfig`）
- 迁移逻辑（INI→JSON）写在 `CAppCtrl::Init()` 中，代码混杂 UI 交互

### 2.3 内存安全问题（`CAppCtrl`）

- `vector<CNoteDlg*>` 使用原始指针，析构函数未释放
- `bVisible=false` 时走 `delete pDlg` 但对象从未真正清理干净

### 2.4 编码与现代 C++ 问题

- 源文件使用 GBK 编码，中文注释在不同环境下乱码
- 大量 `GetBuffer() / ReleaseBuffer()` 调用配合 RapidJSON，生命周期脆弱
- 使用已废弃的 `std::codecvt_utf8`（C++17 deprecated）

### 2.5 前端与 Native 协议无类型化

- IPC 消息事件名为魔术字符串（`"move"`, `"lock"`, `"task"` 等），双端无任何类型校验

---

## 三、目标架构（重构后）

```
Notes/
├── core/                         ← 纯业务域，无 Win32 / MFC 依赖
│   ├── domain/
│   │   ├── Note.h/.cpp               Note 值对象（不可变 + 工厂）
│   │   ├── NoteItem.h/.cpp           NoteItem 值对象
│   │   └── AppSetting.h/.cpp         AppSetting 值对象
│   ├── ports/
│   │   └── INoteRepository.h         纯虚接口：Load/Save/Rename/Delete/ListAll
│   └── services/
│       └── NoteService.h/.cpp        业务服务层（协调 Domain + Repository）
│
├── infra/                        ← 平台适配器，实现 core/ports 接口
│   ├── JsonNoteRepository.h/.cpp     基于 JSON 文件的存储实现
│   ├── AppSettingStore.h/.cpp        INI/JSON 设置存储实现
│   └── DataMigrator.h/.cpp          旧版 INI→JSON 迁移（一次性）
│
├── host/                         ← Win32 / MFC / WebView2 宿主层
│   ├── NoteHostWindow.h/.cpp         便签原生窗口（仅 Win32 职责）
│   ├── WebMessageDispatcher.h/.cpp   IPC 消息分发路由器
│   ├── NoteManager.h/.cpp            便签实例生命周期管理
│   └── MainControlPanel.h/.cpp      主控制面板（托盘 + 快捷键 + 设置）
│
├── control/
│   └── HotKeyEdit.h/.cpp            热键输入控件（保留）
│
├── ref/                          ← 通用工具库（保留，仅清理警告）
│   └── Path / Registry / HotKey / Cvt / Ini / Log / Shell / XFile ...
│
├── stdafx.h / stdafx.cpp            保留，精简 include 列表
└── Notes.vcxproj

themes/                           ← 前端主题（独立 Vite 工程，基本保留）
├── default/
└── simple/

docs/
├── REFACTOR_PLAN.md              ← 本文件
├── CONTEXT.md                    ← 领域词汇表（持续维护）
└── adr/
    ├── 0001-ipc-typed-protocol.md
    └── 0002-hexagonal-arch.md
```

---

## 四、主线重构任务清单（Master Task List）

### Phase 0：基础准备（约 0.5 天）✦ 前置必做 【✅ 已完成】

| # | 任务 | 产出文件 | 状态 | 说明 |
| :--- | :--- | :--- | :--- | :--- |
| P0-1 | 全部源文件转换为 **UTF-8 with BOM** 编码 | 所有 `.cpp` / `.h` | ✅ 已完成 | 彻底消除乱码与代码页问题 |
| P0-2 | 替换 `wcscpy`→`wcscpy_s`、`swscanf`→`swscanf_s`、`std::codecvt_utf8`→WIL 等价，消除 C4996 | `ref/Cvt.cpp`, `ref/Path.cpp`, `app/Note.cpp` | ✅ 已完成 | 消除全部编译过时警告 |
| P0-3 | 在所有被修改文件头部添加 Apache 2.0 修改声明 | 各 `.cpp` / `.h` | ✅ 已完成 | 遵守开源协议规范 |
| P0-4 | 创建 `docs/CONTEXT.md` 领域词汇表，固化术语 | `docs/CONTEXT.md` | ✅ 已完成 | 统一定义统一术语与概念 |

---

### Phase 1：Domain 层建立（约 1 天）✦ 核心地基 【✅ 已完成】

> 目标：将 `defintion.h` 的 C 式 struct 提升为具有不变量的领域对象，消除 `CNoteConfig::*` 全局静态依赖。

| # | 任务 | 产出文件 | 状态 | 说明 |
| :--- | :--- | :--- | :--- | :--- |
| P1-1 | 提取 `NoteItem` 值对象，含构造工厂与 JSON 序列化接口 | `core/domain/NoteItem.h/.cpp` | ✅ 已完成 | 取代 `_NoteItem` struct |
| P1-2 | 提取 `Note` 领域对象，封装不变量（name 非空、rect 合法等） | `core/domain/Note.h/.cpp` | ✅ 已完成 | 取代 `NoteGroup` struct |
| P1-3 | 提取 `AppSetting` 值对象，含校验逻辑 | `core/domain/AppSetting.h/.cpp` | ✅ 已完成 | 取代 `Setting` struct |
| P1-4 | 定义 `INoteRepository` 纯虚接口（ListAll / Load / Save / Rename / Delete） | `core/ports/INoteRepository.h` | ✅ 已完成 | 持久化抽象接缝 |
| P1-5 | 实现 `JsonNoteRepository`，迁入 `CNoteConfig` 全部 JSON 读写逻辑 | `infra/JsonNoteRepository.h/.cpp` | ✅ 已完成 | 取代 `CNoteConfig` |
| P1-6 | 删除 `defintion.h` 与旧版 `CConfig`（INI 格式，迁移逻辑已处理） | — | ✅ 已完成 | 清理技术债 |

---

### Phase 2：Service 层建立（约 0.5 天）✦ 业务协调 【✅ 已完成】

> 目标：将散落在 `CNoteDlg::OnWebMessageReceived` 和 `CNote::*` 里的业务逻辑集中到 `NoteService`。

| # | 任务 | 产出文件 | 状态 | 说明 |
| :--- | :--- | :--- | :--- | :--- |
| P2-1 | 建立 `NoteService`，整合 Add / Update / Remove / UpdateAll / Rename / Hide / Clear | `core/services/NoteService.h/.cpp` | ✅ 已完成 | 依赖注入 `INoteRepository` |
| P2-2 | 将 `CNote::MakeTask()`（ICS 日历生成）迁移为 `NoteService::ExportToCalendar()` | `core/services/NoteService.cpp` | ✅ 已完成 | 消除对 `ShellExecute` 的直接调用 |
| P2-3 | `NoteService` 通过构造函数注入 `INoteRepository`，消除全局静态依赖 | `core/services/NoteService.h` | ✅ 已完成 | 为单元测试打好基础 |

---

### Phase 3：IPC 协议强类型化（约 0.5 天）✦ 接缝规范 【✅ 已完成】

> 目标：将松散字符串 IPC 协议升级为枚举驱动的强类型消息契约。

| # | 任务 | 产出文件 | 状态 | 说明 |
| :--- | :--- | :--- | :--- | :--- |
| P3-1 | 定义 `WebMessageType` 枚举，收录全部 IPC 事件（Move / Lock / Top / Add / Update …） | `host/WebMessageDispatcher.h` | ✅ 已完成 | C++ 侧类型约束 |
| P3-2 | 定义 `WebMessageContract` 结构体（含解析工厂 `ParseFromJson`） | `host/WebMessageDispatcher.h` | ✅ 已完成 | 取代 if-else 字符串比对 |
| P3-3 | 建立 `WebMessageDispatcher`，用函数表 `std::map<WebMessageType, Handler>` 替代 18 个 if-else | `host/WebMessageDispatcher.h/.cpp` | ✅ 已完成 | 新增事件只需注册一行 |
| P3-4 | 前端提取 `ipc.ts` 类型化契约文件，与 C++ 侧枚举保持一致 | `themes/*/src/ipc.ts` | ✅ 已完成 | TypeScript 侧类型约束 |

---

### Phase 4：Host 层重构（约 1 天）✦ 窗口职责净化 【✅ 已完成】

> 目标：将 `CNoteDlg` 的 5 类职责拆分归位，使其仅负责 Win32 + WebView2 生命周期。

| # | 任务 | 产出文件 | 状态 | 说明 |
| :--- | :--- | :--- | :--- | :--- |
| P4-1 | 重命名 `CNoteDlg` → `NoteHostWindow`，移除全部业务逻辑代码 | `host/NoteHostWindow.h/.cpp` | ✅ 已完成 | 仅保留窗口 + WebView2 职责 |
| P4-2 | `NoteHostWindow` 构造注入 `NoteService`，委托所有业务操作 | `host/NoteHostWindow.h` | ✅ 已完成 | 解耦业务与 UI |
| P4-3 | `NoteHostWindow` 持有 `WebMessageDispatcher`，仅负责接收原始 JSON 并转交 | `host/NoteHostWindow.cpp` | ✅ 已完成 | 解耦 IPC 路由 |
| P4-4 | 鼠标穿透、置顶、透明度调整封装为私有辅助方法 | `host/NoteHostWindow.cpp` | ✅ 已完成 | 提升可读性 |

---

### Phase 5：生命周期管理重构（约 0.5 天）✦ 内存安全 【✅ 已完成】

> 目标：消除原始指针列表，引入智能指针与安全关闭回调。

| # | 任务 | 产出文件 | 状态 | 说明 |
| :--- | :--- | :--- | :--- | :--- |
| P5-1 | 建立 `NoteManager`，用 `vector<unique_ptr<NoteHostWindow>>` 管理便签实例 | `host/NoteManager.h/.cpp` | ✅ 已完成 | 取代 `CAppCtrl` 原始指针列表 |
| P5-2 | `NoteHostWindow` 关闭时通过回调通知 `NoteManager` 移除，防止悬挂指针 | `host/NoteManager.h` | ✅ 已完成 | use-after-free 防护 |
| P5-3 | 将 INI→JSON 迁移逻辑抽离为 `DataMigrator`，从 `NoteManager::Init()` 解耦 | `infra/DataMigrator.h/.cpp` | ✅ 已完成 | 迁移后可一键删除此模块 |

---

### Phase 6：MainControlPanel 重构（约 0.5 天）✦ 入口整洁 【✅ 已完成】

> 目标：精简 `CNotesDlg`，归位托盘/快捷键/设置职责。

| # | 任务 | 产出文件 | 状态 | 说明 |
| :--- | :--- | :--- | :--- | :--- |
| P6-1 | 重命名 `CNotesDlg` → `MainControlPanel`，保留 MFC 对话框基类 | `host/MainControlPanel.h/.cpp` | ✅ 已完成 | 语义明确 |
| P6-2 | `MainControlPanel` 持有 `NoteManager` 引用，消除 `static CAppCtrl` 全局状态 | `host/MainControlPanel.h` | ✅ 已完成 | 消除静态全局 |
| P6-3 | 将设置读写迁移到 `AppSettingStore`，`MainControlPanel` 仅负责 UI 绑定 | `infra/AppSettingStore.h/.cpp` | ✅ 已完成 | 持久化解耦 |

---

### Phase 7：废弃代码清理、前端现代化与发布验证 【✅ 已完成】

> 彻底清理旧架构残留，优化前端主题打包与分包，完成全链路发布打包。

| # | 任务 | 产出文件 | 状态 | 说明 |
| :--- | :--- | :--- | :--- | :--- |
| P7-1 | 彻底移除废弃的 C++ 旧代码与目录（NotesDlg, NoteDlg, defintion.h, app/*） | `Notes.vcxproj`, `Notes.vcxproj.filters`, `stdafx.h` | ✅ 已完成 | 0 错误 0 警告编译 |
| P7-2 | 前端全面接入 `ipc.ts` 契约，消除 SCSS 废弃警告，配置 Rollup 分包策略 | `themes/*/src/ipc.ts`, `style.scss`, `vite.config.ts` | ✅ 已完成 | 消除大 chunk 警告 |
| P7-3 | 更新架构文档、README.md 与 Apache 2.0 合规声明 | `README.md`, `docs/REFACTOR_PLAN.md` | ✅ 已完成 | 文档与合规对齐 |
| P7-4 | 执行全链路发布打包终验 | `output/Sticky.Notes.1.1.2.x64.exe` | ✅ 已完成 | Inno Setup 成功生成安装包 |

---

## 五、依赖关系与执行顺序

```
P0（基础准备）
   │
   ├──► P1（Domain 层）
   │       │
   │       └──► P2（Service 层）
   │                │
   │                └──► P3（IPC 协议）──► P4（Host 重构）
   │                                            │
   │                                            └──► P5（生命周期）──► P6（面板整洁）
   │
   └──► P7（前端增强，可与 P1~P6 并行）
```

**工作量估算汇总**

| Phase | 说明 | 预估 |
| :--- | :--- | :--- |
| P0 | 基础准备 | 0.5 天 |
| P1 | Domain 层 | 1.0 天 |
| P2 | Service 层 | 0.5 天 |
| P3 | IPC 类型化 | 0.5 天 |
| P4 | Host 重构 | 1.0 天 |
| P5 | 生命周期 | 0.5 天 |
| P6 | 面板整洁 | 0.5 天 |
| P7 | 前端增强（可选） | 1~2 天 |
| **合计** | | **≈ 4.5~6.5 天** |

---

## 六、Apache 2.0 合规清单

每个被修改的文件顶部添加如下声明：

```cpp
// ------------------------------------------------------------------
// Original Work Copyright (c) imlinhanchao
// https://github.com/imlinhanchao/sticky_notes
// Modified by [Your Name / Team] (2026): [Brief change summary]
// Licensed under the Apache License, Version 2.0
// ------------------------------------------------------------------
```

> 新建文件（`core/`、`infra/`、`host/` 下的全新文件）版权属于你，按你的意愿标注即可。

---

## 七、验证计划

每个 Phase 完成后，必须通过以下全部验证门槛再进入下一阶段：

| 验证项 | 命令 / 方式 |
| :--- | :--- |
| C++ 编译无错误 | `MSBuild Notes.sln /p:Configuration=Release /p:Platform=x64` |
| 应用可正常启动 | 手动启动 `x64\Release\Notes.exe` |
| 便签内容持久化 | 关闭后重启，验证内容仍在 |
| IPC 消息全链路 | 在前端触发所有操作，验证 Native 侧正确响应 |
| 安装包构建成功 | `ISCC.exe setupx64.iss` 成功生成安装程序 |

---

*本文档随重构进展持续更新。*
