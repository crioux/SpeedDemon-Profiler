/////////////////////////////////////////////////////////////////////////////
// Name:        ColumnsDlg.h
// Author:      XX
// Created:     XX/XX/XX
// Copyright:   XX
/////////////////////////////////////////////////////////////////////////////

#ifndef __ColumnsDlg_H__
#define __ColumnsDlg_H__

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
    #pragma interface "ColumnsDlg.cpp"
#endif

#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "spdreader_wdr.h"

// WDR: class declarations

//----------------------------------------------------------------------------
// CColumnsDlg
//----------------------------------------------------------------------------

class CColumnsDlg: public wxDialog
{
public:
    // constructors and destructors
    CColumnsDlg( wxWindow *parent, wxWindowID id, const wxString &title,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER  );
    
    // WDR: method declarations for CColumnsDlg
    wxCheckListBox* GetColumns()  { return (wxCheckListBox*) FindWindow( ID_COLUMNS ); }
    
private:
    // WDR: member variable declarations for CColumnsDlg
    
private:
    // WDR: handler declarations for CColumnsDlg

private:
    DECLARE_EVENT_TABLE()
};




#endif
