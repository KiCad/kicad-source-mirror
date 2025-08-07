/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012-2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
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


#ifndef _BASE_UNITS_H_
#define _BASE_UNITS_H_

/*  Note about internal units and max size for boards and items

    The largest distance that we (and Kicad) can support is INT_MAX, since it represents
    distance often in a wxCoord or wxSize. As a scalar, a distance is always
    positive. Because int is 32 bits and INT_MAX is
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

    The next choice is what to use for internal units (IU), sometimes called
    world units.  If nanometers, then the virtual space must be limited to
    about 1.5 x 1.5 meters square.  This is 1518500251 divided by 1e9 nm/meter.

    The maximum zoom factor then depends on the client window size.  If we ask
    wx to handle something outside INT_MIN to INT_MAX, there are unreported
    problems in the non-Debug build because wxRound() goes silent.

    Pcbnew uses nanometers because we need to convert coordinates and size between
    millimeters and inches. using a iu = 1 nm avoid rounding issues

    Gerbview uses iu = 10 nm because we can have coordinates far from origin, and
    1 nm is too small to avoid int overflow.
    (Conversions between millimeters and inches are not critical)
*/

/**
 * @brief some macros and functions to convert a value in mils, decimils or mm to the internal
 * unit used in pcbnew, cvpcb or gerbview (nanometer or deci-mil) depending on compile time option
 */

constexpr double GERB_IU_PER_MM = 1e5; ///< Gerbview IU is 10 nanometers.
constexpr double PCB_IU_PER_MM = 1e6;  ///< Pcbnew IU is 1 nanometer.
constexpr double PL_IU_PER_MM = 1e3;   ///< Internal units in micron (should be enough).
constexpr double SCH_IU_PER_MM = 1e4;  ///< Schematic internal units 1=100nm.

struct EDA_IU_SCALE
{
    const double IU_PER_MM;
    const double IU_PER_MILS;
    const double IU_PER_PS{ 1e6 };        ///< Internal time units are attoseconds
    const double IU_PER_PS_PER_MM{ 1e6 }; ///< Internal delay units are attoseconds/mm
    const double MM_PER_IU;


    constexpr EDA_IU_SCALE( double aIUPerMM ) :
            IU_PER_MM( aIUPerMM ),
            IU_PER_MILS( aIUPerMM * 0.0254 ),
            MM_PER_IU( 1 / IU_PER_MM )
    {
    }

    constexpr inline double IUTomm( int iu ) const { return iu / IU_PER_MM; }

    constexpr inline int mmToIU( double mm ) const
    {
        return (int) ( mm < 0 ? ( mm * IU_PER_MM - 0.5 ) : ( mm * IU_PER_MM + 0.5 ) );
    }

    constexpr inline int MilsToIU( int mils ) const
    {
        double x = mils * IU_PER_MILS;
        return int( x < 0 ? x - 0.5 : x + 0.5 );
    }

    constexpr inline int IUToMils( int iu ) const
    {
        double mils = iu / IU_PER_MILS;

        return static_cast<int>( mils < 0 ? mils - 0.5 : mils + 0.5 );
    }
};

constexpr EDA_IU_SCALE gerbIUScale = EDA_IU_SCALE( GERB_IU_PER_MM );
constexpr EDA_IU_SCALE pcbIUScale = EDA_IU_SCALE( PCB_IU_PER_MM );
constexpr EDA_IU_SCALE drawSheetIUScale = EDA_IU_SCALE( PL_IU_PER_MM );
constexpr EDA_IU_SCALE schIUScale = EDA_IU_SCALE( SCH_IU_PER_MM );
constexpr EDA_IU_SCALE unityScale = EDA_IU_SCALE( 1 );

// Allowed error to approximate an arg by segments, in millimeters
constexpr double ARC_LOW_DEF_MM = 0.02;
constexpr double ARC_HIGH_DEF_MM = 0.005;

#ifndef SWIG
// The max error is the distance between the middle of a segment, and the circle
// for circle/arc to segment approximation.
// Warning: too small values can create very long calculation time in zone filling
// 0.05 to 0.005 mm are reasonable values

// Allowed error to approximate an arg by segments, in Pcbnew IU
constexpr int ARC_LOW_DEF = pcbIUScale.mmToIU( ARC_LOW_DEF_MM );
constexpr int ARC_HIGH_DEF = pcbIUScale.mmToIU( ARC_HIGH_DEF_MM );
#endif

#endif   // _BASE_UNITS_H_
