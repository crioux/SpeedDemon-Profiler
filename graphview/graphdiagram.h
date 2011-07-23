#ifndef __INC_GRAPHDIAGRAM_H
#define __INC_GRAPHDIAGRAM_H

#include"ogl.h"

class CGraphDiagram: public wxDiagram
{
	DECLARE_DYNAMIC_CLASS(CGraphDiagram)

protected:

	double m_fZoomFactor;

public:
	
	CGraphDiagram();
	virtual ~CGraphDiagram();
	virtual void Redraw(wxDC& dc);
	
	void SetZoomFactor(double fZoomFactor);
	double GetZoomFactor(void);
};

#endif