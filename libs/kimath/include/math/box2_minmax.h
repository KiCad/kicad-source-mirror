/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 Alex Shvartzkop <dudesuchamazing@gmail.com>
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

#ifndef __BOX2_MINMAX_H
#define __BOX2_MINMAX_H

#include <math/box2.h>
#include <geometry/seg.h>
#include <geometry/shape_arc.h>


/**
 * A min-max version of BOX2 for fast intersection checking.
 */
struct BOX2I_MINMAX
{
    BOX2I_MINMAX() : m_Left( 0 ), m_Top( 0 ), m_Right( 0 ), m_Bottom( 0 ) {}

    BOX2I_MINMAX( int aLeft, int aTop, int aRight, int aBottom ) :
            m_Left( aLeft ), m_Top( aTop ), m_Right( aRight ), m_Bottom( aBottom )
    {
    }

    BOX2I_MINMAX( const VECTOR2I& aPt ) : BOX2I_MINMAX( aPt.x, aPt.y ) {}

    BOX2I_MINMAX( int aX, int aY ) : m_Left( aX ), m_Top( aY ), m_Right( aX ), m_Bottom( aY ) {}

    BOX2I_MINMAX( const BOX2I& aBox ) :
            m_Left( aBox.GetLeft() ), m_Top( aBox.GetTop() ), m_Right( aBox.GetRight() ),
            m_Bottom( aBox.GetBottom() )
    {
    }

    BOX2I_MINMAX( const VECTOR2I& aA, const VECTOR2I& aB )
    {
        m_Left = std::min( aA.x, aB.x );
        m_Right = std::max( aA.x, aB.x );
        m_Top = std::min( aA.y, aB.y );
        m_Bottom = std::max( aA.y, aB.y );
    }

    operator BOX2I()
    {
        return BOX2ISafe( VECTOR2I( m_Left, m_Top ),
                          VECTOR2L( int64_t( m_Right ) - m_Left, int64_t( m_Bottom ) - m_Top ) );
    }

    BOX2I_MINMAX( const SHAPE_ARC& aArc ) : BOX2I_MINMAX( aArc.BBox() ) {}

    BOX2I_MINMAX( const SEG& aSeg ) : BOX2I_MINMAX( aSeg.A, aSeg.B ) {}

    inline bool Intersects( const BOX2I_MINMAX& aOther ) const
    {
        // calculate the left common area coordinate:
        int left = std::max( m_Left, aOther.m_Left );
        // calculate the right common area coordinate:
        int right = std::min( m_Right, aOther.m_Right );
        // calculate the upper common area coordinate:
        int top = std::max( m_Top, aOther.m_Top );
        // calculate the lower common area coordinate:
        int bottom = std::min( m_Bottom, aOther.m_Bottom );

        // if a common area exists, it must have a positive (null accepted) size
        return left <= right && top <= bottom;
    }

    void Merge( const VECTOR2I& aPt )
    {
        m_Left = std::min( m_Left, aPt.x );
        m_Right = std::max( m_Right, aPt.x );
        m_Top = std::min( m_Top, aPt.y );
        m_Bottom = std::max( m_Bottom, aPt.y );
    }

    VECTOR2I GetCenter() const
    {
        int cx = ( (int64_t) m_Left + m_Right ) / 2;
        int cy = ( (int64_t) m_Top + m_Bottom ) / 2;

        return VECTOR2I( cx, cy );
    }

    double GetDiameter() const
    {
        VECTOR2L start( m_Left, m_Top );
        VECTOR2L end( m_Right, m_Bottom );
        VECTOR2L d = end - start;

        return std::hypot( d.x, d.y );
    }

    int m_Left;
    int m_Top;
    int m_Right;
    int m_Bottom;
};


#endif
