/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ENUM_VECTOR_H
#define ENUM_VECTOR_H

#include <iterator>
#include <type_traits>

/**
 * Macro to create const vectors containing enum values to enable easy iteration.
 *
 * @warning Do not use in new code. Use the #DEFINE_ENUM_CLASS_WITH_ITERATOR and
 *          #DECLARE_ENUM_CLASS_ITERATOR macros instead (unless they don't work ;) ).
 *
 * Usage:
 * [header]
 * A {
 *     DEFINE_ENUM_VECTOR( COLOR, { RED, GREEN, BLUE } )
 * };
 *
 * [source]
 * for( COLOR color : COLOR_vector ) {
 *     // do sth with color
 * }
 *
 * DECLARE_ENUM_VECTOR( COLORS );
 */
#define DEFINE_ENUM_VECTOR( enumName, ... ) \
    enum enumName __VA_ARGS__; \
    static constexpr enumName enumName##_vector[] = __VA_ARGS__;

#define DECLARE_ENUM_VECTOR( className, enumName ) \
    constexpr className::enumName className::enumName##_vector[];


/**
 * Macro to create const vectors containing enum class values to enable easy iteration.
 *
 * Usage:
 * [header]
 * A {
 *     DEFINE_ENUM_CLASS_WITH_ITERATOR( COLOR, RED, GREEN, BLUE )
 * };
 *
 * [source]
 * for( COLOR color : COLOR_ITERATOR() ) {
 *     // do sth with color
 * }
 */
#define DEFINE_ENUM_CLASS_WITH_ITERATOR( enumName, beginVal, ... ) \
    enum class enumName : int { beginVal, __VA_ARGS__, _ENUM_END }; \
    typedef ENUM_ITERATOR<enumName, enumName::beginVal, enumName::_ENUM_END> enumName##_ITERATOR;

template <typename T, T beginVal, T endVal>
class ENUM_ITERATOR
{
    typedef typename std::underlying_type<T>::type val_t;
    int val;

public:
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = T*;
    using reference = T&;
    using iterator_category = std::input_iterator_tag;

    ENUM_ITERATOR( const T& f ) : val( static_cast<val_t>( f ) ) {}
    ENUM_ITERATOR() : val( static_cast<val_t>( beginVal ) ) {}
    ENUM_ITERATOR operator++()
    {
        val++;
        return *this;
    }

    T operator*() { return static_cast<T>( val ); }
    ENUM_ITERATOR begin() { return *this; }
    ENUM_ITERATOR end() { return ENUM_ITERATOR( endVal ); }
    bool operator==( const ENUM_ITERATOR& aIt ) const { return val == aIt.val; }
    bool operator!=( const ENUM_ITERATOR& aIt ) const { return val != aIt.val; }
};

#endif /* ENUM_VECTOR_H */
