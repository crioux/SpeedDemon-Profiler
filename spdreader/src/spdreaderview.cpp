#include"spdreader_pch.h"
#include"spdreader.h"
#include"mainframe.h"
#include"spdreaderdoc.h"
#include"spdreaderview.h"
#include"functionlistview.h"
#include"functiondetailview.h"
#include"threadlistview.h"
#include"codegraph.h"
#include"columnsdlg.h"
#include"threadfilterdlg.h"
#include"finddlg.h"
#include"funcgraphdesc.h"
#include"defsext.h"
#include"edit.h"
#include"wxflatnotebook.h"

#define SHOW_PROFILER_TIME 1 

const wxChar *g_colnames[]={
	wxT("Name"),
	wxT("Source"),
	wxT("# Of Calls"),
	wxT("Min. F Time"),
	wxT("Avg. F Time"),
	wxT("Max. F Time"),
	wxT("Min. F+C Time"),
	wxT("Avg. F+C Time"),
	wxT("Max. F+C Time"),
	wxT("Total F Time"),
	wxT("Total F+C Time"),
	wxT("Avg. F/Size")
};


IMPLEMENT_DYNAMIC_CLASS(CSPDReaderView,wxView)

BEGIN_EVENT_TABLE(CSPDReaderView,wxView)
	EVT_SIZE(OnSize)
	EVT_MENU(ID_VIEW_DISP_AS_PERC,OnViewDispAsPerc)
	EVT_UPDATE_UI(ID_VIEW_DISP_AS_PERC,OnViewDispAsPercUpdateUI)
	EVT_MENU(ID_VIEW_COLUMNS,OnViewColumns)
	EVT_UPDATE_UI(ID_VIEW_COLUMNS,OnViewColumnsUpdateUI)
	EVT_MENU(ID_VIEW_BACK, OnViewBack)
	EVT_MENU(ID_VIEW_FORWARD,OnViewForward)
	EVT_UPDATE_UI(ID_VIEW_BACK, OnViewBackUpdateUI)
	EVT_UPDATE_UI(ID_VIEW_FORWARD,OnViewForwardUpdateUI)
	EVT_MENU(ID_VIEW_FOCUS, OnViewFocus)
	EVT_MENU(ID_VIEW_CLEAR_FOCUS,OnViewClearFocus)
	EVT_UPDATE_UI(ID_VIEW_FOCUS, OnViewFocusUpdateUI)
	EVT_UPDATE_UI(ID_VIEW_CLEAR_FOCUS,OnViewClearFocusUpdateUI)
	EVT_MENU(ID_VIEW_THREAD_FILTER,OnViewThreadFilter)
	EVT_UPDATE_UI(ID_VIEW_THREAD_FILTER,OnViewThreadFilterUpdateUI)
	EVT_TEXT_ENTER(ID_FIND_TEXT,OnFindText)
	EVT_UPDATE_UI(ID_FIND_TEXT,OnFindTextUpdateUI)
	EVT_MENU(ID_VIEW_FIND,OnViewFind)
	EVT_MENU(ID_VIEW_FIND_NEXT,OnViewFindNext)
	EVT_UPDATE_UI(ID_VIEW_FIND,OnViewFindUpdateUI)
	EVT_UPDATE_UI(ID_VIEW_FIND_NEXT,OnViewFindNextUpdateUI)
	EVT_MENU(ID_VIEW_SOURCE,OnViewSource)
	EVT_UPDATE_UI(ID_VIEW_SOURCE,OnViewSourceUpdateUI)

	EVT_FLATNOTEBOOK_PAGE_CHANGED(12345,OnNotebookPageChanged)
	EVT_FLATNOTEBOOK_PAGE_CLOSING(12345,OnNotebookPageClosing)
	EVT_MENU(ID_VIEW_CLOSE,OnCloseButton)

	EVT_UPDATE_UI(ID_VIEW_CLOSE,OnViewCloseUpdateUI)
	EVT_UPDATE_UI(wxID_CUT,OnEditCutUpdateUI)
	EVT_UPDATE_UI(wxID_COPY,OnEditCopyUpdateUI)
	EVT_UPDATE_UI(wxID_PASTE,OnEditPasteUpdateUI)

END_EVENT_TABLE()


CSPDReaderView::CSPDReaderView()
{	
	m_pSplitter1=NULL;
	m_pSplitter2=NULL;
	m_pFunctionListView=NULL;
	m_pThreadsListView=NULL;
	m_pFunctionDetailView=NULL;
	//m_pCodeGraph=NULL;
	m_nSelectedFunction=-1;
	m_bUseMilliseconds=false;
	m_bDispAsPerc=false;
	m_nHistoryPosition=-1;
	m_strFindText=wxT("");
	m_nFindPosition=-1;
	m_pGraphDesc=NULL;
	m_pCloseButton=NULL;
}

CSPDReaderView::~CSPDReaderView()
{
	if(m_pNotebook)
	{
		delete m_pNotebook;
	}
	if(m_pGraphDesc)
	{
		delete m_pGraphDesc;
	}
}

bool CSPDReaderView::OnCreate(wxDocument *doc, long WXUNUSED(flags)) 
{ 	
	if(!((CSPDReaderDoc *)doc)->IsValid() || !doc->IsKindOf(CLASSINFO(CSPDReaderDoc)))
	{
		wxMessageBox(wxT("Couldn't load because the file was corrupt."),wxT("SpeedDemon Error"),wxOK|wxICON_EXCLAMATION);
		return false;
	}
	m_pDocument=(CSPDReaderDoc *)doc;

	unsigned long vis;
	g_theApp->GetConfig()->Read(wxT("columns"),(long *)&vis,0xE97);
	int col=0;
	m_columnordering.clear();
	while(vis!=0)
	{
		if(vis & 1)
		{
			m_columnordering.push_back((COLUMNS)col);
		}
		col++;
		vis>>=1;
	}

	g_theApp->GetConfig()->Read(wxT("dispasperc"),&m_bDispAsPerc,false);

	wxSize sz=g_theApp->GetMainFrame()->GetClientSize();

	m_pNotebook=new wxFlatNotebook(g_theApp->GetMainFrame(),12345,wxPoint(0,0),sz,wxFNB_NO_X_BUTTON|wxFNB_FANCY_TABS|wxFNB_CTRL_BACKGROUND);
	m_pSplitter1=new wxSplitterWindow(m_pNotebook,-1,wxPoint(0,0),sz,wxSP_3D|wxSP_3DBORDER|wxNO_FULL_REPAINT_ON_RESIZE);
	m_pFunctionListView=new CFunctionListView(m_pSplitter1,m_pDocument,this);
	m_pFunctionDetailView=new CFunctionDetailView(m_pSplitter1,m_pDocument,this);
	//m_pCodeGraph=new CCodeGraph(m_pNotebook,m_pDocument,this);

	//m_pGraphDesc=new CFuncGraphDesc(m_pDocument);
	//m_pCodeGraph->SetGraphDescriptor(m_pGraphDesc);
	
	m_pNotebook->AddPage(m_pSplitter1,wxT("Statistics"),true);
	//m_pNotebook->AddPage(m_pCodeGraph,wxT("Code Graph"),false);
	
	m_pSplitter1->SetSashGravity(0.5);
	m_pSplitter1->SetMinimumPaneSize(16);

	m_pSplitter1->SplitHorizontally(m_pFunctionListView,m_pFunctionDetailView);

	m_pNotebook->Layout();

	m_pSplitter1->SetSashPosition(sz.GetHeight()/2);
	
	return true; 
}

void CSPDReaderView::OnDraw(wxDC *dc)
{

}
 
void CSPDReaderView::OnUpdate(wxView *sender, wxObject *hint)
{
	if(m_pFunctionListView)
	{
		m_pFunctionListView->UpdateData();
	}
	if(m_pFunctionDetailView)
	{
		m_pFunctionDetailView->UpdateData();
	}
//	if(m_pCodeGraph)
//	{
//		m_pCodeGraph->UpdateData();
//	}
}


void CSPDReaderView::OnSize(wxSizeEvent &evt)
{
	wxSize sz=g_theApp->GetMainFrame()->GetClientSize();
	m_pNotebook->SetSize(sz);

	if(m_pCloseButton)
	{
		wxSize nbsz=m_pNotebook->GetClientSize();
		m_pCloseButton->Move(nbsz.x-19,3);
	}

#ifdef SHOW_PROFILER_TIME
	double enterperc=100.0 * GetDoc()->m_PerfInfo.entertime / GetDoc()->m_PerfInfo.totalwallclocktime;
	double exitperc=100.0 * GetDoc()->m_PerfInfo.exittime / GetDoc()->m_PerfInfo.totalwallclocktime;
	double profperc=enterperc+exitperc;
	g_theApp->GetMainFrame()->GetStatusBar()->SetStatusText(
		wxString::Format(wxT("Profiling Time: %.5f%%, Enter Time: %.5f%%, Exit Time: %.5f%%"),
		profperc,enterperc,exitperc));
#endif

}


void CSPDReaderView::SetSelectedFunction(int funcnum, bool bUpdateHistory)
{
	if(m_nSelectedFunction==funcnum)
	{
		return;
	}
	
	m_nSelectedFunction=funcnum;
	
	if(m_pFunctionListView)
	{
		m_pFunctionListView->SetSelectedFunction(funcnum);
	}
	if(m_pFunctionDetailView)
	{
		m_pFunctionDetailView->SetSelectedFunction(funcnum);
	}
//	if(m_pCodeGraph)
//	{
//		m_pCodeGraph->SetSelectedFunction(funcnum);
//	}

	if(bUpdateHistory)
	{
		m_llHistory.erase(m_llHistory.begin()+(m_nHistoryPosition+1),m_llHistory.end());

		m_llHistory.push_back(funcnum);
		if(m_llHistory.size()==256)
		{
			m_llHistory.pop_front();
		}
		m_nHistoryPosition=(int)(m_llHistory.size())-1;
	}
}

int CSPDReaderView::GetSelectedFunction(void)
{
	return m_nSelectedFunction;
}



double CSPDReaderView::ConvertTime(wxUint64 ticks) const
{
	if(m_bUseMilliseconds)
	{
		return ((double)ticks)*1000.0/(double)m_pDocument->m_PerfInfo.tickspersecond;
	}
	
	return ((double)ticks)/(double)m_pDocument->m_PerfInfo.tickspersecond;
}

wxString CSPDReaderView::GetTimeString(wxUint64 ticks) const
{
	if(!m_bDispAsPerc)
	{
		if(m_bUseMilliseconds)
		{
			return wxString::Format(wxT("%.5f s"),ConvertTime(ticks));
		}
		return wxString::Format(wxT("%.5f ms"),ConvertTime(ticks));
	}
	return wxString::Format(wxT("%.2f%%"),100.0*(double)ticks/(double)m_pDocument->m_PerfInfo.totalfilteredtime);
}



void CSPDReaderView::SetUseMilliseconds(bool bUseMsec)
{
	m_bUseMilliseconds=bUseMsec;
	OnUpdate(this,NULL);
}

void CSPDReaderView::SetDispAsPerc(bool bDispAsPerc)
{
	m_bDispAsPerc=bDispAsPerc;
	OnUpdate(this,NULL);
	
	g_theApp->GetConfig()->Write(wxT("dispasperc"),m_bDispAsPerc);

}
bool CSPDReaderView::GetUseMilliseconds(void)
{
	return m_bUseMilliseconds;
}
	
bool CSPDReaderView::GetDispAsPerc(void)
{
	return m_bDispAsPerc;
}

void CSPDReaderView::OnViewDispAsPerc(wxCommandEvent &evt)
{
	SetDispAsPerc(!GetDispAsPerc());
}

void CSPDReaderView::OnViewDispAsPercUpdateUI(wxUpdateUIEvent &evt)
{
	evt.Enable(true);
	evt.Check(GetDispAsPerc());
}

void CSPDReaderView::OnViewColumns(wxCommandEvent &evt)
{
	CColumnsDlg dlg(g_theApp->GetMainFrame(),-1,wxT("Select Visible Columns"));
	wxCheckListBox *pclb=dlg.GetColumns();

	wxArrayString asItems;	
	for(int i=0;i<(sizeof(g_colnames)/sizeof(const wxChar *));i++)
	{
		asItems.Add(g_colnames[i]);
	}

	pclb->InsertItems(asItems,0);

	for(std::vector<COLUMNS>::iterator iter=m_columnordering.begin();iter!=m_columnordering.end();iter++)
	{
		pclb->Check((int)(*iter),true);	
	}

	if(dlg.ShowModal()==wxID_OK)
	{
		m_columnordering.clear();
		long vis=0;
		for(int i=0;i<pclb->GetCount();i++)
		{
			if(pclb->IsChecked(i))
			{
				m_columnordering.push_back((COLUMNS)i);
				vis|=(1<<i);
			}
		}

		g_theApp->GetConfig()->Write(wxT("columns"),vis);

		m_pFunctionListView->UpdateVisibleColumns();
	}
}

void CSPDReaderView::OnViewColumnsUpdateUI(wxUpdateUIEvent &evt)
{
	evt.Enable(true);
}

void CSPDReaderView::OnViewBack(wxCommandEvent &evt)
{
	GoBack();
}

void CSPDReaderView::OnViewForward(wxCommandEvent &evt)
{
	GoForward();
}

void CSPDReaderView::OnViewBackUpdateUI(wxUpdateUIEvent &evt)
{
	evt.Enable(m_nHistoryPosition>0);
}

void CSPDReaderView::OnViewForwardUpdateUI(wxUpdateUIEvent &evt)
{
	evt.Enable(m_nHistoryPosition<((int)(m_llHistory.size()-1)));
}

void CSPDReaderView::GoBack(void)
{
	if(m_nHistoryPosition>0)
	{
		m_nHistoryPosition--;
		SetSelectedFunction(*(m_llHistory.begin()+m_nHistoryPosition),false);
	}
}

void CSPDReaderView::GoForward(void)
{
	if(m_nHistoryPosition<((int)(m_llHistory.size()-1)))
	{
		m_nHistoryPosition++;
		SetSelectedFunction(*(m_llHistory.begin()+m_nHistoryPosition),false);
	}
}


void CSPDReaderView::OnViewFocus(wxCommandEvent &evt)
{
	m_nHistoryPosition=-1;
	m_llHistory.clear();
	m_pDocument->SetFocusFilter(GetSelectedFunction());
	OnUpdate(this,NULL);	
}

void CSPDReaderView::OnViewClearFocus(wxCommandEvent &evt)
{
	m_nHistoryPosition=-1;
	m_llHistory.clear();
	m_pDocument->ClearFocusFilter();
	OnUpdate(this,NULL);
}

void CSPDReaderView::OnViewFocusUpdateUI(wxUpdateUIEvent &evt)
{
	evt.Enable(GetSelectedFunction()!=-1);
}

void CSPDReaderView::OnViewClearFocusUpdateUI(wxUpdateUIEvent &evt)
{
	evt.Enable(m_pDocument->GetFocusFilter()!=-1);
}


void CSPDReaderView::OnViewThreadFilter(wxCommandEvent &evt)
{
	CThreadFilterDlg dlg(g_theApp->GetMainFrame(),-1,wxT("Filter Threads"));

	for(size_t t=1;t<GetDoc()->m_arrThreadData.size();t++)
	{
		wxString strFuncName=wxT("<unknown>");
		if(GetDoc()->m_arrThreadData[t].TopLevelFunctions.size()==1)
		{
			strFuncName=GetDoc()->m_arrFunctionData[*(GetDoc()->m_arrThreadData[t].TopLevelFunctions.begin())].Desc.strBaseName;
		}
		else if(GetDoc()->m_arrThreadData[t].TopLevelFunctions.size()>1)
		{
			strFuncName=wxT("<multiple top-level functions>");
		}
		dlg.GetThreads()->Append(wxString::Format(wxT("%u: %s"),t,(const TCHAR *)strFuncName));
	}

	for(std::list<wxUint32>::iterator iter=GetDoc()->m_llFilterThreadSeqNums.begin();iter!=GetDoc()->m_llFilterThreadSeqNums.end();iter++)
	{
		dlg.GetThreads()->Check((*iter)-1,true);
	}

	if(dlg.ShowModal()==wxID_OK)
	{
		std::list<wxUint32> tids;
		for(size_t t=1;t<GetDoc()->m_arrThreadData.size();t++)
		{
			if(dlg.GetThreads()->IsChecked(t-1))
			{
				tids.push_back(t);
			}
		}

		m_nHistoryPosition=-1;
		m_llHistory.clear();
		GetDoc()->SetThreadFilter(tids);
		OnUpdate(this,NULL);
	}

}

void CSPDReaderView::OnViewThreadFilterUpdateUI(wxUpdateUIEvent &evt)
{
	evt.Enable(true);
}


void CSPDReaderView::OnFindText(wxCommandEvent &evt)
{
	if(m_pNotebook->GetCurrentPage()==m_pSplitter1)
	{
		wxComboBox *pft=(wxComboBox *)g_theApp->GetMainFrame()->FindWindow(ID_FIND_TEXT);
		if(pft==NULL)
		{
			return;
		}
		wxString strFindText=pft->GetValue();
		
		m_strFindText=strFindText;

		DoFind();
	}
	else
	{
		Edit *pEdit=(Edit *)m_pNotebook->GetCurrentPage();
		pEdit->OnFindText(evt);	
	}
}
	
void CSPDReaderView::OnViewFind(wxCommandEvent &evt)
{
	if(m_pNotebook->GetCurrentPage()==m_pSplitter1)
	{		
		CFindDlg dlg(g_theApp->GetMainFrame(),-1,wxT("Find"));

		dlg.GetFindText()->SetValue(m_strFindText);

		if(dlg.ShowModal()==wxID_OK)
		{
			m_strFindText=dlg.GetFindText()->GetValue();
			m_nFindPosition=-1;

			wxComboBox *pft=(wxComboBox *)g_theApp->GetMainFrame()->FindWindow(ID_FIND_TEXT);
			if(pft!=NULL)
			{
				pft->SetValue(m_strFindText);
			}
			DoFind();
		}
	}
	else
	{
		Edit *pEdit=(Edit *)m_pNotebook->GetCurrentPage();
		pEdit->OnFind(evt);
	}
}

void CSPDReaderView::OnViewFindNext(wxCommandEvent &evt)
{
	if(m_pNotebook->GetCurrentPage()==m_pSplitter1)
	{
		if(m_strFindText.IsEmpty())
		{
			return;
		}
		DoFind();
	}
	else
	{
		Edit *pEdit=(Edit *)m_pNotebook->GetCurrentPage();
		pEdit->OnFindNext(evt);
	}
}

void CSPDReaderView::OnViewFindUpdateUI(wxUpdateUIEvent &evt)
{
	if(m_pNotebook->GetCurrentPage()==m_pSplitter1)
	{
		evt.Enable(true);
	}
	else
	{
		Edit *pEdit=(Edit *)m_pNotebook->GetCurrentPage();
		pEdit->OnFindUpdateUI(evt);
	}
}

void CSPDReaderView::OnViewFindNextUpdateUI(wxUpdateUIEvent &evt)
{
	if(m_pNotebook->GetCurrentPage()==m_pSplitter1)
	{
		evt.Enable(!m_strFindText.IsEmpty());
	}
	else
	{
		Edit *pEdit=(Edit *)m_pNotebook->GetCurrentPage();
		pEdit->OnFindNextUpdateUI(evt);
	}

}

void CSPDReaderView::DoFind(void)
{
	m_nFindPosition++;
	int cnt=(int)GetDoc()->m_llSortedFunctions.size();
	if(cnt==0)
	{
		wxMessageBox(wxT("No matches found."),wxT("Find"),wxOK|wxICON_INFORMATION);
		return;
	}
	if(m_nFindPosition>=cnt)
	{
		m_nFindPosition=0;
	}

	int nStartPosition=m_nFindPosition;
	bool bFound=false;

	do
	{
		int fidx=GetDoc()->m_llSortedFunctions[m_nFindPosition];
		if(GetDoc()->m_arrFunctionData[fidx].Desc.strBaseName.Contains(m_strFindText))
		{
			SetSelectedFunction(fidx,true);
			return;
		}
		
		m_nFindPosition++;
		if(m_nFindPosition==cnt)
		{
			m_nFindPosition=0;
		}
	} while(m_nFindPosition!=nStartPosition);
		
	wxMessageBox(wxT("No matches found."),wxT("Find"),wxOK|wxICON_INFORMATION);
}

void CSPDReaderView::OnFindTextUpdateUI(wxUpdateUIEvent &evt)
{
	if(m_pNotebook->GetCurrentPage()==m_pSplitter1)
	{
		evt.Enable(true);
	}
	else
	{
		Edit *pEdit=(Edit *)m_pNotebook->GetCurrentPage();
		pEdit->OnFindTextUpdateUI(evt);
	}
}

void CSPDReaderView::OpenSource(wxUint32 funcid)
{
	CFunctionData *pfd=&(m_pDocument->m_arrFunctionData[funcid]);
	if(pfd->Desc.strSourceFile==wxT("<unknown>"))
	{
		return;
	}
	wxFileName fname(pfd->Desc.strSourceFile);
	if(!fname.FileExists())
	{
		wxMessageBox(wxString::Format(wxT("File '%s' does not exist."),(const TCHAR *)fname.GetFullPath()),wxT("File not found"),wxOK|wxICON_WARNING);
		return;
	}

	std::map<wxString,wxWindow *>::iterator iter=m_openFiles.find(fname.GetFullPath());
	Edit *pstc=NULL;
	if(iter!=m_openFiles.end())
	{
		int n;
		for(n=0;n<m_pNotebook->GetPageCount();n++)
		{
			if(m_pNotebook->GetPage(n)==iter->second)
			{
				m_pNotebook->SetSelection(n);
				pstc=(Edit *)(iter->second);
				break;
			}
		}
	}

	if(!pstc)
	{
		pstc=new Edit(m_pNotebook);
		pstc->LoadFile(pfd->Desc.strSourceFile);
		pstc->SetReadOnly(true);
		pstc->SetHighlightGuide(1);
		m_pNotebook->AddPage(pstc,fname.GetFullName(),true);
		m_openFiles[fname.GetFullPath()]=pstc;
	}

	pstc->GotoLine(pfd->Desc.nFirstSourceLine-1);
	pstc->SetSelectionStart(pstc->PositionFromLine(pfd->Desc.nFirstSourceLine-1));
	pstc->SetSelectionEnd(pstc->GetLineEndPosition(pfd->Desc.nLastSourceLine-1));
}

void CSPDReaderView::OnViewSource(wxCommandEvent &evt)
{
	wxUint32 func=GetSelectedFunction();
	if(func==-1)
	{
		return;
	}
	OpenSource(func);
}

void CSPDReaderView::OnViewSourceUpdateUI(wxUpdateUIEvent &evt)
{
	evt.Enable(GetSelectedFunction()!=-1);
}

void CSPDReaderView::OnNotebookPageClosing(wxFlatNotebookEvent &evt)
{
	wxWindow *pPage=m_pNotebook->GetPage(evt.GetSelection());

	for(std::map<wxString,wxWindow *>::iterator iter=m_openFiles.begin();iter!=m_openFiles.end();iter++)
	{
		if(iter->second==pPage)
		{
			m_openFiles.erase(iter);
			break;
		}
	}
}

void CSPDReaderView::OnNotebookPageChanged(wxFlatNotebookEvent &evt)
{
	bool bHasButton=false;

	if(m_pNotebook->GetCurrentPage()==m_pSplitter1)
	{
		bHasButton=false;
	}
	else
	{
		bHasButton=true;
	}

	if(bHasButton)
	{
		m_pNotebook->SetWindowStyleFlag(wxFNB_FANCY_TABS|wxFNB_CTRL_BACKGROUND);
	}
	else
	{
		m_pNotebook->SetWindowStyleFlag(wxFNB_NO_X_BUTTON|wxFNB_FANCY_TABS|wxFNB_CTRL_BACKGROUND);
	}
}

void CSPDReaderView::OnCloseButton(wxCommandEvent &evt)
{
	wxWindow *pPage=m_pNotebook->GetCurrentPage();
	bool bFound=false;
	int n;
	for(n=0;n<m_pNotebook->GetPageCount();n++)
	{
		if(m_pNotebook->GetPage(n)==pPage)
		{
			bFound=true;
			break;
		}
	}
	if(!bFound)
	{
		return;
	}

	m_pNotebook->DeletePage(n);
}

void CSPDReaderView::OnViewCloseUpdateUI(wxUpdateUIEvent &evt)
{
	evt.Enable(m_pCloseButton!=NULL);
}


void CSPDReaderView::OnEditCutUpdateUI(wxUpdateUIEvent &evt)
{
	if(m_pNotebook->GetCurrentPage()==m_pSplitter1)
	{
		evt.Enable(false);
	}
	else
	{	
		Edit *pEdit=(Edit *)m_pNotebook->GetCurrentPage();
		pEdit->OnEditCutUpdateUI(evt);
	}
}

void CSPDReaderView::OnEditCopyUpdateUI(wxUpdateUIEvent &evt)
{
	if(m_pNotebook->GetCurrentPage()==m_pSplitter1)
	{
		evt.Enable(false);
	}
	else
	{	
		Edit *pEdit=(Edit *)m_pNotebook->GetCurrentPage();
		pEdit->OnEditCopyUpdateUI(evt);
	}
}

void CSPDReaderView::OnEditPasteUpdateUI(wxUpdateUIEvent &evt)
{
	if(m_pNotebook->GetCurrentPage()==m_pSplitter1)
	{
		evt.Enable(false);
	}
	else
	{	
		Edit *pEdit=(Edit *)m_pNotebook->GetCurrentPage();
		pEdit->OnEditPasteUpdateUI(evt);
	}
}