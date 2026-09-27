// ------------------------------------------------------------------
// Copyright (c) Sticky Notes Refactoring Team (2026)
// Part of Sticky Notes Refactoring Project
// Licensed under the Apache License, Version 2.0
// ------------------------------------------------------------------
#pragma once

#include <afxwin.h>
#include <afxdialogex.h>
#include "control/HotKeyEdit.h"
#include "core/domain/AppSetting.h"
#include "host/NoteManager.h"
#include "Resource.h"

class MainControlPanel : public CDialogEx
{
public:
	explicit MainControlPanel(CWnd* pParent = nullptr);
	virtual ~MainControlPanel() override = default;

	void ApplySettings(const AppSetting& setting);

	enum { IDD = IDD_NOTES_DIALOG, WM_TRAYICON = WM_USER + 100 };

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;
	virtual BOOL OnInitDialog() override;
	virtual BOOL PreTranslateMessage(MSG* pMsg) override;
	virtual void OnOK() override;
	virtual void OnCancel() override;

	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnClose();
	afx_msg void OnDestroy();
	afx_msg LRESULT OnTrayIcon(WPARAM wParam, LPARAM lParam);
	afx_msg void OnNew();
	afx_msg void OnHideall();
	afx_msg void OnShowall();
	afx_msg void OnShow();
	afx_msg void OnQuit();
	afx_msg void OnBnClickedBtnBrowse();
	afx_msg void OnBnClickedBtnBrowseRuntime();
	afx_msg void OnBnClickedOk();
	afx_msg void OnMenuThroughAllOn();
	afx_msg void OnMenuThroughAllOff();
	afx_msg void OnManager();

protected:
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam) override;

	DECLARE_MESSAGE_MAP()

private:
	void Init();
	void InitSetting(const AppSetting& setting);
	AppSetting ReadSetting();
	void SetHotKey(const AppSetting& setting);
	void ClearHotKey(const AppSetting& setting);
	void SetTrayIcon();
	void UpdateTrayMenu(const CString& language);
	void SetWindowAlpha(float fAlpha);
	bool ShowInTaskbar(HWND hWnd, bool isShow);

private:
	HICON          m_hIcon;
	CHotKeyEdit    m_ActiveHKey;
	CHotKeyEdit    m_UnActiveHKey;
	CHotKeyEdit    m_ActiveAllHKey;
	CHotKeyEdit    m_NewHKey;
	CMenu          m_MenuTray;
	NOTIFYICONDATA m_nid;
	HANDLE         m_Instance;

	NoteManager    m_manager;
	AppSetting     m_setting;
};
