// ------------------------------------------------------------------
// Copyright (c) Sticky Notes Refactoring Team (2026)
// Part of Sticky Notes Refactoring Project
// Licensed under the Apache License, Version 2.0
// ------------------------------------------------------------------
#pragma once

#include <vector>
#include <afxwin.h>
#include "core/domain/Note.h"

class INoteRepository
{
public:
	virtual ~INoteRepository() = default;
	virtual std::vector<CString> ListAll() = 0;
	virtual std::vector<CString> ListActive() = 0;
	virtual std::vector<CString> ListArchived() = 0;
	virtual std::vector<CString> ListTrash() = 0;
	virtual bool Load(const CString& name, Note& outNote) = 0;
	virtual void Save(const Note& note) = 0;
	virtual bool Rename(const CString& oldName, const CString& newName) = 0;
	virtual void Delete(const CString& name) = 0;
	virtual bool Archive(const CString& name) = 0;
	virtual bool Unarchive(const CString& name) = 0;
	virtual bool SoftDelete(const CString& name) = 0;
	virtual bool Restore(const CString& name) = 0;
	virtual bool PermanentDelete(const CString& name) = 0;
	virtual bool ClearTrash() = 0;
};
