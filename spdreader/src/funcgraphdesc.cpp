#include"spdreader_pch.h"
#include"graphview.h"
#include"graphdescriptor.h"
#include"funcgraphdesc.h"
#include"spdreaderdoc.h"


CFuncGraphDesc::CFuncGraphDesc(CSPDReaderDoc *pDoc)
{
	m_pDocument=pDoc;

}

CFuncGraphDesc::~CFuncGraphDesc()
{
}


void CFuncGraphDesc::Build(void)
{
	// Get size of node and edge tables
	int nNodeCount=0,nEdgeCount=0;
	for(stdext::hash_set<wxUint32>::iterator iter=m_pDocument->m_llFilteredFunctions.begin();iter!=m_pDocument->m_llFilteredFunctions.end();iter++)
	{
		CFunctionData *pfd=&(m_pDocument->m_arrFunctionData[*iter]);
		nNodeCount++;
		nEdgeCount+=(int)pfd->FilteredData.Children.size();
	}

	m_nodes.resize(nNodeCount);
	m_edges.resize(nEdgeCount);

	int node=0;
	int edge=0;
	for(stdext::hash_set<wxUint32>::iterator iter=m_pDocument->m_llFilteredFunctions.begin();iter!=m_pDocument->m_llFilteredFunctions.end();iter++)
	{
		CFunctionData *pfd=&(m_pDocument->m_arrFunctionData[*iter]);

		m_nodes[node].id=*iter;
		m_nodes[node].stats=&(pfd->FilteredData.Stats);

		m_nodes[node].edges.resize(pfd->FilteredData.Children.size());

		for(stdext::hash_map<wxUint32,CTimingStats>::iterator iterc=pfd->FilteredData.Children.begin();iterc!=pfd->FilteredData.Children.end();iterc++)
		{
			m_edges[edge].fromid=*iter;
			m_edges[edge].toid=iterc->first;
			m_edges[edge].stats=&(iterc->second);
			
			m_nodes[node].edges.push_back(edge);

			edge++;
		}

		node++;
	}
}

void CFuncGraphDesc::StartNodeIteration(void)
{
	m_iter_node=0;
}

bool CFuncGraphDesc::GetNextNode(long *pnNodeId)
{
	if(m_iter_node==m_nodes.size())
	{
		return false;
	}
	
	if(pnNodeId)
	{
		*pnNodeId=(long)m_iter_node;
	}

	m_iter_node++;
	return true;
}

void CFuncGraphDesc::GetNodeInfo(long nNodeId, wxString *pStrName, wxString *pStrText, NODETYPE *pNodeType)
{
	CFunctionData *pfd=&(m_pDocument->m_arrFunctionData[m_nodes[nNodeId].id]);

	if(pStrName)
	{
		*pStrName=pfd->Desc.strBaseName;
	}
	if(pStrText)
	{
		*pStrText=pfd->Desc.strFullName;
	}
	if(pNodeType)
	{
		size_t nc=pfd->FilteredData.Children.size();
		size_t np=pfd->FilteredData.Parents.size();
		if(nc==0 || np==0)
		{
			if(np==0)
			{
				*pNodeType=SELECTEDCIRCLE;
			}
			else
			{
				*pNodeType=CIRCLE;
			}
		}

		if(nc==1 && np==1)
		{
			*pNodeType=ELLIPSE;
		}

		*pNodeType=RECTANGLE;
	}
}

void CFuncGraphDesc::StartOutEdgeIteration(long nNodeId)
{
	m_iter_edge_nodeid=nNodeId;
	m_iter_edge=0;
}

bool CFuncGraphDesc::GetNextOutEdge(long *pnEdgeId)
{
	if(m_iter_edge==m_nodes[m_iter_edge_nodeid].edges.size())
	{
		return false;
	}
	
	if(pnEdgeId)
	{
		*pnEdgeId=m_nodes[m_iter_edge_nodeid].edges[m_iter_edge];
	}

	m_iter_edge++;
	return true;
}

void CFuncGraphDesc::GetEdgeInfo(long nEdgeId, long *pnNodeFrom, long *pnNodeTo, wxString *pStrLabel, LINETYPE *pLineType)
{
	EDGE *pEdge=&(m_edges[nEdgeId]);

	if(pnNodeFrom)
	{
		*pnNodeFrom=pEdge->fromid;
	}
	if(pnNodeTo)
	{
		*pnNodeTo=pEdge->toid;
	}
	if(pStrLabel)
	{
		*pStrLabel=wxT("");
	}
	if(pLineType)
	{
		*pLineType=REGULAR;
	}
}

void CFuncGraphDesc::ExtraDrawing(long nId, wxDC *pdc, int cx, int cy, int w, int h)
{
}

