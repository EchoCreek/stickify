// ------------------------------------------------------------------
// Copyright (c) Sticky Notes Refactoring Team (2026)
// Part of Sticky Notes Refactoring Project
// Licensed under the Apache License, Version 2.0
// ------------------------------------------------------------------
#include "stdafx.h"
#include "JsonNoteRepository.h"
#include "host/NoteDto.h"
#include "ref/Path.h"
#include "ref/XFile.h"
#include "ref/Log.h"
#include "ref/Cvt.h"

JsonNoteRepository::JsonNoteRepository(const CString& storageDir)
	: m_storageDir(storageDir)
{
}

void JsonNoteRepository::SetStorageDir(const CString& dir)
{
	m_storageDir = dir;
}

CString JsonNoteRepository::GetStorageDir() const
{
	return m_storageDir;
}

CString JsonNoteRepository::EnsureDirectory() const
{
	CString dir = m_storageDir;
	if (dir.IsEmpty())
	{
		dir = Easy::Path::GetCurDirectory(_T("notes\\"));
	}
	if (!Easy::Path::Exists(dir))
	{
		Easy::Path::Create(dir);
	}
	return dir;
}

std::vector<CString> JsonNoteRepository::ListAll()
{
	return ListActive();
}

std::vector<CString> JsonNoteRepository::ListActive()
{
	std::vector<CString> lstName;
	std::vector<CString> lstConfig = Easy::Path::GetFileList(EnsureDirectory(), _T("*.json"));
	for (size_t i = 0; i < lstConfig.size(); i++)
	{
		CString sNote = Easy::Path::GetFileName(lstConfig[i]);
		if (sNote.GetLength() > 5 && sNote.Right(5).CompareNoCase(_T(".json")) == 0)
		{
			CString noteName = sNote.Mid(0, sNote.GetLength() - 5);
			Note note;
			if (Load(noteName, note))
			{
				if (!note.isDeleted && !note.isArchived)
				{
					lstName.push_back(noteName);
				}
			}
			else
			{
				lstName.push_back(noteName);
			}
		}
	}
	return lstName;
}

std::vector<CString> JsonNoteRepository::ListArchived()
{
	std::vector<CString> lstName;
	std::vector<CString> lstConfig = Easy::Path::GetFileList(EnsureDirectory(), _T("*.json"));
	for (size_t i = 0; i < lstConfig.size(); i++)
	{
		CString sNote = Easy::Path::GetFileName(lstConfig[i]);
		if (sNote.GetLength() > 5 && sNote.Right(5).CompareNoCase(_T(".json")) == 0)
		{
			CString noteName = sNote.Mid(0, sNote.GetLength() - 5);
			Note note;
			if (Load(noteName, note) && !note.isDeleted && note.isArchived)
			{
				lstName.push_back(noteName);
			}
		}
	}
	return lstName;
}

std::vector<CString> JsonNoteRepository::ListTrash()
{
	std::vector<CString> lstName;
	std::vector<CString> lstConfig = Easy::Path::GetFileList(EnsureDirectory(), _T("*.json"));
	for (size_t i = 0; i < lstConfig.size(); i++)
	{
		CString sNote = Easy::Path::GetFileName(lstConfig[i]);
		if (sNote.GetLength() > 5 && sNote.Right(5).CompareNoCase(_T(".json")) == 0)
		{
			CString noteName = sNote.Mid(0, sNote.GetLength() - 5);
			Note note;
			if (Load(noteName, note) && note.isDeleted)
			{
				lstName.push_back(noteName);
			}
		}
	}
	return lstName;
}

bool JsonNoteRepository::Load(const CString& name, Note& outNote)
{
	CString sConfigFile = Easy::Path::Resolve(EnsureDirectory(), name + _T(".json"));

	outNote.name = name;
	if (!Easy::Path::Exists(sConfigFile))
	{
		CLogApp::Warn(_T("Config File Not Exists: ") + sConfigFile);
		return false;
	}

	CString sJson;
	if (!Easy::XFile::ReadFile(sConfigFile, sJson))
	{
		CLogApp::Error(_T("Read Config File Failed: ") + sConfigFile + _T(", ErrorCode: %d"), GetLastError());
		return false;
	}

	rapidjson::GenericDocument<rapidjson::UTF16<TCHAR>> doc;
	doc.Parse(sJson.GetString());
	if (doc.HasParseError())
	{
		CLogApp::Error(_T("Parse Config File Failed: ") + sConfigFile + _T(", ErrorCode: %d"), doc.GetParseError());
		return false;
	}

	outNote = NoteDto::FromJson(name, doc);
	return true;
}

void JsonNoteRepository::Save(const Note& note)
{
	CString sConfigFile = Easy::Path::Resolve(EnsureDirectory(), note.name + _T(".json"));

	CLogApp::Info(_T("JsonNoteRepository::Save: ") + sConfigFile);
	RJDoc doc;
	auto& allocator = doc.GetAllocator();
	RJValue vData = NoteDto::Detail(note, allocator);

	CString sJson = NoteDto::Stringify(vData);
	if (!Easy::XFile::WriteFile(sConfigFile, sJson))
	{
		CLogApp::Error(_T("Write Config File Failed: ") + sConfigFile + _T(", ErrorCode: %d"), GetLastError());
	}
}

bool JsonNoteRepository::Rename(const CString& oldName, const CString& newName)
{
	CString sOldConfigFile = Easy::Path::Resolve(EnsureDirectory(), oldName + _T(".json"));
	CString sNewConfigFile = Easy::Path::Resolve(EnsureDirectory(), newName + _T(".json"));
	if (Easy::Path::Exists(sOldConfigFile) && !Easy::Path::Exists(sNewConfigFile))
	{
		if (CopyFile(sOldConfigFile, sNewConfigFile, FALSE))
		{
			DeleteFile(sOldConfigFile);
			return true;
		}
	}
	return false;
}

void JsonNoteRepository::Delete(const CString& name)
{
	PermanentDelete(name);
}

bool JsonNoteRepository::Archive(const CString& name)
{
	Note note;
	if (!Load(name, note)) return false;
	note.isArchived = true;
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	ULARGE_INTEGER uli;
	uli.LowPart = ft.dwLowDateTime;
	uli.HighPart = ft.dwHighDateTime;
	note.archivedAt = (uli.QuadPart - 116444736000000000ULL) / 10000ULL;
	note.visible = false;
	Save(note);
	return true;
}

bool JsonNoteRepository::Unarchive(const CString& name)
{
	Note note;
	if (!Load(name, note)) return false;
	note.isArchived = false;
	note.archivedAt = 0;
	Save(note);
	return true;
}

bool JsonNoteRepository::SoftDelete(const CString& name)
{
	Note note;
	if (!Load(name, note)) return false;
	note.isDeleted = true;
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	ULARGE_INTEGER uli;
	uli.LowPart = ft.dwLowDateTime;
	uli.HighPart = ft.dwHighDateTime;
	note.deletedAt = (uli.QuadPart - 116444736000000000ULL) / 10000ULL;
	note.visible = false;
	Save(note);
	return true;
}

bool JsonNoteRepository::Restore(const CString& name)
{
	Note note;
	if (!Load(name, note)) return false;
	note.isDeleted = false;
	note.deletedAt = 0;
	Save(note);
	return true;
}

bool JsonNoteRepository::PermanentDelete(const CString& name)
{
	CString sConfigFile = Easy::Path::Resolve(EnsureDirectory(), name + _T(".json"));
	if (Easy::Path::Exists(sConfigFile))
	{
		return DeleteFile(sConfigFile) != FALSE;
	}
	return false;
}

bool JsonNoteRepository::ClearTrash()
{
	std::vector<CString> trash = ListTrash();
	for (const auto& name : trash)
	{
		PermanentDelete(name);
	}
	return true;
}
