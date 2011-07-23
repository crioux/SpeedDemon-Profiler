#ifndef __INC_GRAPHDESCRIPTOR_H
#define __INC_GRAPHDESCRIPTOR_H


class CGraphDescriptor
{

public:
	
	typedef enum {
		RECTANGLE,
		CIRCLE,
		SELECTEDCIRCLE,
		ELLIPSE,
		TRIANGLE,
		DIAMOND,
		RHOMBUS,
		TRAPEZOID,
	} NODETYPE;

	typedef enum {
		REGULAR,
		DOTTED,
	} LINETYPE;

	virtual ~CGraphDescriptor() {};

	// Graph accessors
	virtual void StartNodeIteration(void)=0;
	virtual bool GetNextNode(long *pnNodeId)=0;
	virtual void GetNodeInfo(long nNodeId, wxString *pStrName=NULL, wxString *pStrText=NULL, NODETYPE *pNodeType=NULL)=0;
	virtual void StartOutEdgeIteration(long nNodeId)=0;
	virtual bool GetNextOutEdge(long *pnEdgeId)=0;
	virtual void GetEdgeInfo(long nEdgeId, long *pnNodeFrom, long *pnNodeTo, wxString *pStrLabel=NULL, LINETYPE *pLineType=NULL)=0;
	virtual void ExtraDrawing(long nId, wxDC *pdc, int cx, int cy, int w, int h) {}
};

#endif