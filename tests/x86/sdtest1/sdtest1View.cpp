// sdtest1View.cpp : implementation of the Csdtest1View class
//

#include "stdafx.h"
#include "sdtest1.h"

#include "sdtest1Doc.h"
#include "sdtest1View.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// Csdtest1View

IMPLEMENT_DYNCREATE(Csdtest1View, CView)

BEGIN_MESSAGE_MAP(Csdtest1View, CView)
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
END_MESSAGE_MAP()

// Csdtest1View construction/destruction

Csdtest1View::Csdtest1View()
{
	// TODO: add construction code here

}

Csdtest1View::~Csdtest1View()
{
}

BOOL Csdtest1View::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}

// Csdtest1View drawing

void Csdtest1View::OnDraw(CDC* /*pDC*/)
{
	Csdtest1Doc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	// TODO: add draw code for native data here
}


// Csdtest1View printing

BOOL Csdtest1View::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void Csdtest1View::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void Csdtest1View::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}


// Csdtest1View diagnostics

#ifdef _DEBUG
void Csdtest1View::AssertValid() const
{
	CView::AssertValid();
}

void Csdtest1View::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

Csdtest1Doc* Csdtest1View::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(Csdtest1Doc)));
	return (Csdtest1Doc*)m_pDocument;
}
#endif //_DEBUG


// Csdtest1View message handlers
