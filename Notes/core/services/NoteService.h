// ------------------------------------------------------------------
// Copyright (c) Sticky Notes Refactoring Team (2026)
// Part of Sticky Notes Refactoring Project
// Licensed under the Apache License, Version 2.0
// ------------------------------------------------------------------
#pragma once

#include <afxwin.h>
#include <vector>
#include "core/domain/Note.h"
#include "core/domain/NoteItem.h"
#include "core/ports/INoteRepository.h"

class NoteService
{
public:
	explicit NoteService(INoteRepository& repo);
	~NoteService() = default;

	// 条目操作
	bool AddItem(Note& note, const NoteItem& item);
	bool UpdateItem(Note& note, const NoteItem& item);
	bool RemoveItem(Note& note, uint64_t itemId);
	bool UpdateAllItems(Note& note, const std::vector<NoteItem>& items);

	// 便签整体操作
	void UpdateAppearance(Note& note, COLORREF bgColor, bool opacityEnabled, int opacity, bool topMost);
	void UpdateTitle(Note& note, const CString& title);
	void UpdateRect(Note& note, const CRect& rect);
	bool Rename(Note& note, const CString& newName);
	void SetVisibility(Note& note, bool visible);
	void Hide(Note& note);
	void Clear(const Note& note);
	bool Archive(Note& note);
	bool Archive(const CString& name);
	bool Unarchive(Note& note);
	bool Unarchive(const CString& name);
	bool SoftDelete(Note& note);
	bool SoftDelete(const CString& name);
	bool Restore(Note& note);
	bool Restore(const CString& name);
	bool PermanentDelete(const CString& name);
	bool ClearTrash();

	// 日历导出（原 CNote::MakeTask）
	void ExportToCalendar(const NoteItem& item);

private:
	INoteRepository& m_repo;
};
