// ------------------------------------------------------------------
// Copyright (c) Sticky Notes Refactoring Team (2026)
// Part of Sticky Notes Refactoring Project
// Licensed under the Apache License, Version 2.0
// ------------------------------------------------------------------
#pragma once

#include <afxwin.h>
#include <cstdint>

class NoteItem
{
public:
	uint64_t uId;
	CString  sContent;
	bool     bFinished;

public:
	NoteItem();
	NoteItem(uint64_t id, const CString& content, bool finished = false);

	// Static Factory
	static NoteItem Create(uint64_t id, const CString& content, bool finished = false);
};
