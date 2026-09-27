// ------------------------------------------------------------------
// Copyright (c) Sticky Notes Refactoring Team (2026)
// Part of Sticky Notes Refactoring Project
// Licensed under the Apache License, Version 2.0
// ------------------------------------------------------------------
#include "stdafx.h"
#include "host/NoteManagerWindow.h"
#include "host/NoteManager.h"
#include "host/WebViewEnvironmentManager.h"
#include "infra/AppSettingStore.h"
#include "ref/Path.h"
#include "ref/Log.h"
#include "ref/Cvt.h"
#include "ref/HotKey.h"
#include "ref/Utility.h"
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#include <shobjidl_core.h>
#pragma comment(lib, "shell32.lib")
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "host/NoteDto.h"

// ------------------------------------------------------------------
// Named constants (replaces magic literals scattered across the file)
// ------------------------------------------------------------------
namespace NoteManagerConst {
    // DWM attribute IDs (not always available in older SDK headers)
    static constexpr DWORD  kDwmWindowCornerPref      = 33; // DWMWA_WINDOW_CORNER_PREFERENCE
    static constexpr DWORD  kDwmTransitionsDisabled   = 2;  // DWMWA_TRANSITIONS_FORCEDISABLED
    // Manager window minimum dimensions
    static constexpr int    kMinWidth                 = 1220;
    static constexpr int    kMinHeight                = 780;
    // SC_SIZE sub-codes for resize direction proxying
    static constexpr WPARAM kScSizeW   = 0xF001;
    static constexpr WPARAM kScSizeE   = 0xF002;
    static constexpr WPARAM kScSizeN   = 0xF003;
    static constexpr WPARAM kScSizeNW  = 0xF004;
    static constexpr WPARAM kScSizeNE  = 0xF005;
    static constexpr WPARAM kScSizeS   = 0xF006;
    static constexpr WPARAM kScSizeSW  = 0xF007;
    static constexpr WPARAM kScSizeSE  = 0xF008; // default
    static constexpr WPARAM kScMove    = 0xF012; // SC_MOVE + HTCAPTION
    // Default note appearance
    static constexpr TCHAR  kDefaultBgColor[]         = _T("#0d1117");
    static constexpr BYTE   kDefaultBgR               = 13;
    static constexpr BYTE   kDefaultBgG               = 17;
    static constexpr BYTE   kDefaultBgB               = 23;
}


IMPLEMENT_DYNAMIC(NoteManagerWindow, CDialogEx)

namespace {
	struct LambdaManagerHandler : public INoteManagerMessageHandler {
		std::function<void(const NoteManagerMessage&)> fn;
		explicit LambdaManagerHandler(std::function<void(const NoteManagerMessage&)> f) : fn(std::move(f)) {}
		virtual void Handle(const NoteManagerMessage& msg) override {
			if (fn) fn(msg);
		}
	};
}

NoteManagerWindow::NoteManagerWindow(NoteManager& manager, SqliteNoteRepository& repo, CWnd* pParent)
	: CDialogEx(NoteManagerWindow::IDD, pParent)
	, m_manager(manager)
	, m_repo(repo)
{
}

NoteManagerWindow::~NoteManagerWindow()
{
	m_webView = nullptr;
	if (m_controller != nullptr)
	{
		m_controller->Close();
		m_controller = nullptr;
	}
}

void NoteManagerWindow::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(NoteManagerWindow, CDialogEx)
	ON_WM_CLOSE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_MOVE()
	ON_WM_ACTIVATE()
	ON_WM_GETMINMAXINFO()
	ON_WM_ERASEBKGND()
	ON_WM_NCCALCSIZE()
	ON_WM_NCHITTEST()
	ON_WM_WINDOWPOSCHANGING()
	ON_WM_WINDOWPOSCHANGED()
	ON_MESSAGE(WM_DISPLAYCHANGE, OnDisplayChange)
	ON_MESSAGE(WM_DPICHANGED, OnDpiChanged)
	ON_WM_SETTINGCHANGE()
END_MESSAGE_MAP()

BOOL NoteManagerWindow::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	CLogApp::Write(_T("NoteManagerWindow::OnInitDialog: hwnd=0x%p"), m_hWnd);

	// 关键：彻底解除与主控/便签窗口的 Owner 关联，使其成为完全独立的顶级窗口，绝不继承任何前台/置顶层级
	::SetWindowLongPtr(m_hWnd, GWLP_HWNDPARENT, (LONG_PTR)NULL);

	// 关键：保留 WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU，并移除 WS_POPUP。
	// Windows DWM 仅对包含 WS_CAPTION 样式的顶级窗口启用飞入任务栏与全屏扩展的系统级硬件加速过渡动画。
	// 通过在 OnNcCalcSize 中返回 0 消除非客户区标题栏占用，实现 100% 现代沉浸式无边框 + 100% DWM 原生过渡动画！
	ModifyStyle(WS_POPUP, WS_CAPTION | WS_CLIPCHILDREN | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
	ModifyStyleEx(WS_EX_TOOLWINDOW | WS_EX_TOPMOST, WS_EX_APPWINDOW);
	SetWindowText(_T("Sticky Notes Manager"));

	// 设置任务栏与 Alt+Tab 独立图标
	HICON hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	if (hIcon)
	{
		SetIcon(hIcon, TRUE);  // 32x32 大图标
		SetIcon(hIcon, FALSE); // 16x16 小图标
	}

	// 将默认打开的尺寸设置为精确的 1220 x 780 并居中显示 (比 1200 恰好多 20px)
	HMONITOR hMon = ::MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
	MONITORINFO mi = { sizeof(MONITORINFO) };
	int workW = 1920, workH = 1080;
	int leftOffset = 0, topOffset = 0;
	if (hMon && ::GetMonitorInfo(hMon, &mi))
	{
		workW = mi.rcWork.right - mi.rcWork.left;
		workH = mi.rcWork.bottom - mi.rcWork.top;
		leftOffset = mi.rcWork.left;
		topOffset = mi.rcWork.top;
	}

	int width = min(NoteManagerConst::kMinWidth,  (int)(workW * 0.95));
	int height = min(NoteManagerConst::kMinHeight, (int)(workH * 0.95));
	int x = leftOffset + (workW - width) / 2;
	int y = topOffset + (workH - height) / 2;

	// 初始化时先隐藏窗口，等待 WebView2 内容就绪后再显示，且明确不置顶 (wndNoTopMost)
	SetWindowPos(&wndNoTopMost, x, y, width, height, SWP_HIDEWINDOW | SWP_FRAMECHANGED);

	// 启用 Windows 11 DWM 系统级圆角过渡
	typedef enum {
		_DWMWCP_DEFAULT = 0,
		_DWMWCP_DONOTROUND = 1,
		_DWMWCP_ROUND = 2,
		_DWMWCP_ROUNDSMALL = 3
	} _DWM_WINDOW_CORNER_PREFERENCE;
	DWM_WINDOW_CORNER_PREFERENCE cornerPref = DWMWCP_ROUND;
	DwmSetWindowAttribute(m_hWnd, NoteManagerConst::kDwmWindowCornerPref, &cornerPref, sizeof(cornerPref));

	// 启用 DWM 最小化/最大化过渡动画
	BOOL disableTransitions = FALSE;
	DwmSetWindowAttribute(m_hWnd, NoteManagerConst::kDwmTransitionsDisabled, &disableTransitions, sizeof(disableTransitions));

	// 注册 ITaskbarList3 使窗口在任务栏显示独立缩略图按钮并激活任务栏动画锚点
	{
		Microsoft::WRL::ComPtr<ITaskbarList3> pTaskbarList;
		if (SUCCEEDED(::CoCreateInstance(CLSID_TaskbarList, nullptr, CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS(&pTaskbarList))))
		{
			pTaskbarList->HrInit();
			pTaskbarList->AddTab(m_hWnd);
		}
	}

	m_busSubscription = NoteEventBus::Instance().Subscribe([this](const NoteEvent& ev) {
		BroadcastDataChanged(ev.TypeToString(), ev.noteName);
	});

	RegisterHandlers();
	InitWebView();

	return TRUE;
}

void NoteManagerWindow::OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp)
{
	if (bCalcValidRects && lpncsp != nullptr)
	{
		if (IsZoomed())
		{
			// 关键修复：当无边框窗口最大化时，Windows 默认会向四周外溢 8px（用于避让标准边框）。
			// 针对无边框沉浸式窗口，必须在最大化时将 client rect 限制在屏幕工作区内，彻底解决四周 UI 超出屏幕被裁切的问题！
			HMONITOR hMon = ::MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
			MONITORINFO mi = { sizeof(MONITORINFO) };
			if (hMon && ::GetMonitorInfo(hMon, &mi))
			{
				lpncsp->rgrc[0] = mi.rcWork;
				return;
			}
		}
		// 正常非最大化状态：消除非客户区原生标题栏，让客户区填满整个窗口，同时保留 WS_CAPTION 以激活 DWM 动画
		return;
	}
	CDialogEx::OnNcCalcSize(bCalcValidRects, lpncsp);
}

LRESULT NoteManagerWindow::OnNcHitTest(CPoint point)
{
	if (IsZoomed())
	{
		return HTCLIENT;
	}

	CRect rc;
	GetWindowRect(&rc);

	// 关键修复：标题栏右上角控制按钮区域（最小化/最大化/关闭按钮，宽约 140px，高 38px）
	// 必须强制返回 HTCLIENT，确保鼠标悬停到右上角角落时完整触发前端 Hover 与 Click，不被系统 NC 调整大小光标截断！
	if (point.y >= rc.top && point.y < rc.top + 38 && point.x > rc.right - 140)
	{
		return HTCLIENT;
	}

	constexpr int BORDER = 6;
	bool left = (point.x >= rc.left && point.x < rc.left + BORDER);
	bool right = (point.x <= rc.right && point.x > rc.right - BORDER);
	bool top = (point.y >= rc.top && point.y < rc.top + BORDER);
	bool bottom = (point.y <= rc.bottom && point.y > rc.bottom - BORDER);

	if (top && left) return HTTOPLEFT;
	if (top && right) return HTTOPRIGHT;
	if (bottom && left) return HTBOTTOMLEFT;
	if (bottom && right) return HTBOTTOMRIGHT;
	if (left) return HTLEFT;
	if (right) return HTRIGHT;
	if (top) return HTTOP;
	if (bottom) return HTBOTTOM;

	return HTCLIENT;
}

void NoteManagerWindow::OnWindowPosChanging(WINDOWPOS* lpwpos)
{
	CDialogEx::OnWindowPosChanging(lpwpos);
	if (lpwpos)
	{
		lpwpos->flags |= SWP_NOCOPYBITS;
		// 严防任何外部或同进程机制将管理器窗口误置顶
		if (lpwpos->hwndInsertAfter == HWND_TOPMOST)
		{
			lpwpos->hwndInsertAfter = HWND_NOTOPMOST;
		}
	}
}

void NoteManagerWindow::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	CDialogEx::OnGetMinMaxInfo(lpMMI);
	lpMMI->ptMinTrackSize.x = NoteManagerConst::kMinWidth;
	lpMMI->ptMinTrackSize.y = NoteManagerConst::kMinHeight;

	// 关键修复：指定最大化时的坐标与尺寸为当前显示器工作区，防止窗口撑出屏幕四周或遮挡任务栏
	HMONITOR hMon = ::MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
	MONITORINFO mi = { sizeof(MONITORINFO) };
	if (hMon && ::GetMonitorInfo(hMon, &mi))
	{
		lpMMI->ptMaxPosition.x = abs(mi.rcWork.left - mi.rcMonitor.left);
		lpMMI->ptMaxPosition.y = abs(mi.rcWork.top - mi.rcMonitor.top);
		lpMMI->ptMaxSize.x = mi.rcWork.right - mi.rcWork.left;
		lpMMI->ptMaxSize.y = mi.rcWork.bottom - mi.rcWork.top;
	}
}

void NoteManagerWindow::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	if (m_controller != nullptr && cx > 0 && cy > 0)
	{
		RECT rcbounds = { 0, 0, cx, cy };
		m_controller->put_Bounds(rcbounds);
		m_controller->NotifyParentWindowPositionChanged();
	}
}

void NoteManagerWindow::OnMove(int x, int y)
{
	CDialogEx::OnMove(x, y);
	if (m_controller != nullptr)
	{
		m_controller->NotifyParentWindowPositionChanged();
	}
}

void NoteManagerWindow::OnWindowPosChanged(WINDOWPOS* lpwpos)
{
	CDialogEx::OnWindowPosChanged(lpwpos);
	if (m_controller != nullptr)
	{
		RECT rcbounds = { 0 };
		GetClientRect(&rcbounds);
		m_controller->put_Bounds(rcbounds);
		m_controller->NotifyParentWindowPositionChanged();
	}
}

LRESULT NoteManagerWindow::OnDpiChanged(WPARAM wParam, LPARAM lParam)
{
	LPRECT prc = reinterpret_cast<LPRECT>(lParam);
	if (prc)
	{
		SetWindowPos(NULL, prc->left, prc->top, prc->right - prc->left, prc->bottom - prc->top,
			SWP_NOZORDER | SWP_NOACTIVATE);
	}
	if (m_controller != nullptr)
	{
		RECT rcbounds = { 0 };
		GetClientRect(&rcbounds);
		m_controller->put_Bounds(rcbounds);
		m_controller->NotifyParentWindowPositionChanged();
	}
	return 0;
}

BOOL NoteManagerWindow::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}

LRESULT NoteManagerWindow::OnDisplayChange(WPARAM wParam, LPARAM lParam)
{
	CRect rc;
	GetWindowRect(&rc);
	HMONITOR hMon = ::MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
	MONITORINFO mi = { sizeof(MONITORINFO) };
	if (hMon && ::GetMonitorInfo(hMon, &mi))
	{
		const CRect& rcWork = mi.rcWork;
		int w = min(rc.Width(), (int)(rcWork.Width() * 0.95));
		int h = min(rc.Height(), (int)(rcWork.Height() * 0.95));
		int x = rc.left;
		int y = rc.top;
		if (x + w > rcWork.right) x = rcWork.right - w;
		if (x < rcWork.left) x = rcWork.left;
		if (y + h > rcWork.bottom) y = rcWork.bottom - h;
		if (y < rcWork.top) y = rcWork.top;
		SetWindowPos(NULL, x, y, w, h, SWP_NOZORDER | SWP_NOACTIVATE);
	}
	return 0;
}

void NoteManagerWindow::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
	CDialogEx::OnSettingChange(uFlags, lpszSection);
	if (uFlags == SPI_SETWORKAREA)
	{
		OnDisplayChange(0, 0);
	}
}

void NoteManagerWindow::OnClose()
{
	OnCancel();
}

void NoteManagerWindow::OnDestroy()
{
	if (m_busSubscription != 0)
	{
		NoteEventBus::Instance().Unsubscribe(m_busSubscription);
		m_busSubscription = 0;
	}
	m_webView = nullptr;
	if (m_controller != nullptr)
	{
		m_controller->Close();
		m_controller = nullptr;
	}
	CDialogEx::OnDestroy();
}

void NoteManagerWindow::OnCancel()
{
	ShowWindow(SW_HIDE);
}

void NoteManagerWindow::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	CDialogEx::OnActivate(nState, pWndOther, bMinimized);
	if (nState == WA_INACTIVE && m_hWnd && ::IsWindow(m_hWnd))
	{
		// 焦点转移至外部其他窗口时，确保管理器完全处于非置顶层级，顺畅沉底
		::SetWindowPos(m_hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	}
}

void NoteManagerWindow::ShowManagerWindow()
{
	CLogApp::Write(_T("NoteManagerWindow::ShowManagerWindow: hwnd=0x%p"), m_hWnd);
	if (m_hWnd == NULL || !::IsWindow(m_hWnd))
	{
		// 显式传入 Desktop 窗口，彻底切断 MFC 自动将 AfxGetMainWnd() 设为 Owner 的隐式层级绑定
		BOOL created = Create(NoteManagerWindow::IDD, CWnd::GetDesktopWindow());
		if (!created)
		{
			CLogApp::Error(_T("NoteManagerWindow::ShowManagerWindow: Create failed, hwnd=0x%p"), m_hWnd);
		}
		else
		{
			CLogApp::Info(_T("NoteManagerWindow::ShowManagerWindow: Create returned %d, hwnd=0x%p"), created, m_hWnd);
		}
	}
	else
	{
		if (IsIconic())
		{
			ShowWindow(SW_RESTORE);
		}
		else
		{
			ShowWindow(SW_SHOW);
		}
		::SetWindowPos(m_hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
		SetForegroundWindow();
		RefreshAllLists();
	}
}

void NoteManagerWindow::BroadcastDataChanged(const CString& action, const CString& noteName)
{
	if (m_webView == nullptr) return;

	// Use RapidJSON to properly escape action and noteName,
	// preventing JSON injection when note names contain quotes or backslashes.
	RJDoc doc;
	auto& alloc = doc.GetAllocator();
	RJValue v(rapidjson::kObjectType);
	v.AddMember(rapidjson::StringRef(_T("action")),
	            RJValue(action.GetString(), alloc).Move(), alloc);
	v.AddMember(rapidjson::StringRef(_T("name")),
	            RJValue(noteName.GetString(), alloc).Move(), alloc);

	rapidjson::GenericStringBuffer<rapidjson::UTF16<TCHAR>> buffer;
	rapidjson::Writer<rapidjson::GenericStringBuffer<rapidjson::UTF16<TCHAR>>,
	                  rapidjson::UTF16<TCHAR>, rapidjson::UTF16<TCHAR>> writer(buffer);
	v.Accept(writer);

	PostWebMessage(_T("mgr_data_changed"), buffer.GetString());
}

/// Push all three list views to the frontend in one call.
/// Replaces the identical 5-line block that was duplicated 9 times.
void NoteManagerWindow::RefreshAllLists()
{
	NoteManagerMessage emptyMsg;
	emptyMsg.type     = NoteManagerMessageType::ListAll;
	emptyMsg.dataJson = _T("{}");
	HandleListAll(emptyMsg);
	HandleListArchived(emptyMsg);
	HandleListTrash(emptyMsg);
}

void NoteManagerWindow::PostWebMessage(const CString& event, const CString& dataJson)
{
	if (m_webView == nullptr) return;

	CString msg;
	msg.Format(_T("{\"event\":\"%s\",\"data\":%s}"), event.GetString(), dataJson.IsEmpty() ? _T("null") : dataJson.GetString());
	m_webView->PostWebMessageAsJson(msg.GetString());
}

void NoteManagerWindow::InitWebView()
{
	if (m_controller != nullptr) return;

	HWND hWnd = this->GetSafeHwnd();
	WebViewEnvironmentManager::GetOrCreateEnvironment([this, hWnd](HRESULT result, ICoreWebView2Environment* environment) {
		if (result != S_OK || environment == nullptr)
		{
			CLogApp::Error(_T("NoteManagerWindow::InitWebView failed: hr=0x%08X"), result);
			return;
		}

		if (this->m_hWnd == NULL || !::IsWindow(this->m_hWnd))
		{
			return;
		}

		environment->CreateCoreWebView2Controller(hWnd,
			Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
				this, &NoteManagerWindow::OnCreateCoreWebView2ControllerCompleted).Get());
	});
}

HRESULT NoteManagerWindow::OnCreateCoreWebView2ControllerCompleted(HRESULT result, ICoreWebView2Controller* controller)
{
	if (controller != nullptr) {
		m_controller = controller;
		m_controller->get_CoreWebView2(&m_webView);
		m_controller->put_IsVisible(FALSE); // 初始化先隐藏，避免白色底色闪烁
	}
	if (!m_webView) return E_FAIL;

	Microsoft::WRL::ComPtr<ICoreWebView2Controller2> controller2;
	m_controller.As(&controller2);
	if (controller2) {
		COREWEBVIEW2_COLOR darkBg = {
			255,                              // A (fully opaque)
			NoteManagerConst::kDefaultBgR,   // R
			NoteManagerConst::kDefaultBgG,   // G
			NoteManagerConst::kDefaultBgB    // B  => #0d1117
		};
		controller2->put_DefaultBackgroundColor(darkBg);
	}

	wil::com_ptr<ICoreWebView2Settings> settings;
	m_webView->get_Settings(&settings);
	if (settings)
	{
		settings->put_IsScriptEnabled(TRUE);
		settings->put_AreDefaultScriptDialogsEnabled(TRUE);
		settings->put_IsWebMessageEnabled(TRUE);
#ifndef _DEBUG
		// Release: disable developer-facing browser features so end-users cannot
		// open DevTools (F12), inspect elements (right-click), or use browser
		// accelerator shortcuts (Ctrl+U, Ctrl+Shift+I, etc.).
		settings->put_AreDevToolsEnabled(FALSE);
		settings->put_AreDefaultContextMenusEnabled(FALSE);
		// AreBrowserAcceleratorKeysEnabled requires ICoreWebView2Settings3
		wil::com_ptr<ICoreWebView2Settings3> settings3;
		if (SUCCEEDED(settings->QueryInterface(IID_PPV_ARGS(&settings3))) && settings3)
		{
			settings3->put_AreBrowserAcceleratorKeysEnabled(FALSE);
		}
#endif
	}


	RECT bounds;
	::GetClientRect(GetSafeHwnd(), &bounds);
	m_controller->put_Bounds(bounds);

	EventRegistrationToken token;
	m_webView->add_NavigationCompleted(Microsoft::WRL::Callback<ICoreWebView2NavigationCompletedEventHandler>(
		this, &NoteManagerWindow::OnDocumentReady).Get(), &token);

	m_webView->add_WebMessageReceived(Microsoft::WRL::Callback<ICoreWebView2WebMessageReceivedEventHandler>(
		this, &NoteManagerWindow::OnWebMessageReceived).Get(), &token);

	// 注入管理器专属环境变量标记，确保前端首帧 100% 精准识别为管理器模式
	m_webView->AddScriptToExecuteOnDocumentCreated(
		L"window.__IS_NOTE_MANAGER__ = true;",
		nullptr
	);

	// 管理器窗口优先加载独立构建的 Manager 模块（themes/Manager），若未找到则回退至 Default
	CString sManagerFolder = Easy::Path::GetCurDirectory(_T("themes\\Manager"));
	if (!Easy::Path::Exists(Easy::Path::Resolve(sManagerFolder, _T("index.html")))) {
		sManagerFolder = Easy::Path::GetCurDirectory(_T("themes\\Default"));
	}
	if (!Easy::Path::Exists(Easy::Path::Resolve(sManagerFolder, _T("index.html")))) {
		AppSetting fallbackSetting = AppSettingStore::Load();
		sManagerFolder = Easy::Path::GetCurDirectory(_T("themes\\") + fallbackSetting.sTheme);
	}
	CLogApp::Write(_T("NoteManagerWindow: using manager folder=%s"), sManagerFolder.GetString());

	Microsoft::WRL::ComPtr<ICoreWebView2_3> webView3;
	HRESULT hr = m_webView.As(&webView3);

	// 使用毫秒级时间戳 + ?w=mgr 标记，双重保证：
	//   1. 每次打开管理器都绕过 WebView2 对 index.html 的 HTTP 缓存，加载最新版本前端代码
	//   2. 前端通过 window.location.search.includes('w=mgr') 可零误差同步识别管理器窗口
	ULONGLONG ts = ::GetTickCount64();
	CString sNavUrl;

	if (SUCCEEDED(hr) && webView3)
	{
		sManagerFolder.TrimRight(_T("\\/"));
		webView3->SetVirtualHostNameToFolderMapping(
			L"app.manager",
			sManagerFolder.GetString(),
			COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_ALLOW
		);
		sNavUrl.Format(_T("https://app.manager/index.html?w=mgr&t=%I64u"), ts);
		m_webView->Navigate(sNavUrl.GetString());
	}
	else
	{
		CString sTheme = Easy::Path::Resolve(sManagerFolder, _T("index.html"));
		sNavUrl.Format(_T("file:///%s?w=mgr&t=%I64u"), sTheme.GetString(), ts);
		sNavUrl.Replace(_T("\\"), _T("/"));
		m_webView->Navigate(sNavUrl.GetString());
	}

	return S_OK;
}

HRESULT NoteManagerWindow::OnWebMessageReceived(ICoreWebView2* webview, ICoreWebView2WebMessageReceivedEventArgs* args)
{
	wil::unique_cotaskmem_string raw;
	args->TryGetWebMessageAsString(&raw);
	m_dispatcher.Dispatch(raw.get());
	return S_OK;
}

HRESULT NoteManagerWindow::OnDocumentReady(ICoreWebView2* webview, ICoreWebView2NavigationCompletedEventArgs* args)
{
	if (m_controller)
	{
		m_controller->put_IsVisible(TRUE); // 内容就绪后呈现，消除白屏闪烁
	}
	// 内容完全就绪后再展示窗口（首次打开由后台转前台）
	if (!IsWindowVisible())
	{
		ShowWindow(SW_SHOW);
		::SetWindowPos(m_hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
		SetForegroundWindow();
	}
	else
	{
		// 已在显示状态下刷新时，确保处于非置顶层级且不强制夺焦
		::SetWindowPos(m_hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	}
	RefreshAllLists();
	return S_OK;
}

void NoteManagerWindow::RegisterHandlers()
{
	m_handlers.clear();

	auto reg = [this](NoteManagerMessageType type, std::function<void(const NoteManagerMessage&)> fn) {
		auto h = std::make_unique<LambdaManagerHandler>(std::move(fn));
		m_dispatcher.Register(type, h.get());
		m_handlers.push_back(std::move(h));
	};

	reg(NoteManagerMessageType::ListAll, [this](const NoteManagerMessage& msg) { HandleListAll(msg); });
	reg(NoteManagerMessageType::GetNote, [this](const NoteManagerMessage& msg) { HandleGetNote(msg); });
	reg(NoteManagerMessageType::CreateNote, [this](const NoteManagerMessage& msg) { HandleCreateNote(msg); });
	reg(NoteManagerMessageType::DeleteNotes, [this](const NoteManagerMessage& msg) { HandleDeleteNotes(msg); });
	reg(NoteManagerMessageType::ToggleVisible, [this](const NoteManagerMessage& msg) { HandleToggleVisible(msg); });
	reg(NoteManagerMessageType::UpdateNote, [this](const NoteManagerMessage& msg) { HandleUpdateNote(msg); });
	reg(NoteManagerMessageType::LocateWindow, [this](const NoteManagerMessage& msg) { HandleLocateWindow(msg); });
	reg(NoteManagerMessageType::Export, [this](const NoteManagerMessage& msg) { HandleExport(msg); });
	reg(NoteManagerMessageType::Move, [this](const NoteManagerMessage& msg) { HandleMove(msg); });
	reg(NoteManagerMessageType::Resize, [this](const NoteManagerMessage& msg) { HandleResize(msg); });
	reg(NoteManagerMessageType::GetAppSettings, [this](const NoteManagerMessage& msg) { HandleGetAppSettings(msg); });
	reg(NoteManagerMessageType::SaveAppSettings, [this](const NoteManagerMessage& msg) { HandleSaveAppSettings(msg); });
	reg(NoteManagerMessageType::BrowseFolder, [this](const NoteManagerMessage& msg) { HandleBrowseFolder(msg); });
	reg(NoteManagerMessageType::OpenFolder, [this](const NoteManagerMessage& msg) { HandleOpenFolder(msg); });
	reg(NoteManagerMessageType::ListTrash, [this](const NoteManagerMessage& msg) { HandleListTrash(msg); });
	reg(NoteManagerMessageType::RestoreNote, [this](const NoteManagerMessage& msg) { HandleRestoreNote(msg); });
	reg(NoteManagerMessageType::PermanentDelete, [this](const NoteManagerMessage& msg) { HandlePermanentDelete(msg); });
	reg(NoteManagerMessageType::ClearTrash, [this](const NoteManagerMessage& msg) { HandleClearTrash(msg); });
	reg(NoteManagerMessageType::ListArchived, [this](const NoteManagerMessage& msg) { HandleListArchived(msg); });
	reg(NoteManagerMessageType::ArchiveNote, [this](const NoteManagerMessage& msg) { HandleArchiveNote(msg); });
	reg(NoteManagerMessageType::UnarchiveNote, [this](const NoteManagerMessage& msg) { HandleUnarchiveNote(msg); });
	reg(NoteManagerMessageType::ExportDb, [this](const NoteManagerMessage& msg) { HandleExportDb(msg); });
	reg(NoteManagerMessageType::ImportDb, [this](const NoteManagerMessage& msg) { HandleImportDb(msg); });

	reg(NoteManagerMessageType::Close, [this](const NoteManagerMessage&) { ShowWindow(SW_HIDE); });
	reg(NoteManagerMessageType::Min, [this](const NoteManagerMessage&) {
		::PostMessage(m_hWnd, WM_SYSCOMMAND, SC_MINIMIZE, 0); // 触发 Windows 原生 DWM 任务栏缩放过渡动画
	});
	reg(NoteManagerMessageType::Max, [this](const NoteManagerMessage&) {
		::PostMessage(m_hWnd, WM_SYSCOMMAND, IsZoomed() ? SC_RESTORE : SC_MAXIMIZE, 0); // 触发 Windows 原生 DWM 最大化/还原过渡动画
	});
}

void NoteManagerWindow::HandleResize(const NoteManagerMessage& msg)
{
	rapidjson::GenericDocument<rapidjson::UTF16<TCHAR>> doc;
	doc.Parse(msg.dataJson.GetString());
	if (!doc.HasParseError() && doc.IsString())
	{
		CString dir = doc.GetString();
		WPARAM wparam = NoteManagerConst::kScSizeSE; // default: bottom-right
		if      (dir == _T("left"))         wparam = NoteManagerConst::kScSizeW;
		else if (dir == _T("right"))        wparam = NoteManagerConst::kScSizeE;
		else if (dir == _T("top"))          wparam = NoteManagerConst::kScSizeN;
		else if (dir == _T("top-left"))     wparam = NoteManagerConst::kScSizeNW;
		else if (dir == _T("top-right"))    wparam = NoteManagerConst::kScSizeNE;
		else if (dir == _T("bottom"))       wparam = NoteManagerConst::kScSizeS;
		else if (dir == _T("bottom-left"))  wparam = NoteManagerConst::kScSizeSW;
		else if (dir == _T("bottom-right")) wparam = NoteManagerConst::kScSizeSE;

		::ReleaseCapture();
		::PostMessage(m_hWnd, WM_SYSCOMMAND, wparam, 0);
	}
}

void NoteManagerWindow::HandleMove(const NoteManagerMessage& msg)
{
	rapidjson::GenericDocument<rapidjson::UTF16<TCHAR>> doc;
	doc.Parse(msg.dataJson.GetString());
	if (!doc.HasParseError() && doc.IsBool())
	{
		bool bMove = doc.GetBool();
		if (bMove)
		{
			::ReleaseCapture();
			::PostMessage(m_hWnd, WM_SYSCOMMAND, NoteManagerConst::kScMove, 0);
		}
	}
}


void NoteManagerWindow::HandleListAll(const NoteManagerMessage& msg)
{
	CString keyword;
	CString filter;

	if (!msg.dataJson.IsEmpty())
	{
		RJDoc doc;
		doc.Parse(msg.dataJson.GetString());
		if (!doc.HasParseError() && doc.IsObject())
		{
			if (doc.HasMember(_T("keyword")) && doc[_T("keyword")].IsString())
				keyword = doc[_T("keyword")].GetString();
			if (doc.HasMember(_T("filter")) && doc[_T("filter")].IsString())
				filter = doc[_T("filter")].GetString();
		}
	}

	std::vector<Note> notes = keyword.IsEmpty()
		? m_repo.LoadAllNotes()
		: m_repo.SearchByContent(keyword);

	RJDoc docOut;
	auto& alloc = docOut.GetAllocator();
	RJValue vArr(rapidjson::kArrayType);

	for (const auto& note : notes)
	{
		// Apply client-side filter (active/hidden/pending/completed)
		if (!filter.IsEmpty())
		{
			int pendingCount = 0;
			for (const auto& item : note.items)
				if (!item.bFinished) ++pendingCount;

			if (filter == _T("active")    && !note.visible)                          continue;
			if (filter == _T("hidden")    &&  note.visible)                          continue;
			if (filter == _T("pending")   && pendingCount == 0)                      continue;
			if (filter == _T("completed") && (pendingCount > 0 || note.items.empty())) continue;
		}

		vArr.PushBack(NoteDto::Summary(note, alloc).Move(), alloc);
	}

	rapidjson::GenericStringBuffer<rapidjson::UTF16<TCHAR>> buffer;
	rapidjson::Writer<rapidjson::GenericStringBuffer<rapidjson::UTF16<TCHAR>>,
	                  rapidjson::UTF16<TCHAR>, rapidjson::UTF16<TCHAR>> writer(buffer);
	vArr.Accept(writer);

	PostWebMessage(_T("mgr_note_list"), buffer.GetString());
}


void NoteManagerWindow::HandleGetNote(const NoteManagerMessage& msg)
{
	RJDoc doc;
	doc.Parse(msg.dataJson.GetString());
	if (doc.HasParseError() || !doc.IsObject() || !doc.HasMember(_T("name"))) return;

	CString name = doc[_T("name")].GetString();
	Note note;
	if (!m_repo.Load(name, note)) return;

	RJDoc docOut;
	auto& alloc = docOut.GetAllocator();
	RJValue vNote = NoteDto::Detail(note, alloc);

	rapidjson::GenericStringBuffer<rapidjson::UTF16<TCHAR>> buffer;
	rapidjson::Writer<rapidjson::GenericStringBuffer<rapidjson::UTF16<TCHAR>>,
	                  rapidjson::UTF16<TCHAR>, rapidjson::UTF16<TCHAR>> writer(buffer);
	vNote.Accept(writer);

	PostWebMessage(_T("mgr_note_detail"), buffer.GetString());
}


void NoteManagerWindow::HandleCreateNote(const NoteManagerMessage& msg)
{
	CString title = _T("新建便签");
	CString hexColor = AppSettingStore::Load().sDefaultBgColor;
	if (hexColor.IsEmpty()) hexColor = NoteManagerConst::kDefaultBgColor;

	if (!msg.dataJson.IsEmpty())
	{
		rapidjson::GenericDocument<rapidjson::UTF16<TCHAR>> doc;
		doc.Parse(msg.dataJson.GetString());
		if (!doc.HasParseError() && doc.IsObject())
		{
			if (doc.HasMember(_T("title")) && doc[_T("title")].IsString())
				title = doc[_T("title")].GetString();
			if (doc.HasMember(_T("color")) && doc[_T("color")].IsString() && CString(doc[_T("color")].GetString()).GetLength() > 0)
				hexColor = doc[_T("color")].GetString();
		}
	}

	Note note = Note::Create();
	note.title = title;
	note.bgColor = Cvt::ToColor(hexColor);
	note.visible = true;
	m_repo.Save(note);

	// 立即在桌面上实例化并显示新建的便签窗口
	NoteHostWindow* pWin = m_manager.New(note.name);
	if (pWin && pWin->GetSafeHwnd() && ::IsWindow(pWin->GetSafeHwnd()))
	{
		pWin->ShowWindow(SW_SHOW);
		pWin->SetForegroundWindow();
	}

	BroadcastDataChanged(_T("create"), note.name);

	RefreshAllLists();
}

void NoteManagerWindow::HandleDeleteNotes(const NoteManagerMessage& msg)
{
	rapidjson::GenericDocument<rapidjson::UTF16<TCHAR>> doc;
	doc.Parse(msg.dataJson.GetString());
	if (doc.HasParseError() || !doc.IsObject() || !doc.HasMember(_T("names")) || !doc[_T("names")].IsArray()) return;

	const auto& namesArr = doc[_T("names")];
	for (rapidjson::SizeType i = 0; i < namesArr.Size(); ++i)
	{
		if (namesArr[i].IsString())
		{
			CString name = namesArr[i].GetString();
			m_manager.CloseNoteWindow(name);
			m_repo.Delete(name);
		}
	}

	BroadcastDataChanged(_T("delete"), _T(""));

	RefreshAllLists();
}

void NoteManagerWindow::HandleListTrash(const NoteManagerMessage& msg)
{
	std::vector<Note> notes = m_repo.LoadTrashNotes();

	RJDoc docOut;
	auto& alloc = docOut.GetAllocator();
	RJValue vArr(rapidjson::kArrayType);
	for (const auto& note : notes)
		vArr.PushBack(NoteDto::Summary(note, alloc).Move(), alloc);

	rapidjson::GenericStringBuffer<rapidjson::UTF16<TCHAR>> buffer;
	rapidjson::Writer<rapidjson::GenericStringBuffer<rapidjson::UTF16<TCHAR>>,
	                  rapidjson::UTF16<TCHAR>, rapidjson::UTF16<TCHAR>> writer(buffer);
	vArr.Accept(writer);
	PostWebMessage(_T("mgr_trash_list"), buffer.GetString());
}

void NoteManagerWindow::HandleListArchived(const NoteManagerMessage& msg)
{
	std::vector<Note> notes = m_repo.LoadArchivedNotes();

	RJDoc docOut;
	auto& alloc = docOut.GetAllocator();
	RJValue vArr(rapidjson::kArrayType);
	for (const auto& note : notes)
		vArr.PushBack(NoteDto::Summary(note, alloc).Move(), alloc);

	rapidjson::GenericStringBuffer<rapidjson::UTF16<TCHAR>> buffer;
	rapidjson::Writer<rapidjson::GenericStringBuffer<rapidjson::UTF16<TCHAR>>,
	                  rapidjson::UTF16<TCHAR>, rapidjson::UTF16<TCHAR>> writer(buffer);
	vArr.Accept(writer);
	PostWebMessage(_T("mgr_archived_list"), buffer.GetString());
}


std::vector<CString> NoteManagerWindow::ExtractNoteNames(const NoteManagerMessage& msg)
{
	std::vector<CString> names;
	RJDoc doc;
	doc.Parse(msg.dataJson.GetString());
	if (!doc.HasParseError() && doc.IsObject())
	{
		if (doc.HasMember(_T("names")) && doc[_T("names")].IsArray())
		{
			const auto& namesArr = doc[_T("names")];
			for (rapidjson::SizeType i = 0; i < namesArr.Size(); ++i)
			{
				if (namesArr[i].IsString())
				{
					names.push_back(namesArr[i].GetString());
				}
			}
		}
		else if (doc.HasMember(_T("name")) && doc[_T("name")].IsString())
		{
			names.push_back(doc[_T("name")].GetString());
		}
	}
	return names;
}

void NoteManagerWindow::HandleArchiveNote(const NoteManagerMessage& msg)
{
	for (const auto& name : ExtractNoteNames(msg))
	{
		m_manager.CloseNoteWindow(name);
		m_repo.Archive(name);
	}
	BroadcastDataChanged(_T("archive"), _T(""));
	RefreshAllLists();
}

void NoteManagerWindow::HandleUnarchiveNote(const NoteManagerMessage& msg)
{
	for (const auto& name : ExtractNoteNames(msg))
	{
		m_repo.Unarchive(name);
	}
	BroadcastDataChanged(_T("unarchive"), _T(""));
	RefreshAllLists();
}

void NoteManagerWindow::HandleRestoreNote(const NoteManagerMessage& msg)
{
	for (const auto& name : ExtractNoteNames(msg))
	{
		m_repo.Restore(name);
	}
	BroadcastDataChanged(_T("restore"), _T(""));
	RefreshAllLists();
}

void NoteManagerWindow::HandlePermanentDelete(const NoteManagerMessage& msg)
{
	for (const auto& name : ExtractNoteNames(msg))
	{
		m_manager.CloseNoteWindow(name);
		m_repo.PermanentDelete(name);
	}
	BroadcastDataChanged(_T("permanent_delete"), _T(""));
	RefreshAllLists();
}

void NoteManagerWindow::HandleClearTrash(const NoteManagerMessage& msg)
{
	m_repo.ClearTrash();

	BroadcastDataChanged(_T("clear_trash"), _T(""));

	RefreshAllLists();
}

void NoteManagerWindow::HandleToggleVisible(const NoteManagerMessage& msg)
{
	rapidjson::GenericDocument<rapidjson::UTF16<TCHAR>> doc;
	doc.Parse(msg.dataJson.GetString());
	if (doc.HasParseError() || !doc.IsObject()) return;

	bool visible = true;
	if (doc.HasMember(_T("visible")) && doc[_T("visible")].IsBool())
		visible = doc[_T("visible")].GetBool();

	if (doc.HasMember(_T("names")) && doc[_T("names")].IsArray())
	{
		const auto& namesArr = doc[_T("names")];
		for (rapidjson::SizeType i = 0; i < namesArr.Size(); ++i)
		{
			if (namesArr[i].IsString())
			{
				CString name = namesArr[i].GetString();
				m_manager.SetNoteVisible(name, visible);
			}
		}
	}

	BroadcastDataChanged(_T("update"), _T(""));

	RefreshAllLists();
}

void NoteManagerWindow::HandleUpdateNote(const NoteManagerMessage& msg)
{
	rapidjson::GenericDocument<rapidjson::UTF16<TCHAR>> doc;
	doc.Parse(msg.dataJson.GetString());
	if (doc.HasParseError() || !doc.IsObject() || !doc.HasMember(_T("name"))) return;

	CString name = doc[_T("name")].GetString();
	Note note;
	if (!m_repo.Load(name, note)) return;

	if (doc.HasMember(_T("title")) && doc[_T("title")].IsString())
		note.title = doc[_T("title")].GetString();

	if (doc.HasMember(_T("bgColor")) && doc[_T("bgColor")].IsString())
		note.bgColor = Cvt::ToColor(doc[_T("bgColor")].GetString());

	if (doc.HasMember(_T("opacity")) && doc[_T("opacity")].IsInt())
		note.opacity = doc[_T("opacity")].GetInt();

	if (doc.HasMember(_T("opacityEnabled")) && doc[_T("opacityEnabled")].IsBool())
		note.opacityEnabled = doc[_T("opacityEnabled")].GetBool();

	if (doc.HasMember(_T("visible")) && doc[_T("visible")].IsBool())
		note.visible = doc[_T("visible")].GetBool();

	if (doc.HasMember(_T("topMost")) && doc[_T("topMost")].IsBool())
		note.topMost = doc[_T("topMost")].GetBool();

	if (doc.HasMember(_T("items")) && doc[_T("items")].IsArray())
	{
		note.items.clear();
		const auto& arr = doc[_T("items")];
		for (rapidjson::SizeType i = 0; i < arr.Size(); ++i)
		{
			const auto& it = arr[i];
			uint64_t id = 0;
			CString content = _T("");
			bool finished = false;
			if (it.HasMember(_T("id")))
			{
				if (it[_T("id")].IsUint64()) id = it[_T("id")].GetUint64();
				else if (it[_T("id")].IsInt64()) id = static_cast<uint64_t>(it[_T("id")].GetInt64());
				else if (it[_T("id")].IsUint()) id = static_cast<uint64_t>(it[_T("id")].GetUint());
				else if (it[_T("id")].IsInt()) id = static_cast<uint64_t>(it[_T("id")].GetInt());
			}
			if (it.HasMember(_T("content")) && it[_T("content")].IsString())
				content = it[_T("content")].GetString();
			if (it.HasMember(_T("finished")) && it[_T("finished")].IsBool())
				finished = it[_T("finished")].GetBool();
			if (id == 0)
			{
				id = static_cast<uint64_t>(GetTickCount64());
			}
			note.items.push_back(NoteItem(id, content, finished));
		}
	}

	m_repo.Save(note);
	m_manager.SyncActiveNoteWindow(note);
	BroadcastDataChanged(_T("update"), note.name);

	HandleGetNote(msg);
}

void NoteManagerWindow::HandleLocateWindow(const NoteManagerMessage& msg)
{
	rapidjson::GenericDocument<rapidjson::UTF16<TCHAR>> doc;
	doc.Parse(msg.dataJson.GetString());
	if (doc.HasParseError() || !doc.IsObject() || !doc.HasMember(_T("name"))) return;

	CString name = doc[_T("name")].GetString();
	m_manager.LocateNoteWindow(name);
}

void NoteManagerWindow::HandleExport(const NoteManagerMessage& msg)
{
	rapidjson::GenericDocument<rapidjson::UTF16<TCHAR>> doc;
	doc.Parse(msg.dataJson.GetString());
	if (doc.HasParseError() || !doc.IsObject()) return;

	CString format = _T("md");
	if (doc.HasMember(_T("format")) && doc[_T("format")].IsString())
		format = doc[_T("format")].GetString();

	std::vector<CString> names;
	if (doc.HasMember(_T("names")) && doc[_T("names")].IsArray())
	{
		const auto& arr = doc[_T("names")];
		for (rapidjson::SizeType i = 0; i < arr.Size(); ++i)
		{
			if (arr[i].IsString()) names.push_back(arr[i].GetString());
		}
	}
	else
	{
		names = m_repo.ListAll();
	}

	CString outputText = _T("");
	if (format.CompareNoCase(_T("json")) == 0)
	{
		RJDoc docOut;
		auto& alloc = docOut.GetAllocator();
		RJValue vArr(rapidjson::kArrayType);
		for (const auto& name : names)
		{
			Note n;
			if (m_repo.Load(name, n))
			{
				vArr.PushBack(NoteDto::Detail(n, alloc).Move(), alloc);
			}
		}
		outputText = NoteDto::Stringify(vArr);
	}
	else
	{
		for (const auto& name : names)
		{
			Note n;
			if (m_repo.Load(name, n))
			{
				outputText += CString(_T("# ")) + (n.title.IsEmpty() ? CString(_T("未命名便签")) : n.title) + CString(_T("\r\n\r\n"));
				for (const auto& it : n.items)
				{
					outputText += (it.bFinished ? CString(_T("- [x] ")) : CString(_T("- [ ] "))) + it.sContent + CString(_T("\r\n"));
				}
				outputText += CString(_T("\r\n---\r\n\r\n"));
			}
		}
	}

	rapidjson::GenericDocument<rapidjson::UTF16<TCHAR>> docResult;
	auto& alloc = docResult.GetAllocator();
	rapidjson::GenericValue<rapidjson::UTF16<TCHAR>> vRes(rapidjson::kObjectType);
	vRes.AddMember(rapidjson::StringRef(_T("success")), true, alloc);
	vRes.AddMember(rapidjson::StringRef(_T("format")), rapidjson::GenericValue<rapidjson::UTF16<TCHAR>>(format.GetString(), alloc).Move(), alloc);
	vRes.AddMember(rapidjson::StringRef(_T("content")), rapidjson::GenericValue<rapidjson::UTF16<TCHAR>>(outputText.GetString(), alloc).Move(), alloc);

	rapidjson::GenericStringBuffer<rapidjson::UTF16<TCHAR>> buffer;
	rapidjson::Writer<rapidjson::GenericStringBuffer<rapidjson::UTF16<TCHAR>>, rapidjson::UTF16<TCHAR>, rapidjson::UTF16<TCHAR>> writer(buffer);
	vRes.Accept(writer);

	PostWebMessage(_T("mgr_export_result"), buffer.GetString());
}

void NoteManagerWindow::HandleGetAppSettings(const NoteManagerMessage& msg)
{
	AppSetting setting = m_manager.GetSettings();
	std::vector<CString> themes = AppSettingStore::SearchThemes();
	auto ver = Easy::Utility::GetVersion(Easy::Path::GetProgramPath());
	CString sVersion = ver.ToString();

	rapidjson::GenericDocument<rapidjson::UTF16<TCHAR>> doc;
	doc.SetObject();
	auto& alloc = doc.GetAllocator();

	doc.AddMember(rapidjson::StringRef(_T("autoRun")), setting.bAutoRun, alloc);
	doc.AddMember(rapidjson::StringRef(_T("customWebview2")), setting.bCustomWebview2, alloc);
	doc.AddMember(rapidjson::StringRef(_T("webview2Path")), rapidjson::GenericValue<rapidjson::UTF16<TCHAR>>(setting.sWebview2Path.GetString(), alloc).Move(), alloc);
	doc.AddMember(rapidjson::StringRef(_T("noteDir")), rapidjson::GenericValue<rapidjson::UTF16<TCHAR>>(setting.sNoteDir.GetString(), alloc).Move(), alloc);
	doc.AddMember(rapidjson::StringRef(_T("theme")), rapidjson::GenericValue<rapidjson::UTF16<TCHAR>>(setting.sTheme.GetString(), alloc).Move(), alloc);
	doc.AddMember(rapidjson::StringRef(_T("defaultBgColor")), rapidjson::GenericValue<rapidjson::UTF16<TCHAR>>(setting.sDefaultBgColor.GetString(), alloc).Move(), alloc);
	doc.AddMember(rapidjson::StringRef(_T("language")), rapidjson::GenericValue<rapidjson::UTF16<TCHAR>>(setting.sLanguage.GetString(), alloc).Move(), alloc);
	doc.AddMember(rapidjson::StringRef(_T("appVersion")), rapidjson::GenericValue<rapidjson::UTF16<TCHAR>>(sVersion.GetString(), alloc).Move(), alloc);

	// Themes list
	rapidjson::GenericValue<rapidjson::UTF16<TCHAR>> vThemes(rapidjson::kArrayType);
	for (const auto& t : themes)
	{
		vThemes.PushBack(rapidjson::GenericValue<rapidjson::UTF16<TCHAR>>(t.GetString(), alloc).Move(), alloc);
	}
	doc.AddMember(rapidjson::StringRef(_T("themes")), vThemes.Move(), alloc);

	// Hotkeys list with DWORD value and human readable label
	rapidjson::GenericValue<rapidjson::UTF16<TCHAR>> vHotkeys(rapidjson::kObjectType);

	auto addHk = [&](const wchar_t* keyName, DWORD dwVal) {
		rapidjson::GenericValue<rapidjson::UTF16<TCHAR>> vHk(rapidjson::kObjectType);
		vHk.AddMember(rapidjson::StringRef(_T("value")), static_cast<uint32_t>(dwVal), alloc);
		CString sName = Easy::CHotKey::GetHotKeyName(dwVal);
		vHk.AddMember(rapidjson::StringRef(_T("text")), rapidjson::GenericValue<rapidjson::UTF16<TCHAR>>(sName.GetString(), alloc).Move(), alloc);
		vHotkeys.AddMember(rapidjson::GenericValue<rapidjson::UTF16<TCHAR>>(keyName, alloc).Move(), vHk.Move(), alloc);
	};

	addHk(_T("newNote"), setting.dwNewHotKey);
	addHk(_T("editNote"), setting.dwEditHotKey);
	addHk(_T("hideAll"), setting.dwUnActiveHotKey);
	addHk(_T("showAll"), setting.dwActiveAllHotKey);
	doc.AddMember(rapidjson::StringRef(_T("hotkeys")), vHotkeys.Move(), alloc);

	rapidjson::GenericStringBuffer<rapidjson::UTF16<TCHAR>> buffer;
	rapidjson::Writer<rapidjson::GenericStringBuffer<rapidjson::UTF16<TCHAR>>, rapidjson::UTF16<TCHAR>, rapidjson::UTF16<TCHAR>> writer(buffer);
	doc.Accept(writer);

	PostWebMessage(_T("mgr_app_settings"), buffer.GetString());
}

void NoteManagerWindow::HandleSaveAppSettings(const NoteManagerMessage& msg)
{
	AppSetting newSetting = AppSetting::FromJson(msg.dataJson);

	if (newSetting.sTheme.IsEmpty())
	{
		newSetting.sTheme = _T("Default");
	}
	if (newSetting.sDefaultBgColor.IsEmpty())
	{
		newSetting.sDefaultBgColor = NoteManagerConst::kDefaultBgColor;
	}
	if (newSetting.sLanguage.IsEmpty())
	{
		newSetting.sLanguage = _T("zh-CN");
	}

	bool isEn = (newSetting.sLanguage.CompareNoCase(_T("en-US")) == 0);
	SetWindowText(isEn ? _T("Stickify Manager") : _T("Sticky Notes Manager"));

	m_manager.ApplySettings(newSetting);

	rapidjson::GenericDocument<rapidjson::UTF16<TCHAR>> docResult;
	auto& alloc = docResult.GetAllocator();
	rapidjson::GenericValue<rapidjson::UTF16<TCHAR>> vRes(rapidjson::kObjectType);
	vRes.AddMember(rapidjson::StringRef(_T("success")), true, alloc);
	const wchar_t* pSuccessMsg = isEn ? _T("Settings saved successfully and applied immediately") : _T("设置已成功保存并即时生效");
	vRes.AddMember(rapidjson::StringRef(_T("message")), rapidjson::GenericValue<rapidjson::UTF16<TCHAR>>(pSuccessMsg, alloc).Move(), alloc);

	rapidjson::GenericStringBuffer<rapidjson::UTF16<TCHAR>> buffer;
	rapidjson::Writer<rapidjson::GenericStringBuffer<rapidjson::UTF16<TCHAR>>, rapidjson::UTF16<TCHAR>, rapidjson::UTF16<TCHAR>> writer(buffer);
	vRes.Accept(writer);

	PostWebMessage(_T("mgr_save_settings_result"), buffer.GetString());

	HandleGetAppSettings(msg);
}

void NoteManagerWindow::HandleBrowseFolder(const NoteManagerMessage& msg)
{
	CString currentDir = _T("");
	if (!msg.dataJson.IsEmpty())
	{
		rapidjson::GenericDocument<rapidjson::UTF16<TCHAR>> doc;
		doc.Parse(msg.dataJson.GetString());
		if (!doc.HasParseError() && doc.IsObject() && doc.HasMember(_T("defaultPath")) && doc[_T("defaultPath")].IsString())
		{
			currentDir = doc[_T("defaultPath")].GetString();
		}
	}

	CString selected = m_manager.BrowseFolder(m_hWnd, currentDir);
	if (!selected.IsEmpty())
	{
		rapidjson::GenericDocument<rapidjson::UTF16<TCHAR>> docResult;
		auto& alloc = docResult.GetAllocator();
		rapidjson::GenericValue<rapidjson::UTF16<TCHAR>> vRes(rapidjson::kObjectType);
		vRes.AddMember(rapidjson::StringRef(_T("folder")), rapidjson::GenericValue<rapidjson::UTF16<TCHAR>>(selected.GetString(), alloc).Move(), alloc);

		rapidjson::GenericStringBuffer<rapidjson::UTF16<TCHAR>> buffer;
		rapidjson::Writer<rapidjson::GenericStringBuffer<rapidjson::UTF16<TCHAR>>, rapidjson::UTF16<TCHAR>, rapidjson::UTF16<TCHAR>> writer(buffer);
		vRes.Accept(writer);

		PostWebMessage(_T("mgr_folder_selected"), buffer.GetString());
	}
}

void NoteManagerWindow::HandleOpenFolder(const NoteManagerMessage& msg)
{
	CString folderPath = _T("");
	if (!msg.dataJson.IsEmpty())
	{
		rapidjson::GenericDocument<rapidjson::UTF16<TCHAR>> doc;
		doc.Parse(msg.dataJson.GetString());
		if (!doc.HasParseError() && doc.IsObject() && doc.HasMember(_T("folder")) && doc[_T("folder")].IsString())
		{
			folderPath = doc[_T("folder")].GetString();
		}
	}
	m_manager.OpenFolder(folderPath);
}

void NoteManagerWindow::HandleExportDb(const NoteManagerMessage&)
{
	CString path = m_manager.ExportDatabaseDialog(m_hWnd);
	bool success = !path.IsEmpty();

	rapidjson::GenericDocument<rapidjson::UTF16<TCHAR>> docResult;
	auto& alloc = docResult.GetAllocator();
	rapidjson::GenericValue<rapidjson::UTF16<TCHAR>> vRes(rapidjson::kObjectType);
	vRes.AddMember(rapidjson::StringRef(_T("success")), success, alloc);
	if (success)
	{
		vRes.AddMember(rapidjson::StringRef(_T("path")), rapidjson::GenericValue<rapidjson::UTF16<TCHAR>>(path.GetString(), alloc).Move(), alloc);
	}

	rapidjson::GenericStringBuffer<rapidjson::UTF16<TCHAR>> buffer;
	rapidjson::Writer<rapidjson::GenericStringBuffer<rapidjson::UTF16<TCHAR>>, rapidjson::UTF16<TCHAR>, rapidjson::UTF16<TCHAR>> writer(buffer);
	vRes.Accept(writer);

	PostWebMessage(_T("mgr_export_db_result"), buffer.GetString());
}

void NoteManagerWindow::HandleImportDb(const NoteManagerMessage&)
{
	int noteCount = 0;
	bool success = m_manager.ImportDatabaseDialog(m_hWnd, noteCount);

	rapidjson::GenericDocument<rapidjson::UTF16<TCHAR>> docResult;
	auto& alloc = docResult.GetAllocator();
	rapidjson::GenericValue<rapidjson::UTF16<TCHAR>> vRes(rapidjson::kObjectType);
	vRes.AddMember(rapidjson::StringRef(_T("success")), success, alloc);
	vRes.AddMember(rapidjson::StringRef(_T("count")), noteCount, alloc);

	rapidjson::GenericStringBuffer<rapidjson::UTF16<TCHAR>> buffer;
	rapidjson::Writer<rapidjson::GenericStringBuffer<rapidjson::UTF16<TCHAR>>, rapidjson::UTF16<TCHAR>, rapidjson::UTF16<TCHAR>> writer(buffer);
	vRes.Accept(writer);

	PostWebMessage(_T("mgr_import_db_result"), buffer.GetString());

	if (success)
	{
		BroadcastDataChanged(_T("import_db"), _T(""));
	}
}


