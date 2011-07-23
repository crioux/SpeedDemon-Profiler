/////////////////////////////////////////////////////////////////////////////
// Name:        ThreadFilterDlg.h
// Author:      XX
// Created:     XX/XX/XX
// Copyright:   XX
/////////////////////////////////////////////////////////////////////////////

#ifndef __ThreadFilterDlg_H__
#define __ThreadFilterDlg_H__

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
    #pragma interface "ThreadFilterDlg.cpp"
#endif

#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "spdreader_wdr.h"

// WDR: class declarations

//----------------------------------------------------------------------------
// CThreadFilterDlg
//----------------------------------------------------------------------------

class CThreadFilterDlg: public wxDialog
{
public:
    // constructors and destructors
    CThreadFilterDlg( wxWindow *parent, wxWindowID id, const wxString &title,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxDEFAULT_DIALOG_STYLE );
    
    // WDR: method declarations for CThreadFilterDlg
    wxCheckListBox* GetThreads()  { return (wxCheckListBox*) FindWindow( ID_THREADS ); }
    
private:
    // WDR: member variable declarations for CThreadFilterDlg
    
private:
    // WDR: handler declarations for CThreadFilterDlg
    void OnClickDeselectAll( wxCommandEvent &event );
    void OnClickSelectAll( wxCommandEvent &event );

private:
    DECLARE_EVENT_TABLE()
};




#endif
