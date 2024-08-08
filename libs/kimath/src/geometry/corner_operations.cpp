/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023-2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "geometry/corner_operations.h"

#include <geometry/circle.h>

namespace
{

/**
 * Get the shared endpoint of two segments, if it exists, or std::nullopt
 * if the segments are not connected end-to-end.
 */
std::optional<VECTOR2I> GetSharedEndpoint( const SEG& aSegA, const SEG& aSegB )
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

/**
 * Get the end point of the segment that is _not_ the given point.
 */
VECTOR2I GetOtherPoint( const SEG& aSeg, const VECTOR2I& aPoint )
{
    return ( aSeg.A == aPoint ) ? aSeg.B : aSeg.A;
}


/**
 * Get the bisector of two segments that join at a corner.
 */
SEG GetBisectorOfCornerSegments( const SEG& aSegA, const SEG& aSegB, int aLength )
{
    // Use the "parallelogram" method to find the bisector

    // The intersection point of the two lines is the one that is shared by both segments
    const std::optional<VECTOR2I> corner = GetSharedEndpoint( aSegA, aSegB );

    // Get the vector of a segment pointing away from a point
    const auto getSegVectorPointingAwayFrom = []( const SEG&      aSeg,
                                                  const VECTOR2I& aPoint ) -> VECTOR2I
    {
        const int distA = ( aSeg.A - aPoint ).EuclideanNorm();
        const int distB = ( aSeg.B - aPoint ).EuclideanNorm();

        // If A is closer the segment is already pointing away from the corner
        // If B is closer, we need to reverse the segment
        return ( distA < distB ) ? ( aSeg.B - aSeg.A ) : ( aSeg.A - aSeg.B );
    };

    // The vectors have to be the same length
    const int      maxLen = std::max( aSegA.Length(), aSegB.Length() );
    const VECTOR2I aVecOutward = getSegVectorPointingAwayFrom( aSegA, *corner ).Resize( maxLen );
    const VECTOR2I bVecOutward = getSegVectorPointingAwayFrom( aSegB, *corner ).Resize( maxLen );
    const VECTOR2I bisectorOutward = aVecOutward + bVecOutward;

    return SEG( *corner, *corner + bisectorOutward.Resize( aLength ) );
}

} // namespace

std::optional<CHAMFER_RESULT> ComputeChamferPoints( const SEG& aSegA, const SEG& aSegB,
                                                    const CHAMFER_PARAMS& aChamferParams )
{
    const int line_a_setback = aChamferParams.m_chamfer_setback_a;
    const int line_b_setback = aChamferParams.m_chamfer_setback_b;

    if( line_a_setback == 0 && line_b_setback == 0 )
    {
        // No chamfer to do
        // In theory a chamfer of 0 on one side is kind-of valid (adds a collinear point)
        // so allow it (using an and above, not an or)
        return std::nullopt;
    }

    if( aSegA.Length() < line_a_setback || aSegB.Length() < line_b_setback )
    {
        // Chamfer is too big for the line segments
        return std::nullopt;
    }

    // We only support the case where the lines intersect at the end points
    // otherwise we would need to decide which inside corner to chamfer

    // Figure out which end points are the ones at the intersection
    const std::optional<VECTOR2I> corner = GetSharedEndpoint( aSegA, aSegB );

    if( !corner )
    {
        // The lines are not coterminous
        return std::nullopt;
    }

    // These are the other existing line points (the ones that are not the intersection)
    const VECTOR2I a_end_pt = GetOtherPoint( aSegA, *corner );
    const VECTOR2I b_end_pt = GetOtherPoint( aSegB, *corner );

    // Now, construct segment of the set-back lengths, that begins
    // at the intersection point and is parallel to each line segments
    SEG setback_a( *corner, *corner + VECTOR2I( a_end_pt - *corner ).Resize( line_a_setback ) );
    SEG setback_b( *corner, *corner + VECTOR2I( b_end_pt - *corner ).Resize( line_b_setback ) );

    // The chamfer segment then goes between the end points of the set-back segments
    SEG chamfer( setback_a.B, setback_b.B );

    // The adjusted segments go from the old end points to the chamfer ends

    std::optional<SEG> new_a;

    if( a_end_pt != chamfer.A )
        new_a = SEG{ a_end_pt, chamfer.A };

    std::optional<SEG> new_b;

    if( b_end_pt != chamfer.B )
        new_b = SEG{ b_end_pt, chamfer.B };

    return CHAMFER_RESULT{ chamfer, new_a, new_b };
}


std::optional<DOGBONE_RESULT> ComputeDogbone( const SEG& aSegA, const SEG& aSegB,
                                              int aDogboneRadius )
{
    const std::optional<VECTOR2I> corner = GetSharedEndpoint( aSegA, aSegB );

    // Cannot handle parallel lines
    if( !corner || aSegA.Angle( aSegB ).IsHorizontal() )
    {
        return std::nullopt;
    }

    const SEG bisector = GetBisectorOfCornerSegments( aSegA, aSegB, aDogboneRadius );

    // The dogbone center is the end of the bisector
    const VECTOR2I dogboneCenter = bisector.B;

    // We can find the ends of the arc by considering the corner angle,
    // but it's easier to just intersect the circle with the original segments.

    const CIRCLE circle( dogboneCenter, aDogboneRadius );

    const std::vector<VECTOR2I> segAIntersections = circle.Intersect( aSegA );
    const std::vector<VECTOR2I> segBIntersections = circle.Intersect( aSegB );

    // There can be a little bit of error in the intersection calculation
    static int s_epsilon = 8;

    const auto getPointNotOnCorner =
            [&]( const std::vector<VECTOR2I>& aIntersections, const VECTOR2I& aCorner )
    {
        std::optional<VECTOR2I> result;
        for( const VECTOR2I& pt : aIntersections )
        {
            if( aCorner.Distance( pt ) > s_epsilon )
            {
                result = pt;
                break;
            }
        }
        return result;
    };

    const std::optional<VECTOR2I> ptOnSegA = getPointNotOnCorner( segAIntersections, *corner );
    const std::optional<VECTOR2I> ptOnSegB = getPointNotOnCorner( segBIntersections, *corner );

    // The arc doesn't cross one or both of the segments
    if( !ptOnSegA || !ptOnSegB )
    {
        return std::nullopt;
    }

    const int mouth_width = SEG( *ptOnSegA, *ptOnSegB ).Length();

    const VECTOR2I aOtherPtA = GetOtherPoint( aSegA, *corner );
    const VECTOR2I aOtherPtB = GetOtherPoint( aSegB, *corner );

    // See if we need to update the original segments
    // or if the dogbone consumed them
    std::optional<SEG> new_a, new_b;
    if( aOtherPtA != *ptOnSegA )
        new_a = SEG{ aOtherPtA, *ptOnSegA };

    if( aOtherPtB != *ptOnSegB )
        new_b = SEG{ aOtherPtB, *ptOnSegB };

    return DOGBONE_RESULT{
        *ptOnSegA, *corner, *ptOnSegB, new_a, new_b, mouth_width < aDogboneRadius,
    };
}