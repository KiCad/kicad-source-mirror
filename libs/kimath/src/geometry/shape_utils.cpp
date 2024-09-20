/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "geometry/shape_utils.h"

#include <geometry/seg.h>
#include <geometry/half_line.h>
#include <geometry/line.h>
#include <geometry/shape_rect.h>


SEG KIGEOM::NormalisedSeg( const SEG& aSeg )
{
    if( LexicographicalCompare( aSeg.A, aSeg.B ) <= 0 )
    {
        return aSeg;
    }
    return aSeg.Reversed();
}


const VECTOR2I& KIGEOM::GetOtherEnd( const SEG& aSeg, const VECTOR2I& aPoint )
{
    return ( aSeg.A == aPoint ) ? aSeg.B : aSeg.A;
}


std::array<SEG, 4> KIGEOM::BoxToSegs( const BOX2I& aBox )
{
    const std::array<VECTOR2I, 4> corners = {
        VECTOR2I{ aBox.GetLeft(), aBox.GetTop() },
        VECTOR2I{ aBox.GetRight(), aBox.GetTop() },
        VECTOR2I{ aBox.GetRight(), aBox.GetBottom() },
        VECTOR2I{ aBox.GetLeft(), aBox.GetBottom() },
    };

    return {
        SEG{ corners[0], corners[1] },
        SEG{ corners[1], corners[2] },
        SEG{ corners[2], corners[3] },
        SEG{ corners[3], corners[0] },
    };
}


std::optional<SEG> KIGEOM::ClipHalfLineToBox( const HALF_LINE& aRay, const BOX2I& aBox )
{
    // Do the naive implementation - if this really is done in a tight loop,
    // the Cohen-Sutherland implementation in ClipLine could be faster, but
    // needs to be adapted to work with half-lines.
    const std::array<SEG, 4> boxSegs = KIGEOM::BoxToSegs( aBox );

    std::optional<VECTOR2I> ptA, ptB;

    for( const SEG& boxSeg : boxSegs )
    {
        OPT_VECTOR2I intersection = aRay.Intersect( boxSeg );

        if( !intersection )
        {
            continue;
        }

        // Init the first point or eat it if it's the same
        if( !ptA || *intersection == *ptA )
        {
            ptA = *intersection;
        }
        else
        {
            ptB = *intersection;
        }
    }

    // If we have exactly two intersections, the ray crossed twice
    // so take the segment between the two points
    if( ptA && ptB )
    {
        return SEG( *ptA, *ptB );
    }

    // It only crosses once, so the start is in the box. Take the segment from
    // the start point to the intersection
    if( ptA && *ptA != aRay.GetStart() )
    {
        return SEG( aRay.GetStart(), *ptA );
    }

    // It didn't cross at all
    return std::nullopt;
}


std::optional<SEG> KIGEOM::ClipLineToBox( const LINE& aLine, const BOX2I& aBox )
{
    // As above, maybe can be optimised?
    const std::array<SEG, 4> boxSegs = KIGEOM::BoxToSegs( aBox );

    std::optional<VECTOR2I> ptA, ptB;

    for( const SEG& boxSeg : boxSegs )
    {
        OPT_VECTOR2I intersection = aLine.Intersect( boxSeg );

        // Reject intersections that are not on the actual box boundary
        if( intersection && boxSeg.Contains( *intersection ) )
        {
            // Init the first point or eat it if it's the same
            if( !ptA || *intersection == *ptA )
            {
                ptA = *intersection;
            }
            else
            {
                ptB = *intersection;
            }
        }
    }

    // If we have exactly two intersections, we have a segment
    // (zero is no intersection, and one is a just crossing a corner exactly)
    if( ptA && ptB )
    {
        return SEG( *ptA, *ptB );
    }

    return std::nullopt;
}


SHAPE_ARC KIGEOM::MakeArcCw90( const VECTOR2I& aCenter, int aRadius, DIRECTION_45::Directions aDir )
{
    switch( aDir )
    {
    case DIRECTION_45::NW:
        return SHAPE_ARC{
            aCenter,
            aCenter + VECTOR2I( -aRadius, 0 ),
            ANGLE_90,
        };
    case DIRECTION_45::NE:
        return SHAPE_ARC{
            aCenter,
            aCenter + VECTOR2I( 0, -aRadius ),
            ANGLE_90,
        };
    case DIRECTION_45::SW:
        return SHAPE_ARC{
            aCenter,
            aCenter + VECTOR2I( 0, aRadius ),
            ANGLE_90,
        };
    case DIRECTION_45::SE:
        return SHAPE_ARC{
            aCenter,
            aCenter + VECTOR2I( aRadius, 0 ),
            ANGLE_90,
        };
    default: wxFAIL_MSG( "Invalid direction" ); return SHAPE_ARC();
    }
}


SHAPE_ARC KIGEOM::MakeArcCw180( const VECTOR2I& aCenter, int aRadius,
                                DIRECTION_45::Directions aDir )
{
    switch( aDir )
    {
    case DIRECTION_45::N:
        return SHAPE_ARC{
            aCenter,
            aCenter + VECTOR2I( -aRadius, 0 ),
            ANGLE_180,
        };
    case DIRECTION_45::E:
        return SHAPE_ARC{
            aCenter,
            aCenter + VECTOR2I( 0, -aRadius ),
            ANGLE_180,
        };
    case DIRECTION_45::S:
        return SHAPE_ARC{
            aCenter,
            aCenter + VECTOR2I( aRadius, 0 ),
            ANGLE_180,
        };
    case DIRECTION_45::W:
        return SHAPE_ARC{
            aCenter,
            aCenter + VECTOR2I( 0, aRadius ),
            ANGLE_180,
        };
    default: wxFAIL_MSG( "Invalid direction" );
    }

    return SHAPE_ARC();
}


VECTOR2I KIGEOM::GetPoint( const SHAPE_RECT& aRect, DIRECTION_45::Directions aDir )
{
    const VECTOR2I nw = aRect.GetPosition();
    switch( aDir )
    {
        // clang-format off
    case DIRECTION_45::N:  return nw + VECTOR2I( aRect.GetWidth() / 2, 0 );
    case DIRECTION_45::E:  return nw + VECTOR2I( aRect.GetWidth(),     aRect.GetHeight() / 2 );
    case DIRECTION_45::S:  return nw + VECTOR2I( aRect.GetWidth() / 2, aRect.GetHeight() );
    case DIRECTION_45::W:  return nw + VECTOR2I( 0,                    aRect.GetHeight() / 2 );
    case DIRECTION_45::NW: return nw;
    case DIRECTION_45::NE: return nw + VECTOR2I( aRect.GetWidth(),     0 );
    case DIRECTION_45::SW: return nw + VECTOR2I( 0,                    aRect.GetHeight() );
    case DIRECTION_45::SE: return nw + VECTOR2I( aRect.GetWidth(),     aRect.GetHeight() );
    default: wxFAIL_MSG( "Invalid direction" );
        // clang-format on
    }
    return VECTOR2I();
}
