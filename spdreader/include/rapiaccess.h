#ifndef __INC_RAPIACCESS_H
#define __INC_RAPIACCESS_H

#include<rapi.h>

extern "C"
{
	typedef BOOL (STDAPICALLTYPE *FUNC_CeFindAllFiles)(LPCWSTR, DWORD, LPDWORD, LPLPCE_FIND_DATA);
	typedef HRESULT (STDAPICALLTYPE *FUNC_CeRapiFreeBuffer)(LPVOID);
	typedef HRESULT (STDAPICALLTYPE *FUNC_CeRapiInit)();
	typedef HRESULT (STDAPICALLTYPE *FUNC_CeRapiUninit)();
	typedef HANDLE (STDAPICALLTYPE *FUNC_CeCreateFile)(LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
	typedef BOOL (STDAPICALLTYPE *FUNC_CeReadFile)(HANDLE, LPVOID, DWORD, LPDWORD, LPOVERLAPPED);
	typedef DWORD (STDAPICALLTYPE *FUNC_CeGetFileSize)(HANDLE, LPDWORD);
	typedef BOOL (STDAPICALLTYPE *FUNC_CeCloseHandle)(HANDLE);

};

class RAPI
{
private:
	FUNC_CeRapiInit m_pCeRapiInit;
	FUNC_CeRapiUninit m_pCeRapiUninit;
	FUNC_CeFindAllFiles m_pCeFindAllFiles;
	FUNC_CeRapiFreeBuffer m_pCeRapiFreeBuffer;
	FUNC_CeCreateFile m_pCeCreateFile;
	FUNC_CeReadFile m_pCeReadFile;
	FUNC_CeGetFileSize m_pCeGetFileSize;
	FUNC_CeCloseHandle m_pCeCloseHandle;

	HMODULE m_hRAPI;

public:
	RAPI();
	~RAPI();
	bool IsValid();

public:
	HRESULT CeRapiInit();
	HRESULT CeRapiUninit();
	BOOL CeFindAllFiles(LPCWSTR a, DWORD b, LPDWORD c, LPLPCE_FIND_DATA d);
	HRESULT CeRapiFreeBuffer(LPVOID a);
	HANDLE CeCreateFile(LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
	BOOL CeReadFile(HANDLE, LPVOID, DWORD, LPDWORD, LPOVERLAPPED);
	DWORD CeGetFileSize(HANDLE, LPDWORD);
	BOOL CeCloseHandle(HANDLE);
};

extern RAPI g_RAPI;

#endif