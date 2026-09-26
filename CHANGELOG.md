# 变更日志 (CHANGELOG)

本项目的所有显著变更均记录于此文档中。  
本项目遵循 [Semantic Versioning](https://semver.org/lang/zh-CN/) 语义化版本规范。

---

## [1.0.0] - 2026-08-25

### 🏛️ 架构重构 (Architecture & Core)
- **全面六边形架构 (Hexagonal Architecture)**：
  - 提取 `core/domain` 纯业务领域实体与值对象（`Note`, `NoteItem`, `AppSetting`），彻底消除对 MFC / Win32 窗口的直接依赖。
  - 定义 `core/ports/INoteRepository` 纯虚接缝接口，实现数据持久化与业务逻辑完全解耦。
  - 封装 `core/services/NoteService` 统领便签用例操作与生命周期协调。
- **现代化仓储体系 (Infrastructure)**：
  - 新增 `infra/SqliteNoteRepository` 提供基于 SQLite 的 ACID 事务型高性能存储引擎。
  - 保留并重构 `infra/JsonNoteRepository` 提供通用文件存储。
  - 增加 `infra/DataMigrator` 与 `infra/SqliteMigrator` 实现旧版 INI / JSON 数据自动平滑迁移与备份。
- **宿主窗口职责净化 (Host Layer)**：
  - 彻底拆分旧版 470 行单体 God Dialog，重构为单一职责的 `host/NoteHostWindow`。
  - 引入 `host/WebMessageDispatcher` 消息分发器，以函数表驱动的强类型 `WebMessageType` 替代 15 个硬编码 if-else 分支。
  - 引入 `host/NoteManager` 使用 `std::unique_ptr` 智能指针安全管理所有便签窗口，消除内存泄漏与悬挂指针隐患。
  - 重构 `host/MainControlPanel` 专注于系统托盘图标、全局热键与设置界面绑定。

### 🎨 前端主题与通信 (Themes & IPC)
- **TypeScript 强类型契约**：在 `themes/default` 与 `themes/simple` 中建立统一的 `ipc.ts` 强类型通信桥梁。
- **构建优化**：重构 Vite 配置与 Rollup 分包规则，按路由和第三方库（Element Plus, FontAwesome）拆分 chunk，消除构建包过大警告。
- **样式升级**：迁移 SCSS 为现代化 `@use` 语法，清除 Sass 过时废弃警告。

### 🛡️ 代码规范与开源合规 (Compliance & Quality)
- **UTF-8 with BOM**：全量 C++ 源码与头文件转码为 UTF-8 with BOM，彻底消除跨平台中文字符乱码。
- **警告清零**：修正 `wcscpy_s`, `swscanf_s`, `WideCharToMultiByte` 等调用，MSBuild 编译实现 **0 错误、0 警告**。
- **Apache 2.0 合规**：建立标准 `NOTICE` 文件，所有源码添加规范的 `Original Work Copyright (c) imlinhanchao` 与 `Modified by EchoCreek (2026)` 修改声明。
- **环境规范**：新增 `.gitattributes` 与优化后的 `.gitignore`，保障 Windows C++ 与 Web 技术栈的跨平台换行与排除一致性。

---

## [1.1.2] - 原作者发布版本 (Baseline)

- 基于 MFC + WebView2 的初代便签工具。
- 支持半透明背景、置顶、鼠标穿透以及简单的 INI/JSON 存储。
- 原作者：[imlinhanchao](https://github.com/imlinhanchao) (https://github.com/imlinhanchao/sticky_notes)
