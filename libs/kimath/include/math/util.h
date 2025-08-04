/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (c) 2005 Michael Niedermayer <michaelni@gmx.at>
 * Copyright (C) CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 *
 * The equals() method to compare two floating point values adapted from
 * AlmostEqualRelativeAndAbs() on
 * https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/
 * (C) Bruce Dawson subject to the Apache 2.0 license.
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

#ifndef UTIL_H
#define UTIL_H

#include <config.h>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>
#include <typeinfo>
#include <type_traits>
#include <algorithm>

/**
 * Helper to avoid directly including wx/log.h for the templated functions in kimath
 */
void kimathLogDebug( const char* aFormatString, ... );

/**
 * Workaround to avoid the empty-string conversion issue in wxWidgets
 */
void kimathLogOverflow( double v, const char* aTypeName );


// Suppress an annoying warning that the explicit rounding we do is not precise
#ifdef HAVE_WIMPLICIT_FLOAT_CONVERSION
    _Pragma( "GCC diagnostic push" ) \
    _Pragma( "GCC diagnostic ignored \"-Wimplicit-int-float-conversion\"" )
#endif


/**
 * Perform a cast between numerical types. Will clamp the return value to numerical type limits.
 *
 * In Debug build an assert fires if will not fit into the return type.
 */
template <typename in_type = long long int, typename ret_type = int>
inline constexpr ret_type KiCheckedCast( in_type v )
{
    if constexpr( std::is_same_v<in_type, long long int> && std::is_same_v<ret_type, int> )
    {
        if( v > std::numeric_limits<int>::max() )
        {
            kimathLogOverflow( double( v ), typeid( int ).name() );

            return std::numeric_limits<int>::max();
        }
        else if( v < std::numeric_limits<int>::lowest() )
        {
            kimathLogOverflow( double( v ), typeid( int ).name() );

            return std::numeric_limits<int>::lowest();
        }

        return int( v );
    }
    else
    {
        return v;
    }
}


/**
 * Round a numeric value to an integer using "round halfway cases away from zero" and
 * clamp the result to the limits of the return type.
 *
 * In Debug build an assert fires if will not fit into the return type.
 */
template <typename fp_type, typename ret_type = int>
constexpr ret_type KiROUND( fp_type v, bool aQuiet = false )
{
    using limits = std::numeric_limits<ret_type>;

#if __cplusplus >= 202302L // isnan is not constexpr until C++23
    if constexpr( std::is_floating_point_v<fp_type> )
    {
        if( std::isnan( v ) )
        {
            if( !aQuiet )
                kimathLogOverflow( double( v ), typeid( ret_type ).name() );

            return 0;
        }
    }
#endif

    long long rounded = std::llround( v );
    long long clamped = std::clamp<long long>( rounded,
                                              static_cast<long long>( limits::lowest() ),
                                              static_cast<long long>( limits::max() ) );

    if( !aQuiet && clamped != rounded )
        kimathLogOverflow( double( v ), typeid( ret_type ).name() );

    return static_cast<ret_type>( clamped );
}

#ifdef HAVE_WIMPLICIT_FLOAT_CONVERSION
    _Pragma( "GCC diagnostic pop" )
#endif

/**
 * Scale a number (value) by rational (numerator/denominator). Numerator must be <= denominator.
 */

template <typename T>
T rescale( T aNumerator, T aValue, T aDenominator )
{
    return aNumerator * aValue / aDenominator;
}

template <typename T>
constexpr int sign( T val )
{
    return ( T( 0 ) < val) - ( val < T( 0 ) );
}

// explicit specializations for integer types, taking care of overflow.
template <>
int rescale( int aNumerator, int aValue, int aDenominator );

template <>
int64_t rescale( int64_t aNumerator, int64_t aValue, int64_t aDenominator );


/**
 * Template to compare two floating point values for equality within a required epsilon.
 *
 * @param aFirst value to compare.
 * @param aSecond value to compare.
 * @param aEpsilon allowed error.
 * @return true if the values considered equal within the specified epsilon, otherwise false.
 */
template <class T>
typename std::enable_if<std::is_floating_point<T>::value, bool>::type
equals( T aFirst, T aSecond, T aEpsilon = std::numeric_limits<T>::epsilon() )
{
    const T diff = std::abs( aFirst - aSecond );

    if( diff < aEpsilon )
    {
        return true;
    }

    aFirst = std::abs( aFirst );
    aSecond = std::abs( aSecond );
    T largest = aFirst > aSecond ? aFirst : aSecond;

    if( diff <= largest * aEpsilon )
    {
        return true;
    }

    return false;
}


#endif // UTIL_H
