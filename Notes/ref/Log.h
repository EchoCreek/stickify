// ------------------------------------------------------------------
// Original Work Copyright (c) imlinhanchao
// https://github.com/imlinhanchao/sticky_notes
// Modified by EchoCreek (2026)
// Licensed under the Apache License, Version 2.0
// ------------------------------------------------------------------
#pragma once

typedef enum { 
	LOG_FILE = 1 << 0, 
	LOG_PRINT = 1 << 1, 
	LOG_DEBUG = 1 << 2, 
	LOG_LIST = 1 << 3, 
	LOG_ALL = 0xffffff
} LOG_TYPE;

enum class LogLevel {
	Debug = 0,
	Info  = 1,
	Warn  = 2,
	Error = 3,
	Fatal = 4
};

class CLogApp
{
public:
	CLogApp(void) = delete;
	~CLogApp(void) = delete;

	static void Init(DWORD dwType, LogLevel minFileLevel = LogLevel::Warn, CString sPath = _T(""));
	static void SetMinFileLogLevel(LogLevel level) { m_minFileLogLevel = level; }
	static LogLevel GetMinFileLogLevel() { return m_minFileLogLevel; }

	// Core logging methods with explicit levels
	static CString Debug(const TCHAR* pszFormat, ...);
	static CString Info(const TCHAR* pszFormat, ...);
	static CString Warn(const TCHAR* pszFormat, ...);
	static CString Error(const TCHAR* pszFormat, ...);
	static CString Fatal(const TCHAR* pszFormat, ...);

	// General/backward-compatible write method (defaults to Info level)
	static CString Write(const TCHAR* pszFormat, ...);

	// Direct variadic dispatch
	static CString Log(LogLevel level, const TCHAR* pszFormat, ...);
	static CString LogV(LogLevel level, const TCHAR* pszFormat, va_list args);

private:
	static CString GetCurDirectory();
	static const TCHAR* GetLevelTag(LogLevel level);

	static DWORD m_dwLogType;
	static LogLevel m_minFileLogLevel;
	static CString m_sPath;
};
