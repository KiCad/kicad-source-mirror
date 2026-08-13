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
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * Test for issue #24134: buses can't branch adjacent to wire-to-bus entries.
 *
 * When a wire-to-bus entry sits on a bus, a third bus segment forking off at the entry root
 * must be recognized as a real bus junction.  Previously the bus entry masked the fork: the
 * junction analysis refused the junction (so the editor would not place one) and, with no
 * junction at the fork, the connection graph left the forked bus on its own NO_NET subgraph,
 * producing spurious "graphically connected to a bus NO_NET" ERC warnings.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <junction_helpers.h>

#include <connection_graph.h>
#include <sch_bus_entry.h>
#include <sch_junction.h>
#include <sch_line.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_screen.h>
#include <schematic.h>
#include <settings/settings_manager.h>
#include <project.h>

using namespace JUNCTION_HELPERS;

static constexpr int BE_SIZE = 25400;

static SCH_LINE* make_bus( const VECTOR2I& aStart, const VECTOR2I& aEnd )
{
    SCH_LINE* const line = new SCH_LINE{ aStart, LAYER_BUS };
    line->SetEndPoint( aEnd );
    return line;
}


/**
 * Junction analysis: a bus fork at a wire-to-bus entry root is a junction that the editor
 * is allowed (and required) to dot.
 */
BOOST_AUTO_TEST_CASE( BusForkAtBusEntryIsJunction )
{
    /*
     *          ||  /--- forked bus
     *          || /
     * ===========O ====== through bus (forks here)
     *          ||\
     *          || \  bus entry
     */
    EE_RTREE items;

    // Through bus passing across the fork point (0, 0) as a midpoint
    items.insert( make_bus( { -BE_SIZE, 0 }, { 0, 0 } ) );
    items.insert( make_bus( { 0, 0 }, { BE_SIZE, 0 } ) );

    // A third bus forking upward from the same point
    items.insert( make_bus( { 0, 0 }, { 0, -BE_SIZE } ) );

    // Wire-to-bus entry rooted at the same point
    items.insert( new SCH_BUS_WIRE_ENTRY( { 0, 0 }, false ) );

    const POINT_INFO info = AnalyzePoint( items, { 0, 0 }, false );

    BOOST_CHECK( info.isJunction );
    BOOST_CHECK( info.hasBusEntry );

    // The genuine three-way bus fork must be recognized despite the entry footprint.
    BOOST_CHECK_MESSAGE( info.hasBusEntryToMultipleBuses,
                         "A bus forking at a bus-entry root must be a real bus junction "
                         "(issue #24134)" );
}


/**
 * Regression guard: a straight bus that merely carries a bus entry (no fork) must NOT be
 * flagged as a multi-bus junction.
 */
BOOST_AUTO_TEST_CASE( StraightBusWithEntryIsNotMultiBusFork )
{
    EE_RTREE items;

    // Straight bus through the point, plus a single wire and a bus entry (no third bus)
    items.insert( make_bus( { -BE_SIZE, 0 }, { 0, 0 } ) );
    items.insert( make_bus( { 0, 0 }, { BE_SIZE, 0 } ) );
    items.insert( new SCH_BUS_WIRE_ENTRY( { 0, 0 }, false ) );

    const POINT_INFO info = AnalyzePoint( items, { 0, 0 }, false );

    BOOST_CHECK( info.hasBusEntry );
    BOOST_CHECK( !info.hasBusEntryToMultipleBuses );
}


/**
 * Regression guard: two buses merely crossing at a bus-entry root are not a fork and must not
 * be auto-joined into a junction (they were unconnected crossings before this change).
 */
BOOST_AUTO_TEST_CASE( CrossingBusesAtEntryRootAreNotAFork )
{
    /*
     *          ||
     * =========++========  two buses pass straight through, crossing
     *          ||\
     *          || \  bus entry
     */
    EE_RTREE items;

    // Two buses passing straight through the point (each as a midpoint, neither terminating)
    items.insert( make_bus( { -BE_SIZE, 0 }, { BE_SIZE, 0 } ) );
    items.insert( make_bus( { 0, -BE_SIZE }, { 0, BE_SIZE } ) );

    items.insert( new SCH_BUS_WIRE_ENTRY( { 0, 0 }, false ) );

    const POINT_INFO info = AnalyzePoint( items, { 0, 0 }, false );

    BOOST_CHECK( info.hasBusEntry );

    // No bus terminates here, so this is a crossing, not a fork.
    BOOST_CHECK_MESSAGE( !info.hasBusEntryToMultipleBuses,
                         "Two buses crossing at a bus-entry root must not be treated as a fork "
                         "(issue #24134)" );
}


/**
 * End-to-end connectivity: with the fork junction in place, the forked bus must share the
 * through bus's subgraph instead of becoming its own NO_NET net.
 */
BOOST_AUTO_TEST_CASE( BusForkAtEntryConnectsToSameBus )
{
    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );

    SCHEMATIC schematic( &manager.Prj() );
    schematic.Reset();
    SCH_SHEET* defaultSheet = schematic.GetTopLevelSheet( 0 );

    SCH_SCREEN* screen = new SCH_SCREEN( nullptr );
    SCH_SHEET*  sheet  = new SCH_SHEET( nullptr, VECTOR2I( 0, 0 ), VECTOR2I( 200000, 200000 ) );
    sheet->SetScreen( screen );
    schematic.AddTopLevelSheet( sheet );
    schematic.RemoveTopLevelSheet( defaultSheet );
    delete defaultSheet;

    SCH_SHEET_PATH sheetPath;
    sheetPath.push_back( sheet );

    // Through bus, not split at the fork point (0, 0)
    SCH_LINE* throughBus = make_bus( { -BE_SIZE, 0 }, { BE_SIZE, 0 } );

    // Forked bus branching from the fork point
    SCH_LINE* forkBus = make_bus( { 0, 0 }, { 0, -BE_SIZE } );

    // Wire-to-bus entry sitting on the bus at the fork point
    SCH_BUS_WIRE_ENTRY* busEntry = new SCH_BUS_WIRE_ENTRY( { 0, 0 }, false );

    screen->Append( throughBus, false );
    screen->Append( forkBus, false );
    screen->Append( busEntry, false );

    // The editor would auto-place this bus junction; it must be permitted at the fork.
    BOOST_REQUIRE_MESSAGE( screen->IsExplicitJunctionNeeded( VECTOR2I( 0, 0 ) ),
                           "A bus junction must be allowed adjacent to the bus entry "
                           "(issue #24134)" );

    SCH_JUNCTION* junction = new SCH_JUNCTION( VECTOR2I( 0, 0 ) );
    screen->Append( junction, false );

    CONNECTION_GRAPH graph;
    graph.SetSchematic( &schematic );

    SCH_SHEET_LIST sheets = schematic.BuildSheetListSortedByPageNumbers();
    graph.Recalculate( sheets, true );

    CONNECTION_SUBGRAPH* sgThrough = graph.GetSubgraphForItem( throughBus );
    CONNECTION_SUBGRAPH* sgFork    = graph.GetSubgraphForItem( forkBus );

    BOOST_REQUIRE( sgThrough );
    BOOST_REQUIRE( sgFork );

    BOOST_CHECK_MESSAGE( sgThrough == sgFork,
                         "Forked bus must belong to the same bus subgraph as the through bus "
                         "rather than a NO_NET net (issue #24134)" );
}
