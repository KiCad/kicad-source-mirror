/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#include <math.h>

#include <geometry/seg.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_poly_set.h>

#include <unit_test_utils/numeric.h>

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
bool SegmentCompletelyInQuadrant( const SEG& aSeg, QUADRANT aQuadrant )
{
    return IsInQuadrant( aSeg.A, aQuadrant)
            && IsInQuadrant( aSeg.B, aQuadrant );
}

/*
 * @brief Check if at least one end of the segment is in Quadrant 1
 */
bool SegmentEndsInQuadrant( const SEG& aSeg, QUADRANT aQuadrant )
{
    return IsInQuadrant( aSeg.A, aQuadrant )
            || IsInQuadrant( aSeg.B, aQuadrant );
}

/*
 * @brief Check if a segment is entirely within a certain radius of a point.
 */
bool SegmentCompletelyWithinRadius( const SEG& aSeg, const VECTOR2I& aPt,
    const int aRadius )
{
    // This is true iff both ends of the segment are within the radius
    return ( ( aSeg.A - aPt ).EuclideanNorm() < aRadius )
            && ( ( aSeg.B - aPt ).EuclideanNorm() < aRadius );
}

/*
 * @brief Check if two vectors are perpendicular
 *
 * @param a: vector A
 * @param b: vector B
 * @param aTolerance: the allowed deviation from PI/2 (e.g. when rounding)
 */

template<typename T>
bool ArePerpendicular( const VECTOR2<T>& a, const VECTOR2<T>& b, double aTolerance )
{
    auto angle = std::abs( a.Angle() - b.Angle() );

    // Normalise: angles of 3*pi/2 are also perpendicular
    if (angle > M_PI)
    {
        angle -= M_PI;
    }

    return KI_TEST::IsWithin( angle, M_PI / 2.0, aTolerance );
}

/**
 * @brief construct a square polygon of given size width and centre
 *
 * @param aSize: the side width (must be divisible by 2 if want to avoid rounding)
 * @param aCentre: the centre of the square
 */
SHAPE_LINE_CHAIN MakeSquarePolyLine( int aSize, const VECTOR2I& aCentre )
{
    SHAPE_LINE_CHAIN polyLine;

    const VECTOR2I corner = aCentre + aSize / 2;

    polyLine.Append( VECTOR2I( corner.x, corner.y ) );
    polyLine.Append( VECTOR2I( -corner.x, corner.y ) ) ;
    polyLine.Append( VECTOR2I( -corner.x, -corner.y ) );
    polyLine.Append( VECTOR2I( corner.x, -corner.y ) );

    polyLine.SetClosed( true );

    return polyLine;
}

/*
 * @brief Fillet every polygon in a set and return a new set
 */
SHAPE_POLY_SET FilletPolySet( SHAPE_POLY_SET& aPolySet, int aRadius,
     int aError )
{
    SHAPE_POLY_SET filletedPolySet;

    for ( int i = 0; i < aPolySet.OutlineCount(); ++i )
    {
        const auto filleted = aPolySet.FilletPolygon( aRadius, aError, i );

        filletedPolySet.AddOutline( filleted[0] );
    }

    return filletedPolySet;
}

}

#endif // GEOM_TEST_UTILS_H