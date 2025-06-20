
// mvsToSlpkUI.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CmvsToSlpkUIApp:
// See mvsToSlpkUI.cpp for the implementation of this class
//

class CmvsToSlpkUIApp : public CWinApp
{
public:
	CmvsToSlpkUIApp();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()

private:
	HANDLE mhMutex = nullptr;
	bool checkForOtherInstances(const CString& title);
public:
	virtual int ExitInstance();
};

extern CmvsToSlpkUIApp theApp;
