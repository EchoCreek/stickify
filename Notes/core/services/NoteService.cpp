// ------------------------------------------------------------------
// Copyright (c) Sticky Notes Refactoring Team (2026)
// Part of Sticky Notes Refactoring Project
// Licensed under the Apache License, Version 2.0
// ------------------------------------------------------------------
#include "stdafx.h"
#include "core/services/NoteService.h"
#include <chrono>
#include <fstream>
#include <string>

NoteService::NoteService(INoteRepository& repo)
	: m_repo(repo)
{
}

bool NoteService::AddItem(Note& note, const NoteItem& item)
{
	NoteItem toAdd = item;
	if (toAdd.uId == 0) {
		auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch()).count();
		toAdd.uId = static_cast<uint64_t>(nowMs);
	}

	for (auto& existing : note.items) {
		if (existing.uId == toAdd.uId) {
			existing = toAdd;
			m_repo.Save(note);
			return true;
		}
	}

	note.items.push_back(toAdd);
	m_repo.Save(note);
	return true;
}

bool NoteService::UpdateItem(Note& note, const NoteItem& item)
{
	for (auto& existing : note.items) {
		if (existing.uId == item.uId) {
			existing = item;
			m_repo.Save(note);
			return true;
		}
	}
	return false;
}

bool NoteService::RemoveItem(Note& note, uint64_t itemId)
{
	for (auto it = note.items.begin(); it != note.items.end(); ++it) {
		if (it->uId == itemId) {
			note.items.erase(it);
			m_repo.Save(note);
			return true;
		}
	}
	return false;
}

bool NoteService::UpdateAllItems(Note& note, const std::vector<NoteItem>& items)
{
	note.items = items;
	m_repo.Save(note);
	return true;
}

void NoteService::UpdateAppearance(Note& note, COLORREF bgColor, bool opacityEnabled, int opacity, bool topMost)
{
	note.bgColor = bgColor;
	note.opacityEnabled = opacityEnabled;
	note.opacity = opacity;
	note.topMost = topMost;
	m_repo.Save(note);
}

void NoteService::UpdateTitle(Note& note, const CString& title)
{
	note.title = title;
	m_repo.Save(note);
}

void NoteService::UpdateRect(Note& note, const CRect& rect)
{
	note.rect = rect;
	m_repo.Save(note);
}

bool NoteService::Rename(Note& note, const CString& newName)
{
	if (newName.IsEmpty() || note.name == newName) return false;
	if (!m_repo.Rename(note.name, newName)) return false;

	note.name = newName;
	return true;
}

void NoteService::SetVisibility(Note& note, bool visible)
{
	note.visible = visible;
	m_repo.Save(note);
}

void NoteService::Hide(Note& note)
{
	SetVisibility(note, false);
}

void NoteService::Clear(const Note& note)
{
	m_repo.Delete(note.name);
}

bool NoteService::Archive(Note& note)
{
	bool ok = m_repo.Archive(note.name);
	if (ok)
	{
		note.isArchived = true;
		auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch()).count();
		note.archivedAt = static_cast<uint64_t>(nowMs);
		note.visible = false;
	}
	return ok;
}

bool NoteService::Archive(const CString& name)
{
	return m_repo.Archive(name);
}

bool NoteService::Unarchive(Note& note)
{
	bool ok = m_repo.Unarchive(note.name);
	if (ok)
	{
		note.isArchived = false;
		note.archivedAt = 0;
	}
	return ok;
}

bool NoteService::Unarchive(const CString& name)
{
	return m_repo.Unarchive(name);
}

bool NoteService::SoftDelete(Note& note)
{
	bool ok = m_repo.SoftDelete(note.name);
	if (ok)
	{
		note.isDeleted = true;
		auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch()).count();
		note.deletedAt = static_cast<uint64_t>(nowMs);
		note.visible = false;
	}
	return ok;
}

bool NoteService::SoftDelete(const CString& name)
{
	return m_repo.SoftDelete(name);
}

bool NoteService::Restore(Note& note)
{
	bool ok = m_repo.Restore(note.name);
	if (ok)
	{
		note.isDeleted = false;
		note.deletedAt = 0;
	}
	return ok;
}

bool NoteService::Restore(const CString& name)
{
	return m_repo.Restore(name);
}

bool NoteService::PermanentDelete(const CString& name)
{
	return m_repo.PermanentDelete(name);
}

bool NoteService::ClearTrash()
{
	return m_repo.ClearTrash();
}

void NoteService::ExportToCalendar(const NoteItem& item)
{
	CString sPath = Path::GetTmpDirectory(Cvt::ToString(CTime::GetCurrentTime(), _T("%Y%m%d%H%M%S")) + _T(".ics"));
	CString icsTpl = _T("BEGIN:VCALENDAR\n\
VERSION:2.0\n\
PRODID:-//Hancel.Lin//StickyNotes//EN\n\
BEGIN:VEVENT\n\
UID:{UID}\n\
DTSTAMP:{DTSTAMP}\n\
DTSTART:{DTSTART}\n\
DTEND :{DTEND}\n\
SUMMARY:{SUMMARY}\n\
LOCATION:\n\
DESCRIPTION:{DESCRIPTION}\n\
END:VEVENT\n\
END:VCALENDAR");

	icsTpl.Replace(_T("{UID}"), Cvt::ToString(item.uId));
	icsTpl.Replace(_T("{DTSTAMP}"), Cvt::ToString(CTime::GetCurrentTime(), _T("%Y%m%dT%H%M%SZ")));
	icsTpl.Replace(_T("{DTSTART}"), Cvt::ToString(CTime::GetCurrentTime(), _T("%Y%m%dT%H%M%SZ")));
	icsTpl.Replace(_T("{DTEND}"), Cvt::ToString(CTime::GetCurrentTime(), _T("%Y%m%dT%H%M%SZ")));
	icsTpl.Replace(_T("{SUMMARY}"), item.sContent);
	icsTpl.Replace(_T("{DESCRIPTION}"), item.sContent);

	std::string utf8String;
	int nUtf8Len = WideCharToMultiByte(CP_UTF8, 0, icsTpl.GetString(), icsTpl.GetLength(), NULL, 0, NULL, NULL);
	if (nUtf8Len > 0) {
		utf8String.resize(nUtf8Len);
		WideCharToMultiByte(CP_UTF8, 0, icsTpl.GetString(), icsTpl.GetLength(), &utf8String[0], nUtf8Len, NULL, NULL);
	}

	std::ofstream file(sPath.GetString());
	if (file.is_open()) {
		file << utf8String;
		file.close();
	}

	ShellExecute(NULL, _T("open"), sPath, NULL, NULL, SW_SHOWNORMAL);
}
