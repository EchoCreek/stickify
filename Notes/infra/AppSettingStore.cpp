// ------------------------------------------------------------------
// Copyright (c) Sticky Notes Refactoring Team (2026)
// Part of Sticky Notes Refactoring Project
// Licensed under the Apache License, Version 2.0
// ------------------------------------------------------------------
#include "stdafx.h"
#include "infra/AppSettingStore.h"
#include "ref/Path.h"
#include "ref/Ini.h"

AppSetting AppSettingStore::Load()
{
	AppSetting setting;
	CString sConfigFile = Easy::Path::GetCurDirectory(_T("setting.ini"));

	if (!Easy::Path::Exists(sConfigFile))
	{
		if (setting.sTheme.IsEmpty())
		{
			setting.sTheme = _T("Default");
		}
		return setting;
	}

	Easy::Ini ini(sConfigFile);
	ini.Read(_T("HotKey"), _T("Edit"), setting.dwEditHotKey);
	ini.Read(_T("HotKey"), _T("New"), setting.dwNewHotKey);
	ini.Read(_T("HotKey"), _T("UnActive"), setting.dwUnActiveHotKey);
	ini.Read(_T("HotKey"), _T("ActiveAll"), setting.dwActiveAllHotKey);
	ini.Read(_T("Setting"), _T("NoteDir"), setting.sNoteDir);
	ini.Read(_T("Setting"), _T("Theme"), setting.sTheme);
	ini.Read(_T("Setting"), _T("AutoRun"), setting.bAutoRun);
	ini.Read(_T("Setting"), _T("CustomWebview2"), setting.bCustomWebview2);
	ini.Read(_T("Setting"), _T("Webview2Path"), setting.sWebview2Path);
	ini.Read(_T("Setting"), _T("DefaultBgColor"), setting.sDefaultBgColor);
	ini.Read(_T("Setting"), _T("Language"), setting.sLanguage);

	if (setting.sLanguage.IsEmpty())
	{
		setting.sLanguage = _T("zh-CN");
	}

	if (setting.sDefaultBgColor.IsEmpty())
	{
		setting.sDefaultBgColor = _T("#0d1117");
	}

	if (setting.sTheme.IsEmpty())
	{
		setting.sTheme = _T("Default");
	}

	return setting;
}

void AppSettingStore::Save(const AppSetting& setting)
{
	CString sConfigFile = Easy::Path::GetCurDirectory(_T("setting.ini"));

	AppSetting toSave = setting;
	if (!toSave.sNoteDir.IsEmpty())
	{
		toSave.sNoteDir = toSave.sNoteDir.Trim('\\') + _T("\\");
	}

	if (toSave.sLanguage.IsEmpty())
	{
		toSave.sLanguage = _T("zh-CN");
	}

	Easy::Ini ini(sConfigFile);
	ini.Write(_T("HotKey"), _T("Edit"), toSave.dwEditHotKey);
	ini.Write(_T("HotKey"), _T("New"), toSave.dwNewHotKey);
	ini.Write(_T("HotKey"), _T("UnActive"), toSave.dwUnActiveHotKey);
	ini.Write(_T("HotKey"), _T("ActiveAll"), toSave.dwActiveAllHotKey);
	ini.Write(_T("Setting"), _T("NoteDir"), toSave.sNoteDir);
	ini.Write(_T("Setting"), _T("Theme"), toSave.sTheme);
	ini.Write(_T("Setting"), _T("AutoRun"), toSave.bAutoRun);
	ini.Write(_T("Setting"), _T("CustomWebview2"), toSave.bCustomWebview2);
	ini.Write(_T("Setting"), _T("Webview2Path"), toSave.sWebview2Path);
	ini.Write(_T("Setting"), _T("DefaultBgColor"), toSave.sDefaultBgColor);
	ini.Write(_T("Setting"), _T("Language"), toSave.sLanguage);
}

std::vector<CString> AppSettingStore::SearchThemes()
{
	std::vector<CString> lstName;
	std::vector<CString> lstConfig = Easy::Path::GetFileList(Easy::Path::GetCurDirectory(_T("themes")), _T("*"), true);
	for (size_t i = 0; i < lstConfig.size(); i++)
	{
		if (!Easy::Path::Exists(_PATH_JOIN(lstConfig[i], _T("index.html"))))
		{
			continue;
		}
		CString sNote = Easy::Path::GetFileName(lstConfig[i]);
		if (sNote.CompareNoCase(_T("Manager")) == 0)
		{
			continue;
		}
		lstName.push_back(sNote);
	}
	return lstName;
}
