// ------------------------------------------------------------------
// Original Work Copyright (c) imlinhanchao
// https://github.com/imlinhanchao/sticky_notes
// Modified by [Refactor] (2026): P0 - UTF-8 encoding, warning cleanup.
// Licensed under the Apache License, Version 2.0
// ------------------------------------------------------------------
#pragma once

#include <afxwin.h>
#include <atltime.h>
#include <vector>
#include <cstdint>

#ifndef _ttof
#define _ttof _tstof
#endif

namespace Easy {

class Cvt
{
public:
	static COleDateTime ToDateTime(CString sDateTime);  // 'YYYY-MM-DD HH:mm:SS' -> DateTime

	static CString ToString(bool     bValue);  // true -> '1'; false -> '0';
	static CString ToString(int      nValue);	
	static CString ToString(long     nValue);
	static CString ToString(UINT     nValue);
	static CString ToString(DWORD    dwValue);
	static CString ToString(uint64_t nValue);
	static CString ToString(float    fValue, int nPrecision=0); // P=0 -> No limited; P=1 -> '0.0'; P=2 -> '0.00'; ...
	static CString ToString(double   fValue, int nPrecision=0);	
	static CString ToString(COleDateTime date);  // DateTime -> 'YYYY-MM-DD HH:mm:SS'
	static CString ToString(const TCHAR* pszFormat, ...);
	static CString ToString(CTime time, CString sFormat=_T("%Y-%m-%d %H:%M:%S"));
	static vector<CString> SplitString(CString sData, CString sSp);
	static CString ToHex(COLORREF color);
	static COLORREF ToColor(CString sHex);

};

}