/*
 * Copyright (C) 1992-2011 jean-pierre.charras
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
 * along with this package; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street - Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef TRANSLINE_CALCULATIONS_UNITS_H
#define TRANSLINE_CALCULATIONS_UNITS_H


#include <cmath>


#ifndef HAVE_CMATH_ASINH
inline double asinh( double x )
{
    return log( x + sqrt( x * x + 1 ) );
}
#endif

#ifndef HAVE_CMATH_ACOSH
inline double acosh( double x )
{
    // must be x>=1, if not return Nan (Not a Number)
    if( x < 1.0 )
        return sqrt( -1.0 );

    // return only the positive result (as sqrt does).
    return log( x + sqrt( x * x - 1.0 ) );
}
#endif

#ifndef HAVE_CMATH_ATANH
inline double atanh( double x )
{
    // must be x>-1, x<1, if not return Nan (Not a Number)
    if( !( x > -1.0 && x < 1.0 ) )
        return sqrt( -1.0 );

    return log( ( 1.0 + x ) / ( 1.0 - x ) ) / 2.0;
}
#endif

namespace TRANSLINE_CALCULATIONS
{
constexpr double MU0 = 12.566370614e-7; // magnetic constant
constexpr double E0 = 8.854e-12;        // permittivity of free space
constexpr double C0 = 299792458.0;      // speed of light in vacuum
constexpr double ZF0 = 376.730313668;   // wave resistance in vacuum

// const to convert a attenuation / loss from log (Neper) to decibel
// (1 Np = 8.68589 dB)
const double LOG2DB = 20.0 / log( 10.0 );

// ZF0 value update:
// https://physics.nist.gov/cgi-bin/cuu/Value?z0
}; // namespace TRANSLINE_CALCULATIONS

#endif // TRANSLINE_CALCULATIONS_UNITS_H
