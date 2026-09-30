/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#pragma once

/**
 * @file math/wide_int.h
 * @brief 128-bit integers for exact products of 64-bit coordinate deltas.
 *
 * Coordinates span 2^32 units, so deltas need 33 bits and their cross and dot products need 66.
 * KI_INT128 holds those products exactly.
 */

#include <bit>
#include <cmath>
#include <cstdint>

#include <math/vector2d.h>

#if defined( _MSC_VER )
// The standard has no 128-bit integer; this is the STL's own integer-class type (it backs iota_view).
// Also used for clang-cl, whose __int128 cannot convert to double without compiler-rt.
#include <__msvc_int128.hpp>
#endif


namespace KIGEOM_WIDE
{
/// Nearest double to hi * 2^64 + lo, ties to even.
inline double WordsToDouble( int64_t aHi, uint64_t aLo )
{
    const bool negative = aHi < 0;
    uint64_t   hi = static_cast<uint64_t>( aHi );
    uint64_t   lo = aLo;

    if( negative )
    {
        lo = ~lo + 1;
        hi = ~hi + ( lo == 0 ? 1 : 0 );
    }

    double magnitude;

    if( hi == 0 )
    {
        magnitude = static_cast<double>( lo );
    }
    else
    {
        // Fold the discarded low bits into a sticky bit so the single uint64 to double rounding
        // matches rounding the full 128-bit value
        int      shift = std::countl_zero( hi );
        uint64_t top = shift == 0 ? hi : ( hi << shift ) | ( lo >> ( 64 - shift ) );

        if( ( lo << shift ) != 0 )
            top |= 1;

        magnitude = std::ldexp( static_cast<double>( top ), 64 - shift );
    }

    return negative ? -magnitude : magnitude;
}
} // namespace KIGEOM_WIDE


#if defined( _MSC_VER )
using KI_INT128 = std::_Signed128;
using KI_UINT128 = std::_Unsigned128;

inline double ToDouble( const KI_INT128& aValue )
{
    return KIGEOM_WIDE::WordsToDouble( static_cast<int64_t>( aValue._Word[1] ), aValue._Word[0] );
}
#else
using KI_INT128 = __int128;
using KI_UINT128 = unsigned __int128;

inline double ToDouble( KI_INT128 aValue )
{
    return static_cast<double>( aValue );
}
#endif


/**
 * Exact aA.x * aB.y - aA.y * aB.x.  Never overflows for any int64 components.
 */
constexpr KI_INT128 CrossWide( const VECTOR2L& aA, const VECTOR2L& aB )
{
    return KI_INT128( aA.x ) * aB.y - KI_INT128( aA.y ) * aB.x;
}


/**
 * Exact aA.x * aB.x + aA.y * aB.y.  Overflows only when all four components are INT64_MIN.
 */
constexpr KI_INT128 DotWide( const VECTOR2L& aA, const VECTOR2L& aB )
{
    return KI_INT128( aA.x ) * aB.x + KI_INT128( aA.y ) * aB.y;
}
