#ifndef __INC_CODEGRAPH_H
#define __INC_CODEGRAPH_H

class CSPDReaderDoc;
class CSPDReaderView;

class CCodeGraph:public CGraphView
{
private:

	CSPDReaderDoc *m_pDocument;
	CSPDReaderView *m_pMainView;
	int m_nSelectedFunction;

public:
	CCodeGraph(wxWindow *parent, CSPDReaderDoc *pDoc, CSPDReaderView *pMainView);

	void UpdateData(void);
	void SetSelectedFunction(int funcnum);


};

#endif