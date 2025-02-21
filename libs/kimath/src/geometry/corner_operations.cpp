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

#include "geometry/corner_operations.h"

#include <geometry/circle.h>
#include <geometry/half_line.h>
#include <geometry/shape_arc.h>
#include <geometry/shape_poly_set.h>
#include <geometry/shape_utils.h>
#include <geometry/vector_utils.h>
#include <geometry/geometry_utils.h>
#include <trigo.h>

namespace
{

/**
 * Get the bisector of two segments that join at a corner.
 */
SEG GetBisectorOfCornerSegments( const SEG& aSegA, const SEG& aSegB, int aLength )
{
    // Use the "parallelogram" method to find the bisector

    // The intersection point of the two lines is the one that is shared by both segments
    const std::optional<VECTOR2I> corner = KIGEOM::GetSharedEndpoint( aSegA, aSegB );

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
    const std::optional<VECTOR2I> corner = KIGEOM::GetSharedEndpoint( aSegA, aSegB );

    if( !corner )
    {
        // The lines are not coterminous
        return std::nullopt;
    }

    // These are the other existing line points (the ones that are not the intersection)
    const VECTOR2I& a_end_pt = KIGEOM::GetOtherEnd( aSegA, *corner );
    const VECTOR2I& b_end_pt = KIGEOM::GetOtherEnd( aSegB, *corner );

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
                                              int aDogboneRadius, bool aAddSlots )
{
    const std::optional<VECTOR2I> corner = KIGEOM::GetSharedEndpoint( aSegA, aSegB );

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

    SHAPE_ARC arc( *ptOnSegA, *corner, *ptOnSegB, 0 );

    const VECTOR2I& aOtherPtA = KIGEOM::GetOtherEnd( aSegA, *corner );
    const VECTOR2I& aOtherPtB = KIGEOM::GetOtherEnd( aSegB, *corner );

    const EDA_ANGLE angle_epsilon( 1e-3, EDA_ANGLE_T::DEGREES_T );
    const bool small_arc_mouth = std::abs( arc.GetCentralAngle() ) > ( ANGLE_180 + angle_epsilon );

    {
        // See if we need to update the original segments
        // or if the dogbone consumed them
        std::optional<SEG> new_a, new_b;
        if( aOtherPtA != *ptOnSegA )
            new_a = SEG{ aOtherPtA, *ptOnSegA };

        if( aOtherPtB != *ptOnSegB )
            new_b = SEG{ aOtherPtB, *ptOnSegB };

        // Nice and easy
        if( !small_arc_mouth || !aAddSlots )
        {
            return DOGBONE_RESULT{
                arc,
                new_a,
                new_b,
                small_arc_mouth,
            };
        }
    }

    // If it's a small mouth, we can try to work out the minimal slot to allow

    // First the arc will be pulled back to 180 degrees
    SHAPE_ARC slotArc = SHAPE_ARC( GetRotated( *corner, dogboneCenter, ANGLE_90 ), *corner,
                                   GetRotated( *corner, dogboneCenter, -ANGLE_90 ), 0 );

    // Make sure P0 is still the 'A' end
    if( !KIGEOM::PointsAreInSameDirection( slotArc.GetP0(), arc.GetP0(), dogboneCenter ) )
    {
        slotArc.Reverse();
    }

    // Take the bisector and glue it to the arc ends
    const HALF_LINE arc_extension_a{
        slotArc.GetP0(),
        slotArc.GetP0() + ( dogboneCenter - *corner ),
    };
    const HALF_LINE arc_extension_b{
        slotArc.GetP1(),
        slotArc.GetP1() + ( dogboneCenter - *corner ),
    };

    const OPT_VECTOR2I ext_a_intersect = arc_extension_a.Intersect( aSegA );
    const OPT_VECTOR2I ext_b_intersect = arc_extension_b.Intersect( aSegB );

    if( !ext_a_intersect || !ext_b_intersect )
    {
        // The arc extensions don't intersect the original segments
        return std::nullopt;
    }

    {
        // See if we need to update the original segments
        // or if the dogbone consumed them
        std::optional<SEG> new_a, new_b;
        if( aOtherPtA != *ext_a_intersect )
            new_a = SEG{ aOtherPtA, *ext_a_intersect };

        if( aOtherPtB != *ext_b_intersect )
            new_b = SEG{ aOtherPtB, *ext_b_intersect };

        return DOGBONE_RESULT{
            slotArc,
            new_a,
            new_b,
            small_arc_mouth,
        };
    }
}


SHAPE_POLY_SET::POLYGON SHAPE_POLY_SET::chamferFilletPolygon( CORNER_MODE aMode,
                                                              unsigned int aDistance,
                                                              int aIndex, int aErrorMax )
{
    // Null segments create serious issues in calculations. Remove them:
    RemoveNullSegments();

    SHAPE_POLY_SET::POLYGON currentPoly = Polygon( aIndex );
    SHAPE_POLY_SET::POLYGON newPoly;

    // If the chamfering distance is zero, then the polygon remain intact.
    if( aDistance == 0 )
    {
        return currentPoly;
    }

    // Iterate through all the contours (outline and holes) of the polygon.
    for( SHAPE_LINE_CHAIN& currContour : currentPoly )
    {
        // Generate a new contour in the new polygon
        SHAPE_LINE_CHAIN newContour;

        // Iterate through the vertices of the contour
        for( int currVertex = 0; currVertex < currContour.PointCount(); currVertex++ )
        {
            // Current vertex
            int x1 = currContour.CPoint( currVertex ).x;
            int y1 = currContour.CPoint( currVertex ).y;

            // Indices for previous and next vertices.
            int prevVertex;
            int nextVertex;

            // Previous and next vertices indices computation. Necessary to manage the edge cases.

            // Previous vertex is the last one if the current vertex is the first one
            prevVertex = currVertex == 0 ? currContour.PointCount() - 1 : currVertex - 1;

            // next vertex is the first one if the current vertex is the last one.
            nextVertex = currVertex == currContour.PointCount() - 1 ? 0 : currVertex + 1;

            // Previous vertex computation
            double xa = currContour.CPoint( prevVertex ).x - x1;
            double ya = currContour.CPoint( prevVertex ).y - y1;

            // Next vertex computation
            double xb = currContour.CPoint( nextVertex ).x - x1;
            double yb = currContour.CPoint( nextVertex ).y - y1;

            // Avoid segments that will generate nans below
            if( std::abs( xa + xb ) < std::numeric_limits<double>::epsilon()
                    && std::abs( ya + yb ) < std::numeric_limits<double>::epsilon() )
            {
                continue;
            }

            // Compute the new distances
            double  lena    = hypot( xa, ya );
            double  lenb    = hypot( xb, yb );

            // Make the final computations depending on the mode selected, chamfered or filleted.
            if( aMode == CORNER_MODE::CHAMFERED )
            {
                double distance = aDistance;

                // Chamfer one half of an edge at most
                if( 0.5 * lena < distance )
                    distance = 0.5 * lena;

                if( 0.5 * lenb < distance )
                    distance = 0.5 * lenb;

                int nx1 = KiROUND( distance * xa / lena );
                int ny1 = KiROUND( distance * ya / lena );

                newContour.Append( x1 + nx1, y1 + ny1 );

                int nx2 = KiROUND( distance * xb / lenb );
                int ny2 = KiROUND( distance * yb / lenb );

                newContour.Append( x1 + nx2, y1 + ny2 );
            }
            else    // CORNER_MODE = FILLETED
            {
                double cosine = ( xa * xb + ya * yb ) / ( lena * lenb );

                double  radius  = aDistance;
                double  denom   = sqrt( 2.0 / ( 1 + cosine ) - 1 );

                // Do nothing in case of parallel edges
                if( std::isinf( denom ) )
                    continue;

                // Limit rounding distance to one half of an edge
                if( 0.5 * lena * denom < radius )
                    radius = 0.5 * lena * denom;

                if( 0.5 * lenb * denom < radius )
                    radius = 0.5 * lenb * denom;

                // Calculate fillet arc absolute center point (xc, yx)
                double  k = radius / sqrt( .5 * ( 1 - cosine ) );
                double  lenab = sqrt( ( xa / lena + xb / lenb ) * ( xa / lena + xb / lenb ) +
                        ( ya / lena + yb / lenb ) * ( ya / lena + yb / lenb ) );
                double  xc  = x1 + k * ( xa / lena + xb / lenb ) / lenab;
                double  yc  = y1 + k * ( ya / lena + yb / lenb ) / lenab;

                // Calculate arc start and end vectors
                k = radius / sqrt( 2 / ( 1 + cosine ) - 1 );
                double  xs  = x1 + k * xa / lena - xc;
                double  ys  = y1 + k * ya / lena - yc;
                double  xe  = x1 + k * xb / lenb - xc;
                double  ye  = y1 + k * yb / lenb - yc;

                // Cosine of arc angle
                double argument = ( xs * xe + ys * ye ) / ( radius * radius );

                // Make sure the argument is in [-1,1], interval in which the acos function is
                // defined
                if( argument < -1 )
                    argument = -1;
                else if( argument > 1 )
                    argument = 1;

                double arcAngle = acos( argument );
                int    segments = GetArcToSegmentCount( radius, aErrorMax,
                                                        EDA_ANGLE( arcAngle, RADIANS_T ) );

                double deltaAngle = arcAngle / segments;
                double startAngle = atan2( -ys, xs );

                // Flip arc for inner corners
                if( xa * yb - ya * xb <= 0 )
                    deltaAngle *= -1;

                double  nx  = xc + xs;
                double  ny  = yc + ys;

                if( std::isnan( nx ) || std::isnan( ny ) )
                    continue;

                newContour.Append( KiROUND( nx ), KiROUND( ny ) );

                // Store the previous added corner to make a sanity check
                int prevX   = KiROUND( nx );
                int prevY   = KiROUND( ny );

                for( int j = 0; j < segments; j++ )
                {
                    nx = xc + cos( startAngle + ( j + 1 ) * deltaAngle ) * radius;
                    ny = yc - sin( startAngle + ( j + 1 ) * deltaAngle ) * radius;

                    if( std::isnan( nx ) || std::isnan( ny ) )
                        continue;

                    // Sanity check: the rounding can produce repeated corners; do not add them.
                    if( KiROUND( nx ) != prevX || KiROUND( ny ) != prevY )
                    {
                        newContour.Append( KiROUND( nx ), KiROUND( ny ) );
                        prevX   = KiROUND( nx );
                        prevY   = KiROUND( ny );
                    }
                }
            }
        }

        // Close the current contour and add it the new polygon
        newContour.SetClosed( true );
        newPoly.push_back( newContour );
    }

    return newPoly;
}


