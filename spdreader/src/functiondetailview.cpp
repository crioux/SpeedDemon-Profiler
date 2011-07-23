#include"spdreader_pch.h"
#include"spdreader.h"
#include"spdreaderdoc.h"
#include"spdreaderview.h"
#include"functiondetailview.h"

BEGIN_EVENT_TABLE(CFunctionDetailView,wxPanel)
	EVT_LIST_ITEM_ACTIVATED(ID_PARENTS_LIST,OnListItemActivatedParents)
	EVT_LIST_ITEM_ACTIVATED(ID_CHILDREN_LIST,OnListItemActivatedChildren)
	EVT_LIST_COL_CLICK(ID_PARENTS_LIST,OnListColClickParents)
	EVT_LIST_COL_CLICK(ID_CHILDREN_LIST,OnListColClickChildren)
	EVT_SIZE(OnSize)
	EVT_IDLE(OnIdle)
END_EVENT_TABLE()

class CListItemData
{
public:
	CListItemData(wxString _title,wxUint32 _numcalls,double _perctime,wxUint32 _funcnum):
	  title(_title),numcalls(_numcalls),perctime(_perctime),funcnum(_funcnum) {}

	wxString title;
	wxUint32 numcalls;
	double perctime;
	wxUint32 funcnum;
};

CFunctionDetailView::CFunctionDetailView(wxWindow* parent,CSPDReaderDoc *pDoc, CSPDReaderView *pMainView):
	wxPanel(parent,wxID_ANY,wxDefaultPosition,wxDefaultSize,wxTAB_TRAVERSAL|wxNO_BORDER|wxCLIP_CHILDREN|wxNO_FULL_REPAINT_ON_RESIZE )
{
	m_pDocument=pDoc;
	m_pMainView=pMainView;
	m_nSelectedFunction=-1;

	FunctionDetailPanel(this,true,true);
	SetSizeHints(16,16,-1,-1,-1,-1);

	GetParentsList()->InsertColumn(0,wxT("Function Name"),wxLIST_FORMAT_LEFT,16);
	GetParentsList()->InsertColumn(1,wxT("# Of Calls"),wxLIST_FORMAT_LEFT,75);
	GetParentsList()->InsertColumn(2,wxT("% Of Time"),wxLIST_FORMAT_LEFT,75);
	
	GetChildrenList()->InsertColumn(0,wxT("Function Name"),wxLIST_FORMAT_LEFT,16);
	GetChildrenList()->InsertColumn(1,wxT("# Of Calls"),wxLIST_FORMAT_LEFT,75);
	GetChildrenList()->InsertColumn(2,wxT("% Of Time"),wxLIST_FORMAT_LEFT,75);

	m_bNeedsResize=true;
	m_nParentsSortCol=2;
	m_nChildrenSortCol=2;
	m_bParentsSortDir=true;
	m_bChildrenSortDir=true;
}

CFunctionDetailView::~CFunctionDetailView()
{
	ClearLists();
}

void CFunctionDetailView::ClearLists(void)
{
	int i;
	for(i=0;i<GetParentsList()->GetItemCount();i++)
	{
		delete (CListItemData *)(size_t)(GetParentsList()->GetItemData(i));
	}
	GetParentsList()->DeleteAllItems();

	for(i=0;i<GetChildrenList()->GetItemCount();i++)
	{
		delete (CListItemData *)(size_t)(GetChildrenList()->GetItemData(i));
	}
	GetChildrenList()->DeleteAllItems();
}

void CFunctionDetailView::DoResize(void)
{
	int wid=0;
	for(int i=1;i<=2;i++)
	{
		wid+=(GetParentsList()->GetColumnWidth(i)+2);
	}
	GetParentsList()->SetColumnWidth(0,GetParentsList()->GetSize().GetWidth()-wid);

	wid=0;
	for(int i=1;i<=2;i++)
	{
		wid+=(GetChildrenList()->GetColumnWidth(i)+2);
	}
	GetChildrenList()->SetColumnWidth(0,GetChildrenList()->GetSize().GetWidth()-wid);
}

void CFunctionDetailView::UpdateData(void)
{
	if(m_nSelectedFunction==-1)
	{
		ClearLists();

		GetName()->Clear();
		GetNumOfCalls()->Clear();
		GetSourceLoc()->Clear();
		GetAvgFTime()->Clear();
		GetMaxFTime()->Clear();
		GetMinFTime()->Clear();
		GetTotalFTime()->Clear();
		GetAvgFCTime()->Clear();
		GetMaxFCTime()->Clear();
		GetMinFCTime()->Clear();
		GetTotalFCTime()->Clear();
		GetFPerc()->SetBitmap(NullBitmapsFunc(0));
		GetFCPerc()->SetBitmap(NullBitmapsFunc(0));
		
		GetParentsList()->Disable();
		GetChildrenList()->Disable();
		GetName()->Disable();
		GetNumOfCalls()->Disable();
		GetSourceLoc()->Disable();
		GetAvgFTime()->Disable();
		GetMaxFTime()->Disable();
		GetMinFTime()->Disable();
		GetTotalFTime()->Disable();
		GetAvgFCTime()->Disable();
		GetMaxFCTime()->Disable();
		GetMinFCTime()->Disable();
		GetTotalFCTime()->Disable();
		GetFPerc()->Disable();
		GetFCPerc()->Disable();
		return;
	}
	
	CFunctionData *pfd=&(m_pDocument->m_arrFunctionData[m_nSelectedFunction]);

	GetParentsList()->Enable();
	GetChildrenList()->Enable();
	GetName()->Enable();
	GetNumOfCalls()->Enable();
	GetSourceLoc()->Enable();
	GetAvgFTime()->Enable();
	GetMaxFTime()->Enable();
	GetMinFTime()->Enable();
	GetTotalFTime()->Enable();
	GetAvgFCTime()->Enable();
	GetMaxFCTime()->Enable();
	GetMinFCTime()->Enable();
	GetTotalFCTime()->Enable();
	GetFPerc()->Enable();
	GetFCPerc()->Enable();

	GetName()->SetValue(pfd->Desc.strFullName);
	GetNumOfCalls()->SetValue(wxString::Format(LLFMT,pfd->FilteredData.Stats.Count));
	GetSourceLoc()->SetValue(wxString::Format(wxT("%s, Lines %d-%d"),(const TCHAR *)pfd->Desc.strSourceFile,pfd->Desc.nFirstSourceLine,pfd->Desc.nLastSourceLine));
	GetAvgFTime()->SetValue(m_pMainView->GetTimeString(pfd->FilteredData.Stats.AverageFunctionTime));
	GetMaxFTime()->SetValue(m_pMainView->GetTimeString(pfd->FilteredData.Stats.MaximumFunctionTime));
	GetMinFTime()->SetValue(m_pMainView->GetTimeString(pfd->FilteredData.Stats.MinimumFunctionTime));
	GetTotalFTime()->SetValue(m_pMainView->GetTimeString(pfd->FilteredData.Stats.TotalFunctionTime));
	GetAvgFCTime()->SetValue(m_pMainView->GetTimeString(pfd->FilteredData.Stats.AverageFunctionAndChildrenTime));
	GetMaxFCTime()->SetValue(m_pMainView->GetTimeString(pfd->FilteredData.Stats.MaximumFunctionAndChildrenTime));
	GetMinFCTime()->SetValue(m_pMainView->GetTimeString(pfd->FilteredData.Stats.MinimumFunctionAndChildrenTime));
	GetTotalFCTime()->SetValue(m_pMainView->GetTimeString(pfd->FilteredData.Stats.TotalFunctionAndChildrenTime));
	
	GetFPerc()->SetBitmap(
		MakePercentageGraph(100.0*((double)pfd->FilteredData.Stats.TotalFunctionTime)/((double)m_pDocument->m_PerfInfo.totalfilteredtime)));
	GetFCPerc()->SetBitmap(
		MakePercentageGraph(100.0*((double)pfd->FilteredData.Stats.TotalFunctionAndChildrenTime)/((double)m_pDocument->m_PerfInfo.totalfilteredtime)));
	
	ClearLists();

	wxUint64 parentstotaltime=0;
	for(stdext::hash_map<wxUint32,CTimingStats>::iterator iterp=pfd->FilteredData.Parents.begin();iterp!=pfd->FilteredData.Parents.end();iterp++)
	{
		parentstotaltime+=iterp->second.TotalFunctionAndChildrenTime;
	}
	wxUint64 childrentotaltime=0;
	for(stdext::hash_map<wxUint32,CTimingStats>::iterator iterc=pfd->FilteredData.Children.begin();iterc!=pfd->FilteredData.Children.end();iterc++)
	{
		childrentotaltime+=iterc->second.TotalFunctionAndChildrenTime;
	}
	

	int nParentItem=0;
	for(stdext::hash_map<wxUint32,CTimingStats>::iterator iterp=pfd->FilteredData.Parents.begin();iterp!=pfd->FilteredData.Parents.end();iterp++)
	{
		CFunctionData *pfdparent=&(m_pDocument->m_arrFunctionData[iterp->first]);
		
		wxString title=pfdparent->Desc.strBaseName;
		wxUint32 count=iterp->second.Count;
		double perc=100.0*((double)iterp->second.TotalFunctionAndChildrenTime)/((double)parentstotaltime);
		wxUint32 funcnum=iterp->first;

		GetParentsList()->InsertItem(nParentItem,title);
		GetParentsList()->SetItem(nParentItem,1,wxString::Format(wxT("%d"),count));
		GetParentsList()->SetItem(nParentItem,2,wxString::Format(wxT("%.2f%%"),perc));
		GetParentsList()->SetItemData(nParentItem,(long)(size_t)new CListItemData(title,count,perc,funcnum));
		nParentItem++;
	}

	int nChildItem=0;
	for(stdext::hash_map<wxUint32,CTimingStats>::iterator iterc=pfd->FilteredData.Children.begin();iterc!=pfd->FilteredData.Children.end();iterc++)
	{
		CFunctionData *pfdchild=&(m_pDocument->m_arrFunctionData[iterc->first]);
		
		wxString title=pfdchild->Desc.strBaseName;
		wxUint32 count=iterc->second.Count;
		double perc=100.0*((double)iterc->second.TotalFunctionAndChildrenTime)/((double)childrentotaltime);
		wxUint32 funcnum=iterc->first;

		GetChildrenList()->InsertItem(nChildItem,title);
		GetChildrenList()->SetItem(nChildItem,1,wxString::Format(wxT("%d"),count));
		GetChildrenList()->SetItem(nChildItem,2,wxString::Format(wxT("%.2f%%"),perc));
		GetChildrenList()->SetItemData(nChildItem,(long)(size_t)new CListItemData(title,count,perc,funcnum));
		nChildItem++;
	}

	GetParentsList()->SortItems(ParentsItemSort,(long)this);
	
	GetChildrenList()->SortItems(ChildrenItemSort,(long)this);
}

void CFunctionDetailView::SetSelectedFunction(int funcnum)
{
	m_nSelectedFunction=funcnum;
	UpdateData();
}



wxBitmap CFunctionDetailView::MakePercentageGraph(double perc)
{
	CPieChart pc;
	pc.AddSlice(new CPieSlice(100-(int)perc,wxSystemSettings::GetColour(wxSYS_COLOUR_3DSHADOW)));
	pc.AddSlice(new CPieSlice((int)perc,wxSystemSettings::GetColour(wxSYS_COLOUR_ACTIVECAPTION)));
	pc.GetSlice(1)->SetCenterOffset(wxRealPoint(.1,.1));
	
	wxBitmap bmp(64,64,32);
	{
		wxMemoryDC dc;
		dc.SelectObject(bmp);
		
		dc.SetBackground(wxBrush(wxColour(255,0,255)));
		dc.Clear();

		pc.Draw2D(&dc,wxPoint(32,32),29,false,0,false);
	}
	bmp.SetMask(new wxMask(bmp,wxColour(255,0,255)));

	return bmp;
}


void CFunctionDetailView::OnListItemActivatedParents(wxListEvent &evt)
{
	CListItemData *plid=(CListItemData *)(size_t)(evt.GetData());
	m_pMainView->SetSelectedFunction(plid->funcnum,true);
}

void CFunctionDetailView::OnListItemActivatedChildren(wxListEvent &evt)
{
	CListItemData *plid=(CListItemData *)(size_t)(evt.GetData());
	m_pMainView->SetSelectedFunction(plid->funcnum,true);
}

void CFunctionDetailView::OnIdle(wxIdleEvent &evt)
{
	if(m_bNeedsResize)
	{
		DoResize();
		m_bNeedsResize=false;
	}
}

void CFunctionDetailView::OnSize(wxSizeEvent &evt)
{
	m_bNeedsResize=true;
	evt.Skip();
}

int wxCALLBACK CFunctionDetailView::ParentsItemSort(long item1, long item2, long lthis)
{
	CFunctionDetailView *pThis=(CFunctionDetailView *)lthis;

	CListItemData *plid1;
	CListItemData *plid2;
	if(pThis->m_bParentsSortDir)
	{
		plid1=(CListItemData *)item2;
		plid2=(CListItemData *)item1;
	}
	else
	{
		plid1=(CListItemData *)item1;
		plid2=(CListItemData *)item2;
	}

	switch(pThis->m_nParentsSortCol)
	{
	case 0:
		if(plid1->title>plid2->title)
		{
			return 1;
		}
		if(plid1->title<plid2->title)
		{
			return -1;
		}
		return 0;
	case 1:
		if(plid1->numcalls>plid2->numcalls)
		{
			return 1;
		}
		if(plid1->numcalls<plid2->numcalls)
		{
			return -1;
		}
		return 0;
	case 2:
		if(plid1->perctime>plid2->perctime)
		{
			return 1;
		}
		if(plid1->perctime<plid2->perctime)
		{
			return -1;
		}
		return 0;
	}

	return 0;
}

int wxCALLBACK CFunctionDetailView::ChildrenItemSort(long item1, long item2, long sortData)
{
	CFunctionDetailView *pThis=(CFunctionDetailView *)sortData;

	CListItemData *plid1;
	CListItemData *plid2;
	if(pThis->m_bChildrenSortDir)
	{
		plid1=(CListItemData *)item2;
		plid2=(CListItemData *)item1;
	}
	else
	{
		plid1=(CListItemData *)item1;
		plid2=(CListItemData *)item2;
	}

	switch(pThis->m_nChildrenSortCol)
	{
	case 0:
		if(plid1->title>plid2->title)
		{
			return 1;
		}
		if(plid1->title<plid2->title)
		{
			return -1;
		}
		return 0;
	case 1:
		if(plid1->numcalls>plid2->numcalls)
		{
			return 1;
		}
		if(plid1->numcalls<plid2->numcalls)
		{
			return -1;
		}
		return 0;
	case 2:
		if(plid1->perctime>plid2->perctime)
		{
			return 1;
		}
		if(plid1->perctime<plid2->perctime)
		{
			return -1;
		}
		return 0;
	}

	return 0;
}


void CFunctionDetailView::OnListColClickParents(wxListEvent &evt)
{
	if(m_nParentsSortCol==evt.GetColumn())
	{
		m_bParentsSortDir=!m_bParentsSortDir;
	}
	else
	{
		m_nParentsSortCol=evt.GetColumn();
	}

	GetParentsList()->SortItems(ParentsItemSort,(long)this);
}

void CFunctionDetailView::OnListColClickChildren(wxListEvent &evt)
{
	if(m_nChildrenSortCol==evt.GetColumn())
	{
		m_bChildrenSortDir=!m_bChildrenSortDir;
	}
	else
	{
		m_nChildrenSortCol=evt.GetColumn();
	}

	GetChildrenList()->SortItems(ChildrenItemSort,(long)this );
}
	
