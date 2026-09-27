// ------------------------------------------------------------------
// Copyright (c) Sticky Notes Refactoring Team (2026)
// Part of Sticky Notes Refactoring Project
// Licensed under the Apache License, Version 2.0
// ------------------------------------------------------------------
#include "stdafx.h"
#include "infra/SqliteMigrator.h"
#include "infra/JsonNoteRepository.h"
#include "ref/Path.h"
#include "ref/Log.h"
#include <vector>

bool SqliteMigrator::MigrateIfNeeded(SqliteNoteRepository& repo, const CString& notesDirParam, bool autoConfirm)
{
	CString notesDir = notesDirParam;
	if (notesDir.IsEmpty())
	{
		CString dbPath = repo.GetDbPath();
		if (!dbPath.IsEmpty())
		{
			notesDir = Easy::Path::GetDirectory(dbPath);
		}
		else
		{
			notesDir = Easy::Path::GetCurDirectory(_T("notes\\"));
		}
	}

	if (!Easy::Path::Exists(notesDir))
	{
		Easy::Path::Create(notesDir);
		return false;
	}

	std::vector<CString> jsonFiles = Easy::Path::GetFileList(notesDir, _T("*.json"));
	if (jsonFiles.empty())
	{
		return false;
	}

	std::vector<CString> existingSqliteNotes = repo.ListAll();
	if (!existingSqliteNotes.empty())
	{
		// SQLite already contains data, skip migration
		return false;
	}

	std::vector<CString> noteNames;
	for (const auto& file : jsonFiles)
	{
		CString sNote = Easy::Path::GetFileName(file);
		if (sNote.GetLength() > 5 && sNote.Right(5).CompareNoCase(_T(".json")) == 0)
		{
			noteNames.push_back(sNote.Mid(0, sNote.GetLength() - 5));
		}
	}

	if (noteNames.empty())
	{
		return false;
	}

	int ret = autoConfirm ? IDYES : ::MessageBox(NULL,
		_T("发现旧版 JSON 便签数据，是否自动迁移到 SQLite 数据库？\r\n迁移后旧 JSON 文件将备份至 json_backup 目录。"),
		_T("Notes"),
		MB_ICONQUESTION | MB_YESNO);

	if (ret == IDYES)
	{
		JsonNoteRepository jsonRepo(notesDir);
		int migratedCount = 0;
		for (const auto& name : noteNames)
		{
			Note note;
			if (jsonRepo.Load(name, note))
			{
				repo.Save(note);
				migratedCount++;
			}
		}

		CLogApp::Write(_T("SqliteMigrator: Successfully migrated %d notes from JSON to SQLite"), migratedCount);

		CString backupDir = Easy::Path::Resolve(notesDir, _T("json_backup\\"));
		if (!Easy::Path::Exists(backupDir))
		{
			Easy::Path::Create(backupDir);
		}

		for (const auto& file : jsonFiles)
		{
			CString fileName = Easy::Path::GetFileName(file);
			CString targetFile = Easy::Path::Resolve(backupDir, fileName);
			if (CopyFile(file, targetFile, FALSE))
			{
				DeleteFile(file);
			}
		}

		return true;
	}

	return false;
}
