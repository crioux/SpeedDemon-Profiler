#include "spdreader_pch.h"
#include <math.h>
#include "PieChart.h"

const UINT CPieSlice::m_nClassVersion = 1;		// ClassVersion is used for...
const UINT CPieChart::m_nClassVersion = 1;		// ...serialization versioning

//*****************************************************************
//* implementation of CPieChart
//*****************************************************************

CPieSlice::CPieSlice()
{
	m_nPercentage = 50;
	m_clrFace = wxColour(PIECOLOR_RED);
	m_szDescription = _T("<no description>");
	m_radiuspercent=1.0;
	m_centeroffset=wxRealPoint(0,0);
}

CPieSlice::CPieSlice( int nPercentage )
{
	SetPercentage(nPercentage);
	m_clrFace = wxColour(PIECOLOR_RED);
	m_szDescription = _T("<no description>");
	m_radiuspercent=1.0;
	m_centeroffset=wxRealPoint(0,0);
}

CPieSlice::CPieSlice( int nPercentage, wxColour clrFace )
{
	SetPercentage(nPercentage);
	SetColor(clrFace);
	m_szDescription = _T("<no description>");
	m_radiuspercent=1.0;
	m_centeroffset=wxRealPoint(0,0);
}

CPieSlice::CPieSlice( int nPercentage, wxColour clrFace, wxString szDescription )
{
	SetPercentage(nPercentage);
	SetColor(clrFace);
	SetDescription(szDescription);
	m_radiuspercent=1.0;
	m_centeroffset=wxRealPoint(0,0);
}

CPieSlice::~CPieSlice()
{

}

//*****************************************************************
//* implementation of CPieChart
//*****************************************************************

CPieChart::CPieChart()
{
	m_clrBorders	= wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWFRAME);
	m_clrText		= wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
	m_szTitle		= _T("");
	m_bAutoDel		= true;
}



CPieChart::~CPieChart()
{	
	// if we need to delete stuff, then delete
	if ( m_bAutoDel == true )
	{
		DeleteAllSlices();
	}
}


int CPieChart::AddSlice( CPieSlice *newSlice )
{
	m_oaSlices.push_back( newSlice );
	return ((int)m_oaSlices.size())-1;
}

CPieSlice* CPieChart::GetSlice( int nPos )
{
	if ( nPos >= (int)(m_oaSlices.size()) )
	{	// what is this guy trying to do?
		wxLogDebug(_T("WARNING: CPieChart::GetSlice -> index out of bounds!\n"));
		return NULL;
	}

	return (CPieSlice*)m_oaSlices[nPos];
}

//****************************************************
//* NOTE: this function does NOT delete anything EVER
//* even it the AutoDelete is set to true.
//****************************************************
CPieSlice* CPieChart::RemoveSlice( int nPos )
{
	if ( nPos >= ((int)m_oaSlices.size()) )
	{	// again, what is this guy trying to do?
		wxLogDebug(_T("WARNING: CPieChart::RemoveSlice -> index out of bounds!\n"));
		return NULL;
	}

	CPieSlice* retVal = (CPieSlice*)m_oaSlices[nPos];
	m_oaSlices.erase( m_oaSlices.begin()+nPos );

	return retVal;
}

void CPieChart::DeleteAllSlices()
{
	CPieSlice* pIter = NULL;
	int nSlices = (int) m_oaSlices.size();

	for ( int i = 0; i < nSlices; i++ )
	{
		delete m_oaSlices[i];
	}

	m_oaSlices.clear();
}

//****************************************************
//* the hot stuff! (-:
//*
//* pDC - the target DC
//* ptCenter - the center point of our cheese pie
//* nRadius - radius of the pie, in logical units
//* bShowTitle - do we need to draw a title?
//* nLegend - do we need to draw a legend, and where.
//*	bFillExtra - do we draw a full circle even if
//*				we have invalid data?
//****************************************************
void CPieChart::Draw2D(
					   wxDC* pDC,
					   wxPoint ptCenter,
					   int nRadius,
					   bool bShowTitle,
					   int nLegend,
					   bool bFillExtra)
{
	assert(pDC);

	// get the number of slices we have to draw
	int nSlices = (int)m_oaSlices.size();

	// we gotta run a little check here:
	// if our bFillExtra is false, we need to check
	// if the sum of sizes of our slices equals 100%
	// otherwise you get really strange apple pies..
	if ( bFillExtra == false )
	{
		int nSumPercentage = 0;

		for ( int i = 0; i < nSlices; i++ )
		{
			CPieSlice* pIter = NULL;
			pIter = (CPieSlice*)m_oaSlices[i];

			if ( pIter != NULL )
			{
				nSumPercentage += pIter->GetPercentage();
			}
		}

		if ( nSumPercentage != 100 )
		{
			wxLogDebug(_T("WARNING: CPieChart data is invalid!\n"));
			wxLogDebug(_T("WARNING: CPieChart not drawn!\n"));
			return;
		}
	}


	// make the world spin around our chart center
	pDC->SetDeviceOrigin( ptCenter.x, ptCenter.y );

	// some initial values
    int x1 = 0;
    int y1 = -nRadius;
    int nStep = 0;
	const int nPercent = 100;

	// and some more..
	CPieSlice* pIter2 = NULL;
	wxColour sliceColor;
	wxBrush sliceBrush;
	// make our DC use the correct pen for the chart borders
	wxPen chartPen(m_clrBorders,1,wxSOLID);
	pDC->SetPen(chartPen);

	// iterate through all the slices and draw
    for ( int j = 0; j < nSlices; j++)
	{
		pIter2 = m_oaSlices[j];
		int nHalfStep = nStep+(pIter2->GetPercentage()/2);
		nStep += pIter2->GetPercentage();

		// if the size of our pie slice is zero,
		// we get a REALLY weird pie (this is due
		// to sin and cos functions, damn thing
		// makes a full circle)
		if ( pIter2->GetPercentage() == 0 && (bFillExtra == false) )
			continue;

		// if we need to fill the extra space with the last pie slice
		if ( (bFillExtra == true) && j == (nSlices-1) )
		{	// yep, this is the last one..
			nStep = 100;
		}

		// a bit of math (-:
		// i kinda 'borrowed' this from a book i read, but it's actually
		// very simple, we calculate the radian value of the next slice
		// and do sin/cos to get x and y pos of the ending point
        double radians = ((double) (nStep * 2 * M_PI) / (double) nPercent) + M_PI;
        double halfradians = ((double) (nHalfStep * 2 * M_PI) / (double) nPercent) + M_PI;
        int x2 = (int) (sin (radians) * nRadius);
        int y2 = (int) (cos (radians) * nRadius);

		// CPaintDC->Pie(..) uses current brush for the pie fill
		sliceColor = pIter2->GetColor();
		sliceBrush=wxBrush(sliceColor);
		
		// take the brush in your left hand
		pDC->SetBrush(sliceBrush);
		
		// and draw a pie
		int xoff=pIter2->GetCenterOffset().x*nRadius*sin(halfradians);
		int yoff=pIter2->GetCenterOffset().y*nRadius*cos(halfradians);
        pDC->DrawArc(x1 * pIter2->GetRadiusPercent()+xoff, y1 * pIter2->GetRadiusPercent()+yoff, 
			x2 * pIter2->GetRadiusPercent()+xoff, y2 * pIter2->GetRadiusPercent()+yoff, 
			xoff, yoff) ;

		// set our last point as the first point of the next slice
        x1 = x2;
        y1 = y2;
    }

	// ok, we got the chart out, now put the title...
	if ( bShowTitle == true )
	{
		wxPoint ptTitle;
//		pDC->SetTextAlign(TA_CENTER);
		pDC->SetTextForeground( m_clrText );
		pDC->SetBackgroundMode(wxTRANSPARENT);
		// do some calculations so we get that title
		// centered properly
		ptTitle.x = ptCenter.x;
		ptTitle.y = ptCenter.y - nRadius - 20;

		//pDC->DPtoLP(&ptTitle);
		pDC->DrawText( m_szTitle, pDC->DeviceToLogicalX(ptTitle.x), pDC->DeviceToLogicalY(ptTitle.y));
		// put the alignment back
//		pDC->SetTextAlign(TA_TOP);
		// put the right color again
//		pDC->SetTextColor( oldColor );
	}

	// see if we need to draw the legend
	// NOTE:	i didn't make PIELEGEND_TOP and PIELEGEND_LEFT
	//			because it would require a lot more calculations
	//			than i wanna do right now.
	//			OK, I'm lazy, SO WHAT!
	switch ( nLegend )
	{
	case PIELEGEND_BELOW:
		{
			wxPoint thePoint;
			thePoint.y = ptCenter.y + nRadius + 30;
			thePoint.x = ptCenter.x - nRadius;
			DrawLegend( pDC, thePoint, bFillExtra );
		}
		break;
	case PIELEGEND_RIGHT:
		{
			wxPoint thePoint;
			// (nSlices*20) equals the height of our
			// legend, so this way we will center it
			// with the center of our pie
			thePoint.y = ptCenter.y - (nSlices*20)/2;
			// BUT, if the legend Y point is ABOVE the
			// pie top, there is a good possibility
			// that our legend will be drawn OVER our
			// title.. and we kinda don't want that..
			if ( thePoint.y < ( ptCenter.y - nRadius ) )
				thePoint.y = ( ptCenter.y - nRadius );

			thePoint.x = ptCenter.x + nRadius + 30;
			DrawLegend( pDC, thePoint, bFillExtra );
		}
		break;
	default:
		// do nothing..
		break;
	}

}


//****************************************************
//* Note that you can use this function to draw a
//* legend anywhere you want, it doesn't have to be
//* 'packed' with the cheese pie itself.
//*
//* pDC - the target DC
//* ptLocation - the upper left cornet of our legend
//*	bFillExtra - do we need to fill in the last
//*				percentage info??
//****************************************************
void CPieChart::DrawLegend( wxDC * pDC, wxPoint ptLocation, bool bFillExtra )
{
	assert(pDC);

	// get the number of slices
	int nSlices = (int) m_oaSlices.size();

	wxPoint spot = ptLocation;
	wxRect rect;
	wxColour clrIter;
	wxString szIter;
	CPieSlice* pIter = NULL;
	int nPercent = 0;

	// set the correct color
	pDC->SetTextForeground( m_clrText );

	for ( int i = 0; i < nSlices; i++ )
	{
		pIter = (CPieSlice*)m_oaSlices[i];

		if ( pIter != NULL )
		{
			clrIter = pIter->GetColor();
			szIter = pIter->GetDescription();
			wxString percent;

			if ( (bFillExtra == true) && i == (nSlices-1) )
			{	// if we need to fill the last one, then
				// calculate the rest of the percentage
				percent.Format( _T("(%d%%)"), 100-nPercent );
			}
			else
			{
				percent.Format( _T(" (%d%%)"), pIter->GetPercentage() );
				nPercent += pIter->GetPercentage();
			}
			// concat
			szIter += percent;

			// move our rect to a right position
			rect.y = spot.y;
			rect.x = spot.x;
			rect.height=15;
			rect.width=30;

			//pDC->DPtoLP(rect);

			// and draw it
			//pDC->FillSolidRect( rect, clrIter );
			pDC->DrawRectangle(
				pDC->DeviceToLogicalX(rect.x),
				pDC->DeviceToLogicalY(rect.y),
				pDC->DeviceToLogicalX(rect.x+rect.width)-pDC->DeviceToLogicalX(rect.x),
				pDC->DeviceToLogicalY(rect.y+rect.height)-pDC->DeviceToLogicalY(rect.y)
				);

				
			pDC->SetBackgroundMode(wxTRANSPARENT);
			//pDC->SetBrush(wxNullBrush);
			//pDC->TextOut( rect.right+5, rect.top, szIter );
			pDC->DrawText(szIter , rect.GetRight()+5, rect.GetTop());

		}

		// move down by the height of one line
		spot.y += 20;
	}
}