/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2014 CERN
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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
 * Class PNS_LAYERSET
 *
 * Represents a contiguous set of PCB layers.
 */
class PNS_LAYERSET
{
public:
    PNS_LAYERSET() :
        m_start( -1 ),
        m_end( -1 )
    {};

    PNS_LAYERSET( int aStart, int aEnd )
    {
        if( aStart > aEnd )
            std::swap( aStart, aEnd );

        m_start = aStart;
        m_end = aEnd;
    }

    PNS_LAYERSET( int aLayer )
    {
        m_start = m_end = aLayer;
    }

    PNS_LAYERSET( const PNS_LAYERSET& b ) :
        m_start( b.m_start ),
        m_end( b.m_end )
    {}

    ~PNS_LAYERSET() {};

    const PNS_LAYERSET& operator=( const PNS_LAYERSET& b )
    {
        m_start = b.m_start;
        m_end = b.m_end;
        return *this;
    }

    bool Overlaps( const PNS_LAYERSET& aOther ) const
    {
        return m_end >= aOther.m_start && m_start <= aOther.m_end;
    }

    bool Overlaps( const int aLayer ) const
    {
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

    void Merge( const PNS_LAYERSET& aOther )
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

    ///> Shortcut for comparisons/overlap tests
    static PNS_LAYERSET All()
    {
        return PNS_LAYERSET( 0, 256 );
    }

private:
    int m_start;
    int m_end;
};

#endif    // __PNS_LAYERSET_H
