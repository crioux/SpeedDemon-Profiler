#include"graphview_pch.h"
#include <math.h>
#include "libdotgraphlayout.h"
#include "graphlayout.h"

using namespace std;

/****************** typedefs **************************************/

class CGraphEdge;
class CGraphNode
{
public:
	long id; // JACS
	wxString name;

	float x;
	float y;

	CGraphNode *next;
	CGraphNode **prevp; /* doublelinked list by pointing to last 'next'*/
	CGraphEdge *inedges;
	CGraphEdge *outedges;
	CGraphNode(void)
	{
		x = 0; y = 0; id = -1; next = NULL;
		prevp = NULL; inedges = NULL; outedges = NULL;
	}
};

class CGraphEdge
{
public:
	long id; // JACS

	bool drawn;

	CGraphNode *from;
	CGraphNode *to;
	CGraphEdge *next_inedge;
	CGraphEdge *next_outedge;
	std::list<wxRealPoint> controlpoints;

	CGraphEdge(void)
	{
		id = 0; from = NULL; to = NULL; next_inedge = NULL; next_outedge = NULL;
		drawn = 0;
	}
};

#define DIST 3		/* node distances in horizontal placement */
#define Exch( x1, x2)	{float h=x1; x1=x2; x2=h;}

/*
* Abstract graph
*
*/

CGraphLayout::CGraphLayout()
{
	// Private variables
	m_pFirstNode=NULL;
	m_pVPlace=NULL;
	m_pHPlace=NULL;
	m_nNumNodes=0;
	m_nMaxX=0;
	m_nMaxY=0;
	m_nDebugLevel=9;
	m_nRotation=0;
	m_bFlexScale=true;
	m_bRouteEdges=true;
	m_bMergeEdges=false;
	m_bTransform=true;


	m_pNodeHashTable = NULL;

	Clear();

	m_pNodeHashTable = new wxHashTable(wxKEY_INTEGER);


	m_afBBox[0] = 0.0;
	m_afBBox[1] = 0.0;
	m_afBBox[2] = 400.0;
	m_afBBox[3] = 400.0;

	m_afCurrBox[0] = 0.0;
	m_afCurrBox[1] = 0.0;
	m_afCurrBox[2] = 400.0;
	m_afCurrBox[3] = 400.0;

	if(m_nDebugLevel>0) {
		m_pErrOut=fopen("graphlayout.out","wt");
	} else {
		m_pErrOut=NULL;
	}

	m_nIterArc=0;

}

CGraphLayout::~CGraphLayout()
{
	Clear();
	delete m_pNodeHashTable;

	if(m_pErrOut) {
		fclose(m_pErrOut);
	}
}


void CGraphLayout::AddNode(long id, const wxString &name)
{
	AddNodeInternal(id, name);
}

CGraphNode *CGraphLayout::AddNodeInternal(long id, const wxString &name)
{
	CGraphNode *node;

	node=(CGraphNode *)m_pNodeHashTable->Get(id);
	if(!node) {
		node=new CGraphNode;
		m_nodeArray.push_back(node);

		node->name=name;
		node->id=id;

		m_pNodeHashTable->Put(id, (wxObject *)node);

		LinkNode( &m_pFirstNode, node);
		m_nNumNodes++;
	}

	return node;
}

void CGraphLayout::AddArc(long edge_id, long fromId, long toId, bool hide)
{
	CGraphEdge *edge;
	CGraphNode *fromnode, *tonode;

	fromnode = AddNodeInternal(fromId,wxT(""));
	tonode = AddNodeInternal(toId,wxT(""));

	edge = new CGraphEdge;
	edge->id=edge_id;

	m_arcArray.push_back(edge);

	LinkEdge(edge, fromnode, tonode);

	if (m_nDebugLevel >= 4) {
		Verbose( "read_edge: edge from %s to %s has address %p\n", fromnode->name, tonode->name, edge);
	}
}

void CGraphLayout::DoLayout(void)
{
	if(m_pFirstNode==NULL)
		return;

	PlaceGraph();

/*	if(m_bTransform) {
		SetScale();
		int i,count=m_nodeArray.GetCount();
		for(i=0;i<count;i++) {
			CGraphNode *node = m_nodeArray[i];
			DoScale(&(node->x), &(node->y));
		}
	}
 */
}

void
CGraphLayout::SetNodeSize(float width, float height)
{
    m_fNodeWidth = width;
    m_fNodeHeight = height;
}

void CGraphLayout::Clear(void)
{
	// Delete structures
	size_t i,count=m_nodeArray.size();
	for(i=0;i<count;i++) {
		CGraphNode *node=m_nodeArray[i];
		delete node;
	}
	m_nodeArray.clear();

	if(m_pNodeHashTable) {
		m_pNodeHashTable->Clear();
	}

	count=m_arcArray.size();
	for(i=0;i<count;i++) {
		delete m_arcArray[i];
	}
	m_arcArray.size();


	m_pFirstNode = NULL;

	if (m_pVPlace) delete[] m_pVPlace;
	if (m_pHPlace) delete[] m_pHPlace;
	m_pVPlace = NULL;
	m_pHPlace = NULL;
	m_nNumNodes = 0;
	m_nMaxX = 0;
	m_nMaxY = 0;

	m_afBBox[0] = 0.0;
	m_afBBox[1] = 0.0;
	m_afBBox[2] = 400.0;
	m_afBBox[3] = 400.0;

	for (i = 0; i < 3; i++)
		for (int j = 0; j < 2; j++)
			m_afScale[j][i] = 0.0;
}

void CGraphLayout::Draw(wxDC *pdc)
{
	pdc->Clear();
	DrawArcs(pdc);
	DrawNodes(pdc);
}

void CGraphLayout::DrawNodes(wxDC *pdc)
{
	m_pNodeHashTable->BeginFind();
	wxNode *wxnode = m_pNodeHashTable->Next();
	while (wxnode) {
		CGraphNode *node = (CGraphNode *)wxnode->GetData();
		DrawNode(pdc, node->id);
		wxnode = m_pNodeHashTable->Next();
	}
}

void CGraphLayout::DrawArcs(wxDC *pdc)
{
	size_t i,count=m_arcArray.size();
	for(i=0;i<count;i++) {
		CGraphEdge *edge = m_arcArray[i];
		DrawArc(pdc, edge->from, edge->to);
	}
}

void CGraphLayout::DrawNode(wxDC *pdc, long id)
{
	wxString strName=GetNodeName(id);

	wxCoord x = 20;
	wxCoord y = 20;

//	pdc->GetTextExtent(strName, &x, &y);

	float x1,y1;
	GetNodePosition(id,&x1,&y1);

	x1*=30;
	y1*=30;

	if(m_nDebugLevel>3) {
		Verbose("node %d -> (%f,%f)\n",id,x1,y1);
	}

	float margin = 3;

	pdc->SetBrush(*wxCYAN_BRUSH);
	pdc->SetPen(*wxBLACK_PEN);
	pdc->SetTextBackground(*wxCYAN);
	pdc->DrawRoundedRectangle((float)(x1 - margin), (float)(y1 - (y/2.0f) - margin), (float)(x+2*margin), (float)(y+2*margin), -0.3f);
//	pdc->DrawText(strName, x1, (float)(y1 - (y/2.0f)));
}

void CGraphLayout::DrawArc(wxDC *pdc, CGraphNode *from, CGraphNode *to)
{

	pdc->DrawLine(from->x*30,from->y*30,to->x*30,to->y*30);
}

void CGraphLayout::SetBoundingBox(float x1, float y1, float x2, float y2)
{
	m_afBBox[0] = x1;
	m_afBBox[1] = y1;
	m_afBBox[2] = x2;
	m_afBBox[3] = y2;
}

wxRealPoint CGraphLayout::GetNodePosition(long id)
{
	CGraphNode *node = (CGraphNode *)m_pNodeHashTable->Get(id);
	if (node) {
		return wxRealPoint(node->x,node->y);
	}

	return wxRealPoint(0.0,0.0);
}

wxString CGraphLayout::GetNodeName(long id)
{
	CGraphNode *node = (CGraphNode *)m_pNodeHashTable->Get(id);
	if(node)
		return node->name;
	return wxT("");
}

void CGraphLayout::StartNodeIteration(void)
{
	m_pNodeHashTable->BeginFind();
}

bool CGraphLayout::GetNextNode( wxRealPoint *pPos, long *pId, wxString *pName)
{
	CGraphNode *node;

	wxNode *wxnode = m_pNodeHashTable->Next();
	if(wxnode==NULL)
		return false;

	node = (CGraphNode *)wxnode->GetData();

	if(pPos)
		*pPos=wxRealPoint(node->x,node->y);
	if(pId)
		*pId=node->id;
	if(pName)
		*pName=node->name;

	return true;
}

void CGraphLayout::StartArcIteration(void)
{
	m_nIterArc=0;
}

bool CGraphLayout::GetNextArc(long *pnEdgeId, wxRealPoint *pPosStart, wxRealPoint *pPosEnd, long *pIdFrom, long *pIdTo, std::vector<wxRealPoint> *pCtrlPoints)
{
	CGraphEdge *edge;
	if(m_nIterArc>=m_arcArray.size())
		return false;

	edge=m_arcArray[m_nIterArc];

	m_nIterArc++;

	if(pPosStart)
		*pPosStart=wxRealPoint(edge->from->x,edge->from->y);
	if(pIdFrom)
		*pIdFrom=edge->from->id;

	if(pCtrlPoints) {
		pCtrlPoints->clear();
		list<wxRealPoint>::iterator it = edge->controlpoints.begin();
		while(it != edge->controlpoints.end()) {
			pCtrlPoints->push_back(*it++);
		}
	}

	if(pPosEnd)
		*pPosEnd=wxRealPoint(edge->to->x,edge->to->y);
	if(pIdTo)
		*pIdTo=edge->to->id;

	if(pnEdgeId)
		*pnEdgeId=edge->id;


	return true;
}

static double
distancefromedge(double x1, double y1, double x2, double y2)
{
    double a = x1 - x2;
    a *= a;

    double b = y1 - y2;
    b *= b;

    return a + b;
}

static void
reducecontrolpoints(list<wxRealPoint> *points)
{
	if (points->size() < 2) return;

	double total_len = 0;
	list<double> lengths;

	list<wxRealPoint>::iterator it = points->begin();
	double x = it->x, y = it->y;
	it++;
	while (it != points->end()) {
		double len = sqrt(distancefromedge(x, y, it->x, it->y));
		lengths.push_back(len);
		total_len += len;
		x = it->x;
		y = it->y;
		it++;
	}

	double cumlen = 0.0;
	int cnt = 2;
	int removed = 0;
	it = points->begin();
	it++;
	list<double>::iterator lenit = lengths.begin();
	while (lenit != lengths.end()) {
		if (cumlen + *lenit < total_len * 0.10) {
			cumlen += *lenit++;
			if (lenit != lengths.end()) {
//				printf("removed point %d\n", cnt++);
				it = points->erase(it);
				removed++;
			}

		} else {
			it++;
			lenit++;
			cumlen = 0.0;
		}
	}
/*	printf("removed %d/%d points\n-----------------------------\n",
		   removed, points->size()+removed);
	fflush(stdout);
 */
}

void
CGraphLayout::PlaceGraph()
{
    string out;
	map<string, CGraphNode *> nodes;

	// this lout stuff is because we want the graph to be given to libdot as
	// ordered, not reverse ordered
	list<string> lout;

	// compute the strings to pass to libdot (in dot's language)
	CGraphNode *node = m_pFirstNode;
	while (node) {
		CGraphEdge *edge = node->outedges;
		while (edge) {
			char id1[100];
			char id2[100];
			char buf[200];

			sprintf(id1, "ID0x%x", node->id);
			sprintf(id2, "ID0x%x", edge->to->id);
			sprintf(buf, " [width=\"%f\", height=\"%f\"]; ",
					m_fNodeWidth / 50.0, m_fNodeHeight / 50.0);

			// make our boxes big
			out += id1;
			out += buf;
			out += id2;
			out += buf;

			// make link
			sprintf(buf, "%s -> %s ;\n", id1, id2);
			out += buf;

			// put into list for future reversal
			lout.push_front(out);
			out = "";

			// keep this around so we can update positions after parsing
			nodes[id1] = node;
			nodes[id2] = edge->to;

			edge->drawn = 0;
			edge = edge->next_outedge;
		}
		node = node->next;
	}

	// reverse graph for libdot
    out = "digraph foo {\nnodesep=\"1.0\";\n";
	list<string>::iterator it = lout.begin();
	while (it != lout.end()) out += *it++;
    out += "}";

/*    printf("GENERATED\n<<<<\n%s\n>>>>>\n", out.c_str());
 */

	// let libdot do layout!
	out = GraphLayout::getgraph(out.c_str());

/*    printf("LAYOUT COMPLETE:\n<<<<<\n%s\n>>>>>>>\n", out.c_str());
    fflush(stdout);
 */

	// now to parse libdot's plaintext output
    char *q, *p = (char*)out.c_str();
	int maxx = 0, maxy = 0;
	bool firstedge = 1;
    while (1) {
        q = strtok(p, " ");
        p = NULL;
        if (q == NULL)
            break;

        if (strcmp(q, "graph") == 0) {
            strtok(NULL, " \r\n");
			double w = strtod(strtok(NULL, " \r\n"), NULL);
			double h = strtod(strtok(NULL, " \r\n"), NULL);

        } else if (strcmp(q, "node") == 0) {
            char *name;
            double x;
            double y;

            name = strtok(NULL, " \r\n");
            x = strtod(strtok(NULL, " \r\n"), NULL) * 50.0;
            y = strtod(strtok(NULL, " \r\n"), NULL) * 50.0;
            strtok(NULL, "\n");

			// modify node with laid-out x,y and keep track of maxx,maxy
			node = nodes[name];
			node->x = x;
			node->y = y;
			if (x > maxx) maxx = x;
			if (y > maxy) maxy = y;

//			DPRINTF("placing node %d at %f, %f\n", node->id, x, y);

        } else if (strcmp(q, "edge") == 0) {
			if (firstedge) {
				firstedge = 0;

				m_nMaxX = maxx;
				m_nMaxY = maxy;
//				DPRINTF("maxx(%d) maxy(%d)\n", maxx, maxy);

				// flip graph upsidedown
				node = m_pFirstNode;
				while (node) {
					node->y = maxy - node->y;
					node = node->next;
				}
			}

			char *p1;
			char *p2;

			p1 = strtok(NULL, " \r\n");
			p2 = strtok(NULL, " \r\n");
			int n = atoi(strtok(NULL, " \r\n"));

			CGraphNode *n1 = nodes[p1];
			CGraphNode *n2 = nodes[p2];

			// get first non-drawn edge
			CGraphEdge *edge = n1->outedges;
			while (edge) {
				if (edge->to == n2 && edge->drawn == 0) {
					edge->drawn = 1;
					break;
				}
				edge = edge->next_outedge;
			}

			// add the control points to the edge
			if (edge) {
				bool flip = 0;
				list<wxRealPoint> points;

				// add control points to temp array because we might need to
				// flip the order of the points if libdot screwed up
				while (n--) {
					double x = strtod(strtok(NULL, " \r\n"), NULL) * 50.0;
					double y = strtod(strtok(NULL, " \r\n"), NULL) * 50.0;
					y = maxy - y; // graph was flipped upside down

					points.push_back(wxRealPoint(x, y));
				}

				// find distance from control point edges to first point so we
				// can correct if libdot returns us points in the wrong order
				double dist1 = distancefromedge(n1->x, n1->y,
												points.begin()->x,
												points.begin()->y);
				double dist2 = distancefromedge(n1->x, n1->y,
												points.rbegin()->x,
												points.rbegin()->y);

				// flip or not?
				if (dist1 < dist2) {
				    edge->controlpoints.push_back(wxRealPoint(n1->x, n1->y));
					list<wxRealPoint>::iterator it = points.begin();
					while (it != points.end())
						edge->controlpoints.push_back(*it++);
				    edge->controlpoints.push_back(wxRealPoint(n2->x, n2->y));

				} else {
				    edge->controlpoints.push_back(wxRealPoint(n2->x, n2->y));
					list<wxRealPoint>::reverse_iterator it = points.rbegin();
					while (it != points.rend())
						edge->controlpoints.push_back(*it++);
				    edge->controlpoints.push_back(wxRealPoint(n1->x, n1->y));
				}

				reducecontrolpoints(&edge->controlpoints);
				edge->controlpoints.erase(edge->controlpoints.begin());
				edge->controlpoints.erase(--(edge->controlpoints.end()));
			}
            strtok(NULL, "\n");

        } else if (strcmp(q, "stop") == 0) {
            break;
        }
    }
}


void CGraphLayout::Verbose( char *fmt, ...)
{
	va_list args;

	va_start( args, fmt);
	vfprintf( m_pErrOut, fmt, args);
	va_end( args);
}

void CGraphLayout::SetScale()
{
	/* graph is originally placed with */
	/* 0 <= x <= m_nMaxX, right positive, unit distance 'DIST' */
	/* 0 <= y <= m_nMaxY, down positive, unit distance 1. */
	/* now setup a transformation matrix to m_afScale this to */
	/* the requested bounding box m_afBBox[4] = {xll, yll, xur, yur} */

	m_afScale[0][0] = 1.0;
	m_afScale[1][1] = 1.0;
	m_afCurrBox[2] = m_nMaxX;
	m_afCurrBox[3] = m_nMaxY;

	/* Turn original y-placement to unit distances in up+ coords */
	//	MultY( -DIST);

	/* perform requested rotation */
	if (m_nRotation) Rotate( m_nRotation);

	float mx, my;

	/* prevent division by 0 */
	if (0.0 == m_afCurrBox[2] - m_afCurrBox[0]) {
		m_afCurrBox[0] -= (float)(0.5*DIST);
		m_afCurrBox[2] += (float)(0.5*DIST);
	}

	if (0.0 == m_afCurrBox[3] - m_afCurrBox[1]) {
		m_afCurrBox[1] -= (float)(0.5*DIST);
		m_afCurrBox[3] += (float)(0.5*DIST);
	}

	mx = (m_afBBox[2] - m_afBBox[0])/(m_afCurrBox[2] - m_afCurrBox[0]);
	my = (m_afBBox[3] - m_afBBox[1])/(m_afCurrBox[3] - m_afCurrBox[1]);

	//mm = (float)(margin/100.0);
	//mm = 0.0;

	//mx *= (float)(1.0 - 2.0*mm);
	//my *= (float)(1.0 - 2.0*mm);

	if (!m_bFlexScale)
	{
		if (mx > my)
			mx = my;
		else
			my = mx;
	}

	/* now transform to m_afBBox */
	/* sizing */
	MultX(mx);
	MultY(my);

	/* move to lower-left = 0,0 */
	Translate( m_afBBox[0] - m_afCurrBox[0], m_afBBox[1] - m_afCurrBox[1]);

	/* move to requested margin */
	//Translate( mm*(m_afBBox[2]-m_afBBox[0]), mm*(m_afBBox[3]-m_afBBox[1]));

	Translate(m_afBBox[0], m_afBBox[1]);


	if (m_nDebugLevel >= 2)
	{	Verbose( "Transformation matrix is:  %g %g %g\n",
	m_afScale[0][0], m_afScale[0][1], m_afScale[0][2]);
	Verbose( "                           %g %g %g\n",
		m_afScale[1][0], m_afScale[1][1], m_afScale[1][2]);
	Verbose( "Graph m_afBBox after scaling is %g %g %g %g\n",
		m_afCurrBox[0], m_afCurrBox[1], m_afCurrBox[2], m_afCurrBox[3]);
	}
}

void CGraphLayout::DoScale(float *x, float *y)
{
	float xin = *x;
	float yin = *y;

	*x = (float)(m_afScale[0][0] * xin + m_afScale[0][1] * yin + m_afScale[0][2]);
	*y = (float)(m_afScale[1][0] * xin + m_afScale[1][1] * yin + m_afScale[1][2]);
}

void CGraphLayout::MultY( float f)
{
	int i;

	for (i=0; i<3; i++)
		m_afScale[1][i] *= f;

	m_afCurrBox[1] *= f;
	m_afCurrBox[3] *= f;
	if (f < 0.0) Exch( m_afCurrBox[1], m_afCurrBox[3]);
}

void CGraphLayout::MultX( float f)
{
	int i;
	for (i=0; i<3; i++)
		m_afScale[0][i] *= f;

	m_afCurrBox[0] *= f;
	m_afCurrBox[2] *= f;
	if (f < 0.0) Exch( m_afCurrBox[0], m_afCurrBox[2]);
}

void CGraphLayout::Translate( float dx, float dy)
{
	m_afScale[0][2] += dx;
	m_afScale[1][2] += dy;

	m_afCurrBox[0] += dx;
	m_afCurrBox[2] += dx;
	m_afCurrBox[1] += dy;
	m_afCurrBox[3] += dy;
}

void CGraphLayout::Rotate( int n)
{	/* Rotate clockwise over n*90 degrees */
	int j;
	float h;

	/* transform n to be within 0 <= n <= 3 */
	if (n<0)
	{	MultY( -1);
	n = -n;
	}
	n = n & 3;

	while (n--)
	{	/* Rotate over 90 degrees */
		for (j=0; j<3; j++)
		{	Exch( m_afScale[0][j], m_afScale[1][j]);
		m_afScale[0][j] *= -1;
		}

		h = m_afCurrBox[0];
		m_afCurrBox[0] = - m_afCurrBox[3];
		m_afCurrBox[3] = m_afCurrBox[2];
		m_afCurrBox[2] = - m_afCurrBox[1];
		m_afCurrBox[1] = h;
	}
}

void CGraphLayout::LinkNode(CGraphNode **listp, CGraphNode *node)
{
	(node)->next = *(listp);
	if (*(listp))
		(*(listp))->prevp = &((node)->next);
	*(listp) = node;
	(node)->prevp = listp;
}

void CGraphLayout::UnlinkNode(CGraphNode *node)
{
	if ((node)->next)
		(node)->next->prevp = (node)->prevp;
	if ((node)->prevp)
		*((node)->prevp) = (node)->next;
	(node)->next = NULL;
	(node)->prevp = NULL;
}

void CGraphLayout::LinkEdge(CGraphEdge *edge, CGraphNode *fromnode, CGraphNode *tonode)
{
	edge->from = fromnode;
	edge->to = tonode;
	edge->next_outedge = (fromnode)->outedges;
	(fromnode)->outedges = edge;
	edge->next_inedge = (tonode)->inedges;
	(tonode)->inedges = edge;
}

void CGraphLayout::UnlinkEdge(CGraphEdge *edge)
{
	CGraphEdge **Ep;
	for (Ep=&((edge)->from->outedges); *Ep; Ep=&((*Ep)->next_outedge))
		if (*Ep == edge) break;
		if (*Ep) *Ep = (edge)->next_outedge;
		for (Ep=&((edge)->to->inedges); *Ep; Ep=&((*Ep)->next_inedge))
			if (*Ep == edge) break;
			if (*Ep) *Ep = (edge)->next_inedge;
			(edge)->to = NULL;
			(edge)->from = NULL;
}
