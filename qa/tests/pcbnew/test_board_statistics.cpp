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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <board_statistics.h>
#include <board_statistics_report.h>
#include <base_units.h>
#include <algorithm>
#include <vector>


BOOST_AUTO_TEST_SUITE( BoardStatistics )


/**
 * Test that DRILL_LINE_ITEM::COMPARE satisfies strict weak ordering for COL_PLATED.
 * This is a regression test for issue #22708.
 */
BOOST_AUTO_TEST_CASE( DrillCompareStrictWeakOrderingPlated )
{
    // Create test items with different plated values
    DRILL_LINE_ITEM platedItem( 100, 100, PAD_DRILL_SHAPE::CIRCLE, true, true, F_Cu, B_Cu );
    DRILL_LINE_ITEM unplatedItem( 100, 100, PAD_DRILL_SHAPE::CIRCLE, false, true, F_Cu, B_Cu );

    platedItem.m_Qty = 1;
    unplatedItem.m_Qty = 1;

    // Test ascending comparator
    DRILL_LINE_ITEM::COMPARE cmpAsc( DRILL_LINE_ITEM::COL_PLATED, true );

    // Strict weak ordering requirement 1: irreflexivity - compare(a, a) must be false
    BOOST_CHECK( !cmpAsc( platedItem, platedItem ) );
    BOOST_CHECK( !cmpAsc( unplatedItem, unplatedItem ) );

    // Strict weak ordering requirement 2: asymmetry - if compare(a, b) then !compare(b, a)
    bool aLessB = cmpAsc( unplatedItem, platedItem );  // false < true
    bool bLessA = cmpAsc( platedItem, unplatedItem );  // true < false

    BOOST_CHECK( aLessB != bLessA || (!aLessB && !bLessA) );

    // Test that sorting works without crashing (this would cause undefined behavior with broken comparator)
    std::vector<DRILL_LINE_ITEM> items;
    items.push_back( platedItem );
    items.push_back( unplatedItem );
    items.push_back( platedItem );
    items.push_back( unplatedItem );

    BOOST_CHECK_NO_THROW( std::sort( items.begin(), items.end(), cmpAsc ) );

    // Verify the sort order is correct: false (0) values should come before true (1) values
    BOOST_CHECK( !items[0].isPlated );
    BOOST_CHECK( !items[1].isPlated );
    BOOST_CHECK( items[2].isPlated );
    BOOST_CHECK( items[3].isPlated );

    // Test descending order
    DRILL_LINE_ITEM::COMPARE cmpDesc( DRILL_LINE_ITEM::COL_PLATED, false );

    BOOST_CHECK( !cmpDesc( platedItem, platedItem ) );
    BOOST_CHECK( !cmpDesc( unplatedItem, unplatedItem ) );

    BOOST_CHECK_NO_THROW( std::sort( items.begin(), items.end(), cmpDesc ) );

    // Verify descending order: true (1) values should come before false (0) values
    BOOST_CHECK( items[0].isPlated );
    BOOST_CHECK( items[1].isPlated );
    BOOST_CHECK( !items[2].isPlated );
    BOOST_CHECK( !items[3].isPlated );
}


/**
 * Test that DRILL_LINE_ITEM::COMPARE satisfies strict weak ordering for COL_VIA_PAD.
 * This is a regression test for issue #22708.
 */
BOOST_AUTO_TEST_CASE( DrillCompareStrictWeakOrderingViaPad )
{
    // Create test items with different isPad values
    DRILL_LINE_ITEM padItem( 100, 100, PAD_DRILL_SHAPE::CIRCLE, true, true, F_Cu, B_Cu );
    DRILL_LINE_ITEM viaItem( 100, 100, PAD_DRILL_SHAPE::CIRCLE, true, false, F_Cu, B_Cu );

    padItem.m_Qty = 1;
    viaItem.m_Qty = 1;

    // Test ascending comparator
    DRILL_LINE_ITEM::COMPARE cmpAsc( DRILL_LINE_ITEM::COL_VIA_PAD, true );

    // Strict weak ordering requirement 1: irreflexivity - compare(a, a) must be false
    BOOST_CHECK( !cmpAsc( padItem, padItem ) );
    BOOST_CHECK( !cmpAsc( viaItem, viaItem ) );

    // Strict weak ordering requirement 2: asymmetry - if compare(a, b) then !compare(b, a)
    bool aLessB = cmpAsc( viaItem, padItem );  // false < true
    bool bLessA = cmpAsc( padItem, viaItem );  // true < false

    BOOST_CHECK( aLessB != bLessA || (!aLessB && !bLessA) );

    // Test that sorting works without crashing
    std::vector<DRILL_LINE_ITEM> items;
    items.push_back( padItem );
    items.push_back( viaItem );
    items.push_back( padItem );
    items.push_back( viaItem );

    BOOST_CHECK_NO_THROW( std::sort( items.begin(), items.end(), cmpAsc ) );

    // Verify the sort order is correct: false (via) values should come before true (pad) values
    BOOST_CHECK( !items[0].isPad );
    BOOST_CHECK( !items[1].isPad );
    BOOST_CHECK( items[2].isPad );
    BOOST_CHECK( items[3].isPad );

    // Test descending order
    DRILL_LINE_ITEM::COMPARE cmpDesc( DRILL_LINE_ITEM::COL_VIA_PAD, false );

    BOOST_CHECK( !cmpDesc( padItem, padItem ) );
    BOOST_CHECK( !cmpDesc( viaItem, viaItem ) );

    BOOST_CHECK_NO_THROW( std::sort( items.begin(), items.end(), cmpDesc ) );

    // Verify descending order: true (pad) values should come before false (via) values
    BOOST_CHECK( items[0].isPad );
    BOOST_CHECK( items[1].isPad );
    BOOST_CHECK( !items[2].isPad );
    BOOST_CHECK( !items[3].isPad );
}


/**
 * Regression test for issue #23218. FormatBoardStatisticsReport crashed on MSVC
 * due to an incomplete printf format specifier ("%.2f %" instead of "%.2f %%")
 * in the component density lines.
 */
BOOST_AUTO_TEST_CASE( FormatReportWithDensity )
{
    BOARD_STATISTICS_DATA data;
    InitializeBoardStatisticsData( data );

    data.hasOutline = true;
    data.boardWidth = 100000000;
    data.boardHeight = 50000000;
    data.boardArea = 5e15;
    data.frontFootprintDensity = 42.57;
    data.backFootprintDensity = 18.93;

    UNITS_PROVIDER unitsProvider( pcbIUScale, EDA_UNITS::MM );

    wxString report;
    BOOST_CHECK_NO_THROW(
            report = FormatBoardStatisticsReport( data, nullptr, unitsProvider,
                                                  wxT( "TestProject" ), wxT( "TestBoard" ) ) );

    BOOST_CHECK( report.Contains( wxT( "42.57 %" ) ) );
    BOOST_CHECK( report.Contains( wxT( "18.93 %" ) ) );
}


/**
 * Verify FormatBoardStatisticsJson doesn't crash with density data.
 */
BOOST_AUTO_TEST_CASE( FormatJsonWithDensity )
{
    BOARD_STATISTICS_DATA data;
    InitializeBoardStatisticsData( data );

    data.hasOutline = true;
    data.boardWidth = 100000000;
    data.boardHeight = 50000000;
    data.boardArea = 5e15;
    data.frontFootprintDensity = 42.57;
    data.backFootprintDensity = 18.93;

    UNITS_PROVIDER unitsProvider( pcbIUScale, EDA_UNITS::MM );

    wxString json;
    BOOST_CHECK_NO_THROW(
            json = FormatBoardStatisticsJson( data, nullptr, unitsProvider,
                                              wxT( "TestProject" ), wxT( "TestBoard" ) ) );

    BOOST_CHECK( json.Contains( wxT( "42.57" ) ) );
    BOOST_CHECK( json.Contains( wxT( "18.93" ) ) );
}


BOOST_AUTO_TEST_SUITE_END()
