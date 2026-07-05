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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file test_bus_migration_precondition.cpp
 *
 * The migrate-buses prompt in SCH_EDIT_FRAME::OpenProjectFiles() is gated on
 * ConnectionGraph::GetBusesNeedingMigration().  That list is only populated by a
 * connection-graph recalculation; ConnectionGraph::Reset() clears it.  The prompt
 * must therefore be evaluated after RecalculateConnections(), not between Reset()
 * and the recalculation, or it can never fire for a file that genuinely needs it.
 *
 * The dialog itself is quasi-modal GUI and cannot run headless, so this test locks
 * down the structural precondition instead.  A bus subgraph carrying two differently
 * named vector bus labels is reported only while the graph is populated, never after
 * a Reset().
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <connection_graph.h>
#include <schematic.h>
#include <sch_label.h>
#include <sch_line.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <settings/settings_manager.h>


BOOST_AUTO_TEST_SUITE( BusMigrationPrecondition )


struct BUS_MIGRATION_FIXTURE
{
    BUS_MIGRATION_FIXTURE()
    {
        m_mgr.LoadProject( "" );
        m_schematic = std::make_unique<SCHEMATIC>( &m_mgr.Prj() );
        m_schematic->Reset();
        SCH_SHEET* defaultSheet = m_schematic->GetTopLevelSheet( 0 );

        m_screen = new SCH_SCREEN( m_schematic.get() );
        m_sheet = new SCH_SHEET( m_schematic.get() );
        m_sheet->SetScreen( m_screen );
        m_schematic->AddTopLevelSheet( m_sheet );
        m_schematic->RemoveTopLevelSheet( defaultSheet );
        delete defaultSheet;
    }

    SETTINGS_MANAGER           m_mgr;
    std::unique_ptr<SCHEMATIC> m_schematic;
    SCH_SCREEN*                m_screen;
    SCH_SHEET*                 m_sheet;
};


BOOST_FIXTURE_TEST_CASE( MigrationDataOnlyExistsAfterRecalculate, BUS_MIGRATION_FIXTURE )
{
    // Two differently named vector bus labels on one bus wire is exactly the pre-6.0
    // ambiguity the migrate-buses dialog resolves.
    SCH_LINE* busWire = new SCH_LINE( VECTOR2I( 0, 0 ), LAYER_BUS );
    busWire->SetEndPoint( VECTOR2I( 5000000, 0 ) );
    m_screen->Append( busWire );

    m_screen->Append( new SCH_LABEL( VECTOR2I( 0, 0 ), wxT( "DATA[0..3]" ) ) );
    m_screen->Append( new SCH_LABEL( VECTOR2I( 5000000, 0 ), wxT( "ADDR[0..3]" ) ) );

    SCH_SHEET_LIST    sheets = m_schematic->BuildSheetListSortedByPageNumbers();
    CONNECTION_GRAPH* graph = m_schematic->ConnectionGraph();

    graph->Recalculate( sheets, true );

    // The load path evaluates the prompt here, once connectivity has been built.
    BOOST_CHECK_MESSAGE( !graph->GetBusesNeedingMigration().empty(),
                         "Conflicting bus must be reported once connectivity is built" );

    // The load path calls Reset() before rebuilding connectivity.  With the subgraphs
    // cleared the migration list is empty regardless of schematic content, so a prompt
    // evaluated in that window can never fire.
    graph->Reset();

    BOOST_CHECK_MESSAGE( graph->GetBusesNeedingMigration().empty(),
                         "A freshly-Reset() graph has no migration data" );
}


BOOST_AUTO_TEST_SUITE_END()
