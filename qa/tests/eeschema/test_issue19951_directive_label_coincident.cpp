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
 * Test for issue #19951: netclass directive not applied when it shares a connection point
 * with a local label.
 *
 * The reporter's schematic has two identical net stubs.  On the first, the local label and
 * the netclass directive sit at different points on the wire and everything resolves.  On
 * the second they share one point in the middle of the wire, and the label, the directive
 * and the wire ended up in two separate subgraphs: the wire got an auto-generated net name
 * and the Default netclass.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <schematic_utils/schematic_file_util.h>

#include <connection_graph.h>
#include <netclass.h>
#include <project.h>
#include <project/net_settings.h>
#include <project/project_file.h>
#include <sch_connection.h>
#include <sch_label.h>
#include <sch_line.h>
#include <sch_screen.h>
#include <sch_sheet_path.h>
#include <schematic.h>
#include <settings/settings_manager.h>


struct ISSUE19951_FIXTURE
{
    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;

    SCH_LABEL* FindLabel( const wxString& aText ) const
    {
        for( SCH_ITEM* item : m_schematic->RootScreen()->Items().OfType( SCH_LABEL_T ) )
        {
            SCH_LABEL* label = static_cast<SCH_LABEL*>( item );

            if( label->GetText() == aText )
                return label;
        }

        return nullptr;
    }

    SCH_DIRECTIVE_LABEL* FindDirectiveAt( const VECTOR2I& aPos ) const
    {
        for( SCH_ITEM* item : m_schematic->RootScreen()->Items().OfType( SCH_DIRECTIVE_LABEL_T ) )
        {
            SCH_DIRECTIVE_LABEL* directive = static_cast<SCH_DIRECTIVE_LABEL*>( item );

            if( directive->GetPosition() == aPos )
                return directive;
        }

        return nullptr;
    }

    SCH_LINE* FindWireUnder( const VECTOR2I& aPos ) const
    {
        for( SCH_ITEM* item : m_schematic->RootScreen()->Items().OfType( SCH_LINE_T ) )
        {
            SCH_LINE* line = static_cast<SCH_LINE*>( item );

            if( line->IsWire() && line->HitTest( aPos, 0 ) )
                return line;
        }

        return nullptr;
    }
};


BOOST_FIXTURE_TEST_SUITE( Issue19951DirectiveLabelCoincident, ISSUE19951_FIXTURE )


// local_label01 and its directive sit at different points on the same wire, which always worked
// This case guards the fix against regressing the ordinary topology
BOOST_AUTO_TEST_CASE( DirectiveOnSeparatePointDrivesWire )
{
    KI_TEST::LoadSchematic( m_settingsManager, wxT( "issue19951/issue19951" ), m_schematic );

    SCH_LABEL* label = FindLabel( wxT( "local_label01" ) );
    BOOST_REQUIRE( label );

    SCH_LINE* wire = FindWireUnder( label->GetPosition() );
    BOOST_REQUIRE( wire );

    // The directive is deliberately not coincident with the label in this stub
    BOOST_REQUIRE( !FindDirectiveAt( label->GetPosition() ) );

    CONNECTION_GRAPH*    graph = m_schematic->ConnectionGraph();
    CONNECTION_SUBGRAPH* labelSubgraph = graph->GetSubgraphForItem( label );
    CONNECTION_SUBGRAPH* wireSubgraph = graph->GetSubgraphForItem( wire );

    BOOST_REQUIRE( labelSubgraph );
    BOOST_REQUIRE( wireSubgraph );
    BOOST_CHECK( labelSubgraph == wireSubgraph );

    const SCH_SHEET_PATH& sheet = wireSubgraph->GetSheet();

    BOOST_REQUIRE( wire->Connection( &sheet ) );
    BOOST_CHECK_EQUAL( wire->Connection( &sheet )->Name(), wxString( wxT( "/local_label01" ) ) );
}


// The reported failure, where local_label02 and its directive share one point in the middle of
// the wire and neither recorded a connection to it
BOOST_AUTO_TEST_CASE( CoincidentDirectiveAndLabelJoinWire )
{
    KI_TEST::LoadSchematic( m_settingsManager, wxT( "issue19951/issue19951" ), m_schematic );

    SCH_LABEL* label = FindLabel( wxT( "local_label02" ) );
    BOOST_REQUIRE( label );

    SCH_DIRECTIVE_LABEL* directive = FindDirectiveAt( label->GetPosition() );
    BOOST_REQUIRE( directive );

    SCH_LINE* wire = FindWireUnder( label->GetPosition() );
    BOOST_REQUIRE( wire );

    CONNECTION_GRAPH*    graph = m_schematic->ConnectionGraph();
    CONNECTION_SUBGRAPH* labelSubgraph = graph->GetSubgraphForItem( label );
    CONNECTION_SUBGRAPH* directiveSubgraph = graph->GetSubgraphForItem( directive );
    CONNECTION_SUBGRAPH* wireSubgraph = graph->GetSubgraphForItem( wire );

    BOOST_REQUIRE( labelSubgraph );
    BOOST_REQUIRE( wireSubgraph );

    BOOST_CHECK_MESSAGE( labelSubgraph == wireSubgraph,
                         "Label sharing a point with a netclass directive must still connect to "
                         "the wire under it (issue #19951)" );
    BOOST_CHECK_MESSAGE( directiveSubgraph == wireSubgraph,
                         "Netclass directive sharing a point with a label must still connect to "
                         "the wire under it (issue #19951)" );

    const SCH_SHEET_PATH& sheet = wireSubgraph->GetSheet();

    BOOST_REQUIRE( wire->Connection( &sheet ) );

    const wxString wireNet = wire->Connection( &sheet )->Name();

    BOOST_CHECK_EQUAL( wireNet, wxString( wxT( "/local_label02" ) ) );

    // The reported symptom was Default, because the wire's net was the auto-generated one the
    // directive knows nothing about
    std::shared_ptr<NET_SETTINGS>& netSettings =
            m_schematic->Project().GetProjectFile().m_NetSettings;

    BOOST_REQUIRE( netSettings );

    std::shared_ptr<NETCLASS> netclass = netSettings->GetEffectiveNetClass( wireNet );

    BOOST_REQUIRE( netclass );
    BOOST_CHECK_EQUAL( netclass->GetName(), wxString( wxT( "netclass_2" ) ) );
}


BOOST_AUTO_TEST_SUITE_END()
