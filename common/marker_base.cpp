/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2018 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file marker_base.cpp
 * @brief Implementation of MARKER_BASE class.
 * Markers are used to show something (usually a drc/erc problem).
 * Markers in Pcbnew and Eeschema are derived from this base class.
 */


#include "fctsys.h"
#include "base_screen.h"
#include "common.h"
#include "macros.h"
#include "marker_base.h"
#include <geometry/shape_line_chain.h>
#include "dialog_display_info_HTML_base.h"


/* The graphic shape of markers is a polygon.
 * MarkerShapeCorners contains the coordinates of corners of the polygonal default shape
 * they are arbitrary units to make coding shape easy.
 * internal units coordinates are these values scaled by .m_ScalingFactor
 */
static const VECTOR2I MarkerShapeCorners[] =
{
    VECTOR2I( 0,  0 ),
    VECTOR2I( 8,  1 ),
    VECTOR2I( 4,  3 ),
    VECTOR2I( 13, 8 ),
    VECTOR2I( 9, 9 ),
    VECTOR2I( 8,  13 ),
    VECTOR2I( 3,  4 ),
    VECTOR2I( 1,  8 ),
    VECTOR2I( 0,  0 )
};
const unsigned CORNERS_COUNT = arrayDim( MarkerShapeCorners );

/*******************/
/* Classe MARKER_BASE */
/*******************/

void MARKER_BASE::init()
{
    m_MarkerType = MARKER_UNSPEC;
    m_ErrorLevel = MARKER_SEVERITY_UNSPEC;
    m_Color = RED;
    const VECTOR2I* point_shape = GetShapePolygon();
    wxPoint start( point_shape->x, point_shape->y );
    wxPoint end = start;

    for( int ii = 1; ii < GetShapePolygonCornerCount(); ii++ )
    {
        ++point_shape;
        start.x = std::min( start.x, point_shape->x);
        start.y = std::min( start.y, point_shape->y);
        end.x = std::max( end.x, point_shape->x);
        end.y = std::max( end.y, point_shape->y);
    }

    m_ShapeBoundingBox.SetOrigin(start);
    m_ShapeBoundingBox.SetEnd(end);
}


MARKER_BASE::MARKER_BASE( const MARKER_BASE& aMarker )
{
    m_Pos = aMarker.m_Pos;
    m_ErrorLevel = aMarker.m_ErrorLevel;
    m_MarkerType = aMarker.m_MarkerType;
    m_Color = aMarker.m_Color;
    m_ShapeBoundingBox = aMarker.m_ShapeBoundingBox;
    m_ScalingFactor = aMarker.m_ScalingFactor;
}


MARKER_BASE::MARKER_BASE( int aScalingFactor )
{
    m_ScalingFactor = aScalingFactor;
    init();
}


MARKER_BASE::MARKER_BASE( EDA_UNITS_T aUnits, int aErrorCode, const wxPoint& aMarkerPos,
                          EDA_ITEM* aItem, const wxPoint& aPos,
                          EDA_ITEM* bItem, const wxPoint& bPos, int aScalingFactor )
{
    m_ScalingFactor = aScalingFactor;
    init();

    SetData( aUnits, aErrorCode, aMarkerPos, aItem, aPos, bItem, bPos );
}


MARKER_BASE::MARKER_BASE( int aErrorCode, const wxPoint& aMarkerPos,
                          const wxString& aText, const wxPoint& aPos,
                          const wxString& bText, const wxPoint& bPos, int aScalingFactor )
{
    m_ScalingFactor = aScalingFactor;
    init();

    SetData( aErrorCode, aMarkerPos, aText, aPos, bText, bPos );
}


MARKER_BASE::MARKER_BASE( int aErrorCode, const wxPoint& aMarkerPos,
                          const wxString& aText, const wxPoint& aPos, int aScalingFactor )
{
    m_ScalingFactor = aScalingFactor;
    init();

    SetData( aErrorCode, aMarkerPos, aText, aPos );
}


MARKER_BASE::~MARKER_BASE()
{
}


void MARKER_BASE::SetData( EDA_UNITS_T aUnits, int aErrorCode, const wxPoint& aMarkerPos,
                           EDA_ITEM* aItem, const wxPoint& aPos,
                           EDA_ITEM* bItem, const wxPoint& bPos )
{
    m_Pos = aMarkerPos;
    m_drc.SetData( aUnits, aErrorCode, aItem, aPos, bItem, bPos );
    m_drc.SetParent( this );
}


void MARKER_BASE::SetData( int aErrorCode, const wxPoint& aMarkerPos,
                           const wxString& aText, const wxPoint& aPos,
                           const wxString& bText, const wxPoint& bPos )
{
    m_Pos = aMarkerPos;
    m_drc.SetData( aErrorCode, aText, aPos, bText, bPos );
    m_drc.SetParent( this );
}


bool MARKER_BASE::HitTestMarker( const wxPoint& aHitPosition, int aAccuracy ) const
{
    EDA_RECT bbox = GetBoundingBoxMarker();
    bbox.Inflate( aAccuracy );

    // Fast hit test using boundary box. A finer test will be made if requested
    bool hit = bbox.Contains( aHitPosition );

    if( hit )   // Fine test
    {
        SHAPE_LINE_CHAIN polygon;
        ShapeToPolygon( polygon );
        VECTOR2I rel_pos( aHitPosition - m_Pos );
        hit = polygon.PointInside( rel_pos, aAccuracy );
    }

    return hit;
}


void MARKER_BASE::ShapeToPolygon( SHAPE_LINE_CHAIN& aPolygon) const
{
    // Build the marker shape polygon in internal units:
    const int ccount = GetShapePolygonCornerCount();

    for( int ii = 0; ii < ccount; ii++ )
        aPolygon.Append( GetShapePolygonCorner( ii ) * MarkerScale() );

    // Be sure aPolygon is seen as a closed polyline:
    aPolygon.SetClosed( true );
}


const VECTOR2I* MARKER_BASE::GetShapePolygon() const
{
    return MarkerShapeCorners;
}


const VECTOR2I& MARKER_BASE::GetShapePolygonCorner( int aIdx ) const
{
    return MarkerShapeCorners[aIdx];
}


int MARKER_BASE::GetShapePolygonCornerCount() const
{
    return CORNERS_COUNT;
}


EDA_RECT MARKER_BASE::GetBoundingBoxMarker() const
{
    wxSize size_iu = m_ShapeBoundingBox.GetSize();
    wxPoint position_iu = m_ShapeBoundingBox.GetPosition();
    size_iu.x *= m_ScalingFactor;
    size_iu.y *= m_ScalingFactor;
    position_iu.x *= m_ScalingFactor;
    position_iu.y *= m_ScalingFactor;
    position_iu += m_Pos;

    return EDA_RECT( position_iu, size_iu );
}



void MARKER_BASE::DisplayMarkerInfo( EDA_DRAW_FRAME* aFrame )
{
    wxString msg = m_drc.ShowHtml( aFrame->GetUserUnits() );
    DIALOG_DISPLAY_HTML_TEXT_BASE infodisplay( (wxWindow*)aFrame, wxID_ANY, _( "Marker Info" ),
                                               wxGetMousePosition(), wxSize( 550, 140 ) );

    infodisplay.m_htmlWindow->SetPage( msg );
    infodisplay.ShowModal();
}


void MARKER_BASE::PrintMarker( wxDC* aDC, const wxPoint& aOffset )
{
    // Build the marker shape polygon in internal units:
    const int ccount = GetShapePolygonCornerCount();
    std::vector<wxPoint> shape;
    shape.reserve( ccount );

    for( int ii = 0; ii < ccount; ii++ )
    {
        shape.emplace_back( GetShapePolygonCorner( ii ).x * MarkerScale(),
                                  GetShapePolygonCorner( ii ).y * MarkerScale() );
    }

    for( int ii = 0; ii < ccount; ii++ )
        shape[ii] += m_Pos + aOffset;

    GRClosedPoly( nullptr, aDC, ccount, &shape[0], true, 0, m_Color, m_Color );
}
