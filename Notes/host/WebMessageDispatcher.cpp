// ------------------------------------------------------------------
// Copyright (c) Sticky Notes Refactoring Team (2026)
// Part of Sticky Notes Refactoring Project
// Licensed under the Apache License, Version 2.0
// ------------------------------------------------------------------
#include "stdafx.h"
#include "host/WebMessageDispatcher.h"
#include "ref/Log.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

WebMessageType WebMessage::TypeFromString(const CString& eventName)
{
	static const std::map<CString, WebMessageType> s_typeMap = {
		{ _T("move"),        WebMessageType::Move },
		{ _T("resize"),      WebMessageType::Resize },
		{ _T("lock"),        WebMessageType::Lock },
		{ _T("top"),         WebMessageType::Top },
		{ _T("opacityable"),  WebMessageType::OpacityAble },
		{ _T("bgcolor"),     WebMessageType::BgColor },
		{ _T("title"),       WebMessageType::Title },
		{ _T("close"),       WebMessageType::Close },
		{ _T("add"),         WebMessageType::Add },
		{ _T("task"),        WebMessageType::Task },
		{ _T("update"),      WebMessageType::Update },
		{ _T("update_all"),  WebMessageType::UpdateAll },
		{ _T("remove"),      WebMessageType::Remove },
		{ _T("hide"),        WebMessageType::Hide },
		{ _T("clear"),       WebMessageType::Clear },
		{ _T("listen"),      WebMessageType::Listen },
		{ _T("restore_dock"),WebMessageType::RestoreDock },
		{ _T("edge_lock"),   WebMessageType::EdgeLock },
		{ _T("mgr_list_all"),      WebMessageType::MgrListAll },
		{ _T("mgr_get_note"),      WebMessageType::MgrGetNote },
		{ _T("mgr_create_note"),   WebMessageType::MgrCreateNote },
		{ _T("mgr_delete_notes"),  WebMessageType::MgrDeleteNotes },
		{ _T("mgr_toggle_visible"),WebMessageType::MgrToggleVisible },
		{ _T("mgr_update_note"),   WebMessageType::MgrUpdateNote },
		{ _T("mgr_locate_window"), WebMessageType::MgrLocateWindow },
		{ _T("mgr_export"),        WebMessageType::MgrExport },
		{ _T("mgr_close"),         WebMessageType::MgrClose },
		{ _T("mgr_min"),           WebMessageType::MgrMin },
		{ _T("mgr_max"),           WebMessageType::MgrMax },
		{ _T("mgr_move"),          WebMessageType::MgrMove },
		{ _T("mgr_resize"),        WebMessageType::MgrResize },
		{ _T("mgr_get_app_settings"),  WebMessageType::MgrGetAppSettings },
		{ _T("mgr_save_app_settings"), WebMessageType::MgrSaveAppSettings },
		{ _T("mgr_browse_folder"),     WebMessageType::MgrBrowseFolder },
		{ _T("mgr_open_folder"),       WebMessageType::MgrOpenFolder }
	};

	auto it = s_typeMap.find(eventName);
	if (it != s_typeMap.end())
	{
		return it->second;
	}
	return WebMessageType::Unknown;
}

WebMessage WebMessage::Parse(const wchar_t* rawJson)
{
	WebMessage msg;
	msg.type = WebMessageType::Unknown;
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

void WebMessageDispatcher::Register(WebMessageType type, IWebMessageHandler* handler)
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

void WebMessageDispatcher::Dispatch(const wchar_t* rawJson)
{
	WebMessage msg = WebMessage::Parse(rawJson);
	if (msg.type == WebMessageType::Unknown)
	{
		CLogApp::Warn(_T("WebMessageDispatcher::Dispatch: Unknown or malformed message: %s"), rawJson ? rawJson : _T("null"));
		return;
	}

	auto it = m_handlers.find(msg.type);
	if (it != m_handlers.end() && it->second != nullptr)
	{
		it->second->Handle(msg);
	}
	else
	{
		CLogApp::Warn(_T("WebMessageDispatcher::Dispatch: No handler registered for WebMessageType: %d"), static_cast<int>(msg.type));
	}
}
