/////////////////////////////////////////////////////////////////////////////
// Name:        AboutDlg.h
// Author:      XX
// Created:     XX/XX/XX
// Copyright:   XX
/////////////////////////////////////////////////////////////////////////////

#ifndef __AboutDlg_H__
#define __AboutDlg_H__

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
    #pragma interface "AboutDlg.cpp"
#endif

#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "spdreader_wdr.h"

// WDR: class declarations

//----------------------------------------------------------------------------
// CAboutDlg
//----------------------------------------------------------------------------

class CAboutDlg: public wxDialog
{
public:
    // constructors and destructors
    CAboutDlg( wxWindow *parent, wxWindowID id, const wxString &title,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxDEFAULT_DIALOG_STYLE );
    
    // WDR: method declarations for CAboutDlg
    
private:
    // WDR: member variable declarations for CAboutDlg
    
private:
    // WDR: handler declarations for CAboutDlg

private:
    DECLARE_EVENT_TABLE()
};




#endif
