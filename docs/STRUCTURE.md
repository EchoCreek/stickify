# Sticky Notes — 项目结构全览

> **阅读指引**：本文件是为后续 AI 会话准备的项目结构速查手册。
> 先读 §1 掌握整体形态，再按需跳入对应 §2（C++ 后端）或 §3（前端主题）查阅细节。
> 领域术语定义见 [`CONTEXT.md`](./CONTEXT.md)，重构历程见 [`REFACTOR_PLAN.md`](./REFACTOR_PLAN.md)。

---

## §1 整体形态

项目是一个 **Windows 原生 + Web 混合桌面应用**：

```
Sticky Notes/
├── Notes/              ← C++ 原生宿主（MFC + Win32 + WebView2）
│   ├── core/           ← 纯业务域，零平台依赖
│   ├── infra/          ← 持久化适配器
│   ├── host/           ← Win32/MFC/WebView2 宿主层
│   ├── control/        ← 自定义控件
│   └── ref/            ← 通用工具库
├── themes/             ← 前端主题（Vite + Vue 3 + TypeScript）
│   ├── default/        ← 主力主题（Element Plus + TailwindCSS）
│   └── simple/         ← 简约主题（结构同 default）
├── tests/              ← C++ 单元测试
├── docs/               ← 架构文档（本文件所在位置）
├── scripts/            ← 构建辅助脚本
└── Notes.sln           ← Visual Studio 解决方案
```

**通信模型**：每个便签是一个独立的 `NoteHostWindow`（Win32 窗口），内嵌 WebView2 加载前端主题页面。前后端通过 **IPC（`window.chrome.webview.postMessage`）** 双向通信，消息格式为 `{ event, data }` JSON，在 C++ 侧由 `WebMessageDispatcher` 强类型路由，在 TypeScript 侧由 `ipc.ts` 类型化契约约束。

---

## §2 C++ 原生宿主（`Notes/`）

### §2.1 架构层次

```
六边形架构（Hexagonal Architecture）

┌──────────────────────────────────────────┐
│              host 层（Win32 宿主）         │
│  MainControlPanel → NoteManager          │
│  NoteHostWindow ← WebMessageDispatcher   │
├──────────────────────────────────────────┤
│              core 层（业务域）             │
│  NoteService ← INoteRepository（接缝）   │
│  Note / NoteItem / AppSetting            │
├──────────────────────────────────────────┤
│              infra 层（适配器）            │
│  JsonNoteRepository / AppSettingStore    │
│  DataMigrator                            │
└──────────────────────────────────────────┘
         ↕ ref 层（跨层通用工具）
```

依赖方向：`host` → `core` ← `infra`，`ref` 被各层按需引用。

---

### §2.2 `core/` — 纯业务域

> 无任何 Win32 / MFC 头文件依赖，可独立单元测试。

#### `core/domain/` — 领域对象

| 文件 | 类 | 职责 |
| :--- | :--- | :--- |
| `Note.h/.cpp` | `Note` | 一个便签的全部状态：name、title、rect、bgColor、opacity、visible、topMost、items。含静态工厂 `Create()`、JSON 序列化 `ToJson()` / `FromJson()` |
| `NoteItem.h/.cpp` | `NoteItem` | 一条待办条目：`uId`（uint64_t）、`sContent`、`bFinished`。含静态工厂 `Create()` 及 JSON 序列化 |
| `AppSetting.h/.cpp` | `AppSetting` | 全局设置：自启、主题名、存储目录、4 组热键（`DWORD`）、自定义 WebView2 路径。含 `IsValid()` 校验 |

#### `core/ports/` — 接缝接口

| 文件 | 接口 | 方法签名 |
| :--- | :--- | :--- |
| `INoteRepository.h` | `INoteRepository`（纯虚） | `ListAll()` / `Load(name, &note)` / `Save(note)` / `Rename(old, new)` / `Delete(name)` |

#### `core/services/` — 业务协调

| 文件 | 类 | 职责 |
| :--- | :--- | :--- |
| `NoteService.h/.cpp` | `NoteService` | 构造注入 `INoteRepository&`。提供：条目 CRUD（`AddItem/UpdateItem/RemoveItem/UpdateAllItems`）、外观更新（`UpdateAppearance/UpdateTitle/UpdateRect`）、生命周期（`Rename/Hide/Clear`）、日历导出（`ExportToCalendar`） |

---

### §2.3 `infra/` — 持久化适配器

| 文件 | 类 | 职责 |
| :--- | :--- | :--- |
| `JsonNoteRepository.h/.cpp` | `JsonNoteRepository` | 实现 `INoteRepository`。每个便签存为独立 JSON 文件，目录可配置（`SetStorageDir`）。依赖 RapidJSON |
| `AppSettingStore.h/.cpp` | `AppSettingStore` | 静态方法：`Load()` / `Save()` / `SearchThemes()`。读写 INI 格式的全局设置文件 |
| `DataMigrator.h/.cpp` | `DataMigrator` | 静态方法 `MigrateIfNeeded(repo)`。检测旧版 INI 便签数据，一次性迁移到 JSON 格式后删除旧文件 |

---

### §2.4 `host/` — Win32 宿主层

#### `NoteHostWindow`（`NoteHostWindow.h/.cpp`）

**职责**：单个便签的原生窗口容器。持有 WebView2 COM 对象、处理 Win32 消息、委托业务给 `NoteService`、IPC 消息转交给 `WebMessageDispatcher`。

**关键接口**（供 `NoteManager` 调用）：

```cpp
bool  Init(const CString& noteName);  // 加载 Note 数据并初始化窗口
void  SetMouseThrough(bool through);
bool  IsMouseThrough() const;
void  SetWindowAlpha(float alpha);    // 0.0~100.0
const Note& GetNote() const;
void  SetOnClosedCallback(std::function<void(NoteHostWindow*)> cb);
```

**内部能力**：贴边自动收缩（`EdgeDockState`：None / Left / Right / Top）、拖动移动、透明度分层窗口、WebView2 初始化与事件绑定。

**私有推送方法**（Native → Web）：`PushNoteItems()` / `PushNoteSetting()` / `PushMouseThrough()` / `PostWebMessage(event, dataJson)`。

#### `WebMessageDispatcher`（`WebMessageDispatcher.h/.cpp`）

**职责**：将 Web→Native 的原始 JSON 字符串解析为强类型 `WebMessage`，路由到已注册 Handler。

```cpp
enum class WebMessageType {
    Move, Resize, Lock, Top, OpacityAble, BgColor, Title,
    Close, Add, Task, Update, UpdateAll, Remove,
    Hide, Clear, Listen, RestoreDock, EdgeLock
};

struct WebMessage { WebMessageType type; CString dataJson; };

class WebMessageDispatcher {
    void Register(WebMessageType type, IWebMessageHandler* handler);
    void Dispatch(const wchar_t* rawJson);
};
```

#### `NoteManager`（`NoteManager.h/.cpp`）

**职责**：所有 `NoteHostWindow` 实例的生命周期管理器。

```cpp
void Init();                          // 迁移旧数据 + 加载全部便签并建窗口
NoteHostWindow* New(noteName);        // 新建便签窗口
bool CheckEdit();                     // 鼠标命中检测，切换编辑模式
void SetMouseThrough(bool through);   // 全局穿透
void SetVisible(bool show);           // 全局显隐
void OnWindowClosed(NoteHostWindow*); // 关闭回调，防悬挂指针
```

**所有权**：`m_windows` 为 `vector<unique_ptr<NoteHostWindow>>`，自动管理内存。
`m_jsonRepo`（`JsonNoteRepository`）与 `m_service`（`NoteService`）均为自持实例。

#### `MainControlPanel`（`MainControlPanel.h/.cpp`）

**职责**：应用入口 MFC 对话框（隐藏窗口）。系统托盘图标、全局热键注册/注销、设置 UI 绑定。
持有 `NoteManager m_manager` 与 `AppSetting m_setting`。

---

### §2.5 `control/` — 自定义控件

| 文件 | 类 | 职责 |
| :--- | :--- | :--- |
| `HotKeyEdit.h/.cpp` | `CHotKeyEdit` | MFC 热键输入编辑框控件，用于设置面板中四组全局快捷键的 UI 录入 |

---

### §2.6 `ref/` — 通用工具库

所有工具类均在 `Easy` 命名空间下。

| 文件 | 主要内容 |
| :--- | :--- |
| `Path.h/.cpp` | 路径操作：`GetCurDirectory` / `Create` / `Exist` / `AppPath` 等 |
| `Ini.h/.cpp` | INI 文件读写封装（`CIniFile`） |
| `Log.h/.cpp` | 日志记录工具（`CLog`） |
| `Cvt.h/.cpp` | 字符编码转换：`WStrToStr` / `StrToWStr` / `WStrToUTF8` 等（WIL 实现） |
| `Shell.h/.cpp` | Shell 操作：文件关联、`ShellExecute` 封装、ICS 日历文件写入 |
| `Registry.h/.cpp` | 注册表读写封装（`CRegistry`） |
| `HotKey.h/.cpp` | Win32 全局热键注册/注销封装 |
| `RawInput.h/.cpp` | Raw Input 鼠标消息注册封装 |
| `Utility.h/.cpp` | 杂项工具函数 |
| `XFile.h/.cpp` | 简易文件读写（临时文件、测试辅助） |

---

### §2.7 入口与项目文件

| 文件 | 说明 |
| :--- | :--- |
| `Notes.h/.cpp` | MFC 应用对象 `CNotesApp`，`WinMain` 入口，实例互斥锁，启动 `MainControlPanel` |
| `stdafx.h` | 预编译头，统一 include：MFC、WRL、WIL、WebView2、RapidJSON、ref/ 全部工具、core/ 域对象 |
| `resource.h` / `Notes.rc` | MFC 资源定义（对话框 ID、图标、菜单） |
| `Notes.vcxproj` | VS2022 项目文件，包含所有 `.cpp` 编译单元与 NuGet 包引用（WebView2、WIL） |

---

### §2.8 `tests/`

| 文件 | 说明 |
| :--- | :--- |
| `tests/Notes.Tests.cpp` | 独立 C++ 单元测试（无第三方框架，自定义 `TEST_ASSERT` 宏）。覆盖：64-bit ID 边界与持久化、NoteService CRUD、WebMessageDispatcher 路由、AppSetting 序列化 |

---

## §3 前端主题（`themes/`）

两套主题（`default` / `simple`）**目录结构完全相同**，`simple` 是 `default` 的简化变体，以下以 `default` 为准。

### §3.1 技术栈

| 技术 | 版本/说明 |
| :--- | :--- |
| Vue 3 | Composition API + `<script setup>` |
| TypeScript | ~5.0 |
| Vite 4 | 构建工具，`base: './'`（相对路径，供 WebView2 本地加载） |
| Vue Router 4 | 哈希路由，`/` → `HomeView`，App.vue 作为布局壳 |
| Pinia 2 | 状态管理（当前使用较少，主要状态通过 `App` 类静态属性缓存） |
| Element Plus 2 | UI 组件库（按需使用） |
| TailwindCSS 3 | Utility-first CSS |
| FontAwesome 6 | 图标 |
| marked 5 | Markdown 渲染（NoteItem 内容） |
| vuedraggable 4 | 拖拽排序（HomeView 条目列表） |

---

### §3.2 源文件结构（`themes/default/src/`）

```
src/
├── ipc.ts          ← IPC 类型化契约（核心，与 C++ 侧保持同步）
├── main.ts         ← Vue 应用挂载、FontAwesome 注册、Router/Pinia 安装
├── App.vue         ← 布局壳：标题栏、颜色选择、透明度/置顶/锁定控制、RouterView
├── views/
│   ├── HomeView.vue    ← 便签主体：条目列表（vuedraggable）+ 快捷添加框 + 状态栏
│   └── NoteItem.vue    ← 单条待办：Markdown 渲染、inline 编辑、完成勾选、删除、拖拽把手
├── utils/
│   └── index.ts    ← 业务工具层：App / Config / Note 三个类，封装全部 IPC 调用
├── components/     ← 通用组件（当前主要是脚手架生成占位组件）
├── stores/
│   └── counter.ts  ← Pinia 示例 Store（当前业务未深度使用）
├── router/         ← Vue Router 配置（/ → HomeView）
└── themes/         ← CSS 主题变量文件
```

---

### §3.3 `ipc.ts` — IPC 契约（关键文件）

**与 C++ 侧 `WebMessageType` 枚举一一对应，双方必须保持同步。**

```typescript
// Web → Native 消息类型（共 18 条）
export type WebMessageType =
  | 'move' | 'resize' | 'lock' | 'top' | 'opacityable' | 'bgcolor' | 'title'
  | 'close' | 'add' | 'task' | 'update' | 'update_all' | 'remove'
  | 'hide' | 'clear' | 'listen' | 'restore_dock' | 'edge_lock';

// 核心函数
sendToNative(event, data?)   // 向 Native 发 IPC 消息
onNativeMessage(handler)     // 注册 Native→Web 消息监听，返回注销函数
isWebview                    // 环境检测（WebView2 内为 true，浏览器调试为 false）
```

---

### §3.4 `utils/index.ts` — 业务工具层

三个静态类封装全部 IPC 语义：

**`App` 类** — 便签窗口级操作 + 事件总线

| 方法 | IPC 事件 | 说明 |
| :--- | :--- | :--- |
| `App.init()` | `listen` | 通知 Native 开始推送数据 |
| `App.hide()` | `hide` | 隐藏当前便签 |
| `App.move(bool)` | `move` | 触发/停止窗口拖动 |
| `App.resize(dir)` | `resize` | 触发 Win32 缩放（8 方向） |
| `App.close()` | `close` | 关闭当前便签 |
| `App.restoreDock()` | `restore_dock` | 从贴边隐藏状态还原 |
| `App.lockEdge(bool)` | `edge_lock` | 贴边锁定开关 |
| `App.on(event, cb)` | — | 监听 Native→Web 事件，支持**状态回放** |
| `App.off(event, cb)` | — | 取消监听 |

> **状态回放机制**：`utils/index.ts` 模块级立即调用 `listen()`，将所有 Native 消息缓存至 `App.state`。
> `App.on()` 注册时若已有缓存数据则同步触发，解决 Vue 组件 `onMounted` 晚于 Native 推送的竞态。

**`Config` 类** — 便签外观配置

| 方法 | IPC 事件 |
| :--- | :--- |
| `Config.bgcolor(color)` | `bgcolor` |
| `Config.opacityable(bool)` | `opacityable` |
| `Config.lock(bool)` | `lock` |
| `Config.top(bool)` | `top` |
| `Config.title(str)` | `title` |

**`Note` 类** — 便签条目操作

| 方法 | IPC 事件 |
| :--- | :--- |
| `Note.add(data)` | `add` |
| `Note.update(data)` | `update` |
| `Note.remove(data)` | `remove` |
| `Note.makeTask(data)` | `task` |
| `Note.clear()` | `clear` |
| `Note.updateAll(list)` | `update_all` |

---

### §3.5 视图层职责

**`App.vue`** — 便签布局壳

- **标题栏**：便签名（可 inline 编辑，`Config.title()`）、颜色选择器 Popover、透明度/置顶开关、关闭/隐藏按钮
- **贴边拉手**：贴边隐藏时显示的还原触发区域（`App.restoreDock()`）
- **主体区域**：`<RouterView />`（渲染 `HomeView`）
- **拖动区域**：标题栏 `@mousedown` 触发 `App.move(true)`，`@mouseup` 触发 `App.move(false)`

**`HomeView.vue`** — 待办列表主体

- 使用 `vuedraggable` 渲染可拖拽排序的 `NoteItem` 列表
- 底部快捷添加框（`Enter` 添加，`Shift+Enter` 换行）
- 底部状态栏：待办计数 + "清空已完成"按钮（触发 `Note.updateAll()`）
- 监听 `App.on('data')` 接收条目数据，监听 `App.on('lock')` 控制穿透模式 UI 隐藏

**`NoteItem.vue`** — 单条待办卡片

- Markdown 渲染（`marked`）/ inline 编辑切换
- 完成勾选（`Note.update()`）、删除（`Note.remove()`）
- 添加日历任务（`Note.makeTask()`）
- 拖拽把手（`.drag-handle`，配合 `vuedraggable`）

---

## §4 IPC 消息全量对照表

| TypeScript 事件名 | C++ `WebMessageType` | 方向 | 数据类型 | 说明 |
| :--- | :--- | :--- | :--- | :--- |
| `listen` | `Listen` | W→N | — | 请求 Native 开始推送 setting + data |
| `move` | `Move` | W→N | `bool` | 开始/停止窗口拖动 |
| `resize` | `Resize` | W→N | `string` 方向 | 触发 Win32 窗口缩放 |
| `lock` | `Lock` | W→N | `bool` | 开启/关闭鼠标穿透 |
| `top` | `Top` | W→N | `bool` | 窗口置顶 |
| `opacityable` | `OpacityAble` | W→N | `bool` | 开启/关闭半透明 |
| `bgcolor` | `BgColor` | W→N | `string` hex | 设置背景色 |
| `title` | `Title` | W→N | `string` | 设置便签标题 |
| `close` | `Close` | W→N | — | 关闭便签窗口 |
| `add` | `Add` | W→N | `Note` 对象 | 新增条目 |
| `update` | `Update` | W→N | `Note` 对象 | 更新单条条目 |
| `update_all` | `UpdateAll` | W→N | `Note[]` | 批量更新（排序） |
| `remove` | `Remove` | W→N | `Note` 对象 | 删除条目 |
| `task` | `Task` | W→N | `Note` 对象 | 导出条目为 ICS 日历 |
| `hide` | `Hide` | W→N | — | 隐藏便签 |
| `clear` | `Clear` | W→N | — | 清空所有条目 |
| `restore_dock` | `RestoreDock` | W→N | — | 从贴边隐藏还原 |
| `edge_lock` | `EdgeLock` | W→N | `bool` | 贴边锁定开关 |
| `setting` | — | N→W | `AppSetting` 结构 | Native 推送便签设置（由 `listen` 触发） |
| `data` | — | N→W | `NoteItem[]` | Native 推送条目数据 |
| `lock` | — | N→W | `bool` | Native 推送鼠标穿透状态 |

> **方向说明**：W→N = Web（前端）→ Native（C++），N→W = Native → Web。

---

## §5 构建与产物

### C++ 原生宿主

```powershell
MSBuild Notes.sln /p:Configuration=Release /p:Platform=x64
# 产物：Notes/x64/Release/Notes.exe
```

### 前端主题

```powershell
cd themes/default && pnpm install && pnpm build
# 产物：themes/default/dist/（index.html + assets/）
# NoteHostWindow 通过 WebView2 加载 dist/index.html
```

### 安装包

```powershell
ISCC.exe setupx64.iss
# 产物：output/Sticky.Notes.1.1.2.x64.exe
```

---

*本文件由 AI 根据源码结构生成，2026-08-20。随后续改动应同步更新。*
