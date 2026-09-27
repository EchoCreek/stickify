// ------------------------------------------------------------------
// Copyright (c) Sticky Notes Refactoring Team (2026)
// Part of Sticky Notes Refactoring Project
// Licensed under the Apache License, Version 2.0
// ------------------------------------------------------------------
#include "stdafx.h"
#include "AppSetting.h"
#include "ref/Path.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

// HotKey modifiers (matching WIN | SHIFT)
static const DWORD HOTKEY_MOD_WIN_SHIFT = (0x08 | 0x04) << 8;

AppSetting::AppSetting()
	: bAutoRun(true)
	, bCustomWebview2(false)
	, dwEditHotKey(HOTKEY_MOD_WIN_SHIFT | VK_F7)
	, dwNewHotKey(HOTKEY_MOD_WIN_SHIFT | VK_F8)
	, dwUnActiveHotKey(HOTKEY_MOD_WIN_SHIFT | VK_F9)
	, dwActiveAllHotKey(HOTKEY_MOD_WIN_SHIFT | VK_F10)
	, sNoteDir(_T(""))
	, sWebview2Path(_T(""))
	, sTheme(_T("Default"))
	, sDefaultBgColor(_T("#0d1117"))
	, sLanguage(_T("zh-CN"))
{
	sNoteDir = Easy::Path::GetCurDirectory(_T("notes\\"));
}

bool AppSetting::IsValid() const
{
	return !sNoteDir.IsEmpty() && !sTheme.IsEmpty();
}

CString AppSetting::ToJson() const
{
	rapidjson::GenericDocument<rapidjson::UTF16<TCHAR>> doc;
	doc.SetObject();
	auto& alloc = doc.GetAllocator();

	doc.AddMember(rapidjson::StringRef(_T("autoRun")), bAutoRun, alloc);
	doc.AddMember(rapidjson::StringRef(_T("customWebview2")), bCustomWebview2, alloc);
	doc.AddMember(rapidjson::StringRef(_T("webview2Path")), rapidjson::GenericValue<rapidjson::UTF16<TCHAR>>(sWebview2Path.GetString(), alloc).Move(), alloc);
	doc.AddMember(rapidjson::StringRef(_T("noteDir")), rapidjson::GenericValue<rapidjson::UTF16<TCHAR>>(sNoteDir.GetString(), alloc).Move(), alloc);
	doc.AddMember(rapidjson::StringRef(_T("theme")), rapidjson::GenericValue<rapidjson::UTF16<TCHAR>>(sTheme.GetString(), alloc).Move(), alloc);
	doc.AddMember(rapidjson::StringRef(_T("defaultBgColor")), rapidjson::GenericValue<rapidjson::UTF16<TCHAR>>(sDefaultBgColor.GetString(), alloc).Move(), alloc);
	doc.AddMember(rapidjson::StringRef(_T("language")), rapidjson::GenericValue<rapidjson::UTF16<TCHAR>>(sLanguage.GetString(), alloc).Move(), alloc);
	doc.AddMember(rapidjson::StringRef(_T("editHotKey")), static_cast<uint32_t>(dwEditHotKey), alloc);
	doc.AddMember(rapidjson::StringRef(_T("newHotKey")), static_cast<uint32_t>(dwNewHotKey), alloc);
	doc.AddMember(rapidjson::StringRef(_T("unActiveHotKey")), static_cast<uint32_t>(dwUnActiveHotKey), alloc);
	doc.AddMember(rapidjson::StringRef(_T("activeAllHotKey")), static_cast<uint32_t>(dwActiveAllHotKey), alloc);

	rapidjson::GenericStringBuffer<rapidjson::UTF16<TCHAR>> buffer;
	rapidjson::Writer<rapidjson::GenericStringBuffer<rapidjson::UTF16<TCHAR>>, rapidjson::UTF16<TCHAR>, rapidjson::UTF16<TCHAR>> writer(buffer);
	doc.Accept(writer);

	return CString(buffer.GetString());
}

AppSetting AppSetting::FromJson(const wchar_t* jsonStr)
{
	AppSetting setting;
	if (jsonStr == nullptr || jsonStr[0] == L'\0')
	{
		return setting;
	}

	rapidjson::GenericDocument<rapidjson::UTF16<TCHAR>> doc;
	doc.Parse(jsonStr);
	if (doc.HasParseError() || !doc.IsObject())
	{
		return setting;
	}

	if (doc.HasMember(_T("autoRun")) && doc[_T("autoRun")].IsBool())
		setting.bAutoRun = doc[_T("autoRun")].GetBool();
	else if (doc.HasMember(_T("autorun")) && doc[_T("autorun")].IsBool())
		setting.bAutoRun = doc[_T("autorun")].GetBool();

	if (doc.HasMember(_T("customWebview2")) && doc[_T("customWebview2")].IsBool())
		setting.bCustomWebview2 = doc[_T("customWebview2")].GetBool();
	else if (doc.HasMember(_T("customwebview2")) && doc[_T("customwebview2")].IsBool())
		setting.bCustomWebview2 = doc[_T("customwebview2")].GetBool();

	if (doc.HasMember(_T("webview2Path")) && doc[_T("webview2Path")].IsString())
		setting.sWebview2Path = doc[_T("webview2Path")].GetString();
	else if (doc.HasMember(_T("webview2path")) && doc[_T("webview2path")].IsString())
		setting.sWebview2Path = doc[_T("webview2path")].GetString();

	if (doc.HasMember(_T("noteDir")) && doc[_T("noteDir")].IsString())
		setting.sNoteDir = doc[_T("noteDir")].GetString();
	else if (doc.HasMember(_T("notedir")) && doc[_T("notedir")].IsString())
		setting.sNoteDir = doc[_T("notedir")].GetString();

	if (doc.HasMember(_T("theme")) && doc[_T("theme")].IsString())
		setting.sTheme = doc[_T("theme")].GetString();

	if (doc.HasMember(_T("defaultBgColor")) && doc[_T("defaultBgColor")].IsString())
		setting.sDefaultBgColor = doc[_T("defaultBgColor")].GetString();
	else if (doc.HasMember(_T("defaultbgcolor")) && doc[_T("defaultbgcolor")].IsString())
		setting.sDefaultBgColor = doc[_T("defaultbgcolor")].GetString();

	if (doc.HasMember(_T("language")) && doc[_T("language")].IsString())
		setting.sLanguage = doc[_T("language")].GetString();
	else if (doc.HasMember(_T("lang")) && doc[_T("lang")].IsString())
		setting.sLanguage = doc[_T("lang")].GetString();

	if (setting.sLanguage.IsEmpty())
	{
		setting.sLanguage = _T("zh-CN");
	}

	auto parseHotKey = [&doc](const wchar_t* k1, const wchar_t* k2, DWORD& outVal) {
		if (doc.HasMember(k1))
		{
			if (doc[k1].IsUint()) outVal = doc[k1].GetUint();
			else if (doc[k1].IsInt()) outVal = static_cast<DWORD>(doc[k1].GetInt());
		}
		else if (k2 && doc.HasMember(k2))
		{
			if (doc[k2].IsUint()) outVal = doc[k2].GetUint();
			else if (doc[k2].IsInt()) outVal = static_cast<DWORD>(doc[k2].GetInt());
		}
	};

	parseHotKey(_T("editHotKey"), _T("edithotkey"), setting.dwEditHotKey);
	parseHotKey(_T("newHotKey"), _T("newhotkey"), setting.dwNewHotKey);
	parseHotKey(_T("unActiveHotKey"), _T("unactivehotkey"), setting.dwUnActiveHotKey);
	parseHotKey(_T("activeAllHotKey"), _T("activeallhotkey"), setting.dwActiveAllHotKey);

	return setting;
}

AppSetting AppSetting::FromJson(const CString& jsonStr)
{
	return FromJson(jsonStr.GetString());
}
