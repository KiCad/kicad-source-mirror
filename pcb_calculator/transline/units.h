/*
 * units.h - some conversion definitions
 *
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

#ifndef __UNITS_H
#define __UNITS_H

#include <config.h>
#include <cmath>

#include "units_scales.h"

#ifndef HAVE_CMATH_ASINH
inline double asinh( double x )
{
    return log( x+sqrt(x*x+1) );
}
#endif

#ifndef HAVE_CMATH_ACOSH
inline double acosh( double x )
{
    // must be x>=1, if not return Nan (Not a Number)
    if( x < 1.0 ) return sqrt( -1.0 );

    // return only the positive result (as sqrt does).
    return log( x+sqrt( x*x-1.0 ) );
}
#endif

#ifndef HAVE_CMATH_ATANH
inline double atanh( double x )
{
    // must be x>-1, x<1, if not return Nan (Not a Number)
    if( !(x>-1.0 && x<1.0) ) return sqrt( -1.0 );

    return log( (1.0+x)/(1.0-x) ) / 2.0;
}
#endif

#define MU0  12.566370614e-7          // magnetic constant
#define C0   299792458.0              // speed of light in vacuum

// wave resistance in vacuum
//
// - From 1948 to 2019, Z₀ was defined to be exactly π*119.9169832 Ω (≈376.730_313_462 Ω).
//
// - The 2019 revision to SI changed it from a defined value to a measured value.  In accordance
//   with this, the "2018" revision (based on 2018 measurements, values published 2019, rationale
//   published 2021) of the "CODATA Recommended Values of the Fundamental Physical Constants"
//   (<https://tsapps.nist.gov/publication/get_pdf.cfm?pub_id=931443>) gave the accepted measured
//   value as 376.730_313_668(57) Ω (the number in parenthesis is the margin of error).
//
// - The "2022" revision (based on 2022 measurements, values published 2024, rationale published
//   2025) of CODATA (<https://tsapps.nist.gov/publication/get_pdf.cfm?pub_id=958143>) gave the
//   accepted measured value as 376.730_313_412(59) Ω.
//
// The most recent CODATA value can always be found at
// <https://physics.nist.gov/cgi-bin/cuu/Value?z0>.
//
// - From its first use in 2011 (pre-v4), KiCad used the value 376.730_313_469_585_043_649_63, which
//   mysteriously doesn't quite match any of the above.
//
// - In 2021 (v6), KiCad updated to the 2018 CODATA value.
//
// - In 2026, KiCad updated to the 2022 CODATA value.
#define ZF0 376.730313412

// const to convert a attenuation / loss from log (Neper) to decibel
// (1 Np = 8.68589 dB)
const double LOG2DB = 20.0 / log( 10.0 );

#endif /* __UNITS_H */
