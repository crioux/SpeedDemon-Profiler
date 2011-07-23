#ifndef __INC_SPDREADERVIEW_H
#define __INC_SPDREADERVIEW_H

class CFunctionListView;
class CThreadListView;
class CFunctionDetailView;
class CSPDReaderDoc;
class CCodeGraph;
class CFuncGraphDesc;
class wxFlatNotebook;
class wxFlatNotebookEvent;

class CSPDReaderView:public wxView
{
	friend class CFunctionListView;
	friend class CThreadListView;
	friend class CFunctionDetailView;
	friend class CSPDReaderDoc;
	friend class CCodeGraph;

	DECLARE_DYNAMIC_CLASS(CSPDReaderView)
public:
	CSPDReaderView();
	~CSPDReaderView();

	CSPDReaderDoc *GetDoc() { return (CSPDReaderDoc *)GetDocument(); }

	void SetSelectedFunction(int funcnum, bool bUpdateHistory);
	int GetSelectedFunction(void);
	void GoBack(void);
	void GoForward(void);

	void SetUseMilliseconds(bool bUseMsec);
	bool GetUseMilliseconds(void);
	void SetDispAsPerc(bool bDispAsPerc);
	bool GetDispAsPerc(void);

	double ConvertTime(wxUint64 ticks) const;
	wxString GetTimeString(wxUint64 ticks) const;

protected:
	wxFlatNotebook *m_pNotebook;
	wxSplitterWindow *m_pSplitter1;
	wxSplitterWindow *m_pSplitter2;
	int m_nSelectedFunction;
	bool m_bUseMilliseconds;
	bool m_bDispAsPerc;
	std::deque<int> m_llHistory;
	int m_nHistoryPosition;
	wxString m_strFindText;
	int m_nFindPosition;
	CFuncGraphDesc *m_pGraphDesc;
	std::map<wxString,wxWindow *> m_openFiles;

	enum COLUMNS
	{			
		NAME=0,
		SOURCE,
		NUMOFCALLS,
		MINFTIME,
		AVGFTIME,
		MAXFTIME,
		MINFCTIME,
		AVGFCTIME,
		MAXFCTIME,
		TOTALFTIME,
		TOTALFCTIME,
		AVGFOVERSIZE,
	};
	std::vector<COLUMNS> m_columnordering;

	CSPDReaderDoc *m_pDocument;


	CFunctionListView *m_pFunctionListView;
	CThreadListView *m_pThreadsListView;
	CFunctionDetailView *m_pFunctionDetailView;
	CCodeGraph *m_pCodeGraph;

	wxBitmapButton *m_pCloseButton;

	virtual bool OnCreate(wxDocument *doc, long flags);
	virtual void OnDraw(wxDC *dc);
	virtual void OnUpdate(wxView *sender, wxObject *hint = (wxObject *) NULL);

	void DoFind(void);
	void OpenSource(wxUint32 funcid);

protected:
	DECLARE_EVENT_TABLE()
	
	void OnSize(wxSizeEvent &evt);
	void OnViewDispAsPerc(wxCommandEvent &evt);
	void OnViewDispAsPercUpdateUI(wxUpdateUIEvent &evt);
	void OnViewColumns(wxCommandEvent &evt);
	void OnViewColumnsUpdateUI(wxUpdateUIEvent &evt);
	void OnViewBack(wxCommandEvent &evt);
	void OnViewForward(wxCommandEvent &evt);
	void OnViewBackUpdateUI(wxUpdateUIEvent &evt);
	void OnViewForwardUpdateUI(wxUpdateUIEvent &evt);
	void OnViewFocus(wxCommandEvent &evt);
	void OnViewClearFocus(wxCommandEvent &evt);
	void OnViewFocusUpdateUI(wxUpdateUIEvent &evt);
	void OnViewClearFocusUpdateUI(wxUpdateUIEvent &evt);
	void OnViewThreadFilter(wxCommandEvent &evt);
	void OnViewThreadFilterUpdateUI(wxUpdateUIEvent &evt);
	void OnFindText(wxCommandEvent &evt);
	void OnFindTextUpdateUI(wxUpdateUIEvent &evt);
	void OnViewFind(wxCommandEvent &evt);
	void OnViewFindNext(wxCommandEvent &evt);
	void OnViewFindUpdateUI(wxUpdateUIEvent &evt);
	void OnViewFindNextUpdateUI(wxUpdateUIEvent &evt);
	void OnViewSource(wxCommandEvent &evt);
	void OnViewSourceUpdateUI(wxUpdateUIEvent &evt);
	void OnNotebookPageChanged(wxFlatNotebookEvent &evt);
	void OnNotebookPageClosing(wxFlatNotebookEvent &evt);
	void OnCloseButton(wxCommandEvent &evt);
	void OnViewCloseUpdateUI(wxUpdateUIEvent &evt);
	void OnEditCutUpdateUI(wxUpdateUIEvent &evt);
	void OnEditCopyUpdateUI(wxUpdateUIEvent &evt);
	void OnEditPasteUpdateUI(wxUpdateUIEvent &evt);
};

extern const wxChar *g_colnames[];



#endif