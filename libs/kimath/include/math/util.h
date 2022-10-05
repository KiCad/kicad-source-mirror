/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (c) 2005 Michael Niedermayer <michaelni@gmx.at>
 * Copyright (C) CERN
 * Copyright (C) 2021-2022 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <cmath>
#include <cstdint>
#include <limits>
#include <typeinfo>
#include <type_traits>

/**
 * Helper to avoid directly including wx/log.h for the templated functions in kimath
 */
void kimathLogDebug( const char* aFormatString, ... );

/**
 * Workaround to avoid the empty-string conversion issue in wxWidgets
 */
void kimathLogOverflow( double v, const char* aTypeName );

/**
 * Limit @a value within the range @a lower <= @a value <= @a upper.
 *
 * It will work on temporary expressions, since they are evaluated only once, and it should
 * work on most if not all numeric types, string types, or any type for which "operator < ()"
 * is present. The arguments are accepted in this order so you can remember the expression as
 * a memory aid:
 * <p>
 * result is:  lower <= value <= upper
 *</p>
 */
template <typename T> inline constexpr T Clamp( const T& lower, const T& value, const T& upper )
{
    if( value < lower )
        return lower;
    else if( upper < value )
        return upper;
    return value;
}

// Suppress an annoying warning that the explicit rounding we do is not precise
#ifdef HAVE_WIMPLICIT_FLOAT_CONVERSION
    _Pragma( "GCC diagnostic push" ) \
    _Pragma( "GCC diagnostic ignored \"-Wimplicit-int-float-conversion\"" )
#endif

/**
 * Round a floating point number to an integer using "round halfway cases away from zero".
 *
 * In Debug build an assert fires if will not fit into the return type.
 */
template <typename fp_type, typename ret_type = int>
constexpr ret_type KiROUND( fp_type v )
{
    using max_ret = long long int;
    fp_type ret = v < 0 ? v - 0.5 : v + 0.5;

    if( ret > std::numeric_limits<ret_type>::max() )
    {
        kimathLogOverflow( double( v ), typeid( ret_type ).name() );
        
        return std::numeric_limits<ret_type>::max() - 1;
    }
    else if( ret < std::numeric_limits<ret_type>::lowest() )
    {
        kimathLogOverflow( double( v ), typeid( ret_type ).name() );

        if( std::numeric_limits<ret_type>::is_signed )
            return std::numeric_limits<ret_type>::lowest() + 1;
        else
            return 0;
    }

    return ret_type( max_ret( ret ) );
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
int sign( T val )
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
    T diff = std::abs( aFirst - aSecond );

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
