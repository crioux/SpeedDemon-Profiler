#include"spdreader_pch.h"
#include"spdreader.h"
#include"mainframe.h"
#include"spdreaderdoc.h"
#include"spdreaderview.h"

IMPLEMENT_APP(CSPDReaderApp)

CSPDReaderApp *g_theApp;


//! global print data, to remember settings during the session
wxPrintData *g_printData = (wxPrintData*) NULL;
wxPageSetupData *g_pageSetupData = (wxPageSetupData*) NULL;

// initialize print data and setup


bool CSPDReaderApp::OnInit()
{
#ifdef _WIN32
	HRESULT hr;	
    hr = CoInitialize(NULL);
    if (FAILED(hr))
    {
        return false;
    }
#endif

	g_theApp=this;

	SetAppName(wxT("SPDReader"));
	SetClassName(wxT("SPDReader"));
	SetVendorName(wxT("NoctemWare"));

	m_pDocManager=new wxDocManager();
	m_pDocManager->SetMaxDocsOpen(1);
	
	new wxDocTemplate(
		m_pDocManager,
		wxT("SPDReader Report Files"),
		wxT("*.spr"),
		g_theApp->m_strLastFolder,
		wxT("spr"),
		wxT("SPDReader Report File"),
		wxT("SPDReader View"),
		CLASSINFO(CSPDReaderDoc),
		CLASSINFO(CSPDReaderView));

	m_pConfig=new wxConfig(wxT("SPDReader"));

	ReadConfig();

	g_printData = new wxPrintData;
	g_pageSetupData = new wxPageSetupDialogData;

	long maxstyle=0;
	m_pMainFrame = new CMainFrame(m_pDocManager,wxPoint(m_x,m_y),wxSize(m_w,m_h),wxDEFAULT_FRAME_STYLE);
	if(m_bMaximized)
	{
		m_pMainFrame->Maximize();
	}

	m_pMainFrame->Show(true);
	SetTopWindow(m_pMainFrame);

	m_pDocManager->FileHistoryAddFilesToMenu();
	
	return true;
}

int CSPDReaderApp::OnExit()
{

	WriteConfig();

	delete m_pConfig;
		
#ifdef _WIN32
	CoUninitialize();
#endif

	return wxApp::OnExit();
}



void CSPDReaderApp::ReadConfig()
{
	int max_x=wxSystemSettings::GetMetric(wxSYS_SCREEN_X,m_pMainFrame);
	int max_y=wxSystemSettings::GetMetric(wxSYS_SCREEN_Y,m_pMainFrame);
	
	int def_w=max_x*3/4;
	int def_h=max_y*3/4;
	int def_x=(max_x/2)-(def_w/2);
	int def_y=(max_y/2)-(def_h/2);

	m_w=m_pConfig->Read(wxT("w"),def_w);
	m_h=m_pConfig->Read(wxT("h"),def_h);
	m_x=m_pConfig->Read(wxT("x"),def_x);
	m_y=m_pConfig->Read(wxT("y"),def_y);
	m_bMaximized=(m_pConfig->Read(wxT("maximized"),(long)0))!=0;
	m_strLastFolder=m_pConfig->Read(wxT("lastfolder"),wxFileName(argv[0]).GetPath());
	m_strLastDeviceFile=m_pConfig->Read(wxT("lastdevicefile"),wxT("\\"));
	m_strLastProfileReport=m_pConfig->Read(wxT("lastprofilereport"),wxT(""));
	m_strLastImportFile=m_pConfig->Read(wxT("lastimportfile"),wxT(""));

	m_pDocManager->FileHistoryLoad(*m_pConfig);
}

void CSPDReaderApp::WriteConfig()
{
	m_pDocManager->FileHistorySave(*m_pConfig);

	m_pConfig->Write(wxT("w"),m_w);
	m_pConfig->Write(wxT("h"),m_h);
	m_pConfig->Write(wxT("x"),m_x);
	m_pConfig->Write(wxT("y"),m_y);
	m_pConfig->Write(wxT("maximized"),(long)m_bMaximized);
	m_pConfig->Write(wxT("lastfolder"),m_strLastFolder);
	m_pConfig->Write(wxT("lastdevicefile"),m_strLastDeviceFile);
	m_pConfig->Write(wxT("lastprofilereport"),m_strLastProfileReport);
	m_pConfig->Write(wxT("lastimportfile"),m_strLastImportFile);
}

