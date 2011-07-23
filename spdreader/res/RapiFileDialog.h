/////////////////////////////////////////////////////////////////////////////
// Name:        RapiFileDialog.h
// Author:      XX
// Created:     XX/XX/XX
// Copyright:   XX
/////////////////////////////////////////////////////////////////////////////

#ifndef __RapiFileDialog_H__
#define __RapiFileDialog_H__

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
    #pragma interface "RapiFileDialog.cpp"
#endif

#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "spdreader_wdr.h"

// WDR: class declarations

//----------------------------------------------------------------------------
// CRapiFileDialog
//----------------------------------------------------------------------------

class CRapiFileDialog: public wxDialog
{
public:
    // constructors and destructors
    CRapiFileDialog( wxWindow *parent, wxWindowID id, const wxString &title,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
    
    // WDR: method declarations for CRapiFileDialog
    wxButton* GetCancel()  { return (wxButton*) FindWindow( wxID_CANCEL ); }
    wxButton* GetOk()  { return (wxButton*) FindWindow( wxID_OK ); }
    wxChoice* GetFilesOfType()  { return (wxChoice*) FindWindow( ID_FILES_OF_TYPE ); }
    wxComboBox* GetFileName()  { return (wxComboBox*) FindWindow( ID_FILE_NAME ); }
    wxListCtrl* GetFileList()  { return (wxListCtrl*) FindWindow( ID_FILE_LIST ); }
    wxBitmapButton* GetUp()  { return (wxBitmapButton*) FindWindow( ID_UP ); }
    wxBitmapButton* GetBack()  { return (wxBitmapButton*) FindWindow( ID_BACK ); }
    wxChoice* GetLookIn()  { return (wxChoice*) FindWindow( ID_LOOK_IN ); }

    wxString GetPathName(void) const;
    void SetPathName(wxString strPath, bool bUpdateHistory=true);
    void AddFileType(wxString strText, wxString strMatch);

    void UpdateDialog(void);

private:
    // WDR: member variable declarations for CRapiFileDialog
    
    wxFileName m_pathname;
    std::list<wxString> m_history;
    std::vector<wxString> m_currentpaths;
    std::vector<wxUint64> m_currentsizes;
    std::vector<FILETIME> m_currentdates;
    std::vector<wxString> m_filetypes;

    bool m_bSortDirection;
    int m_nSortColumn;
    
    enum RAPIDIALOGLISTMODE
    {
        REPORTMODE=0,
        ICONMODE,
        SMALLICONMODE,
        LISTMODE
    };

    RAPIDIALOGLISTMODE m_mode;

    static int wxCALLBACK ListCompareFunction(long item1, long item2, long sortData);

private:
    // WDR: handler declarations for CRapiFileDialog
    void OnFileListColClick( wxListEvent &event );
    void OnChooseFilesOfType( wxCommandEvent &event );
    void OnFileNameTextEnter( wxCommandEvent &event );
    void OnFileListItemActivated( wxListEvent &event );
    void OnFileListItemSelected( wxListEvent &event );
    void OnUp( wxCommandEvent &event );
    void OnBack( wxCommandEvent &event );
    void OnLookIn( wxCommandEvent &event );

    

private:
    DECLARE_EVENT_TABLE()
};




#endif
