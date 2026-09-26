// ------------------------------------------------------------------
// Original Work Copyright (c) imlinhanchao
// https://github.com/imlinhanchao/sticky_notes
// Modified by EchoCreek (2026)
// Licensed under the Apache License, Version 2.0
// ------------------------------------------------------------------
#pragma once

namespace Easy
{

typedef struct _MODULE_VER { // Module version
	WORD wMajor;
	WORD wMinor;
	WORD wRevision;
	WORD wBuild;  

	CString ToString()
	{
		return Cvt::ToString(_T("%d.%d.%d"), wMajor, wMinor, wRevision);
	}
}MODULE_VER;

class Utility
{
public:
	static bool IsWow64();
	static MODULE_VER GetVersion(CString sModuleName);
	static void SetAutoRun(bool bAuto);
	static HANDLE ProgramLock(CString sInstanceName);
};

}
