// sdtest_ppc1Doc.h : interface of the Csdtest_ppc1Doc class
//


#pragma once

class Csdtest_ppc1Doc : public CDocument
{
protected: // create from serialization only
	Csdtest_ppc1Doc();
	DECLARE_DYNCREATE(Csdtest_ppc1Doc)

// Attributes
public:

// Operations
public:

// Overrides
public:
	virtual BOOL OnNewDocument();
#ifndef _WIN32_WCE_NO_ARCHIVE_SUPPORT
	virtual void Serialize(CArchive& ar);
#endif // !_WIN32_WCE_NO_ARCHIVE_SUPPORT

// Implementation
public:
	virtual ~Csdtest_ppc1Doc();
#ifdef _DEBUG
	virtual void AssertValid() const;
#endif

protected:

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
};


