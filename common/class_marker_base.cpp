/**********************************************************************************
* class MARKER_BASE; markers are used to show something (usually a drc/erc problem)
* Markers in pcbnew and eeschema are derived from this basic class
**********************************************************************************/

/* file class_marker_base.cpp
 */

#include "fctsys.h"
#include "gr_basic.h"
#include "class_base_screen.h"
#include "common.h"
#include "macros.h"
#include "class_drawpanel.h"
#include "class_marker_base.h"


// Default marquer shape:
#define M_SHAPE_SCALE 6     // default scaling factor for MarkerShapeCorners coordinates
#define CORNERS_COUNT 8
// corners of the default shape
static wxPoint MarkerShapeCorners[CORNERS_COUNT] =
{
    wxPoint( 0,  0 ),
    wxPoint( 8,  1 ),
    wxPoint( 4,  3 ),
    wxPoint( 13, 8 ),
    wxPoint( 9, 9 ),
    wxPoint( 8,  13 ),
    wxPoint( 3,  4 ),
    wxPoint( 1,  8 )
};

/*******************/
/* Classe MARKER_BASE */
/*******************/

void MARKER_BASE::init()
{
    m_MarkerType = 0;
    m_Color = RED;
    for( unsigned ii = 0; ii < CORNERS_COUNT; ii++ )
    {
        wxPoint corner = MarkerShapeCorners[ii];
        m_Corners.push_back( corner );
        m_Size.x = MAX( m_Size.x, corner.x);
        m_Size.y = MAX( m_Size.y, corner.y);
    }
}


MARKER_BASE::MARKER_BASE()
{
    m_ScalingFactor = M_SHAPE_SCALE;
    init();
}


MARKER_BASE::MARKER_BASE( int aErrorCode, const wxPoint& aMarkerPos,
                          const wxString& aText, const wxPoint& aPos,
                          const wxString& bText, const wxPoint& bPos )
{
    m_ScalingFactor = M_SHAPE_SCALE;
    init();

    SetData( aErrorCode, aMarkerPos,
             aText, aPos,
             bText, bPos );
}


MARKER_BASE::MARKER_BASE( int aErrorCode, const wxPoint& aMarkerPos,
                          const wxString& aText, const wxPoint& aPos )
{
    m_ScalingFactor = M_SHAPE_SCALE;
    init();
    SetData( aErrorCode, aMarkerPos, aText, aPos );
}


/* Effacement memoire de la structure */
MARKER_BASE::~MARKER_BASE()
{
}


void MARKER_BASE::SetData( int aErrorCode, const wxPoint& aMarkerPos,
                           const wxString& aText, const wxPoint& aPos,
                           const wxString& bText, const wxPoint& bPos )
{
    m_Pos = aMarkerPos;
    m_drc.SetData( aErrorCode,
                   aText, bText, aPos, bPos );
}


void MARKER_BASE::SetData( int aErrorCode, const wxPoint& aMarkerPos,
                           const wxString& aText, const wxPoint& aPos )
{
    m_Pos = aMarkerPos;
    m_drc.SetData( aErrorCode,
                   aText, aPos );
}


/**********************************************/
bool MARKER_BASE::HitTestMarker( const wxPoint& refPos )
/**********************************************/
{
    int     dx = refPos.x - m_Pos.x;
    int     dy = refPos.y - m_Pos.y;

    wxSize Realsize = m_Size;
    Realsize.x *= m_ScalingFactor;
    Realsize.y *= m_ScalingFactor;

    /* is refPos in the box: Marker size to right an bottom,
     *  or size/2 to left or top */
    if( dx <= Realsize.x  && dy <= Realsize.y
        && dx >= -Realsize.x / 2  && dy >= -Realsize.y / 2 )
        return true;
    else
        return false;
}

/**
 * Function GetBoundingBoxMarker
 * returns the orthogonal, bounding box of this object for display purposes.
 * This box should be an enclosing perimeter for visible components of this
 * object, and the units should be in the pcb or schematic coordinate system.
 * It is OK to overestimate the size by a few counts.
 */
EDA_Rect MARKER_BASE::GetBoundingBoxMarker()
{
    wxSize Realsize = m_Size;
    Realsize.x *= m_ScalingFactor;
    Realsize.y *= m_ScalingFactor;
    return EDA_Rect( m_Pos,Realsize );
}

/**********************************************************************/
void MARKER_BASE::DrawMarker( WinEDA_DrawPanel* aPanel, wxDC* aDC, int aDrawMode,
                              const wxPoint& aOffset )
/**********************************************************************/

/**  Function DrawMarker
 *  The shape is the polygon defined in m_Corners (array of wxPoints)
 */
{
    wxPoint corners[CORNERS_COUNT];

    GRSetDrawMode( aDC, aDrawMode );

    for( unsigned ii = 0; ii < m_Corners.size(); ii++ )
    {
        corners[ii] = m_Corners[ii];
        corners[ii].x *= m_ScalingFactor;
        corners[ii].y *= m_ScalingFactor;
        corners[ii] += m_Pos + aOffset;
    }

    GRClosedPoly( &aPanel->m_ClipBox, aDC, CORNERS_COUNT, corners,
                  true,         // = Filled
                  0,            // outline width
                  m_Color,      // outline color
                  m_Color       // fill collor
                  );
}
