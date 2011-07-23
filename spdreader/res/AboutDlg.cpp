#include"spdreader_pch.h"
#include "AboutDlg.h"

// WDR: class implementations

//----------------------------------------------------------------------------
// CAboutDlg
//----------------------------------------------------------------------------

// WDR: event table for CAboutDlg

BEGIN_EVENT_TABLE(CAboutDlg,wxDialog)
END_EVENT_TABLE()

CAboutDlg::CAboutDlg( wxWindow *parent, wxWindowID id, const wxString &title,
    const wxPoint &position, const wxSize& size, long style ) :
    wxDialog( parent, id, title, position, size, style )
{
    // WDR: dialog function AboutDlgFunc for CAboutDlg
    AboutDlgFunc( this, TRUE ); 
}

// WDR: handler implementations for CAboutDlg




