// ------------------------------------------------------------------
// Copyright (c) Sticky Notes Refactoring Team (2026)
// Part of Sticky Notes Refactoring Project
// Licensed under the Apache License, Version 2.0
// ------------------------------------------------------------------
#pragma once

#include <vector>
#include <afxwin.h>
#include "core/ports/INoteRepository.h"

struct sqlite3;
struct sqlite3_stmt;

class SqliteNoteRepository : public INoteRepository
{
public:
	explicit SqliteNoteRepository(const CString& dbPath = _T(""));
	virtual ~SqliteNoteRepository() override;

	// INoteRepository interface implementation
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

	// Database management & extended queries
	void SetDbPath(const CString& dbPath);
	CString GetDbPath() const;
	bool Checkpoint();
	bool SwitchDatabase(const CString& newDbPath, bool copyIfTargetMissing = true);
	bool ExportDatabase(const CString& targetBackupPath);
	bool ImportDatabase(const CString& sourceBackupPath);
	void Close();

	std::vector<Note> LoadAllNotes();
	std::vector<Note> LoadArchivedNotes();
	std::vector<Note> LoadTrashNotes();
	std::vector<Note> SearchByContent(const CString& keyword);

private:
	bool EnsureOpen();
	bool EnsureSchema();
	CString EnsureDbPath();

	/// Bulk-load all notes matching the given WHERE/ORDER clauses.
	/// Executes exactly 2 SQL queries regardless of note count (eliminates N+1).
	/// @param notesFilter  raw SQL fragment for the WHERE clause on `notes` (no user input)
	/// @param notesOrder   raw SQL fragment for the ORDER BY clause on `notes`
	std::vector<Note> LoadBulkByFilter(const wchar_t* notesFilter, const wchar_t* notesOrder);

	static COLORREF HexToColor(const CString& hex);

	void FinalizeCachedStatements();
	sqlite3_stmt* GetOrCreateStmt(sqlite3_stmt*& stmtSlot, const wchar_t* sql);

private:
	CString  m_dbPath;
	sqlite3* m_db = nullptr;

	// High-frequency PreparedStatement statement cache (P3-A)
	sqlite3_stmt* m_stmtLoadNote       = nullptr;
	sqlite3_stmt* m_stmtLoadItems      = nullptr;
	sqlite3_stmt* m_stmtSaveNote       = nullptr;
	sqlite3_stmt* m_stmtDelItems       = nullptr;
	sqlite3_stmt* m_stmtInsItem        = nullptr;
	sqlite3_stmt* m_stmtArchive        = nullptr;
	sqlite3_stmt* m_stmtUnarchive      = nullptr;
	sqlite3_stmt* m_stmtSoftDelete     = nullptr;
	sqlite3_stmt* m_stmtRestore        = nullptr;
	sqlite3_stmt* m_stmtPermDelItems   = nullptr;
	sqlite3_stmt* m_stmtPermDelNote    = nullptr;
};
