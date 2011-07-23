#include"spdreader_pch.h"
#include"spdreader.h"
#include"spdreaderdoc.h"
#include"spdreaderview.h"
#include"threadlistview.h"

BEGIN_EVENT_TABLE(CThreadListView,wxTreeMultiCtrl)

END_EVENT_TABLE()


CThreadListView::CThreadListView(wxWindow* parent,CSPDReaderDoc *pDoc, CSPDReaderView *pMainView):
	wxTreeMultiCtrl(parent,-1,wxDefaultPosition,wxDefaultSize,wxNO_BORDER)
{
	m_pDocument=pDoc;
	m_pMainView=pMainView;
}

void CThreadListView::UpdateData(void)
{
	
}
	