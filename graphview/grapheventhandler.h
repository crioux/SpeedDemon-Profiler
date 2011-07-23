#ifndef __INC_GRAPHEVENTHANDLER_H
#define __INC_GRAPHEVENTHANDLER_H

#include"ogl.h"
#include"graphdescriptor.h"

/*
* All shape event behaviour is routed through this handler, so we don't
* have to derive from each shape class. We plug this in to each shape.
*/

class CGraphEventHandler: public wxShapeEvtHandler
{
public:
	CGraphDescriptor *m_pGraphDescriptor;
	int m_nNodeId;
	
	CGraphEventHandler(wxShapeEvtHandler *prev = NULL, wxShape *shape = NULL, CGraphDescriptor *pgd=NULL);
	~CGraphEventHandler(void);
	
	virtual void CopyData(wxShapeEvtHandler& copy);

	void OnDraw(wxDC& dc);
	void OnLeftClick(double x, double y, int keys = 0, int attachment = 0);
	void OnSize(double x, double y);
	bool OnMovePre(wxDC& dc, double x, double y, double old_x, double old_y, bool display);
};

#endif