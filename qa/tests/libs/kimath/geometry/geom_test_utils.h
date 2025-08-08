/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#ifndef GEOM_TEST_UTILS_H
#define GEOM_TEST_UTILS_H

#include <cmath>

#include <geometry/point_types.h>
#include <geometry/seg.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_poly_set.h>

#include <qa_utils/numeric.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

/**
 * @brief Utility functions for testing geometry functions.
 */
namespace GEOM_TEST
{

/**
 * @brief Geometric quadrants, from top-right, anti-clockwise
 *
 *     ^ y
 *     |
 *  Q2 | Q1
 *  -------> x
 *  Q3 | Q4
 */
enum class QUADRANT {
    Q1, Q2, Q3, Q4
};

/*
 * @brief Check value in Quadrant 1 (x and y both >= 0)
 */
template<typename T>
bool IsInQuadrant( const VECTOR2<T>& aPoint, QUADRANT aQuadrant )
{
    bool isInQuad = false;

    switch( aQuadrant )
    {
        case QUADRANT::Q1:
            isInQuad = aPoint.x >= 0 && aPoint.y >= 0;
            break;
        case QUADRANT::Q2:
            isInQuad = aPoint.x <= 0 && aPoint.y >= 0;
            break;
        case QUADRANT::Q3:
            isInQuad = aPoint.x <= 0 && aPoint.y <= 0;
            break;
        case QUADRANT::Q4:
            isInQuad = aPoint.x >= 0 && aPoint.y <= 0;
            break;
    }

    return isInQuad;
}

/*
 * @Brief Check if both ends of a segment are in Quadrant 1
 */
inline bool SegmentCompletelyInQuadrant( const SEG& aSeg, QUADRANT aQuadrant )
{
    return IsInQuadrant( aSeg.A, aQuadrant)
            && IsInQuadrant( aSeg.B, aQuadrant );
}

/*
 * @brief Check if at least one end of the segment is in Quadrant 1
 */
inline bool SegmentEndsInQuadrant( const SEG& aSeg, QUADRANT aQuadrant )
{
    return IsInQuadrant( aSeg.A, aQuadrant )
            || IsInQuadrant( aSeg.B, aQuadrant );
}

/*
 * @brief Check if a segment is entirely within a certain radius of a point.
 */
inline bool SegmentCompletelyWithinRadius( const SEG& aSeg, const VECTOR2I& aPt, const int aRadius )
{
    // This is true iff both ends of the segment are within the radius
    return ( ( aSeg.A - aPt ).EuclideanNorm() < aRadius )
            && ( ( aSeg.B - aPt ).EuclideanNorm() < aRadius );
}

/**
 * Check that two points are the given distance apart, within the given tolerance.
 *
 * @tparam T the dimension type
 * @param aPtA the first point
 * @param aPtB the second point
 * @param aExpDist the expected distance
 * @param aTol the permitted tolerance
 */
template <typename T>
bool IsPointAtDistance( const VECTOR2<T>& aPtA, const VECTOR2<T>& aPtB, T aExpDist, T aTol )
{
    const int  dist = ( aPtB - aPtA ).EuclideanNorm();
    const bool ok = KI_TEST::IsWithin( dist, aExpDist, aTol );

    if( !ok )
    {
        BOOST_TEST_INFO( "Points not at expected distance: distance is " << dist << ", expected "
                                                                         << aExpDist );
    }

    return ok;
}

/**
 * Predicate for checking a set of points is within a certain tolerance of
 * a circle
 * @param  aPoints   the points to check
 * @param  aCentre   the circle centre
 * @param  aRad      the circle radius
 * @param  aTolEnds  the tolerance for the endpoint-centre distance
 * @return           true if predicate met
 */
template <typename T>
bool ArePointsNearCircle(
        const std::vector<VECTOR2<T>>& aPoints, const VECTOR2<T>& aCentre, T aRad, T aTol )
{
    bool ok = true;

    for( unsigned i = 0; i < aPoints.size(); ++i )
    {
        if( !IsPointAtDistance( aPoints[i], aCentre, aRad, aTol ) )
        {
            BOOST_TEST_INFO( "Point " << i << " " << aPoints[i] << " is not within tolerance ("
                                      << aTol << ") of radius (" << aRad << ") from centre point "
                                      << aCentre );
            ok = false;
        }
    }

    return ok;
}

/*
 * @brief Check if two vectors are perpendicular
 *
 * @param a: vector A
 * @param b: vector B
 * @param aTolerance: the allowed deviation from PI/2 (e.g. when rounding)
 */

template<typename T>
bool ArePerpendicular( const VECTOR2<T>& a, const VECTOR2<T>& b, const EDA_ANGLE& aTolerance )
{
    EDA_ANGLE angle = std::abs( EDA_ANGLE( a ) - EDA_ANGLE( b ) );

    // Normalise: angles of 3*pi/2 are also perpendicular
    if (angle > ANGLE_180)
        angle -= ANGLE_180;

    return KI_TEST::IsWithin( angle.AsRadians(), ANGLE_90.AsRadians(), aTolerance.AsRadians() );
}

/*
 * @brief Fillet every polygon in a set and return a new set
 */
inline SHAPE_POLY_SET FilletPolySet( SHAPE_POLY_SET& aPolySet, int aRadius, int aError )
{
    SHAPE_POLY_SET filletedPolySet;

    for ( int i = 0; i < aPolySet.OutlineCount(); ++i )
    {
        const auto filleted = aPolySet.FilletPolygon( aRadius, aError, i );

        filletedPolySet.AddOutline( filleted[0] );
    }

    return filletedPolySet;
}

/**
 * Verify that a SHAPE_LINE_CHAIN has been assembled correctly by ensuring that the
 * arc start and end points match points on the chain and that any points inside the arcs
 * actually collide with the arc segments (with an error margin of 5000 IU)
 *
 * @param aChain to test
 * @return true if outline is valid
*/
inline bool IsOutlineValid( const SHAPE_LINE_CHAIN& aChain )
{
    ssize_t           prevArcIdx = -1;
    std::set<size_t> testedArcs;

    if( aChain.PointCount() > 0 && !aChain.IsClosed() && aChain.IsSharedPt( 0 ) )
        return false; //can't have first point being shared on an open chain

    for( int i = 0; i < aChain.PointCount(); i++ )
    {
        ssize_t arcIdx = aChain.ArcIndex( i );

        if( arcIdx >= 0 )
        {
            // Point on arc, lets make sure it collides with the arc shape and we haven't
            // previously seen the same arc index

            if( prevArcIdx != arcIdx && testedArcs.count( arcIdx ) )
                return false; // we've already seen this arc before, not contiguous

            if( !aChain.Arc( arcIdx ).Collide( aChain.CPoint( i ),
                                               SHAPE_ARC::DefaultAccuracyForPCB() ) )
            {
                return false;
            }

            testedArcs.insert( arcIdx );
        }

        if( prevArcIdx != arcIdx )
        {
            // we have changed arc shapes, run a few extra tests

            if( prevArcIdx >= 0 )
            {
                // prev point on arc, test that the last arc point on the chain
                // matches the end point of the arc
                VECTOR2I pointToTest = aChain.CPoint( i );

                if( !aChain.IsSharedPt( i ) )
                    pointToTest = aChain.CPoint( i - 1 );

                SHAPE_ARC lastArc = aChain.Arc( prevArcIdx );

                if( lastArc.GetP1() != pointToTest )
                    return false;
            }

            if( arcIdx >= 0 )
            {
                // new arc, test that the start point of the arc matches the point on the chain
                VECTOR2I pointToTest = aChain.CPoint( i );
                SHAPE_ARC currentArc = aChain.Arc( arcIdx );

                if( currentArc.GetP0() != pointToTest )
                    return false;
            }
        }

        prevArcIdx = arcIdx;
    }

    // Make sure last arc point matches the end of the arc
    if( prevArcIdx >= 0 )
    {
        if( aChain.IsClosed() && aChain.IsSharedPt( 0 ) )
        {
            if( aChain.CShapes()[0].first != prevArcIdx )
                return false;

            if( aChain.Arc( prevArcIdx ).GetP1() != aChain.CPoint( 0 ) )
                return false;
        }
        else
        {
            if( aChain.Arc( prevArcIdx ).GetP1() != aChain.CLastPoint() )
                return false;
        }
    }

    return true;
}

/**
 * Verify that a SHAPE_POLY_SET has been assembled correctly by verifying each of the outlines
 * and holes contained within
 *
 * @param aSet to test
 * @return true if the poly set is valid
*/
inline bool IsPolySetValid( const SHAPE_POLY_SET& aSet )
{
    for( int i = 0; i < aSet.OutlineCount(); i++ )
    {
        if( !IsOutlineValid( aSet.Outline( i ) ) )
            return false;

        for( int j = 0; j < aSet.HoleCount( i ); j++ )
        {
            if( !IsOutlineValid( aSet.CHole( i, j ) ) )
                return false;
        }
    }

    return true;
}

/**
 * @brief Check that two SEGs have the same end points, in either order
 *
 * That is to say SEG(A, B) == SEG(A, B), but also SEG(A, B) == SEG(B, A)
 */
inline bool SegmentsHaveSameEndPoints( const SEG& aSeg1, const SEG& aSeg2 )
{
    return ( aSeg1.A == aSeg2.A && aSeg1.B == aSeg2.B )
           || ( aSeg1.A == aSeg2.B && aSeg1.B == aSeg2.A );
}

} // namespace GEOM_TEST


// Stream printing for geometry types

std::ostream& boost_test_print_type( std::ostream& os, const SHAPE_LINE_CHAIN& c );

// Not clear why boost_test_print_type doesn't work on Debian specifically for this type,
// but this works on all platforms
std::ostream& operator<<( std::ostream& os, const TYPED_POINT2I& c );

#endif // GEOM_TEST_UTILS_H
