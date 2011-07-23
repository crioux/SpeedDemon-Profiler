#ifndef __INC_FUNCTIONDETAILVIEW_H
#define __INC_FUNCTIONDETAILVIEW_H

class CFunctionDetailView:public wxPanel
{
private:
	
	CSPDReaderDoc *m_pDocument;
	CSPDReaderView *m_pMainView;
	int m_nSelectedFunction;
	bool m_bNeedsResize;
	int m_nParentsSortCol;
	int m_nChildrenSortCol;
	bool m_bParentsSortDir;
	bool m_bChildrenSortDir;
	
public:
	CFunctionDetailView(wxWindow* parent, CSPDReaderDoc *pDoc, CSPDReaderView *pMainView);
	~CFunctionDetailView();

	void UpdateData(void);
	void SetSelectedFunction(int funcnum);
	void ClearLists(void);
	
	wxBitmap MakePercentageGraph(double perc);
	void DoResize(void);

protected:
	DECLARE_EVENT_TABLE()
	void OnListItemActivatedParents(wxListEvent &evt);
	void OnListItemActivatedChildren(wxListEvent &evt);
	void OnIdle(wxIdleEvent &evt);
	void OnSize(wxSizeEvent &evt);
	void OnDispAsPerc(wxCommandEvent &evt);
	void OnListColClickParents(wxListEvent &evt);
	void OnListColClickChildren(wxListEvent &evt);

	wxListCtrl *GetParentsList() { return (wxListCtrl *)FindWindow(ID_PARENTS_LIST); }
	wxListCtrl *GetChildrenList() { return (wxListCtrl *)FindWindow(ID_CHILDREN_LIST); }
	wxTextCtrl *GetName() { return (wxTextCtrl *)FindWindow(ID_NAME); }
	wxTextCtrl *GetNumOfCalls() { return (wxTextCtrl *)FindWindow(ID_NUM_OF_CALLS); }
	wxTextCtrl *GetSourceLoc() { return (wxTextCtrl *)FindWindow(ID_SOURCE_LOC); }
	wxTextCtrl *GetAvgFTime() { return (wxTextCtrl *)FindWindow(ID_AVG_F_TIME); }
	wxTextCtrl *GetMaxFTime() { return (wxTextCtrl *)FindWindow(ID_MAX_F_TIME); }
	wxTextCtrl *GetMinFTime() { return (wxTextCtrl *)FindWindow(ID_MIN_F_TIME); }
	wxTextCtrl *GetTotalFTime() { return (wxTextCtrl *)FindWindow(ID_TOTAL_F_TIME); }
	wxTextCtrl *GetAvgFCTime() { return (wxTextCtrl *)FindWindow(ID_AVG_FC_TIME); }
	wxTextCtrl *GetMaxFCTime() { return (wxTextCtrl *)FindWindow(ID_MAX_FC_TIME); }
	wxTextCtrl *GetMinFCTime() { return (wxTextCtrl *)FindWindow(ID_MIN_FC_TIME); }
	wxTextCtrl *GetTotalFCTime() { return (wxTextCtrl *)FindWindow(ID_TOTAL_FC_TIME); }
	wxStaticBitmap *GetFPerc() { return (wxStaticBitmap *)FindWindow(ID_F_PERC); }
	wxStaticBitmap *GetFCPerc() { return (wxStaticBitmap *)FindWindow(ID_FC_PERC); }

	static int wxCALLBACK ParentsItemSort(long item1, long item2, long sortData);
	static int wxCALLBACK ChildrenItemSort(long item1, long item2, long sortData);

};

#endif