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

#pragma once

/**
 * @file geometry/shape_utils.h
 *
 * @brief Utility functions for working with shapes.
 *
 * These are free functions to avoid bloating the shape classes with functions
 * that only need to be used in a few places and can just use the public
 * interfaces.
 */

#include <array>
#include <optional>

#include <math/vector2d.h>
#include <math/box2.h>

#include <geometry/direction45.h>

class CIRCLE;
class HALF_LINE;
class LINE;
class SEG;
class SHAPE_RECT;
class SHAPE_POLY_SET;
struct TYPED_POINT2I;

namespace KIGEOM
{

/**
 * Returns a SEG such that the start point is smaller or equal
 * in x and y compared to the end point.
 */
SEG NormalisedSeg( const SEG& aSeg );

/**
 * Get the end point of the segment that is _not_ the given point.
 */
const VECTOR2I& GetOtherEnd( const SEG& aSeg, const VECTOR2I& aPoint );

/**
 * Get the shared endpoint of two segments, if it exists, or std::nullopt
 * if the segments are not connected end-to-end.
 */
OPT_VECTOR2I GetSharedEndpoint( const SEG& aSegA, const SEG& aSegB );

/**
 * Decompose a BOX2 into four segments.
 *
 * Segments are returned in the order: Top, Right, Bottom, Left.
 */
std::array<SEG, 4> BoxToSegs( const BOX2I& aBox );

/**
 * Add the 4 corners of a BOX2I to a vector.
 */
void CollectBoxCorners( const BOX2I& aBox, std::vector<VECTOR2I>& aCorners );

/**
 * Get the segments of a box that are in the given direction.
 *
 * N,E,S,W are the sides of the box.
 * NE,SE,SW,NW are the corners of the box, which return the two segments that meet at the corner,
 * in the order of the direction (e.g. NW = N, then W).
 */
std::vector<SEG> GetSegsInDirection( const BOX2I& aBox, DIRECTION_45::Directions aDir );

/**
 * Get the segment of a half-line that is inside a box, if any.
 */
std::optional<SEG> ClipHalfLineToBox( const HALF_LINE& aRay, const BOX2I& aBox );

/**
 * Get the segment of a line that is inside a box, if any.
 */
std::optional<SEG> ClipLineToBox( const LINE& aLine, const BOX2I& aBox );


/**
 * Get a SHAPE_ARC representing a 90-degree arc in the clockwise direction with the
 * midpoint in the given direction from the center.
 *
 *    _
 *     \
 *    + | <--- This is the NE arc from the + point.
 *
 * +-->x
 * |     (So Southerly point are bigger in y)
 * v y
 *
 * @param aCenter is the arc center.
 * @param aRadius is the arc radius.
 * @param aDir is the direction from the center to the midpoint (only NW, NE, SW, SE are valid).
 */
SHAPE_ARC MakeArcCw90( const VECTOR2I& aCenter, int aRadius, DIRECTION_45::Directions aDir );

/**
 * Get a SHAPE_ARC representing a 180-degree arc in the clockwise direction with the
 * midpoint in the given direction from the center.
 *
 * @param aDir is the direction from the center to the midpoint (only N, E, S, W are valid).
 */
SHAPE_ARC MakeArcCw180( const VECTOR2I& aCenter, int aRadius, DIRECTION_45::Directions aDir );

/**
 * Get the point on a rectangle that corresponds to a given direction.
 *
 * For directions N, E, S, W, the point is the center of the side.
 * For directions NW, NE, SW, SE, the point is the corner.
 *
 * @param aOutset is a distance to move the point outwards from the rectangle,
 *                in the direction of the corner (i.e. perpendicular to the side,
 *                or 45 degrees from the corner).
 */
VECTOR2I GetPoint( const SHAPE_RECT& aRect, DIRECTION_45::Directions aDir, int aOutset = 0 );


/**
 * Get key points of an CIRCLE.
 *
 * - The four cardinal points
 * - Optionally the center
 */
std::vector<TYPED_POINT2I> GetCircleKeyPoints( const CIRCLE& aCircle, bool aIncludeCenter );


/*
 * Take a polygon and 'rectify' it, so that all sides are H/V.
 *
 * The entire original polygon is contained within the new one.
 * The new polygon will pass though each original corner,
 * but it can have additional corners, or corners can be simplified away.
 *
 * E.g.:
 *     ____           _______
 *    /    \___    -> |     |___
 *    |________\      |_________|
 */
SHAPE_LINE_CHAIN RectifyPolygon( const SHAPE_LINE_CHAIN& aPoly );


/**
 * Adds a hole to a polygon if it is valid (i.e. it has 3 or more points
 * and a non-zero area.)
 *
 * This is important if you are reading in a polygon from a file and
 * aren't sure if it will end up being valid. Leaving invalid holes in
 * a SHAP_POLY_SET will violate the invariant that holes are non-null.
 *
 * @param aOutline is the outline to add the hole to.
 * @param aHole is the hole to add.
 * @return true if the hole was added, false if it was not.
 */
bool AddHoleIfValid( SHAPE_POLY_SET& aOutline, SHAPE_LINE_CHAIN&& aHole );


/**
 * Get the corners of a regular polygon from the centre, one point
 * and the number of sides.
 */
std::vector<VECTOR2I> MakeRegularPolygonPoints( const VECTOR2I& aCenter, size_t aN,
                                                const VECTOR2I& aPt0 );

/**
 * Make a regular polygon of the given size across the corners.
 *
 * @param aCenter the center of the polygon
 * @param aN polygon side count (e.g. 6 = hexagon)
 * @param aRadius the distance from the centre to a corner/flat
 * @param aAcrossCorners true if the radius is to the corners, false to the side midpoints
 * @param aAngle is the angle from the centre to one point. 0 is the point is to the right,
 *               and CCW from there.
 */
std::vector<VECTOR2I> MakeRegularPolygonPoints( const VECTOR2I& aCenter, size_t aN, int aRadius,
                                                bool aAcrossCorners, EDA_ANGLE aAngle );

/**
 * Create the two segments for a cross
 *
 * @param aCenter is the center of the cross
 * @param aSize is the size of the cross (can be x != y)
 * @param aAngle is the angle of the cross (postive = rotate cross CCW)
 */
std::vector<SEG> MakeCrossSegments( const VECTOR2I& aCenter, const VECTOR2I& aSize,
                                    EDA_ANGLE aAngle );

} // namespace KIGEOM