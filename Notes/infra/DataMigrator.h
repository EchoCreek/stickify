// ------------------------------------------------------------------
// Copyright (c) Sticky Notes Refactoring Team (2026)
// Part of Sticky Notes Refactoring Project
// Licensed under the Apache License, Version 2.0
// ------------------------------------------------------------------
#pragma once

#include "core/ports/INoteRepository.h"

class DataMigrator {
public:
	// 检查并执行迁移：若存在旧 INI 文件，弹出确认对话框，迁移后删除旧文件
	// 返回 true 表示执行了迁移，false 表示无需迁移
	static bool MigrateIfNeeded(INoteRepository& newRepo, bool autoConfirm = false);
};
