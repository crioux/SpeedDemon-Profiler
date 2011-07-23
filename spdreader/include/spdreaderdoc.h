#ifndef __INC_SPDREADERDOC_H
#define __INC_SPDREADERDOC_H

#define SD_X86_WIN32	1
#define SD_X86_WCE		2
#define SD_PXA270_WCE	3
#define SD_PXA255_WCE	4
#define SD_X64_WIN64	5

class BasedAddress
{
public:
	wxUint64 nAddr;
	wxInt32 nModule;

	BasedAddress():nAddr(-1),nModule(-1) {}
	BasedAddress(wxUint64 _nAddr, wxInt32 _nModule):nAddr(_nAddr),nModule(_nModule) {}
	bool IsValid() const { return !(nModule==-1 && nAddr==-1); }
	bool operator<(const BasedAddress &other) const { return (nModule<other.nModule) || (nModule==other.nModule && nAddr<other.nAddr); }
	
};

namespace stdext
{
	template<> inline size_t hash_value(const BasedAddress &x) { return hash_value(x.nAddr) ^ hash_value(x.nModule); }
};

class CPerfData
{
public:
	BasedAddress nParentAddr;
	BasedAddress nFromAddr;
	BasedAddress nToAddr;
	wxUint32 nThreadSeqNum;
	
	wxUint32 nTotalCalls;
	wxUint64 nTotalTime;
	wxUint64 nFirstStartTime;
	wxUint64 nLastEndTime;
	wxUint64 nMinTime;
	wxUint64 nMaxTime;
	
	wxUint64 nTotalNodeFCTime;
	wxUint64 nMinNodeFCTime;
	wxUint64 nMaxNodeFCTime;
	
	wxUint64 nTotalEdgeFCTime;
	wxUint64 nMinEdgeFCTime;
	wxUint64 nMaxEdgeFCTime;
};

class CModuleInfo
{
public:
	wxUint64 nBase;
	wxUint64 nLength;
	wxUint64 nProfileEntries;
	wxUint64 nProfileExits;
	wxString strName;
	wxString strExePath;
	wxString strDebugPath;
};

class CPerfInfo
{
public:
	wxUint32 targetmode;
	wxString mainpath;
	wxUint32 perfdatasize;
	wxUint32 modulecount;
	wxUint64 tickspersecond;
	wxUint64 entertime;
	wxUint64 exittime;
	wxUint64 totalwallclocktime;
	wxUint64 totalfilteredtime;
	std::vector<CModuleInfo> modules;

	wxUint32 addrbits;
};

struct CFunctionDescription
{	
	wxString strBaseName;
	wxString strFullName;
	wxString strMangledName;
	wxString strSourceFile;
	wxUint32 nFirstSourceLine;
	wxUint32 nLastSourceLine;
	wxUint64 nByteLength;
	BasedAddress nAddress;
	
	CFunctionDescription()
	{
		strBaseName=wxT("<unknown>");
		strFullName=wxT("<unknown>");
		strMangledName=wxT("<unknown>");
		strSourceFile=wxT("<unknown>");
		nFirstSourceLine=0;
		nLastSourceLine=0;
		nByteLength=0;
	}
};

struct CTimingStats
{
	wxUint64 Count;
	wxUint64 MinimumFunctionTime;
	wxUint64 AverageFunctionTime;
	wxUint64 MaximumFunctionTime;
	wxUint64 MinimumFunctionAndChildrenTime;
	wxUint64 AverageFunctionAndChildrenTime;
	wxUint64 MaximumFunctionAndChildrenTime;
	wxUint64 TotalFunctionTime;
	wxUint64 TotalFunctionAndChildrenTime;

public:
	CTimingStats();
	void AddPerfData(const CPerfData &pd, bool bEdge);
	void AddFunctionStats(const CTimingStats &pd);
	void CalculateAverages(void);
};

struct CFunctionDataPerThread
{
	CTimingStats Stats;

	stdext::hash_map<wxUint32,CTimingStats> Parents;	// Map function number to number of times called from
	stdext::hash_map<wxUint32,CTimingStats> Children;	// Map function number to number of times called to
};


typedef stdext::hash_map<wxUint32,CFunctionDataPerThread> FunctionDataPerThreadHash;
typedef stdext::hash_map<BasedAddress,int> AddressMapHash;


struct CFunctionData
{
	// Function Description
	CFunctionDescription Desc;
	
	// Raw collated thread timings
	FunctionDataPerThreadHash ThreadData;
	
	// Calculated total results with the current filter
	CFunctionDataPerThread FilteredData;
};


class CThreadData
{
public:
	CThreadData()
	{
		nThreadSeqNum=-1;
		nThreadStartTime=~(wxUint64)0;
		nThreadEndTime=0;
		nTotalRecordedThreadTime=0;
	}

	int nThreadSeqNum;
    wxUint64 nThreadStartTime;
	wxUint64 nThreadEndTime;
	wxUint64 nTotalRecordedThreadTime;
	stdext::hash_set<wxUint32> TopLevelFunctions;
};


class CSPDReaderDoc:public wxDocument
{
	DECLARE_DYNAMIC_CLASS(CSPDReaderDoc)

public:
	std::vector<CPerfData> m_arrPerfData;
	std::vector<CFunctionData> m_arrFunctionData;
	std::vector<CThreadData> m_arrThreadData; 
	AddressMapHash m_addressMap;

	CPerfInfo m_PerfInfo;

	// Filters
	std::list<wxUint32> m_llFilterThreadSeqNums;
	
	stdext::hash_set<wxUint32> m_llFilteredFunctions; // calculated
	int m_nFunctionFilterRoot;

	// Sorting
	std::vector<wxUint32> m_llSortedFunctions;
	stdext::hash_map<wxUint32,int> m_hmSortedFunctionIndex;
	

protected:
	bool LoadDebugSymbols(void);
	bool Collate(void);

	void UpdateSortedList();
	void UpdateFilters();

	bool BinarySortFunctionList(wxUint32 idx1, wxUint32 idx2);
	
	virtual wxInputStream& LoadObject(wxInputStream& stream);
	virtual wxOutputStream& SaveObject(wxOutputStream& stream);

private:
	bool m_bValid;

	int m_nSortField;
	bool m_bSortDirection;	

public:
	CSPDReaderDoc();
	~CSPDReaderDoc();

	wxInputStream& ImportData(wxInputStream & stream);

	bool IsValid() { return m_bValid; } 
	bool GetSortDirection() { return m_bSortDirection; }
	int GetSortColumn() { return m_nSortField; }

	void SetThreadFilter(std::list<wxUint32> &llThreadSeqNums);
	void ClearThreadFilter(void);
	
	void SetFocusFilter(wxInt32 functionnumber);
	void ClearFocusFilter(void);
	wxInt32 GetFocusFilter(void);
	
	void SortFunctions(int field, bool bBackward);
	

};

#endif