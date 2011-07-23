// sdtest1Doc.cpp : implementation of the Csdtest1Doc class
//

#include "stdafx.h"
#include "sdtest1.h"

#include "sdtest1Doc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// Csdtest1Doc

IMPLEMENT_DYNCREATE(Csdtest1Doc, CDocument)

BEGIN_MESSAGE_MAP(Csdtest1Doc, CDocument)
END_MESSAGE_MAP()


// Csdtest1Doc construction/destruction

Csdtest1Doc::Csdtest1Doc()
{
	// TODO: add one-time construction code here

}

Csdtest1Doc::~Csdtest1Doc()
{
}

BOOL Csdtest1Doc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}




// Csdtest1Doc serialization

void Csdtest1Doc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}


// Csdtest1Doc diagnostics

#ifdef _DEBUG
void Csdtest1Doc::AssertValid() const
{
	CDocument::AssertValid();
}

void Csdtest1Doc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


// Csdtest1Doc commands
