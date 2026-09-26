# CONTEXT — Sticky Notes 重构领域词汇表

> 本文件是重构过程中全员统一使用的领域通用语言（Ubiquitous Language）。  
> 代码、注释、文档、PR 描述中必须使用这里定义的术语，禁止混用同义词。  
> 本文件只记录术语定义，不记录实现细节。

---

## 核心术语

### Note（便签）

一个独立的便签实体。由一个原生宿主窗口（`NoteHostWindow`）托管，内嵌 WebView2 渲染 UI。  
包含：名称、标题、位置/尺寸、背景色、透明度、置顶状态、可见状态，以及若干 `NoteItem`。  
**不是**：文本文档、标签页、分组。

### NoteItem（便签条目）

便签中的单条待办事项。包含：唯一 ID（`uId`）、文本内容（`sContent`）、完成状态（`bFinished`）。  
**不是**：Note 本身，也不是 Note 的子窗口。

### AppSetting（应用设置）

全局配置，包含：存储目录路径、当前主题名、四组全局快捷键（新建、激活、全部激活、全部取消激活）。  
**不是**：便签个体的外观设置（那属于 `Note` 的字段）。

### NoteRepository（便签仓库）

负责 `Note` 持久化读写的抽象接口（`INoteRepository`）。  
实现：`JsonNoteRepository`（基于 JSON 文件）。  
**不是**：直接文件 I/O，也不是全局静态工具类。

### NoteService（便签服务）

业务服务层。协调 `Note` 领域对象与 `INoteRepository` 的业务操作，如新建、重命名、更新条目、隐藏、清除、导出日历。  
**不是**：UI 控制器，也不是持久化实现。

### NoteHostWindow（主机窗口）

承载单个 `Note` 的 Win32 原生窗口，内嵌 WebView2 控件渲染前端 UI。  
职责：Win32 窗口生命周期、WebView2 初始化、鼠标穿透/置顶/透明度的 Win32 调用、IPC 消息的接收与转交。  
**不是**：业务逻辑执行者，也不是 IPC 路由器。

### WebMessageDispatcher（消息分发器）

将 Web→Native IPC 消息（`{ event, data }` JSON）解析为强类型的 `WebMessageType` 枚举，并路由到对应 Handler。  
**不是**：`NoteHostWindow` 的一部分，而是独立模块，被 `NoteHostWindow` 持有并调用。

### NoteManager（便签管理器）

管理所有运行中的 `NoteHostWindow` 实例的生命周期，包括：创建、列表维护、关闭回调、全局显示/隐藏/穿透。  
**不是**：`CAppCtrl`（旧名，已废弃）。

### MainControlPanel（主控制面板）

应用入口的 MFC 对话框，负责：系统托盘图标、全局快捷键注册/注销、设置 UI 的展示与绑定。  
**不是**：`CNotesDlg`（旧名，已废弃），也不是便签窗口本身。

---

## 废弃术语（禁止在新代码中使用）

| 废弃术语 | 替换为 |
| :--- | :--- |
| `NoteGroup` | `Note` |
| `CConfig` | `AppSettingStore` |
| `CNoteConfig` | `JsonNoteRepository` |
| `CNote` | `NoteService`（业务操作）或 `Note`（领域对象） |
| `CNoteDlg` | `NoteHostWindow` |
| `CNotesDlg` | `MainControlPanel` |
| `CAppCtrl` | `NoteManager` |
| `Setting` | `AppSetting` |
