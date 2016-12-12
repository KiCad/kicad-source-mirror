/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012-2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2016 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#ifndef CONVERT_TO_BIU_H_
#define CONVERT_TO_BIU_H_

/**
 * @file convert_to_biu.h
 */

/**
 * @brief some define and functions to convert a value in mils, decimils or mm
 * to the internal unit used in pcbnew, cvpcb or gerbview (nanometer or deci-mil)
 * depending on compile time option
 */

/// Scaling factor to convert mils to internal units.
#if defined(PCBNEW) || defined(CVPCB) || defined(GERBVIEW)
 #if defined(GERBVIEW)
  constexpr double IU_PER_MM = 1e5;     // Gerbview IU is 10 nanometers.
 #else
  constexpr double IU_PER_MM = 1e6;     // Pcbnew IU is 1 nanometer.
 #endif

constexpr double IU_PER_MILS = IU_PER_MM * 0.0254;

/// Convert mils to PCBNEW internal units (iu).
inline int Mils2iu( int mils )
{
    double x = mils * IU_PER_MILS;
    return int( x < 0 ? x - 0.5 : x + 0.5 );
}

#elif defined (PL_EDITOR)
constexpr double IU_PER_MM   =   1e3; // internal units in micron (should be enough)
constexpr double IU_PER_MILS = (IU_PER_MM * 0.0254);

/// Convert mils to page layout editor internal units (iu).
inline int Mils2iu( int mils )
{
    double x = mils * IU_PER_MILS;
    return int( x < 0 ? x - 0.5 : x + 0.5 );
}

#elif defined (EESCHEMA)            // Eeschema
constexpr double IU_PER_MILS = 1.0;
constexpr double IU_PER_MM   = ( IU_PER_MILS / 0.0254 );

constexpr inline int Mils2iu( int mils )
{
    return mils;
}
#else
// Here, we do not know the value of internal units: do not define
// conversion functions (They do not have meaning)
#define UNKNOWN_IU
#endif

#ifndef UNKNOWN_IU
// Other definitions used in a few files
constexpr double MM_PER_IU = ( 1 / IU_PER_MM );

/// Convert mm to internal units (iu).
constexpr inline int Millimeter2iu( double mm )
{
    return (int) ( mm < 0 ? mm * IU_PER_MM - 0.5 : mm * IU_PER_MM + 0.5 );
}

/// Convert mm to internal units (iu).
constexpr inline double Iu2Millimeter( int iu )
{
    return iu / IU_PER_MM;
}

/// Convert mm to internal units (iu).
constexpr inline double Iu2Mils( int iu )
{
    return iu / IU_PER_MILS;
}
#endif

#endif  // CONVERT_TO_BIU_H_
