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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <boost/test/unit_test.hpp>

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <schematic_utils/schematic_file_util.h>

#include <connection_graph.h>
#include <schematic.h>
#include <sch_netchain.h>
#include <sch_sheet.h>
#include <settings/settings_manager.h>
#include <locale_io.h>


struct NETCHAIN_RENAME_FIXTURE
{
    NETCHAIN_RENAME_FIXTURE() : m_settingsManager() {}

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
};


// Promote a potential, then rename + delete it via the Phase 8.1 APIs.
// Validates that:
//  * RenameCommittedNetChain rekeys the override maps and rejects collisions.
//  * DeleteCommittedNetChain removes from m_committedNetChains and clears overrides.
BOOST_FIXTURE_TEST_CASE( NetChain_RemoveRenameRoundTrip, NETCHAIN_RENAME_FIXTURE )
{
    LOCALE_IO dummy;
    KI_TEST::LoadSchematic( m_settingsManager, wxString( "net_chains_four_nets" ), m_schematic );

    CONNECTION_GRAPH* graph = m_schematic->ConnectionGraph();
    BOOST_REQUIRE( graph );

    graph->Recalculate( m_schematic->BuildSheetListSortedByPageNumbers(), true );

    const auto& potentials = graph->GetPotentialNetChains();
    BOOST_REQUIRE( !potentials.empty() );

    // Seed netclass + color override maps so RenameCommittedNetChain has
    // something to rekey on the simple paths.
    std::map<wxString, wxString>       classes;
    std::map<wxString, KIGFX::COLOR4D> colors;
    classes[wxT( "FIRST" )] = wxT( "DDR_DATA" );
    colors[wxT( "FIRST" )]  = KIGFX::COLOR4D( 0.1, 0.2, 0.3, 1.0 );
    graph->SetNetChainNetClassOverrides( classes );
    graph->SetNetChainColorOverrides( colors );

    SCH_NETCHAIN* committed =
            graph->CreateNetChainFromPotential( potentials.front().get(), wxT( "FIRST" ) );

    BOOST_REQUIRE( committed );
    BOOST_CHECK_EQUAL( committed->GetName(), wxT( "FIRST" ) );
    BOOST_CHECK_EQUAL( committed->GetNetClass(), wxT( "DDR_DATA" ) );

    // Seed the terminal-ref / terminal-pin override maps from the live committed
    // chain so the values match whatever pins the fixture's potential picked.
    // This exercises the same restore path RebuildNetChains() takes after a
    // file reload, and stays valid if the fixture is edited.
    const wxString liveRefA  = committed->GetTerminalRef( 0 );
    const wxString livePinA  = committed->GetTerminalPinNum( 0 );
    const wxString liveRefB  = committed->GetTerminalRef( 1 );
    const wxString livePinB  = committed->GetTerminalPinNum( 1 );
    const KIID     livePinUA = committed->GetTerminalPinA();
    const KIID     livePinUB = committed->GetTerminalPinB();

    BOOST_REQUIRE( !liveRefA.IsEmpty() );
    BOOST_REQUIRE( !liveRefB.IsEmpty() );

    std::map<wxString, CONNECTION_GRAPH::CHAIN_TERMINAL_REFS> termRefs;
    termRefs[wxT( "FIRST" )] = { { liveRefA, livePinA }, { liveRefB, livePinB } };
    graph->SetNetChainTerminalRefOverrides( termRefs );

    std::map<wxString, std::pair<KIID, KIID>> termPins;
    termPins[wxT( "FIRST" )] = std::make_pair( livePinUA, livePinUB );
    graph->SetNetChainTerminalOverrides( termPins );

    // Promote a second one so we have something to collide with.
    if( potentials.size() > 1 )
    {
        SCH_NETCHAIN* second = graph->CreateNetChainFromPotential( potentials[1].get(),
                                                                   wxT( "SECOND" ) );
        BOOST_REQUIRE( second );

        // Renaming FIRST -> SECOND should fail (collision).
        BOOST_CHECK( !graph->RenameCommittedNetChain( wxT( "FIRST" ), wxT( "SECOND" ) ) );
        BOOST_CHECK_EQUAL( committed->GetName(), wxT( "FIRST" ) );
    }

    // Empty / unchanged renames should fail.
    BOOST_CHECK( !graph->RenameCommittedNetChain( wxT( "FIRST" ), wxEmptyString ) );
    BOOST_CHECK( !graph->RenameCommittedNetChain( wxEmptyString, wxT( "X" ) ) );
    BOOST_CHECK( !graph->RenameCommittedNetChain( wxT( "FIRST" ), wxT( "FIRST" ) ) );

    // Successful rename.
    BOOST_CHECK( graph->RenameCommittedNetChain( wxT( "FIRST" ), wxT( "RENAMED" ) ) );
    BOOST_CHECK_EQUAL( committed->GetName(), wxT( "RENAMED" ) );

    // The override maps must follow the rename: old key gone, new key carries
    // the previous values.
    const auto& nccOverrides = graph->GetNetChainNetClassOverrides();
    BOOST_CHECK( nccOverrides.find( wxT( "FIRST" ) ) == nccOverrides.end() );
    BOOST_CHECK( nccOverrides.find( wxT( "RENAMED" ) ) != nccOverrides.end() );
    BOOST_CHECK_EQUAL( nccOverrides.at( wxT( "RENAMED" ) ), wxT( "DDR_DATA" ) );

    const auto& colOverrides = graph->GetNetChainColorOverrides();
    BOOST_CHECK( colOverrides.find( wxT( "FIRST" ) ) == colOverrides.end() );
    BOOST_CHECK( colOverrides.find( wxT( "RENAMED" ) ) != colOverrides.end() );

    // Terminal-ref overrides must follow the rename so RebuildNetChains() restores
    // the chain under the new name on the next graph rebuild.
    const auto& refOverrides = graph->GetNetChainTerminalRefOverrides();
    BOOST_CHECK( refOverrides.find( wxT( "FIRST" ) ) == refOverrides.end() );
    BOOST_REQUIRE( refOverrides.find( wxT( "RENAMED" ) ) != refOverrides.end() );
    BOOST_CHECK_EQUAL( refOverrides.at( wxT( "RENAMED" ) ).first.ref, liveRefA );
    BOOST_CHECK_EQUAL( refOverrides.at( wxT( "RENAMED" ) ).first.pin, livePinA );
    BOOST_CHECK_EQUAL( refOverrides.at( wxT( "RENAMED" ) ).second.ref, liveRefB );
    BOOST_CHECK_EQUAL( refOverrides.at( wxT( "RENAMED" ) ).second.pin, livePinB );

    const auto& pinOverrides = graph->GetNetChainTerminalOverrides();
    BOOST_CHECK( pinOverrides.find( wxT( "FIRST" ) ) == pinOverrides.end() );
    BOOST_REQUIRE( pinOverrides.find( wxT( "RENAMED" ) ) != pinOverrides.end() );
    BOOST_CHECK( pinOverrides.at( wxT( "RENAMED" ) ).first == livePinUA );
    BOOST_CHECK( pinOverrides.at( wxT( "RENAMED" ) ).second == livePinUB );

    // After Recalculate the chain must still appear only under the new name; the
    // committed restore path keys on m_netChainTerminalRefOverrides, so a stale
    // "FIRST" entry would resurrect a duplicate chain.
    graph->Recalculate( m_schematic->BuildSheetListSortedByPageNumbers(), true );

    {
        const auto& chains = graph->GetCommittedNetChains();
        bool        sawFirst   = false;
        bool        sawRenamed = false;

        for( const std::unique_ptr<SCH_NETCHAIN>& s : chains )
        {
            if( !s )
                continue;

            if( s->GetName() == wxT( "FIRST" ) )
                sawFirst = true;

            if( s->GetName() == wxT( "RENAMED" ) )
                sawRenamed = true;
        }

        BOOST_CHECK( !sawFirst );
        BOOST_CHECK( sawRenamed );
    }

    // Rejecting deletion of an unknown chain.
    BOOST_CHECK( !graph->DeleteCommittedNetChain( wxT( "DOES_NOT_EXIST" ) ) );
    BOOST_CHECK( !graph->DeleteCommittedNetChain( wxEmptyString ) );

    // Successful deletion drops the chain and clears its overrides.
    BOOST_CHECK( graph->DeleteCommittedNetChain( wxT( "RENAMED" ) ) );

    {
        const auto& netChains = graph->GetCommittedNetChains();
        bool        found   = false;

        for( const std::unique_ptr<SCH_NETCHAIN>& s : netChains )
        {
            if( s && s->GetName() == wxT( "RENAMED" ) )
                found = true;
        }

        BOOST_CHECK( !found );
    }

    BOOST_CHECK( graph->GetNetChainNetClassOverrides().find( wxT( "RENAMED" ) )
                 == graph->GetNetChainNetClassOverrides().end() );
    BOOST_CHECK( graph->GetNetChainColorOverrides().find( wxT( "RENAMED" ) )
                 == graph->GetNetChainColorOverrides().end() );
    BOOST_CHECK( graph->GetNetChainTerminalRefOverrides().find( wxT( "RENAMED" ) )
                 == graph->GetNetChainTerminalRefOverrides().end() );
    BOOST_CHECK( graph->GetNetChainTerminalOverrides().find( wxT( "RENAMED" ) )
                 == graph->GetNetChainTerminalOverrides().end() );
}
