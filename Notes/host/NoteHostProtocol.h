// ------------------------------------------------------------------
// Copyright (c) Sticky Notes Refactoring Team (2026)
// Part of Sticky Notes Refactoring Project
// Licensed under the Apache License, Version 2.0
// ------------------------------------------------------------------
#pragma once

#include <afxwin.h>
#include <map>
#include <memory>

enum class NoteHostMessageType {
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
	EdgeLock
};

struct NoteHostMessage {
	NoteHostMessageType type;
	CString             dataJson;

	static NoteHostMessage Parse(const wchar_t* rawJson);
	static NoteHostMessageType TypeFromString(const CString& eventName);
};

class INoteHostMessageHandler {
public:
	virtual ~INoteHostMessageHandler() = default;
	virtual void Handle(const NoteHostMessage& msg) = 0;
};

class NoteHostMessageDispatcher {
public:
	void Register(NoteHostMessageType type, INoteHostMessageHandler* handler);
	void Dispatch(const wchar_t* rawJson);

private:
	std::map<NoteHostMessageType, INoteHostMessageHandler*> m_handlers;
};
