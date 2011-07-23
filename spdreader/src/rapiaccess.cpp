#include"spdreader_pch.h"
#include"rapi.h"
#include"rapiaccess.h"

RAPI::RAPI()
{
	m_hRAPI=LoadLibrary(TEXT("rapi.dll"));
	if(m_hRAPI==NULL)
	{
		m_pCeFindAllFiles=NULL;
		m_pCeRapiFreeBuffer=NULL;
		m_pCeRapiInit=NULL;
		m_pCeRapiUninit=NULL;
		m_pCeCreateFile=NULL;
		m_pCeReadFile=NULL;
		m_pCeGetFileSize=NULL;
		m_pCeCloseHandle=NULL;
	}
	else
	{
		m_pCeFindAllFiles=(FUNC_CeFindAllFiles)GetProcAddress(m_hRAPI,"CeFindAllFiles");
		m_pCeRapiFreeBuffer=(FUNC_CeRapiFreeBuffer)GetProcAddress(m_hRAPI,"CeRapiFreeBuffer");
		m_pCeRapiInit=(FUNC_CeRapiInit)GetProcAddress(m_hRAPI,"CeRapiInit");
		m_pCeRapiUninit=(FUNC_CeRapiUninit)GetProcAddress(m_hRAPI,"CeRapiUninit");
		m_pCeCreateFile=(FUNC_CeCreateFile)GetProcAddress(m_hRAPI,"CeCreateFile");
		m_pCeReadFile=(FUNC_CeReadFile)GetProcAddress(m_hRAPI,"CeReadFile");
		m_pCeGetFileSize=(FUNC_CeGetFileSize)GetProcAddress(m_hRAPI,"CeGetFileSize");
		m_pCeCloseHandle=(FUNC_CeCloseHandle)GetProcAddress(m_hRAPI,"CeCloseHandle");
	}
}

RAPI::~RAPI()
{
	if(m_hRAPI)
	{
		FreeLibrary(m_hRAPI);
	}
}

bool RAPI::IsValid()
{
	return m_hRAPI!=NULL;
}

BOOL RAPI::CeFindAllFiles(LPCWSTR a, DWORD b, LPDWORD c, LPLPCE_FIND_DATA d)
{
	if(m_hRAPI==NULL)
	{
		wxASSERT(0);
		return FALSE;
	}

	return (*m_pCeFindAllFiles)(a,b,c,d);
}

HRESULT RAPI::CeRapiFreeBuffer(LPVOID a)
{
	if(m_hRAPI==NULL)
	{
		wxASSERT(0);
		return -1;
	}
	return (*m_pCeRapiFreeBuffer)(a);
}

HRESULT RAPI::CeRapiInit()
{
	if(m_hRAPI==NULL)
	{
		wxASSERT(0);
		return -1;
	}
	return (*m_pCeRapiInit)();
}

HRESULT RAPI::CeRapiUninit()
{
	if(m_hRAPI==NULL)
	{
		wxASSERT(0);
		return -1;
	}
	return (*m_pCeRapiUninit)();
}

		

HANDLE RAPI::CeCreateFile(LPCWSTR a, DWORD b, DWORD c, LPSECURITY_ATTRIBUTES d, DWORD e, DWORD f, HANDLE g)
{
	if(m_hRAPI==NULL)
	{
		wxASSERT(0);
		return INVALID_HANDLE_VALUE;
	}
	return (*m_pCeCreateFile)(a,b,c,d,e,f,g);
}

BOOL RAPI::CeReadFile(HANDLE a, LPVOID b, DWORD c, LPDWORD d, LPOVERLAPPED e)
{
	if(m_hRAPI==NULL)
	{
		wxASSERT(0);
		return FALSE;
	}
	return (*m_pCeReadFile)(a,b,c,d,e);
}

DWORD RAPI::CeGetFileSize(HANDLE a, LPDWORD b)
{
	if(m_hRAPI==NULL)
	{
		wxASSERT(0);
		return 0;
	}
	return (*m_pCeGetFileSize)(a,b);
}

BOOL RAPI::CeCloseHandle(HANDLE a)
{
	if(m_hRAPI==NULL)
	{
		wxASSERT(0);
		return FALSE;
	}
	return (*m_pCeCloseHandle)(a);
}


RAPI g_RAPI;