// sdtest_ppc1View.cpp : implementation of the Csdtest_ppc1View class
//

#include "stdafx.h"
#include "sdtest_ppc1.h"

#include "sdtest_ppc1Doc.h"
#include "sdtest_ppc1View.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// Csdtest_ppc1View

IMPLEMENT_DYNCREATE(Csdtest_ppc1View, CView)

BEGIN_MESSAGE_MAP(Csdtest_ppc1View, CView)
END_MESSAGE_MAP()

// Csdtest_ppc1View construction/destruction

Csdtest_ppc1View::Csdtest_ppc1View()
{
	// TODO: add construction code here

}

Csdtest_ppc1View::~Csdtest_ppc1View()
{
}

BOOL Csdtest_ppc1View::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}


// Csdtest_ppc1View drawing
void Csdtest_ppc1View::OnDraw(CDC* /*pDC*/)
{
	Csdtest_ppc1Doc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	// TODO: add draw code for native data here
}



// Csdtest_ppc1View diagnostics

#ifdef _DEBUG
void Csdtest_ppc1View::AssertValid() const
{
	CView::AssertValid();
}

Csdtest_ppc1Doc* Csdtest_ppc1View::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(Csdtest_ppc1Doc)));
	return (Csdtest_ppc1Doc*)m_pDocument;
}
#endif //_DEBUG


// Csdtest_ppc1View message handlers
