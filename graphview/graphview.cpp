#include"graphview_pch.h"

#include"graphview.h"
#include"graphlayout.h"
#include"graphdescriptor.h"
#include"grapheventhandler.h"

//#include"wx/wxexpr.h"


BEGIN_EVENT_TABLE(CGraphView,wxShapeCanvas)
	EVT_MOUSE_EVENTS(CGraphView::OnMouseEvent)
	EVT_ACTIVATE(CGraphView::OnActivate)
	EVT_SIZE(CGraphView::OnSize)
END_EVENT_TABLE()

IMPLEMENT_DYNAMIC_CLASS(CNodeEvent,wxCommandEvent)
DEFINE_EVENT_TYPE(GV_NODE_SELECTED)
DEFINE_EVENT_TYPE(GV_NODE_ACTIVATED)
DEFINE_EVENT_TYPE(GV_NODE_MENU)

CGraphView::CGraphView(wxWindow *parent, wxWindowID id, const wxPoint& pos, const wxSize& size,
					   long style):wxShapeCanvas(parent,id,pos,size,style)
{
	m_diag.SetCanvas(this);
	SetDiagram(&m_diag);

	m_pbrGreen=new wxBrush(wxColour(144,219,63),wxSOLID);
	m_pbrOffWhite=new wxBrush(wxColour(255,251,236),wxSOLID);
	m_pbrOffWhiteDarker=new wxBrush(wxColour(255,240,183),wxSOLID);
	m_pbrRed=new wxBrush(wxColour(229,78,58),wxSOLID);
	m_pbrYellow=new wxBrush(wxColour(247,230,111),wxSOLID);
	m_pbrBlue=new wxBrush(wxColour(105,160,234),wxSOLID);
	m_pbrOrange=new wxBrush(wxColour(255,158,61),wxSOLID);
	m_pbrPurple=new wxBrush(wxColour(171,104,204),wxSOLID);
	m_pbrPink=new wxBrush(wxColour(255,132,214),wxSOLID);

	m_fntText=new wxFont(8,wxSWISS,wxNORMAL,wxNORMAL,false,wxT("Arial"));
	m_penDot=new wxPen(wxColour(0,0,0),1,wxDOT);

#ifdef WIN32
	m_pCurArrow=new wxCursor((const wxString &)"arrow.cur",(long)wxBITMAP_TYPE_CUR,1,1);
	m_pCurHand=new wxCursor((const wxString &)"hand.cur",(long)wxBITMAP_TYPE_CUR,8,8);
	m_pCurHandClosed=new wxCursor((const wxString &)"handclosed.cur",(long)wxBITMAP_TYPE_CUR,8,8);
	m_pCurFinger=new wxCursor((const wxString &)"finger.cur",(long)wxBITMAP_TYPE_CUR,12,3);
	m_pCurMagnifier=new wxCursor((const wxString &)"magnifier.cur",(long)wxBITMAP_TYPE_CUR,7,8);
#else
#endif

	m_nMode=HAND;
	m_bGrabbing=false;
	m_bZooming=false;
	m_pHoverShape=NULL;
	m_pClickShape=NULL;

	SetBackgroundColour(wxColour(255,255,255));
	
	m_pGraphDescriptor=NULL;
}

CGraphView::~CGraphView()
{
	delete m_pCurMagnifier;
	delete m_pCurHandClosed;
	delete m_pCurHand;
	delete m_pCurFinger;
	delete m_pCurArrow;
	delete m_fntText;
	delete m_pbrGreen;
	delete m_pbrOffWhite;
	delete m_pbrOffWhiteDarker;
	delete m_pbrRed;
	delete m_pbrYellow;
	delete m_pbrBlue;
	delete m_pbrOrange;
	delete m_pbrPurple;
	delete m_pbrPink;
	delete m_penDot;

	m_diag.DeleteAllShapes();
}

void CGraphView::UpdateCursor(void)
{
	wxCursor *cur=wxSTANDARD_CURSOR;
	switch(m_nMode) {
	case HAND:
		if(m_bGrabbing) {
			cur=m_pCurHandClosed;
		} else if(m_bZooming) {
			cur=m_pCurMagnifier;
		} else {
			if(m_pHoverShape) {
				cur=m_pCurFinger;
			} else {
				cur=m_pCurHand;
			}
		}
		break;
	case ARROW:
		if(m_bZooming) {
			cur=m_pCurMagnifier;
		} else {
			cur=m_pCurArrow;
		}
		break;
	}
	SetCursor(*cur);
}

void CGraphView::SetCursorMode(CURSORMODE cm)
{
	m_nMode=cm;
	UpdateCursor();
}

bool CGraphView::SetGraphDescriptor(CGraphDescriptor *pgd)
{
	m_pGraphDescriptor=pgd;
	m_diag.DeleteAllShapes();
	return true;
}

bool CGraphView::AutoLayoutDiagram(void)
{
	int cw,ch;
	GetClientSize(&cw,&ch);
	
	wxClientDC dc(this);

	// Clear old graph
	m_diag.DeleteAllShapes();
	
	if(m_pGraphDescriptor==NULL) {
		Refresh(true);
		return true;
	}

	// Build new graph layout
	CGraphLayout gl;
	gl.SetTransform(false);
	gl.SetRouteEdges(false);

	// Add all nodes
	long nNodeId;
	wxString strName,strText;
	CGraphDescriptor::NODETYPE nodeType;
	std::list<int> ilNodeIds;

	m_pGraphDescriptor->StartNodeIteration();
	while(m_pGraphDescriptor->GetNextNode(&nNodeId)) {
		m_pGraphDescriptor->GetNodeInfo(nNodeId,&strName,&strText,&nodeType);
		
		gl.AddNode(nNodeId,strName);
		ilNodeIds.push_back(nNodeId);
	}

	// Add all edges
	for(std::list<int>::iterator iter=ilNodeIds.begin();iter!=ilNodeIds.end();iter++)
	{
		int id=*iter;
		
		m_pGraphDescriptor->StartOutEdgeIteration(id);

		long nEdgeId;
		long nNodeTo;
		wxString strLabel;
		CGraphDescriptor::LINETYPE lineType;

		while(m_pGraphDescriptor->GetNextOutEdge(&nEdgeId)) {
			m_pGraphDescriptor->GetEdgeInfo(nEdgeId,NULL,&nNodeTo, &strLabel, &lineType);

			gl.AddArc(nEdgeId,id,nNodeTo);
		}
	}
	
	// Lay out graph
	int shape_w,shape_h;
	shape_w=130;
	shape_h=100;
	gl.SetNodeSize(shape_w,shape_h);
	gl.DoLayout();

	// Get dimensions and shape size
	int graph_w,graph_h;
	gl.GetGraphSize(&graph_w,&graph_h);

	// Now add shapes to diagram
	gl.StartNodeIteration();
	wxRealPoint pos;

	while(gl.GetNextNode(&pos,&nNodeId,&strName)) {			
		wxString text;
		wxString body;
		CGraphDescriptor::NODETYPE nodetype;
		m_pGraphDescriptor->GetNodeInfo(nNodeId,&text,&body,&nodetype);

		wxShape *shape;
		switch(nodetype) {
		case CGraphDescriptor::RECTANGLE:
			shape=new wxRectangleShape(shape_w,shape_h);
			shape->SetBrush(m_pbrBlue);
			break;
		case CGraphDescriptor::CIRCLE:
			shape=new wxCircleShape(shape_h);
			shape->SetBrush(m_pbrOffWhite);
			break;
		case CGraphDescriptor::SELECTEDCIRCLE:
			shape=new wxCircleShape(shape_h);
			shape->SetBrush(m_pbrOffWhiteDarker);
			break;
		case CGraphDescriptor::ELLIPSE:
			shape=new wxEllipseShape(shape_w,shape_h);
			shape->SetBrush(m_pbrGreen);
			break;
		case CGraphDescriptor::TRIANGLE:
			{
				wxList *pts=new wxList;
				pts->Append((wxObject *)new wxRealPoint(0.0f,-((float)shape_h)/2.0f));
				pts->Append((wxObject *)new wxRealPoint(((float)shape_w)/2.0f,((float)shape_h)/2.0f));
				pts->Append((wxObject *)new wxRealPoint(-((float)shape_w)/2.0f,((float)shape_h)/2.0f));
				shape=new wxPolygonShape();
				((wxPolygonShape *)shape)->Create(pts);
				shape->SetBrush(m_pbrYellow);
			}
			break;
		case CGraphDescriptor::DIAMOND:
			{
				wxList *pts=new wxList;
				pts->Append((wxObject *)new wxRealPoint(0.0f,-((float)shape_h)/2.0f));
				pts->Append((wxObject *)new wxRealPoint(((float)shape_w)/2.0f,0.0f));
				pts->Append((wxObject *)new wxRealPoint(0.0f,((float)shape_h)/2.0f));
				pts->Append((wxObject *)new wxRealPoint(-((float)shape_w)/2.0f,0.0f));
				shape=new wxPolygonShape();
				((wxPolygonShape *)shape)->Create(pts);
				shape->SetBrush(m_pbrOrange);
			}
			break;
		case CGraphDescriptor::RHOMBUS:
			{
				wxList *pts=new wxList;
				pts->Append((wxObject *)new wxRealPoint(-((float)shape_w)/2.0f*3.0f/4.0f,-((float)shape_h)/2.0f));
				pts->Append((wxObject *)new wxRealPoint(((float)shape_w)/2.0f,-((float)shape_h)/2.0f));
				pts->Append((wxObject *)new wxRealPoint(((float)shape_w)/2.0f*3.0f/4.0f,((float)shape_h)/2.0f));
				pts->Append((wxObject *)new wxRealPoint(-((float)shape_w)/2.0f,((float)shape_h)/2.0f));
				shape=new wxPolygonShape();
				((wxPolygonShape *)shape)->Create(pts);
				shape->SetBrush(m_pbrPurple);
			}
			break;
		case CGraphDescriptor::TRAPEZOID:
			{
				wxList *pts=new wxList;
				pts->Append((wxObject *)new wxRealPoint(-((float)shape_w)/2.0f*2.0f/4.0f,-((float)shape_h)/2.0f));
				pts->Append((wxObject *)new wxRealPoint( ((float)shape_w)/2.0f*2.0f/4.0f,-((float)shape_h)/2.0f));
				pts->Append((wxObject *)new wxRealPoint(((float)shape_w)/2.0f,((float)shape_h)/2.0f));
				pts->Append((wxObject *)new wxRealPoint(-((float)shape_w)/2.0f,((float)shape_h)/2.0f));
				shape=new wxPolygonShape();
				((wxPolygonShape *)shape)->Create(pts);
				shape->SetBrush(m_pbrPink);
			}
			break;
		}	
		
		shape->SetEventHandler(new CGraphEventHandler(shape->GetEventHandler(),shape,m_pGraphDescriptor));
		shape->SetFont(m_fntText);
		shape->FormatText(dc,text);
		
		shape->SetX( pos.x );
		shape->SetY( pos.y );
		shape->SetId(nNodeId);

		shape->Show(true);
		m_diag.AddShape(shape);
	}

	// Now add edges to diagram
	gl.StartArcIteration();

	wxRealPoint start,end;
	long idfrom,idend,edge_id;
	std::vector<wxRealPoint> ctrlpts;

	while(gl.GetNextArc(&edge_id,&start,&end,&idfrom,&idend,&ctrlpts)) {
		wxShape *fromshape=m_diag.FindShape(idfrom);
		wxShape *toshape=m_diag.FindShape(idend);

		// Get line information
		CGraphDescriptor::LINETYPE linetype;
		wxString strLabel;
		m_pGraphDescriptor->GetEdgeInfo(edge_id,NULL,NULL,&strLabel,&linetype);

		// Add line to diagram
		wxLineShape *pls=new wxLineShape();
		pls->SetEventHandler(new CGraphEventHandler(pls->GetEventHandler(),pls));
		m_diag.AddShape(pls);
		fromshape->AddLine(pls,toshape);

		// Add control points
		size_t count=ctrlpts.size();
        	
		if(count>0) {
			
			// Make bezier with remaining points
			pls->MakeLineControlPoints((int)(2+count));
			wxList *pList=pls->GetLineControlPoints();
			size_t i;
			for(i=0;i<count;i++) {
				wxRealPoint *prp=(wxRealPoint *)(pList->Item(1+i)->GetData());
				prp->x=ctrlpts[i].x;
				prp->y=ctrlpts[i].y;
				
			}
			pls->SetSpline(true);
		
		} else {
			pls->MakeLineControlPoints(2);
		}

		// Set line style
		pls->SetBrush(wxBLACK_BRUSH);
		pls->AddArrow(ARROW_ARROW, ARROW_POSITION_END, 10.0, 0.0, wxT("Normal arrowhead"));
		pls->SetFont(m_fntText);
		pls->SetFormatMode(FORMAT_SIZE_TO_CONTENTS);
		pls->FormatText(dc,strLabel);
		pls->SetId(edge_id);
		pls->SetEventHandler(new CGraphEventHandler(pls->GetEventHandler(),pls,m_pGraphDescriptor));
		
		if(linetype==CGraphDescriptor::REGULAR) {
			pls->SetPen(wxBLACK_PEN);
		} if(linetype==CGraphDescriptor::DOTTED) {
			pls->SetPen(m_penDot);
		}

		// Initialize and show
		pls->Initialise();
		pls->Show(true);
	}

/*
	double zoom=m_diag.GetZoomFactor();
	double minx,miny,maxx,maxy;
	m_diag.GetDiagramExtents(minx,miny,maxx,maxy);

	// Readjust diagram
	if((maxx-minx)>=cw) {
		m_diag.TranslateAllShapes(-minx,0);
	} else {
		m_diag.TranslateAllShapes(-(minx-(cw/2-((maxx-minx)/2))),0);
	}
	
	if((maxy-miny)>=ch) {
		m_diag.TranslateAllShapes(0,-miny);
	} else {
		m_diag.TranslateAllShapes(0,-(miny-(ch/2-((maxy-miny)/2))));
	}

	int gw=(int)((maxx-minx)*zoom);
	int gh=(int)((maxy-miny)*zoom);
	int sx=gw/2;
	int sy=gh/2;

	SetScrollbars(1,1,gw,gh,sx,sy,true);*/

	wxSizeEvent wxse(GetClientSize());
	OnSize(wxse);

	return true;
}

/*bool CGraphView::LoadDiagramLayout(wxExprDatabase *pdb)
{
	// Clear old graph
	m_diag.DeleteAllShapes();
	
	if(m_pGraphDescriptor==NULL) {
		Refresh(true);
		return true;
	}

	if(!m_diag.SerializeFromDatabase(pdb))
		return false;
	
	// Set up event handlers
	int i,count=m_diag.GetShapeList()->GetCount();
	for(i=0;i<count;i++) {
		wxShape *shape=(wxShape *)(m_diag.GetShapeList()->Item(i)->GetData());
		shape->SetEventHandler(new CGraphEventHandler(shape->GetEventHandler(),shape,m_pGraphDescriptor));
		shape->SetFont(m_fntText);
	}

	
	pdb->BeginFind();
	wxExpr *expr=pdb->FindClauseByFunctor("graphview");
	
	long gw,gh,sx,sy;
	expr->GetAttributeValue("gw",gw);
	expr->GetAttributeValue("gh",gh);
	expr->GetAttributeValue("sx",sx);
	expr->GetAttributeValue("sy",sy);
	
	SetScrollbars(1,1,gw,gh,sx,sy,false);
	Refresh(true);

	return true;
}


bool CGraphView::SaveDiagramLayout(wxExprDatabase *pdb)
{
	int gw,gh,sx,sy;
	GetVirtualSize(&gw,&gh);
	GetViewStart(&sx,&sy);

	pdb->ClearDatabase();
	m_diag.SerializeToDatabase(pdb);

	wxExpr *expr=new wxExpr("graphview");
	expr->AddAttributeValue("gw",(long)gw);
	expr->AddAttributeValue("gh",(long)gh);
	expr->AddAttributeValue("sx",(long)sx);
	expr->AddAttributeValue("sy",(long)sy);
	
	pdb->Append(expr);

	return true;
}
*/
void CGraphView::OnActivate(wxActivateEvent &event)
{
	UpdateCursor();
	event.Skip();
}

void CGraphView::OnSize(wxSizeEvent &event)
{
//	Beep(256,200);

	event.Skip();

	int cw=event.GetSize().GetWidth();
	int ch=event.GetSize().GetHeight();
	
	double zoom=m_diag.GetZoomFactor();
	double minx,miny,maxx,maxy;
	m_diag.GetDiagramExtents(minx,miny,maxx,maxy);

	// Readjust diagram
	int gw=(int)((maxx-minx)*zoom);
	int gh=(int)((maxy-miny)*zoom);

	if(gw>cw) {
		m_diag.TranslateAllShapes(-minx,0);
	} else {
		m_diag.TranslateAllShapes(-(minx-(cw/2/zoom-((maxx-minx)/2))),0);
	}
	
	if(gh>ch) {
		m_diag.TranslateAllShapes(0,-miny);
	} else {
		m_diag.TranslateAllShapes(0,-(miny-(ch/2/zoom-((maxy-miny)/2))));
	}

	
	int sx,sy;
	GetViewStart(&sx,&sy);

	SetScrollbars(1,1,gw,gh,sx,sy,true);
	Refresh(true);
}

void CGraphView::OnMouseEvent(wxMouseEvent &event)
{
	if(m_nMode==ARROW) {
		wxShapeCanvas::OnMouseEvent(event);
		return;
	}

	wxClientDC dc(this);
	PrepareDC(dc);
	int ch,cw;
	dc.GetSize(&cw,&ch);
	
	wxPoint logPos(event.GetLogicalPosition(dc));
	
	double x, y;
	x = (double) logPos.x;
	y = (double) logPos.y;
	
	int keys = 0;
	if (event.ShiftDown())
		keys = keys | KEY_SHIFT;
	if (event.ControlDown())
		keys = keys | KEY_CTRL;
	
	bool dragging = event.Dragging();
	
	
    // Find the nearest object
	if(event.IsButton()) {
		if(event.RightDown()) {
			
			if(m_pHoverShape) {
				CNodeEvent evt;
				evt.SetEventType(GV_NODE_MENU);
				evt.SetNode(m_pHoverShape->GetId());
				evt.SetPosition(event.GetPosition());
				AddPendingEvent(evt);
			}

		} else if(event.LeftDClick()) {

			if(m_pHoverShape) {

				CNodeEvent evt;
				evt.SetEventType(GV_NODE_ACTIVATED);
				evt.SetNode(m_pHoverShape->GetId());
				evt.SetPosition(event.GetPosition());
				AddPendingEvent(evt);
			}
			
		} else if(event.LeftDown()) {
			
			if(m_pHoverShape) {
				m_pClickShape=m_pHoverShape;
				return;

			} else {
				m_pClickShape=NULL;
			
				if(event.ShiftDown()) {
					//m_bZooming=true;
					//m_ptGrab=event.GetPosition();
					//UpdateCursor();
					//CaptureMouse();
				} else {
					m_bGrabbing=true;
					m_ptGrab=event.GetPosition();
					int vx,vy;
					GetViewStart(&vx,&vy);
					m_ptGrabLogical=wxPoint(vx,vy);
					UpdateCursor();
					CaptureMouse();
				}
				return;
			}
		} else if(event.LeftUp()) {

			if(m_pClickShape && (m_pClickShape==m_pHoverShape)) {	
				CNodeEvent evt;
				evt.SetEventType(GV_NODE_SELECTED);
				evt.SetNode(m_pHoverShape->GetId());
				evt.SetPosition(event.GetPosition());
				AddPendingEvent(evt);
				return;
			}

			if(m_bGrabbing || m_bZooming) {
				ReleaseMouse();
			}
			m_bZooming=false;
			m_bGrabbing=false;
			UpdateCursor();

		}
		
//		if(nearest_object) {
		

//		}

		return;
	} else if(event.Moving()) {
	
		if(m_bGrabbing) {
			int nx,ny;
			event.GetPosition(&nx,&ny);
			nx=nx-m_ptGrab.x;
			ny=ny-m_ptGrab.y;
			
			int vx,vy;
			vx=m_ptGrabLogical.x-nx;
			vy=m_ptGrabLogical.y-ny;

			Scroll(vx,vy);
			UpdateCursor();
		} else if(m_bZooming) {
		/*	int nx,ny;
			event.GetPosition(&nx,&ny);
			nx=nx-m_ptGrab.x;
			ny=ny-m_ptGrab.y;
			
			double scale;
			if(ny>0.0) {
				scale=1.0+3.0*(ny/(ch/2.0));
			} else {
				scale=1.0-0.75*(-ny/(ch/2.0));
			}
			
			int vsx,vsy;
			GetViewStart(&vsx,&vsy);
				
			vsx=vsx*scale/m_diag.GetZoomFactor();
			vsy=vsy*scale/m_diag.GetZoomFactor();

			m_diag.SetZoomFactor(scale);
			SetScrollbars(1,1,m_nGraphWidth*scale,m_nGraphHeight*scale,vsx,vsy,true);
			Refresh(true);
			*/
		} else {

			int attachment=0;
			wxShape *shape=FindShape(x,y,&attachment,NULL,NULL,CLASSINFO(wxLineShape));
			if(shape!=m_pHoverShape) {
				m_pHoverShape=shape;
				UpdateCursor();
			}
			

		}

	} else if(event.Entering()) {
		UpdateCursor();	
	}

	

}

void CGraphView::SetZoom(double zoom)
{	
	double oldzoom=m_diag.GetZoomFactor();
	m_diag.SetZoomFactor(zoom);

	int vsx,vsy;
	GetViewStart(&vsx,&vsy);

	double minx,miny,maxx,maxy;
	m_diag.GetDiagramExtents(minx,miny,maxx,maxy);

	int gw=(int)((maxx-minx)*zoom);
	int gh=(int)((maxy-miny)*zoom);
	int sx=(int)(((double)vsx)*(zoom/oldzoom));
	int sy=(int)(((double)vsy)*(zoom/oldzoom));
	
	SetScrollbars(1,1,gw,gh,sx,sy,true);

	wxSizeEvent wxse(GetClientSize());
	OnSize(wxse);
}

double CGraphView::GetZoom(void)
{
	return m_diag.GetZoomFactor();
}
