#include"graphview_pch.h"
#include "grapheventhandler.h"

CGraphEventHandler::CGraphEventHandler(wxShapeEvtHandler *prev, wxShape *shape, CGraphDescriptor *pgd):wxShapeEvtHandler(prev, shape)
{
	m_pGraphDescriptor=pgd;
}

CGraphEventHandler::~CGraphEventHandler()
{
}

void CGraphEventHandler::CopyData(wxShapeEvtHandler& copy)
{
	((CGraphEventHandler &)copy).m_pGraphDescriptor=m_pGraphDescriptor;
}

/*
* CGraphEventHandler: an event handler class for all shapes
*/

void CGraphEventHandler::OnDraw(wxDC& dc)
{
	if(GetPreviousHandler())
		GetPreviousHandler()->OnDraw(dc);

	if(m_pGraphDescriptor) {
		double w, h;
		GetShape()->GetBoundingBoxMax(&w,&h);
		m_pGraphDescriptor->ExtraDrawing(GetShape()->GetId(),&dc,
			GetShape()->GetX(),GetShape()->GetY(),
			(int)w,(int)h);
		
	}
}


void CGraphEventHandler::OnSize(double x, double y)
{
	if(GetPreviousHandler())
		GetPreviousHandler()->OnSize(x,y);

	GetShape()->GetCanvas()->GetDiagram()->SetModified(true);
}

bool CGraphEventHandler::OnMovePre(wxDC& dc, double x, double y, double old_x, double old_y, bool display)
{
	GetShape()->GetCanvas()->GetDiagram()->SetModified(true);

	if(GetPreviousHandler())
		return GetPreviousHandler()->OnMovePre(dc,x,y,old_x,old_y,display);	

	return TRUE;
}



void CGraphEventHandler::OnLeftClick(double x, double y, int keys, int attachment)
{
	wxClientDC dc(GetShape()->GetCanvas());
	GetShape()->GetCanvas()->PrepareDC(dc);
	
	if (keys == 0) {
		// Selection is a concept the library knows about
		if (GetShape()->Selected()) {
			GetShape()->Select(FALSE, &dc);
			GetShape()->GetCanvas()->Redraw(dc); // Redraw because bits of objects will be are missing
		} else {
			// Ensure no other shape is selected
			bool redraw = FALSE;
			wxNode *node = GetShape()->GetCanvas()->GetDiagram()->GetShapeList()->GetFirst();
			while (node) {
				wxShape *eachShape = (wxShape *)node->GetData();
				if (eachShape->GetParent() == NULL) {
					if (eachShape->Selected()) {
						eachShape->Select(FALSE, &dc);
						redraw = TRUE;
					}
				}
				node = node->GetNext();
			}
			GetShape()->Select(TRUE, &dc);
			if (redraw)
				GetShape()->GetCanvas()->Redraw(dc);
		}
	} else if (keys & KEY_CTRL) {
		// Do something for CONTROL
	}
}

