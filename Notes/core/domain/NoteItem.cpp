// ------------------------------------------------------------------
// Copyright (c) Sticky Notes Refactoring Team (2026)
// Part of Sticky Notes Refactoring Project
// Licensed under the Apache License, Version 2.0
// ------------------------------------------------------------------
#include "stdafx.h"
#include "NoteItem.h"

NoteItem::NoteItem()
	: uId(0)
	, sContent(_T(""))
	, bFinished(false)
{
}

NoteItem::NoteItem(uint64_t id, const CString& content, bool finished)
	: uId(id)
	, sContent(content)
	, bFinished(finished)
{
}

NoteItem NoteItem::Create(uint64_t id, const CString& content, bool finished)
{
	return NoteItem(id, content, finished);
}
