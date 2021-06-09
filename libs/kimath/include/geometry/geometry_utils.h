/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <math.h>           // for copysign
#include <stdlib.h>         // for abs

#include <math/vector2d.h>

class EDA_RECT;

/**
 * When approximating an arc or circle, should the error be placed on the outside
 * or inside of the curve?  (Generally speaking filled shape errors go on the inside
 * and knockout errors go on the outside.  This preserves minimum clearances.)
 */
enum ERROR_LOC { ERROR_OUTSIDE, ERROR_INSIDE };

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
 * When creating polygons to create a clearance polygonal area, the polygon must
 * be same or bigger than the original shape.
 * Polygons are bigger if the original shape has arcs (round rectangles, ovals,
 * circles...).  However, when building the solder mask layer modifying the shapes
 * when converting them to polygons is not acceptable (the modification can break
 * calculations).
 * So one can disable the shape expansion within a particular scope by allocating
 * a DISABLE_ARC_CORRECTION.
 */
class DISABLE_ARC_RADIUS_CORRECTION
{
public:
    DISABLE_ARC_RADIUS_CORRECTION();
    ~DISABLE_ARC_RADIUS_CORRECTION();
};

/**
 * @return the radius correction to approximate a circle.
 * @param aMaxError is the same error value used to calculate the number of segments.
 *
 * When creating a polygon from a circle, the polygon is inside the circle.
 * Only corners are on the circle.
 * This is incorrect when building clearance areas of circles, that need to build
 * the equivalent polygon outside the circle.
 */
int GetCircleToPolyCorrection( int aMaxError );

/**
 * Snap a vector onto the nearest 0, 45 or 90 degree line.
 *
 * The magnitude of the vector is NOT kept, instead the coordinates are
 * set equal (and/or opposite) or to zero as needed. The effect of this is
 * that if the starting vector is on a square grid, the resulting snapped
 * vector will still be on the same grid.

 * @param a vector to be snapped
 * @return the snapped vector
 */
template<typename T>
VECTOR2<T> GetVectorSnapped45( const VECTOR2<T>& aVec, bool only45 = false )
{
    auto newVec = aVec;
    const VECTOR2<T> absVec { std::abs( aVec.x ), std::abs( aVec.y ) };

    if ( !only45 && absVec.x > absVec.y * 2 )
    {
        // snap along x-axis
        newVec.y = 0;
    }
    else if ( !only45 && absVec.y > absVec.x * 2 )
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


/**
 * Test if any part of a line falls within the bounds of a rectangle.
 *
 * Please note that this is only accurate for lines that are one pixel wide.
 *
 * @param aClipBox - The rectangle to test.
 * @param x1 - X coordinate of one end of a line.
 * @param y1 - Y coordinate of one end of a line.
 * @param x2 - X coordinate of the other end of a line.
 * @param y2 - Y coordinate of the other  end of a line.
 *
 * @return - False if any part of the line lies within the rectangle.
 */
bool ClipLine( const EDA_RECT *aClipBox, int &x1, int &y1, int &x2, int &y2 );


/**
 * Dashed and dotted line patterns.
 *
 * Note: these are all macros because they're included from files with different
 * IU definitions.
 */

#define DOT_WIDTH_MILS 0.0254

#define DOT_MARK_LEN( aLineWidth ) \
    ( std::max( 1.0, IU_PER_MILS * DOT_WIDTH_MILS - aLineWidth ) )

#define DASH_GAP_LEN( aLineWidth ) \
    ( 3.0 * DOT_MARK_LEN( aLineWidth ) + ( 2.0 * aLineWidth ) )

#define DASH_MARK_LEN( aLineWidth ) \
    ( std::max( DASH_GAP_LEN( aLineWidth ), 5.0 * DOT_MARK_LEN( aLineWidth ) ) )



#endif  // #ifndef GEOMETRY_UTILS_H

