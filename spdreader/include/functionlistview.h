#ifndef __INC_FUNCTIONLISTVIEW_H
#define __INC_FUNCTIONLISTVIEW_H


class CFunctionListView:public CNoFlickerListCtrl
{
private:
	CSPDReaderDoc *m_pDocument;
	CSPDReaderView *m_pMainView;

	wxListItemAttr m_row;
	wxListItemAttr m_rowshaded;

	int m_nSelectedFunction;
	bool m_bNeedsResize;
	bool m_bDragging;

public:
	CFunctionListView(wxWindow* parent, CSPDReaderDoc *pDoc, CSPDReaderView *pMainView);

	void UpdateData(void);
	void DoResize(void);

	void SetSelectedFunction(int funcnum);
	void UpdateVisibleColumns(void);


protected:
	virtual wxListItemAttr* OnGetItemAttr(long item) const;
	virtual int OnGetItemImage(long item) const;
	virtual wxString OnGetItemText(long item,long column) const;


protected:
	DECLARE_EVENT_TABLE()
	
	void OnClickColumn(wxListEvent &evt);
	void OnItemFocused(wxListEvent &evt);
	void OnIdle(wxIdleEvent &evt);
	void OnSize(wxSizeEvent &evt);
	void OnColBeginDrag(wxListEvent &evt);
	void OnColEndDrag(wxListEvent &evt);
	void OnListItemRightClick(wxListEvent &evt);
	void OnItemActivated(wxListEvent &evt);

};

#endif