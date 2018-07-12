/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file geometry_utils.h
 * @brief a few functions useful in geometry calculations.
 */

#ifndef GEOMETRY_UTILS_H
#define GEOMETRY_UTILS_H

#include <math/vector2d.h>

/**
 * @return the number of segments to approximate a arc by segments
 * with a given max error (this number is >= 1)
 * @param aRadius is the radius od the circle or arc
 * @param aErrorMax is the max error
 * This is the max distance between the middle of a segment and the circle.
 * @param aArcAngleDegree is the arc angle in degrees
 */
int GetArcToSegmentCount( int aRadius, int aErrorMax, double aArcAngleDegree );

/**
 * @return the correction factor to approximate a circle by segments
 * @param aSegCountforCircle is the number of segments to approximate the circle
 *
 * When creating a polygon from a circle, the polygon is inside the circle.
 * Only corners are on the circle.
 * This is incorrect when building clearance areas of circles, that need to build
 * the equivalent polygon outside the circle
 * The correction factor is a scaling factor to apply to the radius to build a
 * polygon outside the circle (only the middle of each segment is on the circle
 */
double GetCircletoPolyCorrectionFactor( int aSegCountforCircle );

/**
 * Snap a vector onto the nearest 0, 45 or 90 degree line.
 *
 * The magnitude of the vector is NOT kept, instead the co-ordinates are
 * set equal (and/or opposite) or to zero as needed. The effect of this is
 * that if the starting vector is on a square grid, the resulting snapped
 * vector will still be on the same grid.

 * @param a vector to be snapped
 * @return the snapped vector
 */
template<typename T>
VECTOR2<T> GetVectorSnapped45( const VECTOR2<T>& aVec )
{
    auto newVec = aVec;
    const VECTOR2<T> absVec { std::abs( aVec.x ), std::abs( aVec.y ) };

    if ( absVec.x > absVec.y * 2 )
    {
        // snap along x-axis
        newVec.y = 0;
    }
    else if ( absVec.y > absVec.x * 2 )
    {
        // snap onto y-axis
        newVec.x = 0;
    }
    else if ( absVec.x > absVec.y )
    {
        // snap away from x-axis towards 45
        newVec.y = std::copysign( aVec.x, aVec.y );
    } else
    {
        // snap away from y-axis towards 45
        newVec.x = std::copysign( aVec.y, aVec.x );
    }

    return newVec;
}

#endif  // #ifndef GEOMETRY_UTILS_H

