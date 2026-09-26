
// ------------------------------------------------------------------
// Original Work Copyright (c) imlinhanchao
// https://github.com/imlinhanchao/sticky_notes
// Modified by Sticky Notes Refactoring Team (2026): Switched to MainControlPanel host
// Licensed under the Apache License, Version 2.0
// ------------------------------------------------------------------
// Notes.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "Notes.h"
#include "host/MainControlPanel.h"
#include "host/WebViewEnvironmentManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// ------------------------------------------------------------------
// GPU 节能与兼容性调度策略：
// 明确指示 NVIDIA Optimus 与 AMD PowerXpress 显卡驱动：
// Stickify 作为轻量桌面便签工具，优先使用集成显卡（核显）运行，避免唤醒高性能独显造成发热与电量消耗。
// 纯独显/纯核显/虚拟机系统会自动忽略此标志，完全透明兼容。
// ------------------------------------------------------------------
extern "C" {
	__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000000;
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 0;
}

// CNotesApp

BEGIN_MESSAGE_MAP(CNotesApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CNotesApp construction

CNotesApp::CNotesApp()
{
	// support Restart Manager
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;

	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only CNotesApp object

CNotesApp theApp;


#include <DbgHelp.h>
#pragma comment(lib, "dbghelp.lib")

static bool s_bIsExiting = false;

void SetAppExiting(bool exiting)
{
	s_bIsExiting = exiting;
}

bool IsAppExiting()
{
	return s_bIsExiting;
}

static LONG WINAPI GlobalUnhandledExceptionFilter(EXCEPTION_POINTERS* pExp)
{
	// 进程已进入正常退出销毁阶段（ExitProcess/CRT cleanup），子线程异步终结异常不属于应用业务崩溃，直接放行终止
	if (s_bIsExiting)
	{
		return EXCEPTION_EXECUTE_HANDLER;
	}

	if (pExp && pExp->ExceptionRecord)
	{
		CLogApp::Fatal(_T("FATAL EXCEPTION: Code=0x%08X, Address=0x%p"),
			pExp->ExceptionRecord->ExceptionCode,
			pExp->ExceptionRecord->ExceptionAddress);

		// 自动生成 MiniDump 供深度诊断分析
		TCHAR szDumpPath[MAX_PATH];
		GetModuleFileName(NULL, szDumpPath, MAX_PATH);
		TCHAR* pSlash = _tcsrchr(szDumpPath, _T('\\'));
		if (pSlash) *(pSlash + 1) = _T('\0');
		_tcscat_s(szDumpPath, _T("crash.dmp"));

		HANDLE hFile = CreateFile(szDumpPath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile != INVALID_HANDLE_VALUE)
		{
			MINIDUMP_EXCEPTION_INFORMATION mei;
			mei.ThreadId = GetCurrentThreadId();
			mei.ExceptionPointers = pExp;
			mei.ClientPointers = FALSE;
			MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &mei, NULL, NULL);
			CloseHandle(hFile);
		}
	}
	return EXCEPTION_CONTINUE_SEARCH;
}

// CNotesApp initialization

BOOL CNotesApp::InitInstance()
{
	// 启用 Windows 原生 Per-Monitor V2 高 DPI 识别，彻底杜绝跨屏坐标偏移与缩放失真
	HMODULE hUser32 = GetModuleHandle(_T("user32.dll"));
	if (hUser32 != NULL)
	{
		typedef BOOL(WINAPI* PFN_SetProcessDpiAwarenessContext)(DPI_AWARENESS_CONTEXT);
		auto pfnSetProcessDpiAwarenessContext = reinterpret_cast<PFN_SetProcessDpiAwarenessContext>(
			GetProcAddress(hUser32, "SetProcessDpiAwarenessContext"));
		if (pfnSetProcessDpiAwarenessContext != nullptr)
		{
			pfnSetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
		}
	}

	// Release builds: emit to debugger (OutputDebugString), and only write Warn/Error/Fatal to error.log.
	// Debug builds  : enable all channels and record all log levels.
#ifdef _DEBUG
	CLogApp::Init(LOG_ALL, LogLevel::Debug);
#else
	CLogApp::Init(LOG_FILE | LOG_DEBUG, LogLevel::Warn);
#endif
	SetUnhandledExceptionFilter(GlobalUnhandledExceptionFilter);
	CLogApp::Info(_T("NotesApp::InitInstance: Starting up (Per-Monitor V2 DPI enabled)"));

	// 保证注册表存在正确的 .js MIME 类型，防止 WebView2 将 ES 模块识别为 text/plain 导致白屏
	HKEY hKey = NULL;
	if (RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\Classes\\.js"), 0, NULL, 0, KEY_SET_VALUE, NULL, &hKey, NULL) == ERROR_SUCCESS)
	{
		const TCHAR szMime[] = _T("text/javascript");
		RegSetValueEx(hKey, _T("Content Type"), 0, REG_SZ, (const BYTE*)szMime, (DWORD)(sizeof(szMime)));
		RegCloseKey(hKey);
	}

	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	AfxEnableControlContainer();

	// Standard initialization
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));

	m_pMainPanel = std::make_unique<MainControlPanel>();
	if (!m_pMainPanel->Create(MainControlPanel::IDD, nullptr))
	{
		CLogApp::Error(_T("NotesApp::InitInstance: Failed to create MainControlPanel"));
		return FALSE;
	}
	m_pMainWnd = m_pMainPanel.get();

	CLogApp::Info(_T("NotesApp::InitInstance: Entering standard message pump"));
	return TRUE;
}

int CNotesApp::ExitInstance()
{
	s_bIsExiting = true;
	CLogApp::Write(_T("NotesApp::ExitInstance: Exiting application"));
	m_pMainPanel.reset();
	WebViewEnvironmentManager::ReleaseEnvironment();
	int ret = CWinApp::ExitInstance();
	// 关键：主程序生命周期与所有对象已完成规范析构，立即通知 Windows 内核原子回收进程句柄，杜绝后台工作线程挂起/死锁
	::ExitProcess(0);
	return ret;
}

