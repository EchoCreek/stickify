// ------------------------------------------------------------------
// Copyright (c) Sticky Notes Refactoring Team (2026)
// Part of Sticky Notes Refactoring Project
// Licensed under the Apache License, Version 2.0
// ------------------------------------------------------------------
#include "stdafx.h"
#include "host/NoteHostProtocol.h"
#include "ref/Log.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

NoteHostMessageType NoteHostMessage::TypeFromString(const CString& eventName)
{
	static const std::map<CString, NoteHostMessageType> s_typeMap = {
		{ _T("move"),         NoteHostMessageType::Move },
		{ _T("resize"),       NoteHostMessageType::Resize },
		{ _T("lock"),         NoteHostMessageType::Lock },
		{ _T("top"),          NoteHostMessageType::Top },
		{ _T("opacityable"),  NoteHostMessageType::OpacityAble },
		{ _T("bgcolor"),      NoteHostMessageType::BgColor },
		{ _T("title"),        NoteHostMessageType::Title },
		{ _T("close"),        NoteHostMessageType::Close },
		{ _T("add"),          NoteHostMessageType::Add },
		{ _T("task"),         NoteHostMessageType::Task },
		{ _T("update"),       NoteHostMessageType::Update },
		{ _T("update_all"),   NoteHostMessageType::UpdateAll },
		{ _T("remove"),       NoteHostMessageType::Remove },
		{ _T("hide"),         NoteHostMessageType::Hide },
		{ _T("clear"),        NoteHostMessageType::Clear },
		{ _T("listen"),       NoteHostMessageType::Listen },
		{ _T("restore_dock"), NoteHostMessageType::RestoreDock },
		{ _T("edge_lock"),    NoteHostMessageType::EdgeLock }
	};

	auto it = s_typeMap.find(eventName);
	if (it != s_typeMap.end())
	{
		return it->second;
	}
	return NoteHostMessageType::Unknown;
}

NoteHostMessage NoteHostMessage::Parse(const wchar_t* rawJson)
{
	NoteHostMessage msg;
	msg.type = NoteHostMessageType::Unknown;
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

void NoteHostMessageDispatcher::Register(NoteHostMessageType type, INoteHostMessageHandler* handler)
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

void NoteHostMessageDispatcher::Dispatch(const wchar_t* rawJson)
{
	NoteHostMessage msg = NoteHostMessage::Parse(rawJson);
	if (msg.type == NoteHostMessageType::Unknown)
	{
		CLogApp::Write(_T("NoteHostMessageDispatcher::Dispatch: Unknown or malformed message: %s"), rawJson ? rawJson : _T("null"));
		return;
	}

	auto it = m_handlers.find(msg.type);
	if (it != m_handlers.end() && it->second != nullptr)
	{
		it->second->Handle(msg);
	}
	else
	{
		CLogApp::Write(_T("NoteHostMessageDispatcher::Dispatch: No handler registered for NoteHostMessageType: %d"), static_cast<int>(msg.type));
	}
}
