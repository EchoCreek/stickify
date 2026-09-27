// ------------------------------------------------------------------
// Copyright (c) Sticky Notes Refactoring Team (2026)
// Part of Sticky Notes Refactoring Project
// Licensed under the Apache License, Version 2.0
// ------------------------------------------------------------------
#pragma once

#include <afxwin.h>
#include "infra/SqliteNoteRepository.h"

class SqliteMigrator
{
public:
	// 检查并执行迁移：若 SQLite 库为空且存在旧版 JSON 文件，弹出确认对话框，
	// 迁移至 SQLite 后将旧 JSON 文件备份到 json_backup 目录。
	// 返回 true 表示执行了迁移，false 表示无需迁移
	static bool MigrateIfNeeded(SqliteNoteRepository& repo, const CString& notesDir = _T(""), bool autoConfirm = false);
};
