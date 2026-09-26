# 便签管理器设计方案（SQLite 路线）

> **决策状态**：✅ 已确定采用 SQLite 作为统一存储后端。
> 阅读前置：[`STRUCTURE.md`](./STRUCTURE.md)、[`CONTEXT.md`](./CONTEXT.md)

---

## 一、背景与决策

### 1.1 原始存储（已分析）

每个便签 = 一个独立 JSON 文件（`<NoteDir>/<name>.json`），文件名为时间戳 ID。
全局设置存于 `<EXE目录>/setting.ini`。

现有存储的**核心痛点**：

| 痛点 | 说明 |
| :--- | :--- |
| 写冲突 | 管理器与主程序同时写同一文件，无锁机制，数据可能损坏 |
| 无变更通知 | 两个进程间数据变更无感知机制 |
| 无全局排序 | 便签列表顺序依赖文件系统枚举，不稳定 |
| 无跨便签查询 | 搜索需全量读取所有 JSON 文件 |

### 1.2 决策：引入 SQLite

**理由**：

1. **`INoteRepository` 接缝已就位**：只需新增 `SqliteNoteRepository` 实现该接口，`NoteService` 和上层代码零改动
2. **WAL 模式**：SQLite WAL（Write-Ahead Logging）原生支持多读者 + 单写者，彻底解决写冲突
3. **单文件**：`notes.db` 便于备份、版本控制、跨工具访问
4. **补全能力**：原生支持排序字段、搜索查询、事务原子性

---

## 二、数据库 Schema 设计

### 2.1 表定义

```sql
-- 便签表
CREATE TABLE IF NOT EXISTS notes (
    name        TEXT    PRIMARY KEY,    -- 便签唯一标识（时间戳字符串，继承现有格式）
    title       TEXT    NOT NULL DEFAULT '',
    win_left    INTEGER NOT NULL DEFAULT 100,
    win_top     INTEGER NOT NULL DEFAULT 100,
    win_right   INTEGER NOT NULL DEFAULT 530,
    win_bottom  INTEGER NOT NULL DEFAULT 530,
    bgcolor     TEXT    NOT NULL DEFAULT '#0d1117',  -- #RRGGBB
    opacity     INTEGER NOT NULL DEFAULT 50,          -- 0~100
    opacity_on  INTEGER NOT NULL DEFAULT 0,           -- 0/1 boolean
    visible     INTEGER NOT NULL DEFAULT 1,           -- 0/1 boolean
    topmost     INTEGER NOT NULL DEFAULT 1,           -- 0/1 boolean
    sort_order  INTEGER NOT NULL DEFAULT 0,           -- 便签全局排序（JSON 方案缺失）
    updated_at  INTEGER NOT NULL DEFAULT 0            -- Unix 毫秒时间戳，供变更检测
);

-- 便签条目表
CREATE TABLE IF NOT EXISTS note_items (
    id          INTEGER PRIMARY KEY,                  -- uint64 时间戳 ID（JavaScript 生成）
    note_name   TEXT    NOT NULL
                REFERENCES notes(name) ON DELETE CASCADE,
    content     TEXT    NOT NULL DEFAULT '',
    finished    INTEGER NOT NULL DEFAULT 0,           -- 0/1 boolean
    sort_order  INTEGER NOT NULL DEFAULT 0            -- 条目在便签内排序
);

CREATE INDEX IF NOT EXISTS idx_note_items_note_name ON note_items(note_name);
```

### 2.2 字段映射（JSON → SQLite）

| JSON 字段 | SQLite 列 | 类型变化 |
| :--- | :--- | :--- |
| `name` | `notes.name` | 无 |
| `title` | `notes.title` | 无 |
| `left/top/right/bottom` | `notes.win_left/top/right/bottom` | 无 |
| `bgcolor` | `notes.bgcolor` | 无（保持 #RRGGBB 字符串） |
| `opacity` | `notes.opacity` | 无 |
| `opacityable` | `notes.opacity_on` | bool → INTEGER 0/1 |
| `visible` | `notes.visible` | bool → INTEGER 0/1 |
| `topmost` | `notes.topmost` | bool → INTEGER 0/1 |
| `notes[].id` | `note_items.id` | 无 |
| `notes[].content` | `note_items.content` | 无 |
| `notes[].finish` | `note_items.finished` | bool → INTEGER 0/1 |
| ❌ 无 | `notes.sort_order` | 新增 |
| ❌ 无 | `notes.updated_at` | 新增 |
| ❌ 无 | `note_items.sort_order` | 新增 |

---

## 三、C++ 实现规划

### 3.1 新增文件

```
Notes/
├── infra/
│   ├── SqliteNoteRepository.h   [NEW]  ← 实现 INoteRepository，使用 sqlite3
│   └── SqliteNoteRepository.cpp [NEW]
└── packages.config                     ← 添加 SQLite NuGet 包引用
```

### 3.2 `SqliteNoteRepository` 接口设计

```cpp
// infra/SqliteNoteRepository.h
class SqliteNoteRepository : public INoteRepository {
public:
    explicit SqliteNoteRepository(const CString& dbPath = _T(""));
    virtual ~SqliteNoteRepository() override;

    // INoteRepository 接口实现
    virtual std::vector<CString> ListAll() override;
    virtual bool Load(const CString& name, Note& outNote) override;
    virtual void Save(const Note& note) override;
    virtual bool Rename(const CString& oldName, const CString& newName) override;
    virtual void Delete(const CString& name) override;

    // 新增能力（管理器专用）
    void SetDbPath(const CString& path);
    CString GetDbPath() const;
    std::vector<Note> LoadAllNotes();           // 一次性加载全部便签（管理器使用）
    std::vector<Note> SearchByContent(const CString& keyword);  // 全文搜索

private:
    bool EnsureOpen();
    bool EnsureSchema();
    Note RowToNote(sqlite3_stmt* stmt) const;

private:
    CString  m_dbPath;
    sqlite3* m_db = nullptr;
};
```

### 3.3 `NoteManager` 切换

`NoteManager` 目前自持 `JsonNoteRepository m_jsonRepo`，切换时只需替换该成员类型：

```cpp
// host/NoteManager.h — 只改这一处
class NoteManager {
private:
    SqliteNoteRepository m_sqliteRepo;   // 替换 JsonNoteRepository m_jsonRepo
    INoteRepository&     m_repo;         // 保持引用接缝，上层零改动
    NoteService          m_service;
    ...
};
```

### 3.4 数据迁移

利用现有 `DataMigrator` 模式，新增 `SqliteMigrator`：

```
迁移流程（首次启动检测）：
1. 检测 notes.db 是否存在
2. 若不存在，扫描 <NoteDir>/*.json 文件
3. 若存在 JSON 文件，弹出确认对话框（复用 DataMigrator UI 风格）
4. 逐文件读取 JSON → 转换为 Note 对象 → 写入 SQLite
5. 迁移完成后将旧 JSON 文件归档到 <NoteDir>/json_backup/
6. 写入 notes.db 标记迁移已完成
```

---

## 四、便签管理器（独立模块）规划

### 4.1 管理器定位

- **独立于主程序的管理视图**（可以是独立进程，也可以是主程序内新增 MFC 对话框/WebView2 页面）
- **数据源**：直接读取 `notes.db`（SQLite WAL 模式支持多进程安全并发读）
- **写操作**：通过 `SqliteNoteRepository` 的 WAL 写，SQLite 自动串行化

### 4.2 管理器功能列表

| 功能 | 说明 |
| :--- | :--- |
| 便签列表总览 | 显示所有便签：标题、条目数、未完成数、可见状态 |
| 全文搜索 | 跨便签搜索条目内容（`LIKE '%keyword%'`） |
| 批量操作 | 批量显示/隐藏、批量删除 |
| 单便签详情 | 展开查看该便签全部条目，支持勾选/取消 |
| 新建便签 | 通过管理器创建，写入 SQLite，主程序重启后生效（或实时 IPC 通知） |
| 导出 | 将选中便签导出为 Markdown 或 CSV |

### 4.3 UI 技术选型

| 选项 | 说明 | 推荐度 |
| :--- | :--- | :--- |
| 新增 WebView2 + Vue 前端页面 | 与现有主题工程一致，直接复用 ipc.ts 契约 | ⭐⭐⭐⭐⭐ |
| MFC 对话框 | 改动小，无需前端工程 | ⭐⭐⭐ |
| 独立 Electron/Tauri 程序 | 独立进程，SQLite 多读无冲突 | ⭐⭐ |

**推荐**：在主程序内新增一个 `NoteManagerPanel`（WebView2 页面），前端新增 `themes/manager/` 主题工程，读写均通过 `SqliteNoteRepository` 走 SQLite。

---

## 五、实施路径（分阶段）

```
Phase A：SQLite 基础设施（约 1 天）
├── A1 引入 sqlite3 NuGet 包
├── A2 实现 SqliteNoteRepository（含 Schema 初始化）
├── A3 实现 SqliteMigrator（JSON → SQLite 一次性迁移）
└── A4 NoteManager 切换到 SqliteNoteRepository，全链路验证

Phase B：管理器后端（约 0.5 天）
├── B1 SqliteNoteRepository 新增 LoadAllNotes / SearchByContent
└── B2 主程序新增管理器入口（托盘菜单 "管理便签"）

Phase C：管理器前端（约 1~2 天）
├── C1 新建 themes/manager/ Vite + Vue 工程
├── C2 实现便签列表、搜索、批量操作 UI
└── C3 前后端 IPC 联调
```

---

## 六、关键约束

- SQLite 依赖版本：**3.43+**（WAL2 支持，`PRAGMA journal_mode=WAL`）
- `note_items.id` 保持 `uint64_t`（JavaScript 端用 `Date.now() * 1000 + random` 生成，需确认不超过 SQLite INTEGER 范围，SQLite INTEGER 最大 2^63-1，安全）
- 现有 `NoteItem` 的 JSON 字段名 `finish`（不是 `finished`），`SqliteNoteRepository` 在读写 C++ 结构体时使用 `bFinished` 字段即可，字段名不影响 DB 列名
- `DataMigrator::MigrateIfNeeded` 保持签名不变，`SqliteMigrator` 作为新类独立存在

---

*本文件 2026-08-20 更新，SQLite 路线已确认，JSON 方案分析已归档删除。*
