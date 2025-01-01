/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 * or you may search the http://www.gnu.org website for the version 32 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <junction_helpers.h>

#include <sch_bus_entry.h>
#include <sch_line.h>
#include <sch_sheet.h>
#include <sch_sheet_pin.h>

using namespace JUNCTION_HELPERS;

static constexpr int BE_SIZE = 25400;

struct JUNCTION_HELPER_FIXTURE
{
    EE_RTREE items;
};

static SCH_LINE* make_wire( const VECTOR2I& aStart, const VECTOR2I& aEnd )
{
    SCH_LINE* const line = new SCH_LINE{ aStart, LAYER_WIRE };
    line->SetEndPoint( aEnd );
    return line;
}

static SCH_LINE* make_bus( const VECTOR2I& aStart, const VECTOR2I& aEnd )
{
    SCH_LINE* const line = new SCH_LINE{ aStart, LAYER_BUS };
    line->SetEndPoint( aEnd );
    return line;
}

BOOST_FIXTURE_TEST_SUITE( JunctionHelpers, JUNCTION_HELPER_FIXTURE )

/**
 * Check that we can get the basic properties out as expected
 */
BOOST_AUTO_TEST_CASE( Empty )
{
    const POINT_INFO info = AnalyzePoint( items, { 0, 0 }, false );

    BOOST_CHECK( !info.isJunction );
}

BOOST_AUTO_TEST_CASE( SingleWireEnd )
{
    /*
     *            not a junction
     *            V
     *  -----------
     */
    items.insert( make_wire( { 0, 0 }, { 0, 100 } ) );

    const POINT_INFO info = AnalyzePoint( items, { 0, 0 }, false );

    BOOST_CHECK( !info.isJunction );
}

BOOST_AUTO_TEST_CASE( WireCorner )
{
    /*
     *  |
     *  |_____
     *  ^
     *  not a junction
     */
    items.insert( make_wire( { 0, 0 }, { 100, 0 } ) );
    items.insert( make_wire( { 0, 0 }, { 0, 100 } ) );

    const POINT_INFO info = AnalyzePoint( items, { 0, 0 }, false );

    BOOST_CHECK( !info.isJunction );
}

BOOST_AUTO_TEST_CASE( WireTee )
{
    /*
     *       |  /-- junction in the middle
     *       | /
     *  -----O------
     */
    items.insert( make_wire( { 0, 0 }, { 100, 0 } ) );
    items.insert( make_wire( { 0, 0 }, { -100, 0 } ) );
    items.insert( make_wire( { 0, 0 }, { 0, 100 } ) );

    const POINT_INFO info = AnalyzePoint( items, { 0, 0 }, false );

    BOOST_CHECK( info.isJunction );
    BOOST_CHECK( !info.hasBusEntry );
    BOOST_CHECK( !info.hasBusEntryToMultipleWires );
}

BOOST_AUTO_TEST_CASE( BusEntryOnBus )
{
    /*
    *    ||  <-- not a junction (it is a connection!)
     *   ||\
     *   || \
     *   ||  \ <-- also not a junction
     */

    items.insert( make_bus( { 0, 0 }, { 0, BE_SIZE } ) );

    SCH_BUS_WIRE_ENTRY* const busEntry = new SCH_BUS_WIRE_ENTRY( { 0, 0 }, false );

    // Make sure the BE is where we think it is
    BOOST_REQUIRE_EQUAL( busEntry->GetPosition(), VECTOR2I( 0, 0 ) );
    BOOST_REQUIRE_EQUAL( busEntry->GetEnd(), VECTOR2I( BE_SIZE, BE_SIZE ) );

    items.insert( busEntry );

    // BE-bus point
    const POINT_INFO info_start = AnalyzePoint( items, { 0, 0 }, false );
    BOOST_CHECK( !info_start.isJunction );
    BOOST_CHECK( info_start.hasBusEntry );

    // Dangling end of the bus entry
    const POINT_INFO info_end = AnalyzePoint( items, { BE_SIZE, BE_SIZE }, false );
    BOOST_CHECK( !info_end.isJunction );
    BOOST_CHECK( info_end.hasBusEntry );
    // There is no wire here
    BOOST_CHECK( !info_end.hasBusEntryToMultipleWires );
}

BOOST_AUTO_TEST_CASE( BusEntryToWire )
{
    /*
     *   \
     *    \ /--- <-- not a junction
     *     \V________________
     */
    SCH_BUS_WIRE_ENTRY* const busEntry = new SCH_BUS_WIRE_ENTRY( { 0, 0 }, false );
    items.insert( busEntry );
    items.insert( make_wire( { BE_SIZE, BE_SIZE }, { 2 * BE_SIZE, BE_SIZE } ) );

    const POINT_INFO info = AnalyzePoint( items, { BE_SIZE, BE_SIZE }, false );
    BOOST_CHECK( !info.isJunction );
    BOOST_CHECK( info.hasBusEntry );
    // This is a single wire to a bus entry, not multiple
    BOOST_CHECK( !info.hasBusEntryToMultipleWires );
}

BOOST_AUTO_TEST_CASE( WireDirectToBus )
{
    /*
     * || /--- <-- not a junction
     * ||______________
     */
    items.insert( make_bus( { 0, 0 }, { 0, BE_SIZE } ) );
    items.insert( make_wire( { 0, 0 }, { BE_SIZE, 0 } ) );

    const POINT_INFO info = AnalyzePoint( items, { 0, 0 }, false );
    BOOST_CHECK( !info.isJunction );
    BOOST_CHECK( !info.hasBusEntry );
    // It's a single wire, and not a bus entry
    BOOST_CHECK( !info.hasBusEntryToMultipleWires );
}

BOOST_AUTO_TEST_CASE( WireCrossingBus )
{
    /*          ||   __ not a junction
     *          || /
     * ______________________
     *          ||
     *          ||
     */
    items.insert( make_bus( { 0, 0 }, { 0, -BE_SIZE } ) );
    items.insert( make_bus( { 0, 0 }, { 0, BE_SIZE } ) );
    items.insert( make_wire( { 0, 0 }, { BE_SIZE, 0 } ) );
    items.insert( make_wire( { 0, 0 }, { -BE_SIZE, 0 } ) );

    const POINT_INFO info = AnalyzePoint( items, { 0, 0 }, false );
    // Two wires and two buses meet, which isn't a junction
    BOOST_CHECK( !info.isJunction );
    BOOST_CHECK( !info.hasBusEntry );
    BOOST_CHECK( !info.hasBusEntryToMultipleWires );
}

BOOST_AUTO_TEST_CASE( WireToBusEntryRoot )
{
    /*
     *   /--- <-- not a junction
     *  _____________
     * ||\
     * || \
     * ||  \
     */
    items.insert( make_bus( { 0, 0 }, { 0, BE_SIZE } ) );
    items.insert( make_wire( { 0, 0 }, { BE_SIZE, 0 } ) );
    items.insert( new SCH_BUS_WIRE_ENTRY( { 0, 0 }, false ) );

    const POINT_INFO info = AnalyzePoint( items, { 0, 0 }, false );
    BOOST_CHECK( !info.isJunction );
    BOOST_CHECK( info.hasBusEntry );
    // The bus/bus-entry point isn't valid
    BOOST_CHECK( !info.hasBusEntryToMultipleWires );
}

BOOST_AUTO_TEST_CASE( WireCrossingBusEntryRoot )
{
    /*          ||
     *          || /--- <-- it's a junction, but the client can choose not
     *          ||           to put a dot here
     * ______________________
     *          ||\
     *          || \
     *          ||  \
     */
    items.insert( make_bus( { 0, 0 }, { 0, -BE_SIZE } ) );
    items.insert( make_bus( { 0, 0 }, { 0, BE_SIZE } ) );
    items.insert( make_wire( { 0, 0 }, { BE_SIZE, 0 } ) );
    items.insert( make_wire( { 0, 0 }, { -BE_SIZE, 0 } ) );
    items.insert( new SCH_BUS_WIRE_ENTRY( { 0, 0 }, false ) );

    const POINT_INFO info = AnalyzePoint( items, { 0, 0 }, false );
    // Three buses meet here, so this is a junction
    BOOST_CHECK( info.isJunction );
    BOOST_CHECK( info.hasBusEntry );
    // There are multiple wires but they don't count here
    BOOST_CHECK( !info.hasBusEntryToMultipleWires );
}

BOOST_AUTO_TEST_CASE( WireCornerToBusEntry )
{
    /*
     * ||
     * ||
     * ||
     *   \   /--- junction here
     *    \ |
     *     O-------------
     *     |
     *     |
     */
    items.insert( make_bus( { 0, 0 }, { 0, BE_SIZE } ) );
    items.insert( make_wire( { BE_SIZE, BE_SIZE }, { 2 * BE_SIZE, BE_SIZE } ) );
    items.insert( make_wire( { BE_SIZE, BE_SIZE }, { BE_SIZE, 2 * BE_SIZE } ) );
    items.insert( new SCH_BUS_WIRE_ENTRY( { 0, 0 }, false ) );

    const POINT_INFO info = AnalyzePoint( items, { BE_SIZE, BE_SIZE }, false );
    BOOST_CHECK( info.isJunction );
    BOOST_CHECK( info.hasBusEntry );
    BOOST_CHECK( info.hasBusEntryToMultipleWires );
}

BOOST_AUTO_TEST_CASE( WireTeeToBusEntry )
{
    /*
    *  ||
     * ||
     * ||  |
     *   \ |  /--- junction here
     *    \| /
     *     O-------------
     *     |
     *     |
     */
    items.insert( make_bus( { 0, 0 }, { 0, BE_SIZE } ) );
    items.insert( make_wire( { BE_SIZE, BE_SIZE }, { 2 * BE_SIZE, BE_SIZE } ) ); // right
    items.insert( make_wire( { BE_SIZE, BE_SIZE }, { BE_SIZE, 0 } ) );           //up
    items.insert( make_wire( { BE_SIZE, BE_SIZE }, { BE_SIZE, 2 * BE_SIZE } ) ); // down
    items.insert( new SCH_BUS_WIRE_ENTRY( { 0, 0 }, false ) );

    const POINT_INFO info = AnalyzePoint( items, { BE_SIZE, BE_SIZE }, false );
    BOOST_CHECK( info.isJunction );
    BOOST_CHECK( info.hasBusEntry );
    BOOST_CHECK( info.hasBusEntryToMultipleWires );
}

BOOST_AUTO_TEST_CASE( SheetPinToOneWire )
{
    /*
    *  ---+ ___not a junction
    *     |/
    *  |=>|---------
    *     |
    *   --+
     */

    // The point of interest is at (0, 0)

    items.insert( make_wire( { 0, 0 }, { BE_SIZE, 0 } ) ); // right

    SCH_SHEET* const sheet = new SCH_SHEET( nullptr, { -10 * BE_SIZE, -10 * BE_SIZE },
                                            { 10 * BE_SIZE, 20 * BE_SIZE } );
    SCH_SHEET_PIN* const pin = new SCH_SHEET_PIN( sheet, { 0, 0 }, "Pin Name" );
    pin->SetSide( SHEET_SIDE::RIGHT );

    sheet->AddPin( pin );
    items.insert( sheet );

    // This test won't make sense if the pin isn't where we think it is!
    BOOST_REQUIRE( sheet->IsConnected( { 0, 0 } ) );

    const POINT_INFO info = AnalyzePoint( items, { 0, 0 }, false );
    BOOST_CHECK( !info.isJunction );
    BOOST_CHECK( !info.hasBusEntry );
}

BOOST_AUTO_TEST_CASE( SheetPinToTwoWires )
{
    /*
    * A sheet pin counts as a wire, so this is a junction
    *
    *  ---+ ___this is a junction
    *     |/
    *  |=>O---------
    *     |\
    *     | \
    *     |  \
    *   --+
    */
    // The point of interest is at (0, 0)

    items.insert( make_wire( { 0, 0 }, { BE_SIZE, 0 } ) );       // right
    items.insert( make_wire( { 0, 0 }, { BE_SIZE, BE_SIZE } ) ); // right and down

    SCH_SHEET* const sheet = new SCH_SHEET( nullptr, { -10 * BE_SIZE, -10 * BE_SIZE },
                                            { 10 * BE_SIZE, 20 * BE_SIZE } );
    SCH_SHEET_PIN* const pin = new SCH_SHEET_PIN( sheet, { 0, 0 }, "Pin Name" );
    pin->SetSide( SHEET_SIDE::RIGHT );

    sheet->AddPin( pin );
    items.insert( sheet );

    BOOST_REQUIRE( sheet->IsConnected( { 0, 0 } ) );

    const POINT_INFO info = AnalyzePoint( items, { 0, 0 }, false );
    BOOST_CHECK( info.isJunction );
    BOOST_CHECK( !info.hasBusEntry );
}

BOOST_AUTO_TEST_SUITE_END()