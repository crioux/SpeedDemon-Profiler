extern "C" 
{

#include"tlhelp32.h"

#ifdef WINDOWSCE

#ifdef __cplusplus
extern "C" {
#endif

#define THSNAP_RESERVE (1024*1024)

#define TH32CS_SNAPNOHEAPS	0x40000000	// optimization for text shell to not snapshot heaps

typedef struct TH32HEAPENTRY {
	HEAPENTRY32 heapentry;
	struct TH32HEAPENTRY *pNext;
} TH32HEAPENTRY;

typedef struct TH32HEAPLIST {
	HEAPLIST32 heaplist;
	TH32HEAPENTRY *pHeapEntry;
	struct TH32HEAPLIST *pNext;
} TH32HEAPLIST, *PTH32HEAPLIST;	

typedef struct TH32PROC {
	PROCESSENTRY32 procentry;
	TH32HEAPENTRY *pMainHeapEntry;
	struct TH32PROC *pNext;
} TH32PROC;

typedef struct TH32MOD {
	MODULEENTRY32 modentry;
	struct TH32MOD *pNext;
} TH32MOD;

typedef struct TH32THREAD {
	THREADENTRY32 threadentry;
	struct TH32THREAD *pNext;
} TH32THREAD;

typedef struct THSNAP {
	LPBYTE pNextFree;
	LPBYTE pHighCommit;
	LPBYTE pHighReserve;
	TH32PROC *pProc;
	TH32MOD *pMod;
	TH32THREAD *pThread;
	TH32HEAPLIST *pHeap;

    /* Keep track of the location of snapshot data. */
    TH32HEAPENTRY *pCurHeapEntry;
    TH32HEAPLIST *pCurHeapList;
} THSNAP;


__declspec(dllimport) THSNAP * THCreateSnapshot(DWORD dwFlags, DWORD dwProcID);

#ifdef __cplusplus
}
#endif

#elif defined (WIN32)

typedef struct _UNICODE_STRING {
  USHORT  Length;
  USHORT  MaximumLength;
  PWSTR   Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _LDR_MODULE {

  LIST_ENTRY              InLoadOrderModuleList;
  LIST_ENTRY              InMemoryOrderModuleList;
  LIST_ENTRY              InInitializationOrderModuleList;
  PVOID                   BaseAddress;
  PVOID                   EntryPoint;
  ULONG                   SizeOfImage;
  UNICODE_STRING          FullDllName;
  UNICODE_STRING          BaseDllName;
  ULONG                   Flags;
  SHORT                   LoadCount;
  SHORT                   TlsIndex;
  LIST_ENTRY              HashTableEntry;
  ULONG                   TimeDateStamp;

} LDR_MODULE, *PLDR_MODULE;

typedef struct _PEB_LDR_DATA {


  ULONG                   Length;
  BOOLEAN                 Initialized;
  PVOID                   SsHandle;
  LIST_ENTRY              InLoadOrderModuleList;
  LIST_ENTRY              InMemoryOrderModuleList;
  LIST_ENTRY              InInitializationOrderModuleList;

} PEB_LDR_DATA, *PPEB_LDR_DATA;

typedef struct _PEB 
{
  BOOLEAN                 InheritedAddressSpace;
  BOOLEAN                 ReadImageFileExecOptions;
  BOOLEAN                 BeingDebugged;
  BOOLEAN                 Spare;
  HANDLE                  Mutant;
  PVOID                   ImageBaseAddress;
  PPEB_LDR_DATA           LoaderData;
  PVOID					  ProcessParameters;
  PVOID                   SubSystemData;
  PVOID                   ProcessHeap;
  PVOID                   FastPebLock;
  PVOID					  FastPebLockRoutine;
  PVOID					  FastPebUnlockRoutine;
  ULONG                   EnvironmentUpdateCount;
  PVOID                   KernelCallbackTable;
  PVOID                   EventLogSection;
  PVOID                   EventLog;
  PVOID					  FreeList;
  ULONG                   TlsExpansionCounter;
  PVOID                   TlsBitmap;
  ULONG                   TlsBitmapBits[0x2];
  PVOID                   ReadOnlySharedMemoryBase;
  PVOID                   ReadOnlySharedMemoryHeap;
  PVOID                   ReadOnlyStaticServerData;
  PVOID                   AnsiCodePageData;
  PVOID                   OemCodePageData;
  PVOID                   UnicodeCaseTableData;
  ULONG                   NumberOfProcessors;
  ULONG                   NtGlobalFlag;
  BYTE                    Spare2[0x4];
  LARGE_INTEGER           CriticalSectionTimeout;
  ULONG                   HeapSegmentReserve;
  ULONG                   HeapSegmentCommit;
  ULONG                   HeapDeCommitTotalFreeThreshold;
  ULONG                   HeapDeCommitFreeBlockThreshold;
  ULONG                   NumberOfHeaps;
  ULONG                   MaximumNumberOfHeaps;
  PVOID                   *ProcessHeaps;
  PVOID                   GdiSharedHandleTable;
  PVOID                   ProcessStarterHelper;
  PVOID                   GdiDCAttributeList;
  PVOID                   LoaderLock;
  ULONG                   OSMajorVersion;
  ULONG                   OSMinorVersion;
  ULONG                   OSBuildNumber;
  ULONG                   OSPlatformId;
  ULONG                   ImageSubSystem;
  ULONG                   ImageSubSystemMajorVersion;
  ULONG                   ImageSubSystemMinorVersion;
  ULONG                   GdiHandleBuffer[0x22];
  ULONG                   PostProcessInitRoutine;
  ULONG                   TlsExpansionBitmap;
  BYTE                    TlsExpansionBitmapBits[0x80];
  ULONG                   SessionId;
} PEB, *PPEB;


typedef struct _PROCESS_BASIC_INFORMATION {
    DWORD ExitStatus;
    PPEB PebBaseAddress;
    ULONG_PTR AffinityMask;
    LONG BasePriority;
    ULONG_PTR UniqueProcessId;
    ULONG_PTR InheritedFromUniqueProcessId;
} PBI;

typedef DWORD (NTAPI *TYPEOF_ZWQUERYINFORMATIONPROCESS)(HANDLE, DWORD, PVOID, ULONG, PULONG); 
TYPEOF_ZWQUERYINFORMATIONPROCESS g_pZWQueryInformationProcess=NULL;

inline void *GetPEB(void)
{
	if(g_pZWQueryInformationProcess==NULL)
	{
		HMODULE hModule = GetModuleHandle("ntdll");
		g_pZWQueryInformationProcess=(TYPEOF_ZWQUERYINFORMATIONPROCESS)GetProcAddress(hModule, "ZwQueryInformationProcess");
		if(g_pZWQueryInformationProcess==NULL) 
		{
			return NULL;
		}
	}
	
	PBI Procinfo;
	ULONG ReturnLength;
	if((*g_pZWQueryInformationProcess)(GetCurrentProcess(), 0, &Procinfo, sizeof(Procinfo), &ReturnLength)!=0)
	{
		return NULL;
	}
	
	return Procinfo.PebBaseAddress;
}


#endif

///////////////////////////////////////////////////////////////////////////////////////////////////
// Forward Declarations 

inline void *_xstrlcpy(char *dst, const char *src, size_t maxbufsize);
inline void *_xwcslcpy(wchar_t *dst, const wchar_t *src, size_t maxbufsize);
#ifdef UNICODE
#define _xtcslcpy _xwcslcpy
#else
#define _xtcslcpy _xstrlcpy
#endif
inline size_t _xstrlen(const char *str);

inline int _xstrcmp(const char *str1, const char *str2);
inline void *_xmemset(void *ptr, int val , size_t size);
inline void *_xmemcpy(void *dst, const void *src, size_t size);
inline void *_xalloc(size_t size, unsigned char fill);
inline void _xfree(void *ptr);
inline void *_shmemopen(const TCHAR *name, size_t *psize,FILEMAP *pmap);
inline void _shmemclose(void *ptr,FILEMAP map);
inline void *_shmemcreate(size_t size, const TCHAR *name,FILEMAP *pmap);
inline bool _createmonitor(void);
inline void _xerror(const TCHAR *msg);
inline void __declspec(noreturn) _xterminate(void);
inline void _signalresize(void);
inline void _signaldoneresizing(void);
inline UINT32 _xgetcurrentthreadid(void);
inline UINT32 _xgetcurrentprocessid(void);
inline THREAD _xgetcurrentthread(void);
inline bool _xisthreaddead(THREAD thr);
inline void _xreleasethread(THREAD thr);
inline void _getmoduledebugpath(moduleinfo *pModule);
inline void _updatemodulelist(void);
inline int _findmodulebyname(const char *svModuleName);
inline int _findloadedmodule(ADDRESS addr);
inline void _getmainpath(char *mainpath, size_t mainpathlen);


static inline void _updatemodule(MODULEENTRY32 *me32);
		

///////////////////////////////////////////////////////////////////////////////////////////////////
// Windows Function Definitions


	


inline void *_xstrlcpy(char *dst, const char *src, size_t maxbufsize)
{
	for(size_t x=0;x<maxbufsize;x++)
	{
		char c=((const char *)src)[x];
		((char *)dst)[x]=c;
		if(c==0)
		{
			break;
		}
	}
	dst[maxbufsize-1]=0;
	return dst;
}

inline void *_xwcslcpy(wchar_t *dst, const wchar_t *src, size_t maxbufsize)
{
	for(size_t x=0;x<maxbufsize;x++)
	{
		wchar_t  c=((const wchar_t *)src)[x];
		((wchar_t  *)dst)[x]=c;
		if(c==0)
		{
			break;
		}
	}
	dst[maxbufsize-1]=0;
	return dst;
}

inline int _xstrcmp(const char *str1, const char *str2)
{
	while(*str1!=0 && *str2!=0 && *str1==*str2)
	{
		str1++;
		str2++;
	}
	if(*str1==*str2)
	{
		return 0;
	}
	return ((*str1)>(*str2))?1:-1;
}

inline size_t _xstrlen(const char *str)
{
	const char *str2=str;
	while(*str!=0)
	{
		str++;
	}	
	return str-str2;
}

inline size_t _xwcslen(const wchar_t *str)
{
	const wchar_t *str2=str;
	while(*str!=0)
	{
		str++;
	}	
	return str-str2;
}

#pragma optimize("",off)

inline void *_xmemset(void *ptr, int val , size_t size)
{
	for(size_t x=0;x<size;x++)
	{
		((char *)ptr)[x]=(char)val;
	}
	return ptr;
}

inline void *_xmemcpy(void *dst, const void *src , size_t size)
{
	for(size_t x=0;x<size;x++)
	{
		((char *)dst)[x]=((const char *)src)[x];
	}
	return dst;
}

#pragma optimize("",on)

inline void *_xalloc(size_t size, unsigned char fill) 
{ 
	void *ptr=HeapAlloc(GetProcessHeap(),0,size); 
	if(!ptr)
	{
		return NULL;
	}
	_xmemset(ptr,fill,size);
	return ptr;
}

inline void _xfree(void *ptr) 
{ 
	HeapFree(GetProcessHeap(),0,ptr); 
}

inline void *_shmemopen(const TCHAR *name, size_t *psize, FILEMAP *pHandle)
{ 
	HANDLE hfm=CreateFileMapping(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,1024,name);
	if(hfm==NULL || GetLastError()!=ERROR_ALREADY_EXISTS)
	{
		if(hfm!=NULL)
		{
			CloseHandle(hfm);
		}
		return NULL;
	}

	void *ptr=MapViewOfFile(hfm,FILE_MAP_WRITE|FILE_MAP_READ,0,0,0);
	if(!ptr)
	{
		CloseHandle(hfm);
		*pHandle=NULL;
		return NULL;
	}

	if(psize)
	{
		*psize=(size_t)GetFileSize(hfm,NULL);
	}

	*pHandle=hfm;
	return ptr;
}

inline void _shmemclose(void *ptr, FILEMAP pHandle)
{
	UnmapViewOfFile(ptr);
	CloseHandle(pHandle);
}

inline void *_shmemcreate(size_t size, const TCHAR *name, FILEMAP *pHandle)
{
	HANDLE hfm=CreateFileMapping(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,(DWORD)size,name);
	if(hfm==NULL || GetLastError()==ERROR_ALREADY_EXISTS)
	{
		if(hfm!=NULL)
		{
			CloseHandle(hfm);
		}
		return NULL;
	}

	void *ptr=MapViewOfFile(hfm,FILE_MAP_WRITE|FILE_MAP_READ,0,0,0);
	if(!ptr)
	{
		CloseHandle(hfm);
		*pHandle=NULL;
		return NULL;
	}
	
	_xmemset(ptr,0,size);
	*pHandle=hfm;

	return ptr;
}

inline bool _createmonitor(void)
{
	// Extract sdmon.exe
	HANDLE h=CreateFile(TEXT("sdmon.exe"),GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if(h==INVALID_HANDLE_VALUE)
	{
		_xerror(TEXT("Failed to create monitor executable."));
		_xterminate();
	}

	DWORD dwBytes;
	if(!WriteFile(h,&(g_sdmon[0]),sizeof(g_sdmon),&dwBytes,NULL))
	{
		_xerror(TEXT("Failed to write monitor executable."));
		_xterminate();
	}

	CloseHandle(h);

	// Create the command line for the monitor process, 
	// including our current process id.
	TCHAR cmdline[MAX_PATH];
	wsprintf(cmdline,TEXT("sdmon.exe %u"),GetCurrentProcessId());

	// Create event so we know when the monitor is going
	HANDLE hwait=CreateEvent(NULL,FALSE,FALSE,TEXT("sd_monitor_wait"));
	if(hwait==NULL)
	{
		_xerror(TEXT("Failed to create monitor wait event."));
		_xterminate();
	}

	// Create the monitor process
	PROCESS_INFORMATION pi;
#ifdef WINDOWSCE
	BOOL ret=CreateProcess(TEXT("sdmon.exe"),cmdline,NULL,NULL,FALSE,0,NULL,NULL,NULL,&pi);
#else
	STARTUPINFO si;
	_xmemset(&si,0,sizeof(si));
	si.cb=sizeof(STARTUPINFO);
	BOOL ret=CreateProcess(TEXT("sdmon.exe"),cmdline,NULL,NULL,FALSE,DETACHED_PROCESS,NULL,NULL,&si,&pi);
#endif
	if(!ret)
	{
		_xerror(TEXT("Failed to start monitor process."));
		_xterminate();
	}

	// Wait until the monitor is ready to go
	WaitForSingleObject(hwait,INFINITE);

	// Don't need these handles for anything
	CloseHandle(hwait);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return true;
}

inline void _xerror(const TCHAR *msg)
{
	MessageBox(NULL,msg,TEXT("Profiling Error"),MB_OK|MB_ICONSTOP);
}

inline void __declspec(noreturn) _xterminate(void)
{
	ExitProcess(0);
}

inline void _getmoduledebugpath(moduleinfo *pModule)
{
	
#if (SD_ARCH==SD_X86_WCE) || (SD_ARCH==SD_PXA255_WCE) || (SD_ARCH==SD_PXA270_WCE) 
	TCHAR fname[512];
	MultiByteToWideChar(CP_UTF8,0,pModule->svExePath,-1,fname,512);
	HANDLE hFile=CreateFile(fname,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	if(hFile==INVALID_HANDLE_VALUE)
	{
		pModule->svDebugPath[0]='\0';
		return;
	}
	IMAGE_DOS_HEADER idh;
	DWORD dwBytes;
	if(!ReadFile(hFile,&idh,sizeof(idh),&dwBytes,NULL) || dwBytes!=sizeof(idh))
	{
		CloseHandle(hFile);
		pModule->svDebugPath[0]='\0';
		return;
	}
	SetFilePointer(hFile,idh.e_lfanew,NULL,FILE_BEGIN);
	
	IMAGE_NT_HEADERS32 inh;
	if(!ReadFile(hFile,&inh,sizeof(inh),&dwBytes,NULL) || dwBytes!=sizeof(inh))
	{
		CloseHandle(hFile);
		pModule->svDebugPath[0]='\0';
		return;
	}
	
	CloseHandle(hFile);	

	ADDRESS debugaddr=inh.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress;
	if(debugaddr==0)
	{
		pModule->svDebugPath[0]='\0';
		return;
	}

	IMAGE_DEBUG_DIRECTORY *pidd=(IMAGE_DEBUG_DIRECTORY*)(((char *)pModule->base)+debugaddr);
	if(IsBadReadPtr(pidd,4))
	{
		pModule->svDebugPath[0]='\0';
		return;
	}

	if(pidd->Type==IMAGE_DEBUG_TYPE_CODEVIEW)
	{
		char *ptr=(char *)(((char *)pModule->base)+pidd->AddressOfRawData);
		if(ptr[0]=='N' && ptr[1]=='B' && ptr[2]=='1' &&	ptr[3]=='0') 
		{
			_xstrlcpy(pModule->svDebugPath,(const char *)(ptr+16),sizeof(pModule->svDebugPath));
			return;
		}
		else if (ptr[0]=='R' && ptr[1]=='S' && ptr[2]=='D' && ptr[3]=='S') 
		{
			_xstrlcpy(pModule->svDebugPath,(const char *)(ptr+24),sizeof(pModule->svDebugPath));
			return;
		}
	}


#elif (SD_ARCH==SD_X86_WIN32)
	IMAGE_DOS_HEADER *pidh=(IMAGE_DOS_HEADER *)pModule->base;
	IMAGE_NT_HEADERS32 *pinh=(IMAGE_NT_HEADERS32 *)(((char *)pidh)+pidh->e_lfanew);
	
	ADDRESS debugaddr=(ADDRESS)pinh->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress;
	if(debugaddr==0)
	{
		pModule->svDebugPath[0]='\0';
		return;
	}

	IMAGE_DEBUG_DIRECTORY *pidd=(IMAGE_DEBUG_DIRECTORY*)(((char *)pModule->base)+debugaddr);
	if(pidd->Type==IMAGE_DEBUG_TYPE_CODEVIEW)
	{
		char *ptr=(char *)(((char *)pModule->base)+pidd->AddressOfRawData);
		if(ptr[0]=='N' && ptr[1]=='B' && ptr[2]=='1' &&	ptr[3]=='0') 
		{
			_xstrlcpy(pModule->svDebugPath,(const char *)(ptr+16),sizeof(pModule->svDebugPath));
			return;
		}
		else if (ptr[0]=='R' && ptr[1]=='S' && ptr[2]=='D' && ptr[3]=='S') 
		{
			_xstrlcpy(pModule->svDebugPath,(const char *)(ptr+24),sizeof(pModule->svDebugPath));
			return;
		}
	}

#elif (SD_ARCH==SD_X64_WIN64)

	IMAGE_DOS_HEADER *pidh=(IMAGE_DOS_HEADER *)pModule->base;
	IMAGE_NT_HEADERS64 *pinh=(IMAGE_NT_HEADERS64 *)(((char *)pidh)+pidh->e_lfanew);
	
	ADDRESS debugaddr=(ADDRESS)pinh->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress;
	if(debugaddr==0)
	{
		pModule->svDebugPath[0]='\0';
		return;
	}

	IMAGE_DEBUG_DIRECTORY *pidd=(IMAGE_DEBUG_DIRECTORY*)(((char *)pModule->base)+debugaddr);
	if(pidd->Type==IMAGE_DEBUG_TYPE_CODEVIEW)
	{
		char *ptr=(char *)(((char *)pModule->base)+pidd->AddressOfRawData);
		if(ptr[0]=='N' && ptr[1]=='B' && ptr[2]=='1' &&	ptr[3]=='0') 
		{
			_xstrlcpy(pModule->svDebugPath,(const char *)(ptr+16),sizeof(pModule->svDebugPath));
			return;
		}
		else if (ptr[0]=='R' && ptr[1]=='S' && ptr[2]=='D' && ptr[3]=='S') 
		{
			_xstrlcpy(pModule->svDebugPath,(const char *)(ptr+24),sizeof(pModule->svDebugPath));
			return;
		}
	}
#else
#error "WIN128?"
#endif

	pModule->svDebugPath[0]='\0';
}

#ifdef WINDOWSCE

static inline bool _getbaseandlengthfromfile(TCHAR *path, DWORD *pBase, DWORD *pLen)
{
	HANDLE hFile=CreateFile(path,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	if(hFile==INVALID_HANDLE_VALUE)
	{
		return false;
	}
	IMAGE_DOS_HEADER idh;
	DWORD dwBytes;
	if(!ReadFile(hFile,&idh,sizeof(idh),&dwBytes,NULL) || dwBytes!=sizeof(idh))
	{
		CloseHandle(hFile);
		return false;
	}
	SetFilePointer(hFile,idh.e_lfanew,NULL,FILE_BEGIN);
	
	IMAGE_NT_HEADERS32 inh;
	if(!ReadFile(hFile,&inh,sizeof(inh),&dwBytes,NULL) || dwBytes!=sizeof(inh))
	{
		CloseHandle(hFile);
		return false;
	}
	
	CloseHandle(hFile);	

	*pBase=inh.OptionalHeader.ImageBase;
	*pLen=inh.OptionalHeader.SizeOfImage;

	return true;
}

#endif

inline void _updatemodule(MODULEENTRY32 *me32)
{
	// See if this module has been loaded before
	char modulename[256];
	char modulepath[512];
#ifdef UNICODE
	WideCharToMultiByte(CP_UTF8,0,me32->szModule,-1,modulename,256,NULL,NULL);
	WideCharToMultiByte(CP_UTF8,0,me32->szExePath,-1,modulepath,512,NULL,NULL);
#else
	_xstrlcpy(modulename,me32->szModule,256);
	_xstrlcpy(modulepath,me32->szExePath,512);
#endif
	int mod=_findmodulebyname(modulename);
	if(mod==-1)
	{
		// Create module index for this new module
        mod=DATA->m_pPerfInfo->modulecount;
		if(mod>=(int)DATA->m_pPerfInfo->maxmodules)
		{
			_xerror(TEXT("Exceeded number of profilable modules."));
			_xterminate();
		}
        
		// Fill in module info
		DATA->m_pPerfInfo->modules[mod].base=(ADDRESS)me32->modBaseAddr;
		DATA->m_pPerfInfo->modules[mod].length=(ADDRESS)me32->modBaseSize;
		DATA->m_pPerfInfo->modules[mod].profileentries=0;
		DATA->m_pPerfInfo->modules[mod].profileexits=0;
		_xstrlcpy(DATA->m_pPerfInfo->modules[mod].svName,modulename,256);
#ifdef WINDOWSCE
		TCHAR fullname[512];
		GetModuleFileName(me32->hModule,fullname,512);
		WideCharToMultiByte(CP_UTF8,0,fullname,-1,DATA->m_pPerfInfo->modules[mod].svExePath,512,NULL,NULL);
#else
		_xstrlcpy(DATA->m_pPerfInfo->modules[mod].svExePath,modulepath,512);
#endif
		_getmoduledebugpath(&(DATA->m_pPerfInfo->modules[mod]));
		DATA->m_pPerfInfo->modulecount++;
	}
	else
	{
		// Record that this module is loaded at this location,
		// since it may have been relocated since last time we saw it
		DATA->m_pPerfInfo->modules[mod].base=(ADDRESS)me32->modBaseAddr;
		DATA->m_pPerfInfo->modules[mod].length=(ADDRESS)me32->modBaseSize;
	}
	
	// State that it's loaded
    if(DATA->m_nLoadedModuleCount==MAX_MODULES)
	{
		_xerror(TEXT("Exceeded number of profilable loaded modules."));
		_xterminate();
	}
	DATA->m_pLoadedModules[DATA->m_nLoadedModuleCount]=&(DATA->m_pPerfInfo->modules[mod]);
	DATA->m_nLoadedModuleCount++;	

#ifdef PE32_MODULE_TAGGING
	// Tag the module so we can verify we're in the right place later for loadlibrary/freelibrary
	// We tag inside the pe32 header timestamp field
	DWORD dwOldProt;
    if(VirtualProtect((LPVOID)(DATA->m_pPerfInfo->modules[mod].base+4),4,PAGE_READWRITE,&dwOldProt) ||
	   VirtualAlloc((LPVOID)DATA->m_pPerfInfo->modules[mod].base,8,MEM_COMMIT,PAGE_READWRITE))
	{
		DATA->m_pPerfInfo->modules[mod].signature=DATA->m_nNextModuleSeqNum;
		((UINT32 *)DATA->m_pPerfInfo->modules[mod].base)[1]=DATA->m_nNextModuleSeqNum;
		DATA->m_nNextModuleSeqNum++;
		VirtualProtect((LPVOID)(DATA->m_pPerfInfo->modules[mod].base+4),4,dwOldProt,&dwOldProt);
	}
	else
	{
		DATA->m_pPerfInfo->modules[mod].signature=0;
	}
	
#endif

	// Fill fast module searching table
#ifdef FAST_MODULE_TABLE
	for(size_t i=(DATA->m_pPerfInfo->modules[mod].base>>16);
		i<=((DATA->m_pPerfInfo->modules[mod].base+DATA->m_pPerfInfo->modules[mod].length-1+65535)>>16);
		i++)
	{
		DATA->m_pLoadedModuleTable[i]=(DATA->m_nLoadedModuleCount-1);
	}
#else

#endif

}

inline void _updatemodulelist(void)
{
	

	// Clear loaded module list
	DATA->m_nLoadedModuleCount=0;
#ifdef FAST_MODULE_TABLE
	if(!DATA->m_pLoadedModuleTable)
	{
		DATA->m_pLoadedModuleTable=(int*)_xalloc(65536*sizeof(int),0xFF);
	}
	else
	{
		_xmemset(DATA->m_pLoadedModuleTable,0xFF,65536*sizeof(int));
	}
#endif

#ifdef WINDOWSCE

	HMODULE thismod=GetModuleHandle(NULL);
	TCHAR modfname[512];
	GetModuleFileName(NULL,modfname,512);
	TCHAR modname[512];
	int p;
	for(p=(int)_xwcslen(modfname)-1;p>=0;p--)
	{
		if(modfname[p]==TEXT('\\'))
		{
			p++;
			break;
		}
	}
	_xtcslcpy(modname,modfname+p,512);
	
	MODULEENTRY32 m;
	m.dwSize=sizeof(MODULEENTRY32);
	m.GlblcntUsage=1;
	m.hModule=thismod;
	m.th32ModuleID=(DWORD)thismod;
	m.th32ProcessID=GetCurrentProcessId();
	m.ProccntUsage=1;
	_xwcslcpy(m.szExePath,modfname,MAX_PATH);
	_xwcslcpy(m.szModule,modname,MAX_PATH);
	
	DWORD dwBase,dwLen;
	_getbaseandlengthfromfile(m.szExePath,&dwBase,&dwLen);

	m.modBaseAddr=(BYTE *)dwBase;
	m.modBaseSize=dwLen;

	_updatemodule(&m);

#endif

#if defined(WINDOWSCE)
	THSNAP *data=THCreateSnapshot(TH32CS_SNAPMODULE,(DWORD)GetCurrentProcess());
	if(data==(THSNAP *)-1)
	{
		DebugBreak();
	}
	// Get module list snapshot from operating system 
	TH32MOD *mod=data->pMod;
	while(mod)
	{
		_updatemodule(&(mod->modentry));
		mod=mod->pNext;
	}
    VirtualFree(data,THSNAP_RESERVE,MEM_DECOMMIT);
    VirtualFree(data,0,MEM_RELEASE);

#elif defined(WIN32)

	PEB *peb=(PEB *)GetPEB();
	PPEB_LDR_DATA pldr=peb->LoaderData;
	LDR_MODULE *pmodhead=(LDR_MODULE *)&(pldr->InLoadOrderModuleList);
	LDR_MODULE *pmod=pmodhead;
	while(pmod->InLoadOrderModuleList.Flink!=(LIST_ENTRY *)pmodhead)
	{
		pmod=(LDR_MODULE *)pmod->InLoadOrderModuleList.Flink;

		MODULEENTRY32 m;
		m.dwSize=sizeof(MODULEENTRY32);
		m.th32ModuleID=(DWORD)(size_t)pmod->BaseAddress;
		m.th32ProcessID=GetCurrentProcessId();
		m.GlblcntUsage=pmod->LoadCount; // not technically right, but who cares
		m.ProccntUsage=pmod->LoadCount;
		m.modBaseAddr=(BYTE *)pmod->BaseAddress;
		m.modBaseSize=pmod->SizeOfImage;
		m.hModule=GetModuleHandleW(pmod->BaseDllName.Buffer);
		
		WideCharToMultiByte(CP_UTF8,0,pmod->BaseDllName.Buffer,pmod->BaseDllName.Length,m.szModule,MAX_MODULE_NAME32+1,NULL,NULL);
		WideCharToMultiByte(CP_UTF8,0,pmod->FullDllName.Buffer,pmod->FullDllName.Length,m.szExePath,MAX_PATH,NULL,NULL);

		_updatemodule(&m);
	} 

#endif

	
}

inline int _findmodulebyname(const char *svModuleName)
{
	for(UINT32 i=0;i<DATA->m_pPerfInfo->modulecount;i++)
	{
		if(_xstrcmp(DATA->m_pPerfInfo->modules[i].svName,svModuleName)==0)
		{
			return (int)i;
		}
	}
	return -1;
}

inline int _findloadedmodule_internal(ADDRESS addr)
{
#ifdef FAST_MODULE_TABLE
	return DATA->m_pLoadedModuleTable[addr>>16];
#else
	for(UINT32 i=0;i<DATA->m_nLoadedModuleCount;i++)
	{
		if(DATA->m_pLoadedModules[i]->base<=addr &&
		   (DATA->m_pLoadedModules[i]->base+DATA->m_pLoadedModules[i]->length)>addr)
		{
			return (int)i;
			break;
		}
	}
	return -1;
#endif
}

inline int _findloadedmodule(ADDRESS addr)
{
	int mod=_findloadedmodule_internal(addr);

#ifdef PE32_MODULE_TAGGING

#if WINDOWSCE==1
	__try
	{
#endif
		// Verify module is correct by checking 
		if(mod==-1 || (DATA->m_pLoadedModules[mod]->signature!=0 &&
					DATA->m_pLoadedModules[mod]->signature!=((UINT32 *)DATA->m_pLoadedModules[mod]->base)[1]))
		{
			_updatemodulelist();
			return _findloadedmodule_internal(addr);
		}
#if WINDOWSCE==1
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		_updatemodulelist();
		return _findloadedmodule_internal(addr);
	}
#endif

#endif

	return mod;
}

inline void _getmainpath(char *mainpath, size_t mainpathlen)
{
	TCHAR fname[512];
	GetModuleFileName(NULL,fname,512);
#ifdef UNICODE
	WideCharToMultiByte(CP_UTF8,0,fname,-1,mainpath,mainpathlen,NULL,NULL);
#else
	_xstrlcpy(mainpath,fname,mainpathlen);
#endif
}


void _xyield(void)
{
	Sleep(0);
}

void _xsleep(UINT32 msec)
{
	Sleep(msec);
}


inline void _signalresize(void)
{
	HANDLE hResizeEvent=OpenEvent(EVENT_ALL_ACCESS,FALSE,TEXT("sd_resize"));
	if(!hResizeEvent)
	{
		_xerror(TEXT("Resize event doesn't exist"));
		_xterminate();
	}
	SetEvent(hResizeEvent);
	CloseHandle(hResizeEvent);
}

inline void _signaldoneresizing(void)
{
	HANDLE hSdMonSyncEvent=OpenEvent(EVENT_ALL_ACCESS,FALSE,TEXT("sdmon_sync"));
	HANDLE hResizeEvent=OpenEvent(EVENT_ALL_ACCESS,FALSE,TEXT("sd_doneresizing"));
	if(!hResizeEvent)
	{
		_xerror(TEXT("Resize event doesn't exist"));
		_xterminate();
	}
	SetEvent(hResizeEvent);
	CloseHandle(hResizeEvent);
	
	WaitForSingleObject(hSdMonSyncEvent,INFINITE);
	CloseHandle(hSdMonSyncEvent);
}


inline UINT32 _xgetcurrentthreadid(void)
{
	return GetCurrentThreadId();
}

inline UINT32 _xgetcurrentprocessid(void)
{
	return GetCurrentProcessId();
}

#ifdef WINDOWSCE

	inline THREAD _xgetcurrentthread(void)
	{
		
		return GetCurrentThreadId();
	}

	inline void _xreleasethread(THREAD thr)
	{
	}

	inline bool _xisthreaddead(THREAD thr)
	{
		return (WaitForSingleObject((HANDLE)thr,0)==WAIT_OBJECT_0);		
	}

#else

	inline THREAD _xgetcurrentthread(void)
	{
		return GetCurrentThread();
	}

	inline void _xreleasethread(THREAD thr)
	{
		CloseHandle(thr);
	}

	inline bool _xisthreaddead(THREAD thr)
	{
		return (WaitForSingleObject(thr,0)==WAIT_OBJECT_0);
	}

#endif


}