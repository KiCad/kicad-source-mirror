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

#include <algorithm>

#include <boost/test/unit_test.hpp>

#include <base_units.h>
#include <gerber_draw_item.h>
#include <gerber_file_image.h>
#include <qa_utils/wx_utils/unit_test_utils.h>


BOOST_AUTO_TEST_SUITE( GerbviewCoordinates )


BOOST_AUTO_TEST_CASE( DecimalModeDoesNotLeakBetweenAxes )
{
    wxString          path = KI_TEST::GetTestDataRootDir() + "gerbview/mixed_coordinates.gbr";
    GERBER_FILE_IMAGE image( 0 );

    BOOST_REQUIRE( image.LoadGerberFile( path ) );
    BOOST_CHECK_EQUAL( image.GetMessages().GetCount(), 0u );

    const GERBER_DRAW_ITEMS& items = image.GetItems();
    BOOST_REQUIRE_EQUAL( items.size(), 10u );

    // Both squares have 1 mm sides. The second starts at X=3 mm and uses decimal X
    // with fixed-format Y. A leaked decimal flag stretches its height to 10 mm.
    const int      unit = gerbIUScale.mmToIU( 1.0 );
    const VECTOR2I corners[] = { { 0, 0 }, { unit, 0 }, { unit, unit }, { 0, unit } };

    for( int square = 0; square < 2; ++square )
    {
        VECTOR2I offset( square * 3 * unit, 0 );

        for( int edge = 0; edge < 4; ++edge )
        {
            BOOST_TEST_CONTEXT( "square " << square << ", edge " << edge )
            {
                const GERBER_DRAW_ITEM* item = items[square * 4 + edge];
                VECTOR2I                start = corners[edge] + offset;
                VECTOR2I                end = corners[( edge + 1 ) % 4] + offset;

                BOOST_CHECK_EQUAL( item->m_Start.x, start.x );
                BOOST_CHECK_EQUAL( item->m_Start.y, start.y );
                BOOST_CHECK_EQUAL( item->m_End.x, end.x );
                BOOST_CHECK_EQUAL( item->m_End.y, end.y );
            }
        }
    }

    // Both centers are offset (1, 1) mm from the Gerber start point. The second
    // arc uses decimal I with fixed-format J, which must still mean 1 mm.
    for( int arc = 0; arc < 2; ++arc )
    {
        BOOST_TEST_CONTEXT( "arc " << arc )
        {
            const GERBER_DRAW_ITEM* item = items[8 + arc];
            const int               offsetX = arc * 4 * unit;

            BOOST_CHECK_EQUAL( item->m_ShapeType, GBR_ARC );
            BOOST_CHECK_EQUAL( item->m_ArcCentre.x, offsetX + unit );
            BOOST_CHECK_EQUAL( item->m_ArcCentre.y, 4 * unit );
            BOOST_CHECK_EQUAL( std::min( item->m_Start.x, item->m_End.x ), offsetX );
            BOOST_CHECK_EQUAL( std::max( item->m_Start.x, item->m_End.x ), offsetX + 2 * unit );
            BOOST_CHECK_EQUAL( item->m_Start.y, 3 * unit );
            BOOST_CHECK_EQUAL( item->m_End.y, 3 * unit );
            BOOST_CHECK_EQUAL( item->m_Start.x - offsetX, items[8]->m_Start.x );
            BOOST_CHECK_EQUAL( item->m_End.x - offsetX, items[8]->m_End.x );
        }
    }
}


BOOST_AUTO_TEST_CASE( FractionalScaleFactorsRoundTrip )
{
    wxString          path = KI_TEST::GetTestDataRootDir() + "gerbview/fractional_scale.gbr";
    GERBER_FILE_IMAGE image( 0 );

    BOOST_REQUIRE( image.LoadGerberFile( path ) );
    BOOST_CHECK_EQUAL( image.GetMessages().GetCount(), 0u );

    const GERBER_DRAW_ITEMS& items = image.GetItems();
    BOOST_REQUIRE_EQUAL( items.size(), 2u );

    const int      unit = gerbIUScale.mmToIU( 1.0 );
    const VECTOR2I expected[] = { { unit, -3 * unit }, { 3 * unit, -unit } };

    // Swap the fractional scales between flashes to exercise both axes and ensure
    // each item keeps its own scale. Display coordinates invert the Gerber Y axis.
    for( size_t i = 0; i < items.size(); ++i )
    {
        BOOST_TEST_CONTEXT( "flash " << i )
        {
            const GERBER_DRAW_ITEM* item = items[i];
            VECTOR2I                position = item->GetABPosition( item->m_Start );

            // Require correct forward scaling before trying the inverse: truncating
            // 0.5 to zero used to cause integer division by zero in GetXYPosition().
            BOOST_REQUIRE_EQUAL( position.x, expected[i].x );
            BOOST_REQUIRE_EQUAL( position.y, expected[i].y );

            VECTOR2I original = item->GetXYPosition( position );

            BOOST_CHECK_EQUAL( original.x, 2 * unit );
            BOOST_CHECK_EQUAL( original.y, 2 * unit );
        }
    }
}


BOOST_AUTO_TEST_SUITE_END()
