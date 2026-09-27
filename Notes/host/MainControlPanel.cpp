// ------------------------------------------------------------------
// Copyright (c) Sticky Notes Refactoring Team (2026)
// Part of Sticky Notes Refactoring Project
// Licensed under the Apache License, Version 2.0
// ------------------------------------------------------------------
#include "stdafx.h"
#include "Notes.h"
#include "host/MainControlPanel.h"
#include "infra/AppSettingStore.h"
#include "afxdialogex.h"
#include "ref/Log.h"
#include "ref/Path.h"
#include "ref/Utility.h"
#include "ref/HotKey.h"
#include <ole2.h>
#include <vector>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace {

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;

protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

} // anonymous namespace

// MainControlPanel dialog

MainControlPanel::MainControlPanel(CWnd* pParent /*=nullptr*/)
	: CDialogEx(MainControlPanel::IDD, pParent)
	, m_Instance(nullptr)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void MainControlPanel::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_HKEY_ACTIVE, m_ActiveHKey);
	DDX_Control(pDX, IDC_HKEY_NEW, m_NewHKey);
	DDX_Control(pDX, IDC_HKEY_UNACTIVE, m_UnActiveHKey);
	DDX_Control(pDX, IDC_HKEY_ACTIVEALL, m_ActiveAllHKey);
}

BEGIN_MESSAGE_MAP(MainControlPanel, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_CLOSE()
	ON_WM_DESTROY()
	ON_MESSAGE(WM_TRAYICON, OnTrayIcon)
	ON_COMMAND(ID_NEW, &MainControlPanel::OnNew)
	ON_COMMAND(ID_HIDEALL, &MainControlPanel::OnHideall)
	ON_COMMAND(ID_SHOWALL, &MainControlPanel::OnShowall)
	ON_COMMAND(ID_SHOW, &MainControlPanel::OnShow)
	ON_COMMAND(ID_QUIT, &MainControlPanel::OnQuit)
	ON_BN_CLICKED(IDC_BTN_BROWSE, &MainControlPanel::OnBnClickedBtnBrowse)
	ON_BN_CLICKED(IDC_BTN_BROWSE_RUNTIME, &MainControlPanel::OnBnClickedBtnBrowseRuntime)
	ON_BN_CLICKED(IDOK, &MainControlPanel::OnBnClickedOk)
	ON_COMMAND(ID_MENU_THROUGHALLON, &MainControlPanel::OnMenuThroughAllOn)
	ON_COMMAND(ID_MENU_THROUGHALLOFF, &MainControlPanel::OnMenuThroughAllOff)
	ON_COMMAND(ID_MANAGER, &MainControlPanel::OnManager)
END_MESSAGE_MAP()

BOOL MainControlPanel::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		CString strAboutMenu;
		if (strAboutMenu.LoadString(IDS_ABOUTBOX) && !strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	SetIcon(m_hIcon, TRUE);
	SetIcon(m_hIcon, FALSE);

	m_Instance = Easy::Utility::ProgramLock(_T("HANCEL_STICKY_NOTES_APP"));
	if (m_Instance == nullptr)
	{
		return FALSE;
	}

	Init();

	return TRUE;
}

void MainControlPanel::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

void MainControlPanel::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this);

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

HCURSOR MainControlPanel::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

bool MainControlPanel::ShowInTaskbar(HWND hWnd, bool isShow)
{
	ITaskbarList* pTaskbarList = nullptr;
	HRESULT hr = CoCreateInstance(CLSID_TaskbarList, nullptr, CLSCTX_INPROC_SERVER, IID_ITaskbarList, reinterpret_cast<void**>(&pTaskbarList));

	if (SUCCEEDED(hr) && pTaskbarList != nullptr)
	{
		pTaskbarList->HrInit();

		if (isShow)
		{
			pTaskbarList->AddTab(hWnd);
		}
		else
		{
			pTaskbarList->DeleteTab(hWnd);
		}

		pTaskbarList->Release();
		return true;
	}

	return false;
}

void MainControlPanel::Init()
{
	CLogApp::Init(LOG_ALL);

	std::vector<CString> lstTheme = AppSettingStore::SearchThemes();
	CComboBox* pCbbTheme = static_cast<CComboBox*>(GetDlgItem(IDC_CBB_THEME));
	if (pCbbTheme != nullptr)
	{
		pCbbTheme->ResetContent();
		for (const auto& theme : lstTheme)
		{
			pCbbTheme->AddString(theme.GetString());
		}
	}

	m_setting = AppSettingStore::Load();
	AppSettingStore::Save(m_setting);
	InitSetting(m_setting);

	SetHotKey(m_setting);
	SetTrayIcon();
	ShowInTaskbar(m_hWnd, false);
	SetWindowAlpha(0);

	SetWindowPos(&wndTopMost, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_HIDEWINDOW);

	m_manager.SetOnSettingsChangedCallback([this](const AppSetting& newSetting) {
		ApplySettings(newSetting);
	});

	m_manager.Init();
}

void MainControlPanel::InitSetting(const AppSetting& setting)
{
	CButton* pChkAuto = static_cast<CButton*>(GetDlgItem(IDC_CHK_AUTO));
	if (pChkAuto != nullptr)
	{
		pChkAuto->SetCheck(setting.bAutoRun ? BST_CHECKED : BST_UNCHECKED);
	}

	CButton* pChkCustom = static_cast<CButton*>(GetDlgItem(IDC_CHK_CUSTOM));
	if (pChkCustom != nullptr)
	{
		pChkCustom->SetCheck(setting.bCustomWebview2 ? BST_CHECKED : BST_UNCHECKED);
	}

	m_ActiveHKey.SetHotKey(setting.dwEditHotKey);
	m_NewHKey.SetHotKey(setting.dwNewHotKey);
	m_UnActiveHKey.SetHotKey(setting.dwUnActiveHotKey);
	m_ActiveAllHKey.SetHotKey(setting.dwActiveAllHotKey);
	SetDlgItemText(IDC_EDIT_NOTE, setting.sNoteDir);
	SetDlgItemText(IDC_EDIT_RUNTIME, setting.sWebview2Path);

	CComboBox* pCbbTheme = static_cast<CComboBox*>(GetDlgItem(IDC_CBB_THEME));
	if (pCbbTheme != nullptr)
	{
		pCbbTheme->SelectString(0, setting.sTheme);
	}
}

AppSetting MainControlPanel::ReadSetting()
{
	AppSetting setting;
	CButton* pChkAuto = static_cast<CButton*>(GetDlgItem(IDC_CHK_AUTO));
	setting.bAutoRun = (pChkAuto != nullptr && pChkAuto->GetCheck() == BST_CHECKED);

	CButton* pChkCustom = static_cast<CButton*>(GetDlgItem(IDC_CHK_CUSTOM));
	setting.bCustomWebview2 = (pChkCustom != nullptr && pChkCustom->GetCheck() == BST_CHECKED);

	setting.dwEditHotKey = m_ActiveHKey.GetHotKey();
	setting.dwNewHotKey = m_NewHKey.GetHotKey();
	setting.dwUnActiveHotKey = m_UnActiveHKey.GetHotKey();
	setting.dwActiveAllHotKey = m_ActiveAllHKey.GetHotKey();
	GetDlgItemText(IDC_EDIT_NOTE, setting.sNoteDir);
	GetDlgItemText(IDC_EDIT_RUNTIME, setting.sWebview2Path);
	GetDlgItemText(IDC_CBB_THEME, setting.sTheme);
	return setting;
}

void MainControlPanel::SetHotKey(const AppSetting& setting)
{
	if (setting.dwUnActiveHotKey != 0)
	{
		CHotKey::SetWithCall(setting.dwUnActiveHotKey, [](LPVOID lpParam) -> void {
			MainControlPanel* pMain = static_cast<MainControlPanel*>(lpParam);
			pMain->m_manager.SetVisible(false);
		}, this, GetSafeHwnd());
	}

	if (setting.dwActiveAllHotKey != 0)
	{
		CHotKey::SetWithCall(setting.dwActiveAllHotKey, [](LPVOID lpParam) -> void {
			MainControlPanel* pMain = static_cast<MainControlPanel*>(lpParam);
			pMain->m_manager.SetVisible(true);
		}, this, GetSafeHwnd());
	}

	if (setting.dwEditHotKey != 0)
	{
		CHotKey::SetWithCall(setting.dwEditHotKey, [](LPVOID lpParam) -> void {
			MainControlPanel* pMain = static_cast<MainControlPanel*>(lpParam);
			if (!pMain->m_manager.CheckEdit())
			{
				pMain->m_manager.FocusFirstNote();
			}
		}, this, GetSafeHwnd());
	}

	if (setting.dwNewHotKey != 0)
	{
		CHotKey::SetWithCall(setting.dwNewHotKey, [](LPVOID lpParam) -> void {
			MainControlPanel* pMain = static_cast<MainControlPanel*>(lpParam);
			pMain->m_manager.New();
		}, this, GetSafeHwnd());
	}
}

void MainControlPanel::ClearHotKey(const AppSetting& setting)
{
	HWND hWnd = GetSafeHwnd();
	if (setting.dwUnActiveHotKey != 0) CHotKey::RemoveHotKey(setting.dwUnActiveHotKey, hWnd);
	if (setting.dwActiveAllHotKey != 0) CHotKey::RemoveHotKey(setting.dwActiveAllHotKey, hWnd);
	if (setting.dwEditHotKey != 0) CHotKey::RemoveHotKey(setting.dwEditHotKey, hWnd);
	if (setting.dwNewHotKey != 0) CHotKey::RemoveHotKey(setting.dwNewHotKey, hWnd);
}

void MainControlPanel::SetTrayIcon()
{
	m_nid.cbSize = sizeof(NOTIFYICONDATA);
	m_nid.hWnd = GetSafeHwnd();
	m_nid.uID = IDR_MAINFRAME;
	m_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	m_nid.uCallbackMessage = WM_TRAYICON;
	m_nid.hIcon = m_hIcon;
	CString sTip;
	auto version = Easy::Utility::GetVersion(Easy::Path::GetProgramPath());
	sTip = _T("Stickify V") + version.ToString();
	_tcscpy_s(m_nid.szTip, sTip.GetString());
	Shell_NotifyIcon(NIM_ADD, &m_nid);

	UpdateTrayMenu(m_setting.sLanguage);
	ShowWindow(SW_HIDE);
}

void MainControlPanel::UpdateTrayMenu(const CString& language)
{
	bool isEn = (language.CompareNoCase(_T("en-US")) == 0);

	if (m_MenuTray.GetSafeHmenu())
	{
		m_MenuTray.DestroyMenu();
	}

	CMenu popup;
	popup.CreatePopupMenu();
	if (isEn)
	{
		popup.AppendMenu(MF_STRING, ID_NEW, _T("New Note (&N)"));
		popup.AppendMenu(MF_STRING, ID_MANAGER, _T("Manager & Settings (&M)"));
		popup.AppendMenu(MF_SEPARATOR);
		popup.AppendMenu(MF_STRING, ID_SHOWALL, _T("Show All (&S)"));
		popup.AppendMenu(MF_STRING, ID_HIDEALL, _T("Hide All (&H)"));
		popup.AppendMenu(MF_STRING, ID_MENU_THROUGHALLOFF, _T("Disable All Click-Through (&T)"));
		popup.AppendMenu(MF_SEPARATOR);
		popup.AppendMenu(MF_STRING, ID_QUIT, _T("Quit (&Q)"));
	}
	else
	{
		popup.AppendMenu(MF_STRING, ID_NEW, _T("新建便签 (&N)"));
		popup.AppendMenu(MF_STRING, ID_MANAGER, _T("便签管理与设置 (&M)"));
		popup.AppendMenu(MF_SEPARATOR);
		popup.AppendMenu(MF_STRING, ID_SHOWALL, _T("显示所有便签 (&S)"));
		popup.AppendMenu(MF_STRING, ID_HIDEALL, _T("隐藏所有便签 (&H)"));
		popup.AppendMenu(MF_STRING, ID_MENU_THROUGHALLOFF, _T("解除所有穿透 (&T)"));
		popup.AppendMenu(MF_SEPARATOR);
		popup.AppendMenu(MF_STRING, ID_QUIT, _T("退出程序 (&Q)"));
	}

	m_MenuTray.CreateMenu();
	m_MenuTray.AppendMenu(MF_POPUP, reinterpret_cast<UINT_PTR>(popup.Detach()), _T("Menu"));
}

void MainControlPanel::SetWindowAlpha(float fAlpha)
{
	ModifyStyleEx(0, WS_EX_LAYERED);
	SetLayeredWindowAttributes(0, static_cast<BYTE>(255 * fAlpha / 100), LWA_ALPHA);
}

BOOL MainControlPanel::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_HOTKEY)
	{
		CHotKey::Execute(pMsg->wParam);
	}
	if (pMsg->message == WM_KEYDOWN && (pMsg->wParam == VK_RETURN || pMsg->wParam == VK_ESCAPE))
	{
		return TRUE;
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}

void MainControlPanel::OnClose()
{
	ShowWindow(SW_HIDE);
}

void MainControlPanel::OnDestroy()
{
	SetAppExiting(true);
	CLogApp::Write(_T("MainControlPanel::OnDestroy() called"));
	m_manager.Cleanup();
	CDialogEx::OnDestroy();
	Shell_NotifyIcon(NIM_DELETE, &m_nid);
}

LRESULT MainControlPanel::OnTrayIcon(WPARAM wParam, LPARAM lParam)
{
	if (wParam != IDR_MAINFRAME)
		return 1;
	switch (lParam)
	{
	case WM_LBUTTONDBLCLK:
		OnShow();
		break;
	case WM_RBUTTONDOWN:
		{
			CPoint pt;
			GetCursorPos(&pt);
			SetForegroundWindow();
			m_MenuTray.GetSubMenu(0)->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON, pt.x, pt.y, this);
		}
		break;
	}
	return 0;
}

void MainControlPanel::OnOK()
{
	// Keep main panel dialog message pump alive
}

void MainControlPanel::OnCancel()
{
	CLogApp::Write(_T("MainControlPanel::OnCancel() hiding dialog"));
	ShowWindow(SW_HIDE);
}

void MainControlPanel::OnNew()
{
	CLogApp::Write(_T("MainControlPanel::OnNew() triggered"));
	m_manager.New();
}

void MainControlPanel::OnHideall()
{
	m_manager.SetVisible(false);
}

void MainControlPanel::OnShowall()
{
	m_manager.SetVisible(true);
}

void MainControlPanel::OnShow()
{
	m_manager.ShowManager();
}

void MainControlPanel::OnQuit()
{
	SetAppExiting(true);
	CLogApp::Write(_T("MainControlPanel::OnQuit() triggered - exiting app"));
	Shell_NotifyIcon(NIM_DELETE, &m_nid);
	m_manager.Cleanup();
	DestroyWindow();
	::PostQuitMessage(0);
}

void MainControlPanel::OnBnClickedBtnBrowse()
{
	CString sPath = Easy::Path::Folder(m_hWnd, m_setting.sNoteDir);
	if (sPath.IsEmpty()) return;
	SetDlgItemText(IDC_EDIT_NOTE, sPath);
}

void MainControlPanel::OnBnClickedBtnBrowseRuntime()
{
	CString sPath = Easy::Path::Browse(_T("Webview2 File|msedgewebview2.exe;||"), _T(""), TRUE, _T(""));
	if (sPath.IsEmpty()) return;
	SetDlgItemText(IDC_EDIT_RUNTIME, sPath);
}

void MainControlPanel::ApplySettings(const AppSetting& newSetting)
{
	AppSetting toApply = newSetting;

	if (toApply.sNoteDir.IsEmpty())
	{
		toApply.sNoteDir = Easy::Path::GetCurDirectory(_T("notes\\"));
	}

	if (toApply.sNoteDir.Right(1) != _T("\\") && toApply.sNoteDir.Right(1) != _T("/"))
	{
		toApply.sNoteDir += _T("\\");
	}

	if (toApply.sNoteDir.CompareNoCase(m_setting.sNoteDir) != 0 && !m_setting.sNoteDir.IsEmpty())
	{
		m_manager.SetNoteDir(toApply.sNoteDir);
	}

	Easy::Utility::SetAutoRun(toApply.bAutoRun);

	ClearHotKey(m_setting);
	SetHotKey(toApply);

	m_setting = toApply;
	AppSettingStore::Save(m_setting);
	UpdateTrayMenu(m_setting.sLanguage);

	if (GetSafeHwnd() && ::IsWindow(GetSafeHwnd()))
	{
		InitSetting(m_setting);
	}
}

void MainControlPanel::OnBnClickedOk()
{
	AppSetting setting = ReadSetting();

	if (setting.sNoteDir.IsEmpty()) {
		MessageBox(_T("请选择一个文件夹用于存放便签！"));
		return;
	}

	ApplySettings(setting);
}

void MainControlPanel::OnMenuThroughAllOn()
{
	m_manager.SetMouseThrough(true);
}

void MainControlPanel::OnMenuThroughAllOff()
{
	m_manager.SetMouseThrough(false);
}

void MainControlPanel::OnManager()
{
	CLogApp::Write(_T("MainControlPanel::OnManager() triggered"));
	m_manager.ShowManager();
}
