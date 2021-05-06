/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright (C) 2019-2021 KiCad Developers, see CHANGELOG.TXT for contributors.
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


} // namespace alg

#endif /* INCLUDE_CORE_KICAD_ALGO_H_ */
