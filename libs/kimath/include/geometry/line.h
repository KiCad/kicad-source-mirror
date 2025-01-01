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

#include <geometry/seg.h>

/*
 * A geometric line of infinite length.
 *
 * In terms of geometric ops, a SEG would probably do as it has the same definition,
 * but a separate class is more explicit and also allows compile-time
 * reasoning about the meaning of the object through the type system.
 */
class LINE
{
public:
    using ecoord = VECTOR2I::extended_type;

    LINE( const SEG& aSeg ) : m_seg( aSeg ) {}

    LINE( const VECTOR2I& aStart, const VECTOR2I& aEnd ) : m_seg( aStart, aEnd ) {}

    bool operator==( const LINE& aOther ) const { return m_seg == aOther.m_seg; }

    /**
     * Gets the (one of the infinite number of) segments that the line passes through.
     */
    const SEG& GetContainedSeg() const { return m_seg; }

    OPT_VECTOR2I Intersect( const SEG& aOther ) const;
    OPT_VECTOR2I Intersect( const LINE& aOther ) const;

    /**
     * Gets the distance from the line to the given point.
     */
    int Distance( const VECTOR2I& aPoint ) const;

    /**
     * Gets the nearest point on the line to the given point.
     */
    VECTOR2I NearestPoint( const VECTOR2I& aPoint ) const;

private:
    /// Internally, we can represent a just a segment that the line passes through
    SEG m_seg;
};