struct perfdata
{
	UINT32 nThreadSeqNum;

	ADDRESS nParentAddr_BASED;
	ADDRESS nFromAddr_BASED;
	INT32 nParentModule;

	ADDRESS nToAddr_BASED;
	INT32 nToModule;

	UINT32 nTotalCalls;
	UINT64 nTotalTime;
	UINT64 nFirstStartTime;
	UINT64 nLastEndTime;
	UINT64 nMaxTime;
	UINT64 nMinTime;

	UINT64 nTotalNodeFCTime;
	UINT64 nMinNodeFCTime;
	UINT64 nMaxNodeFCTime;

	UINT64 nTotalEdgeFCTime;
	UINT64 nMinEdgeFCTime;
	UINT64 nMaxEdgeFCTime;
};

struct moduleinfo
{
	ADDRESS base;
	ADDRESS length;
	UINT64 profileentries;
	UINT64 profileexits;
	char svName[256];
	char svExePath[512];
	char svDebugPath[512];
	UINT32 signature;
};

struct perfinfo
{
	char mainpath[512];
	UINT32 targetmode;
	UINT32 perfdatasize;
	UINT32 maxmodules;
	UINT32 modulecount;
	UINT64 entertime;
	UINT64 exittime;
	moduleinfo modules[MAX_MODULES];
};

struct stackinfo
{
	perfdata *perfdata;
	UINT64 nCurrentTotalTime;
	UINT64 nCurrentChildTime;
	UINT32 nRecursiveNodeDepth;
	UINT32 nRecursiveEdgeDepth;
	ADDRESS m_stackpointer;
};

struct threadinfo
{
	int threadlock;
	UINT32 threadid;
	THREAD thread;
	UINT32 seqnum;
	stackinfo *pCallStack;
	UINT32 nCallStackPos;
	UINT64 nLastClock;
	UINT64 nLastStartTime;
	bool bTiming;
};
