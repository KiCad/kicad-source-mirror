/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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


// Regression for [H-1]. CONNECTION_GRAPH::Reset() clears every committed chain's
// non-owning symbol pointer set to drop stale SCH_SYMBOL references before the rest
// of the graph is rebuilt.  RebuildNetChains() then iterates the persisted override
// maps and used to skip any name that was already in m_committedNetChains, leaving
// the chain with an empty m_symbols and stale derived state.  Downstream consumers
// (netlist export, the setup panel, the tuner cache) trusted those caches.
//
// The fix refreshes the committed chain in place during the rebuild restore pass
// rather than skipping it.  This test exercises the full Recalculate(unconditional)
// cycle and asserts the committed chain still has populated m_symbols and m_nets
// afterwards.
struct NETCHAIN_RECALC_REFRESH_FIXTURE
{
    NETCHAIN_RECALC_REFRESH_FIXTURE() : m_settingsManager() {}

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
};


BOOST_FIXTURE_TEST_CASE( NetChain_RefreshCommittedChainAcrossUnconditionalRecalc,
                         NETCHAIN_RECALC_REFRESH_FIXTURE )
{
    LOCALE_IO dummy;
    KI_TEST::LoadSchematic( m_settingsManager, wxString( "net_chains_four_nets" ), m_schematic );

    CONNECTION_GRAPH* graph = m_schematic->ConnectionGraph();
    BOOST_REQUIRE( graph );

    SCH_SHEET_LIST sheets = m_schematic->BuildSheetListSortedByPageNumbers();
    graph->Recalculate( sheets, /*aUnconditional=*/true );

    const auto& potentials = graph->GetPotentialNetChains();
    BOOST_REQUIRE( !potentials.empty() );

    SCH_NETCHAIN* potential = potentials.front().get();
    BOOST_REQUIRE( potential );

    const std::set<wxString> originalNets    = potential->GetNets();
    const std::size_t        originalNetCnt  = originalNets.size();
    const std::size_t        originalSymCnt  = potential->GetSymbols().size();

    BOOST_REQUIRE_GT( originalNetCnt, 0u );
    BOOST_REQUIRE_GT( originalSymCnt, 0u );

    SCH_NETCHAIN* committed = graph->CreateNetChainFromPotential( potential, wxT( "REFRESH_TEST" ) );
    BOOST_REQUIRE( committed );
    BOOST_REQUIRE_EQUAL( committed->GetNets().size(), originalNetCnt );
    BOOST_REQUIRE_EQUAL( committed->GetSymbols().size(), originalSymCnt );

    // The hazard.  Recalculate(true) -> Reset() clears m_symbols on every committed chain,
    // and the rebuild restore pass used to skip names already present in
    // m_committedNetChains, leaving the chain permanently empty.
    graph->Recalculate( sheets, /*aUnconditional=*/true );

    SCH_NETCHAIN* refreshed = graph->GetNetChainByName( wxT( "REFRESH_TEST" ) );
    BOOST_REQUIRE_MESSAGE( refreshed,
                           "Committed chain disappeared across unconditional Recalculate" );

    BOOST_CHECK_MESSAGE( !refreshed->GetSymbols().empty(),
                         "Committed chain has empty m_symbols after unconditional Recalculate; "
                         "Reset() cleared the cache and RebuildNetChains() failed to refresh it" );

    BOOST_CHECK_MESSAGE( !refreshed->GetNets().empty(),
                         "Committed chain has empty m_nets after unconditional Recalculate" );

    BOOST_CHECK_EQUAL( refreshed->GetNets().size(), originalNetCnt );
    BOOST_CHECK_EQUAL( refreshed->GetSymbols().size(), originalSymCnt );

    // Terminal pin/ref data must also survive the round trip; the setup panel and the PCB
    // tuner walk these to find the bookend pads.
    BOOST_CHECK( !refreshed->GetTerminalRef( 0 ).IsEmpty() );
    BOOST_CHECK( !refreshed->GetTerminalRef( 1 ).IsEmpty() );

    // A second round trip must remain stable (no slow leak of derived state).
    graph->Recalculate( sheets, /*aUnconditional=*/true );

    SCH_NETCHAIN* twice = graph->GetNetChainByName( wxT( "REFRESH_TEST" ) );
    BOOST_REQUIRE( twice );
    BOOST_CHECK( !twice->GetSymbols().empty() );
    BOOST_CHECK( !twice->GetNets().empty() );
    BOOST_CHECK_EQUAL( twice->GetNets().size(), originalNetCnt );
    BOOST_CHECK_EQUAL( twice->GetSymbols().size(), originalSymCnt );
}


// Companion check.  User-set netclass and color overrides live on the SCH_NETCHAIN itself
// (not in the override map) once the chain is committed.  The in-place refresh must NOT
// reset them.
BOOST_FIXTURE_TEST_CASE( NetChain_RefreshPreservesOverridesOnCommittedChain,
                         NETCHAIN_RECALC_REFRESH_FIXTURE )
{
    LOCALE_IO dummy;
    KI_TEST::LoadSchematic( m_settingsManager, wxString( "net_chains_four_nets" ), m_schematic );

    CONNECTION_GRAPH* graph = m_schematic->ConnectionGraph();
    BOOST_REQUIRE( graph );

    SCH_SHEET_LIST sheets = m_schematic->BuildSheetListSortedByPageNumbers();
    graph->Recalculate( sheets, /*aUnconditional=*/true );

    const auto& potentials = graph->GetPotentialNetChains();
    BOOST_REQUIRE( !potentials.empty() );

    SCH_NETCHAIN* committed = graph->CreateNetChainFromPotential( potentials.front().get(),
                                                                  wxT( "OVERRIDE_TEST" ) );
    BOOST_REQUIRE( committed );

    committed->SetNetClass( wxT( "DDR_DATA" ) );
    committed->SetColor( KIGFX::COLOR4D( 1.0, 0.5, 0.25, 1.0 ) );

    graph->Recalculate( sheets, /*aUnconditional=*/true );

    SCH_NETCHAIN* refreshed = graph->GetNetChainByName( wxT( "OVERRIDE_TEST" ) );
    BOOST_REQUIRE( refreshed );

    BOOST_CHECK_EQUAL( refreshed->GetNetClass(), wxT( "DDR_DATA" ) );
    BOOST_CHECK( refreshed->GetColor() != KIGFX::COLOR4D::UNSPECIFIED );
    BOOST_CHECK_CLOSE( refreshed->GetColor().r, 1.0, 1e-6 );
    BOOST_CHECK_CLOSE( refreshed->GetColor().g, 0.5, 1e-6 );
    BOOST_CHECK_CLOSE( refreshed->GetColor().b, 0.25, 1e-6 );
}


// Regression for [H-2].  ReplaceNetChainTerminalPin writes into m_netChainTerminalOverrides,
// and the legacy pass-4 restore loop reapplies that override to potential chains by name.
// The new in-place refresh helpers on the committed-chain path used to copy the source
// chain's terminal pins unconditionally, silently undoing the user's "Replace terminal pin"
// action across an unconditional Recalculate.  The fix consults the override map first.
BOOST_FIXTURE_TEST_CASE( NetChain_RefreshPreservesTerminalPinOverride,
                         NETCHAIN_RECALC_REFRESH_FIXTURE )
{
    LOCALE_IO dummy;
    KI_TEST::LoadSchematic( m_settingsManager, wxString( "net_chains_four_nets" ), m_schematic );

    CONNECTION_GRAPH* graph = m_schematic->ConnectionGraph();
    BOOST_REQUIRE( graph );

    SCH_SHEET_LIST sheets = m_schematic->BuildSheetListSortedByPageNumbers();
    graph->Recalculate( sheets, /*aUnconditional=*/true );

    const auto& potentials = graph->GetPotentialNetChains();
    BOOST_REQUIRE( !potentials.empty() );

    SCH_NETCHAIN* potential = potentials.front().get();
    BOOST_REQUIRE( potential );

    SCH_NETCHAIN* committed = graph->CreateNetChainFromPotential( potential, wxT( "TERM_OVERRIDE" ) );
    BOOST_REQUIRE( committed );

    const KIID originalPinA = committed->GetTerminalPinA();
    const KIID originalPinB = committed->GetTerminalPinB();
    BOOST_REQUIRE( originalPinA != niluuid );
    BOOST_REQUIRE( originalPinB != niluuid );

    // Synthesize a fresh KIID and retarget the A endpoint as if the user had selected
    // "Replace terminal pin" on a different schematic pin.  We don't need the new id to
    // resolve to a real pin in the schematic; the override map only stores the raw KIIDs.
    const KIID newPinA;
    BOOST_REQUIRE( newPinA != originalPinA );

    graph->ReplaceNetChainTerminalPin( wxT( "TERM_OVERRIDE" ), originalPinA, newPinA );

    BOOST_REQUIRE_EQUAL( committed->GetTerminalPinA().AsString(), newPinA.AsString() );
    BOOST_REQUIRE_EQUAL( committed->GetTerminalPinB().AsString(), originalPinB.AsString() );

    graph->Recalculate( sheets, /*aUnconditional=*/true );

    SCH_NETCHAIN* refreshed = graph->GetNetChainByName( wxT( "TERM_OVERRIDE" ) );
    BOOST_REQUIRE( refreshed );

    BOOST_CHECK_MESSAGE( refreshed->GetTerminalPinA() == newPinA,
                         "Refresh helper overwrote the user's terminal-pin override on pin A" );
    BOOST_CHECK_MESSAGE( refreshed->GetTerminalPinB() == originalPinB,
                         "Refresh helper changed the unaffected terminal pin B" );

    // Stability across a second round trip.
    graph->Recalculate( sheets, /*aUnconditional=*/true );

    SCH_NETCHAIN* twice = graph->GetNetChainByName( wxT( "TERM_OVERRIDE" ) );
    BOOST_REQUIRE( twice );
    BOOST_CHECK( twice->GetTerminalPinA() == newPinA );
    BOOST_CHECK( twice->GetTerminalPinB() == originalPinB );
}
