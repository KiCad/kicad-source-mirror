/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * Regression test for the Excellon repeat hole command R#(X#Y#).
 *
 * The command repeats the previous hole a number of times, adding an
 * incremental X and/or Y step each time. It was not implemented, so every R
 * line was rejected with "Unexpected symbol 0x52 <R>" and its holes dropped.
 */

#include <cstdlib>

#include <boost/test/unit_test.hpp>

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <gerber_file_image.h>
#include <excellon_image.h>
#include <excellon_defaults.h>
#include <gerber_draw_item.h>


BOOST_AUTO_TEST_SUITE( GerbviewExcellonRepeat )


BOOST_AUTO_TEST_CASE( RepeatHoleCommand )
{
    wxString path = KI_TEST::GetTestDataRootDir() + "gerbview/excellon/repeat_holes.drl";

    EXCELLON_IMAGE    image( 0 );
    EXCELLON_DEFAULTS defaults;

    BOOST_REQUIRE( image.LoadFile( path, &defaults ) );

    // The R lines used to each add an "Unexpected symbol 0x52" message.
    BOOST_CHECK_EQUAL( image.GetMessages().GetCount(), 0u );

    // T01: 5 rows of a hole plus R07X5000 -> 8 holes each = 40 holes
    //      rows: items 0..7, 8..15, 16..23, 24..31, 32..39
    // T02: one hole plus R03Y5000 -> 4 holes = items 40..43
    BOOST_REQUIRE_EQUAL( image.GetItemsCount(), 44 );

    const GERBER_DRAW_ITEMS& items = image.GetItems();

    // Step of the X repeat, taken from the first row. Checks are on deltas only,
    // so they are independent of the internal unit scale.
    VECTOR2I stepX = items[1]->GetPosition() - items[0]->GetPosition();
    BOOST_CHECK( stepX.x != 0 );
    BOOST_CHECK_EQUAL( stepX.y, 0 );

    // All 8 holes in each of the 5 rows are evenly spaced by stepX.
    for( int row = 0; row < 5; ++row )
    {
        for( int col = 0; col < 7; ++col )
        {
            int i = row * 8 + col;
            BOOST_CHECK( ( items[i + 1]->GetPosition() - items[i]->GetPosition() ) == stepX );
        }
    }

    // Rows are one explicit 5 mm Y move apart. That move must match the R X step
    // magnitude, which ties the repeat step to a non-repeat move of known size.
    VECTOR2I rowStep = items[8]->GetPosition() - items[0]->GetPosition();
    BOOST_CHECK_EQUAL( rowStep.x, 0 );
    BOOST_CHECK_EQUAL( std::abs( rowStep.y ), std::abs( stepX.x ) );

    // T02 column: R03Y5000 gives 4 evenly spaced holes stepping in Y only, same
    // 5 mm magnitude as the X repeat.
    VECTOR2I stepY = items[41]->GetPosition() - items[40]->GetPosition();
    BOOST_CHECK_EQUAL( stepY.x, 0 );
    BOOST_CHECK_EQUAL( std::abs( stepY.y ), std::abs( stepX.x ) );
    BOOST_CHECK( ( items[42]->GetPosition() - items[41]->GetPosition() ) == stepY );
    BOOST_CHECK( ( items[43]->GetPosition() - items[42]->GetPosition() ) == stepY );
}


BOOST_AUTO_TEST_SUITE_END()
