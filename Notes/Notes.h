
// ------------------------------------------------------------------
// Original Work Copyright (c) imlinhanchao
// https://github.com/imlinhanchao/sticky_notes
// Modified by Sticky Notes Refactoring Team (2026)
// Licensed under the Apache License, Version 2.0
// ------------------------------------------------------------------
// Notes.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols
#include <memory>


// CNotesApp:
// See Notes.cpp for the implementation of this class
//

void SetAppExiting(bool exiting);
bool IsAppExiting();

class MainControlPanel;

class CNotesApp : public CWinApp
{
public:
	CNotesApp();

// Overrides
public:
	virtual BOOL InitInstance() override;
	virtual int ExitInstance() override;

// Implementation

	DECLARE_MESSAGE_MAP()

private:
	std::unique_ptr<MainControlPanel> m_pMainPanel;
};

extern CNotesApp theApp;