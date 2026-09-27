// ------------------------------------------------------------------
// Copyright (c) Sticky Notes Refactoring Team (2026)
// Part of Sticky Notes Refactoring Project
// Licensed under the Apache License, Version 2.0
// ------------------------------------------------------------------
#include "stdafx.h"
#include "host/NoteHostWindow.h"
#include "host/NoteDto.h"
#include "host/WebViewEnvironmentManager.h"
#include "infra/AppSettingStore.h"
#include "afxdialogex.h"
#include <functional>
#include <string>
#include <vector>
#include <memory>

// NoteHostWindow dialog

IMPLEMENT_DYNAMIC(NoteHostWindow, CDialogEx)

NoteHostWindow::NoteHostWindow(NoteService& service,
                               INoteRepository& repo,
                               CWnd* pParent /*= nullptr*/)
	: CDialogEx(NoteHostWindow::IDD, pParent)
	, m_service(service)
	, m_repo(repo)
	, m_bMoveWindow(false)
	, m_bMouseThrough(false)
{
	m_brush.CreateStockObject(NULL_BRUSH);
}

NoteHostWindow::~NoteHostWindow()
{
	// 关键防崩：析构阶段清空回调，严禁在外部 vector/容器迭代销毁过程中重入触发 OnWindowClosed/erase
	m_onClosed = nullptr;
	if (m_hWnd != NULL && ::IsWindow(m_hWnd))
	{
		DestroyWindow();
	}
}

void NoteHostWindow::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(NoteHostWindow, CDialogEx)
	ON_WM_SIZE()
	ON_WM_NCHITTEST()
	ON_WM_NCMOUSEMOVE()
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_INPUT()
	ON_WM_MOVE()
	ON_WM_GETMINMAXINFO()
	ON_WM_WINDOWPOSCHANGING()
	ON_WM_WINDOWPOSCHANGED()
	ON_WM_SIZING()
	ON_WM_MOVING()
	ON_WM_NCCALCSIZE()
	ON_WM_EXITSIZEMOVE()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSELEAVE()
	ON_WM_TIMER()
	ON_WM_SHOWWINDOW()
	ON_WM_CLOSE()
	ON_WM_DESTROY()
	ON_MESSAGE(WM_DISPLAYCHANGE, OnDisplayChange)
	ON_MESSAGE(WM_DPICHANGED, OnDpiChanged)
	ON_WM_SETTINGCHANGE()
END_MESSAGE_MAP()

void NoteHostWindow::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CDialogEx::OnShowWindow(bShow, nStatus);
	if (m_controller != nullptr)
	{
		// 性能关键优化：隐藏时通知 WebView2 暂停 DOM/Timer 循环并释放合成表面，显存与 CPU 占用降至最低
		m_controller->put_IsVisible(bShow);
	}
	if (bShow)
	{
		// 重新显示便签时，重置贴边缓冲期与展开状态，杜绝由于离开计数未重置导致的瞬时缩回/抽搐
		m_nStartupWarmupTicks = 40; // 给予 1 秒缓冲期
		m_nLeaveCount = 0;
		m_nEdgeHiddenTicks = 0;
		if (m_bEdgeHidden)
		{
			RestoreFromEdge();
		}
		else
		{
			UpdateEdgeDockRegion(false);
		}
	}
	// 窗口显隐状态改变时同步刷新定时器：仅贴边且可见时才需要高频检测
	UpdateEdgeTimer();
}

void NoteHostWindow::SetOnClosedCallback(std::function<void(NoteHostWindow*)> cb)
{
	m_onClosed = std::move(cb);
}

void NoteHostWindow::OnClose()
{
	OnCancel();
}

void NoteHostWindow::OnDestroy()
{
	KillTimer(1002);
	CRawInput::Remove(GetSafeHwnd(), RAW_TYPE_MS);
	UpdateEdgeDockRegion(false);
	// 规范 COM 释放顺序：先释放子接口引用 m_webView，再关闭并释放宿主控制器 m_controller
	m_webView = nullptr;
	if (m_controller != nullptr)
	{
		m_controller->Close();
		m_controller = nullptr;
	}
	auto cb = m_onClosed;
	m_onClosed = nullptr; // 取出后立即置空，杜绝重复调用
	if (cb)
	{
		cb(this);
	}
	CDialogEx::OnDestroy();
}

void NoteHostWindow::OnCancel()
{
	if (m_hWnd != NULL && ::IsWindow(m_hWnd))
	{
		DestroyWindow(); // DestroyWindow 内部会触发 OnDestroy 并安全派发回调
	}
	else
	{
		auto cb = m_onClosed;
		m_onClosed = nullptr;
		if (cb)
		{
			cb(this);
		}
	}
}

bool NoteHostWindow::Init(const CString& noteName)
{
	CLogApp::Write(_T("NoteHostWindow::Init: noteName='%s'"), noteName.GetString());
	if (!m_repo.Load(noteName, m_note))
	{
		m_note = Note::Create(noteName);
		CString defaultColorHex = AppSettingStore::Load().sDefaultBgColor;
		if (!defaultColorHex.IsEmpty())
		{
			m_note.bgColor = Easy::Cvt::ToColor(defaultColorHex);
		}
		m_repo.Save(m_note);
	}

	return true;
}

const Note& NoteHostWindow::GetNote() const
{
	return m_note;
}

void NoteHostWindow::UpdateNoteFromExternal(const Note& updatedNote)
{
	m_note = updatedNote;
	float alpha = m_note.opacityEnabled ? static_cast<float>(m_note.opacity) : 100.0f;
	SetWindowAlpha(alpha);
	if (m_controller)
	{
		m_controller->put_IsVisible(m_note.visible ? TRUE : FALSE);
	}
	if (m_note.visible)
	{
		m_nStartupWarmupTicks = 40;
		m_nLeaveCount = 0;
		m_nEdgeHiddenTicks = 0;
		if (m_bEdgeHidden)
		{
			RestoreFromEdge();
		}
	}
	if (m_hWnd != NULL && ::IsWindow(m_hWnd))
	{
		ApplyTopMost(m_note.topMost);
		SetWindowPos(m_note.topMost ? &wndTopMost : &wndNoTopMost,
			m_note.rect.left, m_note.rect.top,
			m_note.rect.Width(), m_note.rect.Height(),
			SWP_NOACTIVATE | (m_note.visible ? SWP_SHOWWINDOW : SWP_HIDEWINDOW));
	}
	if (m_webView != nullptr)
	{
		PushNoteItems();
		PushNoteSetting();
	}
}

// ------------------------------------------------------------------
// 便签实际 UI 组成结构与物理内容尺寸基准推导：
// 1. 宽度构成（Content-Based Width Metric）：
//    - 标题文本最小可读展示区：80px
//    - 顶部右侧3组操作按钮与间隙（主题/穿透/关闭）：84px
//    - 左右边距与安全空隙（Padding）：20px
//    - 物理窗口边框与内嵌补偿（Border Inset）：6px
//    => 基准内容宽度 BASE_MIN_WIDTH = 80 + 84 + 20 + 6 = 190px (取整标准 196px)
//
// 2. 高度构成（Content-Based Height Metric）：
//    - 头部平铺导航栏固定高度：38px
//    - 中间列表最小可见高度（至少完整呈现1条事项或空提示）：36px
//    - 底部快捷输入栏固定高度：38px
//    - 底部已完成/清空状态栏高度：24px
//    - 上下物理边框内嵌补偿：6px
//    => 基准内容高度 BASE_MIN_HEIGHT = 38 + 36 + 38 + 24 + 6 = 142px
// ------------------------------------------------------------------
static CSize CalculateContentBasedMinSize(HWND hWnd)
{
	constexpr int BASE_MIN_WIDTH = 196;
	constexpr int BASE_MIN_HEIGHT = 142;

	UINT dpi = 96;
	HMODULE hUser32 = GetModuleHandle(_T("user32.dll"));
	if (hUser32 != NULL)
	{
		typedef UINT(WINAPI* PFN_GetDpiForWindow)(HWND);
		auto pfnGetDpiForWindow = reinterpret_cast<PFN_GetDpiForWindow>(GetProcAddress(hUser32, "GetDpiForWindow"));
		if (pfnGetDpiForWindow && hWnd != NULL)
		{
			dpi = pfnGetDpiForWindow(hWnd);
		}
		else
		{
			HDC hdc = ::GetDC(hWnd);
			if (hdc)
			{
				dpi = ::GetDeviceCaps(hdc, LOGPIXELSX);
				::ReleaseDC(hWnd, hdc);
			}
		}
	}

	int scaledWidth = ::MulDiv(BASE_MIN_WIDTH, static_cast<int>(dpi), 96);
	int scaledHeight = ::MulDiv(BASE_MIN_HEIGHT, static_cast<int>(dpi), 96);

	return CSize(scaledWidth, scaledHeight);
}

static CRect SnapRectToWorkArea(const CRect& proposedRect, HWND hWnd);
static CRect EnsureRectVisibleOnScreen(const CRect& proposedRect, HWND hWnd);

BOOL NoteHostWindow::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 开启 WS_CLIPCHILDREN，杜绝闪烁与重绘干扰
	ModifyStyle(0, WS_CLIPCHILDREN);

	RegisterHandlers();

	// 启动防闪烁核心机制：初始化时将 Alpha 置为 0 (全透明)，彻底杜绝 WebView2 尚未就绪时 Win32 原生对话框的白/黑屏闪烁
	SetWindowAlpha(0.0f);

	CSize minSize = CalculateContentBasedMinSize(m_hWnd);
	int w = max(minSize.cx, m_note.rect.Width());
	int h = max(minSize.cy, m_note.rect.Height());
	CRect proposed(m_note.rect.left, m_note.rect.top, m_note.rect.left + w, m_note.rect.top + h);
	CRect safeRect = EnsureRectVisibleOnScreen(proposed, m_hWnd);
	m_note.rect = safeRect;
	m_rcRestored = safeRect;

	ApplyTopMost(m_note.topMost);

	SetWindowPos(m_note.topMost ? &wndTopMost : &wndNoTopMost,
	             safeRect.left, safeRect.top,
	             safeRect.Width(), safeRect.Height(),
	             SWP_SHOWWINDOW | SWP_NOACTIVATE);

	CLogApp::Write(_T("NoteHostWindow::OnInitDialog: HWND=0x%p, name='%s', rect=(%d,%d,%d,%d)"),
		m_hWnd, m_note.name.GetString(), m_note.rect.left, m_note.rect.top, m_note.rect.right, m_note.rect.bottom);

	// P0 性能优化：初始化时缓存 DPI，避免 OnTimer 每 25ms 重复 GetProcAddress
	{
		HMODULE hUser32 = GetModuleHandle(_T("user32.dll"));
		if (hUser32)
		{
			typedef UINT(WINAPI* PFN_GetDpiForWindow)(HWND);
			auto pfn = reinterpret_cast<PFN_GetDpiForWindow>(GetProcAddress(hUser32, "GetDpiForWindow"));
			if (pfn) m_nCachedDpi = pfn(m_hWnd);
		}
	}

	m_nStartupWarmupTicks = 40; // 启动等待 1 秒 (40 x 25ms) 缓冲期，保障渲染稳定且让用户清晰看清便签
	m_nLeaveCount = 0;
	m_bRegionApplied = false;
	CheckEdgeDockState(); // CheckEdgeDockState 末尾已调用 UpdateEdgeTimer，此处无需再单独调用
	InitWebView();

	return TRUE;
}

void NoteHostWindow::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	CDialogEx::OnGetMinMaxInfo(lpMMI);

	CSize minSize = CalculateContentBasedMinSize(m_hWnd);
	lpMMI->ptMinTrackSize.x = minSize.cx;
	lpMMI->ptMinTrackSize.y = minSize.cy;
}

void NoteHostWindow::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	if (!IsWindowVisible()) return;
	RECT rcbounds = { 0 };
	GetClientRect(&rcbounds);

	if (m_controller != nullptr)
	{
		GetWindowRect(&m_note.rect);
		m_controller->put_Bounds(rcbounds);
		m_controller->NotifyParentWindowPositionChanged();
	}
}

void NoteHostWindow::OnExitSizeMove()
{
	CDialogEx::OnExitSizeMove();

	// 性能关键优化：仅在用户拖拽或缩放结束时执行 1 次原子磁盘持久化，I/O 削减 99%
	CRect rc;
	GetWindowRect(&rc);
	m_service.UpdateRect(m_note, rc);
	CLogApp::Write(_T("NoteHostWindow::OnExitSizeMove: Persisted rect (%d,%d,%d,%d) to disk"), rc.left, rc.top, rc.right, rc.bottom);

	if (m_controller != nullptr)
	{
		m_controller->NotifyParentWindowPositionChanged();
	}
}

void NoteHostWindow::OnMove(int x, int y)
{
	CDialogEx::OnMove(x, y);

	if (!IsWindowVisible()) return;

	CRect rc;
	GetWindowRect(&rc);
	m_note.rect = rc;

	if (m_controller != nullptr)
	{
		m_controller->NotifyParentWindowPositionChanged();
	}
}

void NoteHostWindow::OnWindowPosChanged(WINDOWPOS* lpwpos)
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

LRESULT NoteHostWindow::OnDpiChanged(WPARAM wParam, LPARAM lParam)
{
	// P0 性能优化：DPI 变化时更新缓存，避免 OnTimer 重复查找
	m_nCachedDpi = HIWORD(wParam) ? HIWORD(wParam) : 96;

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
	CRect rc;
	GetWindowRect(&rc);
	m_note.rect = rc;
	m_rcRestored = rc;
	CheckEdgeDockState();
	return 0;
}

void NoteHostWindow::OnWindowPosChanging(WINDOWPOS* lpwpos)
{
	CDialogEx::OnWindowPosChanging(lpwpos);
	if (lpwpos)
	{
		// 关键防黑框机制：禁用 Windows 默认的位图位移复制，彻底杜绝从左边或顶边拉伸时由于旧位图偏移产生的黑色拖影
		lpwpos->flags |= SWP_NOCOPYBITS;
	}
}

void NoteHostWindow::OnSizing(UINT fwSide, LPRECT pRect)
{
	CDialogEx::OnSizing(fwSide, pRect);
	if (m_controller != nullptr && pRect != nullptr)
	{
		RECT rcbounds = { 0, 0, pRect->right - pRect->left, pRect->bottom - pRect->top };
		m_controller->put_Bounds(rcbounds);
	}
}

void NoteHostWindow::OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp)
{
	if (bCalcValidRects && lpncsp != nullptr)
	{
		// 消除非客户区位移伪影
		lpncsp->rgrc[1] = lpncsp->rgrc[0];
		return;
	}
	CDialogEx::OnNcCalcSize(bCalcValidRects, lpncsp);
}

LRESULT NoteHostWindow::OnNcHitTest(CPoint point)
{
	if (m_bMouseThrough || m_bEdgeHidden)
	{
		return HTTRANSPARENT;
	}
	return CDialogEx::OnNcHitTest(point);
}

BOOL NoteHostWindow::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}

void NoteHostWindow::OnPaint()
{
	CPaintDC dc(this);
}

void NoteHostWindow::OnRawInput(UINT nInputcode, HRAWINPUT hRawInput)
{
	UINT dwSize = 0;
	if (GetRawInputData(hRawInput, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER)) == 0 && dwSize > 0)
	{
		BYTE stackBuffer[128];
		BYTE* pBuffer = stackBuffer;
		std::vector<BYTE> heapBuffer;
		if (dwSize > sizeof(stackBuffer))
		{
			heapBuffer.resize(dwSize);
			pBuffer = heapBuffer.data();
		}

		if (GetRawInputData(hRawInput, RID_INPUT, pBuffer, &dwSize, sizeof(RAWINPUTHEADER)) == dwSize)
		{
			RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(pBuffer);
			if (raw->header.dwType == RIM_TYPEMOUSE)
			{
				CPoint pt;
				GetCursorPos(&pt);
				OnMouseMoving(pt);
			}
		}
	}

	CDialogEx::OnRawInput(nInputcode, hRawInput);
}

// ------------------------------------------------------------------
// 屏幕工作区越界重映射与安全可见性保证（换屏幕/换分辨率/断开外接显示器自愈）
// ------------------------------------------------------------------
static CRect EnsureRectVisibleOnScreen(const CRect& proposedRect, HWND hWnd)
{
	CRect result = proposedRect;
	int width = proposedRect.Width();
	int height = proposedRect.Height();

	// 1. 查找包含该矩形中心点或距离最近的活动显示器
	POINT ptCenter = { result.CenterPoint().x, result.CenterPoint().y };
	HMONITOR hMonitor = ::MonitorFromPoint(ptCenter, MONITOR_DEFAULTTONEAREST);
	MONITORINFO mi = { sizeof(MONITORINFO) };
	if (!hMonitor || !::GetMonitorInfo(hMonitor, &mi))
	{
		hMonitor = ::MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		if (!hMonitor || !::GetMonitorInfo(hMonitor, &mi))
		{
			// 回退到系统主屏幕工作区
			CRect rcWork(0, 0, 1024, 768);
			::SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWork, 0);
			width = min(width, (int)(rcWork.Width() * 0.9));
			height = min(height, (int)(rcWork.Height() * 0.9));
			return CRect(rcWork.left + 50, rcWork.top + 50, rcWork.left + 50 + width, rcWork.top + 50 + height);
		}
	}

	const CRect& rcWork = mi.rcWork;

	// 2. 如果窗口尺寸大于当前屏幕工作区，等比缩减至不超过工作区 95%
	if (width > rcWork.Width())
	{
		width = max(240, (int)(rcWork.Width() * 0.95));
	}
	if (height > rcWork.Height())
	{
		height = max(180, (int)(rcWork.Height() * 0.95));
	}
	result.right = result.left + width;
	result.bottom = result.top + height;

	// 3. 屏幕边界越界兜底检测与重映射：
	if (result.right > rcWork.right)
	{
		result.right = rcWork.right;
		result.left = result.right - width;
	}
	if (result.left < rcWork.left)
	{
		result.left = rcWork.left;
		result.right = result.left + width;
	}

	if (result.bottom > rcWork.bottom)
	{
		result.bottom = rcWork.bottom;
		result.top = result.bottom - height;
	}
	if (result.top < rcWork.top)
	{
		result.top = rcWork.top;
		result.bottom = result.top + height;
	}

	// 4. 在确保处于工作区内部的前提下，再应用磁吸贴边效果
	return SnapRectToWorkArea(result, hWnd);
}

// ------------------------------------------------------------------
// 多显示器边界判定辅助：判断指定工作区边缘是否为多显示器虚拟桌面的最外层真实物理边界
// （若该边缘外侧紧邻另一台活动显示器，则属于屏幕间接缝，绝不触发贴边隐入，防止窗口滑入另一屏幕且避免接缝处拉手误触）
// ------------------------------------------------------------------
static bool IsOuterEdgeLeft(const CRect& rcWork)
{
	POINT pt = { rcWork.left - 10, rcWork.top + rcWork.Height() / 2 };
	HMONITOR hMon = ::MonitorFromPoint(pt, MONITOR_DEFAULTTONULL);
	return (hMon == NULL);
}

static bool IsOuterEdgeRight(const CRect& rcWork)
{
	POINT pt = { rcWork.right + 10, rcWork.top + rcWork.Height() / 2 };
	HMONITOR hMon = ::MonitorFromPoint(pt, MONITOR_DEFAULTTONULL);
	return (hMon == NULL);
}

static bool IsOuterEdgeTop(const CRect& rcWork)
{
	POINT pt = { rcWork.left + rcWork.Width() / 2, rcWork.top - 10 };
	HMONITOR hMon = ::MonitorFromPoint(pt, MONITOR_DEFAULTTONULL);
	return (hMon == NULL);
}

static bool IsOuterEdgeBottom(const CRect& rcWork)
{
	POINT pt = { rcWork.left + rcWork.Width() / 2, rcWork.bottom + 10 };
	HMONITOR hMon = ::MonitorFromPoint(pt, MONITOR_DEFAULTTONULL);
	return (hMon == NULL);
}

// ------------------------------------------------------------------
// PotPlayer 风格屏幕工作区磁吸贴边与四角吸附计算（精准避让任务栏与四角）
// ------------------------------------------------------------------
static CRect SnapRectToWorkArea(const CRect& proposedRect, HWND hWnd)
{
	CRect result = proposedRect;
	const int width = proposedRect.Width();
	const int height = proposedRect.Height();

	// 1. 根据当前屏幕 DPI 计算磁吸阈值（基准 18px，高分屏等比自适应）
	UINT dpi = 96;
	HMODULE hUser32 = GetModuleHandle(_T("user32.dll"));
	if (hUser32 != NULL)
	{
		typedef UINT(WINAPI* PFN_GetDpiForWindow)(HWND);
		auto pfnGetDpiForWindow = reinterpret_cast<PFN_GetDpiForWindow>(GetProcAddress(hUser32, "GetDpiForWindow"));
		if (pfnGetDpiForWindow && hWnd != NULL)
		{
			dpi = pfnGetDpiForWindow(hWnd);
		}
	}
	const int SNAP_THRESHOLD = ::MulDiv(18, static_cast<int>(dpi), 96);

	// 2. 屏幕工作区磁吸贴边与四角吸附（按中心点精准定位所在显示器，全面支持跨屏幕多显示器与屏幕接缝处磁吸）
	POINT ptCenter = { result.CenterPoint().x, result.CenterPoint().y };
	HMONITOR hMonitor = ::MonitorFromPoint(ptCenter, MONITOR_DEFAULTTONEAREST);
	MONITORINFO mi = { sizeof(MONITORINFO) };
	if (hMonitor && ::GetMonitorInfo(hMonitor, &mi))
	{
		const CRect& rcWork = mi.rcWork;

		// 水平磁吸贴边（左边贴靠 / 右边贴靠）
		if (abs(result.left - rcWork.left) <= SNAP_THRESHOLD)
		{
			result.left = rcWork.left;
			result.right = result.left + width;
		}
		else if (abs(result.right - rcWork.right) <= SNAP_THRESHOLD)
		{
			result.right = rcWork.right;
			result.left = result.right - width;
		}

		// 垂直磁吸贴边（顶边贴靠 / 底边贴靠）
		if (abs(result.top - rcWork.top) <= SNAP_THRESHOLD)
		{
			result.top = rcWork.top;
			result.bottom = result.top + height;
		}
		else if (abs(result.bottom - rcWork.bottom) <= SNAP_THRESHOLD)
		{
			result.bottom = rcWork.bottom;
			result.top = result.bottom - height;
		}
	}

	return result;
}

void NoteHostWindow::OnMoving(UINT fwSide, LPRECT pRect)
{
	CDialogEx::OnMoving(fwSide, pRect);
	if (pRect != nullptr)
	{
		CRect snapped = SnapRectToWorkArea(CRect(*pRect), m_hWnd);
		*pRect = snapped;
	}
}

void NoteHostWindow::PushEdgeDockState()
{
	CString sEdgeSide = _T("none");
	if (m_edgeDockState == EdgeDockState::Left) sEdgeSide = _T("left");
	else if (m_edgeDockState == EdgeDockState::Right) sEdgeSide = _T("right");
	else if (m_edgeDockState == EdgeDockState::Top) sEdgeSide = _T("top");

	CString sMsg;
	sMsg.Format(_T("{\"hidden\":%s,\"edge\":\"%s\",\"locked\":%s}"),
		m_bEdgeHidden ? _T("true") : _T("false"),
		sEdgeSide.GetString(),
		m_bEdgeLocked ? _T("true") : _T("false"));
	PostWebMessage(_T("edge_state"), sMsg);
}

void NoteHostWindow::CheckEdgeDockState()
{
	if (m_bEdgeHidden)
	{
		PushEdgeDockState();
		UpdateEdgeTimer();
		return;
	}

	CRect rc;
	GetWindowRect(&rc);

	HMONITOR hMonitor = ::MonitorFromRect(&rc, MONITOR_DEFAULTTONEAREST);
	MONITORINFO mi = { sizeof(MONITORINFO) };
	if (!hMonitor || !::GetMonitorInfo(hMonitor, &mi))
	{
		m_edgeDockState = EdgeDockState::None;
		m_rcRestored = rc;
		PushEdgeDockState();
		UpdateEdgeTimer();
		return;
	}

	const CRect& rcWork = mi.rcWork;
	UINT dpi = 96;
	HMODULE hUser32 = GetModuleHandle(_T("user32.dll"));
	if (hUser32 != NULL)
	{
		typedef UINT(WINAPI* PFN_GetDpiForWindow)(HWND);
		auto pfnGetDpiForWindow = reinterpret_cast<PFN_GetDpiForWindow>(GetProcAddress(hUser32, "GetDpiForWindow"));
		if (pfnGetDpiForWindow && m_hWnd != NULL)
		{
			dpi = pfnGetDpiForWindow(m_hWnd);
		}
	}
	const int EDGE_TOLERANCE = ::MulDiv(24, static_cast<int>(dpi), 96);

	// 仅当边缘为多屏桌面真实外层物理边界时才允许贴边停靠，彻底杜绝在屏幕接缝处被误判为贴边
	if (IsOuterEdgeLeft(rcWork) && abs(rc.left - rcWork.left) <= EDGE_TOLERANCE)
	{
		m_edgeDockState = EdgeDockState::Left;
		m_rcRestored = rc;
		m_rcRestored.left = rcWork.left;
		m_rcRestored.right = m_rcRestored.left + rc.Width();
		m_nLeaveCount = 0;
		PushEdgeDockState();
	}
	else if (IsOuterEdgeRight(rcWork) && abs(rc.right - rcWork.right) <= EDGE_TOLERANCE)
	{
		m_edgeDockState = EdgeDockState::Right;
		m_rcRestored = rc;
		m_rcRestored.right = rcWork.right;
		m_rcRestored.left = m_rcRestored.right - rc.Width();
		m_nLeaveCount = 0;
		PushEdgeDockState();
	}
	else if (IsOuterEdgeTop(rcWork) && abs(rc.top - rcWork.top) <= EDGE_TOLERANCE)
	{
		m_edgeDockState = EdgeDockState::Top;
		m_rcRestored = rc;
		m_rcRestored.top = rcWork.top;
		m_rcRestored.bottom = m_rcRestored.top + rc.Height();
		m_nLeaveCount = 0;
		PushEdgeDockState();
	}
	else
	{
		m_edgeDockState = EdgeDockState::None;
		m_bEdgeHidden = false;
		m_bEdgeLocked = false;
		m_nLeaveCount = 0;
		m_rcRestored = rc;
		PushEdgeDockState();
	}
	UpdateEdgeTimer();
}

// 按需启停贴边检测定时器：
//   - 仅当便签处于贴边状态（Left/Right/Top）、未锁定且窗口可见时才运行 25ms 高频定时器。
//   - 其余情况（居中、锁定、隐藏）立即 KillTimer，彻底消除空转开销。
// SetTimer 在定时器已存在时重置而不创建第二个；KillTimer 在定时器不存在时是空操作，两者均安全。
void NoteHostWindow::UpdateEdgeTimer()
{
	if (!m_hWnd || !::IsWindow(m_hWnd)) return;
	bool needTimer = (m_edgeDockState != EdgeDockState::None)
	                 && !m_bEdgeLocked
	                 && ::IsWindowVisible(m_hWnd);
	if (needTimer)
		SetTimer(1002, 25, nullptr);
	else
		KillTimer(1002);
}

void NoteHostWindow::UpdateEdgeDockRegion(bool bHidden)
{
	if (!m_hWnd || !::IsWindow(m_hWnd)) return;

	// P0 性能优化：防止在贴边稳定后每帧重复调用 DWM/SetWindowLong（每次调用强制 DWM 重建合成树）
	if (bHidden && m_bRegionApplied) return;

	m_bRegionApplied = bHidden;

	// 关键：彻底移除 1-bit GDI Region 裁剪，让 WebView2/DirectComposition 100% 以原生次像素级抗锯齿渲染圆角，彻底消除锯齿与杂色边缘！
	::SetWindowRgn(m_hWnd, NULL, TRUE);

	if (bHidden && m_edgeDockState != EdgeDockState::None)
	{
		// 贴边隐入状态：设置 WS_EX_TRANSPARENT 实现 100% 鼠标穿透，底层桌面与所有应用零遮挡、零误触
		SetMouseThrough(true);
	}
	else
	{
		// 恢复状态：立即恢复全窗口正常鼠标交互
		SetMouseThrough(false);
	}
}

void NoteHostWindow::RetractToEdge()
{
	if (m_edgeDockState == EdgeDockState::None || m_bEdgeHidden || m_bMoveWindow || m_bEdgeLocked || m_webView == nullptr)
		return;

	CPoint ptCursor;
	GetCursorPos(&ptCursor);
	CRect rcCurrent;
	GetWindowRect(&rcCurrent);
	if (rcCurrent.PtInRect(ptCursor))
		return;

	m_bEdgeHidden = true;
	m_nEdgeHiddenTicks = 0;
	// 1. 先触发前端 CSS 硬件加速流畅滑出动效（0.22s）
	PushEdgeDockState();
}

void NoteHostWindow::RestoreFromEdge()
{
	if (!m_bEdgeHidden)
		return;

	m_bEdgeHidden = false;
	m_bRegionApplied = false;  // 重置防重标志，允许下次收缩时重新应用穿透
	m_nLeaveCount = 0;
	m_nEdgeHiddenTicks = 0;
	// P2 恢复：展开时将定时器恢复为高灵敏 25ms，以准确检测下次鼠标离开
	KillTimer(1002);
	SetTimer(1002, 25, nullptr);
	UpdateEdgeDockRegion(false); // 立即还原全窗口交互区域，保证所有区域可点击
	if (m_controller != nullptr)
	{
		m_controller->NotifyParentWindowPositionChanged();
	}
	PushEdgeDockState();
}

void NoteHostWindow::OnMouseMove(UINT nFlags, CPoint point)
{
	CDialogEx::OnMouseMove(nFlags, point);
	if (m_bEdgeHidden)
	{
		RestoreFromEdge();
	}
}

void NoteHostWindow::OnMouseLeave()
{
	CDialogEx::OnMouseLeave();
}

void NoteHostWindow::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == 1002) // 贴边高频灵敏检测 (25ms 频率，40 FPS 极速响应)
	{
		if (m_bMoveWindow) return;

		// 启动等待 1 秒缓冲期：刚打开软件时保持全展开 1 秒，让用户看清内容并确保渲染完全稳定后再贴边
		if (m_nStartupWarmupTicks > 0)
		{
			m_nStartupWarmupTicks--;
			return;
		}

		if (m_edgeDockState != EdgeDockState::None)
		{
			if (m_bEdgeLocked)
			{
				// 锁定状态下不隐入桌面外
				return;
			}
			CPoint ptCursor;
			GetCursorPos(&ptCursor);

			if (!m_bEdgeHidden)
			{
				CRect rcWindow;
				GetWindowRect(&rcWindow);
				if (!rcWindow.PtInRect(ptCursor))
				{
					m_nLeaveCount++;
					if (m_nLeaveCount >= 4) // ~100ms 离开防抖，既灵敏又杜绝误收
					{
						RetractToEdge();
					}
				}
				else
				{
					m_nLeaveCount = 0;
				}
			}
			else
			{
				// 贴边隐入状态：给 CSS 滑出动效预留 ~200ms 的无抖动硬件加速渲染窗口，动效完成后无缝接入 WS_EX_TRANSPARENT 穿透
				m_nEdgeHiddenTicks++;
				if (m_nEdgeHiddenTicks == 9) // 9 * 25ms = 225ms，仅首次到达时执行一次（P0：修复 >= 9 导致的每帧重复调用）
				{
					UpdateEdgeDockRegion(true);
					// P2 性能优化：动画稳定后降频至 80ms（约 12Hz），大幅减少空转开销
					KillTimer(1002);
					SetTimer(1002, 80, nullptr);
				}

				// 使用缓存 DPI（P0：消除每帧 GetModuleHandle + GetProcAddress 符号查找）
				const int TAB_THICKNESS = ::MulDiv(36, static_cast<int>(m_nCachedDpi), 96);
				const int TAB_LENGTH    = ::MulDiv(80, static_cast<int>(m_nCachedDpi), 96);

				CRect rcWindow;
				GetWindowRect(&rcWindow);
				CRect rcTabScreen; // 局部胶囊小方块热区

				if (m_edgeDockState == EdgeDockState::Left)
				{
					int tabTop = rcWindow.top + (rcWindow.Height() - TAB_LENGTH) / 2;
					rcTabScreen = CRect(rcWindow.left, tabTop, rcWindow.left + TAB_THICKNESS, tabTop + TAB_LENGTH);
				}
				else if (m_edgeDockState == EdgeDockState::Right)
				{
					int tabTop = rcWindow.top + (rcWindow.Height() - TAB_LENGTH) / 2;
					rcTabScreen = CRect(rcWindow.right - TAB_THICKNESS, tabTop, rcWindow.right, tabTop + TAB_LENGTH);
				}
				else if (m_edgeDockState == EdgeDockState::Top)
				{
					int tabLeft = rcWindow.left + (rcWindow.Width() - TAB_LENGTH) / 2;
					rcTabScreen = CRect(tabLeft, rcWindow.top, tabLeft + TAB_LENGTH, rcWindow.top + TAB_THICKNESS);
				}

				// 仅当鼠标精准触碰局部拉手小方块时才触发还原
				if (rcTabScreen.PtInRect(ptCursor))
				{
					RestoreFromEdge();
					m_nLeaveCount = 0;
				}
			}
		}
		return;
	}
	CDialogEx::OnTimer(nIDEvent);
}

LRESULT NoteHostWindow::OnDisplayChange(WPARAM wParam, LPARAM lParam)
{
	CRect current;
	GetWindowRect(&current);
	CRect safeRect = EnsureRectVisibleOnScreen(current, m_hWnd);
	if (safeRect != current)
	{
		SetWindowPos(NULL, safeRect.left, safeRect.top, safeRect.Width(), safeRect.Height(), SWP_NOZORDER | SWP_NOACTIVATE);
		m_note.rect = safeRect;
		m_rcRestored = safeRect;
		m_service.UpdateRect(m_note, safeRect);
		CheckEdgeDockState();
	}
	return 0;
}

void NoteHostWindow::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
	CDialogEx::OnSettingChange(uFlags, lpszSection);
	if (uFlags == SPI_SETWORKAREA)
	{
		OnDisplayChange(0, 0);
	}
}

void NoteHostWindow::OnNcMouseMove(UINT nHitTest, CPoint point)
{
	if (m_bEdgeHidden)
	{
		RestoreFromEdge();
	}

	CDialogEx::OnNcMouseMove(nHitTest, point);
}

void NoteHostWindow::OnMouseMoving(CPoint pt)
{
	if (!m_bMoveWindow || m_bEdgeLocked) return;

	// 安全自愈：若左键已松开，立即退出移动状态并保存位置
	if ((GetKeyState(VK_LBUTTON) & 0x8000) == 0)
	{
		m_bMoveWindow = false;
		CRawInput::Remove(GetSafeHwnd(), RAW_TYPE_MS);
		CRect rc;
		GetWindowRect(&rc);
		m_service.UpdateRect(m_note, rc);
		m_rcRestored = rc;
		CheckEdgeDockState();
		if (m_controller != nullptr)
		{
			m_controller->NotifyParentWindowPositionChanged();
		}
		return;
	}

	CPoint ptOffset = m_BeginMovePoint - m_BeginMoveRect.TopLeft();
	CPoint newPosition = pt - ptOffset;
	CRect proposedRect(newPosition.x, newPosition.y, newPosition.x + m_BeginMoveRect.Width(), newPosition.y + m_BeginMoveRect.Height());

	// 应用屏幕工作区磁吸贴边与四角吸附
	CRect snappedRect = SnapRectToWorkArea(proposedRect, m_hWnd);

	SetWindowPos(NULL, snappedRect.left, snappedRect.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
	m_rcRestored = snappedRect;
	if (m_controller != nullptr)
	{
		m_controller->NotifyParentWindowPositionChanged();
	}
}

void NoteHostWindow::SetWindowAlpha(float alpha)
{
	ModifyStyleEx(0, WS_EX_LAYERED);
	SetLayeredWindowAttributes(0, static_cast<BYTE>(255.0f * alpha / 100.0f), LWA_ALPHA);
}

void NoteHostWindow::LocateAndHighlight()
{
	if (!m_hWnd || !::IsWindow(m_hWnd)) return;

	// 1. 若贴边隐入，立即恢复全展开
	RestoreFromEdge();

	// 2. 取消鼠标穿透模式，恢复全窗口交互
	SetMouseThrough(false);

	// 3. 确保坐标在当前屏幕范围内
	CRect rc;
	GetWindowRect(&rc);
	CRect safeRect = EnsureRectVisibleOnScreen(rc, m_hWnd);
	if (safeRect != rc)
	{
		SetWindowPos(NULL, safeRect.left, safeRect.top, safeRect.Width(), safeRect.Height(), SWP_NOZORDER | SWP_NOACTIVATE);
		m_rcRestored = safeRect;
	}

	// 4. 同步状态：标记为可见并广播数据变更
	if (!m_note.visible)
	{
		m_note.visible = true;
		m_service.SetVisibility(m_note, true);
		NoteEventBus::Instance().Publish(NoteEvent(NoteEventType::VisibilityChanged, m_note.name));
	}

	// 5. 显示窗口并激活至最前（严格保持并强化 topMost 置顶特性）
	if (m_controller)
	{
		m_controller->put_IsVisible(TRUE);
	}
	ShowWindow(SW_SHOW);
	::SetWindowPos(m_hWnd, m_note.topMost ? HWND_TOPMOST : HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
	SetForegroundWindow();

	// 6. 触发前端炫彩呼吸光环高亮动效
	PostWebMessage(_T("locate_pulse"), _T("{}"));

	// 5. 触发 Windows 系统级窗口闪烁提示
	FLASHWINFO fi = { sizeof(FLASHWINFO) };
	fi.cbSize = sizeof(FLASHWINFO);
	fi.hwnd = m_hWnd;
	fi.dwFlags = FLASHW_ALL | FLASHW_TIMERNOFG;
	fi.uCount = 4;
	fi.dwTimeout = 120;
	::FlashWindowEx(&fi);
}

void NoteHostWindow::SetMouseThrough(bool through)
{
	m_bMouseThrough = through;
	DWORD dwExStyle = GetWindowLong(m_hWnd, GWL_EXSTYLE);
	if (through)
	{
		dwExStyle |= (WS_EX_TRANSPARENT | WS_EX_LAYERED);
	}
	else
	{
		dwExStyle &= ~WS_EX_TRANSPARENT;
	}
	if (m_note.topMost)
	{
		dwExStyle |= WS_EX_TOPMOST;
	}
	else
	{
		dwExStyle &= ~WS_EX_TOPMOST;
	}
	SetWindowLong(m_hWnd, GWL_EXSTYLE, dwExStyle);

	::SetWindowPos(m_hWnd, m_note.topMost ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED);

	if (m_webView != nullptr) PushMouseThrough();
}

bool NoteHostWindow::IsMouseThrough() const
{
	return m_bMouseThrough;
}

void NoteHostWindow::ApplyTopMost(bool topMost)
{
	m_note.topMost = topMost;
	if (m_hWnd != NULL && ::IsWindow(m_hWnd))
	{
		// 1. 同步 Win32 扩展窗口样式 WS_EX_TOPMOST
		if (topMost)
		{
			ModifyStyleEx(0, WS_EX_TOPMOST, SWP_FRAMECHANGED);
		}
		else
		{
			ModifyStyleEx(WS_EX_TOPMOST, 0, SWP_FRAMECHANGED);
		}

		// 2. 物理调整 Z-Order 频带（使用 SetWindowPos 明确切换 HWND_TOPMOST / HWND_NOTOPMOST）
		::SetWindowPos(m_hWnd, topMost ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0,
			SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED);
	}

	if (m_webView != nullptr)
	{
		PushNoteSetting();
	}
}

void NoteHostWindow::PushNoteItems()
{
	RJDoc doc;
	auto& allocator = doc.GetAllocator();
	RJValue vItems = NoteDto::SerializeItemsArray(m_note.items, allocator);
	CString sData = NoteDto::Stringify(vItems);
	PostWebMessage(_T("data"), sData);
}

void NoteHostWindow::PushNoteSetting()
{
	RJDoc doc;
	auto& allocator = doc.GetAllocator();
	RJValue vData = NoteDto::SerializeSetting(m_note, allocator);
	CString sSetting = NoteDto::Stringify(vData);
	PostWebMessage(_T("setting"), sSetting);
}

void NoteHostWindow::PushMouseThrough()
{
	PostWebMessage(_T("lock"), IsMouseThrough() ? _T("true") : _T("false"));
}

void NoteHostWindow::PostWebMessage(const CString& event, const CString& dataJson)
{
	if (m_webView == nullptr) return;

	CString msg;
	msg.Format(_T("{\"event\":\"%s\",\"data\":%s}"), event.GetString(), dataJson.IsEmpty() ? _T("null") : dataJson.GetString());
#ifdef _DEBUG
	// P0 性能优化：Release 下不调用 OutputDebugString（避免每帧高频 IPC 时内核开销）
	CLogApp::Write(_T("NoteHostWindow::PostWebMessage: hwnd=0x%p, event=%s"), m_hWnd, event.GetString());
#endif
	m_webView->PostWebMessageAsJson(msg.GetString());
}

void NoteHostWindow::InitWebView()
{
	if (m_controller != nullptr)
	{
		return;
	}

	HWND hWnd = this->GetSafeHwnd();
	WebViewEnvironmentManager::GetOrCreateEnvironment([this, hWnd](HRESULT result, ICoreWebView2Environment* environment) {
		if (result != S_OK || environment == nullptr)
		{
			CLogApp::Error(_T("NoteHostWindow::InitWebView failed: hr=0x%08X"), result);
			if (IDYES == MessageBox(_T("未检测到 Edge Webview2，请点击【是】打开 https://developer.microsoft.com/zh-cn/microsoft-edge/webview2/#download-section 下载安装！"), NULL, MB_YESNO))
				ShellExecute(NULL, _T("open"), _T("https://developer.microsoft.com/zh-cn/microsoft-edge/webview2/#download-section"), NULL, NULL, SW_SHOW);
			return;
		}

		if (this->m_hWnd == NULL || !::IsWindow(this->m_hWnd))
		{
			return;
		}

		environment->CreateCoreWebView2Controller(hWnd,
			Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
				this, &NoteHostWindow::OnCreateCoreWebView2ControllerCompleted).Get());
	});
}

HRESULT NoteHostWindow::OnCreateCoreWebView2ControllerCompleted(HRESULT result, ICoreWebView2Controller* controller)
{
	if (FAILED(result) || controller == nullptr) {
		CLogApp::Error(_T("NoteHostWindow::OnCreateCoreWebView2ControllerCompleted failed: hwnd=0x%p, hr=0x%08X"), m_hWnd, result);
		return result;
	}
	CLogApp::Info(_T("NoteHostWindow::OnCreateCoreWebView2ControllerCompleted: hwnd=0x%p, hr=0x%08X"), m_hWnd, result);

	m_controller = controller;
	m_controller->get_CoreWebView2(&m_webView);
	if (!m_webView) {
		return E_FAIL;
	}
	m_controller->put_IsVisible(FALSE); // 初始化先隐藏，避免首帧白色闪烁

	Microsoft::WRL::ComPtr<ICoreWebView2Controller2> controller2;
	m_controller.As(&controller2);

	if (controller2) {
		// 设置背景色为透明
		COREWEBVIEW2_COLOR bgColor = { 0, 0, 0, 0 };
		controller2->put_DefaultBackgroundColor(bgColor); // BGRA格式，A=0表示全透明
	}

	wil::com_ptr<ICoreWebView2Settings> settings;
	m_webView->get_Settings(&settings);
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



	RECT bounds;
	::GetClientRect(GetSafeHwnd(), &bounds);
	m_controller->put_Bounds(bounds);

	EventRegistrationToken token;
	m_webView->add_NavigationStarting(Microsoft::WRL::Callback<ICoreWebView2NavigationStartingEventHandler>(
		[](ICoreWebView2* webview, ICoreWebView2NavigationStartingEventArgs* args) -> HRESULT {
			return S_OK;
		}).Get(), &token);

	m_webView->add_NavigationCompleted(Microsoft::WRL::Callback<ICoreWebView2NavigationCompletedEventHandler>(
		this, &NoteHostWindow::OnDocumentReady).Get(), &token);

	m_webView->add_WebMessageReceived(Microsoft::WRL::Callback<ICoreWebView2WebMessageReceivedEventHandler>(
		this, &NoteHostWindow::OnWebMessageReceived).Get(), &token);

	m_webView->AddScriptToExecuteOnDocumentCreated(
		LR"(
			window.addEventListener('error', function(e) {
				if (window.chrome && window.chrome.webview) {
					window.chrome.webview.postMessage(JSON.stringify({event: 'error', data: {msg: e.message, src: e.filename, line: e.lineno, col: e.colno, stack: e.error ? e.error.stack : ''}}));
				}
			});
			window.addEventListener('unhandledrejection', function(e) {
				if (window.chrome && window.chrome.webview) {
					window.chrome.webview.postMessage(JSON.stringify({event: 'error', data: {msg: e.reason ? (e.reason.message || String(e.reason)) : 'unhandled rejection', stack: e.reason ? e.reason.stack : ''}}));
				}
			});
			console.error = function() {
				var args = Array.prototype.slice.call(arguments);
				if (window.chrome && window.chrome.webview) {
					window.chrome.webview.postMessage(JSON.stringify({event: 'error', data: {msg: args.join(' ')}}));
				}
			};
		)",
		nullptr);

#ifdef _DEBUG_HTTP
	CString sUrl = _T("http://localhost:5173");
	CLogApp::Write(_T("NoteHostWindow::Navigate: debug_url=%s"), sUrl.GetString());
	m_webView->Navigate(sUrl.GetString());
#else
	AppSetting currentSetting = AppSettingStore::Load();
	CString sThemeFolder = Easy::Path::GetCurDirectory(_T("themes\\") + currentSetting.sTheme);
	if (!Easy::Path::Exists(Easy::Path::Resolve(sThemeFolder, _T("index.html")))) {
		sThemeFolder = Easy::Path::GetCurDirectory(_T("themes\\Default"));
	}

	Microsoft::WRL::ComPtr<ICoreWebView2_3> webView3;
	HRESULT hr = m_webView.As(&webView3);
	if (SUCCEEDED(hr) && webView3)
	{
		sThemeFolder.TrimRight(_T("\\/"));
		CLogApp::Write(_T("NoteHostWindow::Navigate: virtual host mapping folder=%s"), sThemeFolder.GetString());
		webView3->SetVirtualHostNameToFolderMapping(
			L"app.notes",
			sThemeFolder.GetString(),
			COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_ALLOW
		);
		// 时间戳绕过 WebView2 对 index.html 的 HTTP 缓存，确保加载最新版本
		ULONGLONG ts = ::GetTickCount64();
		CString sNoteUrl;
		sNoteUrl.Format(_T("https://app.notes/index.html?t=%I64u"), ts);
		m_webView->Navigate(sNoteUrl.GetString());
	}
	else
	{
		CString sTheme = Easy::Path::Resolve(sThemeFolder, _T("index.html"));
		CString sUrl = CString(_T("file:///")) + sTheme;
		sUrl.Replace(_T("\\"), _T("/"));
		CLogApp::Write(_T("NoteHostWindow::Navigate: file url=%s"), sUrl.GetString());
		m_webView->Navigate(sUrl.GetString());
	}
#endif // _DEBUG_HTTP

	return S_OK;
}

HRESULT NoteHostWindow::OnWebMessageReceived(ICoreWebView2* webview, ICoreWebView2WebMessageReceivedEventArgs* args)
{
	wil::unique_cotaskmem_string raw;
	args->TryGetWebMessageAsString(&raw);
	CLogApp::Write(_T("NoteHostWindow::OnWebMessageReceived: hwnd=0x%p, raw=%s"), m_hWnd, raw.get() ? raw.get() : L"null");
	m_dispatcher.Dispatch(raw.get());
	return S_OK;
}

HRESULT NoteHostWindow::OnDocumentReady(ICoreWebView2* webview, ICoreWebView2NavigationCompletedEventArgs* args)
{
	BOOL isSuccess = FALSE;
	if (args) args->get_IsSuccess(&isSuccess);
	CLogApp::Write(_T("NoteHostWindow::OnDocumentReady: hwnd=0x%p, name='%s', isSuccess=%d"), m_hWnd, m_note.name.GetString(), isSuccess);
	SetWindowAlpha(m_note.opacityEnabled ? static_cast<float>(m_note.opacity) : 100.0f);
	if (m_controller)
	{
		m_controller->put_IsVisible(TRUE); // DOM 与样式就绪后呈现，0 闪烁
	}
	CheckEdgeDockState();

#ifdef _DEBUG
	// 检查 DOM 渲染状态（仅 Debug 构建，Release 完全跳过此跨进程 JS 注入）
	m_webView->ExecuteScript(
		L"(function() { return JSON.stringify({ app: document.getElementById('app') ? document.getElementById('app').children.length : -1, html: document.body.innerHTML.substring(0, 150) }); })()",
		Microsoft::WRL::Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
			[this](HRESULT hr, LPCWSTR result) -> HRESULT {
				CLogApp::Write(_T("NoteHostWindow::DOM_State: hwnd=0x%p, result=%s"), m_hWnd, result ? result : _T("null"));
				return S_OK;
			}).Get());
#endif

	return S_OK;
}

struct LambdaHostHandler : public INoteHostMessageHandler {
	std::function<void(const NoteHostMessage&)> fn;
	explicit LambdaHostHandler(std::function<void(const NoteHostMessage&)> f) : fn(std::move(f)) {}
	virtual void Handle(const NoteHostMessage& msg) override {
		if (fn) fn(msg);
	}
};

void NoteHostWindow::RegisterHandler(NoteHostMessageType type, std::function<void(const NoteHostMessage&)> fn)
{
	auto handler = std::make_unique<LambdaHostHandler>(std::move(fn));
	m_dispatcher.Register(type, handler.get());
	m_handlers.push_back(std::move(handler));
}

void NoteHostWindow::RegisterHandlers()
{
	m_handlers.clear();

	// 1. Move (Win32)
	RegisterHandler(NoteHostMessageType::Move, [this](const NoteHostMessage& msg) {
		rapidjson::GenericDocument<rapidjson::UTF16<TCHAR>> doc;
		doc.Parse(msg.dataJson.GetString());
		if (!doc.HasParseError() && doc.IsBool()) {
			bool bMove = doc.GetBool();
			if (bMove && m_bEdgeLocked) {
				return; // 贴边锁定状态下禁止拖动移动！
			}
			if (bMove && m_bEdgeHidden)
			{
				RestoreFromEdge();
			}
			m_bMoveWindow = bMove;
			GetClientRect(&m_BeginMoveRect);
			ClientToScreen(&m_BeginMoveRect);
			GetCursorPos(&m_BeginMovePoint);
			if (m_bMoveWindow)
			{
				CRawInput::Register(GetSafeHwnd(), RAW_TYPE_MS);
			}
			else
			{
				CRawInput::Remove(GetSafeHwnd(), RAW_TYPE_MS);
				CRect rc;
				GetWindowRect(&rc);
				m_service.UpdateRect(m_note, rc);
				m_rcRestored = rc;
				CheckEdgeDockState();
				if (m_controller != nullptr)
				{
					m_controller->NotifyParentWindowPositionChanged();
				}
			}
		}
	});

	// 2. Resize (Win32 Native Modal Sizing Loop)
	RegisterHandler(NoteHostMessageType::Resize, [this](const NoteHostMessage& msg) {
		rapidjson::GenericDocument<rapidjson::UTF16<TCHAR>> doc;
		doc.Parse(msg.dataJson.GetString());
		if (!doc.HasParseError() && doc.IsString()) {
			CString dir = doc.GetString();
			WPARAM wparam = 0xF008; // 默认右下角
			if (dir == _T("left")) wparam = 0xF001;          // W
			else if (dir == _T("right")) wparam = 0xF002;    // E
			else if (dir == _T("top")) wparam = 0xF003;      // N
			else if (dir == _T("top-left")) wparam = 0xF004; // NW
			else if (dir == _T("top-right")) wparam = 0xF005;// NE
			else if (dir == _T("bottom")) wparam = 0xF006;   // S
			else if (dir == _T("bottom-left")) wparam = 0xF007;  // SW
			else if (dir == _T("bottom-right")) wparam = 0xF008; // SE

			::ReleaseCapture();
			::PostMessage(m_hWnd, WM_SYSCOMMAND, wparam, 0);
		}
	});

	// 3. Lock (Win32 穿透模式)
	RegisterHandler(NoteHostMessageType::Lock, [this](const NoteHostMessage& msg) {
		rapidjson::GenericDocument<rapidjson::UTF16<TCHAR>> doc;
		doc.Parse(msg.dataJson.GetString());
		if (!doc.HasParseError() && doc.IsBool()) {
			bool bMouseThrough = doc.GetBool();
			SetMouseThrough(bMouseThrough);
		}
	});

	// 3.1 EdgeLock (贴边锁定控制：禁止拖动且不隐入桌面外)
	RegisterHandler(NoteHostMessageType::EdgeLock, [this](const NoteHostMessage& msg) {
		rapidjson::GenericDocument<rapidjson::UTF16<TCHAR>> doc;
		doc.Parse(msg.dataJson.GetString());
		if (!doc.HasParseError() && doc.IsBool()) {
			m_bEdgeLocked = doc.GetBool();
			if (m_bEdgeLocked && m_bEdgeHidden)
			{
				RestoreFromEdge();
			}
			PushEdgeDockState();
			// 锁定状态改变时同步刷新定时器：锁定时 KillTimer，解锁且贴边时重新 SetTimer
			UpdateEdgeTimer();
		}
	});

	// 3. Top (Business + Win32)
	RegisterHandler(NoteHostMessageType::Top, [this](const NoteHostMessage& msg) {
		rapidjson::GenericDocument<rapidjson::UTF16<TCHAR>> doc;
		doc.Parse(msg.dataJson.GetString());
		if (!doc.HasParseError() && doc.IsBool()) {
			bool bTop = doc.GetBool();
			m_service.UpdateAppearance(m_note, m_note.bgColor, m_note.opacityEnabled, m_note.opacity, bTop);
			ApplyTopMost(bTop);
			NoteEventBus::Instance().Publish(NoteEvent(NoteEventType::Updated, m_note.name));
		}
	});

	// 4. OpacityAble (Business + Win32)
	RegisterHandler(NoteHostMessageType::OpacityAble, [this](const NoteHostMessage& msg) {
		rapidjson::GenericDocument<rapidjson::UTF16<TCHAR>> doc;
		doc.Parse(msg.dataJson.GetString());
		if (!doc.HasParseError() && doc.IsBool()) {
			bool bOpacity = doc.GetBool();
			m_service.UpdateAppearance(m_note, m_note.bgColor, bOpacity, m_note.opacity, m_note.topMost);
			if (bOpacity) SetWindowAlpha(static_cast<float>(m_note.opacity));
			else SetWindowAlpha(100.0f);
			NoteEventBus::Instance().Publish(NoteEvent(NoteEventType::Updated, m_note.name));
		}
	});

	// 5. BgColor (Business)
	RegisterHandler(NoteHostMessageType::BgColor, [this](const NoteHostMessage& msg) {
		rapidjson::GenericDocument<rapidjson::UTF16<TCHAR>> doc;
		doc.Parse(msg.dataJson.GetString());
		CString sHex = _T("#0b0f14");
		if (!doc.HasParseError() && doc.IsString()) {
			sHex = doc.GetString();
		}
		COLORREF color = Cvt::ToColor(sHex);
		m_brush.DeleteObject();
		m_brush.CreateSolidBrush(color);
		m_service.UpdateAppearance(m_note, color, m_note.opacityEnabled, m_note.opacity, m_note.topMost);
		NoteEventBus::Instance().Publish(NoteEvent(NoteEventType::Updated, m_note.name));
	});

	// 6. Title (Business)
	RegisterHandler(NoteHostMessageType::Title, [this](const NoteHostMessage& msg) {
		rapidjson::GenericDocument<rapidjson::UTF16<TCHAR>> doc;
		doc.Parse(msg.dataJson.GetString());
		CString sTitle = _T("");
		if (!doc.HasParseError() && doc.IsString()) {
			sTitle = doc.GetString();
		}
		m_service.UpdateTitle(m_note, sTitle);
		NoteEventBus::Instance().Publish(NoteEvent(NoteEventType::Updated, m_note.name));
	});

	// 7. Close (Win32 + Business Visibility Sync)
	RegisterHandler(NoteHostMessageType::Close, [this](const NoteHostMessage&) {
		m_note.visible = false;
		m_service.SetVisibility(m_note, false);
		if (m_controller) m_controller->put_IsVisible(FALSE);
		ShowWindow(SW_HIDE);
		NoteEventBus::Instance().Publish(NoteEvent(NoteEventType::VisibilityChanged, m_note.name));
	});

	// 8. Add (Business)
	RegisterHandler(NoteHostMessageType::Add, [this](const NoteHostMessage& msg) {
		RJDoc doc;
		doc.Parse(msg.dataJson.GetString());
		if (!doc.HasParseError() && doc.IsObject()) {
			NoteItem item = NoteDto::ItemFromJson(doc);
			m_service.AddItem(m_note, item);
			NoteEventBus::Instance().Publish(NoteEvent(NoteEventType::Updated, m_note.name));
		}
	});

	// 9. Task (Business)
	RegisterHandler(NoteHostMessageType::Task, [this](const NoteHostMessage& msg) {
		RJDoc doc;
		doc.Parse(msg.dataJson.GetString());
		if (!doc.HasParseError() && doc.IsObject()) {
			NoteItem item = NoteDto::ItemFromJson(doc);
			m_service.ExportToCalendar(item);
		}
	});

	// 10. Update (Business)
	RegisterHandler(NoteHostMessageType::Update, [this](const NoteHostMessage& msg) {
		RJDoc doc;
		doc.Parse(msg.dataJson.GetString());
		if (!doc.HasParseError() && doc.IsObject()) {
			NoteItem item = NoteDto::ItemFromJson(doc);
			m_service.UpdateItem(m_note, item);
			NoteEventBus::Instance().Publish(NoteEvent(NoteEventType::Updated, m_note.name));
		}
	});

	// 11. UpdateAll (Business)
	RegisterHandler(NoteHostMessageType::UpdateAll, [this](const NoteHostMessage& msg) {
		RJDoc doc;
		doc.Parse(msg.dataJson.GetString());
		if (!doc.HasParseError() && doc.IsArray()) {
			std::vector<NoteItem> items;
			for (rapidjson::SizeType i = 0; i < doc.Size(); i++) {
				items.push_back(NoteDto::ItemFromJson(doc[i]));
			}
			m_service.UpdateAllItems(m_note, items);
			NoteEventBus::Instance().Publish(NoteEvent(NoteEventType::Updated, m_note.name));
		}
	});

	// 12. Remove (Business)
	RegisterHandler(NoteHostMessageType::Remove, [this](const NoteHostMessage& msg) {
		rapidjson::GenericDocument<rapidjson::UTF16<TCHAR>> doc;
		doc.Parse(msg.dataJson.GetString());
		if (!doc.HasParseError()) {
			uint64_t id = 0;
			if (doc.IsObject() && doc.HasMember(_T("id"))) {
				const auto& vId = doc[_T("id")];
				if (vId.IsUint64()) id = vId.GetUint64();
				else if (vId.IsUint()) id = static_cast<uint64_t>(vId.GetUint());
				else if (vId.IsInt64()) id = static_cast<uint64_t>(vId.GetInt64());
				else if (vId.IsInt()) id = static_cast<uint64_t>(vId.GetInt());
			} else if (doc.IsUint64()) {
				id = doc.GetUint64();
			} else if (doc.IsUint()) {
				id = static_cast<uint64_t>(doc.GetUint());
			}
			if (id != 0) {
				m_service.RemoveItem(m_note, id);
				NoteEventBus::Instance().Publish(NoteEvent(NoteEventType::Updated, m_note.name));
			}
		}
	});

	// 13. Hide (Business + Win32)
	RegisterHandler(NoteHostMessageType::Hide, [this](const NoteHostMessage&) {
		m_service.Hide(m_note);
		if (m_controller) m_controller->put_IsVisible(FALSE);
		ShowWindow(SW_HIDE);
		NoteEventBus::Instance().Publish(NoteEvent(NoteEventType::VisibilityChanged, m_note.name));
	});

	// 14. Clear (Business + Win32)
	RegisterHandler(NoteHostMessageType::Clear, [this](const NoteHostMessage&) {
		m_service.Clear(m_note);
		NoteEventBus::Instance().Publish(NoteEvent(NoteEventType::Deleted, m_note.name));
		OnCancel();
	});

	// 15. Listen (Host Sync)
	RegisterHandler(NoteHostMessageType::Listen, [this](const NoteHostMessage&) {
		PushNoteItems();
		PushNoteSetting();
		PushMouseThrough();
		CheckEdgeDockState();
		if (m_controller && m_note.visible)
		{
			m_controller->put_IsVisible(TRUE);
		}
		// 前端组件已挂载并开始监听，确保窗口以设定的透明度丝滑呈现
		float alpha = m_note.opacityEnabled ? static_cast<float>(m_note.opacity) : 100.0f;
		SetWindowAlpha(alpha);
	});

	// 16. RestoreDock (Edge Dock Restore)
	RegisterHandler(NoteHostMessageType::RestoreDock, [this](const NoteHostMessage&) {
		if (m_controller) m_controller->put_IsVisible(TRUE);
		RestoreFromEdge();
	});
}
