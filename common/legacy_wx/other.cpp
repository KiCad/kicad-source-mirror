#include "fctsys.h"
#include "gr_basic.h"
#include "base_screen.h"
#include "common.h"
#include "macros.h"
#include "class_drawpanel.h"
#include "marker_base.h"
#include "dialog_display_info_HTML_base.h"


void MARKER_BASE::DrawMarker( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aOffset )
{
    // Build the marker shape polygon in internal units:
    const int ccount = GetShapePolygonCornerCount();
    std::vector<wxPoint> shape;
    shape.reserve( ccount );

    for( int ii = 0; ii < ccount; ii++ )
    {
        shape.push_back( wxPoint( GetShapePolygonCorner( ii ).x * MarkerScale(),
                                  GetShapePolygonCorner( ii ).y * MarkerScale() ) );
    }

    for( int ii = 0; ii < ccount; ii++ )
    {
        shape[ii] += m_Pos + aOffset;
    }

    GRClosedPoly( aPanel->GetClipBox(), aDC, ccount, &shape[0],
                  true,         // = Filled
                  0,            // outline width
                  m_Color,      // outline color
                  m_Color       // fill collor
                  );
}
