// sdtest_ppc1.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "sdtest_ppc1.h"
#include "MainFrm.h"

#include "sdtest_ppc1Doc.h"
#include "sdtest_ppc1View.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// Csdtest_ppc1App

BEGIN_MESSAGE_MAP(Csdtest_ppc1App, CWinApp)
	ON_COMMAND(ID_APP_ABOUT, &Csdtest_ppc1App::OnAppAbout)
	ON_COMMAND(ID_FILE_NEW, &CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, &CWinApp::OnFileOpen)
END_MESSAGE_MAP()



// Csdtest_ppc1App construction
Csdtest_ppc1App::Csdtest_ppc1App()
	: CWinApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only Csdtest_ppc1App object
Csdtest_ppc1App theApp;

// Csdtest_ppc1App initialization

BOOL Csdtest_ppc1App::InitInstance()
{
    // SHInitExtraControls should be called once during your application's initialization to initialize any
    // of the Windows Mobile specific controls such as CAPEDIT and SIPPREF.
    SHInitExtraControls();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));
	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(Csdtest_ppc1Doc),
		RUNTIME_CLASS(CMainFrame),       // main SDI frame window
		RUNTIME_CLASS(Csdtest_ppc1View));
	if (!pDocTemplate)
		return FALSE;
	AddDocTemplate(pDocTemplate);

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	// Dispatch commands specified on the command line.  Will return FALSE if
	// app was launched with /RegServer, /Register, /Unregserver or /Unregister.
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	// The one and only window has been initialized, so show and update it
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();
	return TRUE;
}


// CAboutDlg dialog used for App About
class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
#ifdef _DEVICE_RESOLUTION_AWARE
	afx_msg void OnSize(UINT /*nType*/, int /*cx*/, int /*cy*/);
#endif
	virtual BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BOOL CAboutDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	return TRUE;	// return TRUE unless you set the focus to a control
			// EXCEPTION: OCX Property Pages should return FALSE
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
#ifdef _DEVICE_RESOLUTION_AWARE
	ON_WM_SIZE()
#endif
END_MESSAGE_MAP()

#ifdef _DEVICE_RESOLUTION_AWARE
void CAboutDlg::OnSize(UINT /*nType*/, int /*cx*/, int /*cy*/)
{
	DRA::RelayoutDialog(
		AfxGetInstanceHandle(), 
		this->m_hWnd, 
		DRA::GetDisplayMode() != DRA::Portrait ? MAKEINTRESOURCE(IDD_ABOUTBOX_WIDE) : MAKEINTRESOURCE(IDD_ABOUTBOX));
}
#endif

// App command to run the dialog
void Csdtest_ppc1App::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}
