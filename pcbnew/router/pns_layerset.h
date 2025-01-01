/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2014 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#ifndef __PNS_LAYERSET_H
#define __PNS_LAYERSET_H

#include <algorithm>

/**
 * Represent a contiguous set of PCB layers.
 */
class PNS_LAYER_RANGE
{
public:
    PNS_LAYER_RANGE() :
        m_start( -1 ),
        m_end( -1 )
    {};

    PNS_LAYER_RANGE( int aStart, int aEnd )
    {
        if( aStart > aEnd )
            std::swap( aStart, aEnd );

        m_start = aStart;
        m_end = aEnd;
    }

    PNS_LAYER_RANGE( int aLayer )
    {
        m_start = m_end = aLayer;
    }

    PNS_LAYER_RANGE( const PNS_LAYER_RANGE& aB ) :
        m_start( aB.m_start ),
        m_end( aB.m_end )
    {}

    ~PNS_LAYER_RANGE() {};

    PNS_LAYER_RANGE& operator=( const PNS_LAYER_RANGE& aB )
    {
        m_start = aB.m_start;
        m_end = aB.m_end;
        return *this;
    }

    bool Overlaps( const PNS_LAYER_RANGE& aOther ) const
    {
        if ( m_start < 0 || m_end < 0 || aOther.m_start < 0 || aOther.m_end < 0 )
            return false;
        return m_end >= aOther.m_start && m_start <= aOther.m_end;
    }

    bool Overlaps( const int aLayer ) const
    {
        if ( m_start < 0 || m_end < 0 || aLayer < 0 )
            return false;
        return aLayer >= m_start && aLayer <= m_end;
    }

    bool IsMultilayer() const
    {
        return m_start != m_end;
    }

    int Start() const
    {
        return m_start;
    }

    int End() const
    {
        return m_end;
    }

    void Merge( const PNS_LAYER_RANGE& aOther )
    {
        if( m_start < 0 || m_end < 0 )
        {
            m_start = aOther.m_start;
            m_end = aOther.m_end;
            return;
        }

        if( aOther.m_start < m_start )
            m_start = aOther.m_start;

        if( aOther.m_end > m_end )
            m_end = aOther.m_end;
    }

    PNS_LAYER_RANGE Intersection( const PNS_LAYER_RANGE& aOther ) const
    {
        PNS_LAYER_RANGE overlap;

        overlap.m_start = std::max( m_start, aOther.m_start );

        if( m_end < 0 )
            overlap.m_end = aOther.m_end;
        else if( aOther.m_end < 0 )
            overlap.m_end = m_end;
        else
            overlap.m_end = std::min( m_end, aOther.m_end );

        return overlap;
    }

    ///< Shortcut for comparisons/overlap tests
    static PNS_LAYER_RANGE All()
    {
        return PNS_LAYER_RANGE( 0, 256 ); // fixme: use layer IDs header
    }

    bool operator==( const PNS_LAYER_RANGE& aOther ) const
    {
        return ( m_start == aOther.m_start ) && ( m_end == aOther.m_end );
    }

    bool operator!=( const PNS_LAYER_RANGE& aOther ) const
    {
        return ( m_start != aOther.m_start ) || ( m_end != aOther.m_end );
    }

private:
    int m_start;
    int m_end;
};

#endif    // __PNS_LAYERSET_H
