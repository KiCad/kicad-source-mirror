#include "fctsys.h"
#include "gr_basic.h"
#include "base_screen.h"
#include "common.h"
#include "macros.h"
#include "class_drawpanel.h"
#include "marker_base.h"
#include "dialog_display_info_HTML_base.h"


static const wxPoint MarkerShapeCorners[] =
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
const unsigned CORNERS_COUNT = DIM( MarkerShapeCorners );



void MARKER_BASE::DrawMarker( EDA_DRAW_PANEL* aPanel, wxDC* aDC, GR_DRAWMODE aDrawMode,
                              const wxPoint& aOffset )
{
    wxPoint corners[CORNERS_COUNT];

    GRSetDrawMode( aDC, aDrawMode );

    for( unsigned ii = 0; ii < CORNERS_COUNT; ii++ )
    {
        corners[ii] = MarkerShapeCorners[ii];
        corners[ii].x *= m_ScalingFactor;
        corners[ii].y *= m_ScalingFactor;
        corners[ii] += m_Pos + aOffset;
    }

    GRClosedPoly( aPanel->GetClipBox(), aDC, CORNERS_COUNT, corners,
                  true,         // = Filled
                  0,            // outline width
                  m_Color,      // outline color
                  m_Color       // fill collor
                  );
}
