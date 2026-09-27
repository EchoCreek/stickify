// ------------------------------------------------------------------
// Copyright (c) Sticky Notes Refactoring Team (2026)
// Part of Sticky Notes Refactoring Project
// Licensed under the Apache License, Version 2.0
// ------------------------------------------------------------------
#pragma once

#include <afxwin.h>
#include <map>
#include <memory>

enum class NoteManagerMessageType {
	Unknown = 0,
	ListAll,
	GetNote,
	CreateNote,
	DeleteNotes,
	ToggleVisible,
	UpdateNote,
	LocateWindow,
	Export,
	Close,
	Min,
	Max,
	Move,
	Resize,
	GetAppSettings,
	SaveAppSettings,
	BrowseFolder,
	OpenFolder,
	ListTrash,
	RestoreNote,
	PermanentDelete,
	ClearTrash,
	ListArchived,
	ArchiveNote,
	UnarchiveNote,
	ExportDb,
	ImportDb
};

struct NoteManagerMessage {
	NoteManagerMessageType type;
	CString                dataJson;

	static NoteManagerMessage Parse(const wchar_t* rawJson);
	static NoteManagerMessageType TypeFromString(const CString& eventName);
};

class INoteManagerMessageHandler {
public:
	virtual ~INoteManagerMessageHandler() = default;
	virtual void Handle(const NoteManagerMessage& msg) = 0;
};

class NoteManagerMessageDispatcher {
public:
	void Register(NoteManagerMessageType type, INoteManagerMessageHandler* handler);
	void Dispatch(const wchar_t* rawJson);

private:
	std::map<NoteManagerMessageType, INoteManagerMessageHandler*> m_handlers;
};
