#include"spdreader_pch.h"
#include"spdreader.h"
#include"spdreaderdoc.h"
#include"spdreaderview.h"
#include"functionlistview.h"


BEGIN_EVENT_TABLE(CFunctionListView,wxListCtrl)
	EVT_LIST_COL_CLICK(-1, OnClickColumn) 
	EVT_LIST_ITEM_FOCUSED(-1, OnItemFocused) 
	EVT_LIST_ITEM_ACTIVATED(-1, OnItemActivated)
	EVT_IDLE(OnIdle)
	EVT_SIZE(OnSize)
	EVT_LIST_COL_BEGIN_DRAG(-1, OnColBeginDrag) 
	EVT_LIST_COL_END_DRAG(-1, OnColEndDrag) 
	EVT_LIST_ITEM_RIGHT_CLICK(-1,OnListItemRightClick) 
END_EVENT_TABLE()


CFunctionListView::CFunctionListView(wxWindow* parent, CSPDReaderDoc *pDoc, CSPDReaderView *pMainView):
	CNoFlickerListCtrl(parent,-1,wxDefaultPosition,wxDefaultSize,wxLC_REPORT|wxLC_VIRTUAL|wxLC_SINGLE_SEL|wxLC_VRULES|wxNO_BORDER|wxNO_FULL_REPAINT_ON_RESIZE|wxCLIP_CHILDREN)
{
	m_pDocument=pDoc;
	m_pMainView=pMainView;

	SetItemCount(0);

	UpdateVisibleColumns();

	m_row.SetTextColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
	m_row.SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
	
	
	wxColour bgcolor=m_row.GetBackgroundColour();
	int r=bgcolor.Red();
	int g=bgcolor.Green();
	int b=bgcolor.Blue();
	r=r<128?(r*105/100):(r*95/100);
	g=g<128?(g*105/100):(g*95/100);
	b=b<128?(b*105/100):(b*95/100);

	m_rowshaded.SetTextColour(m_row.GetTextColour());	
	m_rowshaded.SetBackgroundColour(wxColour(r,g,b));

	m_nSelectedFunction=-1;

	m_bNeedsResize=true;
	m_bDragging=false;
}

void CFunctionListView::UpdateData(void)
{
	int cnt=(long)m_pDocument->m_llSortedFunctions.size();

	SetItemCount(cnt);
	RefreshItems(0,cnt-1);
	
	Refresh();
}

void CFunctionListView::DoResize(void)
{
	int wid=0;
	for(int i=1;i<GetColumnCount();i++)
	{
		wid+=(GetColumnWidth(i)+2);
	}
	
	SetColumnWidth(0,GetSize().GetWidth()-wid+1);
}

wxListItemAttr* CFunctionListView::OnGetItemAttr(long item) const
{
	if((item & 1)==1)
	{
		return (wxListItemAttr*) &m_rowshaded;
	}

	return (wxListItemAttr*) &m_row;
}

int CFunctionListView::OnGetItemImage(long item) const
{
	return -1;
}

wxString CFunctionListView::OnGetItemText(long item,long column) const
{
	CFunctionData *pfd=&(m_pDocument->m_arrFunctionData[m_pDocument->m_llSortedFunctions[item]]);

	CSPDReaderView::COLUMNS realcol=m_pMainView->m_columnordering[column];

	switch(realcol)
	{
	case CSPDReaderView::NAME:
		return pfd->Desc.strBaseName;
	case CSPDReaderView::SOURCE:
		return wxString::Format(wxT("%s, Lines %d-%d"),(const TCHAR *)pfd->Desc.strSourceFile,pfd->Desc.nFirstSourceLine,pfd->Desc.nLastSourceLine);
	case CSPDReaderView::NUMOFCALLS:
		return wxString::Format(LLFMT,pfd->FilteredData.Stats.Count);
	case CSPDReaderView::MINFTIME:
		return m_pMainView->GetTimeString(pfd->FilteredData.Stats.MinimumFunctionTime);
	case CSPDReaderView::AVGFTIME:
		return m_pMainView->GetTimeString(pfd->FilteredData.Stats.AverageFunctionTime);
	case CSPDReaderView::MAXFTIME:
		return m_pMainView->GetTimeString(pfd->FilteredData.Stats.MaximumFunctionTime);
	case CSPDReaderView::MINFCTIME:
		return m_pMainView->GetTimeString(pfd->FilteredData.Stats.MinimumFunctionAndChildrenTime);
	case CSPDReaderView::AVGFCTIME:
		return m_pMainView->GetTimeString(pfd->FilteredData.Stats.AverageFunctionAndChildrenTime);
	case CSPDReaderView::MAXFCTIME:
		return m_pMainView->GetTimeString(pfd->FilteredData.Stats.MaximumFunctionAndChildrenTime);
	case CSPDReaderView::TOTALFTIME:
		return m_pMainView->GetTimeString(pfd->FilteredData.Stats.TotalFunctionTime);
	case CSPDReaderView::TOTALFCTIME:
		return m_pMainView->GetTimeString(pfd->FilteredData.Stats.TotalFunctionAndChildrenTime);
	case CSPDReaderView::AVGFOVERSIZE:
		{
			wxUint64 div=0;
			if(pfd->Desc.nByteLength!=0)
			{
				div=pfd->FilteredData.Stats.AverageFunctionTime/pfd->Desc.nByteLength;
			}
			return wxString::Format(wxT("%.5f"),((double)div)/((double)m_pDocument->m_PerfInfo.tickspersecond));
		}
	default:
		break;
	}

	return wxT("???");
}

void CFunctionListView::OnClickColumn(wxListEvent &evt)
{
	int col=m_pMainView->m_columnordering[evt.m_col];
	
	bool bForward=m_pDocument->GetSortDirection();
	int nSortCol=m_pDocument->GetSortColumn();
	if(nSortCol==col)
	{
		bForward=!bForward;
	}
	else
	{
		nSortCol=col;
	}

	m_pDocument->SortFunctions(nSortCol,bForward);
	UpdateData();
}




void CFunctionListView::OnItemFocused(wxListEvent &evt)
{
	long idx=evt.GetIndex();
	wxUint32 funcnum=m_pDocument->m_llSortedFunctions[idx];

	m_pMainView->SetSelectedFunction(funcnum,true);
}

void CFunctionListView::SetSelectedFunction(int funcnum)
{
	long idx=m_pDocument->m_hmSortedFunctionIndex[funcnum];

	SetItemState(idx,wxLIST_STATE_FOCUSED|wxLIST_STATE_SELECTED,wxLIST_STATE_FOCUSED|wxLIST_STATE_SELECTED);
	EnsureVisible(idx);
}

void CFunctionListView::OnIdle(wxIdleEvent &evt)
{
	if(m_bNeedsResize && !m_bDragging)
	{
		DoResize();
		m_bNeedsResize=false;
	}
}

void CFunctionListView::OnSize(wxSizeEvent &evt)
{
	m_bNeedsResize=true;
	evt.Skip();
}


void CFunctionListView::UpdateVisibleColumns(void)
{
	while(GetColumnCount()>0)
	{
		DeleteColumn(0);
	}

	int col=0;
	for(std::vector<CSPDReaderView::COLUMNS>::iterator iter=m_pMainView->m_columnordering.begin();iter!=m_pMainView->m_columnordering.end();iter++)
	{
		switch(*iter)
		{
		case CSPDReaderView::NAME:
			InsertColumn(col,g_colnames[CSPDReaderView::NAME],wxLIST_FORMAT_LEFT,wxLIST_AUTOSIZE);
			break;
		case CSPDReaderView::NUMOFCALLS:
			InsertColumn(col,g_colnames[CSPDReaderView::NUMOFCALLS],wxLIST_FORMAT_LEFT,wxLIST_AUTOSIZE_USEHEADER);
			break;
		case CSPDReaderView::MINFTIME:
			InsertColumn(col,g_colnames[CSPDReaderView::MINFTIME],wxLIST_FORMAT_LEFT,wxLIST_AUTOSIZE_USEHEADER);
			break;
		case CSPDReaderView::AVGFTIME:
			InsertColumn(col,g_colnames[CSPDReaderView::AVGFTIME],wxLIST_FORMAT_LEFT,wxLIST_AUTOSIZE_USEHEADER);
			break;
		case CSPDReaderView::MAXFTIME:
			InsertColumn(col,g_colnames[CSPDReaderView::MAXFTIME],wxLIST_FORMAT_LEFT,wxLIST_AUTOSIZE_USEHEADER);
			break;
		case CSPDReaderView::MINFCTIME:
			InsertColumn(col,g_colnames[CSPDReaderView::MINFCTIME],wxLIST_FORMAT_LEFT,wxLIST_AUTOSIZE_USEHEADER);
			break;
		case CSPDReaderView::AVGFCTIME:
			InsertColumn(col,g_colnames[CSPDReaderView::AVGFCTIME],wxLIST_FORMAT_LEFT,wxLIST_AUTOSIZE_USEHEADER);
			break;
		case CSPDReaderView::MAXFCTIME:
			InsertColumn(col,g_colnames[CSPDReaderView::MAXFCTIME],wxLIST_FORMAT_LEFT,wxLIST_AUTOSIZE_USEHEADER);
			break;
		case CSPDReaderView::TOTALFTIME:
			InsertColumn(col,g_colnames[CSPDReaderView::TOTALFTIME],wxLIST_FORMAT_LEFT,wxLIST_AUTOSIZE_USEHEADER);
			break;
		case CSPDReaderView::TOTALFCTIME:
			InsertColumn(col,g_colnames[CSPDReaderView::TOTALFCTIME],wxLIST_FORMAT_LEFT,wxLIST_AUTOSIZE_USEHEADER);
			break;
		case CSPDReaderView::SOURCE:
			InsertColumn(col,g_colnames[CSPDReaderView::SOURCE],wxLIST_FORMAT_LEFT,wxLIST_AUTOSIZE_USEHEADER);
			break;
		case CSPDReaderView::AVGFOVERSIZE:
			InsertColumn(col,g_colnames[CSPDReaderView::AVGFOVERSIZE],wxLIST_FORMAT_LEFT,wxLIST_AUTOSIZE_USEHEADER);
			break;
		}
		col++;
	}
}

void CFunctionListView::OnColBeginDrag(wxListEvent &evt)
{
	m_bDragging=true;
	evt.Skip();
}

void CFunctionListView::OnColEndDrag(wxListEvent &evt)
{
	m_bDragging=false;
	evt.Skip();
}

void CFunctionListView::OnListItemRightClick(wxListEvent &evt)
{
	wxInt32 func=evt.GetIndex();

	m_pMainView->SetSelectedFunction(m_pDocument->m_llSortedFunctions[func],true);

	wxMenuBar *pMenuBar=FunctionPopupMenu();
	PopupMenu(pMenuBar->GetMenu(0));
	delete pMenuBar;
}

void CFunctionListView::OnItemActivated(wxListEvent &evt)
{
	long idx=evt.GetIndex();
	wxUint32 funcnum=m_pDocument->m_llSortedFunctions[idx];

	m_pMainView->SetSelectedFunction(funcnum,true);
}

