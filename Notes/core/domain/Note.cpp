// ------------------------------------------------------------------
// Copyright (c) Sticky Notes Refactoring Team (2026)
// Part of Sticky Notes Refactoring Project
// Licensed under the Apache License, Version 2.0
// ------------------------------------------------------------------
#include "stdafx.h"
#include "Note.h"

static CString GenerateNoteTimestamp()
{
	SYSTEMTIME st;
	GetLocalTime(&st);
	CString sName;
	sName.Format(_T("%04d%02d%02d%02d%02d%02d%03d"),
		st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	return sName;
}

Note::Note()
	: Note(_T(""))
{
}

Note::Note(const CString& noteName)
	: bgColor(RGB(11, 15, 20))
	, rect(100, 100, 530, 530)
	, opacity(50)
	, opacityEnabled(false)
	, visible(true)
	, topMost(true)
	, isDeleted(false)
	, deletedAt(0)
	, isArchived(false)
	, archivedAt(0)
	, title(_T(""))
{
	if (!noteName.IsEmpty())
	{
		name = noteName;
	}
	else
	{
		name = GenerateNoteTimestamp();
	}
}

Note Note::Create(const CString& name)
{
	return Note(name);
}

CString Note::ToHexColor() const
{
	CString sHex;
	sHex.Format(_T("#%02X%02X%02X"), GetRValue(bgColor), GetGValue(bgColor), GetBValue(bgColor));
	return sHex;
}
