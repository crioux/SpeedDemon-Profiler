#ifndef __INC_SPDREADER_H
#define __INC_SPDREADER_H

class CMainFrame;

class CSPDReaderApp: public wxApp
{
public:
	// Configuration
	int m_x;
	int m_y;
	int m_w;
	int m_h;
	bool m_bMaximized;
	wxString m_strLastFolder;
	wxString m_strLastDeviceFile;
	wxString m_strLastProfileReport;
	wxString m_strLastImportFile;

private:

	wxDocManager *m_pDocManager;
	CMainFrame *m_pMainFrame;
	wxConfig *m_pConfig;
	
public:
	virtual bool OnInit();
	virtual int OnExit();

	void ReadConfig();
	void WriteConfig();

	CMainFrame *GetMainFrame() { return m_pMainFrame; }
	wxConfig *GetConfig() { return m_pConfig; }
	wxDocManager *GetDocManager() { return m_pDocManager; }
};

DECLARE_APP(CSPDReaderApp)

extern CSPDReaderApp *g_theApp;

#endif