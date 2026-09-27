// ------------------------------------------------------------------
// Copyright (c) Sticky Notes Refactoring Team (2026)
// Part of Sticky Notes Refactoring Project
// Licensed under the Apache License, Version 2.0
// ------------------------------------------------------------------
#include "stdafx.h"
#include "SqliteNoteRepository.h"
#include "ref/sqlite/sqlite3.h"
#include "ref/Path.h"
#include "ref/Log.h"
#include <chrono>
#include <unordered_map>
#include <string>

namespace {
	class SqliteStmtHelper
	{
	public:
		SqliteStmtHelper(sqlite3* db, const wchar_t* sql)
		{
			sqlite3_prepare16_v2(db, sql, -1, &m_stmt, nullptr);
		}
		~SqliteStmtHelper()
		{
			if (m_stmt)
			{
				sqlite3_finalize(m_stmt);
			}
		}
		sqlite3_stmt* Get() const { return m_stmt; }
		operator sqlite3_stmt*() const { return m_stmt; }
		bool IsValid() const { return m_stmt != nullptr; }

		void Reset()
		{
			if (m_stmt)
			{
				sqlite3_reset(m_stmt);
				sqlite3_clear_bindings(m_stmt);
			}
		}
	private:
		sqlite3_stmt* m_stmt = nullptr;
	};

	/// RAII Guard for cached prepared statements.
	/// Does NOT finalize the statement on destruction — only resets & clears bindings.
	class CachedStmtGuard
	{
	public:
		explicit CachedStmtGuard(sqlite3_stmt* stmt) : m_stmt(stmt) {}
		~CachedStmtGuard()
		{
			if (m_stmt)
			{
				sqlite3_reset(m_stmt);
				sqlite3_clear_bindings(m_stmt);
			}
		}
		sqlite3_stmt* Get() const { return m_stmt; }
		operator sqlite3_stmt*() const { return m_stmt; }
		bool IsValid() const { return m_stmt != nullptr; }

		void Reset()
		{
			if (m_stmt)
			{
				sqlite3_reset(m_stmt);
				sqlite3_clear_bindings(m_stmt);
			}
		}
	private:
		sqlite3_stmt* m_stmt = nullptr;
	};
}

SqliteNoteRepository::SqliteNoteRepository(const CString& dbPath)
	: m_dbPath(dbPath)
	, m_db(nullptr)
{
}

SqliteNoteRepository::~SqliteNoteRepository()
{
	Close();
}

void SqliteNoteRepository::SetDbPath(const CString& dbPath)
{
	if (m_dbPath != dbPath)
	{
		Close();
		m_dbPath = dbPath;
	}
}

CString SqliteNoteRepository::GetDbPath() const
{
	return m_dbPath;
}

bool SqliteNoteRepository::Checkpoint()
{
	if (!m_db) return true;
	sqlite3_exec(m_db, "PRAGMA incremental_vacuum(50);", nullptr, nullptr, nullptr);
	int rc = sqlite3_wal_checkpoint_v2(m_db, nullptr, SQLITE_CHECKPOINT_TRUNCATE, nullptr, nullptr);
	if (rc != SQLITE_OK)
	{
		CLogApp::Warn(_T("SqliteNoteRepository::Checkpoint failed: rc=%d"), rc);
		return false;
	}
	return true;
}

bool SqliteNoteRepository::SwitchDatabase(const CString& newDbPath, bool copyIfTargetMissing)
{
	if (newDbPath.IsEmpty()) return false;

	CString oldPath = m_dbPath.IsEmpty() ? EnsureDbPath() : m_dbPath;
	if (oldPath.CompareNoCase(newDbPath) == 0)
	{
		return true;
	}

	// 1. Read notes from source before closing
	std::vector<Note> sourceNotes;
	if (m_db)
	{
		Checkpoint();
		sourceNotes = LoadAllNotes();
		Close();
	}
	else if (Easy::Path::Exists(oldPath))
	{
		SqliteNoteRepository oldRepo(oldPath);
		sourceNotes = oldRepo.LoadAllNotes();
	}

	// 2. Ensure target directory exists
	CString targetDir = Easy::Path::GetDirectory(newDbPath);
	if (!targetDir.IsEmpty() && !Easy::Path::Exists(targetDir))
	{
		Easy::Path::Create(targetDir);
	}

	// 3. If target DB doesn't exist and source DB exists, copy it
	if (copyIfTargetMissing && !Easy::Path::Exists(newDbPath) && Easy::Path::Exists(oldPath))
	{
		BOOL copyOk = ::CopyFile(oldPath, newDbPath, FALSE);
		if (!copyOk)
		{
			CLogApp::Error(_T("SqliteNoteRepository::SwitchDatabase copy failed from '%s' to '%s', err=%d"),
				oldPath.GetString(), newDbPath.GetString(), ::GetLastError());
		}
		else
		{
			// Also copy json_backup if present in old directory
			CString oldDir = Easy::Path::GetDirectory(oldPath);
			CString oldJsonBackup = Easy::Path::Resolve(oldDir, _T("json_backup"));
			CString newJsonBackup = Easy::Path::Resolve(targetDir, _T("json_backup"));
			if (Easy::Path::Exists(oldJsonBackup) && !Easy::Path::Exists(newJsonBackup))
			{
				Easy::Path::Create(newJsonBackup);
				std::vector<CString> jsonFiles = Easy::Path::GetFileList(oldJsonBackup, _T("*.json"));
				for (const auto& jf : jsonFiles)
				{
					CString fileName = Easy::Path::GetFileName(jf);
					::CopyFile(jf, Easy::Path::Resolve(newJsonBackup, fileName), FALSE);
				}
			}
		}
	}

	// 4. Update path and re-open
	m_dbPath = newDbPath;
	bool openOk = EnsureOpen();
	if (openOk)
	{
		// If target database is empty (0 notes) and source had notes, migrate them into target!
		std::vector<CString> currentNotes = ListAll();
		if (currentNotes.empty() && !sourceNotes.empty())
		{
			CLogApp::Info(_T("SqliteNoteRepository::SwitchDatabase: Target DB is empty, migrating %d notes from source"),
				static_cast<int>(sourceNotes.size()));
			for (const auto& note : sourceNotes)
			{
				Save(note);
			}
			Checkpoint();
		}
	}
	return openOk;
}

void SqliteNoteRepository::FinalizeCachedStatements()
{
	auto finalizeOne = [](sqlite3_stmt*& stmt) {
		if (stmt)
		{
			sqlite3_finalize(stmt);
			stmt = nullptr;
		}
	};
	finalizeOne(m_stmtLoadNote);
	finalizeOne(m_stmtLoadItems);
	finalizeOne(m_stmtSaveNote);
	finalizeOne(m_stmtDelItems);
	finalizeOne(m_stmtInsItem);
	finalizeOne(m_stmtArchive);
	finalizeOne(m_stmtUnarchive);
	finalizeOne(m_stmtSoftDelete);
	finalizeOne(m_stmtRestore);
	finalizeOne(m_stmtPermDelItems);
	finalizeOne(m_stmtPermDelNote);
}

sqlite3_stmt* SqliteNoteRepository::GetOrCreateStmt(sqlite3_stmt*& stmtSlot, const wchar_t* sql)
{
	if (!m_db) return nullptr;
	if (stmtSlot == nullptr)
	{
		int rc = sqlite3_prepare16_v2(m_db, sql, -1, &stmtSlot, nullptr);
		if (rc != SQLITE_OK)
		{
			CLogApp::Warn(_T("SqliteNoteRepository: Failed to prepare cached stmt rc=%d"), rc);
			stmtSlot = nullptr;
		}
	}
	return stmtSlot;
}

void SqliteNoteRepository::Close()
{
	if (m_db)
	{
		FinalizeCachedStatements();
		Checkpoint();
		sqlite3_close(m_db);
		m_db = nullptr;
	}
}

CString SqliteNoteRepository::EnsureDbPath()
{
	if (m_dbPath.IsEmpty())
	{
		CString dir = Easy::Path::GetCurDirectory(_T("notes\\"));
		if (!Easy::Path::Exists(dir))
		{
			Easy::Path::Create(dir);
		}
		m_dbPath = Easy::Path::Resolve(dir, _T("notes.db"));
	}
	return m_dbPath;
}

bool SqliteNoteRepository::EnsureOpen()
{
	if (m_db != nullptr)
	{
		return true;
	}

	CString dbPath = EnsureDbPath();
	CString dir = Easy::Path::GetDirectory(dbPath);
	if (!dir.IsEmpty() && !Easy::Path::Exists(dir))
	{
		Easy::Path::Create(dir);
	}

	int rc = sqlite3_open16(dbPath.GetString(), &m_db);
	if (rc != SQLITE_OK)
	{
		CLogApp::Error(_T("SqliteNoteRepository: Failed to open db '%s', rc=%d"), dbPath.GetString(), rc);
		if (m_db)
		{
			sqlite3_close(m_db);
			m_db = nullptr;
		}
		return false;
	}

	sqlite3_busy_timeout(m_db, 5000);
	sqlite3_exec(m_db, "PRAGMA journal_mode = WAL;", nullptr, nullptr, nullptr);
	sqlite3_exec(m_db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);
	sqlite3_exec(m_db, "PRAGMA synchronous = NORMAL;", nullptr, nullptr, nullptr);

	return EnsureSchema();
}

bool SqliteNoteRepository::EnsureSchema()
{
	if (!m_db) return false;

	const char* sql =
		"CREATE TABLE IF NOT EXISTS notes ("
		"    name        TEXT    PRIMARY KEY,"
		"    title       TEXT    NOT NULL DEFAULT '',"
		"    win_left    INTEGER NOT NULL DEFAULT 100,"
		"    win_top     INTEGER NOT NULL DEFAULT 100,"
		"    win_right   INTEGER NOT NULL DEFAULT 530,"
		"    win_bottom  INTEGER NOT NULL DEFAULT 530,"
		"    bgcolor     TEXT    NOT NULL DEFAULT '#0d1117',"
		"    opacity     INTEGER NOT NULL DEFAULT 50,"
		"    opacity_on  INTEGER NOT NULL DEFAULT 0,"
		"    visible     INTEGER NOT NULL DEFAULT 1,"
		"    topmost     INTEGER NOT NULL DEFAULT 1,"
		"    sort_order  INTEGER NOT NULL DEFAULT 0,"
		"    is_deleted  INTEGER NOT NULL DEFAULT 0,"
		"    deleted_at  INTEGER NOT NULL DEFAULT 0,"
		"    is_archived INTEGER NOT NULL DEFAULT 0,"
		"    archived_at INTEGER NOT NULL DEFAULT 0,"
		"    updated_at  INTEGER NOT NULL DEFAULT 0"
		");"
		"CREATE TABLE IF NOT EXISTS note_items ("
		"    id          INTEGER PRIMARY KEY,"
		"    note_name   TEXT    NOT NULL REFERENCES notes(name) ON DELETE CASCADE ON UPDATE CASCADE,"
		"    content     TEXT    NOT NULL DEFAULT '',"
		"    finished    INTEGER NOT NULL DEFAULT 0,"
		"    sort_order  INTEGER NOT NULL DEFAULT 0"
		");"
		"CREATE INDEX IF NOT EXISTS idx_note_items_note_name ON note_items(note_name);";

	char* err = nullptr;
	int rc = sqlite3_exec(m_db, sql, nullptr, nullptr, &err);
	if (rc != SQLITE_OK)
	{
		CLogApp::Error(_T("SqliteNoteRepository: Failed to create schema, err=%S"), err ? err : "unknown");
		if (err) sqlite3_free(err);
		return false;
	}

	// Dynamic migration: check if columns exist in older tables
	bool hasIsDeleted = false;
	bool hasDeletedAt = false;
	bool hasIsArchived = false;
	bool hasArchivedAt = false;
	const wchar_t* sqlInfo = L"PRAGMA table_info(notes);";
	SqliteStmtHelper stmtInfo(m_db, sqlInfo);
	if (stmtInfo.IsValid())
	{
		while (sqlite3_step(stmtInfo.Get()) == SQLITE_ROW)
		{
			const char* colName = (const char*)sqlite3_column_text(stmtInfo.Get(), 1);
			if (colName)
			{
				if (_stricmp(colName, "is_deleted") == 0) hasIsDeleted = true;
				if (_stricmp(colName, "deleted_at") == 0) hasDeletedAt = true;
				if (_stricmp(colName, "is_archived") == 0) hasIsArchived = true;
				if (_stricmp(colName, "archived_at") == 0) hasArchivedAt = true;
			}
		}
	}

	if (!hasIsDeleted)
	{
		sqlite3_exec(m_db, "ALTER TABLE notes ADD COLUMN is_deleted INTEGER NOT NULL DEFAULT 0;", nullptr, nullptr, nullptr);
	}
	if (!hasDeletedAt)
	{
		sqlite3_exec(m_db, "ALTER TABLE notes ADD COLUMN deleted_at INTEGER NOT NULL DEFAULT 0;", nullptr, nullptr, nullptr);
	}
	if (!hasIsArchived)
	{
		sqlite3_exec(m_db, "ALTER TABLE notes ADD COLUMN is_archived INTEGER NOT NULL DEFAULT 0;", nullptr, nullptr, nullptr);
	}
	if (!hasArchivedAt)
	{
		sqlite3_exec(m_db, "ALTER TABLE notes ADD COLUMN archived_at INTEGER NOT NULL DEFAULT 0;", nullptr, nullptr, nullptr);
	}

	sqlite3_exec(m_db, "CREATE INDEX IF NOT EXISTS idx_notes_is_deleted ON notes(is_deleted);", nullptr, nullptr, nullptr);
	sqlite3_exec(m_db, "CREATE INDEX IF NOT EXISTS idx_notes_is_archived ON notes(is_archived);", nullptr, nullptr, nullptr);
	sqlite3_exec(m_db, "CREATE INDEX IF NOT EXISTS idx_notes_active_sort ON notes(is_deleted, is_archived, sort_order ASC, rowid ASC);", nullptr, nullptr, nullptr);
	sqlite3_exec(m_db, "CREATE INDEX IF NOT EXISTS idx_notes_archived_sort ON notes(is_archived, is_deleted, archived_at DESC, rowid DESC);", nullptr, nullptr, nullptr);
	sqlite3_exec(m_db, "CREATE INDEX IF NOT EXISTS idx_notes_trash_sort ON notes(is_deleted, deleted_at DESC, rowid DESC);", nullptr, nullptr, nullptr);
	sqlite3_exec(m_db, "CREATE INDEX IF NOT EXISTS idx_note_items_batch_sort ON note_items(note_name, sort_order ASC, rowid ASC);", nullptr, nullptr, nullptr);

	return true;
}

COLORREF SqliteNoteRepository::HexToColor(const CString& hex)
{
	CString sColor = hex;
	sColor.TrimLeft(_T('#'));
	if (sColor.GetLength() == 6)
	{
		unsigned int r = 0, g = 0, b = 0;
		if (_stscanf_s(sColor.GetString(), _T("%02x%02x%02x"), &r, &g, &b) == 3)
		{
			return RGB(r & 0xFF, g & 0xFF, b & 0xFF);
		}
	}
	return RGB(11, 15, 20);
}

std::vector<CString> SqliteNoteRepository::ListAll()
{
	return ListActive();
}

std::vector<CString> SqliteNoteRepository::ListActive()
{
	std::vector<CString> list;
	if (!EnsureOpen()) return list;

	const wchar_t* sql = L"SELECT name FROM notes WHERE is_deleted = 0 AND is_archived = 0 ORDER BY sort_order ASC, rowid ASC;";
	SqliteStmtHelper stmt(m_db, sql);
	if (!stmt.IsValid()) return list;

	while (sqlite3_step(stmt.Get()) == SQLITE_ROW)
	{
		const wchar_t* pName = (const wchar_t*)sqlite3_column_text16(stmt.Get(), 0);
		if (pName)
		{
			list.push_back(CString(pName));
		}
	}
	return list;
}

std::vector<CString> SqliteNoteRepository::ListArchived()
{
	std::vector<CString> list;
	if (!EnsureOpen()) return list;

	const wchar_t* sql = L"SELECT name FROM notes WHERE is_deleted = 0 AND is_archived = 1 ORDER BY archived_at DESC, rowid DESC;";
	SqliteStmtHelper stmt(m_db, sql);
	if (!stmt.IsValid()) return list;

	while (sqlite3_step(stmt.Get()) == SQLITE_ROW)
	{
		const wchar_t* pName = (const wchar_t*)sqlite3_column_text16(stmt.Get(), 0);
		if (pName)
		{
			list.push_back(CString(pName));
		}
	}
	return list;
}

std::vector<CString> SqliteNoteRepository::ListTrash()
{
	std::vector<CString> list;
	if (!EnsureOpen()) return list;

	const wchar_t* sql = L"SELECT name FROM notes WHERE is_deleted = 1 ORDER BY deleted_at DESC, rowid DESC;";
	SqliteStmtHelper stmt(m_db, sql);
	if (!stmt.IsValid()) return list;

	while (sqlite3_step(stmt.Get()) == SQLITE_ROW)
	{
		const wchar_t* pName = (const wchar_t*)sqlite3_column_text16(stmt.Get(), 0);
		if (pName)
		{
			list.push_back(CString(pName));
		}
	}
	return list;
}

bool SqliteNoteRepository::Load(const CString& name, Note& outNote)
{
	if (!EnsureOpen()) return false;

	const wchar_t* sqlNote =
		L"SELECT name, title, win_left, win_top, win_right, win_bottom, bgcolor, opacity, opacity_on, visible, topmost, is_deleted, deleted_at, is_archived, archived_at "
		L"FROM notes WHERE name = ?;";
	CachedStmtGuard stmtNote(GetOrCreateStmt(m_stmtLoadNote, sqlNote));
	if (!stmtNote.IsValid()) return false;

	sqlite3_bind_text16(stmtNote.Get(), 1, name.GetString(), -1, SQLITE_STATIC);
	if (sqlite3_step(stmtNote.Get()) != SQLITE_ROW)
	{
		return false;
	}

	const wchar_t* pName = (const wchar_t*)sqlite3_column_text16(stmtNote.Get(), 0);
	const wchar_t* pTitle = (const wchar_t*)sqlite3_column_text16(stmtNote.Get(), 1);
	int left = sqlite3_column_int(stmtNote.Get(), 2);
	int top = sqlite3_column_int(stmtNote.Get(), 3);
	int right = sqlite3_column_int(stmtNote.Get(), 4);
	int bottom = sqlite3_column_int(stmtNote.Get(), 5);
	const wchar_t* pBgColor = (const wchar_t*)sqlite3_column_text16(stmtNote.Get(), 6);
	int opacity = sqlite3_column_int(stmtNote.Get(), 7);
	int opacityOn = sqlite3_column_int(stmtNote.Get(), 8);
	int visible = sqlite3_column_int(stmtNote.Get(), 9);
	int topmost = sqlite3_column_int(stmtNote.Get(), 10);
	int isDeleted = sqlite3_column_int(stmtNote.Get(), 11);
	uint64_t deletedAt = (uint64_t)sqlite3_column_int64(stmtNote.Get(), 12);
	int isArchived = sqlite3_column_int(stmtNote.Get(), 13);
	uint64_t archivedAt = (uint64_t)sqlite3_column_int64(stmtNote.Get(), 14);

	outNote.name = pName ? CString(pName) : CString(name);
	outNote.title = pTitle ? CString(pTitle) : CString(_T(""));
	outNote.rect = CRect(left, top, right, bottom);
	outNote.bgColor = HexToColor(pBgColor ? CString(pBgColor) : CString(_T("#0d1117")));
	outNote.opacity = opacity;
	outNote.opacityEnabled = (opacityOn != 0);
	outNote.visible = (visible != 0);
	outNote.topMost = (topmost != 0);
	outNote.isDeleted = (isDeleted != 0);
	outNote.deletedAt = deletedAt;
	outNote.isArchived = (isArchived != 0);
	outNote.archivedAt = archivedAt;
	outNote.items.clear();

	const wchar_t* sqlItems =
		L"SELECT id, content, finished FROM note_items WHERE note_name = ? ORDER BY sort_order ASC, rowid ASC;";
	CachedStmtGuard stmtItems(GetOrCreateStmt(m_stmtLoadItems, sqlItems));
	if (stmtItems.IsValid())
	{
		sqlite3_bind_text16(stmtItems.Get(), 1, name.GetString(), -1, SQLITE_STATIC);
		while (sqlite3_step(stmtItems.Get()) == SQLITE_ROW)
		{
			uint64_t id = (uint64_t)sqlite3_column_int64(stmtItems.Get(), 0);
			const wchar_t* pContent = (const wchar_t*)sqlite3_column_text16(stmtItems.Get(), 1);
			bool finished = (sqlite3_column_int(stmtItems.Get(), 2) != 0);
			outNote.items.push_back(NoteItem(id, pContent ? CString(pContent) : CString(_T("")), finished));
		}
	}

	return true;
}

void SqliteNoteRepository::Save(const Note& note)
{
	if (!EnsureOpen()) return;

	sqlite3_exec(m_db, "BEGIN IMMEDIATE TRANSACTION;", nullptr, nullptr, nullptr);

	auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::system_clock::now().time_since_epoch()).count();

	const wchar_t* sqlNote =
		L"INSERT INTO notes (name, title, win_left, win_top, win_right, win_bottom, bgcolor, opacity, opacity_on, visible, topmost, is_deleted, deleted_at, is_archived, archived_at, updated_at) "
		L"VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?) "
		L"ON CONFLICT(name) DO UPDATE SET "
		L"title=excluded.title, win_left=excluded.win_left, win_top=excluded.win_top, win_right=excluded.win_right, win_bottom=excluded.win_bottom, "
		L"bgcolor=excluded.bgcolor, opacity=excluded.opacity, opacity_on=excluded.opacity_on, visible=excluded.visible, topmost=excluded.topmost, "
		L"is_deleted=excluded.is_deleted, deleted_at=excluded.deleted_at, is_archived=excluded.is_archived, archived_at=excluded.archived_at, updated_at=excluded.updated_at;";

	CachedStmtGuard stmtNote(GetOrCreateStmt(m_stmtSaveNote, sqlNote));
	if (stmtNote.IsValid())
	{
		CString hexColor = note.ToHexColor();
		sqlite3_bind_text16(stmtNote.Get(), 1, note.name.GetString(), -1, SQLITE_STATIC);
		sqlite3_bind_text16(stmtNote.Get(), 2, note.title.GetString(), -1, SQLITE_STATIC);
		sqlite3_bind_int(stmtNote.Get(), 3, note.rect.left);
		sqlite3_bind_int(stmtNote.Get(), 4, note.rect.top);
		sqlite3_bind_int(stmtNote.Get(), 5, note.rect.right);
		sqlite3_bind_int(stmtNote.Get(), 6, note.rect.bottom);
		sqlite3_bind_text16(stmtNote.Get(), 7, hexColor.GetString(), -1, SQLITE_STATIC);
		sqlite3_bind_int(stmtNote.Get(), 8, note.opacity);
		sqlite3_bind_int(stmtNote.Get(), 9, note.opacityEnabled ? 1 : 0);
		sqlite3_bind_int(stmtNote.Get(), 10, note.visible ? 1 : 0);
		sqlite3_bind_int(stmtNote.Get(), 11, note.topMost ? 1 : 0);
		sqlite3_bind_int(stmtNote.Get(), 12, note.isDeleted ? 1 : 0);
		sqlite3_bind_int64(stmtNote.Get(), 13, static_cast<sqlite3_int64>(note.deletedAt));
		sqlite3_bind_int(stmtNote.Get(), 14, note.isArchived ? 1 : 0);
		sqlite3_bind_int64(stmtNote.Get(), 15, static_cast<sqlite3_int64>(note.archivedAt));
		sqlite3_bind_int64(stmtNote.Get(), 16, static_cast<sqlite3_int64>(nowMs));

		sqlite3_step(stmtNote.Get());
	}

	const wchar_t* sqlDelItems = L"DELETE FROM note_items WHERE note_name = ?;";
	CachedStmtGuard stmtDel(GetOrCreateStmt(m_stmtDelItems, sqlDelItems));
	if (stmtDel.IsValid())
	{
		sqlite3_bind_text16(stmtDel.Get(), 1, note.name.GetString(), -1, SQLITE_STATIC);
		sqlite3_step(stmtDel.Get());
	}

	const wchar_t* sqlInsItem =
		L"INSERT INTO note_items (id, note_name, content, finished, sort_order) VALUES (?, ?, ?, ?, ?);";
	CachedStmtGuard stmtIns(GetOrCreateStmt(m_stmtInsItem, sqlInsItem));
	if (stmtIns.IsValid())
	{
		for (size_t i = 0; i < note.items.size(); ++i)
		{
			const auto& item = note.items[i];
			stmtIns.Reset();
			sqlite3_bind_int64(stmtIns.Get(), 1, (sqlite3_int64)item.uId);
			sqlite3_bind_text16(stmtIns.Get(), 2, note.name.GetString(), -1, SQLITE_STATIC);
			sqlite3_bind_text16(stmtIns.Get(), 3, item.sContent.GetString(), -1, SQLITE_STATIC);
			sqlite3_bind_int(stmtIns.Get(), 4, item.bFinished ? 1 : 0);
			sqlite3_bind_int(stmtIns.Get(), 5, static_cast<int>(i));
			sqlite3_step(stmtIns.Get());
		}
	}

	sqlite3_exec(m_db, "COMMIT;", nullptr, nullptr, nullptr);
}

bool SqliteNoteRepository::Rename(const CString& oldName, const CString& newName)
{
	if (!EnsureOpen()) return false;
	if (oldName == newName) return true;

	Note testNote;
	if (!Load(oldName, testNote))
	{
		return false;
	}
	if (Load(newName, testNote))
	{
		return false; // Target already exists
	}

	sqlite3_exec(m_db, "BEGIN IMMEDIATE TRANSACTION;", nullptr, nullptr, nullptr);

	const wchar_t* sqlNote = L"UPDATE notes SET name = ? WHERE name = ?;";
	SqliteStmtHelper stmtNote(m_db, sqlNote);
	if (!stmtNote.IsValid())
	{
		sqlite3_exec(m_db, "ROLLBACK;", nullptr, nullptr, nullptr);
		return false;
	}

	sqlite3_bind_text16(stmtNote.Get(), 1, newName.GetString(), -1, SQLITE_STATIC);
	sqlite3_bind_text16(stmtNote.Get(), 2, oldName.GetString(), -1, SQLITE_STATIC);
	int rc = sqlite3_step(stmtNote.Get());
	if (rc != SQLITE_DONE)
	{
		CLogApp::Error(_T("SqliteNoteRepository::Rename failed, rc=%d"), rc);
		sqlite3_exec(m_db, "ROLLBACK;", nullptr, nullptr, nullptr);
		return false;
	}

	sqlite3_exec(m_db, "COMMIT;", nullptr, nullptr, nullptr);
	return true;
}

void SqliteNoteRepository::Delete(const CString& name)
{
	SoftDelete(name);
}

bool SqliteNoteRepository::Archive(const CString& name)
{
	if (!EnsureOpen()) return false;

	auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::system_clock::now().time_since_epoch()).count();

	sqlite3_exec(m_db, "BEGIN IMMEDIATE TRANSACTION;", nullptr, nullptr, nullptr);

	const wchar_t* sql = L"UPDATE notes SET is_archived = 1, archived_at = ?, visible = 0, updated_at = ? WHERE name = ?;";
	CachedStmtGuard stmt(GetOrCreateStmt(m_stmtArchive, sql));
	if (!stmt.IsValid())
	{
		sqlite3_exec(m_db, "ROLLBACK;", nullptr, nullptr, nullptr);
		return false;
	}

	sqlite3_bind_int64(stmt.Get(), 1, static_cast<sqlite3_int64>(nowMs));
	sqlite3_bind_int64(stmt.Get(), 2, static_cast<sqlite3_int64>(nowMs));
	sqlite3_bind_text16(stmt.Get(), 3, name.GetString(), -1, SQLITE_STATIC);

	int rc = sqlite3_step(stmt.Get());
	if (rc != SQLITE_DONE)
	{
		CLogApp::Error(_T("SqliteNoteRepository::Archive failed for '%s', rc=%d"), name.GetString(), rc);
		sqlite3_exec(m_db, "ROLLBACK;", nullptr, nullptr, nullptr);
		return false;
	}

	sqlite3_exec(m_db, "COMMIT;", nullptr, nullptr, nullptr);
	return true;
}

bool SqliteNoteRepository::Unarchive(const CString& name)
{
	if (!EnsureOpen()) return false;

	auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::system_clock::now().time_since_epoch()).count();

	sqlite3_exec(m_db, "BEGIN IMMEDIATE TRANSACTION;", nullptr, nullptr, nullptr);

	const wchar_t* sql = L"UPDATE notes SET is_archived = 0, archived_at = 0, updated_at = ? WHERE name = ?;";
	CachedStmtGuard stmt(GetOrCreateStmt(m_stmtUnarchive, sql));
	if (!stmt.IsValid())
	{
		sqlite3_exec(m_db, "ROLLBACK;", nullptr, nullptr, nullptr);
		return false;
	}

	sqlite3_bind_int64(stmt.Get(), 1, static_cast<sqlite3_int64>(nowMs));
	sqlite3_bind_text16(stmt.Get(), 2, name.GetString(), -1, SQLITE_STATIC);

	int rc = sqlite3_step(stmt.Get());
	if (rc != SQLITE_DONE)
	{
		CLogApp::Error(_T("SqliteNoteRepository::Unarchive failed for '%s', rc=%d"), name.GetString(), rc);
		sqlite3_exec(m_db, "ROLLBACK;", nullptr, nullptr, nullptr);
		return false;
	}

	sqlite3_exec(m_db, "COMMIT;", nullptr, nullptr, nullptr);
	return true;
}

bool SqliteNoteRepository::SoftDelete(const CString& name)
{
	if (!EnsureOpen()) return false;

	auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::system_clock::now().time_since_epoch()).count();

	sqlite3_exec(m_db, "BEGIN IMMEDIATE TRANSACTION;", nullptr, nullptr, nullptr);

	const wchar_t* sql = L"UPDATE notes SET is_deleted = 1, deleted_at = ?, visible = 0, updated_at = ? WHERE name = ?;";
	CachedStmtGuard stmt(GetOrCreateStmt(m_stmtSoftDelete, sql));
	if (!stmt.IsValid())
	{
		sqlite3_exec(m_db, "ROLLBACK;", nullptr, nullptr, nullptr);
		return false;
	}

	sqlite3_bind_int64(stmt.Get(), 1, static_cast<sqlite3_int64>(nowMs));
	sqlite3_bind_int64(stmt.Get(), 2, static_cast<sqlite3_int64>(nowMs));
	sqlite3_bind_text16(stmt.Get(), 3, name.GetString(), -1, SQLITE_STATIC);

	int rc = sqlite3_step(stmt.Get());
	if (rc != SQLITE_DONE)
	{
		CLogApp::Error(_T("SqliteNoteRepository::SoftDelete failed for '%s', rc=%d"), name.GetString(), rc);
		sqlite3_exec(m_db, "ROLLBACK;", nullptr, nullptr, nullptr);
		return false;
	}

	sqlite3_exec(m_db, "COMMIT;", nullptr, nullptr, nullptr);
	return true;
}

bool SqliteNoteRepository::Restore(const CString& name)
{
	if (!EnsureOpen()) return false;

	auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::system_clock::now().time_since_epoch()).count();

	sqlite3_exec(m_db, "BEGIN IMMEDIATE TRANSACTION;", nullptr, nullptr, nullptr);

	const wchar_t* sql = L"UPDATE notes SET is_deleted = 0, deleted_at = 0, updated_at = ? WHERE name = ?;";
	CachedStmtGuard stmt(GetOrCreateStmt(m_stmtRestore, sql));
	if (!stmt.IsValid())
	{
		sqlite3_exec(m_db, "ROLLBACK;", nullptr, nullptr, nullptr);
		return false;
	}

	sqlite3_bind_int64(stmt.Get(), 1, static_cast<sqlite3_int64>(nowMs));
	sqlite3_bind_text16(stmt.Get(), 2, name.GetString(), -1, SQLITE_STATIC);

	int rc = sqlite3_step(stmt.Get());
	if (rc != SQLITE_DONE)
	{
		CLogApp::Error(_T("SqliteNoteRepository::Restore failed for '%s', rc=%d"), name.GetString(), rc);
		sqlite3_exec(m_db, "ROLLBACK;", nullptr, nullptr, nullptr);
		return false;
	}

	sqlite3_exec(m_db, "COMMIT;", nullptr, nullptr, nullptr);
	return true;
}

bool SqliteNoteRepository::PermanentDelete(const CString& name)
{
	if (!EnsureOpen()) return false;

	sqlite3_exec(m_db, "BEGIN IMMEDIATE TRANSACTION;", nullptr, nullptr, nullptr);

	const wchar_t* sqlItems = L"DELETE FROM note_items WHERE note_name = ?;";
	CachedStmtGuard stmtItems(GetOrCreateStmt(m_stmtPermDelItems, sqlItems));
	if (stmtItems.IsValid())
	{
		sqlite3_bind_text16(stmtItems.Get(), 1, name.GetString(), -1, SQLITE_STATIC);
		sqlite3_step(stmtItems.Get());
	}

	const wchar_t* sqlNote = L"DELETE FROM notes WHERE name = ?;";
	CachedStmtGuard stmtNote(GetOrCreateStmt(m_stmtPermDelNote, sqlNote));
	if (stmtNote.IsValid())
	{
		sqlite3_bind_text16(stmtNote.Get(), 1, name.GetString(), -1, SQLITE_STATIC);
		sqlite3_step(stmtNote.Get());
	}

	sqlite3_exec(m_db, "COMMIT;", nullptr, nullptr, nullptr);
	return true;
}

bool SqliteNoteRepository::ClearTrash()
{
	if (!EnsureOpen()) return false;

	sqlite3_exec(m_db, "BEGIN IMMEDIATE TRANSACTION;", nullptr, nullptr, nullptr);

	const wchar_t* sqlItems = L"DELETE FROM note_items WHERE note_name IN (SELECT name FROM notes WHERE is_deleted = 1);";
	SqliteStmtHelper stmtItems(m_db, sqlItems);
	if (stmtItems.IsValid())
	{
		sqlite3_step(stmtItems.Get());
	}

	const wchar_t* sqlNotes = L"DELETE FROM notes WHERE is_deleted = 1;";
	SqliteStmtHelper stmtNotes(m_db, sqlNotes);
	if (stmtNotes.IsValid())
	{
		sqlite3_step(stmtNotes.Get());
	}

	sqlite3_exec(m_db, "COMMIT;", nullptr, nullptr, nullptr);
	return true;
}

// ---------------------------------------------------------------------------
// LoadBulkByFilter — core N+1 eliminator
//
// Replaces the old pattern:
//   ListActive() → 1 query for N names
//   Load(name)   → 2 queries per note  →  1 + 2×N total
//
// New pattern (always exactly 2 queries regardless of N):
//   Query 1: SELECT all notes metadata matching the filter
//   Query 2: SELECT all items for those notes via correlated IN subquery
//   C++: associate items with notes using an unordered_map (O(1) lookup)
//
// @param notesFilter  raw SQL fragment for WHERE clause on `notes` table
//                     (hardcoded callers only — no user input)
// @param notesOrder   raw SQL fragment for ORDER BY clause on `notes`
// ---------------------------------------------------------------------------
std::vector<Note> SqliteNoteRepository::LoadBulkByFilter(
	const wchar_t* notesFilter, const wchar_t* notesOrder)
{
	std::vector<Note> notes;
	if (!EnsureOpen()) return notes;

	// Build the two SQL strings from the shared filter fragment
	std::wstring sqlNotesStr =
		L"SELECT name, title, win_left, win_top, win_right, win_bottom, "
		L"bgcolor, opacity, opacity_on, visible, topmost, "
		L"is_deleted, deleted_at, is_archived, archived_at "
		L"FROM notes WHERE ";
	sqlNotesStr += notesFilter;
	sqlNotesStr += L" ORDER BY ";
	sqlNotesStr += notesOrder;
	sqlNotesStr += L";";

	// Items query reuses the same filter so both queries are always in sync
	std::wstring sqlItemsStr =
		L"SELECT ni.note_name, ni.id, ni.content, ni.finished "
		L"FROM note_items ni "
		L"WHERE ni.note_name IN (SELECT name FROM notes WHERE ";
	sqlItemsStr += notesFilter;
	sqlItemsStr += L") ORDER BY ni.note_name, ni.sort_order ASC, ni.rowid ASC;";

	// --- Query 1: load all notes metadata (preserves declared ordering) ---
	SqliteStmtHelper stmtNotes(m_db, sqlNotesStr.c_str());
	if (!stmtNotes.IsValid()) return notes;

	// nameToIndex: wstring key → index into `notes` vector for O(1) item association
	std::unordered_map<std::wstring, size_t> nameToIndex;

	while (sqlite3_step(stmtNotes.Get()) == SQLITE_ROW)
	{
		const wchar_t* pName  = (const wchar_t*)sqlite3_column_text16(stmtNotes.Get(), 0);
		const wchar_t* pTitle = (const wchar_t*)sqlite3_column_text16(stmtNotes.Get(), 1);
		int nLeft    = sqlite3_column_int(stmtNotes.Get(), 2);
		int nTop     = sqlite3_column_int(stmtNotes.Get(), 3);
		int nRight   = sqlite3_column_int(stmtNotes.Get(), 4);
		int nBottom  = sqlite3_column_int(stmtNotes.Get(), 5);
		const wchar_t* pColor = (const wchar_t*)sqlite3_column_text16(stmtNotes.Get(), 6);
		int opacity    = sqlite3_column_int(stmtNotes.Get(), 7);
		int opacityOn  = sqlite3_column_int(stmtNotes.Get(), 8);
		int visible    = sqlite3_column_int(stmtNotes.Get(), 9);
		int topmost    = sqlite3_column_int(stmtNotes.Get(), 10);
		int isDeleted  = sqlite3_column_int(stmtNotes.Get(), 11);
		uint64_t deletedAt  = static_cast<uint64_t>(sqlite3_column_int64(stmtNotes.Get(), 12));
		int isArchived = sqlite3_column_int(stmtNotes.Get(), 13);
		uint64_t archivedAt = static_cast<uint64_t>(sqlite3_column_int64(stmtNotes.Get(), 14));

		Note note;
		note.name         = pName  ? CString(pName)  : CString(_T(""));
		note.title        = pTitle ? CString(pTitle) : CString(_T(""));
		note.rect         = CRect(nLeft, nTop, nRight, nBottom);
		note.bgColor      = HexToColor(pColor ? CString(pColor) : CString(_T("#0d1117")));
		note.opacity      = opacity;
		note.opacityEnabled = (opacityOn != 0);
		note.visible      = (visible != 0);
		note.topMost      = (topmost != 0);
		note.isDeleted    = (isDeleted != 0);
		note.deletedAt    = deletedAt;
		note.isArchived   = (isArchived != 0);
		note.archivedAt   = archivedAt;

		if (pName) nameToIndex[pName] = notes.size();
		notes.push_back(std::move(note));
	}

	// --- Query 2: load all items for these notes in one round-trip ---
	SqliteStmtHelper stmtItems(m_db, sqlItemsStr.c_str());
	if (stmtItems.IsValid())
	{
		while (sqlite3_step(stmtItems.Get()) == SQLITE_ROW)
		{
			const wchar_t* pNoteName = (const wchar_t*)sqlite3_column_text16(stmtItems.Get(), 0);
			if (!pNoteName) continue;

			auto it = nameToIndex.find(pNoteName);
			if (it == nameToIndex.end()) continue;

			uint64_t id       = static_cast<uint64_t>(sqlite3_column_int64(stmtItems.Get(), 1));
			const wchar_t* pContent = (const wchar_t*)sqlite3_column_text16(stmtItems.Get(), 2);
			bool finished     = (sqlite3_column_int(stmtItems.Get(), 3) != 0);

			notes[it->second].items.emplace_back(
				id,
				pContent ? CString(pContent) : CString(_T("")),
				finished
			);
		}
	}

	return notes;
}

// Public bulk-load methods — each is now a single readable delegation
std::vector<Note> SqliteNoteRepository::LoadAllNotes()
{
	return LoadBulkByFilter(
		L"is_deleted=0 AND is_archived=0",
		L"sort_order ASC, rowid ASC"
	);
}

std::vector<Note> SqliteNoteRepository::LoadArchivedNotes()
{
	return LoadBulkByFilter(
		L"is_archived=1 AND is_deleted=0",
		L"archived_at DESC, rowid DESC"
	);
}

std::vector<Note> SqliteNoteRepository::LoadTrashNotes()
{
	return LoadBulkByFilter(
		L"is_deleted=1",
		L"deleted_at DESC, rowid DESC"
	);
}

std::vector<Note> SqliteNoteRepository::SearchByContent(const CString& keyword)
{
	if (!EnsureOpen() || keyword.IsEmpty()) return {};

	CString likePattern = _T("%") + keyword + _T("%");

	// Query 1: matching notes metadata (no N+1: items not loaded here)
	const wchar_t* sqlNotes =
		L"SELECT n.name, n.title, n.win_left, n.win_top, n.win_right, n.win_bottom, "
		L"n.bgcolor, n.opacity, n.opacity_on, n.visible, n.topmost, "
		L"n.is_deleted, n.deleted_at, n.is_archived, n.archived_at "
		L"FROM notes n "
		L"WHERE n.is_deleted = 0 AND n.name IN ("
		L"  SELECT DISTINCT n2.name FROM notes n2 "
		L"  LEFT JOIN note_items ni ON n2.name = ni.note_name "
		L"  WHERE n2.is_deleted = 0 AND (n2.title LIKE ? OR ni.content LIKE ?)"
		L") ORDER BY n.sort_order ASC, n.rowid ASC;";

	// Query 2: all items for the matching notes in one round-trip
	const wchar_t* sqlItems =
		L"SELECT ni.note_name, ni.id, ni.content, ni.finished "
		L"FROM note_items ni "
		L"WHERE ni.note_name IN ("
		L"  SELECT DISTINCT n.name FROM notes n "
		L"  LEFT JOIN note_items ni2 ON n.name = ni2.note_name "
		L"  WHERE n.is_deleted = 0 AND (n.title LIKE ? OR ni2.content LIKE ?)"
		L") ORDER BY ni.note_name, ni.sort_order ASC, ni.rowid ASC;";

	SqliteStmtHelper stmtNotes(m_db, sqlNotes);
	if (!stmtNotes.IsValid()) return {};

	sqlite3_bind_text16(stmtNotes.Get(), 1, likePattern.GetString(), -1, SQLITE_STATIC);
	sqlite3_bind_text16(stmtNotes.Get(), 2, likePattern.GetString(), -1, SQLITE_STATIC);

	std::vector<Note> results;
	std::unordered_map<std::wstring, size_t> nameToIndex;

	while (sqlite3_step(stmtNotes.Get()) == SQLITE_ROW)
	{
		const wchar_t* pName  = (const wchar_t*)sqlite3_column_text16(stmtNotes.Get(), 0);
		const wchar_t* pTitle = (const wchar_t*)sqlite3_column_text16(stmtNotes.Get(), 1);
		int nLeft    = sqlite3_column_int(stmtNotes.Get(), 2);
		int nTop     = sqlite3_column_int(stmtNotes.Get(), 3);
		int nRight   = sqlite3_column_int(stmtNotes.Get(), 4);
		int nBottom  = sqlite3_column_int(stmtNotes.Get(), 5);
		const wchar_t* pColor = (const wchar_t*)sqlite3_column_text16(stmtNotes.Get(), 6);
		int opacity    = sqlite3_column_int(stmtNotes.Get(), 7);
		int opacityOn  = sqlite3_column_int(stmtNotes.Get(), 8);
		int visible    = sqlite3_column_int(stmtNotes.Get(), 9);
		int topmost    = sqlite3_column_int(stmtNotes.Get(), 10);
		int isDeleted  = sqlite3_column_int(stmtNotes.Get(), 11);
		uint64_t deletedAt  = static_cast<uint64_t>(sqlite3_column_int64(stmtNotes.Get(), 12));
		int isArchived = sqlite3_column_int(stmtNotes.Get(), 13);
		uint64_t archivedAt = static_cast<uint64_t>(sqlite3_column_int64(stmtNotes.Get(), 14));

		Note note;
		note.name         = pName  ? CString(pName)  : CString(_T(""));
		note.title        = pTitle ? CString(pTitle) : CString(_T(""));
		note.rect         = CRect(nLeft, nTop, nRight, nBottom);
		note.bgColor      = HexToColor(pColor ? CString(pColor) : CString(_T("#0d1117")));
		note.opacity      = opacity;
		note.opacityEnabled = (opacityOn != 0);
		note.visible      = (visible != 0);
		note.topMost      = (topmost != 0);
		note.isDeleted    = (isDeleted != 0);
		note.deletedAt    = deletedAt;
		note.isArchived   = (isArchived != 0);
		note.archivedAt   = archivedAt;

		if (pName) nameToIndex[pName] = results.size();
		results.push_back(std::move(note));
	}

	if (!results.empty())
	{
		SqliteStmtHelper stmtItems(m_db, sqlItems);
		if (stmtItems.IsValid())
		{
			sqlite3_bind_text16(stmtItems.Get(), 1, likePattern.GetString(), -1, SQLITE_STATIC);
			sqlite3_bind_text16(stmtItems.Get(), 2, likePattern.GetString(), -1, SQLITE_STATIC);

			while (sqlite3_step(stmtItems.Get()) == SQLITE_ROW)
			{
				const wchar_t* pNoteName = (const wchar_t*)sqlite3_column_text16(stmtItems.Get(), 0);
				if (!pNoteName) continue;
				auto it = nameToIndex.find(pNoteName);
				if (it == nameToIndex.end()) continue;
				uint64_t id = static_cast<uint64_t>(sqlite3_column_int64(stmtItems.Get(), 1));
				const wchar_t* pContent = (const wchar_t*)sqlite3_column_text16(stmtItems.Get(), 2);
				bool finished = (sqlite3_column_int(stmtItems.Get(), 3) != 0);
				results[it->second].items.emplace_back(
					id,
					pContent ? CString(pContent) : CString(_T("")),
					finished
				);
			}
		}
	}

	return results;
}

bool SqliteNoteRepository::ExportDatabase(const CString& targetBackupPath)
{
	if (targetBackupPath.IsEmpty()) return false;
	if (!EnsureOpen()) return false;

	// 1. Flush all WAL data into the main database file
	Checkpoint();

	// 2. Ensure target directory exists
	CString targetDir = Easy::Path::GetDirectory(targetBackupPath);
	if (!targetDir.IsEmpty() && !Easy::Path::Exists(targetDir))
	{
		Easy::Path::Create(targetDir);
	}

	// 3. Perform SQLite Online Backup API for 100% ACID snapshot
	sqlite3* pDest = nullptr;
	int rc = sqlite3_open16(targetBackupPath.GetString(), &pDest);
	if (rc != SQLITE_OK || !pDest)
	{
		CLogApp::Error(_T("SqliteNoteRepository::ExportDatabase: Failed to open target '%s', rc=%d"), targetBackupPath.GetString(), rc);
		if (pDest) sqlite3_close(pDest);
		return false;
	}

	sqlite3_backup* pBackup = sqlite3_backup_init(pDest, "main", m_db, "main");
	if (!pBackup)
	{
		CLogApp::Error(_T("SqliteNoteRepository::ExportDatabase: sqlite3_backup_init failed, err=%S"), sqlite3_errmsg(pDest));
		sqlite3_close(pDest);
		return false;
	}

	rc = sqlite3_backup_step(pBackup, -1);
	sqlite3_backup_finish(pBackup);

	if (rc != SQLITE_DONE)
	{
		CLogApp::Error(_T("SqliteNoteRepository::ExportDatabase: sqlite3_backup_step failed, rc=%d"), rc);
		sqlite3_close(pDest);
		return false;
	}

	sqlite3_exec(pDest, "PRAGMA vacuum;", nullptr, nullptr, nullptr);
	sqlite3_close(pDest);
	return true;
}

bool SqliteNoteRepository::ImportDatabase(const CString& sourceBackupPath)
{
	if (sourceBackupPath.IsEmpty() || !Easy::Path::Exists(sourceBackupPath))
	{
		CLogApp::Error(_T("SqliteNoteRepository::ImportDatabase: Source file does not exist '%s'"), sourceBackupPath.GetString());
		return false;
	}

	// 1. Verify that sourceBackupPath is a valid SQLite 3 database and has a valid notes table schema
	sqlite3* pSource = nullptr;
	int rc = sqlite3_open16(sourceBackupPath.GetString(), &pSource);
	if (rc != SQLITE_OK || !pSource)
	{
		CLogApp::Error(_T("SqliteNoteRepository::ImportDatabase: Source is not a valid SQLite database, rc=%d"), rc);
		if (pSource) sqlite3_close(pSource);
		return false;
	}

	sqlite3_stmt* stmt = nullptr;
	rc = sqlite3_prepare16_v2(pSource, L"SELECT count(*) FROM sqlite_master WHERE type='table' AND name='notes';", -1, &stmt, nullptr);
	bool hasNotesTable = false;
	if (rc == SQLITE_OK && stmt)
	{
		if (sqlite3_step(stmt) == SQLITE_ROW)
		{
			hasNotesTable = (sqlite3_column_int(stmt, 0) > 0);
		}
		sqlite3_finalize(stmt);
	}
	sqlite3_close(pSource);

	if (!hasNotesTable)
	{
		CLogApp::Error(_T("SqliteNoteRepository::ImportDatabase: Source DB missing 'notes' table, rejecting import"));
		return false;
	}

	// 2. Safely close current connection & finalized cached statements
	CString dbPath = EnsureDbPath();
	Close();

	// 3. Remove existing WAL / SHM files if any
	CString walPath = dbPath + _T("-wal");
	CString shmPath = dbPath + _T("-shm");
	if (Easy::Path::Exists(walPath)) ::DeleteFile(walPath.GetString());
	if (Easy::Path::Exists(shmPath)) ::DeleteFile(shmPath.GetString());

	// 4. Overwrite target DB file with the source backup
	BOOL copied = ::CopyFile(sourceBackupPath.GetString(), dbPath.GetString(), FALSE);
	if (!copied)
	{
		CLogApp::Error(_T("SqliteNoteRepository::ImportDatabase: CopyFile failed, err=%d"), ::GetLastError());
		EnsureOpen();
		return false;
	}

	// 5. Re-open connection and ensure schema & migrations
	return EnsureOpen();
}

