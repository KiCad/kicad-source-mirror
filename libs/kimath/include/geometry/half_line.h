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

#include <optional>

#include <geometry/seg.h>
#include <math/box2.h>

/*
 * A geometric half-line of infinite length, starting at a given point and extending infinitely.
 * A.k.a. a ray.
 *
 * In terms of geometric ops, a SEG would probably do in most cases, as it
 * has the same definition, but a separate class is more explicit and also
 * allows compile-time reasoning about the meaning of the object through
 * the type system.
 */
class HALF_LINE
{
public:
    /**
     * Construct a ray from a segment - the ray will start at the segment's A point and
     * extend infinitely in the direction of the segment, passing through its B point.
     */
    HALF_LINE( const SEG& aSeg ) : m_seg( aSeg ) {}

    HALF_LINE( const VECTOR2I& aStart, const VECTOR2I& aOtherContainedPoint ) :
            m_seg( aStart, aOtherContainedPoint )
    {
    }

    /**
     * Get the start point of the ray.
     */
    const VECTOR2I& GetStart() const { return m_seg.A; }

    /**
     * Get one (of the infinite number) of points that the ray passes through.
     */
    const VECTOR2I& GetContainedPoint() const { return m_seg.B; }

    bool Contains( const VECTOR2I& aPoint ) const;

    OPT_VECTOR2I Intersect( const SEG& aSeg ) const;

    OPT_VECTOR2I Intersect( const HALF_LINE& aOther ) const;

    /**
     * Get the nearest point on the ray to the given point.
     *
     * This will be the start point of the ray for half the 2D plane.
     */
    VECTOR2I NearestPoint( const VECTOR2I& aPoint ) const;

    /**
     * Based on the ray being identically defined. TODO: this is not geoemetrical equality?!
     */
    bool operator==( const HALF_LINE& aOther ) const { return m_seg == aOther.m_seg; }

    /**
     * Gets the (one of the infinite number of) segments that the ray passes through.
     *
     * The segment's A point is the start of the ray, and the B point is on the ray.
     */
    const SEG& GetContainedSeg() const { return m_seg; }

private:
    /// Internally, we can represent a just a segment that the ray passes through
    SEG m_seg;
};


std::optional<SEG> ClipHalfLineToBox( const HALF_LINE& aRay, const BOX2I& aBox );