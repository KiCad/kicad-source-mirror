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
#include <dynamic_bitset.h>

#include <core/kicad_algo.h>
#include <import_export.h>

#if defined( _MSC_VER )
// ssize_t is a POSIX extension
// wx usually defines it on windows as a helper
// windows does have SSIZE_T (capital) for the same purpose
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

class APIEXPORT BASE_SET : public sul::dynamic_bitset<uint64_t>
{
public:
    class iterator
    {
    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type = bool;
        using difference_type = std::ptrdiff_t;
        using pointer = void;
        using reference = bool;

        iterator( BASE_SET* set, size_t pos ) : m_set( set ), m_pos( pos ) {}
        bool operator*() const { return m_set->test( m_pos ); }
        iterator& operator++()
        {
            ++m_pos;
            return *this;
        }
        iterator operator+( difference_type n ) const
        {
            return iterator( m_set, m_pos + n );
        }
        difference_type operator-( const iterator& other ) const
        {
            return static_cast<difference_type>(m_pos) - static_cast<difference_type>(other.m_pos);
        }
        auto operator<=>( const iterator& ) const = default;

    private:
        BASE_SET* m_set;
        size_t    m_pos;
    };

    class const_iterator
    {
    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type = bool;
        using difference_type = std::ptrdiff_t;
        using pointer = void;
        using reference = bool;

        const_iterator( const BASE_SET* set, size_t pos ) : m_set( set ), m_pos( pos ) {}
        bool operator*() const { return m_set->test( m_pos ); }
        const_iterator& operator++()
        {
            ++m_pos;
            return *this;
        }
        const_iterator operator+( difference_type n ) const
        {
            return const_iterator( m_set, m_pos + n );
        }
        difference_type operator-( const const_iterator& other ) const
        {
            return static_cast<difference_type>(m_pos) - static_cast<difference_type>(other.m_pos);
        }
        auto operator<=>( const const_iterator& ) const = default;

    private:
        const BASE_SET* m_set;
        size_t          m_pos;
    };

    iterator begin() { return iterator(this, 0); }
    iterator end() { return iterator(this, size()); }
    const_iterator begin() const { return const_iterator(this, 0); }
    const_iterator end() const { return const_iterator(this, size()); }

    BASE_SET( size_t size = 64 ) : sul::dynamic_bitset<uint64_t>( size ) {}

    // Overloads for set, reset, and flip operations

    // Set a bit at the specified position
    BASE_SET& set(size_t pos)
    {
        if( pos >= size() )
            sul::dynamic_bitset<uint64_t>::resize( pos + 1 );

        sul::dynamic_bitset<uint64_t>::set(pos);
        return *this;
    }

    // Set a bit at the specified position to a given value
    BASE_SET& set(size_t pos, bool value)
    {
        if( pos >= size() )
            sul::dynamic_bitset<uint64_t>::resize( pos + 1 );

        sul::dynamic_bitset<uint64_t>::set(pos, value);
        return *this;
    }

    // Set all bits to 1
    BASE_SET& set()
    {
        sul::dynamic_bitset<uint64_t>::set();
        return *this;
    }

    // Reset (clear) a bit at the specified position
    BASE_SET& reset(size_t pos)
    {
        if( pos >= size() )
            sul::dynamic_bitset<uint64_t>::resize( pos + 1 );

        sul::dynamic_bitset<uint64_t>::reset(pos);
        return *this;
    }

    // Reset (clear) all bits
    BASE_SET& reset()
    {
        sul::dynamic_bitset<uint64_t>::reset();
        return *this;
    }

    // Flip a bit at the specified position
    BASE_SET& flip(size_t pos)
    {
        if( pos >= size() )
            sul::dynamic_bitset<uint64_t>::resize( pos + 1 );

        sul::dynamic_bitset<uint64_t>::flip(pos);
        return *this;
    }

    // Flip all bits
    BASE_SET& flip()
    {
        sul::dynamic_bitset<uint64_t>::flip();
        return *this;
    }

    // Overloads for boolean operators

    // Bitwise NOT operator
    BASE_SET operator~() const
    {
        BASE_SET result(*this);
        result.flip();
        return result;
    }

    // Compound assignment AND operator
    BASE_SET& operator&=(const BASE_SET& other)
    {
        sul::dynamic_bitset<uint64_t>::operator&=(other);
        return *this;
    }

    // Compound assignment OR operator
    BASE_SET& operator|=(const BASE_SET& other)
    {
        sul::dynamic_bitset<uint64_t>::operator|=(other);
        return *this;
    }

    // Compound assignment XOR operator
    BASE_SET& operator^=(const BASE_SET& other)
    {
        sul::dynamic_bitset<uint64_t>::operator^=(other);
        return *this;
    }

    int compare( const BASE_SET& other ) const
    {
        return alg::lexicographical_compare_three_way( begin(), end(), other.begin(), other.end() );
    }

    // Define less-than operator for comparison
    bool operator<( const BASE_SET& other ) const
    {
        return alg::lexicographical_compare( begin(), end(), other.begin(), other.end() ) < 0;
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

    protected:
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

    protected:
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
    set_bits_iterator set_bits_end() const { return set_bits_iterator( *this, size() ); }

    set_bits_reverse_iterator set_bits_rbegin() const
    {
        return set_bits_reverse_iterator( *this, size() - 1 );
    }
    set_bits_reverse_iterator set_bits_rend() const
    {
        return set_bits_reverse_iterator( *this, -1 );
    }

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
