/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (c) 2005 Michael Niedermayer <michaelni@gmx.at>
 * Copyright (C) CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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
#include <cstdint>
#include <limits>
#include <typeinfo>

/**
 * Helper to avoid directly including wx/log.h for the templated functions in kimath
 */
void kimathLogDebug( const char* aFormatString, ... );

/**
 * Function Clamp
 * limits @a value within the range @a lower <= @a value <= @a upper.  It will work
 * on temporary expressions, since they are evaluated only once, and it should work
 * on most if not all numeric types, string types, or any type for which "operator < ()"
 * is present. The arguments are accepted in this order so you can remember the
 * expression as a memory aid:
 * <p>
 * result is:  lower <= value <= upper
 */
template <typename T> inline const T& Clamp( const T& lower, const T& value, const T& upper )
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

    if( std::numeric_limits<ret_type>::max() < ret ||
        std::numeric_limits<ret_type>::lowest() > ret )
    {
        kimathLogDebug( "Overflow KiROUND converting value %f to %s", double( v ),
                        typeid( ret_type ).name() );
        return 0;
    }

    return ret_type( max_ret( ret ) );
}

#ifdef HAVE_WIMPLICIT_FLOAT_CONVERSION
    _Pragma( "GCC diagnostic pop" )
#endif

/**
 * Function rescale()
 *
 * Scales a number (value) by rational (numerator/denominator). Numerator must be <= denominator.
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

#endif // UTIL_H
