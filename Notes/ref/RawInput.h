// ------------------------------------------------------------------
// Original Work Copyright (c) imlinhanchao
// https://github.com/imlinhanchao/sticky_notes
// Modified by [Refactor] (2026): P0 - UTF-8 encoding, warning cleanup.
// Licensed under the Apache License, Version 2.0
// ------------------------------------------------------------------
#pragma once

namespace Easy {

enum RAW_TYPE
{
	RAW_TYPE_HID = 0x01,
	RAW_TYPE_KB = 0x02,
	RAW_TYPE_MS = 0x04,
};

class CRawInput
{
	CRawInput();
	~CRawInput();

public:
	static bool Register(HWND hWnd, WORD wRawType);
	static bool Remove(HWND hWnd, WORD wRawType);

private:
	static bool SetRawInput(HWND hWnd, WORD wRawType, bool bRegister);
};

}