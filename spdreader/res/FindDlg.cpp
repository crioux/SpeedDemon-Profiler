#include "spdreader_pch.h"
#include "FindDlg.h"

// WDR: class implementations

//----------------------------------------------------------------------------
// CFindDlg
//----------------------------------------------------------------------------

// WDR: event table for CFindDlg

BEGIN_EVENT_TABLE(CFindDlg,wxDialog)
    EVT_TEXT( ID_FIND_TEXT, CFindDlg::OnFindTextChanged )
END_EVENT_TABLE()

CFindDlg::CFindDlg( wxWindow *parent, wxWindowID id, const wxString &title,
    const wxPoint &position, const wxSize& size, long style ) :
    wxDialog( parent, id, title, position, size, style )
{
    // WDR: dialog function FindDlgFunc for CFindDlg
    FindDlgFunc( this, TRUE ); 
}

// WDR: handler implementations for CFindDlg

void CFindDlg::OnFindTextChanged( wxCommandEvent &event )
{
	GetOk()->Enable(!GetFindText()->GetValue().IsEmpty());
}




