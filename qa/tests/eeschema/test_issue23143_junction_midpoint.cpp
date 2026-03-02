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

/**
 * Test for issue #23143: Pin appears connected but ERC reports it as unconnected.
 *
 * When a junction exists at the midpoint of a wire (without the wire being split),
 * a wire connecting to that junction should still be part of the same net. The
 * connection graph must recognize that a junction on a wire midpoint connects all
 * wires that meet at that point.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <connection_graph.h>
#include <sch_junction.h>
#include <sch_line.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_screen.h>
#include <schematic.h>
#include <settings/settings_manager.h>
#include <project.h>


BOOST_AUTO_TEST_CASE( JunctionAtWireMidpointConnectsNet )
{
    // Reproduce issue #23143: a junction placed at the midpoint of a horizontal wire
    // (without the wire being split) must connect a vertical wire that terminates at
    // the junction to the same net as the horizontal wire.
    //
    // Topology:
    //   horizontal wire: (-1000, 0) -- (1000, 0)   [not split at (0, 0)]
    //   junction at: (0, 0)
    //   vertical wire: (0, 0) -- (0, -1000)
    //
    // Expected: all three items in the same subgraph.

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );

    SCHEMATIC schematic( &manager.Prj() );
    schematic.Reset();
    SCH_SHEET* defaultSheet = schematic.GetTopLevelSheet( 0 );

    SCH_SCREEN* screen = new SCH_SCREEN( nullptr );
    SCH_SHEET*  sheet  = new SCH_SHEET( nullptr, VECTOR2I( 0, 0 ), VECTOR2I( 2000, 2000 ) );
    sheet->SetScreen( screen );
    schematic.AddTopLevelSheet( sheet );
    schematic.RemoveTopLevelSheet( defaultSheet );
    delete defaultSheet;

    SCH_SHEET_PATH sheetPath;
    sheetPath.push_back( sheet );

    // Horizontal wire spanning the junction point as a midpoint (not split)
    SCH_LINE* hWire = new SCH_LINE( VECTOR2I( -1000, 0 ), LAYER_WIRE );
    hWire->SetEndPoint( VECTOR2I( 1000, 0 ) );

    // Junction at the midpoint of the horizontal wire
    SCH_JUNCTION* junction = new SCH_JUNCTION( VECTOR2I( 0, 0 ) );

    // Vertical wire with its endpoint at the junction
    SCH_LINE* vWire = new SCH_LINE( VECTOR2I( 0, 0 ), LAYER_WIRE );
    vWire->SetEndPoint( VECTOR2I( 0, -1000 ) );

    screen->Append( hWire, false );
    screen->Append( junction, false );
    screen->Append( vWire, false );

    CONNECTION_GRAPH graph;
    graph.SetSchematic( &schematic );

    SCH_SHEET_LIST sheets = schematic.BuildSheetListSortedByPageNumbers();
    graph.Recalculate( sheets, true );

    CONNECTION_SUBGRAPH* sgH = graph.GetSubgraphForItem( hWire );
    CONNECTION_SUBGRAPH* sgJ = graph.GetSubgraphForItem( junction );
    CONNECTION_SUBGRAPH* sgV = graph.GetSubgraphForItem( vWire );

    BOOST_REQUIRE( sgH );
    BOOST_REQUIRE( sgJ );
    BOOST_REQUIRE( sgV );

    BOOST_CHECK_MESSAGE( sgH == sgV,
                         "Vertical wire should be in the same subgraph as the horizontal wire "
                         "when a junction exists at the wire midpoint (issue #23143)" );
    BOOST_CHECK_MESSAGE( sgJ == sgH,
                         "Junction should be in the same subgraph as the horizontal wire" );
}
