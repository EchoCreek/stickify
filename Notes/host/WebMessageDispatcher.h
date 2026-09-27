// ------------------------------------------------------------------
// Copyright (c) Sticky Notes Refactoring Team (2026)
// Part of Sticky Notes Refactoring Project
// Licensed under the Apache License, Version 2.0
// ------------------------------------------------------------------
#pragma once

#include <afxwin.h>
#include <map>

enum class WebMessageType {
	Unknown = 0,
	Move,
	Resize,
	Lock,
	Top,
	OpacityAble,
	BgColor,
	Title,
	Close,
	Add,
	Task,
	Update,
	UpdateAll,
	Remove,
	Hide,
	Clear,
	Listen,
	RestoreDock,
	EdgeLock,
	// Manager specific messages
	MgrListAll,
	MgrGetNote,
	MgrCreateNote,
	MgrDeleteNotes,
	MgrToggleVisible,
	MgrUpdateNote,
	MgrLocateWindow,
	MgrExport,
	MgrClose,
	MgrMin,
	MgrMax,
	MgrMove,
	MgrResize,
	MgrGetAppSettings,
	MgrSaveAppSettings,
	MgrBrowseFolder,
	MgrOpenFolder
};

struct WebMessage {
	WebMessageType type;
	CString dataJson;

	static WebMessage Parse(const wchar_t* rawJson);
	static WebMessageType TypeFromString(const CString& eventName);
};

class IWebMessageHandler {
public:
	virtual ~IWebMessageHandler() = default;
	virtual void Handle(const WebMessage& msg) = 0;
};

class WebMessageDispatcher {
public:
	void Register(WebMessageType type, IWebMessageHandler* handler);
	void Dispatch(const wchar_t* rawJson);

private:
	std::map<WebMessageType, IWebMessageHandler*> m_handlers;
};
