#ifndef __INC_GRAPHVIEW_H
#define __INC_GRAPHVIEW_H

#include"ogl.h"

class CGraphDescriptor;

class CNodeEvent:public wxCommandEvent
{
    DECLARE_DYNAMIC_CLASS(CNodeEvent)

protected:
	long m_nNode;
	wxPoint m_ptPosition;
	
public:

	CNodeEvent():wxCommandEvent() { m_nNode=0; }
	CNodeEvent(const CNodeEvent &dup) : wxCommandEvent(dup) {
		m_nNode=dup.m_nNode; 
		m_ptPosition=dup.m_ptPosition; 
	}
	virtual wxEvent *Clone() const {  
		return new CNodeEvent(*this);
	}

	long GetNode(void) { return m_nNode; }
	void SetNode(long node) { m_nNode=node; }

	wxPoint GetPosition(void) { return m_ptPosition; }
	void SetPosition(wxPoint pt) { m_ptPosition=pt; }
};


BEGIN_DECLARE_EVENT_TYPES()
	DECLARE_LOCAL_EVENT_TYPE(GV_NODE_SELECTED,700)
	DECLARE_LOCAL_EVENT_TYPE(GV_NODE_ACTIVATED,701)
	DECLARE_LOCAL_EVENT_TYPE(GV_NODE_MENU,702)
END_DECLARE_EVENT_TYPES()

class CGraphView:public wxShapeCanvas
{

protected:

	wxDiagram m_diag;

	wxBrush *m_pbrGreen;
	wxBrush *m_pbrOffWhite;
	wxBrush *m_pbrOffWhiteDarker;
	wxBrush *m_pbrRed;
	wxBrush *m_pbrYellow;
	wxBrush *m_pbrBlue;
	wxBrush *m_pbrOrange;
	wxBrush *m_pbrPurple;
	wxBrush *m_pbrPink;
	wxPen *m_penDot;
	wxFont *m_fntText;
	wxCursor *m_pCurArrow;
	wxCursor *m_pCurHand;
	wxCursor *m_pCurFinger;
	wxCursor *m_pCurHandClosed;
	wxCursor *m_pCurMagnifier;

	int m_nMode;
	bool m_bGrabbing;
	bool m_bZooming;
	wxPoint m_ptGrab;
	wxPoint m_ptGrabLogical;
	wxShape *m_pHoverShape;
	wxShape *m_pClickShape;
		
	CGraphDescriptor *m_pGraphDescriptor;

	void UpdateCursor(void);

public:
	
	typedef enum {
		HAND,
		ARROW
	} CURSORMODE;

	CGraphView(wxWindow *parent = NULL, wxWindowID id = -1, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
               long style = wxBORDER);
	~CGraphView();

	bool SetGraphDescriptor(CGraphDescriptor *pgd);
	bool AutoLayoutDiagram(void);
//	bool LoadDiagramLayout(wxExprDatabase *pdb);
//	bool SaveDiagramLayout(wxExprDatabase *pdb);

	void SetCursorMode(CURSORMODE cm);
	void SetZoom(double zoom);
	double GetZoom(void);

private:

	DECLARE_EVENT_TABLE();

	void OnActivate(wxActivateEvent &event);
	void OnMouseEvent(wxMouseEvent &event);
	void OnSize(wxSizeEvent &event);
};

#endif
