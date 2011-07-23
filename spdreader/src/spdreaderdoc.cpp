#include"spdreader_pch.h"
#include"spdreader.h"
#include"spdreaderdoc.h"
#include"spdreaderview.h"
#if defined(_WIN32)
#include"diadebug.h"
#endif
#include "boost/bind.hpp"

CTimingStats::CTimingStats()
{
	Count=0;
	MinimumFunctionTime=0;
	AverageFunctionTime=0;
	MaximumFunctionTime=0;
	MinimumFunctionAndChildrenTime=0;
	AverageFunctionAndChildrenTime=0;
	MaximumFunctionAndChildrenTime=0;
	TotalFunctionTime=0;
	TotalFunctionAndChildrenTime=0;
}

void CTimingStats::AddPerfData(const CPerfData &pd, bool bEdge)
{
	if(Count==0)
	{
		Count=pd.nTotalCalls;
		MinimumFunctionTime=pd.nMinTime;
		AverageFunctionTime=pd.nTotalTime;
		MaximumFunctionTime=pd.nMaxTime;
		MinimumFunctionAndChildrenTime=bEdge ? pd.nMinEdgeFCTime: pd.nMinNodeFCTime;
		AverageFunctionAndChildrenTime=bEdge ? pd.nTotalEdgeFCTime: pd.nTotalNodeFCTime;
		MaximumFunctionAndChildrenTime=bEdge ? pd.nMaxEdgeFCTime: pd.nMaxNodeFCTime;
		TotalFunctionTime=pd.nTotalTime;
		TotalFunctionAndChildrenTime=bEdge ? pd.nTotalEdgeFCTime: pd.nTotalNodeFCTime;
	}
	else
	{
		Count+=pd.nTotalCalls;
		if(pd.nMinTime<MinimumFunctionTime)
		{
			MinimumFunctionTime=pd.nMinTime;
		}
		AverageFunctionTime+=pd.nTotalTime;
		if(pd.nMaxTime>MaximumFunctionTime)
		{
			MaximumFunctionTime=pd.nMaxTime;
		}
		wxUint64 minfctime=bEdge ? pd.nMinEdgeFCTime: pd.nMinNodeFCTime;
		if(minfctime<MinimumFunctionAndChildrenTime)
		{
			MinimumFunctionAndChildrenTime=minfctime;
		}
		wxUint64 totalfctime=bEdge ? pd.nTotalEdgeFCTime: pd.nTotalNodeFCTime;	
		AverageFunctionAndChildrenTime+=totalfctime;
		wxUint64 maxfctime=bEdge ? pd.nMaxEdgeFCTime: pd.nMaxNodeFCTime;
		if(maxfctime>MaximumFunctionAndChildrenTime)
		{
			MaximumFunctionAndChildrenTime=maxfctime;
		}
		TotalFunctionTime+=pd.nTotalTime;
		TotalFunctionAndChildrenTime+=totalfctime;
	}
}

void CTimingStats::AddFunctionStats(const CTimingStats &fs)
{
	if(Count==0)
	{
		Count=fs.Count;
		MinimumFunctionTime=fs.MinimumFunctionTime;
		AverageFunctionTime=fs.TotalFunctionTime;
		MaximumFunctionTime=fs.MaximumFunctionTime;
		MinimumFunctionAndChildrenTime=fs.MinimumFunctionAndChildrenTime;
		AverageFunctionAndChildrenTime=fs.TotalFunctionAndChildrenTime;
		MaximumFunctionAndChildrenTime=fs.MaximumFunctionAndChildrenTime;
		TotalFunctionTime=fs.TotalFunctionTime;
		TotalFunctionAndChildrenTime=fs.TotalFunctionAndChildrenTime;
	}
	else
	{
		Count+=fs.Count;
		if(fs.MinimumFunctionTime<MinimumFunctionTime)
		{
			MinimumFunctionTime=fs.MinimumFunctionTime;
		}
		AverageFunctionTime+=fs.TotalFunctionTime;
		if(fs.MaximumFunctionTime>MaximumFunctionTime)
		{
			MaximumFunctionTime=fs.MaximumFunctionTime;
		}
		if(fs.MinimumFunctionAndChildrenTime<MinimumFunctionAndChildrenTime)
		{
			MinimumFunctionAndChildrenTime=fs.MinimumFunctionAndChildrenTime;
		}
		AverageFunctionAndChildrenTime+=fs.TotalFunctionAndChildrenTime;
		if(fs.MaximumFunctionAndChildrenTime>MaximumFunctionAndChildrenTime)
		{
			MaximumFunctionAndChildrenTime=fs.MaximumFunctionAndChildrenTime;
		}
		TotalFunctionTime+=fs.TotalFunctionTime;
		TotalFunctionAndChildrenTime+=fs.TotalFunctionAndChildrenTime;
	}
}


void CTimingStats::CalculateAverages(void)
{
	if(Count!=0)
	{
		AverageFunctionTime/=Count;
		AverageFunctionAndChildrenTime/=Count;
	}
	else
	{
		AverageFunctionTime=0;
		AverageFunctionAndChildrenTime=0;
	}
}


IMPLEMENT_DYNAMIC_CLASS(CSPDReaderDoc,wxDocument)

CSPDReaderDoc::CSPDReaderDoc()
{
	m_bValid=true;
	m_nSortField=0;
	m_bSortDirection=false;	
}

CSPDReaderDoc::~CSPDReaderDoc()
{

}

bool CSPDReaderDoc::LoadDebugSymbols(void)
{
	// Set up keys for address hash map
	for(wxUint32 i=0;i<m_arrPerfData.size();i++)
	{
		// Skip empty hash slots
		if(!m_arrPerfData[i].nToAddr.IsValid())
		{
			continue;
		}

		BasedAddress addr=m_arrPerfData[i].nFromAddr;
		if(addr.IsValid())
		{
			m_addressMap[addr]=-1;
		}
		addr=m_arrPerfData[i].nToAddr;
		if(addr.IsValid())
		{
			m_addressMap[addr]=-1;
		}
		addr=m_arrPerfData[i].nParentAddr;
		if(addr.IsValid())
		{
			m_addressMap[addr]=-1;
		}
	}	

	// Go through all modules we've profiled and load their functions
	// and put their addresses the in the map as well
	std::list<CFunctionDescription> llFuncs;
	for(wxUint32 x=0;x<m_PerfInfo.modulecount;x++)
	{
		if(m_PerfInfo.modules[x].nProfileEntries==0 &&
		   m_PerfInfo.modules[x].nProfileExits==0)
		{
			continue;
		}

		switch(m_PerfInfo.targetmode)
		{
		case SD_X86_WIN32:
		case SD_X86_WCE:
		case SD_PXA270_WCE:
		case SD_PXA255_WCE:
		case SD_X64_WIN64:
			{
	
#if defined(WIN32)
				if(m_PerfInfo.modules[x].strDebugPath.IsEmpty())
				{
					break;
				}
				
				if(!LoadDIASymbols(m_PerfInfo.modules[x].strDebugPath,
								   x,
					               llFuncs,
								   m_addressMap
								   ))
				{
					wxMessageBox(wxString::Format(
									wxT("Unable to load symbols at '%s'\n")
									wxT("The file may be corrupt.\n")
									wxT("Attempting to continue loading other modules."), (const TCHAR *)m_PerfInfo.modules[x].strDebugPath),
								wxT("SPDReader Error"),wxOK|wxICON_INFORMATION);
					break;
				}

#else
				wxMessageBox(wxT("Viewing profiling data from a Windows or Windows CE platform\n")
					         wxT("requires the Windows version of SPDReader.\n"),
							 wxT("SPDReader Platform Limitation"),wxOK|wxICON_INFORMATION);
				return false;
#endif
			}
			break;
		}
	}

	// Any addresses left that point to no function need stubs
	int stubnum=(int)llFuncs.size();
	for(AddressMapHash::iterator iter=m_addressMap.begin();iter!=m_addressMap.end();iter++)
	{
		if(iter->second!=-1)
		{
			continue;
		}
		BasedAddress addr=iter->first;

		// Find which module this belongs to
		wxString modname=wxT("UNKNOWN");
		
		if(addr.IsValid())
		{
			modname=m_PerfInfo.modules[addr.nModule].strName;
		}

		CFunctionDescription func;
		if(m_PerfInfo.addrbits==32)
		{
			func.strBaseName=wxString::Format(wxT("%s+0x%8.8X"),(const TCHAR *)(modname),(wxUint32)addr.nAddr);
		}
		else if(m_PerfInfo.addrbits==64)
		{
#if defined(_WIN32) || defined(_WIN32_WCE)
			func.strBaseName=wxString::Format(wxT("%s+0x%16.16I64X"),(const TCHAR *)(modname),addr.nAddr);
#else
			func.strBaseName=wxString::Format(wxT("%s+0x%16.16llX"),(const TCHAR *)(modname),addr.nAddr);
#endif
		}
		else
		{
			DebugBreak();
		}
		func.strFullName=func.strBaseName;
		func.strMangledName=func.strBaseName;
		func.nAddress=addr;
		
		llFuncs.push_back(func);

		iter->second=stubnum;
		stubnum++;
	}


	// Convert list of functions into member array of functions
	wxUint32 f=0;
	m_arrFunctionData.resize(llFuncs.size());
	for(std::list<CFunctionDescription>::iterator iter=llFuncs.begin();iter!=llFuncs.end();iter++)
	{
		m_arrFunctionData[f].Desc=*iter;
		f++;
	}

	return true;
}

bool CSPDReaderDoc::Collate(void)
{
	int nMaxThreadSeqNum=-1;

	for(wxUint32 i=0;i<m_arrPerfData.size();i++)
	{
		// Skip empty hash slots
		if(!m_arrPerfData[i].nToAddr.IsValid())
		{
			continue;
		}

		// Get a pointer to the performance data record we're currently working with
		CPerfData *ppd=&(m_arrPerfData[i]);
		assert(ppd->nTotalCalls>0);

		// Get the called/child function
		AddressMapHash::iterator iter=m_addressMap.find(ppd->nToAddr);
		if(iter==m_addressMap.end())
		{
			continue;
		}
		if(iter->second==-1)
		{
			continue;
		}
		CFunctionData *pfd=&(m_arrFunctionData[iter->second]);

		// Get the thread timings for this particular thread sequence number
		if(pfd->ThreadData.find(ppd->nThreadSeqNum)==pfd->ThreadData.end())
		{
			pfd->ThreadData[ppd->nThreadSeqNum]=CFunctionDataPerThread();
		}
		CFunctionDataPerThread *pft=&(pfd->ThreadData[ppd->nThreadSeqNum]);
		
		// Get the caller/parent function if it exists
		CFunctionData *pfdparent=NULL;
		AddressMapHash::iterator iterp=m_addressMap.find(m_arrPerfData[i].nParentAddr);
		if(iterp!=m_addressMap.end() && iterp->second!=-1)
		{
			pfdparent=&(m_arrFunctionData[iterp->second]);
		}
		
		// Get the caller/parent per-thread data
		if(pfdparent)
		{
			FunctionDataPerThreadHash::iterator iterppt=pfdparent->ThreadData.find(ppd->nThreadSeqNum);
			CFunctionDataPerThread *pftparent;
			bool bParentFirstTime;
			if(iterppt==pfdparent->ThreadData.end())
			{
				pfdparent->ThreadData[ppd->nThreadSeqNum]=CFunctionDataPerThread();
				pftparent=&(pfdparent->ThreadData[ppd->nThreadSeqNum]);
				bParentFirstTime=true;
			}
			else
			{
				pftparent=&(iterppt->second);
				bParentFirstTime=false;
			}		

			// Build the parent-child relationship for this thread
			if(pft->Parents.find(iterp->second)==pft->Parents.end())
			{
				pft->Parents[iterp->second]=CTimingStats();
			}
			pft->Parents[iterp->second].AddPerfData(*ppd,true);

			if(pftparent->Children.find(iter->second)==pftparent->Children.end())
			{
				pftparent->Children[iter->second]=CTimingStats();
			}
			pftparent->Children[iter->second].AddPerfData(*ppd,true);
		}

		// Add the function counts together for this thread sequence number
		pft->Stats.AddPerfData(*ppd,false);
		
		// Store thread sequence number count
		if(((int)ppd->nThreadSeqNum)>nMaxThreadSeqNum)
		{
			nMaxThreadSeqNum=((int)ppd->nThreadSeqNum);
		}
	}
	
	// Go through and calculate averages for all threads for all functions
	for(std::vector<CFunctionData>::iterator iterf=m_arrFunctionData.begin();iterf!=m_arrFunctionData.end();iterf++)
	{
		for(FunctionDataPerThreadHash::iterator itertt=iterf->ThreadData.begin();itertt!=iterf->ThreadData.end();itertt++)
		{
			itertt->second.Stats.CalculateAverages();

			for(stdext::hash_map<wxUint32,CTimingStats>::iterator iterp=itertt->second.Parents.begin();iterp!=itertt->second.Parents.end();iterp++)
			{
				iterp->second.CalculateAverages();
			}
			for(stdext::hash_map<wxUint32,CTimingStats>::iterator iterc=itertt->second.Children.begin();iterc!=itertt->second.Children.end();iterc++)
			{
				iterc->second.CalculateAverages();
			}
		}
	}

	// Make thread statistics table
	m_arrThreadData.resize(nMaxThreadSeqNum+1);

	// Fill in thread statistics table
	for(wxUint32 i=0;i<m_arrPerfData.size();i++)
	{
		// Skip empty hash slots
		if(!m_arrPerfData[i].nToAddr.IsValid())
		{
			continue;
		}

		// Get a pointer to the performance data record we're currently working with
		CPerfData *ppd=&(m_arrPerfData[i]);
		assert(ppd->nTotalCalls>0);

		m_arrThreadData[ppd->nThreadSeqNum].nThreadSeqNum=ppd->nThreadSeqNum;
		
		if(ppd->nFirstStartTime<m_arrThreadData[ppd->nThreadSeqNum].nThreadStartTime)
		{
			m_arrThreadData[ppd->nThreadSeqNum].nThreadStartTime=ppd->nFirstStartTime;
		}

		if(ppd->nLastEndTime>m_arrThreadData[ppd->nThreadSeqNum].nThreadEndTime)
		{
			m_arrThreadData[ppd->nThreadSeqNum].nThreadEndTime=ppd->nLastEndTime;
		}

		m_arrThreadData[ppd->nThreadSeqNum].nTotalRecordedThreadTime+=ppd->nTotalTime;

		if(!ppd->nParentAddr.IsValid())
		{
			// Get the called/child function
			AddressMapHash::iterator iter=m_addressMap.find(m_arrPerfData[i].nToAddr);
			if(iter!=m_addressMap.end() && iter->second!=-1)
			{
				m_arrThreadData[ppd->nThreadSeqNum].TopLevelFunctions.insert(iter->second);
			}
		}
	}

	// Get total wall clock time
	wxUint64 starttime,endtime;
	for(size_t i=1;i<m_arrThreadData.size();i++)
	{
		if(i==1)
		{
			starttime=m_arrThreadData[i].nThreadStartTime;
			endtime=m_arrThreadData[i].nThreadEndTime;
		}
		else
		{
			if(m_arrThreadData[i].nThreadStartTime<starttime)
			{
				starttime=m_arrThreadData[i].nThreadStartTime;
			}
			if(m_arrThreadData[i].nThreadEndTime>endtime)
			{
				endtime=m_arrThreadData[i].nThreadEndTime;
			}
		}
	}

	m_PerfInfo.totalwallclocktime=(endtime-starttime);
	
	return true;
}

   

wxInputStream& CSPDReaderDoc::ImportData(wxInputStream& instream)
{
	wxInputStream &stream=instream;

	// Load targetmode
	unsigned char targetmode[4];
	if(!stream.Read(targetmode,4).IsOk())
	{
		m_bValid=false;
		return stream;
	}

	// Figure out endianness
	bool bBigEndian;
	if(targetmode[0]==SD_X86_WIN32)
	{
		m_PerfInfo.targetmode=SD_X86_WIN32;
		bBigEndian=false;
		m_PerfInfo.addrbits=32;
	}
	else if(targetmode[0]==SD_X86_WCE)
	{
		m_PerfInfo.targetmode=SD_X86_WCE;
		bBigEndian=false;
		m_PerfInfo.addrbits=32;
	}
	else if(targetmode[0]==SD_PXA270_WCE)
	{
		m_PerfInfo.targetmode=SD_PXA270_WCE;
		bBigEndian=false;
		m_PerfInfo.addrbits=32;
	}
	else if(targetmode[0]==SD_PXA255_WCE)
	{
		m_PerfInfo.targetmode=SD_PXA255_WCE;
		bBigEndian=false;
		m_PerfInfo.addrbits=32;
	}
	else if(targetmode[0]==SD_X64_WIN64)
	{
		m_PerfInfo.targetmode=SD_X64_WIN64;
		bBigEndian=false;
		m_PerfInfo.addrbits=64;
	}
	else
	{
		m_bValid=false;
		return stream;
	}

	// Load mainpath
	char mainpath[512];
	if(!stream.Read(mainpath,512).IsOk())
	{
		m_bValid=false;
		return stream;
	}
	m_PerfInfo.mainpath=wxString(mainpath,wxConvLibc);

	// Load perfdatasize
	wxUint32 perfdatasize;
	if(!stream.Read(&perfdatasize,4).IsOk())
	{
		m_bValid=false;
		return stream;
	}
	if(bBigEndian)
	{
		perfdatasize=wxUINT32_SWAP_ON_LE(perfdatasize);
	}
	m_PerfInfo.perfdatasize=perfdatasize;

	// Load modulecount
	wxUint32 modulecount;
	if(!stream.Read(&modulecount,4).IsOk())
	{
		m_bValid=false;
		return stream;
	}
	if(bBigEndian)
	{
		modulecount=wxUINT32_SWAP_ON_LE(modulecount);
	}
	m_PerfInfo.modulecount=modulecount;
	m_PerfInfo.modules.resize(modulecount);

	// Load tickspersecond
	wxUint64 tickspersecond;
	if(!stream.Read(&tickspersecond,8).IsOk())
	{
		m_bValid=false;
		return stream;
	}
	if(bBigEndian)
	{
		tickspersecond=wxUINT64_SWAP_ON_LE(tickspersecond);
	}
	m_PerfInfo.tickspersecond=tickspersecond;

	// Load entertime
	wxUint64 entertime;
	if(!stream.Read(&entertime,8).IsOk())
	{
		m_bValid=false;
		return stream;
	}
	if(bBigEndian)
	{
		entertime=wxUINT64_SWAP_ON_LE(entertime);
	}
	m_PerfInfo.entertime=entertime;

	// Load exittime
	wxUint64 exittime;
	if(!stream.Read(&exittime,8).IsOk())
	{
		m_bValid=false;
		return stream;
	}
	if(bBigEndian)
	{
		exittime=wxUINT64_SWAP_ON_LE(exittime);
	}
	m_PerfInfo.exittime=exittime;


	// Load modules
	wxUint32 z;
	for(z=0;z<modulecount;z++)
	{
		if(m_PerfInfo.addrbits==32)
		{
			wxUint32 base;
			if(!stream.Read(&base,4).IsOk())
			{	
				m_bValid=false;
				return stream;
			}
			if(bBigEndian)
			{
				base=wxUINT32_SWAP_ON_LE(base);
			}
			m_PerfInfo.modules[z].nBase=(wxUint64)base;
	
			wxUint32 length;
			if(!stream.Read(&length,4).IsOk())
			{	
				m_bValid=false;
				return stream;
			}
			if(bBigEndian)
			{
				length=wxUINT32_SWAP_ON_LE(length);
			}
			m_PerfInfo.modules[z].nLength=(wxUint64)length;
		}
		else
		{
			wxUint64 base;
			if(!stream.Read(&base,8).IsOk())
			{	
				m_bValid=false;
				return stream;
			}
			if(bBigEndian)
			{
				base=wxUINT64_SWAP_ON_LE(base);
			}
			m_PerfInfo.modules[z].nBase=base;
	
			wxUint64 length;
			if(!stream.Read(&length,8).IsOk())
			{	
				m_bValid=false;
				return stream;
			}
			if(bBigEndian)
			{
				length=wxUINT64_SWAP_ON_LE(length);
			}
			m_PerfInfo.modules[z].nLength=length;
		}
		
		// Load nProfileEntries
		wxUint64 nProfileEntries;
		if(!stream.Read(&nProfileEntries,8).IsOk())
		{	
			m_bValid=false;
			return stream;
		}
		if(bBigEndian)
		{
			nProfileEntries=wxUINT64_SWAP_ON_LE(nProfileEntries);
		}
		m_PerfInfo.modules[z].nProfileEntries=nProfileEntries;
		
		// Load nProfileExits
		wxUint64 nProfileExits;
		if(!stream.Read(&nProfileExits,8).IsOk())
		{	
			m_bValid=false;
			return stream;
		}
		if(bBigEndian)
		{
			nProfileExits=wxUINT64_SWAP_ON_LE(nProfileExits);
		}
		m_PerfInfo.modules[z].nProfileExits=nProfileExits;

		// Load svName
		char name[256];
		if(!stream.Read(name,256).IsOk())
		{
			m_bValid=false;
			return stream;
		}
		m_PerfInfo.modules[z].strName=wxString(name,wxConvLibc);
		
		// Load svExePath
		char exepath[512];
		if(!stream.Read(exepath,512).IsOk())
		{
			m_bValid=false;
			return stream;
		}
		m_PerfInfo.modules[z].strExePath=wxString(exepath,wxConvLibc);
		
		// Load svDebugPath
		char dbgpath[512];
		if(!stream.Read(dbgpath,512).IsOk())
		{
			m_bValid=false;
			return stream;
		}
		m_PerfInfo.modules[z].strDebugPath=wxString(dbgpath,wxConvLibc);
		
	}

	// Load performance data
	m_arrPerfData.resize(perfdatasize);
	wxUint32 x;
	for(x=0;x<perfdatasize;x++)
	{
		/////////
		if(m_PerfInfo.addrbits==32)
		{
			wxUint32 parentaddr;
			if(!stream.Read(&parentaddr,4).IsOk())
			{	
				m_bValid=false;
				return stream;
			}
			if(bBigEndian)
			{
				parentaddr=wxUINT32_SWAP_ON_LE(parentaddr);
			}
			if(parentaddr==-1)
			{
				m_arrPerfData[x].nParentAddr.nAddr=(wxUint64)-1;
			}
			else
			{
				m_arrPerfData[x].nParentAddr.nAddr=(wxUint64)parentaddr;
			}
		}
		else if(m_PerfInfo.addrbits==64)
		{
			wxUint64 parentaddr;
			if(!stream.Read(&parentaddr,8).IsOk())
			{	
				m_bValid=false;
				return stream;
			}
			if(bBigEndian)
			{
				parentaddr=wxUINT64_SWAP_ON_LE(parentaddr);
			}
			m_arrPerfData[x].nParentAddr.nAddr=parentaddr;
		}

		/////////
		wxInt32 parentmodulenum;
		if(!stream.Read(&parentmodulenum,4).IsOk())
		{	
			m_bValid=false;
			return stream;
		}
		if(bBigEndian)
		{
			parentmodulenum=wxUINT32_SWAP_ON_LE(parentmodulenum);
		}
        m_arrPerfData[x].nParentAddr.nModule=parentmodulenum;

		/////////
		if(m_PerfInfo.addrbits==32)
		{
			wxUint32 fromaddr;
			if(!stream.Read(&fromaddr,4).IsOk())
			{	
				m_bValid=false;
				return stream;
			}
			if(bBigEndian)
			{
				fromaddr=wxUINT32_SWAP_ON_LE(fromaddr);
			}
			if(fromaddr==-1)
			{
				m_arrPerfData[x].nFromAddr.nAddr=(wxUint64)-1;
			}
			else
			{
				m_arrPerfData[x].nFromAddr.nAddr=(wxUint64)fromaddr;
			}
			m_arrPerfData[x].nFromAddr.nModule=parentmodulenum;
		}
		else if(m_PerfInfo.addrbits==64)
		{
			wxUint64 fromaddr;
			if(!stream.Read(&fromaddr,8).IsOk())
			{	
				m_bValid=false;
				return stream;
			}
			if(bBigEndian)
			{
				fromaddr=wxUINT64_SWAP_ON_LE(fromaddr);
			}
			m_arrPerfData[x].nFromAddr.nAddr=fromaddr;
			m_arrPerfData[x].nFromAddr.nModule=parentmodulenum;
		}

		/////////
		if(m_PerfInfo.addrbits==32)
		{
			wxUint32 toaddr;
			if(!stream.Read(&toaddr,4).IsOk())
			{	
				m_bValid=false;
				return stream;
			}
			if(bBigEndian)
			{
				toaddr=wxUINT32_SWAP_ON_LE(toaddr);
			}
			if(toaddr==-1)
			{
				m_arrPerfData[x].nToAddr.nAddr=(wxUint64)-1;
			}
			else
			{
				m_arrPerfData[x].nToAddr.nAddr=(wxUint64)toaddr;
			}
		}
		else if(m_PerfInfo.addrbits==64)
		{
			wxUint64 toaddr;
			if(!stream.Read(&toaddr,8).IsOk())
			{	
				m_bValid=false;
				return stream;
			}
			if(bBigEndian)
			{
				toaddr=wxUINT64_SWAP_ON_LE(toaddr);
			}
			m_arrPerfData[x].nToAddr.nAddr=toaddr;
		}

		/////////
		wxInt32 tomodulenum;
		if(!stream.Read(&tomodulenum,4).IsOk())
		{	
			m_bValid=false;
			return stream;
		}
		if(bBigEndian)
		{
			tomodulenum=wxUINT32_SWAP_ON_LE(tomodulenum);
		}
        m_arrPerfData[x].nToAddr.nModule=tomodulenum;

		
		/////////
		wxUint32 threadseqnum;
		if(!stream.Read(&threadseqnum,4).IsOk())
		{	
			m_bValid=false;
			return stream;
		}
		if(bBigEndian)
		{
			threadseqnum=wxUINT32_SWAP_ON_LE(threadseqnum);
		}
        m_arrPerfData[x].nThreadSeqNum=threadseqnum;


		
		wxUint32 totalcalls;
		if(!stream.Read(&totalcalls,4).IsOk())
		{	
			m_bValid=false;
			return stream;
		}
		if(bBigEndian)
		{
			totalcalls=wxUINT32_SWAP_ON_LE(totalcalls);
		}
		m_arrPerfData[x].nTotalCalls=(wxUint64)totalcalls;
	
		
		wxUint64 totaltime;
		if(!stream.Read(&totaltime,8).IsOk())
		{	
			m_bValid=false;
			return stream;
		}
		if(bBigEndian)
		{
			totaltime=wxUINT64_SWAP_ON_LE(totaltime);
		}
		m_arrPerfData[x].nTotalTime=totaltime;



		wxUint64 firststarttime;
		if(!stream.Read(&firststarttime,8).IsOk())
		{	
			m_bValid=false;
			return stream;
		}
		if(bBigEndian)
		{
			firststarttime=wxUINT64_SWAP_ON_LE(firststarttime);
		}
		m_arrPerfData[x].nFirstStartTime=firststarttime;


		wxUint64 lastendtime;
		if(!stream.Read(&lastendtime,8).IsOk())
		{	
			m_bValid=false;
			return stream;
		}
		if(bBigEndian)
		{
			lastendtime=wxUINT64_SWAP_ON_LE(lastendtime);
		}
		m_arrPerfData[x].nLastEndTime=lastendtime;


		wxUint64 mintime;
		if(!stream.Read(&mintime,8).IsOk())
		{	
			m_bValid=false;
			return stream;
		}
		if(bBigEndian)
		{
			mintime=wxUINT64_SWAP_ON_LE(mintime);
		}
		m_arrPerfData[x].nMinTime=mintime;


		wxUint64 maxtime;
		if(!stream.Read(&maxtime,8).IsOk())
		{	
			m_bValid=false;
			return stream;
		}
		if(bBigEndian)
		{
			maxtime=wxUINT64_SWAP_ON_LE(maxtime);
		}
		m_arrPerfData[x].nMaxTime=maxtime;


		wxUint64 totalnodefctime;
		if(!stream.Read(&totalnodefctime,8).IsOk())
		{	
			m_bValid=false;
			return stream;
		}
		if(bBigEndian)
		{
			totalnodefctime=wxUINT64_SWAP_ON_LE(totalnodefctime);
		}
		m_arrPerfData[x].nTotalNodeFCTime=totalnodefctime;


		wxUint64 minnodefctime;
		if(!stream.Read(&minnodefctime,8).IsOk())
		{	
			m_bValid=false;
			return stream;
		}
		if(bBigEndian)
		{
			minnodefctime=wxUINT64_SWAP_ON_LE(minnodefctime);
		}
		m_arrPerfData[x].nMinNodeFCTime=minnodefctime;

		wxUint64 maxnodefctime;
		if(!stream.Read(&maxnodefctime,8).IsOk())
		{	
			m_bValid=false;
			return stream;
		}
		if(bBigEndian)
		{
			maxnodefctime=wxUINT64_SWAP_ON_LE(maxnodefctime);
		}
		m_arrPerfData[x].nMaxNodeFCTime=maxnodefctime;

		
		wxUint64 totaledgefctime;
		if(!stream.Read(&totaledgefctime,8).IsOk())
		{	
			m_bValid=false;
			return stream;
		}
		if(bBigEndian)
		{
			totaledgefctime=wxUINT64_SWAP_ON_LE(totaledgefctime);
		}
		m_arrPerfData[x].nTotalEdgeFCTime=totaledgefctime;

		wxUint64 minedgefctime;
		if(!stream.Read(&minedgefctime,8).IsOk())
		{	
			m_bValid=false;
			return stream;
		}
		if(bBigEndian)
		{
			minedgefctime=wxUINT64_SWAP_ON_LE(minedgefctime);
		}
		m_arrPerfData[x].nMinEdgeFCTime=minedgefctime;

		wxUint64 maxedgefctime;
		if(!stream.Read(&maxedgefctime,8).IsOk())
		{	
			m_bValid=false;
			return stream;
		}
		if(bBigEndian)
		{
			maxedgefctime=wxUINT64_SWAP_ON_LE(maxedgefctime);
		}
		m_arrPerfData[x].nMaxEdgeFCTime=maxedgefctime;
	}

	if(!LoadDebugSymbols())
	{
		m_bValid=false;
		return stream;
	}

	if(!Collate())
	{
		m_bValid=false;
		return stream;
	}

	
	// Reset all filters and update
	m_llFilterThreadSeqNums.clear();
	for(wxUint32 i=1;i<m_arrThreadData.size();i++)
	{
		m_llFilterThreadSeqNums.push_back(i);
	}
	m_nFunctionFilterRoot=-1;
	
	UpdateFilters();
	UpdateSortedList();

	return stream;
}

void CSPDReaderDoc::UpdateFilters()
{
	////////////// FUNCTION FILTERS /////////////////////
	if(m_nFunctionFilterRoot==-1)
	{
		// Include all functions
		m_llFilteredFunctions.clear();
		for(wxUint32 i=0;i<m_arrFunctionData.size();i++)
		{
			m_llFilteredFunctions.insert(i);
		}
	}
	else
	{	
		// Need to figure out all the descendents of this function
		// with regard to the thread filter
		m_llFilteredFunctions.clear();
		
		stdext::hash_set<wxInt32> active;
		active.insert(m_nFunctionFilterRoot);
		while(active.size()!=0)
		{
			// Pop first function from the active set
			stdext::hash_set<wxInt32>::iterator iterfirst=active.begin();
			wxInt32 funcnum=*iterfirst;
			active.erase(iterfirst);
			CFunctionData *pFunc=&(m_arrFunctionData[funcnum]);

			// Put this function in the filter list
			m_llFilteredFunctions.insert(funcnum);
			
			// Find all children of this function per thread
			for(std::list<wxUint32>::iterator itert=m_llFilterThreadSeqNums.begin();itert!=m_llFilterThreadSeqNums.end();itert++)
			{
				FunctionDataPerThreadHash::iterator iterpt=pFunc->ThreadData.find(*itert);
				if(iterpt!=pFunc->ThreadData.end())
				{
					for(stdext::hash_map<wxUint32,CTimingStats>::iterator iterc=iterpt->second.Children.begin();iterc!=iterpt->second.Children.end();iterc++)
					{
                        if(m_llFilteredFunctions.find(iterc->first)==m_llFilteredFunctions.end())
						{
							active.insert(iterc->first);
						}
					}
				}
			}
		}
	}


	////////////// THREAD FILTERS //////////////////////

	m_PerfInfo.totalfilteredtime=0;

	std::list<wxUint32> llThreadFilteredFunctions;

	for(stdext::hash_set<wxUint32>::iterator iterf=m_llFilteredFunctions.begin();iterf!=m_llFilteredFunctions.end();iterf++)
	{
		CFunctionData *pFunc=&(m_arrFunctionData[*iterf]);

		pFunc->FilteredData=CFunctionDataPerThread();
		
		bool bFoundThread=false;

		for(std::list<wxUint32>::iterator itert=m_llFilterThreadSeqNums.begin();itert!=m_llFilterThreadSeqNums.end();itert++)
		{
			FunctionDataPerThreadHash::iterator iterpt=pFunc->ThreadData.find(*itert);
			if(iterpt==pFunc->ThreadData.end())
			{
				continue;
			}
			
			bFoundThread=true;
            
			pFunc->FilteredData.Stats.AddFunctionStats(iterpt->second.Stats);
			
			// Combine parents
			for(stdext::hash_map<wxUint32,CTimingStats>::iterator iterpins=iterpt->second.Parents.begin();iterpins!=iterpt->second.Parents.end();iterpins++)
			{
				if(m_llFilteredFunctions.find(iterpins->first)==m_llFilteredFunctions.end())
				{
					continue;
				}

				if(pFunc->FilteredData.Parents.find(iterpins->first)==pFunc->FilteredData.Parents.end())
				{
					pFunc->FilteredData.Parents[iterpins->first]=CTimingStats();
				}
				pFunc->FilteredData.Parents[iterpins->first].AddFunctionStats(iterpins->second);					
			}

			// Combine children
			for(stdext::hash_map<wxUint32,CTimingStats>::iterator itercins=iterpt->second.Children.begin();itercins!=iterpt->second.Children.end();itercins++)
			{
				if(m_llFilteredFunctions.find(itercins->first)==m_llFilteredFunctions.end())
				{
					continue;
				}

				if(pFunc->FilteredData.Children.find(itercins->first)==pFunc->FilteredData.Children.end())
				{
					pFunc->FilteredData.Children[itercins->first]=CTimingStats();
				}
				pFunc->FilteredData.Children[itercins->first].AddFunctionStats(itercins->second);
			}
		}

		if(!bFoundThread)
		{
			llThreadFilteredFunctions.push_back(*iterf);
			continue;
		}

		// Get averages from totals
		pFunc->FilteredData.Stats.CalculateAverages();
		for(stdext::hash_map<wxUint32,CTimingStats>::iterator iterc=pFunc->FilteredData.Children.begin();iterc!=pFunc->FilteredData.Children.end();iterc++)
		{
			iterc->second.CalculateAverages();
		}
		for(stdext::hash_map<wxUint32,CTimingStats>::iterator iterp=pFunc->FilteredData.Parents.begin();iterp!=pFunc->FilteredData.Parents.end();iterp++)
		{
			iterp->second.CalculateAverages();
		}

		m_PerfInfo.totalfilteredtime+=pFunc->FilteredData.Stats.TotalFunctionTime;
	}

	// Remove functions that aren't in any thread from the filtered function list
	for(std::list<wxUint32>::iterator iterdead=llThreadFilteredFunctions.begin();iterdead!=llThreadFilteredFunctions.end();iterdead++)
	{
		m_llFilteredFunctions.erase(*iterdead);
	}
}


void CSPDReaderDoc::SetThreadFilter(std::list<wxUint32> &llThreadSeqNums)
{
	m_llFilterThreadSeqNums=llThreadSeqNums;
	UpdateFilters();
	UpdateSortedList();
}

void CSPDReaderDoc::ClearThreadFilter(void)
{
	m_llFilterThreadSeqNums.clear();
	for(wxUint32 i=0;i<m_arrThreadData.size();i++)
	{
		m_llFilterThreadSeqNums.push_back(i);
	}

	UpdateFilters();
	UpdateSortedList();
}

void CSPDReaderDoc::SetFocusFilter(wxInt32 functionnumber)
{
	m_nFunctionFilterRoot=functionnumber;

	UpdateFilters();
	UpdateSortedList();
}

void CSPDReaderDoc::ClearFocusFilter(void)
{
	m_nFunctionFilterRoot=-1;
	UpdateFilters();
	UpdateSortedList();
}

wxInt32 CSPDReaderDoc::GetFocusFilter(void)
{
	return m_nFunctionFilterRoot;
}
	

bool CSPDReaderDoc::BinarySortFunctionList(wxUint32 idx1, wxUint32 idx2)
{
	CFunctionData *pFunc1;
	CFunctionData *pFunc2;
	
	if(m_bSortDirection)
	{
		pFunc2=&(m_arrFunctionData[idx1]);
		pFunc1=&(m_arrFunctionData[idx2]);
	}
	else
	{
		pFunc1=&(m_arrFunctionData[idx1]);
		pFunc2=&(m_arrFunctionData[idx2]);
	}

	switch(m_nSortField)
	{
	
	case CSPDReaderView::NAME:
		return (pFunc1->Desc.strBaseName<pFunc2->Desc.strBaseName);
	case CSPDReaderView::SOURCE:
		return (pFunc1->Desc.strSourceFile<pFunc2->Desc.strSourceFile) ||
			   ((pFunc1->Desc.strSourceFile==pFunc2->Desc.strSourceFile) && (pFunc1->Desc.nFirstSourceLine<pFunc2->Desc.nFirstSourceLine));		
	case CSPDReaderView::NUMOFCALLS:
		return (pFunc1->FilteredData.Stats.Count<pFunc2->FilteredData.Stats.Count);
	case CSPDReaderView::MINFTIME:
		return (pFunc1->FilteredData.Stats.MinimumFunctionTime<pFunc2->FilteredData.Stats.MinimumFunctionTime);
	case CSPDReaderView::AVGFTIME:
		return (pFunc1->FilteredData.Stats.AverageFunctionTime<pFunc2->FilteredData.Stats.AverageFunctionTime);
	case CSPDReaderView::MAXFTIME:
		return (pFunc1->FilteredData.Stats.MaximumFunctionTime<pFunc2->FilteredData.Stats.MaximumFunctionTime);
	case CSPDReaderView::MINFCTIME:
		return (pFunc1->FilteredData.Stats.MinimumFunctionAndChildrenTime<pFunc2->FilteredData.Stats.MinimumFunctionAndChildrenTime);
	case CSPDReaderView::AVGFCTIME:
		return (pFunc1->FilteredData.Stats.AverageFunctionAndChildrenTime<pFunc2->FilteredData.Stats.AverageFunctionAndChildrenTime);
	case CSPDReaderView::MAXFCTIME:
		return (pFunc1->FilteredData.Stats.MaximumFunctionAndChildrenTime<pFunc2->FilteredData.Stats.MaximumFunctionAndChildrenTime);
	case CSPDReaderView::TOTALFTIME:
		return (pFunc1->FilteredData.Stats.TotalFunctionTime<pFunc2->FilteredData.Stats.TotalFunctionTime);
	case CSPDReaderView::TOTALFCTIME:
		return (pFunc1->FilteredData.Stats.TotalFunctionAndChildrenTime<pFunc2->FilteredData.Stats.TotalFunctionAndChildrenTime);
	case CSPDReaderView::AVGFOVERSIZE:
		{
			wxUint64 div1=0;
			wxUint64 div2=0;
			if(pFunc1->Desc.nByteLength!=0)
			{
				div1=pFunc1->FilteredData.Stats.AverageFunctionTime/pFunc1->Desc.nByteLength;
			}
			if(pFunc2->Desc.nByteLength!=0)
			{
				div2=pFunc2->FilteredData.Stats.AverageFunctionTime/pFunc2->Desc.nByteLength;
			}

			return div1<div2;
		}
	default:
		break;
	}

	return false;
}

void CSPDReaderDoc::UpdateSortedList()
{
	m_llSortedFunctions.clear();

	m_llSortedFunctions.resize(m_llFilteredFunctions.size());
	int curidx=0;
	for(stdext::hash_set<wxUint32>::iterator iterf=m_llFilteredFunctions.begin();iterf!=m_llFilteredFunctions.end();iterf++)
	{
		m_llSortedFunctions[curidx]=*iterf;
		curidx++;
	}

	sort(m_llSortedFunctions.begin(),m_llSortedFunctions.end(),boost::bind(&CSPDReaderDoc::BinarySortFunctionList, this, _1, _2 ));

	// Build index for quick searches
	int i;
	for(i=0;i<(int)m_llSortedFunctions.size();i++)
	{
		m_hmSortedFunctionIndex[m_llSortedFunctions[i]]=i;
	}
}

void CSPDReaderDoc::SortFunctions(int field, bool bBackward)
{
	m_nSortField=field;
	m_bSortDirection=bBackward;
	UpdateSortedList();
}


wxInputStream& CSPDReaderDoc::LoadObject(wxInputStream& instream)
{
	wxInputStream & stream=wxDocument::LoadObject(instream);

	// Load all perfdata and debug stuff

#define READ(s,d) { (s).Read(&(d),sizeof(d)); }
#define READSTRING(s,d) { wxUint32 l; (s).Read(&l,sizeof(l)); (s).Read((d).GetWriteBuf(l),l*sizeof(TCHAR)); (d).UngetWriteBuf(); }

	// Save all perfdata
	wxUint32 i,count;
	READ(stream,count);
	m_arrPerfData.resize(count);
	for(i=0;i<count;i++)
	{
		READ(stream,m_arrPerfData[i].nParentAddr.nAddr);
		READ(stream,m_arrPerfData[i].nParentAddr.nModule);
		READ(stream,m_arrPerfData[i].nFromAddr.nAddr);
		READ(stream,m_arrPerfData[i].nFromAddr.nModule);
		READ(stream,m_arrPerfData[i].nToAddr.nAddr);
		READ(stream,m_arrPerfData[i].nToAddr.nModule);
		READ(stream,m_arrPerfData[i].nThreadSeqNum);
		
		READ(stream,m_arrPerfData[i].nTotalCalls);
		READ(stream,m_arrPerfData[i].nTotalTime);
		READ(stream,m_arrPerfData[i].nFirstStartTime);
		READ(stream,m_arrPerfData[i].nLastEndTime);
		READ(stream,m_arrPerfData[i].nMinTime);
		READ(stream,m_arrPerfData[i].nMaxTime);
		
		READ(stream,m_arrPerfData[i].nTotalNodeFCTime);
		READ(stream,m_arrPerfData[i].nMinNodeFCTime);
		READ(stream,m_arrPerfData[i].nMaxNodeFCTime);
		
		READ(stream,m_arrPerfData[i].nTotalEdgeFCTime);
		READ(stream,m_arrPerfData[i].nMinEdgeFCTime);
		READ(stream,m_arrPerfData[i].nMaxEdgeFCTime);
	}

	// Address map
	READ(stream,count);
	for(i=0;i<count;i++)
	{
		BasedAddress addr;
		wxInt32 funcidx;

		READ(stream,addr);
		READ(stream,funcidx);
		m_addressMap[addr]=funcidx;
	}


	// Function descriptions
	READ(stream,count);
	m_arrFunctionData.resize(count);
	for(i=0;i<count;i++)
	{
		CFunctionDescription desc;

		READSTRING(stream,desc.strBaseName);
		READSTRING(stream,desc.strFullName);
		READSTRING(stream,desc.strMangledName);
		READSTRING(stream,desc.strSourceFile);
		READ(stream,desc.nFirstSourceLine);
		READ(stream,desc.nLastSourceLine);
		READ(stream,desc.nByteLength);
		READ(stream,desc.nAddress.nAddr);
		READ(stream,desc.nAddress.nModule);

		m_arrFunctionData[i].Desc=desc;
	}
	
	// Perfinfo
	READ(stream,m_PerfInfo.targetmode);
	READSTRING(stream,m_PerfInfo.mainpath);
	READ(stream,m_PerfInfo.perfdatasize);
	READ(stream,m_PerfInfo.modulecount);
	READ(stream,m_PerfInfo.tickspersecond);
	READ(stream,m_PerfInfo.entertime);
	READ(stream,m_PerfInfo.exittime);
	READ(stream,m_PerfInfo.totalwallclocktime);
	READ(stream,m_PerfInfo.totalfilteredtime);
	
	READ(stream,count);
	m_PerfInfo.modules.resize(count);
	for(i=0;i<count;i++)
	{
		READ(stream,m_PerfInfo.modules[i].nBase);
		READ(stream,m_PerfInfo.modules[i].nLength);
		READ(stream,m_PerfInfo.modules[i].nProfileEntries);
		READ(stream,m_PerfInfo.modules[i].nProfileExits);
		READSTRING(stream,m_PerfInfo.modules[i].strName);
		READSTRING(stream,m_PerfInfo.modules[i].strExePath);
		READSTRING(stream,m_PerfInfo.modules[i].strDebugPath);
	}

	READ(stream,m_PerfInfo.addrbits);

	// Collate all data together
	if(!Collate())
	{
		m_bValid=false;
		return stream;
	}
	
	// Reset all filters and update
	m_llFilterThreadSeqNums.clear();
	for(wxUint32 i=1;i<m_arrThreadData.size();i++)
	{
		m_llFilterThreadSeqNums.push_back(i);
	}
	m_nFunctionFilterRoot=-1;
	
	UpdateFilters();
	UpdateSortedList();

	return stream;
}

wxOutputStream& CSPDReaderDoc::SaveObject(wxOutputStream& outstream)
{
	wxOutputStream & stream=wxDocument::SaveObject(outstream);

#define WRITE(s,d) { (s).Write(&(d),sizeof(d)); }
#define WRITESTRING(s,d) { wxUint32 l=(wxUint32)(d).Len()+1; (s).Write(&(l),sizeof(l)); (s).Write((const TCHAR *)(d),(l)*sizeof(TCHAR)); }

	// Save all perfdata
	wxUint32 i,count=(wxUint32)m_arrPerfData.size();
	WRITE(stream,count);
	for(i=0;i<count;i++)
	{
		WRITE(stream,m_arrPerfData[i].nParentAddr.nAddr);
		WRITE(stream,m_arrPerfData[i].nParentAddr.nModule);
		WRITE(stream,m_arrPerfData[i].nFromAddr.nAddr);
		WRITE(stream,m_arrPerfData[i].nFromAddr.nModule);
		WRITE(stream,m_arrPerfData[i].nToAddr.nAddr);
		WRITE(stream,m_arrPerfData[i].nToAddr.nModule);
		WRITE(stream,m_arrPerfData[i].nThreadSeqNum);
		
		WRITE(stream,m_arrPerfData[i].nTotalCalls);
		WRITE(stream,m_arrPerfData[i].nTotalTime);
		WRITE(stream,m_arrPerfData[i].nFirstStartTime);
		WRITE(stream,m_arrPerfData[i].nLastEndTime);
		WRITE(stream,m_arrPerfData[i].nMinTime);
		WRITE(stream,m_arrPerfData[i].nMaxTime);
		
		WRITE(stream,m_arrPerfData[i].nTotalNodeFCTime);
		WRITE(stream,m_arrPerfData[i].nMinNodeFCTime);
		WRITE(stream,m_arrPerfData[i].nMaxNodeFCTime);
		
		WRITE(stream,m_arrPerfData[i].nTotalEdgeFCTime);
		WRITE(stream,m_arrPerfData[i].nMinEdgeFCTime);
		WRITE(stream,m_arrPerfData[i].nMaxEdgeFCTime);
	}

	// Address map
	count=(wxUint32)m_addressMap.size();
	WRITE(stream,count);
	for(AddressMapHash::iterator itera=m_addressMap.begin();itera!=m_addressMap.end();itera++)
	{
		BasedAddress addr=itera->first;
		wxInt32 funcidx=itera->second;

		WRITE(stream,addr);
		WRITE(stream,funcidx);
	}


	// Function descriptions
	count=(wxUint32)m_arrFunctionData.size();
	WRITE(stream,count);
	for(i=0;i<count;i++)
	{
		CFunctionDescription desc=m_arrFunctionData[i].Desc;

		WRITESTRING(stream,desc.strBaseName);
		WRITESTRING(stream,desc.strFullName);
		WRITESTRING(stream,desc.strMangledName);
		WRITESTRING(stream,desc.strSourceFile);
		WRITE(stream,desc.nFirstSourceLine);
		WRITE(stream,desc.nLastSourceLine);
		WRITE(stream,desc.nByteLength);
		WRITE(stream,desc.nAddress.nAddr);
		WRITE(stream,desc.nAddress.nModule);
	}

	// Perfinfo
	WRITE(stream,m_PerfInfo.targetmode);
	WRITESTRING(stream,m_PerfInfo.mainpath);
	WRITE(stream,m_PerfInfo.perfdatasize);
	WRITE(stream,m_PerfInfo.modulecount);
	WRITE(stream,m_PerfInfo.tickspersecond);
	WRITE(stream,m_PerfInfo.entertime);
	WRITE(stream,m_PerfInfo.exittime);
	WRITE(stream,m_PerfInfo.totalwallclocktime);
	WRITE(stream,m_PerfInfo.totalfilteredtime);
	
	count=(wxUint32)m_PerfInfo.modules.size();
	WRITE(stream,count);

	for(i=0;i<count;i++)
	{
		WRITE(stream,m_PerfInfo.modules[i].nBase);
		WRITE(stream,m_PerfInfo.modules[i].nLength);
		WRITE(stream,m_PerfInfo.modules[i].nProfileEntries);
		WRITE(stream,m_PerfInfo.modules[i].nProfileExits);
		WRITESTRING(stream,m_PerfInfo.modules[i].strName);
		WRITESTRING(stream,m_PerfInfo.modules[i].strExePath);
		WRITESTRING(stream,m_PerfInfo.modules[i].strDebugPath);
	}

	WRITE(stream,m_PerfInfo.addrbits);
	
	return stream;
}

