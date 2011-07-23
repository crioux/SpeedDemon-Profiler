// sdtest1View.h : interface of the Csdtest1View class
//


#pragma once


class Csdtest1View : public CView
{
protected: // create from serialization only
	Csdtest1View();
	DECLARE_DYNCREATE(Csdtest1View)

// Attributes
public:
	Csdtest1Doc* GetDocument() const;

// Operations
public:

// Overrides
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);

// Implementation
public:
	virtual ~Csdtest1View();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in sdtest1View.cpp
inline Csdtest1Doc* Csdtest1View::GetDocument() const
   { return reinterpret_cast<Csdtest1Doc*>(m_pDocument); }
#endif

