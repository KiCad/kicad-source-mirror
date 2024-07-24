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

#include <algorithm>
#include <iterator>
#include <limits>
#include <ostream>
#include <stdexcept>
#include <vector>

#include <core/kicad_algo.h>
#include <import_export.h>

#if defined( _MSC_VER )
// ssize_t is a POSIX extension
// wx usually defines it on windows as a helper
// windows does have SSIZE_T (capital) for the same purpose
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

class APIEXPORT BASE_SET
{
public:
    using iterator = std::vector<int>::iterator;
    using const_iterator = std::vector<int>::const_iterator;

    BASE_SET( size_t size ) : m_bits( size, 0 ) {}

    bool test( size_t pos ) const
    {
        if( pos >= m_bits.size() )
            return false;

        return m_bits[pos] == 1;
    }

    bool any() const { return std::any_of( m_bits.begin(), m_bits.end(), []( int bit ) { return bit == 1; } ); }

    bool all() const { return std::all_of( m_bits.begin(), m_bits.end(), []( int bit ) { return bit == 1; } ); }

    bool none() const { return std::none_of( m_bits.begin(), m_bits.end(), []( int bit ) { return bit == 1; } ); }

    BASE_SET& set( size_t pos = std::numeric_limits<size_t>::max(), bool value = true )
    {
        if( pos == std::numeric_limits<size_t>::max() )
        {
            std::fill( m_bits.begin(), m_bits.end(), value ? 1 : 0 );
            return *this;
        }

        if( pos >= m_bits.size() )
            m_bits.resize( pos + 1, 0 );

        m_bits[pos] = value ? 1 : 0;
        return *this;
    }

    BASE_SET& reset( size_t pos = std::numeric_limits<size_t>::max() )
    {
        if( pos == std::numeric_limits<size_t>::max() )
        {
            std::fill( m_bits.begin(), m_bits.end(), 0 );
            return *this;
        }

        if( pos >= m_bits.size() )
            m_bits.resize( pos + 1, 0 );

        m_bits[pos] = 0;
        return *this;
    }

    BASE_SET& flip( size_t pos = std::numeric_limits<size_t>::max() )
    {
        if( pos == std::numeric_limits<size_t>::max() )
        {
            std::transform( m_bits.begin(), m_bits.end(), m_bits.begin(), []( int bit ) { return bit ^ 1; } );
            return *this;
        }

        if( pos >= m_bits.size() )
            m_bits.resize( pos + 1, 0 );

        m_bits[pos] ^= 1;
        return *this;
    }

    size_t count() const { return std::count( m_bits.begin(), m_bits.end(), 1 ); }

    size_t size() const { return m_bits.size(); }

    void resize( size_t newSize ) { m_bits.resize( newSize, 0 ); }

    int& operator[]( size_t pos ) { return m_bits[pos]; }

    const int& operator[]( size_t pos ) const { return m_bits[pos]; }

    int compare( const BASE_SET& other ) const
    {
        return alg::lexicographical_compare_3way( m_bits, other.m_bits );
    }

    iterator       begin() { return m_bits.begin(); }
    iterator       end() { return m_bits.end(); }
    const_iterator begin() const { return m_bits.begin(); }
    const_iterator end() const { return m_bits.end(); }

    // Define equality operator
    bool operator==( const BASE_SET& other ) const
    {
        std::size_t minSize = std::min( size(), other.size() );
        if( !std::equal( m_bits.begin(), m_bits.begin() + minSize, other.m_bits.begin() ) )
            return false;

        if( std::any_of( m_bits.begin() + minSize, m_bits.end(), []( int bit ) { return bit != 0; } ) )
            return false;

        if( std::any_of( other.m_bits.begin() + minSize, other.m_bits.end(), []( int bit ) { return bit != 0; } ) )
            return false;

        return true;
    }

    // Define less-than operator for comparison
    bool operator<( const BASE_SET& other ) const
    {
        return std::lexicographical_compare( m_bits.begin(), m_bits.end(), other.m_bits.begin(), other.m_bits.end() );
    }

    // Define output operator
    friend std::ostream& operator<<( std::ostream& os, const BASE_SET& set )
    {
        return os << set.to_string();
    }

    // to_string method
    template <typename CharT = char>
    std::basic_string<CharT> to_string( CharT zero = CharT( '0' ), CharT one = CharT( '1' ) ) const
    {
        std::basic_string<CharT> result( size(), zero );

        for( size_t i = 0; i < size(); ++i )
        {
            if( test( i ) )
            {
                result[size() - 1 - i] = one; // Reverse order to match std::bitset behavior
            }
        }

        return result;
    }

    // Boolean operators
    BASE_SET& operator&=( const BASE_SET& rhs )
    {
        assert( m_bits.size() == rhs.m_bits.size() );

        for( size_t i = 0; i < m_bits.size(); ++i )
            m_bits[i] &= rhs.m_bits[i];

        return *this;
    }

    BASE_SET& operator|=( const BASE_SET& rhs )
    {
        assert( m_bits.size() == rhs.m_bits.size() );

        for( size_t i = 0; i < m_bits.size(); ++i )
            m_bits[i] |= rhs.m_bits[i];

        return *this;
    }

    BASE_SET& operator^=( const BASE_SET& rhs )
    {
        assert( m_bits.size() == rhs.m_bits.size() );

        for( size_t i = 0; i < m_bits.size(); ++i )
            m_bits[i] ^= rhs.m_bits[i];

        return *this;
    }

    BASE_SET operator~() const
    {
        BASE_SET result = *this;
        for( size_t i = 0; i < m_bits.size(); ++i )
            result.m_bits[i] = !m_bits[i];

        return result;
    }

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

inline BASE_SET operator&( const BASE_SET& lhs, const BASE_SET& rhs )
{
    BASE_SET result = lhs;
    result &= rhs;
    return result;
}

inline BASE_SET operator|( const BASE_SET& lhs, const BASE_SET& rhs )
{
    BASE_SET result = lhs;
    result |= rhs;
    return result;
}

inline BASE_SET operator^( const BASE_SET& lhs, const BASE_SET& rhs )
{
    BASE_SET result = lhs;
    result ^= rhs;
    return result;
}

namespace std
{
template <>
struct hash<BASE_SET>
{
    size_t operator()( const BASE_SET& bs ) const
    {
        size_t hashVal = 0;

        for( const auto& bit : bs )
            hashVal = hashVal * 31 + std::hash<int>()( bit );

        return hashVal;
    }
};
} // namespace std

#endif // BASE_SET_H
