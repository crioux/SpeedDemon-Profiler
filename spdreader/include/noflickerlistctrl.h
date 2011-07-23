#ifndef __INC_NOFLICKERLISTCTRL_H
#define __INC_NOFLICKERLISTCTRL_H


class CNoFlickerListCtrl:public wxListCtrl
{
public:
	CNoFlickerListCtrl();
	CNoFlickerListCtrl(wxWindow* parent, wxWindowID id, 
		const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, 
		long style = wxLC_REPORT|wxCLIP_CHILDREN|wxNO_FULL_REPAINT_ON_RESIZE, 
		const wxValidator& validator = wxDefaultValidator, const wxString& name = wxT("noflickerlistctrl"));
	bool Create(wxWindow* parent, wxWindowID id, 
		const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, 
		long style = wxLC_REPORT|wxCLIP_CHILDREN|wxNO_FULL_REPAINT_ON_RESIZE, 
		const wxValidator& validator = wxDefaultValidator, const wxString& name = wxT("noflickerlistctrl"));

protected:
	DECLARE_EVENT_TABLE()
	void OnEraseBackground(wxEraseEvent &evt);
};


#endif