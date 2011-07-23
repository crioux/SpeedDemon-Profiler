#ifndef __INC_THREADLISTVIEW_H
#define __INC_THREADLISTVIEW_H

class CThreadListView:public wxTreeMultiCtrl
{
private:
	CSPDReaderDoc *m_pDocument;
	CSPDReaderView *m_pMainView;

public:
	CThreadListView(wxWindow* parent, CSPDReaderDoc *pDoc, CSPDReaderView *pMainView);
	
	void UpdateData(void);

protected:
	DECLARE_EVENT_TABLE()


};

#endif