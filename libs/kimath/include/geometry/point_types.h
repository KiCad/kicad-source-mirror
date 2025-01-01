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

#ifndef GEOMETRY_POINT_TYPE_H_
#define GEOMETRY_POINT_TYPE_H_

#include <math/vector2d.h>

/**
 * Meanings that can be assigned to a point in pure geometric
 * terms.
 *
 * For example, a circle has a center point and four quadrant points.
 *
 * These can be combined using bitwise OR if a point has multiple meanings.
 */
enum POINT_TYPE
{
    /**
     * No specific point type.
     */
    PT_NONE = 0,
    /**
     * The point is the center of something.
     */
    PT_CENTER = 1 << 0,
    /**
     * The point is at the end of a segment, arc, etc.
     */
    PT_END = 1 << 1,
    /**
     * The point is at the middle of a segment, arc, etc.
     */
    PT_MID = 1 << 2,
    /**
     * The point is on a quadrant of a circle (N, E, S, W points).
     */
    PT_QUADRANT = 1 << 3,
    /**
     * The point is a corner of a polygon, rectangle, etc
     * (you may want to infer PT_END from this)
     */
    PT_CORNER = 1 << 4,
    /**
     * The point is an intersection of two (or more) items.
     */
    PT_INTERSECTION = 1 << 5,
    /**
     * The point is somewhere on another element, but not some specific point.
     * (you can infer this from some other point types)
     */
    PT_ON_ELEMENT = 1 << 6,
};

struct TYPED_POINT2I
{
    VECTOR2I m_point;
    // Bitwise OR of POINT_TYPE values
    int m_types;

    // Clang needs this apparently
    TYPED_POINT2I( const VECTOR2I& aVec, int aTypes ) : m_point( aVec ), m_types( aTypes ) {}

    bool operator==( const TYPED_POINT2I& ) const = default;
};

#endif // GEOMETRY_POINT_TYPE_H_