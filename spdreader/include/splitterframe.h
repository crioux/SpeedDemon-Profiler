#ifndef __INC_SPLITTERFRAME_H
#define __INC_SPLITTERFRAME_H

class CFunctionListView;
class CThreadListView;
class CFunctionDetailView;


class CSplitterFrame:public wxDocChildFrame
{

protected:
	wxSplitterWindow *m_pSplitter1;
	wxSplitterWindow *m_pSplitter2;

	CFunctionListView *m_pFunctionListView;
	CThreadListView *m_pThreadsListView;
	CFunctionDetailView *m_pFunctionDetailView;
		
public:
	CSplitterFrame(wxDocument *pDoc, wxView *pView, wxFrame *pParent,const wxString &title);

protected:
	DECLARE_EVENT_TABLE()
	void OnSize(wxSizeEvent &evt);

};

#endif