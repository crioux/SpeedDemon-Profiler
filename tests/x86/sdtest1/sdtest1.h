// sdtest1.h : main header file for the sdtest1 application
//
#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols


// Csdtest1App:
// See sdtest1.cpp for the implementation of this class
//

class Csdtest1App : public CWinApp
{
public:
	Csdtest1App();


// Overrides
public:
	virtual BOOL InitInstance();

// Implementation
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
};

extern Csdtest1App theApp;