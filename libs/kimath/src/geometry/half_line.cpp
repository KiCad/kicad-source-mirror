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

#include "geometry/half_line.h"

#include <math/box2.h>

#include <geometry/shape_utils.h>


/**
 * Check if two vectors point into the same quadrant.
 */
bool VectorsInSameQuadrant( const VECTOR2I& aA, const VECTOR2I& aB )
{
    // The sign of the x and y components of the vectors must be the same
    return ( ( aA.x >= 0 ) == ( aB.x >= 0 ) ) && ( ( aA.y >= 0 ) == ( aB.y >= 0 ) );
}


bool HALF_LINE::Contains( const VECTOR2I& aPoint ) const
{
    // Check that the point is on the right side of the ray from
    // the start point
    // This is quick, so we can do it first
    if( !VectorsInSameQuadrant( m_seg.B - m_seg.A, aPoint - m_seg.A ) )
    {
        return false;
    }

    // Check that the point is within a distance of 1 from the
    // infinite line of the ray
    return m_seg.LineDistance( aPoint ) <= 1;
}


OPT_VECTOR2I HALF_LINE::Intersect( const SEG& aSeg ) const
{
    // Intsersection of two infinite lines
    const SEG    seg = GetContainedSeg();
    OPT_VECTOR2I intersection = aSeg.Intersect( seg, false, true );

    // Reject parallel lines
    if( !intersection )
    {
        return std::nullopt;
    }

    // Check that the intersection is on the right side of the
    // ray's start point (i.e. equal quadrants)
    if( !VectorsInSameQuadrant( m_seg.B - m_seg.A, *intersection - m_seg.A ) )
    {
        return std::nullopt;
    }

    // Check that the intersection is not somewhere past the end
    // of the segment
    if( !aSeg.Contains( *intersection ) )
    {
        return std::nullopt;
    }

    return intersection;
}

OPT_VECTOR2I HALF_LINE::Intersect( const HALF_LINE& aOther ) const
{
    // Intsersection of two infinite lines
    const SEG    otherSeg = aOther.GetContainedSeg();
    OPT_VECTOR2I intersection = m_seg.Intersect( otherSeg, false, true );

    // Reject parallel lines
    if( !intersection )
    {
        return std::nullopt;
    }

    // Check that the intersection is on the right side of both
    // rays' start points (i.e. equal quadrants)
    if( !VectorsInSameQuadrant( m_seg.B - m_seg.A, *intersection - m_seg.A )
        || !VectorsInSameQuadrant( aOther.m_seg.B - aOther.m_seg.A,
                                   *intersection - aOther.m_seg.A ) )
    {
        return std::nullopt;
    }

    return intersection;
}

VECTOR2I HALF_LINE::NearestPoint( const VECTOR2I& aPoint ) const
{
    // Same as the SEG implementation, but without the early return
    // if the point isn't on the segment.

    // Inlined for performance reasons
    VECTOR2L    d( m_seg.B.x - m_seg.A.x, m_seg.B.y - m_seg.A.y );
    SEG::ecoord l_squared( d.x * d.x + d.y * d.y );

    if( l_squared == 0 )
        return m_seg.A;

    SEG::ecoord t = d.Dot( aPoint - m_seg.A );

    if( t < 0 )
        return m_seg.A;

    SEG::ecoord xp = rescale( t, (SEG::ecoord) d.x, l_squared );
    SEG::ecoord yp = rescale( t, (SEG::ecoord) d.y, l_squared );

    return VECTOR2<SEG::ecoord>( m_seg.A.x + xp, m_seg.A.y + yp );
}
