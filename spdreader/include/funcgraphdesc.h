#ifndef __INC_FUNCGRAPHDESC
#define __INC_FUNCGRAPHDESC

#include"spdreaderdoc.h"
#include"graphdescriptor.h"

class CFuncGraphDesc:public CGraphDescriptor
{
private:
	CSPDReaderDoc *m_pDocument;
	
	
	class EDGE
	{
	public:
		wxUint32 fromid;
		wxUint32 toid;
		CTimingStats *stats;
	};
	
	class NODE
	{
	public:
		wxUint32 id;
		CTimingStats *stats;
		std::vector<int> edges;
	};
	

	std::vector<NODE> m_nodes;
	std::vector<EDGE> m_edges;
	
	int m_iter_node;

	int m_iter_edge_nodeid;
	int m_iter_edge;
	
public:
	CFuncGraphDesc(CSPDReaderDoc *pDoc);
	virtual ~CFuncGraphDesc();

	void Build(void);

	// Graph accessors
	virtual void StartNodeIteration(void);
	virtual bool GetNextNode(long *pnNodeId);
	virtual void GetNodeInfo(long nNodeId, wxString *pStrName=NULL, wxString *pStrText=NULL, NODETYPE *pNodeType=NULL);
	virtual void StartOutEdgeIteration(long nNodeId);
	virtual bool GetNextOutEdge(long *pnEdgeId);
	virtual void GetEdgeInfo(long nEdgeId, long *pnNodeFrom, long *pnNodeTo, wxString *pStrLabel=NULL, LINETYPE *pLineType=NULL);
	virtual void ExtraDrawing(long nId, wxDC *pdc, int cx, int cy, int w, int h);
};

#endif
