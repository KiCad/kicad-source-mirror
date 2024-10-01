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
 */
VECTOR2I GetPoint( const SHAPE_RECT& aRect, DIRECTION_45::Directions aDir );


/**
 * Get key points of an CIRCLE.
 *
 * - The four cardinal points
 * - Optionally the center
 */
std::vector<TYPED_POINT2I> GetCircleKeyPoints( const CIRCLE& aCircle, bool aIncludeCenter );

} // namespace KIGEOM