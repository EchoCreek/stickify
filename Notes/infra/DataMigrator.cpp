// ------------------------------------------------------------------
// Copyright (c) Sticky Notes Refactoring Team (2026)
// Part of Sticky Notes Refactoring Project
// Licensed under the Apache License, Version 2.0
// ------------------------------------------------------------------
#include "stdafx.h"
#include "infra/DataMigrator.h"
#include "infra/JsonNoteRepository.h"
#include "ref/Path.h"
#include "ref/Ini.h"
#include "ref/Cvt.h"
#include "core/domain/Note.h"
#include "core/domain/NoteItem.h"
#include <vector>

bool DataMigrator::MigrateIfNeeded(INoteRepository& newRepo, bool autoConfirm)
{
	CString notesDir = _T("");
	JsonNoteRepository* pJsonRepo = dynamic_cast<JsonNoteRepository*>(&newRepo);
	if (pJsonRepo != nullptr)
	{
		notesDir = pJsonRepo->GetStorageDir();
	}
	if (notesDir.IsEmpty())
	{
		notesDir = Easy::Path::GetCurDirectory(_T("notes\\"));
	}
	if (!Easy::Path::Exists(notesDir))
	{
		Easy::Path::Create(notesDir);
		return false;
	}

	std::vector<CString> iniFiles = Easy::Path::GetFileList(notesDir, _T("*.ini"));
	if (iniFiles.empty())
	{
		return false;
	}

	std::vector<CString> noteNames;
	for (const auto& file : iniFiles)
	{
		CString sNote = Easy::Path::GetFileName(file);
		if (sNote.GetLength() > 4 && sNote.Right(4).CompareNoCase(_T(".ini")) == 0)
		{
			noteNames.push_back(sNote.Mid(0, sNote.GetLength() - 4));
		}
	}

	if (noteNames.empty())
	{
		return false;
	}

	int ret = autoConfirm ? IDYES : ::MessageBox(NULL,
		_T("发现旧版便签数据，是否自动导入？\r\n若选择否，则会删除旧版便签数据！"),
		_T("Notes"),
		MB_ICONQUESTION | MB_YESNO);

	if (ret == IDYES)
	{
		for (const auto& name : noteNames)
		{
			CString sConfigFile = Easy::Path::Resolve(notesDir, name + _T(".ini"));
			if (!Easy::Path::Exists(sConfigFile))
			{
				continue;
			}

			Easy::Ini ini(sConfigFile);
			Note note(name);
			int nCount = 0;
			CString sRect;
			ini.Read(_T("Group"), _T("Count"), nCount);
			ini.Read(_T("Group"), _T("Name"), note.name);
			ini.Read(_T("Group"), _T("Rect"), sRect);
			ini.Read(_T("Group"), _T("Opacity"), note.opacity);
			ini.Read(_T("Group"), _T("OpacityAble"), note.opacityEnabled);
			ini.Read(_T("Group"), _T("Visible"), note.visible);
			ini.Read(_T("Group"), _T("TopMost"), note.topMost);
			ini.Read(_T("Group"), _T("BgColor"), note.bgColor);
			ini.Read(_T("Group"), _T("Title"), note.title);

			std::vector<CString> lstRect = Easy::Cvt::SplitString(sRect, _T(","));
			if (lstRect.size() >= 4)
			{
				note.rect = CRect(_ttoi(lstRect[0]), _ttoi(lstRect[1]), _ttoi(lstRect[2]), _ttoi(lstRect[3]));
			}

			for (int j = 0; j < nCount; ++j)
			{
				NoteItem item;
				CString sKey = _T("Note") + Easy::Cvt::ToString(j);
				ULONG legacyId = 0;
				ini.Read(sKey, _T("Id"), legacyId);
				item.uId = legacyId;
				ini.Read(sKey, _T("Content"), item.sContent);
				ini.Read(sKey, _T("Finished"), item.bFinished);
				item.sContent.Replace(_T("{{\\n}}"), _T("\n"));
				note.items.push_back(item);
			}

			newRepo.Save(note);
		}
	}

	for (const auto& file : iniFiles)
	{
		::DeleteFile(file);
	}

	return true;
}
