// ------------------------------------------------------------------
// Copyright (c) Sticky Notes Refactoring Team (2026)
// Part of Sticky Notes Refactoring Project
// Licensed under the Apache License, Version 2.0
// ------------------------------------------------------------------
#pragma once

#include <afxwin.h>

class AppSetting
{
public:
	bool bAutoRun;
	bool bCustomWebview2;
	DWORD dwEditHotKey;
	DWORD dwNewHotKey;
	DWORD dwUnActiveHotKey;
	DWORD dwActiveAllHotKey;
	CString sNoteDir;
	CString sWebview2Path;
	CString sTheme;
	CString sDefaultBgColor;
	CString sLanguage;

public:
	AppSetting();

	// Validation
	bool IsValid() const;

	// JSON Serialization
	CString ToJson() const;
	static AppSetting FromJson(const wchar_t* jsonStr);
	static AppSetting FromJson(const CString& jsonStr);
};
