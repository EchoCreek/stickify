// ------------------------------------------------------------------
// Copyright (c) Sticky Notes Refactoring Team (2026)
// Part of Sticky Notes Refactoring Project
// Licensed under the Apache License, Version 2.0
// ------------------------------------------------------------------
#pragma once

#include <vector>
#include <afxwin.h>
#include "core/domain/AppSetting.h"

class AppSettingStore {
public:
	static AppSetting Load();
	static void Save(const AppSetting& setting);
	static std::vector<CString> SearchThemes();
};
