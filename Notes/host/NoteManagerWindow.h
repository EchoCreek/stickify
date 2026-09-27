// ------------------------------------------------------------------
// Copyright (c) Sticky Notes Refactoring Team (2026)
// Part of Sticky Notes Refactoring Project
// Licensed under the Apache License, Version 2.0
// ------------------------------------------------------------------
#pragma once

#include <afxwin.h>
#include <afxdialogex.h>
#include <wrl.h>
#include <wil/com.h>
#include <memory>
#include <vector>
#include "WebView2.h"
#include "host/NoteManagerProtocol.h"
#include "core/services/NoteEventBus.h"
#include "infra/SqliteNoteRepository.h"
#include "Resource.h"

class NoteManager;

class NoteManagerWindow : public CDialogEx
{
	DECLARE_DYNAMIC(NoteManagerWindow)
public:
	enum { IDD = IDD_DLG_NOTE };

	NoteManagerWindow(NoteManager& manager, SqliteNoteRepository& repo, CWnd* pParent = nullptr);
	virtual ~NoteManagerWindow() override;

	void ShowManagerWindow();
	void BroadcastDataChanged(const CString& action, const CString& noteName);

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;
	virtual BOOL OnInitDialog() override;
	virtual void OnOK() override {}
	virtual void OnCancel() override;
	afx_msg void OnClose();
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnMove(int x, int y);
	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp);
	afx_msg LRESULT OnNcHitTest(CPoint point);
	afx_msg void OnWindowPosChanging(WINDOWPOS* lpwpos);
	afx_msg void OnWindowPosChanged(WINDOWPOS* lpwpos);
	afx_msg LRESULT OnDisplayChange(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnDpiChanged(WPARAM wParam, LPARAM lParam);
	afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
	DECLARE_MESSAGE_MAP()

private:
	void InitWebView();
	HRESULT OnCreateCoreWebView2ControllerCompleted(HRESULT result, ICoreWebView2Controller* controller);
	HRESULT OnWebMessageReceived(ICoreWebView2* webview, ICoreWebView2WebMessageReceivedEventArgs* args);
	HRESULT OnDocumentReady(ICoreWebView2* webview, ICoreWebView2NavigationCompletedEventArgs* args);

	void PostWebMessage(const CString& event, const CString& dataJson);
	void RegisterHandlers();
	void RefreshAllLists();  ///< Pushes all 3 lists to frontend; call after any data mutation
	std::vector<CString> ExtractNoteNames(const NoteManagerMessage& msg);

	void HandleListAll(const NoteManagerMessage& msg);
	void HandleGetNote(const NoteManagerMessage& msg);
	void HandleCreateNote(const NoteManagerMessage& msg);
	void HandleDeleteNotes(const NoteManagerMessage& msg);
	void HandleToggleVisible(const NoteManagerMessage& msg);
	void HandleUpdateNote(const NoteManagerMessage& msg);
	void HandleLocateWindow(const NoteManagerMessage& msg);
	void HandleExport(const NoteManagerMessage& msg);
	void HandleMove(const NoteManagerMessage& msg);
	void HandleResize(const NoteManagerMessage& msg);
	void HandleGetAppSettings(const NoteManagerMessage& msg);
	void HandleSaveAppSettings(const NoteManagerMessage& msg);
	void HandleBrowseFolder(const NoteManagerMessage& msg);
	void HandleOpenFolder(const NoteManagerMessage& msg);
	void HandleListTrash(const NoteManagerMessage& msg);
	void HandleRestoreNote(const NoteManagerMessage& msg);
	void HandlePermanentDelete(const NoteManagerMessage& msg);
	void HandleClearTrash(const NoteManagerMessage& msg);
	void HandleListArchived(const NoteManagerMessage& msg);
	void HandleArchiveNote(const NoteManagerMessage& msg);
	void HandleUnarchiveNote(const NoteManagerMessage& msg);
	void HandleExportDb(const NoteManagerMessage& msg);
	void HandleImportDb(const NoteManagerMessage& msg);

private:
	NoteManager&          m_manager;
	SqliteNoteRepository& m_repo;

	NoteManagerMessageDispatcher m_dispatcher;
	std::vector<std::unique_ptr<INoteManagerMessageHandler>> m_handlers;
	int                   m_busSubscription = 0;

	Microsoft::WRL::ComPtr<ICoreWebView2Controller>  m_controller;
	Microsoft::WRL::ComPtr<ICoreWebView2>            m_webView;
};
