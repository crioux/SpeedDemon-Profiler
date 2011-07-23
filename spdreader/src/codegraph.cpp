#include"spdreader_pch.h"
#include"codegraph.h"
#include"spdreaderdoc.h"
#include"funcgraphdesc.h"

CCodeGraph::CCodeGraph(wxWindow *parent, CSPDReaderDoc *pDoc, CSPDReaderView *pMainView):
	CGraphView(parent)
{
	m_pDocument=pDoc;
	m_pMainView=pMainView;
	m_nSelectedFunction=-1;
}

void CCodeGraph::UpdateData(void)
{
	((CFuncGraphDesc *)m_pGraphDescriptor)->Build();
	AutoLayoutDiagram();
}

void CCodeGraph::SetSelectedFunction(int funcnum)
{
	m_nSelectedFunction=funcnum;
	Update();
}
