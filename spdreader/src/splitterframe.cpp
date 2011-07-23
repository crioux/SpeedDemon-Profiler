#include"spdreader_pch.h"
#include"spdreader.h"
#include"spdreaderdoc.h"
#include"spdreaderview.h"
#include"splitterframe.h"
#include"functionlistview.h"
#include"threadlistview.h"
#include"functiondetailview.h"

BEGIN_EVENT_TABLE(CSplitterFrame,wxDocChildFrame)
	EVT_SIZE(OnSize)
END_EVENT_TABLE()


CSplitterFrame::CSplitterFrame(wxDocument *pDoc, wxView *pView, wxFrame *pParent,const wxString &title):
	wxDocChildFrame(pDoc,pView,pParent,-1,title)
{
	m_pSplitter1=new wxSplitterWindow(this,-1,wxPoint(0,0),wxSize(400,400),wxSP_3D|wxSP_LIVE_UPDATE|wxSP_3DBORDER);
	m_pSplitter2=new wxSplitterWindow(m_pSplitter1,-1,wxPoint(0,0),wxSize(400,400),wxSP_3D|wxSP_LIVE_UPDATE|wxSP_3DBORDER);
	m_pFunctionListView=new CFunctionListView(m_pSplitter1);
	m_pThreadsListView=new CThreadListView(m_pSplitter2);
	m_pFunctionDetailView=new CFunctionDetailView(m_pSplitter2);

	m_pSplitter1->SplitHorizontally(m_pFunctionListView,m_pSplitter2);
	m_pSplitter2->SplitVertically(m_pThreadsListView,m_pFunctionDetailView);
}

void CSplitterFrame::OnSize(wxSizeEvent &evt)
{
	m_pSplitter1->SetSize(evt.GetSize());
}

