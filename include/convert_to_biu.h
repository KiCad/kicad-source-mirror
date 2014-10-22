/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012-2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2014 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#include <config.h>

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
  #define IU_PER_MM        1e5     // Gerbview IU is 10 nanometers.
 #else
  #define IU_PER_MM        1e6     // Pcbnew IU is 1 nanometer.
 #endif
 #define IU_PER_MILS       (IU_PER_MM * 0.0254)
 #define IU_PER_DECIMILS   (IU_PER_MM * 0.00254)

/// Convert mils to PCBNEW internal units (iu).
inline int Mils2iu( int mils )
{
    double x = mils * IU_PER_MILS;
    return int( x < 0 ? x - 0.5 : x + 0.5 );
}

/// Convert deci-mils to PCBNEW internal units (iu).
inline int DMils2iu( int dmils )
{
    double x = dmils * IU_PER_DECIMILS;
    return int( x < 0 ? x - 0.5 : x + 0.5 );
}

#elif defined (PL_EDITOR)
#define IU_PER_MM           1e3 // internal units in micron (should be enough)
#define IU_PER_MILS       (IU_PER_MM * 0.0254)
#define IU_PER_DECIMILS   (IU_PER_MM * 0.00254)
/// Convert mils to page layout editor internal units (iu).
inline int Mils2iu( int mils )
{
    double x = mils * IU_PER_MILS;
    return int( x < 0 ? x - 0.5 : x + 0.5 );
}

/// Convert deci-mils to page layout editor internal units (iu).
inline int DMils2iu( int dmils )
{
    double x = dmils * IU_PER_DECIMILS;
    return int( x < 0 ? x - 0.5 : x + 0.5 );
}

#else            // Eeschema and anything else.
#define IU_PER_DECIMILS     0.1
#define IU_PER_MILS         1.0
#define IU_PER_MM           (IU_PER_MILS / 0.0254)

inline int Mils2iu( int mils )
{
    return mils;
}
#endif

/// Convert mm to internal units (iu).
inline int Millimeter2iu( double mm )
{
    return (int) ( mm < 0 ? mm * IU_PER_MM - 0.5 : mm * IU_PER_MM + 0.5);
}


#endif  // CONVERT_TO_BIU_H_
