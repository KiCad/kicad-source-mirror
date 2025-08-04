/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef INCLUDE_CORE_KICAD_ALGO_H_
#define INCLUDE_CORE_KICAD_ALGO_H_

#include <algorithm>
#include <functional> // std::function
#include <utility>    // std::pair
#include <vector>
#include <wx/debug.h> // wxCHECK_MSG

namespace alg
{
/**
 *  @brief Apply a function to the first and second element of a std::pair
 *  @param  __pair   A pair of elements (both elements must be the same type).
 *  @param  __f      A unary function object.
 *
 *  Applies the function object @p __f to @p __pair.first and @p __pair.second
 *  If @p __f has a return value it is ignored.
*/
template <typename _Type, typename _Function>
void run_on_pair( std::pair<_Type, _Type>& __pair, _Function __f )
{
    __f( __pair.first );
    __f( __pair.second );
}

/**
 *  @brief Apply a function to every sequential pair of elements of a sequence.
 *  @param  __first  An input iterator.
 *  @param  __last   An input iterator.
 *  @param  __f      A unary function object.
 *
 *  Applies the function object @p __f to each sequential pair of elements in the range
 *  @p [first,last).  @p __f must not modify the order of the sequence.
 *  If @p __f has a return value it is ignored.
*/
template <typename _InputIterator, typename _Function>
void adjacent_pairs( _InputIterator __first, _InputIterator __last, _Function __f )
{
    if( __first != __last )
    {
        _InputIterator __follow = __first;
        ++__first;
        for( ; __first != __last; ++__first, ++__follow )
            __f( *__follow, *__first );
    }
}

/**
 *  @brief Apply a function to every possible pair of elements of a sequence.
 *  @param  __first  An input iterator.
 *  @param  __last   An input iterator.
 *  @param  __f      A unary function object.
 *
 *  Applies the function object @p __f to every possible pair of elements in the range
 *  @p [first,last).  @p __f must not modify the order of the sequence.
 *  If @p __f has a return value it is ignored.
*/
template <typename _InputIterator, typename _Function>
void for_all_pairs( _InputIterator __first, _InputIterator __last, _Function __f )
{
    if( __first != __last )
    {
        _InputIterator __follow = __first;
        ++__first;
        for( ; __first != __last; ++__first, ++__follow )
            for( _InputIterator __it = __first; __it != __last; ++__it )
                __f( *__follow, *__it );
    }
}

/**
 * @brief Returns true if the container contains the given value.
 */
template <class _Container, typename _Value>
bool contains( const _Container& __container, _Value __value )
{
    return std::find( __container.begin(), __container.end(), __value ) != __container.end();
}

/**
 * @brief Returns true if either of the elements in an std::pair contains the given value
 *
 * @param  __pair   A pair of elements (both elements must be the same type).
 * @param  __value  A value to test
 * @return true if @p __value is contained in @p __pair
 */
template <typename _Type, typename _Value>
bool pair_contains( const std::pair<_Type, _Type> __pair, _Value __value )
{
    return __pair.first == static_cast<_Type>( __value )
           || __pair.second == static_cast<_Type>( __value );
}

/**
 * @brief Test if __val lies within __minval and __maxval in a wrapped range.
 *
 * @param  __val     A value to test
 * @param  __minval  Lowest permissible value within the wrapped range
 * @param  __maxval  Highest permissible value within the wrapped range
 * @param  __wrap    Value at which the range wraps around itself (must be positive)
 * @return true if @p __val lies in the wrapped range
 */
template <class T>
bool within_wrapped_range( T __val, T __minval, T __maxval, T __wrap )
{
    wxCHECK_MSG( __wrap > 0, false, wxT( "Wrap must be positive!" ) );

    while( __maxval >= __wrap )
        __maxval -= __wrap;

    while( __maxval < 0 )
        __maxval += __wrap;

    while( __minval >= __wrap )
        __minval -= __wrap;

    while( __minval < 0 )
        __minval += __wrap;

    while( __val < 0 )
        __val += __wrap;

    while( __val >= __wrap )
        __val -= __wrap;

    if( __maxval > __minval )
        return __val >= __minval && __val <= __maxval;
    else
        return __val >= __minval || __val <= __maxval;
}

/**
 * @brief Deletes all duplicate values from \a __c.
 */
template <class _Container>
void remove_duplicates( _Container& __c )
{
    __c.erase( std::unique( __c.begin(), __c.end() ), __c.end() );
}

template <class _Container, class _Function>
void remove_duplicates( _Container& __c, _Function&& __f )
{
    __c.erase( std::unique( __c.begin(), __c.end(), std::forward<_Function>( __f ) ), __c.end() );
}

/**
 * @brief Integral version of std::signbit that works all compilers.
 */
template <typename T, std::enable_if_t<std::is_integral<T>::value, int> = 0>
bool signbit( T v )
{
    return v < 0;
}


/**
 * @brief Returns the length of the longest common subset of values between two containers.
*/
template <class _Container>
size_t longest_common_subset( const _Container& __c1, const _Container& __c2 )
{
    size_t __c1_size = __c1.size();
    size_t __c2_size = __c2.size();

    if( __c1_size == 0 || __c2_size == 0 )
        return 0;

    // Create a 2D table to store the lengths of common subsets
    std::vector<std::vector<size_t>> table( __c1_size + 1, std::vector<size_t>( __c2_size + 1, 0 ) );

    size_t longest = 0;

    for( size_t i = 1; i <= __c1_size; ++i )
    {
        for( size_t j = 1; j <= __c2_size; ++j )
        {
            if( __c1[i - 1] == __c2[j - 1] )
            {
                table[i][j] = table[i - 1][j - 1] + 1;
                longest = std::max( longest, static_cast<size_t>( table[i][j] ) );
            }
        }
    }

    return longest;
}

/**
 * @brief Compares two containers lexicographically.
 *
 * Returns a negative value if the first container is less than the second,
 * zero if they are equal, and a positive value if the first container is
 * greater than the second.  This is a re-implementation of
 * std::lexicographical_compare_three_way because it is not available in all
 * compilers.
 */
template <class Container1Iter, class Container2Iter>
int lexicographical_compare_three_way( Container1Iter aC1_first, Container1Iter aC1_last,
                                       Container2Iter aC2_first, Container2Iter aC2_last )
{
#ifdef __cpp_lib_three_way_comparison // Check to see if we have an optimized version
    auto retval =
            std::lexicographical_compare_three_way( aC1_first, aC1_last, aC2_first, aC2_last );
    return retval == std::strong_ordering::equal
                   ? 0
                   : ( retval == std::strong_ordering::less ? -1 : 1 );
#else
    Container1Iter it1 = aC1_first;
    Container2Iter it2 = aC2_first;

    while( it1 != aC1_last && it2 != aC2_last )
    {
        if( *it1 < *it2 )
            return -1;
        if( *it1 > *it2 )
            return 1;
        ++it1;
        ++it2;
    }

    if( it2 == aC2_last )
        return !( it1 == aC1_last );
    else
        return -1;
#endif // __cpp_lib_three_way_comparison
}


} // namespace alg

#endif /* INCLUDE_CORE_KICAD_ALGO_H_ */
