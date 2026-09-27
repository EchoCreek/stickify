// ------------------------------------------------------------------
// Copyright (c) Sticky Notes Refactoring Team (2026)
// Part of Sticky Notes Refactoring Project
// Licensed under the Apache License, Version 2.0
// ------------------------------------------------------------------
#pragma once

#include <afxwin.h>
#include <vector>
#include <cstdint>
#include "NoteItem.h"

class Note
{
public:
	CString name;
	CString title;
	CRect rect;
	COLORREF bgColor;
	int opacity;
	bool opacityEnabled;
	bool visible;
	bool topMost;
	bool isDeleted;
	uint64_t deletedAt;
	bool isArchived;
	uint64_t archivedAt;
	std::vector<NoteItem> items;

public:
	Note();
	explicit Note(const CString& noteName);

	// Static Factory
	static Note Create(const CString& name = _T(""));

	// Color Helper
	CString ToHexColor() const;
};
