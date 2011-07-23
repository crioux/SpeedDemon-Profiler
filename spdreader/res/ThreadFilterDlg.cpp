#include"spdreader_pch.h"
#include"ThreadFilterDlg.h"


//----------------------------------------------------------------------------
// CThreadFilterDlg
//----------------------------------------------------------------------------

// WDR: event table for CThreadFilterDlg

BEGIN_EVENT_TABLE(CThreadFilterDlg,wxDialog)
    EVT_BUTTON( ID_SELECT_ALL, CThreadFilterDlg::OnClickSelectAll )
    EVT_BUTTON( ID_DESELECT_ALL, CThreadFilterDlg::OnClickDeselectAll )
END_EVENT_TABLE()

CThreadFilterDlg::CThreadFilterDlg( wxWindow *parent, wxWindowID id, const wxString &title,
    const wxPoint &position, const wxSize& size, long style ) :
    wxDialog( parent, id, title, position, size, style )
{
    // WDR: dialog function ThreadFilterDlgFunc for CThreadFilterDlg
    ThreadFilterDlgFunc( this, TRUE ); 
}

// WDR: handler implementations for CThreadFilterDlg

void CThreadFilterDlg::OnClickDeselectAll( wxCommandEvent &event )
{
    for(int i=0;i<GetThreads()->GetCount();i++)
	{
		GetThreads()->Check(i,false);
	}
}

void CThreadFilterDlg::OnClickSelectAll( wxCommandEvent &event )
{
	for(int i=0;i<GetThreads()->GetCount();i++)
	{
		GetThreads()->Check(i,true);
	}
}




