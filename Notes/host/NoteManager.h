// ------------------------------------------------------------------
// Copyright (c) Sticky Notes Refactoring Team (2026)
// Part of Sticky Notes Refactoring Project
// Licensed under the Apache License, Version 2.0
// ------------------------------------------------------------------
#pragma once

#include <vector>
#include <memory>
#include <afxwin.h>
#include "core/ports/INoteRepository.h"
#include "core/services/NoteService.h"
#include "infra/SqliteNoteRepository.h"
#include "host/NoteHostWindow.h"

class NoteManagerWindow;

class NoteManager {
public:
	NoteManager();
	explicit NoteManager(INoteRepository& repo);
	~NoteManager();

	// 初始化：迁移旧数据，加载全部便签并创建窗口
	void Init();

	// 新建一个便签窗口
	NoteHostWindow* New(const CString& noteName = _T(""));

	// 全局操作
	bool CheckEdit();
	void FocusFirstNote();
	void SetMouseThrough(bool through = true);
	void SetVisible(bool show);
	void Cleanup(); // 有序销毁所有便签窗口与管理器窗口，杜绝退出时 COM 竞态崩溃

	// 便签管理中心控制接口
	void ShowManager();
	void BroadcastDataChanged(const CString& action, const CString& noteName);
	void SetNoteVisible(const CString& noteName, bool visible);
	void CloseNoteWindow(const CString& noteName);
	void LocateNoteWindow(const CString& noteName);
	void SyncActiveNoteWindow(const Note& note);

	SqliteNoteRepository& GetRepo() { return m_sqliteRepo; }
	NoteService&          GetService() { return m_service; }

	// 设置与系统服务
	void SetOnSettingsChangedCallback(std::function<void(const AppSetting&)> cb) { m_onSettingsChanged = std::move(cb); }
	void ApplySettings(const AppSetting& setting);
	bool SetNoteDir(const CString& newDir);
	void ReloadNotesFromRepo();
	AppSetting GetSettings() const;
	CString BrowseFolder(HWND hWnd, const CString& defaultPath = _T(""));
	void OpenFolder(const CString& path);
	CString ExportDatabaseDialog(HWND hWnd);
	bool ImportDatabaseDialog(HWND hWnd, int& outNoteCount);

	// 窗口关闭回调
	void OnWindowClosed(NoteHostWindow* win);

private:
	SqliteNoteRepository m_sqliteRepo;
	INoteRepository&     m_repo;
	NoteService          m_service;
	std::vector<std::unique_ptr<NoteHostWindow>> m_windows;
	std::unique_ptr<NoteManagerWindow>           m_managerWindow;
	std::function<void(const AppSetting&)>       m_onSettingsChanged;
};
