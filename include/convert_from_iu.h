/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 CERN
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
 * @file convert_from_iu.h
 * @brief Definitions and functions to convert from internal units to user units
 *        depending upon the application and/or build options.
 */


#ifndef _CONVERT_FROM_IU_
#define _CONVERT_FROM_IU_


/// Convert from internal units to user units.
#if defined(PCBNEW) || defined(CVPCB) || defined(GERBVIEW)
    #if defined(GERBVIEW)
        #define MM_PER_IU   1.0 / 1e5         // Gerbview uses 10 micrometer.
    #else
        #define MM_PER_IU   1.0 / 1e6         // Pcbnew in nanometers.
    #endif
    #define MILS_PER_IU ( MM_PER_IU * 0.0254 )
    #define DECIMILS_PER_IU (MM_PER_IU * 0.00254 )

/// Convert PCBNEW internal units (iu) to mils.
inline int Iu2Mils( int iu )
{
    double x = iu * MILS_PER_IU;
    return int( x < 0 ? x - 0.5 : x + 0.5 );
}

/// Convert PCBNEW internal units (iu) to deci-mils.
inline int Iu2DMils( int iu )
{
    double x = iu * DECIMILS_PER_IU;
    return int( x < 0 ? x - 0.5 : x + 0.5 );
}

#else            // Eeschema and anything else.
#define MILS_PER_IU   1.0
#define MM_PER_IU   (MILS_PER_IU / 0.0254)

inline int Iu2Mils( int iu )
{
    return iu;
}
#endif

#endif  // #define _CONVERT_FROM_IU_
