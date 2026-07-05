/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <cmath>

#include <boost/test/unit_test.hpp>

#include <track_width_calculations.h>

/**
 * Tests for the IPC-2152 (Brooks & Adam) track width / current calculations.
 *
 * Reference values are computed directly from the published fit
 * dT = K * I^a * W^b * Th^c (Brooks & Adam, "Trace Currents and Temperatures Revisited," 2015,
 * Table 3-1) with W and Th in mils and I in amperes.
 */
BOOST_AUTO_TEST_SUITE( TrackWidthCalculations )


// External coefficients: K = 215.3, a = 2, b = -1.15, c = -1.0
BOOST_AUTO_TEST_CASE( ExternalCurrentFromWidth )
{
    // 20 mil wide, 1 oz (1.378 mil) thick, 10 C rise.
    double current = TRACK_WIDTH_CALCULATIONS::CurrentFromWidth( 20.0, 1.378, 10.0, false );

    BOOST_CHECK_CLOSE( current, 1.4164291025677085, 1e-6 );
}


// Internal 1 oz coefficients (default UI thickness): K = 200, a = 1.9, b = -1.10, c = -1.52
BOOST_AUTO_TEST_CASE( InternalCurrentFromWidth )
{
    double current = TRACK_WIDTH_CALCULATIONS::CurrentFromWidth( 20.0, 1.378, 10.0, true );

    BOOST_CHECK_CLOSE( current, 1.513124408404138, 1e-6 );
}


// IPC-2152 reverses the IPC-2221 assumption that internal traces run much hotter; the data showed
// internal traces cool about as well as (or better than) external ones, so an internal trace is
// not derated by the historical factor of two.
BOOST_AUTO_TEST_CASE( InternalNotHalvedRelativeToExternal )
{
    double external = TRACK_WIDTH_CALCULATIONS::CurrentFromWidth( 20.0, 1.378, 10.0, false );
    double internal = TRACK_WIDTH_CALCULATIONS::CurrentFromWidth( 20.0, 1.378, 10.0, true );

    BOOST_CHECK_GT( internal, 0.5 * external );
}


// Coefficient selection should bucket by nominal copper weight; a 2 oz internal trace uses the
// 2 oz fit (K = 300, a = 2), distinct from the 1 oz fit.
BOOST_AUTO_TEST_CASE( InternalCopperWeightSelection )
{
    double current = TRACK_WIDTH_CALCULATIONS::CurrentFromWidth( 20.0, 2.756, 10.0, true );

    // K=300, a=2, b=-1.15, c=-1.52 at W=20, Th=2.756, dT=10
    BOOST_CHECK_CLOSE( current, 2.208736063285966, 1e-6 );
}


// CurrentFromWidth and WidthFromCurrent must be exact inverses.
BOOST_AUTO_TEST_CASE( RoundTripExternal )
{
    const double width = 25.0;
    const double thickness = 1.378;
    const double deltaT = 30.0;

    double current = TRACK_WIDTH_CALCULATIONS::CurrentFromWidth( width, thickness, deltaT, false );
    double recovered = TRACK_WIDTH_CALCULATIONS::WidthFromCurrent( current, thickness, deltaT, false );

    BOOST_CHECK_CLOSE( recovered, width, 1e-6 );
}


BOOST_AUTO_TEST_CASE( RoundTripInternal )
{
    const double width = 25.0;
    const double thickness = 1.378;
    const double deltaT = 30.0;

    double current = TRACK_WIDTH_CALCULATIONS::CurrentFromWidth( width, thickness, deltaT, true );
    double recovered = TRACK_WIDTH_CALCULATIONS::WidthFromCurrent( current, thickness, deltaT, true );

    BOOST_CHECK_CLOSE( recovered, width, 1e-6 );
}


// A wider, thicker trace and a larger allowed temperature rise must each increase current capacity.
BOOST_AUTO_TEST_CASE( Monotonicity )
{
    double base = TRACK_WIDTH_CALCULATIONS::CurrentFromWidth( 20.0, 1.378, 10.0, false );

    BOOST_CHECK_GT( TRACK_WIDTH_CALCULATIONS::CurrentFromWidth( 40.0, 1.378, 10.0, false ), base );
    BOOST_CHECK_GT( TRACK_WIDTH_CALCULATIONS::CurrentFromWidth( 20.0, 2.756, 10.0, false ), base );
    BOOST_CHECK_GT( TRACK_WIDTH_CALCULATIONS::CurrentFromWidth( 20.0, 1.378, 40.0, false ), base );
}


BOOST_AUTO_TEST_SUITE_END()
