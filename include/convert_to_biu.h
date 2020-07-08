/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012-2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2019 KiCad Developers, see CHANGELOG.TXT for contributors.
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
 * @brief some define and functions to convert a value in mils, decimils or mm
 * to the internal unit used in pcbnew, cvpcb or gerbview (nanometer or deci-mil)
 * depending on compile time option
 */

constexpr double GERB_IU_PER_MM = 1e5;  // Gerbview IU is 10 nanometers.
constexpr double PCB_IU_PER_MM  = 1e6;  // Pcbnew IU is 1 nanometer.
constexpr double PL_IU_PER_MM   = 1e3;  // internal units in micron (should be enough)
constexpr double SCH_IU_PER_MM  = 1e4;  // Schematic internal units 1=100nm

/// Scaling factor to convert mils to internal units.
#if defined(PCBNEW) || defined(CVPCB)
constexpr double IU_PER_MM = PCB_IU_PER_MM;
#elif defined(GERBVIEW)
constexpr double IU_PER_MM = GERB_IU_PER_MM;
#elif defined(PL_EDITOR)
constexpr double IU_PER_MM = PL_IU_PER_MM;
#elif defined(EESCHEMA)
constexpr double IU_PER_MM = SCH_IU_PER_MM;
#else
#define UNKNOWN_IU
#endif

#ifndef UNKNOWN_IU
constexpr double IU_PER_MILS = (IU_PER_MM * 0.0254);

constexpr inline int Mils2iu( int mils )
{
    double x = mils * IU_PER_MILS;
    return int( x < 0 ? x - 0.5 : x + 0.5 );
}

#if defined(EESCHEMA)
constexpr inline int Iu2Mils( int iu )
{
    double mils = iu / IU_PER_MILS;

    return static_cast< int >( mils < 0 ? mils - 0.5 : mils + 0.5 );
}
#else
constexpr inline double Iu2Mils( int iu )
{
    double mils = iu / IU_PER_MILS;

    return static_cast< int >( mils < 0 ? mils - 0.5 : mils + 0.5 );
}
#endif

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
// constexpr inline double Iu2Mils( int iu )
// {
//     return iu / IU_PER_MILS;
// }

// The max error is the distance between the middle of a segment, and the circle
// for circle/arc to segment approximation.
// Warning: too small values can create very long calculation time in zone filling
// 0.05 to 0.005 mm are reasonable values

constexpr int ARC_LOW_DEF  = Millimeter2iu( 0.02 );
constexpr int ARC_HIGH_DEF = Millimeter2iu( 0.005 );

#else
constexpr double PCB_IU_PER_MILS = (PCB_IU_PER_MM * 0.0254);
constexpr double SCH_IU_PER_MILS = (SCH_IU_PER_MM * 0.0254);

constexpr inline int PcbMils2iu( int mils )
{
    double x = mils * PCB_IU_PER_MILS;
    return int( x < 0 ? x - 0.5 : x + 0.5 );
}
constexpr inline int SchMils2iu( int mils )
{
    double x = mils * SCH_IU_PER_MILS;
    return int( x < 0 ? x - 0.5 : x + 0.5 );
}

constexpr inline int PcbMillimeter2iu( double mm )
{
    return (int) ( mm < 0 ? mm * PCB_IU_PER_MM - 0.5 : mm * PCB_IU_PER_MM + 0.5 );
}
constexpr inline int SchMillimeter2iu( double mm )
{
    return (int) ( mm < 0 ? mm * SCH_IU_PER_MM - 0.5 : mm * SCH_IU_PER_MM + 0.5 );
}

constexpr inline double PcbIu2Millimeter( int iu )
{
    return iu / PCB_IU_PER_MM;
}
constexpr inline double SchIu2Millimeter( int iu )
{
    return iu / SCH_IU_PER_MM;
}
#endif

/*  ZOOM LIMITS

    The largest distance that wx can support is INT_MAX, since it represents
    distance often in a wxCoord or wxSize. As a scalar, a distance is always
    positive. On most machines which run KiCad, int is 32 bits and INT_MAX is
    2147483647. The most difficult distance for a virtual (world) cartesian
    space is the hypotenuse, or diagonal measurement at a 45 degree angle. This
    puts the most stress on the distance magnitude within the bounded virtual
    space. So if we allow this distance to be our constraint of <= INT_MAX, this
    constraint then propagates to the maximum distance in X and in Y that can be
    supported on each axis. Remember that the hypotenuse of a 1x1 square is
    sqrt( 1x1 + 1x1 ) = sqrt(2) = 1.41421356.

    hypotenuse of any square = sqrt(2) * deltaX;

    Let maximum supported hypotenuse be INT_MAX, then:

    MAX_AXIS = INT_MAX / sqrt(2) = 2147483647 / 1.41421356 = 1518500251

    This maximum distance is imposed by wxWidgets, not by KiCad. The imposition
    comes in the form of the data structures used in the graphics API at the
    wxDC level. Obviously when we are not interacting with wx we can use double
    to compute distances larger than this. For example the computation of the
    total length of a net, can and should be done in double, since it might
    actually be longer than a single diagonal line.

    The next choice is what to use for internal units (IU), sometimes called
    world units.  If nanometers, then the virtual space must be limited to
    about 1.5 x 1.5 meters square.  This is 1518500251 divided by 1e9 nm/meter.

    The maximum zoom factor then depends on the client window size.  If we ask
    wx to handle something outside INT_MIN to INT_MAX, there are unreported
    problems in the non-Debug build because wxRound() goes silent.

    Let:
        const double MAX_AXIS = 1518500251;

    Then a maximum zoom factor for a screen of 1920 pixels wide is
        1518500251 / 1920 = 790885.

    The largest zoom factor allowed is therefore ~ 300 (which computes to 762000).
*/

#define MAX_ZOOM_FACTOR 300.0

// Adjusted to display zoom level ~ 1 when the screen shows a 1:1 image.
// Obviously depends on the monitor, but this is an acceptable value.
#define ZOOM_COEFF 1.1



#endif  // CONVERT_TO_BIU_H_
