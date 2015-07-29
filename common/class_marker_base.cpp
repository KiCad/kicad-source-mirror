/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2014 KiCad Developers, see CHANGELOG.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file class_marker_base.cpp
 * @brief Implementation of MARKER_BASE class.
 * Markers are used to show something (usually a drc/erc problem).
 * Markers in Pcbnew and Eeschema are derived from this base class.
 */

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
const int      M_SHAPE_SCALE = 6;     // default scaling factor for MarkerShapeCorners coordinates
const unsigned CORNERS_COUNT = 8;
/* corners of the default shape
 * actual coordinates are these values * .m_ScalingFactor
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
    m_MarkerType = MARKER_UNSPEC;
    m_ErrorLevel = 0;
    m_Color = RED;
    wxPoint start = MarkerShapeCorners[0];
    wxPoint end = MarkerShapeCorners[0];

    for( unsigned ii = 0; ii < CORNERS_COUNT; ii++ )
    {
        wxPoint corner = MarkerShapeCorners[ii];
        start.x = std::min( start.x, corner.x);
        start.y = std::min( start.y, corner.y);
        end.x = std::max( end.x, corner.x);
        end.y = std::max( end.y, corner.y);
    }

    m_ShapeBoundingBox.SetOrigin(start);
    m_ShapeBoundingBox.SetEnd(end);
}


MARKER_BASE::MARKER_BASE( const MARKER_BASE& aMarker )
{
    m_Pos = aMarker.m_Pos;
    m_MarkerType = aMarker.m_MarkerType;
    m_Color = aMarker.m_Color;
    m_ShapeBoundingBox = aMarker.m_ShapeBoundingBox;
    m_ScalingFactor = aMarker.m_ScalingFactor;
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


bool MARKER_BASE::HitTestMarker( const wxPoint& refPos ) const
{
    wxPoint rel_pos = refPos - m_Pos;
    rel_pos.x /= m_ScalingFactor;
    rel_pos.y /= m_ScalingFactor;

    return m_ShapeBoundingBox.Contains( rel_pos );
}


EDA_RECT MARKER_BASE::GetBoundingBoxMarker() const
{
    wxSize realsize = m_ShapeBoundingBox.GetSize();
    wxPoint realposition = m_ShapeBoundingBox.GetPosition();
    realsize.x *= m_ScalingFactor;
    realsize.y *= m_ScalingFactor;
    realposition.x *= m_ScalingFactor;
    realposition.y *= m_ScalingFactor;
    realposition += m_Pos;
    return EDA_RECT( m_Pos, realsize );
}

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


void MARKER_BASE::DisplayMarkerInfo( EDA_DRAW_FRAME* aFrame )
{
    wxString msg = m_drc.ShowHtml();
    DIALOG_DISPLAY_HTML_TEXT_BASE infodisplay( (wxWindow*)aFrame, wxID_ANY, _( "Marker Info" ),
                                               wxGetMousePosition(), wxSize( 550, 140 ) );

    infodisplay.m_htmlWindow->SetPage( msg );
    infodisplay.ShowModal();
}
