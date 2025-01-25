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

#include <layer_ids.h>

#ifndef LAYER_RANGE_H
#define LAYER_RANGE_H


class LAYER_RANGE
{
private:
    PCB_LAYER_ID m_start;
    PCB_LAYER_ID m_stop;
    int  m_layer_count;

    class LAYER_RANGE_ITERATOR
    {
    private:
        int  m_current;
        int  m_stop;
        int  m_layer_count;
        bool m_reverse;

        int next_layer( int aLayer )
        {
            if( m_reverse )
            {
                if( aLayer == B_Cu )
                    aLayer = m_layer_count == 2 ? F_Cu :
                             static_cast<int>( F_Cu ) + 2 * ( m_layer_count - 2 ) + 2;
                else if( aLayer == m_stop || aLayer == UNDEFINED_LAYER )
                    aLayer = UNDEFINED_LAYER;
                else if( aLayer == In1_Cu )
                    aLayer = F_Cu;
                else
                    aLayer = static_cast<int>( aLayer ) - 2;
            }
            else
            {
                if( aLayer == F_Cu && m_layer_count == 2 )
                    aLayer = B_Cu;
                else if( aLayer == m_stop || aLayer == UNDEFINED_LAYER )
                    aLayer = UNDEFINED_LAYER;
                else if( aLayer == static_cast<int>( F_Cu ) + 2 * ( m_layer_count - 2 ) + 2)
                    aLayer = B_Cu;
                else if( aLayer == F_Cu )
                    aLayer = In1_Cu;
                else
                    aLayer = static_cast<int>( aLayer ) + 2;
            }

            return aLayer;
        }

    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = PCB_LAYER_ID;
        using difference_type = std::ptrdiff_t;
        using pointer = PCB_LAYER_ID*;
        using reference = PCB_LAYER_ID&;

        LAYER_RANGE_ITERATOR( PCB_LAYER_ID start, PCB_LAYER_ID stop, int layer_count ) :
                m_current( start ), m_stop( stop ), m_layer_count( layer_count )
        {
            if( start & 1 || stop & 1 )
                throw std::invalid_argument( "Only works for copper layers" );

            if( stop == B_Cu || m_stop >= m_current )
                m_reverse = false;
            else
                m_reverse = true;
        }

        PCB_LAYER_ID operator*() const { return static_cast<PCB_LAYER_ID>( m_current ); }

        LAYER_RANGE_ITERATOR& operator++()
        {
            m_current = next_layer( m_current );
            return *this;
        }

        LAYER_RANGE_ITERATOR operator++( int )
        {
            LAYER_RANGE_ITERATOR tmp = *this;
            ++( *this );
            return tmp;
        }

        bool operator==( const LAYER_RANGE_ITERATOR& other ) const
        {
            return m_current == other.m_current;
        }

        bool operator!=( const LAYER_RANGE_ITERATOR& other ) const { return !( *this == other ); }
    };

public:
    LAYER_RANGE( PCB_LAYER_ID start, PCB_LAYER_ID stop, int layer_count ) :
            m_start( start ), m_stop( stop ), m_layer_count( layer_count )
    {
            if( start & 1 || stop & 1 )
                throw std::invalid_argument( "Only works for copper layers" );
    }

    LAYER_RANGE_ITERATOR begin() const { return LAYER_RANGE_ITERATOR( m_start, m_stop,
                                                                      m_layer_count ); }

    LAYER_RANGE_ITERATOR end() const
    {
        auto it = LAYER_RANGE_ITERATOR( m_stop, m_stop, m_layer_count );
        return ++it;
    }

    static bool Contains( int aStart_layer, int aEnd_layer, int aTest_layer )
    {
        // B_Cu is the lowest copper layer for Z order copper layers
        // F_cu = top, B_Cu = bottom
        // So set the distance from top for B_Cu to INT_MAX
        if( aTest_layer == B_Cu )
            aTest_layer = INT_MAX;

        if( aStart_layer == B_Cu )
            aStart_layer = INT_MAX;

        if( aEnd_layer == B_Cu )
            aEnd_layer = INT_MAX;

        if( aStart_layer > aEnd_layer )
            std::swap( aStart_layer, aEnd_layer );

        return aTest_layer >= aStart_layer && aTest_layer <= aEnd_layer;
    }

    bool Contains( int aTest_layer )
    {
        return Contains( m_start, m_stop, aTest_layer );
    }

    size_t size() const
    {
        if( m_start == B_Cu )
            return m_layer_count;
        else
            return ( m_stop - m_start ) / 2 + 1;
    }
};

#endif // LAYER_RANGE_H
