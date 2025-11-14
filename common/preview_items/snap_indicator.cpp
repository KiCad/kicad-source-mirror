/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include "preview_items/snap_indicator.h"

#include <gal/graphics_abstraction_layer.h>

using namespace KIGFX;

SNAP_INDICATOR::SNAP_INDICATOR( const COLOR4D& aColor, int aSize, const VECTOR2D& aPosition ) :
        ORIGIN_VIEWITEM( aColor, CIRCLE_CROSS, aSize, aPosition )
{
}


SNAP_INDICATOR* SNAP_INDICATOR::Clone() const
{
    return new SNAP_INDICATOR( m_color, m_size, m_position );
}


const BOX2I SNAP_INDICATOR::ViewBBox() const
{
    return ORIGIN_VIEWITEM::ViewBBox();
}


static void DrawSnapNode( GAL& aGal, const VECTOR2I& aPosition, int aNodeRadius )
{
    aGal.SetIsFill( true );
    aGal.SetIsStroke( false );

    aGal.DrawCircle( aPosition, aNodeRadius );

    aGal.SetIsFill( false );
    aGal.SetIsStroke( true );
}


static void DrawCornerIcon( GAL& aGal, const VECTOR2I& aPosition, int aSize )
{
    const int      nodeRad = aSize / 8;
    const VECTOR2I corner = aPosition - VECTOR2I( aSize / 2, aSize / 2 ) + VECTOR2I( nodeRad, nodeRad );
    aGal.DrawLine( corner, corner + VECTOR2I( aSize - nodeRad, 0 ) );
    aGal.DrawLine( corner, corner + VECTOR2I( 0, aSize - nodeRad ) );

    DrawSnapNode( aGal, corner, nodeRad );
}


static void DrawLineEndpointIcon( GAL& aGal, const VECTOR2I& aPosition, int aSize )
{
    const int      nodeRadius = aSize / 8;
    const VECTOR2I lineStart = aPosition - VECTOR2I( aSize / 2 - nodeRadius, 0 );

    DrawSnapNode( aGal, lineStart, nodeRadius );
    aGal.DrawLine( lineStart, lineStart + VECTOR2I( aSize - nodeRadius, 0 ) );
}


static void DrawMidpointIcon( GAL& aGal, const VECTOR2I& aPosition, int aSize )
{
    const int nodeRadius = aSize / 8;

    DrawSnapNode( aGal, aPosition, nodeRadius );
    aGal.DrawLine( aPosition - VECTOR2I( aSize / 2, 0 ), aPosition + VECTOR2I( aSize / 2, 0 ) );
}


static void DrawCentrePointIcon( GAL& aGal, const VECTOR2I& aPosition, int aSize )
{
    const int ringRadius = aSize / 4;

    aGal.DrawCircle( aPosition, ringRadius );

    aGal.DrawLine( aPosition - VECTOR2I( aSize / 2, 0 ), aPosition + VECTOR2I( aSize / 2, 0 ) );
    aGal.DrawLine( aPosition - VECTOR2I( 0, aSize / 2 ), aPosition + VECTOR2I( 0, aSize / 2 ) );
}


static void DrawQuadrantPointIcon( GAL& aGal, const VECTOR2I& aPosition, int aSize )
{
    const int nodeRadius = aSize / 8;

    const VECTOR2I quadPoint = aPosition - VECTOR2I( 0, aSize / 2 - nodeRadius );
    const int      arcRadius = aSize - nodeRadius * 2;

    DrawSnapNode( aGal, quadPoint, nodeRadius );

    // Most of the top half of a circle, passing through the node centre
    const VECTOR2I arcCenter = quadPoint + VECTOR2I( 0, arcRadius );
    aGal.DrawArc( arcCenter, arcRadius, EDA_ANGLE( -160, EDA_ANGLE_T::DEGREES_T ),
                  EDA_ANGLE( 140, EDA_ANGLE_T::DEGREES_T ) );
}


static void DrawIntersectionIcon( GAL& aGal, const VECTOR2I& aPosition, int aSize )
{
    const int nodeRadius = aSize / 8;

    DrawSnapNode( aGal, aPosition, nodeRadius );

    // Slightly squashed X shape
    VECTOR2I xLeg = VECTOR2I( aSize / 2, aSize / 3 );

    aGal.DrawLine( aPosition - xLeg, aPosition + xLeg );
    xLeg.y = -xLeg.y;
    aGal.DrawLine( aPosition - xLeg, aPosition + xLeg );
}


static void DrawOnElementIcon( GAL& aGal, const VECTOR2I& aPosition, int aSize )
{
    const int nodeRadius = aSize / 8;

    // A bit like midpoint by off to one side
    DrawSnapNode( aGal, aPosition + VECTOR2I( aSize / 4, 0 ), nodeRadius );

    aGal.DrawLine( aPosition - VECTOR2I( aSize / 2, 0 ), aPosition + VECTOR2I( aSize / 2, 0 ) );
}


void SNAP_INDICATOR::ViewDraw( int, VIEW* aView ) const
{
    GAL& gal = *aView->GetGAL();

    // Draw the origin marker
    ORIGIN_VIEWITEM::ViewDraw( 0, aView );

    gal.SetFillColor( m_color );

    // Put the icon near the x-line, so it doesn't overlap with the ruler helpers
    const VECTOR2I typeIconPos = m_position + aView->ToWorld( { 24, 10 }, false );
    const int      size = aView->ToWorld( 16 );

    // For now, choose the first type that is set
    if( m_snapTypes & POINT_TYPE::PT_CORNER )
    {
        DrawCornerIcon( gal, typeIconPos, size );
    }
    else if( m_snapTypes & POINT_TYPE::PT_END )
    {
        DrawLineEndpointIcon( gal, typeIconPos, size );
    }
    else if( m_snapTypes & POINT_TYPE::PT_MID )
    {
        DrawMidpointIcon( gal, typeIconPos, size );
    }
    else if( m_snapTypes & POINT_TYPE::PT_CENTER )
    {
        DrawCentrePointIcon( gal, typeIconPos, size );
    }
    else if( m_snapTypes & POINT_TYPE::PT_QUADRANT )
    {
        DrawQuadrantPointIcon( gal, typeIconPos, size );
    }
    else if( m_snapTypes & POINT_TYPE::PT_INTERSECTION )
    {
        DrawIntersectionIcon( gal, typeIconPos, size );
    }
    else if( m_snapTypes & POINT_TYPE::PT_ON_ELEMENT )
    {
        DrawOnElementIcon( gal, typeIconPos, size );
    }
}
