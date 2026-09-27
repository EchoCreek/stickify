// ------------------------------------------------------------------
// Copyright (c) Sticky Notes Refactoring Team (2026)
// Part of Sticky Notes Refactoring Project
// Licensed under the Apache License, Version 2.0
// ------------------------------------------------------------------
#include "stdafx.h"
#include "host/NoteManagerProtocol.h"
#include "ref/Log.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

NoteManagerMessageType NoteManagerMessage::TypeFromString(const CString& eventName)
{
	static const std::map<CString, NoteManagerMessageType> s_typeMap = {
		{ _T("mgr_list_all"),          NoteManagerMessageType::ListAll },
		{ _T("mgr_get_note"),          NoteManagerMessageType::GetNote },
		{ _T("mgr_create_note"),       NoteManagerMessageType::CreateNote },
		{ _T("mgr_delete_notes"),      NoteManagerMessageType::DeleteNotes },
		{ _T("mgr_toggle_visible"),    NoteManagerMessageType::ToggleVisible },
		{ _T("mgr_update_note"),       NoteManagerMessageType::UpdateNote },
		{ _T("mgr_locate_window"),     NoteManagerMessageType::LocateWindow },
		{ _T("mgr_export"),            NoteManagerMessageType::Export },
		{ _T("mgr_close"),             NoteManagerMessageType::Close },
		{ _T("mgr_min"),               NoteManagerMessageType::Min },
		{ _T("mgr_max"),               NoteManagerMessageType::Max },
		{ _T("mgr_move"),              NoteManagerMessageType::Move },
		{ _T("mgr_resize"),            NoteManagerMessageType::Resize },
		{ _T("mgr_get_app_settings"),  NoteManagerMessageType::GetAppSettings },
		{ _T("mgr_save_app_settings"), NoteManagerMessageType::SaveAppSettings },
		{ _T("mgr_browse_folder"),     NoteManagerMessageType::BrowseFolder },
		{ _T("mgr_open_folder"),       NoteManagerMessageType::OpenFolder },
		{ _T("mgr_list_trash"),        NoteManagerMessageType::ListTrash },
		{ _T("mgr_restore_note"),      NoteManagerMessageType::RestoreNote },
		{ _T("mgr_permanent_delete"),  NoteManagerMessageType::PermanentDelete },
		{ _T("mgr_clear_trash"),       NoteManagerMessageType::ClearTrash },
		{ _T("mgr_list_archived"),     NoteManagerMessageType::ListArchived },
		{ _T("mgr_archive_note"),      NoteManagerMessageType::ArchiveNote },
		{ _T("mgr_unarchive_note"),    NoteManagerMessageType::UnarchiveNote },
		{ _T("mgr_export_db"),         NoteManagerMessageType::ExportDb },
		{ _T("mgr_import_db"),         NoteManagerMessageType::ImportDb }
	};

	auto it = s_typeMap.find(eventName);
	if (it != s_typeMap.end())
	{
		return it->second;
	}
	return NoteManagerMessageType::Unknown;
}

NoteManagerMessage NoteManagerMessage::Parse(const wchar_t* rawJson)
{
	NoteManagerMessage msg;
	msg.type = NoteManagerMessageType::Unknown;
	msg.dataJson = _T("");

	if (rawJson == nullptr || rawJson[0] == L'\0')
	{
		return msg;
	}

	rapidjson::GenericDocument<rapidjson::UTF16<TCHAR>> doc;
	doc.Parse(rawJson);
	if (doc.HasParseError() || !doc.IsObject())
	{
		return msg;
	}

	if (!doc.HasMember(_T("event")) || !doc[_T("event")].IsString())
	{
		return msg;
	}

	msg.type = TypeFromString(doc[_T("event")].GetString());

	if (doc.HasMember(_T("data")))
	{
		rapidjson::GenericStringBuffer<rapidjson::UTF16<TCHAR>> buffer;
		rapidjson::Writer<rapidjson::GenericStringBuffer<rapidjson::UTF16<TCHAR>>, rapidjson::UTF16<TCHAR>, rapidjson::UTF16<TCHAR>> writer(buffer);
		doc[_T("data")].Accept(writer);
		msg.dataJson = buffer.GetString();
	}

	return msg;
}

void NoteManagerMessageDispatcher::Register(NoteManagerMessageType type, INoteManagerMessageHandler* handler)
{
	if (handler == nullptr)
	{
		m_handlers.erase(type);
	}
	else
	{
		m_handlers[type] = handler;
	}
}

void NoteManagerMessageDispatcher::Dispatch(const wchar_t* rawJson)
{
	NoteManagerMessage msg = NoteManagerMessage::Parse(rawJson);
	if (msg.type == NoteManagerMessageType::Unknown)
	{
		CLogApp::Warn(_T("NoteManagerMessageDispatcher::Dispatch: Unknown or malformed message: %s"), rawJson ? rawJson : _T("null"));
		return;
	}

	auto it = m_handlers.find(msg.type);
	if (it != m_handlers.end() && it->second != nullptr)
	{
		it->second->Handle(msg);
	}
	else
	{
		CLogApp::Warn(_T("NoteManagerMessageDispatcher::Dispatch: No handler registered for NoteManagerMessageType: %d"), static_cast<int>(msg.type));
	}
}
