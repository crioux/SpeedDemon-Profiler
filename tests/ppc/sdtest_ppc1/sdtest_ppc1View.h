// sdtest_ppc1View.h : interface of the Csdtest_ppc1View class
//


#pragma once

class Csdtest_ppc1View : public CView
{
protected: // create from serialization only
	Csdtest_ppc1View();
	DECLARE_DYNCREATE(Csdtest_ppc1View)

// Attributes
public:
	Csdtest_ppc1Doc* GetDocument() const;

// Operations
public:

// Overrides
public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

protected:

// Implementation
public:
	virtual ~Csdtest_ppc1View();
#ifdef _DEBUG
	virtual void AssertValid() const;
#endif

protected:

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in sdtest_ppc1View.cpp
inline Csdtest_ppc1Doc* Csdtest_ppc1View::GetDocument() const
   { return reinterpret_cast<Csdtest_ppc1Doc*>(m_pDocument); }
#endif

