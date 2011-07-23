// sdtest_ppc1Doc.cpp : implementation of the Csdtest_ppc1Doc class
//

#include "stdafx.h"
#include "sdtest_ppc1.h"

#include "sdtest_ppc1Doc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// Csdtest_ppc1Doc

IMPLEMENT_DYNCREATE(Csdtest_ppc1Doc, CDocument)

BEGIN_MESSAGE_MAP(Csdtest_ppc1Doc, CDocument)
END_MESSAGE_MAP()

// Csdtest_ppc1Doc construction/destruction

Csdtest_ppc1Doc::Csdtest_ppc1Doc()
{
	// TODO: add one-time construction code here

}

Csdtest_ppc1Doc::~Csdtest_ppc1Doc()
{
}

BOOL Csdtest_ppc1Doc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}

// Csdtest_ppc1Doc serialization

#ifndef _WIN32_WCE_NO_ARCHIVE_SUPPORT
void Csdtest_ppc1Doc::Serialize(CArchive& ar)
{
	(ar);
}
#endif // !_WIN32_WCE_NO_ARCHIVE_SUPPORT


// Csdtest_ppc1Doc diagnostics

#ifdef _DEBUG
void Csdtest_ppc1Doc::AssertValid() const
{
	CDocument::AssertValid();
}
#endif //_DEBUG


// Csdtest_ppc1Doc commands

