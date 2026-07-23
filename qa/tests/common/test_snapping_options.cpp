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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#define BOOST_TEST_NO_MAIN
#include <boost/test/unit_test.hpp>

#include <algorithm>

#include <dialogs/panel_snapping.h>


BOOST_AUTO_TEST_SUITE( SnappingOptions )


BOOST_AUTO_TEST_CASE( RoutingEditorsOfferTheRouterOnlyMode )
{
    std::vector<MAGNETIC_OPTIONS> routing = MagneticSnapOptions( true );
    std::vector<MAGNETIC_OPTIONS> plain = MagneticSnapOptions( false );

    BOOST_CHECK_EQUAL( routing.size(), 3u );
    BOOST_CHECK_EQUAL( plain.size(), 2u );

    BOOST_CHECK( routing[1] == MAGNETIC_OPTIONS::CAPTURE_CURSOR_IN_TRACK_TOOL );
    BOOST_CHECK( std::find( plain.begin(), plain.end(), MAGNETIC_OPTIONS::CAPTURE_CURSOR_IN_TRACK_TOOL )
                 == plain.end() );
}


BOOST_AUTO_TEST_CASE( EveryOfferedOptionRoundTrips )
{
    for( bool routing : { false, true } )
    {
        std::vector<MAGNETIC_OPTIONS> options = MagneticSnapOptions( routing );

        for( MAGNETIC_OPTIONS option : options )
        {
            int index = MagneticSnapIndex( options, option );
            BOOST_CHECK( MagneticSnapValue( options, index ) == option );
        }
    }
}


BOOST_AUTO_TEST_CASE( UnofferedOptionReadsBackAsNever )
{
    // The footprint editor has no router, and its grid helper snaps to pads only in the "always"
    // mode, so a router-only value left in the config must not read back as any kind of snapping.
    std::vector<MAGNETIC_OPTIONS> options = MagneticSnapOptions( false );
    int index = MagneticSnapIndex( options, MAGNETIC_OPTIONS::CAPTURE_CURSOR_IN_TRACK_TOOL );

    BOOST_CHECK( MagneticSnapValue( options, index ) == MAGNETIC_OPTIONS::NO_EFFECT );
}


BOOST_AUTO_TEST_CASE( OutOfRangeIndexIsNever )
{
    std::vector<MAGNETIC_OPTIONS> options = MagneticSnapOptions( true );

    // wxChoice reports wxNOT_FOUND when nothing is selected.
    BOOST_CHECK( MagneticSnapValue( options, -1 ) == MAGNETIC_OPTIONS::NO_EFFECT );
    BOOST_CHECK( MagneticSnapValue( options, 3 ) == MAGNETIC_OPTIONS::NO_EFFECT );
}


BOOST_AUTO_TEST_SUITE_END()
