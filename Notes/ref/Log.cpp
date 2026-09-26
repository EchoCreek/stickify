// ------------------------------------------------------------------
// Original Work Copyright (c) imlinhanchao
// https://github.com/imlinhanchao/sticky_notes
// Modified by EchoCreek (2026)
// Licensed under the Apache License, Version 2.0
// ------------------------------------------------------------------
#include "StdAfx.h"
#include "Log.h"

DWORD CLogApp::m_dwLogType = (DWORD)LOG_DEBUG;
LogLevel CLogApp::m_minFileLogLevel = LogLevel::Warn;
CString CLogApp::m_sPath = CLogApp::GetCurDirectory() + _T("error.log");

void CLogApp::Init(DWORD dwType, LogLevel minFileLevel/*=LogLevel::Warn*/, CString sPath/*=_T("")*/)
{
	m_dwLogType = dwType;
	m_minFileLogLevel = minFileLevel;
	if (sPath.IsEmpty()) sPath = GetCurDirectory() + _T("error.log");
	m_sPath = sPath;

	// Log rotation: if the log file exceeds 5 MB, delete it so it starts fresh.
	// This prevents unbounded growth during long-running debug sessions.
	if (m_dwLogType & LOG_FILE)
	{
		const DWORD kMaxLogBytes = 5 * 1024 * 1024; // 5 MB
		WIN32_FILE_ATTRIBUTE_DATA fa = {};
		if (GetFileAttributesEx(m_sPath, GetFileExInfoStandard, &fa))
		{
			ULONGLONG fileSize = ((ULONGLONG)fa.nFileSizeHigh << 32) | fa.nFileSizeLow;
			if (fileSize > kMaxLogBytes)
			{
				DeleteFile(m_sPath);
			}
		}
	}
}

CString CLogApp::GetCurDirectory()
{
	CString sModuleFile = _T("");
	GetModuleFileName(NULL, sModuleFile.GetBuffer(MAX_PATH), MAX_PATH);
	sModuleFile.ReleaseBuffer();
	if (sModuleFile != _T(""))
	{
		sModuleFile = sModuleFile.Left(sModuleFile.ReverseFind('\\') + 1);
	}

	return sModuleFile;
}

const TCHAR* CLogApp::GetLevelTag(LogLevel level)
{
	switch (level)
	{
	case LogLevel::Debug: return _T("[DEBUG]");
	case LogLevel::Info:  return _T("[INFO]");
	case LogLevel::Warn:  return _T("[WARN]");
	case LogLevel::Error: return _T("[ERROR]");
	case LogLevel::Fatal: return _T("[FATAL]");
	default:              return _T("[LOG]");
	}
}

CString CLogApp::LogV(LogLevel level, const TCHAR* pszFormat, va_list args)
{
	if (!pszFormat || !*pszFormat) return _T("");

	TCHAR szMsg[2048] = { 0 };
	_vsntprintf_s(szMsg, _countof(szMsg), _TRUNCATE, pszFormat, args);

	CString sLog(szMsg);
	CString sTime;
	SYSTEMTIME st;
	memset(&st, 0, sizeof(SYSTEMTIME));
	GetLocalTime(&st);
	sTime.Format(_T("%04d-%02d-%02d %02d:%02d:%02d.%03d"),
		st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

	const TCHAR* tag = GetLevelTag(level);
	CString formattedLine;
	formattedLine.Format(_T("%s %s %s\n"), sTime.GetString(), tag, sLog.GetString());

	// Only write to file if level meets or exceeds the minimum file log level threshold (e.g. Warn/Error/Fatal in Release)
	if ((m_dwLogType & LOG_FILE) && (level >= m_minFileLogLevel))
	{
		CStdioFile Logfile;
		if (Logfile.Open(m_sPath, CFile::modeCreate | CFile::modeWrite | CFile::modeNoTruncate))
		{
			Logfile.SeekToEnd();
			Logfile.WriteString(formattedLine);
			Logfile.Close();
		}
	}

	if (m_dwLogType & LOG_PRINT)
	{
		_tprintf(_T("%s"), formattedLine.GetString());
	}

	if (m_dwLogType & LOG_DEBUG)
	{
		OutputDebugString(formattedLine);
	}

	return sLog;
}

CString CLogApp::Log(LogLevel level, const TCHAR* pszFormat, ...)
{
	if (!pszFormat || !*pszFormat) return _T("");
	va_list vargs;
	va_start(vargs, pszFormat);
	CString res = LogV(level, pszFormat, vargs);
	va_end(vargs);
	return res;
}

CString CLogApp::Debug(const TCHAR* pszFormat, ...)
{
	if (!pszFormat || !*pszFormat) return _T("");
	va_list vargs;
	va_start(vargs, pszFormat);
	CString res = LogV(LogLevel::Debug, pszFormat, vargs);
	va_end(vargs);
	return res;
}

CString CLogApp::Info(const TCHAR* pszFormat, ...)
{
	if (!pszFormat || !*pszFormat) return _T("");
	va_list vargs;
	va_start(vargs, pszFormat);
	CString res = LogV(LogLevel::Info, pszFormat, vargs);
	va_end(vargs);
	return res;
}

CString CLogApp::Warn(const TCHAR* pszFormat, ...)
{
	if (!pszFormat || !*pszFormat) return _T("");
	va_list vargs;
	va_start(vargs, pszFormat);
	CString res = LogV(LogLevel::Warn, pszFormat, vargs);
	va_end(vargs);
	return res;
}

CString CLogApp::Error(const TCHAR* pszFormat, ...)
{
	if (!pszFormat || !*pszFormat) return _T("");
	va_list vargs;
	va_start(vargs, pszFormat);
	CString res = LogV(LogLevel::Error, pszFormat, vargs);
	va_end(vargs);
	return res;
}

CString CLogApp::Fatal(const TCHAR* pszFormat, ...)
{
	if (!pszFormat || !*pszFormat) return _T("");
	va_list vargs;
	va_start(vargs, pszFormat);
	CString res = LogV(LogLevel::Fatal, pszFormat, vargs);
	va_end(vargs);
	return res;
}

CString CLogApp::Write(const TCHAR* pszFormat, ...)
{
	if (!pszFormat || !*pszFormat) return _T("");
	va_list vargs;
	va_start(vargs, pszFormat);
	CString res = LogV(LogLevel::Info, pszFormat, vargs);
	va_end(vargs);
	return res;
}
