#include<windows.h>
#include"CBufferedOutputFile.h"

#define BUFFER_SIZE (65536)

inline void *_xmemcpy(void *dst, const void *src , size_t size)
{
	for(size_t x=0;x<size;x++)
	{
		((char *)dst)[x]=((const char *)src)[x];
	}
	return dst;
}


CBufferedOutputFile::CBufferedOutputFile()
{
	m_bIsValid=false;
	m_pBuffer=HeapAlloc(GetProcessHeap(),0,BUFFER_SIZE);
	m_nBufferTail=0;
	m_hFile=NULL;
}

CBufferedOutputFile::CBufferedOutputFile(const TCHAR *fname)
{
	m_pBuffer=HeapAlloc(GetProcessHeap(),0,BUFFER_SIZE);
	m_nBufferTail=0;
	m_hFile=CreateFile(fname,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if(m_hFile==INVALID_HANDLE_VALUE || m_pBuffer==NULL)
	{
		if(m_hFile!=INVALID_HANDLE_VALUE)
		{
			CloseHandle(m_hFile);
		}
		m_bIsValid=false;
		m_hFile=NULL;
	}
	else
	{
		m_bIsValid=true;
	}
}

CBufferedOutputFile::~CBufferedOutputFile()
{
	if(m_bIsValid)
	{
		Close();
	}

	if(m_pBuffer!=NULL)
	{
		HeapFree(GetProcessHeap(),0,m_pBuffer);
	}
	if(m_hFile)
	{
		CloseHandle(m_hFile);
	}
}

bool CBufferedOutputFile::IsValid(void)
{
	return m_bIsValid;
}

bool CBufferedOutputFile::Open(const TCHAR *fname)
{
	if(m_pBuffer==NULL || m_bIsValid)
	{
		return false;
	}

	m_hFile=CreateFile(fname,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if(m_hFile==INVALID_HANDLE_VALUE)
	{
		return false;
	}

	m_bIsValid=true;
	m_nBufferTail=0;
    
	return true;
}

void CBufferedOutputFile::Close(void)
{
	if(!m_bIsValid)
	{
		return;
	}

	Flush();
	CloseHandle(m_hFile);
	m_hFile=NULL;
	m_bIsValid=false;
	m_nBufferTail=0;
}

void CBufferedOutputFile::Write(const void *data, size_t length)
{
	size_t remaining=length;
	size_t currentpos=0;

	while((remaining+m_nBufferTail)>=BUFFER_SIZE)
	{
		size_t willfit=BUFFER_SIZE-m_nBufferTail;
		
		// Copy as many bytes as will fit into the buffer
		_xmemcpy(((char *)m_pBuffer)+m_nBufferTail,((char *)data)+currentpos,willfit);
		m_nBufferTail=0;

		DWORD dwBytes;
		WriteFile(m_hFile,m_pBuffer,BUFFER_SIZE,&dwBytes,NULL);
		
		remaining-=willfit;
		currentpos+=willfit;
	}
	if(remaining>0)
	{
		_xmemcpy(((char *)m_pBuffer)+m_nBufferTail,((char *)data)+currentpos,remaining);
		m_nBufferTail+=remaining;
	}
}

void CBufferedOutputFile::Flush(void)
{
	DWORD dwBytes;
	WriteFile(m_hFile,m_pBuffer,(DWORD)m_nBufferTail,&dwBytes,NULL);
	m_nBufferTail=0;
	FlushFileBuffers(m_hFile);
}


