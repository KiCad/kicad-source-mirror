/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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
#ifndef BASE_SET_H
#define BASE_SET_H

#include <vector>
#include <stdexcept>
#include <iterator>

class BASE_SET
{
public:
    using iterator = std::vector<int>::iterator;
    using const_iterator = std::vector<int>::const_iterator;

    BASE_SET( size_t size = 0 ) : m_bits( size, 0 ) {}

    bool test( size_t pos ) const
    {
        if( pos >= m_bits.size() )
            throw std::out_of_range( "Index out of range" );

        return m_bits[pos] == 1;
    }

    void set( size_t pos )
    {
        if( pos >= m_bits.size() )
            throw std::out_of_range( "Index out of range" );

        m_bits[pos] = 1;
    }

    void reset( size_t pos )
    {
        if( pos >= m_bits.size() )
            throw std::out_of_range( "Index out of range" );

        m_bits[pos] = 0;
    }

    size_t count() const { return std::count( m_bits.begin(), m_bits.end(), 1 ); }

    size_t size() const { return m_bits.size(); }

    void resize( size_t newSize ) { m_bits.resize( newSize, 0 ); }

    iterator       begin() { return m_bits.begin(); }
    iterator       end() { return m_bits.end(); }
    const_iterator begin() const { return m_bits.begin(); }
    const_iterator end() const { return m_bits.end(); }

    // Custom iterator to iterate over set bits
    class set_bits_iterator
    {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = size_t;
        using difference_type = std::ptrdiff_t;
        using pointer = const size_t*;
        using reference = const size_t&;

        set_bits_iterator( const BASE_SET& baseSet, size_t index ) :
                m_baseSet( baseSet ), m_index( index )
        {
            advance_to_next_set_bit();
        }

        size_t operator*() const { return m_index; }

        set_bits_iterator& operator++()
        {
            ++m_index;
            advance_to_next_set_bit();
            return *this;
        }

        bool operator!=( const set_bits_iterator& other ) const { return m_index != other.m_index; }

        bool operator==( const set_bits_iterator& other ) const { return m_index == other.m_index; }

    private:
        void advance_to_next_set_bit()
        {
            while( m_index < m_baseSet.size() && !m_baseSet.test( m_index ) )
                ++m_index;
        }

        const BASE_SET& m_baseSet;
        size_t          m_index;
    };

    // Custom reverse iterator to iterate over set bits in reverse order
    class set_bits_reverse_iterator
    {
    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = ssize_t;
        using difference_type = std::ptrdiff_t;
        using pointer = const ssize_t*;
        using reference = const ssize_t&;

        set_bits_reverse_iterator( const BASE_SET& baseSet, ssize_t index ) :
                m_baseSet( baseSet ), m_index( index )
        {
            advance_to_previous_set_bit();
        }

        ssize_t operator*() const { return m_index; }

        set_bits_reverse_iterator& operator++()
        {
            --m_index;
            advance_to_previous_set_bit();
            return *this;
        }

        bool operator!=( const set_bits_reverse_iterator& other ) const
        {
            return m_index != other.m_index;
        }

        bool operator==( const set_bits_reverse_iterator& other ) const
        {
            return m_index == other.m_index;
        }

    private:
        void advance_to_previous_set_bit()
        {
            while( m_index >= 0 && !m_baseSet.test( m_index ) )
            {
                --m_index;
            }
        }

        const BASE_SET& m_baseSet;
        ssize_t         m_index;
    };

    set_bits_iterator set_bits_begin() const { return set_bits_iterator( *this, 0 ); }
    set_bits_iterator set_bits_end() const { return set_bits_iterator( *this, m_bits.size() ); }

    set_bits_reverse_iterator set_bits_rbegin() const
    {
        return set_bits_reverse_iterator( *this, m_bits.size() - 1 );
    }
    set_bits_reverse_iterator set_bits_rend() const
    {
        return set_bits_reverse_iterator( *this, -1 );
    }

private:
    std::vector<int> m_bits;
};

#endif // BASE_SET_H
