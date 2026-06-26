/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#define BOOST_TEST_NO_MAIN
#include <boost/test/unit_test.hpp>

#include <vector>

#include <widgets/wx_grid.h>


BOOST_AUTO_TEST_SUITE( WxGrid )


// A grid with more rows than the cap must report a height bounded to the cap, so the dialog
// hosting it can still be shrunk rather than being forced taller than the screen.
BOOST_AUTO_TEST_CASE( CapHeightCapsTallGrid )
{
    const int              header = 24;
    const std::vector<int> rows( 20, 25 );
    const int              fullHeight = header + 20 * 25;

    int capped = WX_GRID::CapHeightToVisibleRows( fullHeight, header, rows, 4 );

    BOOST_CHECK_EQUAL( capped, header + 4 * 25 );
    BOOST_CHECK_LT( capped, fullHeight );
}


// When the grid has fewer rows than the cap, the cap must not shrink it below its own content.
BOOST_AUTO_TEST_CASE( CapHeightKeepsShortGrid )
{
    const int              header = 24;
    const std::vector<int> rows( 2, 25 );
    const int              fullHeight = header + 2 * 25;

    int capped = WX_GRID::CapHeightToVisibleRows( fullHeight, header, rows, 4 );

    BOOST_CHECK_EQUAL( capped, fullHeight );
}


// A negative cap leaves the advertised height untouched (the pre-cap behaviour).
BOOST_AUTO_TEST_CASE( CapHeightDisabled )
{
    const int              header = 24;
    const std::vector<int> rows( 20, 25 );
    const int              fullHeight = header + 20 * 25;

    BOOST_CHECK_EQUAL( WX_GRID::CapHeightToVisibleRows( fullHeight, header, rows, -1 ), fullHeight );
}


BOOST_AUTO_TEST_SUITE_END()
