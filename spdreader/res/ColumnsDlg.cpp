#include"spdreader_pch.h"
#include "ColumnsDlg.h"

// WDR: class implementations

//----------------------------------------------------------------------------
// CColumnsDlg
//----------------------------------------------------------------------------

// WDR: event table for CColumnsDlg

BEGIN_EVENT_TABLE(CColumnsDlg,wxDialog)
END_EVENT_TABLE()

CColumnsDlg::CColumnsDlg( wxWindow *parent, wxWindowID id, const wxString &title,
    const wxPoint &position, const wxSize& size, long style ) :
    wxDialog( parent, id, title, position, size, style )
{
    // WDR: dialog function ColumnsDlgFunc for CColumnsDlg
    ColumnsDlgFunc( this, TRUE ); 
}

// WDR: handler implementations for CColumnsDlg




