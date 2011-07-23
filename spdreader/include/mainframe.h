#ifndef __INC_MAINFRAME_H
#define __INC_MAINFRAME_H

class CMainFrame: public wxDocParentFrame
{

public:
	CMainFrame(wxDocManager *manager, const wxPoint &pos, const wxSize &size, long style);
	~CMainFrame();

	bool OpenFromDevice(const wxString &strPathName);
	bool SaveReport(void);

protected:
	DECLARE_EVENT_TABLE()

	void OnSize(wxSizeEvent &size);
	void OnMove(wxMoveEvent &move);
	void OnHelpDocumentation(wxCommandEvent &evt);
	void OnHelpAbout(wxCommandEvent &evt);
	void OnViewDispAsPercUpdateUI(wxUpdateUIEvent &evt);
	void OnViewColumnsUpdateUI(wxUpdateUIEvent &evt);
	void OnViewBackUpdateUI(wxUpdateUIEvent &evt);
	void OnViewForwardUpdateUI(wxUpdateUIEvent &evt);
	void OnViewFocusUpdateUI(wxUpdateUIEvent &evt);
	void OnViewClearFocusUpdateUI(wxUpdateUIEvent &evt);
	void OnViewThreadFilterUpdateUI(wxUpdateUIEvent &evt);
	void OnViewFindUpdateUI(wxUpdateUIEvent &evt);
	void OnViewFindNextUpdateUI(wxUpdateUIEvent &evt);
	void OnFindTextUpdateUI(wxUpdateUIEvent &evt);
	void OnViewSourceUpdateUI(wxUpdateUIEvent &evt);
	void OnViewCloseUpdateUI(wxUpdateUIEvent &evt);
	void OnEditCutUpdateUI(wxUpdateUIEvent &evt);
	void OnEditCopyUpdateUI(wxUpdateUIEvent &evt);
	void OnEditPasteUpdateUI(wxUpdateUIEvent &evt);
#ifdef WIN32
	void OnImportFromDevice(wxCommandEvent &evt);
#endif
	void OnImportFromFile(wxCommandEvent &evt);
};


#endif