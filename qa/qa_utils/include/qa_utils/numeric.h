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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file numeric.h
 * Numerical test predicates.
 */

#ifndef NUMERIC__H
#define NUMERIC__H

#include <cmath>

namespace KI_TEST
{

/**
 * Check if a value is within a tolerance of a nominal value, wrapping to a given val
 *
 * @return value is in [( aNominal - aError ) % aWrap, ( aNominal + aError ) % aWrap]
 */
template <typename T>
bool IsWithinWrapped( T aValue, T aNominal, T aWrap, T aError )
{
    // Compute shortest signed distance on a ring
    double diff = std::fmod( static_cast<double>( aValue - aNominal ), static_cast<double>( aWrap ) );

    if( diff > aWrap / 2.0 )
        diff -= aWrap;
    else if( diff < -aWrap / 2.0 )
        diff += aWrap;

    return std::abs( diff ) <= aError;
}

/**
 * Check if a value is within a tolerance of a nominal value
 *
 * @return value is in [aNominal - aError, aNominal + aError]
 */
template <typename T> bool IsWithin( T aValue, T aNominal, T aError )
{
    return ( aValue >= aNominal - aError ) && ( aValue <= aNominal + aError );
}

/**
 * Check if a value is within a tolerance of a nominal value,
 * with different allowances for errors above and below.
 *
 * @return value is in [aNominal - aErrorBelow, aNominal + aErrorAbove]
 */
template <typename T> bool IsWithinBounds( T aValue, T aNominal, T aErrorAbove, T aErrorBelow )
{
    return ( aValue >= aNominal - aErrorBelow ) && ( aValue <= aNominal + aErrorAbove );
}

/**
 * value is in range [aNominal - aErrorBelow, aNominal]
 */
template <typename T> bool IsWithinAndBelow( T aValue, T aNominal, T aErrorBelow )
{
    return IsWithinBounds( aValue, aNominal, 0, aErrorBelow );
}

/**
 * value is in range [aNominal, aNominal + aErrorAbove]
 */
template <typename T> bool IsWithinAndAbove( T aValue, T aNominal, T aErrorAbove )
{
    return IsWithinBounds( aValue, aNominal, aErrorAbove, 0 );
}

} // namespace KI_TEST

#endif // NUMERIC__H
