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

#include "geometry/shape_utils.h"

#include <geometry/circle.h>
#include <geometry/seg.h>
#include <geometry/half_line.h>
#include <geometry/line.h>
#include <geometry/point_types.h>
#include <geometry/shape_poly_set.h>
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


OPT_VECTOR2I KIGEOM::GetSharedEndpoint( const SEG& aSegA, const SEG& aSegB )
{
    if( aSegA.A == aSegB.A || aSegA.A == aSegB.B )
    {
        return aSegA.A;
    }
    else if( aSegA.B == aSegB.A || aSegA.B == aSegB.B )
    {
        return aSegA.B;
    }

    return std::nullopt;
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


void KIGEOM::CollectBoxCorners( const BOX2I& aBox, std::vector<VECTOR2I>& aCorners )
{
    aCorners.push_back( { aBox.GetLeft(), aBox.GetTop() } );
    aCorners.push_back( { aBox.GetRight(), aBox.GetTop() } );
    aCorners.push_back( { aBox.GetRight(), aBox.GetBottom() } );
    aCorners.push_back( { aBox.GetLeft(), aBox.GetBottom() } );
}


std::vector<SEG> KIGEOM::GetSegsInDirection( const BOX2I& aBox, DIRECTION_45::Directions aDir )
{
    // clang-format off
    switch( aDir )
    {
    case DIRECTION_45::Directions::N:
        return { SEG{ { aBox.GetLeft(), aBox.GetTop() },
                      { aBox.GetRight(), aBox.GetTop() } } };
    case DIRECTION_45::Directions::E:
        return { SEG{ { aBox.GetRight(), aBox.GetTop() },
                      { aBox.GetRight(), aBox.GetBottom() } } };
    case DIRECTION_45::Directions::S:
        return { SEG{ { aBox.GetLeft(), aBox.GetBottom() },
                      { aBox.GetRight(), aBox.GetBottom() } } };
    case DIRECTION_45::Directions::W:
        return { SEG{ { aBox.GetLeft(), aBox.GetTop() },
                      { aBox.GetLeft(), aBox.GetBottom() } } };
    case DIRECTION_45::Directions::NE:
        return { SEG{ { aBox.GetLeft(), aBox.GetTop() },
                      { aBox.GetRight(), aBox.GetTop() } },
                 SEG{ { aBox.GetRight(), aBox.GetTop() },
                      { aBox.GetRight(), aBox.GetBottom() } } };
    case DIRECTION_45::Directions::SE:
        return { SEG{ { aBox.GetLeft(), aBox.GetBottom() },
                      { aBox.GetRight(), aBox.GetBottom() } },
                 SEG{ { aBox.GetRight(), aBox.GetTop() },
                      { aBox.GetRight(), aBox.GetBottom() } } };
    case DIRECTION_45::Directions::SW:
        return { SEG{ { aBox.GetLeft(), aBox.GetBottom() },
                      { aBox.GetRight(), aBox.GetBottom() } },
                 SEG{ { aBox.GetLeft(), aBox.GetTop() },
                      { aBox.GetLeft(), aBox.GetBottom() } } };
    case DIRECTION_45::Directions::NW:
        return { SEG{ { aBox.GetLeft(), aBox.GetTop() },
                      { aBox.GetRight(), aBox.GetTop() } },
                 SEG{ { aBox.GetLeft(), aBox.GetTop() },
                      { aBox.GetLeft(), aBox.GetBottom() } } };
    case DIRECTION_45::Directions::LAST:
    case DIRECTION_45::Directions::UNDEFINED: break;
    }
    // clang-format on

    wxASSERT( false );
    return {};
};


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


VECTOR2I KIGEOM::GetPoint( const SHAPE_RECT& aRect, DIRECTION_45::Directions aDir, int aOutset )
{
    const VECTOR2I nw = aRect.GetPosition();
    switch( aDir )
    {
        // clang-format off
    case DIRECTION_45::N:
        return nw + VECTOR2I( aRect.GetWidth() / 2, -aOutset );
    case DIRECTION_45::E:
        return nw + VECTOR2I( aRect.GetWidth() + aOutset, aRect.GetHeight() / 2 );
    case DIRECTION_45::S:
        return nw + VECTOR2I( aRect.GetWidth() / 2, aRect.GetHeight() + aOutset );
    case DIRECTION_45::W:
        return nw + VECTOR2I( -aOutset, aRect.GetHeight() / 2 );
    case DIRECTION_45::NW:
        return nw + VECTOR2I( -aOutset, -aOutset );
    case DIRECTION_45::NE:
        return nw + VECTOR2I( aRect.GetWidth() + aOutset, -aOutset );
    case DIRECTION_45::SW:
        return nw + VECTOR2I( -aOutset, aRect.GetHeight() + aOutset );
    case DIRECTION_45::SE:
        return nw + VECTOR2I( aRect.GetWidth() + aOutset, aRect.GetHeight() + aOutset );
    default:
        wxFAIL_MSG( "Invalid direction" );
        // clang-format on
    }
    return VECTOR2I();
}


std::vector<TYPED_POINT2I> KIGEOM::GetCircleKeyPoints( const CIRCLE& aCircle, bool aIncludeCenter )
{
    std::vector<TYPED_POINT2I> pts;

    if( aIncludeCenter )
    {
        pts.emplace_back( VECTOR2I{ 0, 0 }, POINT_TYPE::PT_CENTER );
    }

    pts.emplace_back( VECTOR2I{ 0, aCircle.Radius }, POINT_TYPE::PT_QUADRANT );
    pts.emplace_back( VECTOR2I{ aCircle.Radius, 0 }, POINT_TYPE::PT_QUADRANT );
    pts.emplace_back( VECTOR2I{ 0, -aCircle.Radius }, POINT_TYPE::PT_QUADRANT );
    pts.emplace_back( VECTOR2I{ -aCircle.Radius, 0 }, POINT_TYPE::PT_QUADRANT );

    // Shift the points to the circle center
    for( TYPED_POINT2I& pt : pts )
    {
        pt.m_point += aCircle.Center;
    }

    return pts;
}


SHAPE_LINE_CHAIN KIGEOM::RectifyPolygon( const SHAPE_LINE_CHAIN& aPoly )
{
    SHAPE_LINE_CHAIN raOutline;

    const auto handleSegment = [&]( const SEG& aSeg )
    {
        const VECTOR2I p0( aSeg.A.x, aSeg.B.y );
        const VECTOR2I p1( aSeg.B.x, aSeg.A.y );

        raOutline.Append( aSeg.A );
        if( !aPoly.PointInside( p0 ) )
            raOutline.Append( p0 );
        else
            raOutline.Append( p1 );
    };

    for( int i = 0; i < aPoly.SegmentCount(); i++ )
    {
        handleSegment( aPoly.CSegment( i ) );
    }

    // Manually handle the last segment if not closed
    if( !aPoly.IsClosed() && aPoly.PointCount() >= 2 )
    {
        handleSegment( SEG( aPoly.CLastPoint(), aPoly.CPoint( 0 ) ) );
    }

    raOutline.SetClosed( true );
    raOutline.Simplify();

    return raOutline;
}


bool KIGEOM::AddHoleIfValid( SHAPE_POLY_SET& aOutline, SHAPE_LINE_CHAIN&& aHole )
{
    if( aHole.PointCount() < 3 || aHole.Area() == 0 )
    {
        return false;
    }

    aOutline.AddHole( std::move( aHole ) );
    return true;
}


std::vector<VECTOR2I> KIGEOM::MakeRegularPolygonPoints( const VECTOR2I& aCenter, size_t aN,
                                                        const VECTOR2I& aPt0 )
{
    VECTOR2D              pt0FromC = aPt0 - aCenter;
    std::vector<VECTOR2I> pts;

    for( size_t i = 0; i < aN; i++ )
    {
        VECTOR2D pt = GetRotated( pt0FromC, ( FULL_CIRCLE / double( aN ) ) * i );
        pts.push_back( KiROUND( pt + aCenter ) );
    }

    return pts;
}


std::vector<VECTOR2I> KIGEOM::MakeRegularPolygonPoints( const VECTOR2I& aCenter, size_t aN,
                                                        int aRadius, bool aAcrossCorners,
                                                        EDA_ANGLE aAngle )
{
    if( !aAcrossCorners )
    {
        // if across flats, increase the radius
        aRadius = aRadius / ( FULL_CIRCLE / ( aN * 2 ) ).Cos();
    }

    const VECTOR2I pt0 = aCenter + GetRotated( VECTOR2I{ aRadius, 0 }, aAngle );
    return KIGEOM::MakeRegularPolygonPoints( aCenter, aN, pt0 );
}


std::vector<SEG> KIGEOM::MakeCrossSegments( const VECTOR2I& aCenter, const VECTOR2I& aSize,
                                            EDA_ANGLE aAngle )
{
    std::vector<SEG> segs;

    VECTOR2I pt0 = aCenter - GetRotated( VECTOR2I{ aSize.x / 2, 0 }, aAngle );
    segs.emplace_back( pt0, aCenter - ( pt0 - aCenter ) );

    pt0 = aCenter - GetRotated( VECTOR2I{ 0, aSize.y / 2 }, aAngle );
    segs.emplace_back( pt0, aCenter - ( pt0 - aCenter ) );

    return segs;
}
