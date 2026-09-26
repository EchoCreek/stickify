// ------------------------------------------------------------------
// Original Work Copyright (c) imlinhanchao
// https://github.com/imlinhanchao/sticky_notes
// Modified by EchoCreek (2026)
// Licensed under the Apache License, Version 2.0
// ------------------------------------------------------------------
#pragma once

namespace Easy {

class XFile
{
public:
	static bool ReadFile(CString sFile, CString& data);
	static bool WriteFile(CString sFile, CString data);
	static bool AppendFile(CString sFile, CString data);
};

}

