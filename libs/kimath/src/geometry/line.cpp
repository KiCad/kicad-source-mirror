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

#include <geometry/line.h>

OPT_VECTOR2I LINE::Intersect( const SEG& aSeg ) const
{
    // intersect as two lines
    OPT_VECTOR2I intersection = aSeg.Intersect( m_seg, false, true );

    if( intersection )
    {
        // Not parallel.
        // That was two lines, but we need to check if the intersection is on
        // the requested segment
        if( aSeg.Contains( *intersection ) )
        {
            return intersection;
        }
    }
    return std::nullopt;
}

OPT_VECTOR2I LINE::Intersect( const LINE& aOther ) const
{
    // Defer to the SEG implementation
    return aOther.m_seg.Intersect( m_seg, false, true );
}

int LINE::Distance( const VECTOR2I& aPoint ) const
{
    // Just defer to the SEG implementation
    return m_seg.LineDistance( aPoint );
}

VECTOR2I LINE::NearestPoint( const VECTOR2I& aPoint ) const
{
    // Same as the SEG implementation, but without the early return
    // if the point isn't on the segment.

    // Inlined for performance reasons
    VECTOR2L d( m_seg.B.x - m_seg.A.x, m_seg.B.y - m_seg.A.y );
    ecoord   l_squared( d.x * d.x + d.y * d.y );

    if( l_squared == 0 )
        return m_seg.A;

    ecoord t = d.Dot( aPoint - m_seg.A );

    ecoord xp = rescale( t, (ecoord) d.x, l_squared );
    ecoord yp = rescale( t, (ecoord) d.y, l_squared );

    return VECTOR2<ecoord>( m_seg.A.x + xp, m_seg.A.y + yp );
}