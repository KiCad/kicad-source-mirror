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
#include "dialog_display_info_HTML_base.h"


// Default marquer shape:
#define M_SHAPE_SCALE 6     // default scaling factor for MarkerShapeCorners coordinates
#define CORNERS_COUNT 8
/* corners of the default shape
 * real coordinates are these values * .m_ScalingFactor
*/
static const wxPoint MarkerShapeCorners[CORNERS_COUNT] =
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
    wxPoint start = MarkerShapeCorners[0];
    wxPoint end = MarkerShapeCorners[0];
    for( unsigned ii = 0; ii < CORNERS_COUNT; ii++ )
    {
        wxPoint corner = MarkerShapeCorners[ii];
        m_Corners.push_back( corner );
        start.x = MIN( start.x, corner.x);
        start.y = MIN( start.y, corner.y);
        end.x = MAX( end.x, corner.x);
        end.y = MAX( end.y, corner.y);
    }

    m_ShapeBoundingBox.SetOrigin(start);
    m_ShapeBoundingBox.SetEnd(end);
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


/******************************************************/
bool MARKER_BASE::HitTestMarker( const wxPoint& refPos )
/******************************************************/
{
    wxPoint rel_pos = refPos - m_Pos;
    rel_pos.x /= m_ScalingFactor;
    rel_pos.y /= m_ScalingFactor;

    return m_ShapeBoundingBox.Inside(rel_pos);
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
    wxSize realsize = m_ShapeBoundingBox.GetSize();
    wxPoint realposition = m_ShapeBoundingBox.GetPosition();
    realsize.x *= m_ScalingFactor;
    realsize.y *= m_ScalingFactor;
    realposition.x *= m_ScalingFactor;
    realposition.y *= m_ScalingFactor;
    realposition += m_Pos;
    return EDA_Rect( m_Pos,realsize );
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


/** Function DisplayMarkerInfo()
 * Displays the full info of this marker, within an HTML window
 */
void MARKER_BASE::DisplayMarkerInfo( WinEDA_DrawFrame* aFrame )
{
    wxString msg = m_drc.ShowHtml();
    DIALOG_DISPLAY_HTML_TEXT_BASE
        infodisplay( (wxWindow*)aFrame, wxID_ANY, _("Marker Info"),
        wxGetMousePosition(), wxSize( 550, 140 ) );

    infodisplay.m_htmlWindow->SetPage( msg );
    infodisplay.ShowModal();
}
