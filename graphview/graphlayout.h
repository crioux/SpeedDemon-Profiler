#ifndef __INC_GRAPHLAYOUT_H
#define __INC_GRAPHLAYOUT_H

#include"wx/dynarray.h"

class CGraphNode;
class CGraphEdge;
class wxHashTable;

class CGraphLayout
{

protected:

	int m_nDebugLevel;			// value 0-9, controls the amount of debug output
	bool m_bFlexScale;			// flexible scaling of x and y indep., not square
	int m_nRotation;			// rotation of final graph, * 90 degrees
	bool m_bMergeEdges;
	bool m_bRouteEdges;
	bool m_bTransform;

	float m_afScale[2][3];
	float m_afCurrBox[4];
	float m_afBBox[4];

	float m_fNodeWidth;
	float m_fNodeHeight;

	int m_nMaxX, m_nMaxY;

	wxHashTable *m_pNodeHashTable;
	std::vector<CGraphNode*> m_nodeArray;
	std::vector<CGraphEdge*> m_arcArray;
	CGraphNode *m_pFirstNode;
	int m_nNumNodes;

	CGraphNode **m_pVPlace;
	CGraphNode **m_pHPlace;

	FILE *m_pErrOut;
	unsigned int m_nIterArc;

protected:

	void SetScale();
	void DoScale(float *x, float *y);
	void MultY( float f);
	void MultX( float f);
	void Translate( float dx, float dy);
	void Rotate( int i);
	void PlaceGraph();
	void Verbose( char *fmt, ...);

	void LinkNode(CGraphNode **listp, CGraphNode *node);
	void UnlinkNode(CGraphNode *node);
	void LinkEdge(CGraphEdge *edge, CGraphNode *fromnode, CGraphNode *tonode);
	void UnlinkEdge(CGraphEdge *edge);

	CGraphNode *AddNodeInternal(long id, const wxString &name);

	void DrawNodes(wxDC *pdc);
	void DrawArcs(wxDC *pdc);
	void DrawNode(wxDC *pdc, long id);
	void DrawArc(wxDC *pdc, CGraphNode *from, CGraphNode *to);

	wxString GetNodeName(long id);
	wxRealPoint GetNodePosition(long id);
	void GetNodePosition(long id, float *x, float *y) { wxRealPoint p=GetNodePosition(id); *x=p.x; *y=p.y; }

public:
	CGraphLayout();
	~CGraphLayout();

	// Options
	void SetRouteEdges(bool bRouteEdges) { m_bRouteEdges=bRouteEdges; }
	bool GetRouteEdges(void ) { return m_bRouteEdges; }

	// Bounding box setup
	inline void SetTransform(bool bTransform) { m_bTransform=bTransform; }
	void SetBoundingBox(float x1, float y1, float x2, float y2);
	inline void SetBoundingBox(float x, float y) { SetBoundingBox(0.0f,0.0f,x,y); }
	inline void SetRotation(int rot) { m_nRotation = rot; }
	inline int GetRotation(void) { return m_nRotation; }
	inline void GetGraphSize(int *pWidth, int *pHeight) { *pWidth=m_nMaxX+1; *pHeight=m_nMaxY+1; }
	void SetNodeSize(float width, float height);

	// Graph accessors
	void Clear(void);

	void AddNode(long id, const wxString & name=wxT(""));
	void AddArc(long edge_id, long fromId, long toId, bool hide=false);

	void StartNodeIteration(void);
	bool GetNextNode(wxRealPoint *pPos=NULL, long *pId=NULL, wxString *pName=NULL);

	void StartArcIteration(void);
	bool GetNextArc(long *pnEdgeId, wxRealPoint *pPosStart=NULL, wxRealPoint *pPosEnd=NULL, long *pIdFrom=NULL, long *pIdTo=NULL, std::vector<wxRealPoint> *pCtrlPoints=NULL);

	// Layout functionality
	void DoLayout(void);

	// Node drawing for debugging purposes
	void Draw(wxDC *pdc);
};


#endif

