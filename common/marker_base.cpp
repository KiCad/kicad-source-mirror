/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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


#include "base_screen.h"
#include "marker_base.h"
#include <core/arraydim.h>
#include <geometry/shape_line_chain.h>
#include <geometry/geometry_utils.h>
#include "dialogs/dialog_display_html_text_base.h"


/**
 * The graphic shape of markers is a polygon.
 *
 * MarkerShapeCorners contains the coordinates of corners of the polygonal default shape
 * they are arbitrary units to make coding shape easy.
 * Internal units coordinates are these values scaled by .m_ScalingFactor
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


MARKER_BASE::MARKER_BASE( int aScalingFactor, std::shared_ptr<RC_ITEM> aItem, MARKER_T aType ) :
        m_markerType( aType ),
        m_excluded( false ),
        m_rcItem( std::move( aItem ) ),
        m_scalingFactor( aScalingFactor )
{
    const VECTOR2I* point_shape = MarkerShapeCorners;
    VECTOR2I        start( point_shape->x, point_shape->y );
    VECTOR2I        end = start;

    for( unsigned ii = 1; ii < CORNERS_COUNT; ii++ )
    {
        ++point_shape;
        start.x = std::min( start.x, point_shape->x );
        start.y = std::min( start.y, point_shape->y );
        end.x = std::max( end.x, point_shape->x );
        end.y = std::max( end.y, point_shape->y );
    }

    m_shapeBoundingBox.SetOrigin( start);
    m_shapeBoundingBox.SetEnd( end);
}


bool MARKER_BASE::HitTestMarker( const VECTOR2I& aHitPosition, int aAccuracy ) const
{
    const BOX2I bbox = GetBoundingBoxMarker().GetInflated( aAccuracy );

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


bool MARKER_BASE::HitTestMarker( const BOX2I& aRect, bool aContained, int aAccuracy ) const
{
    const BOX2I bbox = GetBoundingBoxMarker().GetInflated( aAccuracy );

    if( aContained )
        return aRect.Contains( bbox );

    return aRect.Intersects( bbox );
}


bool MARKER_BASE::HitTestMarker( const SHAPE_LINE_CHAIN& aPoly, bool aContained ) const
{
    SHAPE_LINE_CHAIN shape;
    ShapeToPolygon( shape );
    shape.Move( m_Pos );

    return KIGEOM::ShapeHitTest( aPoly, shape, aContained );
}


void MARKER_BASE::ShapeToPolygon( SHAPE_LINE_CHAIN& aPolygon, int aScale ) const
{
    if( aScale < 0 )
        aScale = MarkerScale();

    for( const VECTOR2I& corner : MarkerShapeCorners )
        aPolygon.Append( corner * aScale );

    // Be sure aPolygon is seen as a closed polyline:
    aPolygon.SetClosed( true );
}


BOX2I MARKER_BASE::GetBoundingBoxMarker() const
{
    BOX2I bbox = m_shapeBoundingBox;

    VECTOR2I pos = m_Pos;
    pos += m_shapeBoundingBox.GetPosition() * m_scalingFactor;

    return BOX2I( pos, bbox.GetSize() * m_scalingFactor );
}


