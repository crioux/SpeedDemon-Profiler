#include"spdreader_pch.h"
#include"RapiFileDialog.h"
#include"rapiaccess.h"

// WDR: class implementations

//----------------------------------------------------------------------------
// CRapiFileDialog
//----------------------------------------------------------------------------

// WDR: event table for CRapiFileDialog

BEGIN_EVENT_TABLE(CRapiFileDialog,wxDialog)
    EVT_CHOICE( ID_LOOK_IN, CRapiFileDialog::OnLookIn )
    EVT_BUTTON( ID_BACK, CRapiFileDialog::OnBack )
    EVT_BUTTON( ID_UP, CRapiFileDialog::OnUp )
    EVT_LIST_ITEM_SELECTED( ID_FILE_LIST, CRapiFileDialog::OnFileListItemSelected )
    EVT_LIST_ITEM_ACTIVATED( ID_FILE_LIST, CRapiFileDialog::OnFileListItemActivated )
    EVT_TEXT_ENTER( ID_FILE_NAME, CRapiFileDialog::OnFileNameTextEnter )
    EVT_CHOICE( ID_FILES_OF_TYPE, CRapiFileDialog::OnChooseFilesOfType )
    EVT_LIST_COL_CLICK( ID_FILE_LIST, CRapiFileDialog::OnFileListColClick )
END_EVENT_TABLE()

CRapiFileDialog::CRapiFileDialog( wxWindow *parent, wxWindowID id, const wxString &title,
    const wxPoint &position, const wxSize& size, long style ) :
    wxDialog( parent, id, title, position, size, style )
{

    // WDR: dialog function RapiFileDialogFunc for CRapiFileDialog
    RapiFileDialogFunc( this, TRUE ); 
    
    GetBack()->SetWindowStyle(0);
    GetBack()->SetBitmapLabel(BackButtonFunc(0));
    GetBack()->SetBitmapFocus(BackButtonFunc(1));
    GetBack()->SetBitmapSelected(BackButtonFunc(0));
    GetBack()->SetBitmapDisabled(BackButtonFunc(2));
    GetUp()->SetWindowStyle(0);
    GetUp()->SetBitmapLabel(UpButtonFunc(0));
    GetUp()->SetBitmapFocus(UpButtonFunc(1));
    GetUp()->SetBitmapSelected(UpButtonFunc(0));
    GetUp()->SetBitmapDisabled(UpButtonFunc(2));

    m_pathname=wxT("\\");
    m_mode=REPORTMODE;
    
    UpdateDialog();

    m_bSortDirection=true;
    m_nSortColumn=0;

    SetSize(wxSize(400,400));
}

// WDR: handler implementations for CRapiFileDialog

void CRapiFileDialog::OnFileListColClick( wxListEvent &event )
{
    if(event.GetColumn()==m_nSortColumn)
	{
		m_bSortDirection=!m_bSortDirection;
	}
	else
	{
		m_nSortColumn=event.GetColumn();
	}
    GetFileList()->SortItems(ListCompareFunction,(long)this);
}

void CRapiFileDialog::OnChooseFilesOfType( wxCommandEvent &event )
{
    UpdateDialog();
}

void CRapiFileDialog::OnFileNameTextEnter( wxCommandEvent &event )
{
    wxFileName newfname(m_pathname.GetPath(true)+GetFileName()->GetValue());

    SetPathName(newfname.GetFullPath());
}

void CRapiFileDialog::OnFileListItemActivated( wxListEvent &event )
{
    long data=GetFileList()->GetItemData(event.GetIndex());
    if(data<0 || data>=(long)m_currentpaths.size())
    {
        return;
    }

    bool bDone=m_currentpaths[data].Last()!=wxT('\\');
    
    SetPathName(m_currentpaths[data]);
    if(bDone)
    {
        EndModal(wxID_OK);
    }
}

void CRapiFileDialog::OnFileListItemSelected( wxListEvent &event )
{
    long data=GetFileList()->GetItemData(event.GetIndex());
    if(data<0 || data>=(long)m_currentpaths.size())
    {
        return;
    }

    if(m_currentpaths[data].Last()!=wxT('\\'))
    {
        SetPathName(m_currentpaths[data]);
    }
}

void CRapiFileDialog::OnUp( wxCommandEvent &event )
{
    wxFileName newfname=m_pathname;
    if(newfname.GetDirCount()>0)
    {
        newfname.RemoveLastDir();
    }

    SetPathName(newfname.GetFullPath());
}

void CRapiFileDialog::OnBack( wxCommandEvent &event )
{
    if(m_history.size()==0)
    {
        return;
    }
    
    wxString strLast=m_history.back();
    m_history.pop_back();
    SetPathName(strLast,false);
}

void CRapiFileDialog::OnLookIn( wxCommandEvent &event )
{
    SetPathName(GetLookIn()->GetStringSelection()+wxT("\\"));    
}

wxString CRapiFileDialog::GetPathName(void) const
{
    return m_pathname.GetFullPath();
}

void CRapiFileDialog::SetPathName(wxString strPath, bool bUpdateHistory)
{
    if(bUpdateHistory && (m_history.size()==0 || m_history.back()!=m_pathname.GetFullPath()))
    {
        m_history.push_back(m_pathname.GetFullPath());
        if(m_history.size()==256)
        {
            m_history.pop_front();
        }
    }
    
    if(strPath.IsEmpty())
    {
        strPath=wxT("\\");
    }

    wxFileName newfname;
    if(strPath.Last()==wxT('\\'))
    {
        newfname=wxFileName(strPath,wxT(""));
    }
    else
    {
        newfname=wxFileName(strPath);
    }

    GetOk()->Enable(strPath.Last()!=wxT('\\'));

    bool bUpdateView=false;
    if(m_pathname.GetPath(false,wxPATH_UNIX)!=newfname.GetPath(false,wxPATH_UNIX))
    {
        bUpdateView=true;
    }

    m_pathname=newfname;

    GetFileName()->SetValue(m_pathname.GetFullName());
    
    if(bUpdateView)
    {
        UpdateDialog();
    }
}

static void ConvertFileTimeToWx(wxDateTime *dt, const FILETIME &ft)
{
    FILETIME ftcopy = ft;
    FILETIME ftLocal;
    if ( !::FileTimeToLocalFileTime(&ftcopy, &ftLocal) )
    {
        wxLogLastError(_T("FileTimeToLocalFileTime"));
    }

    SYSTEMTIME st;
    if ( !::FileTimeToSystemTime(&ftLocal, &st) )
    {
        wxLogLastError(_T("FileTimeToSystemTime"));
    }

    dt->Set(st.wDay, wxDateTime::Month(st.wMonth - 1), st.wYear,
            st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
}

static wxString GetDateString(FILETIME ft)
{
    FILETIME lft;
    FileTimeToLocalFileTime(&ft,&lft);

    wxDateTime dt;
    ConvertFileTimeToWx(&dt,lft);
    return dt.FormatDate();
}

static wxString GetSizeString(wxUint64 size)
{
    if(size<(wxUint64)1024)
    {
        return wxString::Format(wxT("%u Bytes"),size);
    }
    else if(size<(wxUint64)(1024*1024))
    {
        return wxString::Format(wxT("%0.2f KB"),size/1024.0);
    }
    else if(size<(wxUint64)(1024*1024*1024))
    {
        return wxString::Format(wxT("%0.2f MB"),size/(1024.0*1024.0));
    }

    return wxString::Format(wxT("%0.2f GB"),size/(1024.0*1024.0*1024.0));
}


void CRapiFileDialog::AddFileType(wxString strText, wxString strMatch)
{
    GetFilesOfType()->Append(strText);
    m_filetypes.push_back(strMatch);
}

static bool LTFILETIME(FILETIME a,FILETIME b)
{
    if(a.dwHighDateTime<b.dwHighDateTime || (a.dwHighDateTime==b.dwHighDateTime && a.dwLowDateTime<b.dwLowDateTime))
    {
        return true;
    }
    return false;
}

static bool GTFILETIME(FILETIME a,FILETIME b)
{
    if(a.dwHighDateTime>b.dwHighDateTime || (a.dwHighDateTime==b.dwHighDateTime && a.dwLowDateTime>b.dwLowDateTime))
    {
        return true;
    }
    return false;
}

int wxCALLBACK CRapiFileDialog::ListCompareFunction(long item1, long item2, long sortData)
{
    CRapiFileDialog *pThis=(CRapiFileDialog *)sortData;

    if(!pThis->m_bSortDirection)
    {
        long x=item1;
        item1=item2;
        item2=x;
    }

    if(pThis->m_nSortColumn==0 || pThis->m_mode!=REPORTMODE )
    {
        if(pThis->m_currentpaths[item1].Last()==wxT('\\') && pThis->m_currentpaths[item2].Last()!=wxT('\\'))
        {
            return -1;
        }
        else if(pThis->m_currentpaths[item1].Last()!=wxT('\\') && pThis->m_currentpaths[item2].Last()==wxT('\\'))
        {
            return 1;
        }
        return pThis->m_currentpaths[item1].CmpNoCase(pThis->m_currentpaths[item2]);
    }
    else if(pThis->m_nSortColumn==1)
    {
        if(pThis->m_currentsizes[item1]<pThis->m_currentsizes[item2])
        {
            return -1;
        }
        else if(pThis->m_currentsizes[item1]>pThis->m_currentsizes[item2])
        {
            return 1;
        }   
        return 0;
    }
    else if(pThis->m_nSortColumn==2)
    {
        if(LTFILETIME(pThis->m_currentdates[item1],pThis->m_currentdates[item2]))
        {
            return -1;
        }
        else if(GTFILETIME(pThis->m_currentdates[item1],pThis->m_currentdates[item2]))
        {
            return 1;
        }   
        return 0;
    }

    return 0;
}

void CRapiFileDialog::UpdateDialog(void)
{
    m_pathname.MakeAbsolute();
    m_currentpaths.clear();
    m_currentsizes.clear();
    m_currentdates.clear();

    GetFileList()->ClearAll();

    if(m_mode==REPORTMODE)
    {
        GetFileList()->SetWindowStyle(wxLC_REPORT|wxLC_SINGLE_SEL);
        GetFileList()->InsertColumn(0,wxT("Name"),wxLIST_FORMAT_LEFT,160);
        GetFileList()->InsertColumn(1,wxT("Size"));
        GetFileList()->InsertColumn(2,wxT("Date"));
    }
    else if(m_mode==ICONMODE)
    {
        GetFileList()->SetWindowStyle(wxLC_ICON|wxLC_SINGLE_SEL);
    }
    else if(m_mode==SMALLICONMODE)
    {
        GetFileList()->SetWindowStyle(wxLC_SMALL_ICON|wxLC_SINGLE_SEL);
    }
    else if(m_mode==LISTMODE)
    {
        GetFileList()->SetWindowStyle(wxLC_LIST|wxLC_SINGLE_SEL);
    }

    // Ensure we have a file type selection
    if(GetFilesOfType()->GetCount()==0)
    {
        AddFileType(wxT("All Files (*.*)"),wxT("*"));
    }
    if(GetFilesOfType()->GetSelection()==wxNOT_FOUND)
    {
        GetFilesOfType()->SetSelection(0);
    }
    
    // Get item names
    DWORD dwFound;
    LPCE_FIND_DATA pFoundDataArray=NULL;
    if(!g_RAPI.CeFindAllFiles(m_pathname.GetPath(true)+wxT("*"),FAF_ATTRIBUTES|FAF_SIZE_HIGH|FAF_SIZE_LOW|FAF_NAME|FAF_LASTWRITE_TIME,&dwFound,&pFoundDataArray))
    {
        GetFileList()->Enable(false);
    }
    else
    {
        GetFileList()->Enable(true);
    }

    if(pFoundDataArray)
    {
        wxString strMatch=m_filetypes[GetFilesOfType()->GetSelection()];

        // Fill in file list
        for(DWORD dw=0;dw<dwFound;dw++)
        {
            if(pFoundDataArray[dw].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                // It's a folder
                wxFileName dname(m_pathname.GetPath(true)+pFoundDataArray[dw].cFileName,wxT(""));
                m_currentpaths.push_back(dname.GetPath(true));
                m_currentsizes.push_back(0);
                m_currentdates.push_back(pFoundDataArray[dw].ftLastWriteTime);
            }
            else
            {
                // It's a file
                wxFileName dname(m_pathname.GetPath(true)+pFoundDataArray[dw].cFileName);

                if(dname.GetFullName().Lower().Matches(strMatch.Lower()))
                {
                    m_currentpaths.push_back(dname.GetFullPath());
                    m_currentsizes.push_back(((wxUint64)pFoundDataArray[dw].nFileSizeLow)|(((wxUint64)pFoundDataArray[dw].nFileSizeHigh)<<32));
                    m_currentdates.push_back(pFoundDataArray[dw].ftLastWriteTime);
                }
            }
        }
        g_RAPI.CeRapiFreeBuffer(pFoundDataArray);
    }

    wxImageList *pilSmall=new wxImageList(16,16,true,1);
    pilSmall->Add(RapiFileDialogBitmapsFunc(0),wxColour(255,0,255));
    pilSmall->Add(RapiFileDialogBitmapsFunc(1),wxColour(255,0,255));
    pilSmall->Add(RapiFileDialogBitmapsFunc(2),wxColour(255,0,255));

    wxImageList *pilBig=new wxImageList(32,32,true,1);
    pilBig->Add(RapiFileDialogBitmapsLgFunc(0),wxColour(255,0,255));
    pilBig->Add(RapiFileDialogBitmapsLgFunc(1),wxColour(255,0,255));
    pilBig->Add(RapiFileDialogBitmapsLgFunc(2),wxColour(255,0,255));

    size_t i=0,count=m_currentpaths.size();
    for(i=0;i<count;i++)
    {
        if(m_currentpaths[i].Last()==wxT('\\'))
        {
            GetFileList()->InsertItem((long)i,wxFileName(m_currentpaths[i],wxT("")).GetDirs().Last(),0);
            GetFileList()->SetItemImage((long)i,0,1);
            if(m_mode==REPORTMODE)
            {
//              GetFileList()->SetItem((long)i,1,(const TCHAR *)GetSizeString(m_currentsizes[i]));
                GetFileList()->SetItem((long)i,2,(const TCHAR *)GetDateString(m_currentdates[i]));
            }

        }
        else
        {
            GetFileList()->InsertItem((long)i,wxFileName(m_currentpaths[i]).GetFullName(),2);
            if(m_mode==REPORTMODE)
            {
                GetFileList()->SetItem((long)i,1,(const TCHAR *)GetSizeString(m_currentsizes[i]));
                GetFileList()->SetItem((long)i,2,(const TCHAR *)GetDateString(m_currentdates[i]));
            }
        }

        if(m_currentpaths[i].Last()==m_pathname.GetFullPath())
        {
            GetFileList()->SetItemState((long)i,wxLIST_STATE_SELECTED|wxLIST_STATE_FOCUSED,wxLIST_STATE_SELECTED|wxLIST_STATE_FOCUSED);
        }
        GetFileList()->SetItemData((long)i,(long)i);
    }

    GetFileList()->AssignImageList(pilSmall,wxIMAGE_LIST_SMALL);
    GetFileList()->AssignImageList(pilBig,wxIMAGE_LIST_NORMAL);

    // Fill in Look-In Choice Box
    GetLookIn()->Clear();
    count=m_pathname.GetDirCount();
    
    wxString dir=wxT("\\");
    GetLookIn()->Append(dir);        

    for(i=0;i<count;i++)
    {
        dir=wxT("\\");
        for(size_t j=0;j<=i;j++)
        {
            dir+=m_pathname.GetDirs()[j];
            if(j!=i)
            {
                dir+=wxT("\\");
            }
        }
        GetLookIn()->Append(dir);
    }
    GetLookIn()->SetSelection((int)count);

    GetUp()->Enable(count>0);
    GetBack()->Enable(m_history.size()>1);
    GetOk()->Enable(m_pathname.GetFullPath().Last()!=wxT('\\'));

    GetFileList()->SortItems(ListCompareFunction,(long)this);
}


    
