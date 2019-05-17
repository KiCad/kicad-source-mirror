/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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
 * with this program.  If not, see <http://www.gnu.or/licenses/>.
 */

#include <geometry/direction45.h>

const SHAPE_LINE_CHAIN DIRECTION_45::BuildInitialTrace( const VECTOR2I& aP0, const VECTOR2I& aP1,
        bool aStartDiagonal, int aMaxRadius ) const
{
    bool start_diagonal;

    if( m_dir == UNDEFINED )
        start_diagonal = aStartDiagonal;
    else
        start_diagonal = IsDiagonal();

    int w = abs( aP1.x - aP0.x );
    int h = abs( aP1.y - aP0.y );
    int sw = sign( aP1.x - aP0.x );
    int sh = sign( aP1.y - aP0.y );

    int  radius = std::min( aMaxRadius, std::min( w, h ) );
    bool use_rounded = aMaxRadius > 0;
    int  dist90 = use_rounded ? KiROUND( ( M_SQRT2 - 1.0 ) * radius ) : 0;
    int  dist45 = use_rounded ? KiROUND( radius * ( 1.0 - M_SQRT1_2 ) ) : 0;

    VECTOR2I mp0, mp1, arc_offset_90, arc_offset_45;

    // we are more horizontal than vertical?
//    if( m_90deg )
//    {
//        if( m_dir == N || m_dir == S )
//
//    }

    if( w > h )
    {
        mp0 = VECTOR2I( ( w - h - dist90 ) * sw, 0 );               // direction: E
        mp1 = VECTOR2I( ( h - dist45 ) * sw, ( h - dist45 ) * sh ); // direction: NE
        arc_offset_90 = VECTOR2I( 0, radius * sh );
        arc_offset_45 = VECTOR2I( sw * radius * M_SQRT1_2, -sh * radius * M_SQRT1_2 );
    }
    else
    {
        mp0 = VECTOR2I( 0, sh * ( h - w - dist90 ) );               // direction: N
        mp1 = VECTOR2I( sw * ( w - dist45 ), sh * ( w - dist45 ) ); // direction: NE
        arc_offset_90 = VECTOR2I( radius * sw, 0 );
        arc_offset_45 = VECTOR2I( -sw * radius * M_SQRT1_2, sh * radius * M_SQRT1_2 );
    }

    SHAPE_LINE_CHAIN pl;
    VECTOR2I arc_center;

    pl.Append( aP0 );
    VECTOR2I next_point;

    if( start_diagonal )
    {
        next_point = aP0 + mp1;
        arc_center = aP0 + mp1 + arc_offset_45;
    }
    else
    {
        next_point = aP0 + mp0;
        arc_center = aP0 + mp0 + arc_offset_90;
    }

    if( use_rounded )
    {
        int sa = start_diagonal ? -sw * sh : sw * sh;

        if( h > w )
            sa = -sa;

        SHAPE_ARC new_arc( arc_center, next_point, sa * 45.0 );
        pl.Append( new_arc );
    }
    else
    {
        pl.Append( next_point );
    }

    pl.Append( aP1 );

    pl.Simplify();
    return pl;
}
