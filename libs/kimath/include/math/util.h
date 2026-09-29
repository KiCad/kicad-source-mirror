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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
#include <utility>
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

    static_assert( limits::digits <= std::numeric_limits<long long>::digits );

    auto overflow = [&]( ret_type aResult )
    {
        if( !aQuiet )
            kimathLogOverflow( double( v ), typeid( ret_type ).name() );

        return aResult;
    };

    // llround would convert through double and lose precision past 2^53
    if constexpr( std::is_integral_v<fp_type> )
    {
        if( std::cmp_greater( v, limits::max() ) )
            return overflow( limits::max() );

        if( std::cmp_less( v, limits::lowest() ) )
            return overflow( limits::lowest() );

        return static_cast<ret_type>( v );
    }

    // llround is unspecified for NaN and for results beyond long long, so saturate before calling it.
    // v != v stands in for std::isnan, which is not constexpr until C++23.
    if constexpr( std::is_floating_point_v<fp_type> )
    {
        if( v != v )
            return overflow( 0 );

        if( v >= fp_type( limits::max() ) + 0.5 )
            return overflow( limits::max() );

        // lowest - 0.5 can round to lowest itself, which llround still handles exactly
        if( v - fp_type( limits::lowest() ) <= -0.5 )
            return overflow( limits::lowest() );
    }

    long long rounded = std::llround( v );
    long long clamped = std::clamp<long long>( rounded,
                                              static_cast<long long>( limits::lowest() ),
                                              static_cast<long long>( limits::max() ) );

    if( clamped != rounded )
        return overflow( static_cast<ret_type>( clamped ) );

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


template <typename T>
constexpr T ct_sqrt_helper( T aX, T aLo, T aHi )
{
    if( aLo == aHi )
        return aLo;

    const T mid = ( aLo + aHi + 1 ) / 2;

    if( aX / mid < mid )
        return ct_sqrt_helper<T>( aX, aLo, mid - 1 );

    return ct_sqrt_helper<T>( aX, mid, aHi );
}

/**
 * Floor of the square root of an integer, evaluated at compile time.
 */
template <typename T>
constexpr T ct_sqrt( T aX )
{
    return ct_sqrt_helper<T>( aX, 0, aX / 2 + 1 );
}

/**
 * Exact floor of the square root of an integer.  Negative input returns the largest root representable in T.
 */
template <typename T>
T isqrt( T aX )
{
    static_assert( std::is_integral<T>::value, "isqrt requires an integer type" );

    constexpr T sqrt_max = ct_sqrt( std::numeric_limits<T>::max() );

    if constexpr( std::is_signed<T>::value )
    {
        if( aX < 0 )
            return sqrt_max;
    }

    T r = (T) std::sqrt( (double) aX );

    // The double conversion loses precision above 2^53
    while( r < sqrt_max && r * r < aX )
        r++;

    while( r > sqrt_max || r * r > aX )
        r--;

    return r;
}


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
