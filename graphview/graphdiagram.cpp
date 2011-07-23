#include"graphview_pch.h"
#include "GraphDiagram.h"

IMPLEMENT_DYNAMIC_CLASS(CGraphDiagram,wxDiagram)

CGraphDiagram::CGraphDiagram()
{
	m_fZoomFactor=1.0;
}

CGraphDiagram::~CGraphDiagram()
{

}

void CGraphDiagram::Redraw(wxDC& dc)
{
	dc.SetUserScale(m_fZoomFactor,m_fZoomFactor);
	wxDiagram::Redraw(dc);
}

void CGraphDiagram::SetZoomFactor(double fZoomFactor)
{
	m_fZoomFactor=fZoomFactor;
}

double CGraphDiagram::GetZoomFactor(void)
{
	return m_fZoomFactor;
}
