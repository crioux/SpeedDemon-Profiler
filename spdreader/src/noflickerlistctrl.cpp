#include"spdreader_pch.h"

BEGIN_EVENT_TABLE(CNoFlickerListCtrl,wxListCtrl)
	EVT_ERASE_BACKGROUND(OnEraseBackground)
END_EVENT_TABLE()


CNoFlickerListCtrl::CNoFlickerListCtrl():wxListCtrl()
{
}

CNoFlickerListCtrl::CNoFlickerListCtrl(wxWindow* parent, wxWindowID id, const wxPoint& pos , const wxSize& size, long style, const wxValidator& validator, const wxString& name)
	:wxListCtrl(parent,id,pos,size,style,validator,name)
{
}

bool CNoFlickerListCtrl::Create(wxWindow* parent, wxWindowID id, const wxPoint& pos , const wxSize& size, long style, const wxValidator& validator, const wxString& name)
{
	return wxListCtrl::Create(parent,id,pos,size,style,validator,name);
}


void CNoFlickerListCtrl::OnEraseBackground(wxEraseEvent &evt)
{
	//wxDC *pdc=evt.GetDC();
	

	evt.Skip();
}