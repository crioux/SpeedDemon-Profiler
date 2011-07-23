#ifndef __PIECHART_H_INCLUDED__
#define __PIECHART_H_INCLUDED__

#ifndef PIECOLORS
#define PIECOLORS

#define PIECOLOR_BLACK		(wxColour(0,0,0))
#define PIECOLOR_WHITE		(wxColour(255,255,255))
#define PIECOLOR_GRAY		(wxColour(128,128,128))
#define PIECOLOR_RED		(wxColour(255,0,0))
#define PIECOLOR_GREEN		(wxColour(0,255,0))
#define PIECOLOR_BLUE		(wxColour(0,0,255))
#define PIECOLOR_YELLOW		(wxColour(255,255,0))
#define PIECOLOR_CYAN	 	(wxColour(0,255,255))
#define PIECOLOR_PURPLE  	(wxColour(255,0,255))

#endif // PIECOLORS

#ifndef PIELEGEND_LOCATIONS
#define PIELEGEND_LOCATIONS

#define PIELEGEND_NONE		0
#define PIELEGEND_RIGHT		1
#define PIELEGEND_BELOW		2

#endif // PIELEGEND_LOCATIONS


class CPieSlice
{
public:
	CPieSlice();
	CPieSlice( int nPercentage );
	CPieSlice( int nPercentage, wxColour clrFace );
	CPieSlice( int nPercentage, wxColour clrFace, wxString szDescription );
	virtual ~CPieSlice();

	//*************************
	//* inline functions
	void SetPercentage( UINT nPercentage )
	{
		if ( nPercentage < 0 )
		{
			wxLogDebug(_T("WARNING: PieSlice size truncation!\n"));
			m_nPercentage = 0;
		}
		else if ( nPercentage > 100 )
		{
			wxLogDebug(_T("WARNING: PieSlice size truncation!\n"));
			m_nPercentage = 100;
		}
		else
			m_nPercentage = nPercentage;
	}

	void SetColor( wxColour clrFace ) {	m_clrFace = clrFace; }
	UINT GetPercentage() { return m_nPercentage; }
	
	void SetRadiusPercent(double perc) { m_radiuspercent=perc; }
	double GetRadiusPercent(void) { return m_radiuspercent; }
	void SetCenterOffset(wxRealPoint off) { m_centeroffset=off; }
	wxRealPoint GetCenterOffset(void) { return m_centeroffset; }

	wxColour GetColor() { return m_clrFace; }

	void SetDescription( wxString szDecription ) { m_szDescription = szDecription; }
	wxString GetDescription() { return m_szDescription; }

private:
	UINT m_nPercentage;			// how big is our piece of cake?
	wxColour m_clrFace;			// filling the slice with this one
	wxString m_szDescription;	// used for drawing the legend
	double m_radiuspercent;
	wxRealPoint m_centeroffset;
	static const UINT m_nClassVersion;	// for serialization
};

class CPieChart
{
public:
	CPieChart();
	virtual ~CPieChart();

	int AddSlice( CPieSlice *newSlice );
	CPieSlice* GetSlice( int nPos );
	CPieSlice* RemoveSlice( int nPos );
	void DeleteAllSlices();

	//*************************
	//* inline functions
	void SetBorderColor( wxColour clrBorder ) { m_clrBorders = clrBorder; }
	wxColour GetBorderColor() {	return m_clrBorders; }

	void SetTextColor( wxColour clrText ) { m_clrText = clrText; }
	wxColour GetTextColor() { return m_clrText; }

	void SetTitle( wxString szTitle ) { m_szTitle = szTitle;	}
	wxString GetTitle() { return m_szTitle; }

	void SetAutoDelete( bool bAutoDel = true ) { m_bAutoDel = bAutoDel; }
	bool GetAutoDelete() { return m_bAutoDel; }

	UINT GetCount() { return (UINT) m_oaSlices.size(); }
	//* end of inline functions
	//*************************

	void Draw2D(
		wxDC* pDC,
		wxPoint ptCenter,
		int nRadius,
		bool bShowTitle = true,
		int nLegend = PIELEGEND_NONE,
		bool bFillExtra = false );

	void DrawLegend(
		wxDC* pDC,
		wxPoint ptLocation,
		bool bFillExtra = false );

private:
	bool m_bAutoDel;	// determines if this class will delete stuff when destroyed
	wxString m_szTitle;					// graph title, drawn above the graph itself
	std::vector<CPieSlice *> m_oaSlices;	// an array of "slices"
	static const double m_dPI;			// you know, 3.14159... (-:
	wxColour m_clrBorders;				// color used to draw the lines on our cake
	wxColour m_clrText;					// color used to output text to DC
	static const UINT m_nClassVersion;	// used for serialization
};

#endif // __PIECHART_H_INCLUDED__
