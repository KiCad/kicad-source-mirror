/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <math/vector2d.h>

class SEG;

namespace KIGEOM
{

/**
 * @file vector_utils.h
 *
 * Supplemental functions for working with vectors and
 * simple objects that interact with vectors.
 */

/*
 * Determine if a point is in a given direction from another point.
 *
 * This returns true if the vector from aFrom to aPoint is within
 * 90 degrees of aDirection.
 *
 *     ------> aDirection
 *
 *     /-- aFrom
 *    O                /-- aPoint
 *                    O
 *
 * If the point is perpendicular to the direction, it is considered
 * to NOT be in the direction (e.g. both the direction and the
 * reversed direction would return false).
 *
 * @param aPoint The point to test.
 * @param aDirection The direction vector.
 * @param aFrom The point to test from.
 *
 * @return true if the point is in the direction.
 */
template <typename T>
bool PointIsInDirection( const VECTOR2<T>& aPoint, const VECTOR2<T>& aDirection,
                         const VECTOR2<T>& aFrom )
{
    return ( aPoint - aFrom ).Dot( aDirection ) > 0;
}


/**
 * Check that the two given points are in the same direction from some other point.
 *
 * I.e. the vectors from aFrom to aPointA and aFrom to aPointB are within 90 degrees.
 */
template <typename T>
bool PointsAreInSameDirection( const VECTOR2<T>& aPointA, const VECTOR2<T>& aPointB,
                               const VECTOR2<T>& aFrom )
{
    return PointIsInDirection( aPointB, aPointA - aFrom, aFrom );
}


/**
 * Determine if a segment's vector is within 90 degrees of a given direction.
 */
bool SegIsInDirection( const SEG& aSeg, const VECTOR2I& aDirection );


/**
 * Determine if a point projects onto a segment.
 *
 *       /--- projects      /--- does not project
 *      o                  o
 *      |                  |
 * |<------------>|        x
 *      aSeg
 */
bool PointProjectsOntoSegment( const VECTOR2I& aPoint, const SEG& aSeg );


/**
 * Get the ratio of the vector to a point from the segment's start,
 * compared to the segment's length.
 *
 *        /--- aPoint
 *  A<---+-------->B  <-- Length L
 *  |    |
 * >|----|<-- Length R
 *
 * The point doesn't have to lie on the segment.
 */
double GetLengthRatioFromStart( const VECTOR2I& aPoint, const SEG& aSeg );

/**
 * Get the ratio of the vector to a point projected onto a segment
 * from the start, relative to the segment's length.
 *
 *       /--- projects
 *      o
 *      |
 *  A<---+-------->B  <-- Length L
 *  |    |
 * >|----|<-- Length R
 *
 * The ratio is R / L. IF 0, the point is at A. If 1, the point is at B.
 * It assumes the point projects onto the segment.
 */
double GetProjectedPointLengthRatio( const VECTOR2I& aPoint, const SEG& aSeg );


/**
 * Get the nearest end of a segment to a point.
 *
 * If equidistant, the start point is returned.
 */
const VECTOR2I& GetNearestEndpoint( const SEG& aSeg, const VECTOR2I& aPoint );

/**
 * Round a vector to the nearest grid point in any direction.
 */
VECTOR2I RoundGrid( const VECTOR2I& aVec, int aGridSize );

/**
 * Round a vector to the nearest grid point in the NW direction.
 *
 * This means x and y are both rounded downwards (regardless of sign).
 */
VECTOR2I RoundNW( const VECTOR2I& aVec, int aGridSize );

/**
 * Round a vector to the nearest grid point in the SE direction.
 *
 * This means x and y are both rounded upwards (regardless of sign).
 */
VECTOR2I RoundSE( const VECTOR2I& aVec, int aGridSize );

} // namespace KIGEOM
