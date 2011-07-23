#include"sdconfig.h"

#ifdef _USRDLL
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
// Hardware Abstractions

#if SD_ARCH == SD_PXA270_WCE
#include"pxa270.inl"
#elif SD_ARCH == SD_PXA255_WCE
#include"pxa255.inl"
#elif (SD_ARCH == SD_X86_WIN32) || (SD_ARCH == SD_X86_WCE)
#include"x86.inl"
#elif (SD_ARCH == SD_X64_WIN64)
#include"x64.inl"
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
// Storage structures


#include "perfinfo.inl"


UINT64 g_nStartTime;
UINT64 g_nEndTime;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
// SpeedDemon Monitor Function Definition

#ifdef WINDOWSCODE


#include "CBufferedOutputFile.h"

extern "C" 
{

void _xyield(void)
{
	Sleep(0);
}

}

inline void _xerror(const TCHAR *msg)
{
	MessageBox(NULL,msg,TEXT("Profiling Error"),MB_OK|MB_ICONSTOP);
}

inline void __declspec(noreturn) _xterminate(void)
{
	ExitProcess(0);
}

inline size_t _xstrlen( const char *string )
{
	const char *str2=string;
	while(*str2!='\0')
	{
		str2++;
	}
	return (str2-string);
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

void Flush(const TCHAR *fname, perfinfo *info,perfdata *data)
{
//	DebugBreak();

	CBufferedOutputFile f;
	if(!f.Open(fname))
	{
		TCHAR msg[1024];
		wsprintf(msg,TEXT("Couldn't open %s for writing"),fname);
		_xerror(msg);
		_xterminate();
	}

	// Calibrate timing system
	UINT64 cal_start,cal_end;
	_xinitclock();
	_xupdateclock(&cal_start);
	Sleep(1000);
	_xupdateclock(&cal_end);
	_xresetclock();
	UINT64 tickspersecond=(cal_end-cal_start);

	f.Write(&(info->targetmode),sizeof(info->targetmode));
	f.Write(&(info->mainpath),sizeof(info->mainpath));
	f.Write(&(info->perfdatasize),sizeof(info->perfdatasize));
	f.Write(&(info->modulecount),sizeof(info->modulecount));
	f.Write(&(tickspersecond),sizeof(tickspersecond));
	f.Write(&(info->entertime),sizeof(info->entertime));
	f.Write(&(info->exittime),sizeof(info->exittime));
	
	for(DWORD m=0;m<info->modulecount;m++)
	{
		f.Write(&(info->modules[m].base),sizeof((info->modules[m].base)));
		f.Write(&(info->modules[m].length),sizeof((info->modules[m].length)));
		f.Write(&(info->modules[m].profileentries),sizeof((info->modules[m].profileentries)));
		f.Write(&(info->modules[m].profileentries),sizeof((info->modules[m].profileexits)));
		f.Write(&(info->modules[m].svName),sizeof((info->modules[m].svName)));
		f.Write(&(info->modules[m].svExePath),sizeof((info->modules[m].svExePath)));
		f.Write(&(info->modules[m].svDebugPath),sizeof((info->modules[m].svDebugPath)));
	}
	
	for(DWORD i=0;i<info->perfdatasize;i++)
	{
		f.Write(&(data[i].nParentAddr_BASED),sizeof(data[i].nParentAddr_BASED));
		f.Write(&(data[i].nParentModule),sizeof(data[i].nParentModule));
		f.Write(&(data[i].nFromAddr_BASED),sizeof(data[i].nFromAddr_BASED));
		f.Write(&(data[i].nToAddr_BASED),sizeof(data[i].nToAddr_BASED));
		f.Write(&(data[i].nToModule),sizeof(data[i].nToModule));
		f.Write(&(data[i].nThreadSeqNum),sizeof(data[i].nThreadSeqNum));
		f.Write(&(data[i].nTotalCalls),sizeof(data[i].nTotalCalls));
		f.Write(&(data[i].nTotalTime),sizeof(data[i].nTotalTime));
		f.Write(&(data[i].nFirstStartTime),sizeof(data[i].nFirstStartTime));
		f.Write(&(data[i].nLastEndTime),sizeof(data[i].nLastEndTime));
		f.Write(&(data[i].nMinTime),sizeof(data[i].nMinTime));
		f.Write(&(data[i].nMaxTime),sizeof(data[i].nMaxTime));
		f.Write(&(data[i].nTotalNodeFCTime),sizeof(data[i].nTotalNodeFCTime));
		f.Write(&(data[i].nMinNodeFCTime),sizeof(data[i].nMinNodeFCTime));
		f.Write(&(data[i].nMaxNodeFCTime),sizeof(data[i].nMaxNodeFCTime));
		f.Write(&(data[i].nTotalEdgeFCTime),sizeof(data[i].nTotalEdgeFCTime));
		f.Write(&(data[i].nMinEdgeFCTime),sizeof(data[i].nMinEdgeFCTime));
		f.Write(&(data[i].nMaxEdgeFCTime),sizeof(data[i].nMaxEdgeFCTime));
	}

	f.Close();
}


#ifdef WINDOWSCE
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpwCmdLine, int nCmdShow)
{
#else
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
#endif

//	DebugBreak();

	LPWSTR cmdlinew=GetCommandLineW();
	char cmdline[256];
	WideCharToMultiByte(CP_UTF8,0,cmdlinew,-1,cmdline,256,NULL,NULL);		

	if(cmdline[0]=='\0')
	{
		_xerror(TEXT("Invalid command line."));
		_xterminate();
	}

	char *endptr=cmdline+_xstrlen(cmdline)-1;

	// Get the process id from the command line
    DWORD id=0;
	char *c=endptr;
	int place=1;
	while(c!=cmdline && (*c)>='0' && (*c)<='9')
	{
		id+=((DWORD)(unsigned char)((*c)-'0'))*place;
		place*=10;
		c--;
	}

	if(id==0)
	{
		_xerror(TEXT("SpeedDemon Monitor program is not intended to be run directly."));
		_xterminate();
	}

	// Open the process
	HANDLE hProcess=OpenProcess(SYNCHRONIZE,FALSE,id);
	if(hProcess==NULL)
	{
		_xerror(TEXT("Monitor couldn't open profiling process."));
		_xterminate();
	}

	// Create the three resize events
	HANDLE hResize=CreateEvent(NULL,FALSE,FALSE,TEXT("sd_resize"));
	if(hResize==NULL)
	{
		_xerror(TEXT("Monitor couldn't create resize event."));
		_xterminate();
	}
	HANDLE hDoneResizing=CreateEvent(NULL,FALSE,FALSE,TEXT("sd_doneresizing"));
	if(hDoneResizing==NULL)
	{
		_xerror(TEXT("Monitor couldn't create resizing event."));
		_xterminate();
	}
	HANDLE hSdMonSync=CreateEvent(NULL,FALSE,FALSE,TEXT("sdmon_sync"));
	if(hSdMonSync==NULL)
	{
		_xerror(TEXT("Monitor couldn't create synchronizing event."));
		_xterminate();
	}

	// Open the initialization event
	HANDLE hInitEvent=OpenEvent(EVENT_ALL_ACCESS,FALSE,TEXT("sd_monitor_wait"));
	if(hInitEvent==NULL)
	{
		_xerror(TEXT("Monitor couldn't open initialization wait event."));
		_xterminate();
	}

	// Open the shared memory objects
	TCHAR perfinfoname[256];
	wsprintf(perfinfoname,TEXT("sd_perfinfo_%d"),id);

	HANDLE hperfinfo;
	perfinfo *info=(perfinfo *)_shmemopen(perfinfoname,NULL,&hperfinfo);
	if(info==NULL)
	{
		_xerror(TEXT("Monitor couldn't open performance info."));
		_xterminate();
	}

	TCHAR sdmemname[256];
	wsprintf(sdmemname,TEXT("sdmem0_%8.8X"),id);

	HANDLE hperfdata;
	perfdata *data=(perfdata *)_shmemopen(sdmemname,NULL,&hperfdata);
	if(data==NULL)
	{
		_xerror(TEXT("Monitor couldn't open performance data."));
		_xterminate();
	}
	bool whichsdmem=false;

	// See if we can make a filename for the output flush
	TCHAR fname[1024];
	size_t end=_xstrlen(info->mainpath);
	if(end!=0)
	{
#ifdef UNICODE
		wsprintf(fname,TEXT("%S"),info->mainpath);
#else
		wsprintf(fname,TEXT("%s"),info->mainpath);
#endif
		size_t origend=end;
		do
		{
			end--;
		}
		while(((int)end)>=0 && fname[end]!=TEXT('.'));
		
		if(fname[end]!=TEXT('.'))
		{
			end=origend-1;
		}
		
		fname[end+1]=TEXT('s');
		fname[end+2]=TEXT('p');
		fname[end+3]=TEXT('d');
		fname[end+4]=TEXT('\0');
	}
	else
	{
		wsprintf(fname,TEXT("profile.spd"));
	}
	
	// Okay, now signal that we're done setting up
	SetEvent(hInitEvent);
	CloseHandle(hInitEvent);
	
	// Wait for a resize or process termination
	HANDLE hWaits[2];
	hWaits[0]=hResize;
	hWaits[1]=hProcess;
	
	bool bDone=false;
	while(!bDone)
	{
		DWORD ret=WaitForMultipleObjects(2,hWaits,FALSE,1000);
		if(ret==WAIT_TIMEOUT)
		{
		}
		else if(ret==WAIT_OBJECT_0)
		{
			// Release shared memory objects
			_shmemclose(data,hperfdata);
			
			// Wait until we've got new data to look at
			WaitForSingleObject(hDoneResizing,INFINITE);

			// Open the new performance data object
			whichsdmem=!whichsdmem;
			TCHAR sdmemname[256];
			wsprintf(sdmemname,TEXT("sdmem%u_%8.8X"),whichsdmem?1:0,id);
			data=(perfdata *)_shmemopen(sdmemname,NULL,&hperfdata);

			// Notify the running process that we've synchronized to the resize
			SetEvent(hSdMonSync);

			// Ensure that everything happened correctly
			if(data==NULL)
			{
				_xerror(TEXT("Monitor couldn't open resized performance data."));
				_xterminate();
			}
		}
		else if(ret==(WAIT_OBJECT_0+1))
		{
            // Process has completed, do the flush
			Flush(fname,info,data);
			bDone=true;
		}
		else
		{
			_xerror(TEXT("Monitoring system was terminated in an unexpected fashion."));
			_xterminate();
		}
	}

	return 0;
}

#endif