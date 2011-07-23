#include"spdreader_pch.h"
#include"spdreader.h"
#include"spdreaderdoc.h"
#include"mainframe.h"
#include"aboutdlg.h"

#ifdef WIN32
#include "rapifiledialog.h"
#include <rapi.h>
#include "rapiaccess.h"
#define ID_IMPORT_FROM_DEVICE (53210)
#endif

BEGIN_EVENT_TABLE(CMainFrame,wxDocParentFrame)
	EVT_SIZE(CMainFrame::OnSize)
	EVT_MOVE(CMainFrame::OnMove)
	EVT_MENU(ID_IMPORT_FROM_FILE, CMainFrame::OnImportFromFile)
	EVT_MENU(ID_HELP_DOCUMENTATION, CMainFrame::OnHelpDocumentation)
	EVT_MENU(ID_HELP_ABOUT, CMainFrame::OnHelpAbout)
	EVT_UPDATE_UI(ID_VIEW_DISP_AS_PERC,OnViewDispAsPercUpdateUI)
	EVT_UPDATE_UI(ID_VIEW_COLUMNS,OnViewColumnsUpdateUI)
	EVT_UPDATE_UI(ID_VIEW_BACK, OnViewBackUpdateUI)
	EVT_UPDATE_UI(ID_VIEW_FORWARD,OnViewForwardUpdateUI)
	EVT_UPDATE_UI(ID_VIEW_FOCUS,OnViewFocusUpdateUI)
	EVT_UPDATE_UI(ID_VIEW_CLEAR_FOCUS,OnViewClearFocusUpdateUI)
	EVT_UPDATE_UI(ID_VIEW_THREAD_FILTER,OnViewThreadFilterUpdateUI)
	EVT_UPDATE_UI(ID_FIND_TEXT,OnFindTextUpdateUI)
	EVT_UPDATE_UI(ID_VIEW_FIND,OnViewFindUpdateUI)
	EVT_UPDATE_UI(ID_VIEW_FIND_NEXT,OnViewFindNextUpdateUI)
	EVT_UPDATE_UI(ID_VIEW_SOURCE,OnViewSourceUpdateUI)
	EVT_UPDATE_UI(ID_VIEW_CLOSE,OnViewCloseUpdateUI)
	EVT_UPDATE_UI(wxID_CUT,OnEditCutUpdateUI)
	EVT_UPDATE_UI(wxID_COPY,OnEditCopyUpdateUI)
	EVT_UPDATE_UI(wxID_PASTE,OnEditPasteUpdateUI)
#ifdef WIN32
	EVT_MENU(ID_IMPORT_FROM_DEVICE, CMainFrame::OnImportFromDevice)
#endif
END_EVENT_TABLE()



CMainFrame::CMainFrame(wxDocManager *manager, const wxPoint &pos, const wxSize &size, long style):
	wxDocParentFrame(manager,NULL,-1,wxT("SPDReader - SpeedDemon Profiler"),pos,size,style)
{
	SetMenuBar(MainMenuFunc());
	wxIcon ico_spdreader(wxICON(SPDREADER));
	SetIcon(ico_spdreader);
	
	wxMenu *pRecentFiles=GetMenuBar()->GetMenu(0)->FindItem(ID_FILE_RECENTFILES)->GetSubMenu();
	m_docManager->FileHistoryUseMenu(pRecentFiles);

	CreateStatusBar(1,wxST_SIZEGRIP);
	CreateToolBar(wxTB_FLAT|wxTB_HORIZONTAL|wxTB_DOCKABLE);
	GetToolBar()->SetToolBitmapSize(wxSize(16,16));
	MainToolbarFunc(GetToolBar());

#ifdef WIN32

	wxMenuBar *pmb=GetMenuBar();
	wxMenuItem *item=new wxMenuItem(pmb->GetMenu(0),ID_IMPORT_FROM_DEVICE,wxT("Import From Device..."),wxT("Import SPD File From Device"));
	item->SetBitmap(MainMenuBitmaps(18)); 
	pmb->GetMenu(0)->Insert(4,item);
	GetToolBar()->InsertTool(4,ID_IMPORT_FROM_DEVICE,MainMenuBitmaps(18),wxNullBitmap,false,NULL,wxT("Import SPD File From Device"));
	GetToolBar()->Realize();

#endif

}

CMainFrame::~CMainFrame()
{
}


void CMainFrame::OnHelpDocumentation(wxCommandEvent &evt)
{
	ShellExecute(NULL,wxT("open"),wxT("docs\\manual.doc"),NULL,wxFileName::GetCwd(),SW_SHOW);
}

void CMainFrame::OnHelpAbout(wxCommandEvent &evt)
{
	CAboutDlg dlg(this,-1,wxT("About SPDReader"));
	dlg.ShowModal();
}


void CMainFrame::OnSize(wxSizeEvent &size)
{
	if(!IsMaximized())
	{	
		wxSize sz=size.GetSize();	
		g_theApp->m_w=sz.x;
		g_theApp->m_h=sz.y;
	}
}

void CMainFrame::OnMove(wxMoveEvent &move)
{
	g_theApp->m_bMaximized=IsMaximized();
	if(!IsMaximized())
	{	
		wxPoint pt=move.GetPosition();
		g_theApp->m_x=pt.x;
		g_theApp->m_y=pt.y;
	}
}

void CMainFrame::OnViewDispAsPercUpdateUI(wxUpdateUIEvent &evt)
{
	evt.Check(false);
	evt.Enable(false);
}

void CMainFrame::OnViewColumnsUpdateUI(wxUpdateUIEvent &evt)
{
	evt.Enable(false);
}

void CMainFrame::OnViewBackUpdateUI(wxUpdateUIEvent &evt)
{
	evt.Enable(false);
}

void CMainFrame::OnViewForwardUpdateUI(wxUpdateUIEvent &evt)
{
	evt.Enable(false);
}

void CMainFrame::OnViewFocusUpdateUI(wxUpdateUIEvent &evt)
{
	evt.Enable(false);
}

void CMainFrame::OnViewClearFocusUpdateUI(wxUpdateUIEvent &evt)
{
	evt.Enable(false);
}

void CMainFrame::OnViewThreadFilterUpdateUI(wxUpdateUIEvent &evt)
{
	evt.Enable(false);
}

void CMainFrame::OnFindTextUpdateUI(wxUpdateUIEvent &evt)
{
	evt.Enable(false);
}

void CMainFrame::OnViewFindUpdateUI(wxUpdateUIEvent &evt)
{
	evt.Enable(false);
}

void CMainFrame::OnViewFindNextUpdateUI(wxUpdateUIEvent &evt)
{
	evt.Enable(false);
}

void CMainFrame::OnViewSourceUpdateUI(wxUpdateUIEvent &evt)
{
	evt.Enable(false);
}

void CMainFrame::OnViewCloseUpdateUI(wxUpdateUIEvent &evt)
{
	evt.Enable(false);
}

void CMainFrame::OnEditCutUpdateUI(wxUpdateUIEvent &evt)
{
	evt.Enable(false);
}

void CMainFrame::OnEditCopyUpdateUI(wxUpdateUIEvent &evt)
{
	evt.Enable(false);
}

void CMainFrame::OnEditPasteUpdateUI(wxUpdateUIEvent &evt)
{
	evt.Enable(false);
}

#ifdef WIN32

bool CMainFrame::OpenFromDevice(const wxString &strPathName)
{
	HANDLE hRemoteFile=g_RAPI.CeCreateFile(strPathName,GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	if(hRemoteFile==INVALID_HANDLE_VALUE)
	{
		return false;
	}

	DWORD dwSize=g_RAPI.CeGetFileSize(hRemoteFile,NULL);
	if(dwSize==0xFFFFFFFF)
	{
		g_RAPI.CeCloseHandle(hRemoteFile);
		return false;
	}

	void *data=malloc(dwSize);
	if(data==NULL)
	{
		g_RAPI.CeCloseHandle(hRemoteFile);
		return false;
	}

	DWORD dwBytesRead;
	if(!g_RAPI.CeReadFile(hRemoteFile,data,dwSize,&dwBytesRead,NULL) || dwBytesRead!=dwSize)
	{
		free(data);
		g_RAPI.CeCloseHandle(hRemoteFile);
		return false;
	}

   	g_RAPI.CeCloseHandle(hRemoteFile);

	CSPDReaderDoc *pDoc=(CSPDReaderDoc *)(g_theApp->GetDocManager()->CreateDocument(wxT("untitled.spr"),wxDOC_NEW));
    pDoc->ImportData(wxMemoryInputStream(data,dwSize));
	pDoc->UpdateAllViews();
	free(data);

	g_theApp->m_strLastDeviceFile=strPathName;
	return true;
}


void CMainFrame::OnImportFromDevice(wxCommandEvent &evt)
{
	if(!g_theApp->GetDocManager()->CloseDocuments(false))
	{
		return;
	}
	
	if(!g_RAPI.IsValid())
	{
		wxMessageBox(wxT("Install ActiveSync to use this feature."),wxT("Can not import remote file."),wxOK|wxICON_WARNING);
		return;
	}

	g_RAPI.CeRapiInit();

	CRapiFileDialog rfd(this,-1,wxT("Import SPD File From Device"));
	rfd.SetPathName(g_theApp->m_strLastDeviceFile);
	rfd.AddFileType(wxT("SPD Files (*.spd)"),wxT("*.spd"));
	rfd.GetFilesOfType()->SetSelection(1);
	rfd.UpdateDialog();
	if(rfd.ShowModal()==wxID_OK)
	{
		wxString strPathName=rfd.GetPathName();
		if(!OpenFromDevice(strPathName))
		{
			wxMessageBox(wxT("Unable to import file on device."),wxT("File import error"),wxOK|wxICON_WARNING);

			g_RAPI.CeRapiUninit();
			return;
		}
	
		g_RAPI.CeRapiUninit();

		SaveReport();

		return;
	}

	g_RAPI.CeRapiUninit();
}

#endif

bool CMainFrame::SaveReport(void)
{
	// Offer to save the merged file
	wxFileName lastpr;
	if(!g_theApp->m_strLastProfileReport.IsEmpty())
	{
		lastpr.Assign(g_theApp->m_strLastProfileReport);
	}
	else
	{
		lastpr.Assign(wxFileName::GetCwd(),wxT(""));
	}
	CSPDReaderDoc *pDoc=(CSPDReaderDoc *)g_theApp->GetDocManager()->GetCurrentDocument();
	wxFileName newpr(pDoc->GetFilename());
	wxFileDialog fd(this,wxT("Choose Output File"),
		lastpr.GetPath(),
		newpr.GetFullName(),
		wxT("SPDReader Report (*.spr)|*.spr"),
		wxSAVE|wxOVERWRITE_PROMPT);
	if(fd.ShowModal()!=wxID_OK)
	{
		g_theApp->GetDocManager()->CloseDocuments();
		return false;
	}

	pDoc->SetFilename(fd.GetPath(),true);
	if(!pDoc->OnSaveDocument(fd.GetPath()))
	{
		wxMessageBox(wxT("Unable to save the report at this location."),wxT("File save error"),wxOK|wxCENTRE|wxICON_ERROR);
		g_theApp->GetDocManager()->CloseDocuments(true);
		return false;
	}

	g_theApp->m_strLastProfileReport=fd.GetPath();
	return true;
}

void CMainFrame::OnImportFromFile(wxCommandEvent &evt)
{
	if(!g_theApp->GetDocManager()->CloseDocuments(false))
	{
		return;
	}
			
	wxFileName lastif(g_theApp->m_strLastImportFile);
	wxFileDialog fd(this,wxT("Import SPD File"), lastif.GetPath(),lastif.GetFullName(),
		wxT("SPD Files (*.spd)|*.spd"),wxOPEN|wxFILE_MUST_EXIST);
	if(fd.ShowModal()==wxID_OK)
	{
		wxString strPathName=fd.GetPath();
		wxFileName fnPath(strPathName);
		fnPath.SetExt(wxT("spr"));
	
		CSPDReaderDoc *pDoc=(CSPDReaderDoc *)(g_theApp->GetDocManager()->CreateDocument(fnPath.GetFullName(),wxDOC_NEW));
		pDoc->ImportData(wxFileInputStream(strPathName));
		pDoc->UpdateAllViews();

		if(!pDoc->IsValid())
		{
			wxMessageBox(wxT("Data was invalid, not importing."),wxT("File import error"),wxOK|wxICON_STOP|wxCENTRE);
			g_theApp->GetDocManager()->CloseDocuments(true);
			return;
		}
		
		g_theApp->m_strLastImportFile=pDoc->GetFilename();

		SaveReport();
	}
}