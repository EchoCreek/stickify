// ------------------------------------------------------------------
// Copyright (c) Sticky Notes Refactoring Team (2026)
// Part of Sticky Notes Refactoring Project
// Licensed under the Apache License, Version 2.0
// ------------------------------------------------------------------
#pragma once

#include "core/ports/INoteRepository.h"

class JsonNoteRepository : public INoteRepository
{
public:
	explicit JsonNoteRepository(const CString& storageDir = _T(""));
	virtual ~JsonNoteRepository() override = default;

	virtual std::vector<CString> ListAll() override;
	virtual std::vector<CString> ListActive() override;
	virtual std::vector<CString> ListArchived() override;
	virtual std::vector<CString> ListTrash() override;
	virtual bool Load(const CString& name, Note& outNote) override;
	virtual void Save(const Note& note) override;
	virtual bool Rename(const CString& oldName, const CString& newName) override;
	virtual void Delete(const CString& name) override;
	virtual bool Archive(const CString& name) override;
	virtual bool Unarchive(const CString& name) override;
	virtual bool SoftDelete(const CString& name) override;
	virtual bool Restore(const CString& name) override;
	virtual bool PermanentDelete(const CString& name) override;
	virtual bool ClearTrash() override;

	void SetStorageDir(const CString& dir);
	CString GetStorageDir() const;

private:
	CString EnsureDirectory() const;

private:
	CString m_storageDir;
};
