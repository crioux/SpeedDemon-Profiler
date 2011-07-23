// sdtest_ppc1.h : main header file for the sdtest_ppc1 application
//
#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resourceppc.h"

// Csdtest_ppc1App:
// See sdtest_ppc1.cpp for the implementation of this class
//

class Csdtest_ppc1App : public CWinApp
{
public:
	Csdtest_ppc1App();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation
public:
	afx_msg void OnAppAbout();

	DECLARE_MESSAGE_MAP()
};

extern Csdtest_ppc1App theApp;
