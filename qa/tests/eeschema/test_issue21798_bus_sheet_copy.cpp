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
 * @file test_issue21798_bus_sheet_copy.cpp
 *
 * Test for issue #21798: Copying or duplicating a sheet with bus connector kills labels.
 *
 * When two hierarchical sheets with the same bus pin name exist on the same parent sheet
 * (as happens after copy/duplicate), both sheets' bus connections must produce valid
 * bus_neighbor links so that hierarchy propagation works through both buses.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <schematic_utils/schematic_file_util.h>

#include <connection_graph.h>
#include <schematic.h>
#include <sch_sheet.h>
#include <sch_sheet_pin.h>
#include <sch_screen.h>
#include <sch_label.h>
#include <sch_bus_entry.h>
#include <sch_line.h>
#include <sch_symbol.h>
#include <sch_pin.h>
#include <settings/settings_manager.h>
#include <locale_io.h>


struct ISSUE21798_FIXTURE
{
    ISSUE21798_FIXTURE() {}

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
};


/**
 * Test that two sheets with the same bus pin name both have working bus connectivity.
 *
 * The test schematic has Sheet1 and Sheet2, each with bus pin D[0..1]. Both buses
 * connect through bus entries to labels D0/D1, which connect to connectors J1/J2.
 *
 * Without the fix, processSubGraphs() renames one bus from D[0..1] to D[0..1]_1 and
 * SetSuffix() also renames bus members from D0 to D0_1. The matching code then can't
 * find label subgraphs for D0_1, so the renamed bus gets no bus_neighbor links and
 * hierarchy propagation breaks for that sheet.
 */
BOOST_FIXTURE_TEST_CASE( Issue21798DuplicatedSheetBusConnectivity, ISSUE21798_FIXTURE )
{
    LOCALE_IO dummy;

    KI_TEST::LoadSchematic( m_settingsManager, "issue21798/issue21798", m_schematic );

    SCH_SHEET_LIST    sheets = m_schematic->BuildSheetListSortedByPageNumbers();
    CONNECTION_GRAPH* graph  = m_schematic->ConnectionGraph();

    BOOST_REQUIRE( graph != nullptr );
    BOOST_REQUIRE( sheets.size() >= 1 );

    SCH_SHEET_PATH rootPath = sheets[0];

    // Count label subgraphs on the root sheet that have bus parents.
    // Each label (D0/D1) should be in a subgraph with at least one bus parent,
    // indicating successful bus member matching.
    int labelSubgraphsWithBusParents = 0;
    int labelSubgraphsTotal = 0;

    // Count distinct bus parent subgraphs. With two sheets, both bus subgraphs
    // should be parents of the label subgraphs.
    std::set<const CONNECTION_SUBGRAPH*> allBusParents;

    for( const auto& [key, subgraphs] : graph->GetNetMap() )
    {
        for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            if( subgraph->GetSheet() != rootPath )
                continue;

            const SCH_CONNECTION* conn = subgraph->GetDriverConnection();

            if( !conn || conn->IsBus() )
                continue;

            bool hasLabel = false;
            wxString labelText;

            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                if( item->Type() == SCH_LABEL_T )
                {
                    SCH_LABEL* label = static_cast<SCH_LABEL*>( item );

                    if( label->GetText() == wxT( "D0" ) || label->GetText() == wxT( "D1" ) )
                    {
                        hasLabel = true;
                        labelText = label->GetText();
                    }
                }
            }

            if( !hasLabel )
                continue;

            labelSubgraphsTotal++;

            const auto& busParents = subgraph->GetBusParents();

            if( !busParents.empty() )
            {
                labelSubgraphsWithBusParents++;

                for( const auto& [busConn, parentSet] : busParents )
                {
                    for( const CONNECTION_SUBGRAPH* parent : parentSet )
                        allBusParents.insert( parent );
                }

                BOOST_TEST_MESSAGE( "Label subgraph '" << labelText
                                    << "' net='" << conn->Name()
                                    << "' has " << busParents.size()
                                    << " bus parent entries" );
            }
            else
            {
                BOOST_TEST_MESSAGE( "Label subgraph '" << labelText
                                    << "' net='" << conn->Name()
                                    << "' has NO bus parents (BUG)" );
            }
        }
    }

    BOOST_CHECK_MESSAGE( labelSubgraphsTotal > 0,
                         "Should find at least one label subgraph for D0/D1" );

    BOOST_CHECK_MESSAGE( labelSubgraphsWithBusParents == labelSubgraphsTotal,
                         "All " << labelSubgraphsTotal
                         << " label subgraphs should have bus parents, but only "
                         << labelSubgraphsWithBusParents << " do" );

    // With the fix, labels should be linked to BOTH bus subgraphs (Sheet1 and Sheet2).
    // Without the fix, labels only link to the unrenamed bus (1 parent instead of 2).
    BOOST_CHECK_MESSAGE( allBusParents.size() == 2,
                         "Labels should have bus parents from both sheets (2), found "
                         << allBusParents.size() );
}
