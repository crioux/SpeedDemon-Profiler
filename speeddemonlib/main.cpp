#include"sdconfig.h"

#define KEEP_PROFILER_TIME 1

#ifdef _USRDLL
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
// Resources

#include"sdmon.inl"

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
// Storage Structures

#include "perfinfo.inl"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
// Profiling Data

class CSDData
{
public:
	bool m_bCurShMemName;
	int m_globallock;
	FILEMAP m_phmapinfo;
	perfinfo *m_pPerfInfo;
	FILEMAP m_phmapdata;
	perfdata *m_pPerfData;
	UINT32 m_nPerfDataEntryCount;
	UINT32 m_nPerfDataSizeBits;
	threadinfo *m_pThreads;
	UINT32 m_nThreadCount;
	UINT32 m_nThreadSlotCount;
	UINT32 m_nNextThreadSeqNum;
	moduleinfo **m_pLoadedModules;
	UINT32 m_nLoadedModuleCount;
	int *m_pLoadedModuleTable;
	int m_nNextModuleSeqNum;
	int	m_nActivationCount;

	void Init(void);
};

static bool s_bInitialized=false;
static CSDData *s_pData=NULL;
static FILEMAP s_pDataMap=NULL;
static int s_initlock=1;
#define DATA (s_pData)

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
// Forward declarations

static inline threadinfo *EnterThreadLock(void);
static inline void ExitThreadLock(threadinfo *pCurThread);
static inline void EnterExclusiveLock(threadinfo *pCurThread);
static inline void ExitExclusiveLock(threadinfo *pCurThread);

static inline void Initialize(void);
//static inline void Terminate(void);

static inline void DisableCurrentStackProc(threadinfo *pCurThread);
static inline void EnableCurrentStackProc(threadinfo *pCurThread);

static inline void ResizePerfDataHash(threadinfo *pCurThread);
static inline perfdata *FindPerfDataHashSlot(const ADDRESS &ParentAddr_BASED, INT32 nParentModule, 
											 const ADDRESS &ToAddr_BASED, INT32 nToModule, 
											 threadinfo *pCurThread);

void ProfileEnter(ADDRESS CurrentProcAddr, ADDRESS TargetProcAddr, ADDRESS StackPointer);
void ProfileExit(ADDRESS CurrentProcAddr, ADDRESS StackPointer);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
// Operating System Abstractions

#ifdef WINDOWSCODE
#include "windows.inl"
#else
#error "Define platform inlines."
#endif


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
// SpeedDemon Implementation

void CSDData::Init(void)
{
	m_bCurShMemName=false;
	m_globallock=1;
	m_phmapinfo=NULL;
	m_pPerfInfo=NULL;
	m_phmapdata=NULL;
	m_pPerfData=NULL;
	m_nPerfDataEntryCount=NULL;
	m_nPerfDataSizeBits=0;
	m_pThreads=NULL;
	m_nThreadCount=0;
	m_nThreadSlotCount=0;
	m_nNextThreadSeqNum=1;
	m_pLoadedModules=NULL;
	m_nLoadedModuleCount=0;
	m_pLoadedModuleTable=NULL;
	m_nNextModuleSeqNum=1;
	m_nActivationCount=1;
}


static inline threadinfo *EnterThreadLock(void)
{
	_xlock(&DATA->m_globallock);
	UINT32 tid=_xgetcurrentthreadid();	

	// Find the thread matching this id
	UINT32 i;
	for(i=0;i<DATA->m_nThreadSlotCount;i++)
	{
		if(DATA->m_pThreads[i].seqnum!=0 && DATA->m_pThreads[i].threadid==tid)
		{
			_xlock(&(DATA->m_pThreads[i].threadlock));
			_xunlock(&DATA->m_globallock);
			return &(DATA->m_pThreads[i]);	
		}
	}
	
	// Enter exclusive lock so we can modify thread table
	UINT32 firstfree=(UINT32)-1;
	for(i=0;i<DATA->m_nThreadSlotCount;i++)
	{
		if(DATA->m_pThreads[i].seqnum==0)
		{
			if(firstfree==(UINT32)-1)
			{
				firstfree=i;
			}
			continue;
		}
		_xlock(&(DATA->m_pThreads[i].threadlock));
	}
	
	// See if this is too much
	if(DATA->m_nThreadCount>=MAX_CONCURRENT_THREADS)
	{
		_xerror(TEXT("Thread count exceeded profiler capacity."));
	}

	if(firstfree==(UINT32)-1)
	{
		firstfree=DATA->m_nThreadSlotCount;
		DATA->m_nThreadSlotCount++;
	}


	// Create new thread descriptor
	DATA->m_pThreads[firstfree].bTiming=false;
	DATA->m_pThreads[firstfree].nCallStackPos=0;
	DATA->m_pThreads[firstfree].nLastClock=0;
	DATA->m_pThreads[firstfree].nLastStartTime=0;
	DATA->m_pThreads[firstfree].pCallStack=(stackinfo*)_xalloc(CALLSTACK_DEPTH * sizeof(stackinfo),0);
	DATA->m_pThreads[firstfree].seqnum=DATA->m_nNextThreadSeqNum++;
	DATA->m_pThreads[firstfree].thread=_xgetcurrentthread();
	DATA->m_pThreads[firstfree].threadid=_xgetcurrentthreadid();
	DATA->m_pThreads[firstfree].threadlock=1;
	DATA->m_nThreadCount++;
	
	// Clean up any dead threads while we're at it
	for(i=0;i<DATA->m_nThreadSlotCount;i++)
	{
		if(DATA->m_pThreads[i].seqnum==0 || i==firstfree)
		{
			continue;
		}

		if(_xisthreaddead(DATA->m_pThreads[i].thread))
		{
			_xfree(DATA->m_pThreads[i].pCallStack);
			_xreleasethread(DATA->m_pThreads[i].thread);
			
			// Mark the thread as being cleaned up
			DATA->m_pThreads[i].seqnum=0;
			if(i==DATA->m_nThreadSlotCount)
			{
				DATA->m_nThreadSlotCount--;
			}

			DATA->m_nThreadCount--;
		}
	}

	// Release exclusive lock and let everyone continue
	for(i=0;i<DATA->m_nThreadSlotCount;i++)
	{
		if(i==firstfree || DATA->m_pThreads[i].seqnum==0)
		{
			continue;
		}
		_xunlock(&(DATA->m_pThreads[i].threadlock));
	}

	_xlock(&(DATA->m_pThreads[firstfree].threadlock));
	_xunlock(&DATA->m_globallock);
	
	return &(DATA->m_pThreads[firstfree]);
}

static inline void ExitThreadLock(threadinfo *pCurThread)
{
	_xunlock(&(pCurThread->threadlock));
}

static inline void EnterExclusiveLock(threadinfo *pCurThread)
{
	_xunlock(&(pCurThread->threadlock));
	_xlock(&DATA->m_globallock);
	
	// Ensure all threads are locked
	UINT32 i;
	for(i=0;i<DATA->m_nThreadSlotCount;i++)
	{
		if(DATA->m_pThreads[i].seqnum==0)
		{
			continue;
		}
		_xlock(&(DATA->m_pThreads[i].threadlock));
	}
}

static inline void ExitExclusiveLock(threadinfo *pCurThread)
{
	// Release all thread locks
	UINT32 i;
	for(i=0;i<DATA->m_nThreadSlotCount;i++)
	{
		if(DATA->m_pThreads[i].seqnum==0)
		{
			continue;
		}
		_xunlock(&(DATA->m_pThreads[i].threadlock));
	}

	_xlock(&(pCurThread->threadlock));
	_xunlock(&DATA->m_globallock);
	
	return;
}





static inline void Initialize(void)
{
	// Check to see if we're already initialized
	if(s_bInitialized)
	{
		_xerror(TEXT("Internal error: Profiling already initialized."));
		_xterminate();
	}

	// See if we can get data section for this process
	TCHAR dataname[256];
	wsprintf(dataname,TEXT("sd_data_%d"),_xgetcurrentprocessid());
	s_pData=(CSDData *)_shmemopen(dataname,NULL,&s_pDataMap);
	if(s_pData!=NULL)
	{
		// Okay, we're good to go
		s_bInitialized=true;
		return;
	}

	// Create data section since this is our first time
	s_pData=(CSDData *)_shmemcreate(sizeof(CSDData),dataname,&s_pDataMap);
	if(s_pData==NULL)
	{
		_xerror(TEXT("Couldn't create data section."));
		_xterminate();
	}
	s_pData->Init();

	// Allocate small shared memory region for performance info header
	TCHAR perfinfoname[256];
	wsprintf(perfinfoname,TEXT("sd_perfinfo_%d"),_xgetcurrentprocessid());

	if(_shmemopen(perfinfoname,NULL,&DATA->m_phmapinfo)!=NULL)
	{
		_xerror(TEXT("Do not attempt to profile a single process twice."));
		_xterminate();
	}

	DATA->m_pPerfInfo=(perfinfo *)_shmemcreate(sizeof(perfinfo),perfinfoname,&DATA->m_phmapinfo);
	if(DATA->m_pPerfInfo==NULL)
	{
		_xerror(TEXT("Couldn't allocate profiling information header."));
		_xterminate();
	}

	// Allocate main shared memory region for performance data
	DATA->m_nPerfDataSizeBits=HASH_SIZE_BITS;
	UINT32 hashlength=(1<<DATA->m_nPerfDataSizeBits);
	UINT32 totalsize=(sizeof(perfdata)*hashlength);

	TCHAR sdmemname[256];
	wsprintf(sdmemname,TEXT("sdmem0_%8.8X"),_xgetcurrentprocessid());

	DATA->m_pPerfData=(perfdata *)_shmemcreate(totalsize,sdmemname,&DATA->m_phmapdata);
	if(DATA->m_pPerfData==NULL)
	{
		_xerror(TEXT("Couldn't allocate profiling memory."));
		_xterminate();
	}
	DATA->m_bCurShMemName=false;
	
	// Fill in empty slots in table
	UINT32 idx;
	for(idx=0;idx<hashlength;idx++)
	{
		DATA->m_pPerfData[idx].nToModule=-1;
		DATA->m_pPerfData[idx].nToAddr_BASED=~(ADDRESS)0;
	}
	
	// Initialize performance info
	DATA->m_pPerfInfo->targetmode=SD_ARCH;
	DATA->m_pPerfInfo->perfdatasize=hashlength;
	DATA->m_nPerfDataEntryCount=0;
	DATA->m_pPerfInfo->maxmodules=MAX_MODULES;
	DATA->m_pPerfInfo->modulecount=0;
	DATA->m_pPerfInfo->entertime=0;
	DATA->m_pPerfInfo->exittime=0;
	_getmainpath(DATA->m_pPerfInfo->mainpath, sizeof(DATA->m_pPerfInfo->mainpath));

	// Update loaded module list
	DATA->m_pLoadedModules=(moduleinfo **)_xalloc(sizeof(moduleinfo)*MAX_MODULES,0);
	DATA->m_nLoadedModuleCount=0;
	DATA->m_pLoadedModuleTable=NULL;
	_updatemodulelist();
	
	// Initialize threads
	DATA->m_nThreadCount=0;
	DATA->m_nThreadSlotCount=0;
	DATA->m_nNextThreadSeqNum=1;
	DATA->m_pThreads=(threadinfo *)_xalloc(MAX_CONCURRENT_THREADS * sizeof(threadinfo),0);
	if(DATA->m_pThreads==NULL)
	{
		_xerror(TEXT("Couldn't creating allocate thread memory."));
		_xterminate();
	}

	// Start monitor process so we can have it flush data when we finish
	if(!_createmonitor())
	{
		_xerror(TEXT("Couldn't creating profiling monitor."));
		_xterminate();
	}

	// Start clock over
	_xinitclock();

	// Okay, we're good to go
	s_bInitialized=true;
}

/*
static inline void Terminate(void)
{
	// Check to see if we're initialized yet
	if(!s_bInitialized)
	{
		_xerror(TEXT("Abnormal termination without initialization."));
		_xterminate();
	}

	// Reset timers
	_xresetclock();
	
	// Free thread objects
	UINT32 i;
	for(i=0;i<DATA->m_nThreadSlotCount;i++)
	{
		if(DATA->m_pThreads[i].seqnum==0)
		{
			continue;
		}
		_xfree(DATA->m_pThreads[i].pCallStack);
		_xreleasethread(DATA->m_pThreads[i].thread);
	}
	_xfree(DATA->m_pThreads);
	DATA->m_pThreads=NULL;
	DATA->m_nThreadCount=0;
	DATA->m_nThreadSlotCount=0;
	DATA->m_nNextThreadSeqNum=1;

	_xfree(DATA->m_pLoadedModules);
	DATA->m_nLoadedModuleCount=0;
	if(DATA->m_pLoadedModuleTable)
	{
		_xfree(DATA->m_pLoadedModuleTable);
		DATA->m_pLoadedModuleTable=NULL;
	}
	
	// Free memory allocations
	_shmemclose(DATA->m_pPerfData,DATA->m_phmapdata);
	DATA->m_pPerfData=NULL;
	DATA->m_nPerfDataEntryCount=0;
	DATA->m_bCurShMemName=false;
	_shmemclose(DATA->m_pPerfInfo,DATA->m_phmapinfo);
	DATA->m_pPerfInfo=NULL;

	// Okay, we're done	
	s_bInitialized=false;
}
*/

static inline void DisableCurrentStackProc(threadinfo *pCurThread)
{
	// Ensure we're actually timing something currently
	if(!pCurThread->bTiming)
	{
		return;
	}

	// Verify that we have a procedure to disable in the callstack
	if(pCurThread->nCallStackPos==0)
	{
		_xerror(TEXT("Stack underflow while disabling timing."));
		_xterminate();
	}

	stackinfo *pcs=&(pCurThread->pCallStack[pCurThread->nCallStackPos-1]);

	// Add the timing contribution gained since the enable
	// to the calldata for this transition at this stack level
	pcs->nCurrentTotalTime+=(pCurThread->nLastClock-pCurThread->nLastStartTime);
	
	// Note that we're not timing any more
	pCurThread->bTiming=false;

	pcs->perfdata->nLastEndTime=pCurThread->nLastClock;
}

static inline void EnableCurrentStackProc(threadinfo *pCurThread)
{
	// If we're timing something else, we should stop first
	if(pCurThread->bTiming)
	{
		DisableCurrentStackProc(pCurThread);
	}

	// Start timing
	pCurThread->nLastStartTime=pCurThread->nLastClock;
	pCurThread->bTiming=true;

	stackinfo *pcs=&(pCurThread->pCallStack[pCurThread->nCallStackPos-1]);

	if(pcs->perfdata->nTotalCalls==1)
	{
		pcs->perfdata->nFirstStartTime=pCurThread->nLastStartTime;
	}
}

static inline void ResizePerfDataHash(threadinfo *pCurThread)
{
	EnterExclusiveLock(pCurThread);
				
	_signalresize();
	
	// Get next biggest size for the hash table
	UINT32 nextsizebits=DATA->m_nPerfDataSizeBits+1;
	if(nextsizebits>=32)
	{
		_xerror(TEXT("Overflow while resizing hashtable. This totally shouldn't happen."));
		_xterminate();
	}
	UINT32 nexthashlength=1<<nextsizebits;
	UINT32 nexttotalsize=nexthashlength*sizeof(perfdata);

	// Swtich over to the other name
	DATA->m_bCurShMemName=!DATA->m_bCurShMemName;

	// Get the name
	TCHAR sdmemname[256];
	wsprintf(sdmemname,TEXT("sdmem%u_%8.8X"),(DATA->m_bCurShMemName?1:0),_xgetcurrentprocessid());
	
	// Create the storage space
	HANDLE nextphmapdata=NULL;
	perfdata *pNextPerfData=(perfdata *)_shmemcreate(nexttotalsize,sdmemname,&nextphmapdata);
	
	if(pNextPerfData==NULL || nextphmapdata==NULL)
	{
		_xerror(TEXT("Couldn't allocate resized profiling memory."));
		_xterminate();
	}

	// Fill in empty slots in new table
	UINT32 nextidx,nextcount=nexthashlength;
	for(nextidx=0;nextidx<nextcount;nextidx++)
	{
		pNextPerfData[nextidx].nToModule=-1;
		pNextPerfData[nextidx].nToAddr_BASED=~(ADDRESS)0;
	}
	
	// Rehash all data into new table
	UINT32 idx,count=DATA->m_pPerfInfo->perfdatasize;
	for(idx=0;idx<count;idx++)
	{
		perfdata *curslot=&(DATA->m_pPerfData[idx]);
		if(curslot->nToModule==-1 && curslot->nToAddr_BASED==~(ADDRESS)0)
		{
			continue;
		}
		
		UINT32 nexthash=_xhash( ((UINT32)curslot->nParentAddr_BASED) ^ curslot->nParentModule ^ 
								((UINT32)curslot->nToAddr_BASED) ^ curslot->nToModule ^
								curslot->nThreadSeqNum );
		UINT32 nextmask=(nexthashlength-1);
		UINT32 nextindex=(nexthash & nextmask);
		
		while(pNextPerfData[nextindex].nToModule!=-1 && pNextPerfData[nextindex].nToAddr_BASED!=~(ADDRESS)0)
		{
			nextindex=((nextindex+1) & nextmask);
		}
        
		pNextPerfData[nextindex]=*curslot;
	}

	// Rewrite all thread callstacks in terms of new pointers
	UINT32 i;
	for(i=0;i<DATA->m_nThreadSlotCount;i++)
	{
		if(DATA->m_pThreads[i].seqnum==0)
		{
			continue;
		}
		UINT32 pos;
		for(pos=0;pos<DATA->m_pThreads[i].nCallStackPos;pos++)
		{
			perfdata *oldframe=DATA->m_pThreads[i].pCallStack[pos].perfdata;
			
			UINT32 nexthash=_xhash( ((UINT32)oldframe->nParentAddr_BASED) ^ oldframe->nParentModule ^
									((UINT32)oldframe->nToAddr_BASED) ^ oldframe->nToModule ^
									oldframe->nThreadSeqNum );
			UINT32 nextmask=(nexthashlength-1);
			UINT32 nextindex=(nexthash & nextmask);		
			while(!(pNextPerfData[nextindex].nParentAddr_BASED==oldframe->nParentAddr_BASED &&
					pNextPerfData[nextindex].nParentModule==oldframe->nParentModule &&
					pNextPerfData[nextindex].nToAddr_BASED==oldframe->nToAddr_BASED &&
				    pNextPerfData[nextindex].nToModule==oldframe->nToModule &&
				    pNextPerfData[nextindex].nThreadSeqNum==oldframe->nThreadSeqNum ))
			{
				nextindex=((nextindex+1) & nextmask);
			}
        
			DATA->m_pThreads[i].pCallStack[pos].perfdata=&(pNextPerfData[nextindex]);
		}
	}
	
	// Release the old data
	_shmemclose(DATA->m_pPerfData,DATA->m_phmapdata);
	
	// Save new hash table information
	DATA->m_pPerfData=pNextPerfData;
	DATA->m_phmapdata=nextphmapdata;
	DATA->m_nPerfDataSizeBits=nextsizebits;
	DATA->m_pPerfInfo->perfdatasize=nexthashlength;
	
	_signaldoneresizing();

	ExitExclusiveLock(pCurThread);

	// Get the time
	_xupdateclock(&(pCurThread->nLastClock));
}

static inline perfdata *FindPerfDataHashSlot(const ADDRESS &ParentAddr_BASED, INT32 nParentModule, 
											 const ADDRESS &ToAddr_BASED, INT32 nToModule, 
											 threadinfo *pCurThread)
{
reinsert:;
	UINT32 hash=_xhash( ((UINT32)ParentAddr_BASED) ^ nParentModule ^
		                ((UINT32)ToAddr_BASED) ^ nToModule ^
						pCurThread->seqnum );
	UINT32 mask=(DATA->m_pPerfInfo->perfdatasize-1);
	UINT32 index=(hash & mask);
	while(1)
	{
		if(DATA->m_pPerfData[index].nToModule==-1 && DATA->m_pPerfData[index].nToAddr_BASED==~(ADDRESS)0)
		{
			// See if it's time to resize	
			UINT32 percentfull=(DATA->m_nPerfDataEntryCount)>>(DATA->m_nPerfDataSizeBits-8);
			if(percentfull>=HASH_FULL_PERCENT256)
			{
				ResizePerfDataHash(pCurThread);
				goto reinsert;
			}

			// Add to our entry count
			DATA->m_nPerfDataEntryCount++;
			return &(DATA->m_pPerfData[index]);
		}
		else if( (DATA->m_pPerfData[index].nParentAddr_BASED==(size_t)ParentAddr_BASED) &&
				 (DATA->m_pPerfData[index].nParentModule==nParentModule) &&
				 (DATA->m_pPerfData[index].nToAddr_BASED==ToAddr_BASED) &&
				 (DATA->m_pPerfData[index].nToModule==nToModule) &&
				 (DATA->m_pPerfData[index].nThreadSeqNum==pCurThread->seqnum) )
		{
			// Old hashtable entry
			return &(DATA->m_pPerfData[index]);
		}
		// Other hashtable entry
		index=((index+1) & mask);
	}
}


static inline void ExitStackFrame(threadinfo *pCurThread, stackinfo *pcs)
{
	// Get the total time spent by this procedure
	// and the total time spent by all of its children
	// Added together, these are the function+descendant time.
	perfdata *ppd=pcs->perfdata;

	UINT64 tf=pcs->nCurrentTotalTime;
	UINT64 tc=tc=pcs->nCurrentChildTime;
	UINT64 tfc=tf+tc;
	
	// Add function time to everything in the callstack to calculate child time
	stackinfo *pcshead=pCurThread->pCallStack;
	for(stackinfo *pcscur=pcs-1;pcscur>=pcshead;pcscur--)
	{
		pcscur->nCurrentChildTime+=tf;
	}

	// Store total, max, and min time
	ppd->nTotalTime+=tf;
	if(tf>ppd->nMaxTime)
	{
		ppd->nMaxTime=tf;
	}
	if(tf<ppd->nMinTime)
	{
		ppd->nMinTime=tf;
	}	

	// Store total, max, and min function+children time
	// Be careful not to re-count child time more than once!
	if(pcs->nRecursiveNodeDepth==0)
	{	
		ppd->nTotalNodeFCTime+=tfc;
		if(tfc>ppd->nMaxNodeFCTime)
		{
			ppd->nMaxNodeFCTime=tfc;
		}
		if(tfc<ppd->nMinNodeFCTime)
		{
			ppd->nMinNodeFCTime=tfc;
		}	
	}
	if(pcs->nRecursiveEdgeDepth==0)
	{	
		ppd->nTotalEdgeFCTime+=tfc;
		if(tfc>ppd->nMaxEdgeFCTime)
		{
			ppd->nMaxEdgeFCTime=tfc;
		}
		if(tfc<ppd->nMinEdgeFCTime)
		{
			ppd->nMinEdgeFCTime=tfc;
		}	
	}

	// Note that something in this module is exiting
	int mod=pcs->perfdata->nToModule;
	if(mod!=-1)
	{
		DATA->m_pPerfInfo->modules[mod].profileexits++;
	}
}


void ProfileEnter(ADDRESS CurrentProcAddr, ADDRESS TargetProcAddr, ADDRESS StackPointer)
{
	//DebugBreak();

	if(!s_bInitialized)
	{
		_xlock(&s_initlock);
		if(!s_bInitialized)
		{
			Initialize();	
		}
		_xunlock(&s_initlock);
	}

#ifdef KEEP_PROFILER_TIME
	UINT64 profclock;
	_xupdateclock(&profclock);
#endif

	threadinfo *pCurThread=EnterThreadLock();
	stackinfo *pCurThreadCallStack=pCurThread->pCallStack;

	// Get the time
	_xupdateclock(&(pCurThread->nLastClock));

	// Get the target
	ADDRESS ToAddr_BASED;
	INT32 ToMod;
	
	int lmodidx=_findloadedmodule(TargetProcAddr);
	if(lmodidx==-1)
	{
		// If we can't find a module for ourselves, then we can't unbase the address
		ToAddr_BASED=TargetProcAddr;
		ToMod=-1;
	}
	else
	{
		// Unbase the target address so we can deal with libraries being rebased
		ToMod=(int)(DATA->m_pLoadedModules[lmodidx]-&(DATA->m_pPerfInfo->modules[0]));
		ToAddr_BASED=TargetProcAddr-(DATA->m_pLoadedModules[lmodidx]->base);
	}
	
	// Note the address of the parent procedure
	ADDRESS ParentAddr_BASED;
	ADDRESS FromAddr_BASED;
	INT32 ParentMod;
	if(pCurThread->nCallStackPos==0)
	{
		// The 'root' of our execution is address zero, since we won't ever
		// be returning to it and nobody actually 'called' it.
		ParentAddr_BASED=~(ADDRESS)0;
		FromAddr_BASED=~(ADDRESS)0;
		ParentMod=-1;
	}
	else
	{
		// Stop timing the parent procedure
		DisableCurrentStackProc(pCurThread);

		perfdata *pParentPerf=pCurThreadCallStack[pCurThread->nCallStackPos-1].perfdata;
		ParentMod=pParentPerf->nToModule;
		ParentAddr_BASED=pParentPerf->nToAddr_BASED;
		if(CurrentProcAddr!=0 && ParentMod!=-1)
		{
			FromAddr_BASED=CurrentProcAddr-DATA->m_pPerfInfo->modules[ParentMod].base;
		}
		else
		{
			FromAddr_BASED=~(ADDRESS)0;
		}
	}

	// Exit all stack frames up to the current stack pointer
	if(pCurThread->nCallStackPos>0)
	{
		stackinfo *pcs=&(pCurThread->pCallStack[pCurThread->nCallStackPos-1]);
		while(
#if STACKGROWSDOWN
			pcs->m_stackpointer<=StackPointer
#else
			pcs->m_stackpointer>=StackPointer
#endif
			)
		{
			// Exit this stack frame
			ExitStackFrame(pCurThread,pcs);		

			// Pop the call stack
			pCurThread->nCallStackPos--;
			if(pCurThread->nCallStackPos<=0)
			{
				break;
			}
			pcs=&(pCurThread->pCallStack[pCurThread->nCallStackPos-1]);
		}
	}



	// Get our slot in the perfinfo hashset
	perfdata *slot=FindPerfDataHashSlot(ParentAddr_BASED,ParentMod,ToAddr_BASED,ToMod,pCurThread);
	if(slot->nToModule==-1 && slot->nToAddr_BASED==~(ADDRESS)0)
	{
		slot->nToModule=ToMod;
		slot->nToAddr_BASED=ToAddr_BASED;
		slot->nParentModule=ParentMod;
		slot->nParentAddr_BASED=ParentAddr_BASED;
		slot->nFromAddr_BASED=FromAddr_BASED;
		slot->nTotalTime=0;
		slot->nTotalCalls=1;
		slot->nThreadSeqNum=pCurThread->seqnum;
		slot->nFirstStartTime=0;
		slot->nLastEndTime=0;
		slot->nMaxTime=0;
		slot->nMinTime=~((UINT64)0);
		slot->nTotalNodeFCTime=0;
		slot->nMinNodeFCTime=~((UINT64)0);
		slot->nMaxNodeFCTime=0;
		slot->nTotalEdgeFCTime=0;
		slot->nMinEdgeFCTime=~((UINT64)0);
		slot->nMaxEdgeFCTime=0;
	}
	else
	{
        slot->nTotalCalls++;
	}

	// Take this index and push it to the call stack
	if(pCurThread->nCallStackPos>=CALLSTACK_DEPTH)
	{
		_xerror(TEXT("Exceeded profiler stack depth."));
		_xterminate();
	}
	
	stackinfo *pcs=&(pCurThreadCallStack[pCurThread->nCallStackPos]);
	pcs->perfdata=slot;
	pcs->nCurrentTotalTime=0;
	pcs->nCurrentChildTime=0;
	pcs->nRecursiveNodeDepth=0;
	pcs->nRecursiveEdgeDepth=0;
	pcs->m_stackpointer=StackPointer;
	
	// See how recursive we are (cache or hash this somehow?)
	stackinfo *pcshead=pCurThread->pCallStack;
	for(stackinfo *pcscur=pcs-1;pcscur>=pcshead;pcscur--)
	{
		if(pcscur->perfdata->nToAddr_BASED==slot->nToAddr_BASED && pcscur->perfdata->nToModule==slot->nToModule)
		{
			pcs->nRecursiveNodeDepth++;
		}
		if(pcscur->perfdata==slot)
		{
			pcs->nRecursiveEdgeDepth++;
		}
	}

	pCurThread->nCallStackPos++;

	// Note that something in this module is entering
	if(ToMod!=-1)
	{
		DATA->m_pPerfInfo->modules[ToMod].profileentries++;			
	}
	
	// Now, start timing the current procedure
	EnableCurrentStackProc(pCurThread);

	ExitThreadLock(pCurThread);

#ifdef KEEP_PROFILER_TIME
	UINT64 profclock2;
	_xupdateclock(&profclock2);
	_xlock(&DATA->m_globallock);
	DATA->m_pPerfInfo->entertime+=(profclock2-profclock);
	_xunlock(&DATA->m_globallock);
#endif

}

void ProfileExit(ADDRESS CurrentProcAddr, ADDRESS StackPointer)
{
#ifdef KEEP_PROFILER_TIME
	UINT64 profclock;
	_xupdateclock(&profclock);
#endif

	threadinfo *pCurThread=EnterThreadLock();

	// Get the time
	_xupdateclock(&(pCurThread->nLastClock));
	
	// Stop timing the current procedure
	DisableCurrentStackProc(pCurThread);

	// Exit all stack frames up to the current stack pointer
	if(pCurThread->nCallStackPos>0)
	{
		stackinfo *pcs=&(pCurThread->pCallStack[pCurThread->nCallStackPos-1]);
		while(
#if STACKGROWSDOWN
			pcs->m_stackpointer<=StackPointer
#else
			pcs->m_stackpointer>=StackPointer
#endif
			)
		{
			// Exit this stack frame
			ExitStackFrame(pCurThread,pcs);		
			
			// Pop the call stack
			pCurThread->nCallStackPos--;
			if(pCurThread->nCallStackPos<=0)
			{
				break;
			}
			pcs=&(pCurThread->pCallStack[pCurThread->nCallStackPos-1]);
		}
	}

	if(pCurThread->nCallStackPos<=0)
	{
		ExitThreadLock(pCurThread);

#ifdef KEEP_PROFILER_TIME
		UINT64 profclock2;
		_xupdateclock(&profclock2);
		_xlock(&DATA->m_globallock);
		DATA->m_pPerfInfo->exittime+=(profclock2-profclock);
		_xunlock(&DATA->m_globallock);
#endif

		return;
	}


	// Start timing the current procedure
	EnableCurrentStackProc(pCurThread);

	ExitThreadLock(pCurThread);

#ifdef KEEP_PROFILER_TIME
	UINT64 profclock2;
	_xupdateclock(&profclock2);
	_xlock(&DATA->m_globallock);
	DATA->m_pPerfInfo->exittime+=(profclock2-profclock);
	_xunlock(&DATA->m_globallock);
#endif

}



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
// Profiler Entrypoints		

extern "C" {

#ifdef HAS_FASTCAP

// This code is for msvc/evc4 /fastcap

DLLEXPORT void _stdcall _CAP_Start_Profiling(void *CurrentProcAddr, void *TargetProcAddr)
{
	register ADDRESS sp=_xgetstackpointer();
	ProfileEnter((ADDRESS)CurrentProcAddr,(ADDRESS)TargetProcAddr,sp);		    
}

DLLEXPORT void _stdcall _CAP_End_Profiling(void *CurrentProcAddr)
{
	register ADDRESS sp=_xgetstackpointer();
	ProfileExit((ADDRESS)CurrentProcAddr,sp);
}

#endif

#ifdef HAS_CALLCAP

// This code is for msvc/evc4 /callcap

DLLEXPORT void _CAP_Enter_Function(void *ProcAddr) 
{
	register ADDRESS sp=_xgetstackpointer();
	ProfileEnter(0,(ADDRESS)ProcAddr,sp);
}

DLLEXPORT void _CAP_Exit_Function(void *ProcAddr) 
{
	register ADDRESS sp=_xgetstackpointer();
	ProfileExit(0,sp);
}

#endif

#if SD_ARCH==SD_X86_WIN32 && defined(HAS_PENTER_PEXIT)

// This code is for msvc/x86 /Gh /GH

DLLEXPORT void __declspec(naked) _penter( void ) 
{
    _asm {
		pushad
		pushfd

		// Get procedure address
		mov ebx,[esp+36]

		// Get stack pointer (procedure esp + 36)
		mov eax,esp
		push eax

		push ebx
		
		xor eax,eax
		push eax
		
		call ProfileEnter
		add esp,12

		popfd
 		popad
		ret
    }
}

DLLEXPORT void __declspec(naked) _pexit( void ) 
{
    _asm {
		pushad
		pushfd

		mov eax,esp // (procedure esp + 36)
		push eax

		xor eax,eax
		push eax

		call ProfileExit
		add esp,8
		
		popfd
 		popad
		ret
	}
}

#endif



#if defined(_USRDLL) && defined(WINDOWSCODE)
BOOL WINAPI DllMain(
  HANDLE hinstDLL, 
  DWORD dwReason, 
  LPVOID lpvReserved
)
{
	return TRUE;
}

#endif

}