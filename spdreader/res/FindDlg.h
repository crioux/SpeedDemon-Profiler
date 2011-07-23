/////////////////////////////////////////////////////////////////////////////
// Name:        FindDlg.h
// Author:      XX
// Created:     XX/XX/XX
// Copyright:   XX
/////////////////////////////////////////////////////////////////////////////

#ifndef __FindDlg_H__
#define __FindDlg_H__

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
    #pragma interface "FindDlg.cpp"
#endif

#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "spdreader_wdr.h"

// WDR: class declarations

//----------------------------------------------------------------------------
// CFindDlg
//----------------------------------------------------------------------------

class CFindDlg: public wxDialog
{
public:
    // constructors and destructors
    CFindDlg( wxWindow *parent, wxWindowID id, const wxString &title,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxDEFAULT_DIALOG_STYLE );
    
    // WDR: method declarations for CFindDlg
    wxButton* GetOk()  { return (wxButton*) FindWindow( wxID_OK ); }
    wxTextCtrl* GetFindText()  { return (wxTextCtrl*) FindWindow( ID_FIND_TEXT ); }
    
private:
    // WDR: member variable declarations for CFindDlg
    
private:
    // WDR: handler declarations for CFindDlg
    void OnFindTextChanged( wxCommandEvent &event );

private:
    DECLARE_EVENT_TABLE()
};




#endif
