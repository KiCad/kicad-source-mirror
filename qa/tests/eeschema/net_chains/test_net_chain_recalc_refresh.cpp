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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <boost/test/unit_test.hpp>

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <schematic_utils/schematic_file_util.h>

#include <connection_graph.h>
#include <advanced_config.h>
#include <schematic.h>
#include <sch_netchain.h>
#include <sch_sheet.h>
#include <sch_screen.h>
#include <sch_pin.h>
#include <sch_symbol.h>
#include <lib_symbol.h>
#include <fstream>
#include <wx/filename.h>
#include <settings/settings_manager.h>
#include <locale_io.h>
#include <scoped_set_reset.h>


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


BOOST_FIXTURE_TEST_CASE( NetChain_RefreshPreservesTerminalPinOverride, NETCHAIN_RECALC_REFRESH_FIXTURE )
{
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, enabled );

    for( bool backend : { false, true } )
    {
        BOOST_TEST_CONTEXT( "ConnectivityEngine=" << backend )
        {
            enabled = backend;
            SETTINGS_MANAGER settingsManager;
            std::unique_ptr<SCHEMATIC> schematic;
            LOCALE_IO locale;
            KI_TEST::LoadSchematic( settingsManager, "net_chains_four_nets", schematic );
            schematic->RebuildConnectivity();
            auto& chains = schematic->NetChains();
            const auto sheets = schematic->BuildSheetListSortedByPageNumbers();
            BOOST_REQUIRE_EQUAL( sheets.size(), 1u );
            const auto& path = sheets.front();
            BOOST_REQUIRE( !chains.GetPotentialNetChains().empty() );
            auto* chain = chains.CreateNetChainFromPotential( chains.GetPotentialNetChains().front().get(),
                                                             "TERM_OVERRIDE" );
            BOOST_REQUIRE( chain );
            const KIID originalA = chain->GetTerminalPinA();
            const KIID originalB = chain->GetTerminalPinB();
            auto* original = dynamic_cast<SCH_PIN*>( schematic->ResolveItem( originalA, nullptr, true ) );
            BOOST_REQUIRE( original );
            const auto connection = original->GetConnectionName( &path );
            BOOST_REQUIRE( connection );
            SCH_PIN* replacement = nullptr;

            for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
            {
                for( SCH_PIN* pin : static_cast<SCH_SYMBOL*>( item )->GetPins( &path ) )
                {
                    const auto net = pin->GetConnectionName( &path );

                    if( pin->m_Uuid != originalA && pin->m_Uuid != originalB
                        && net && *net == *connection )
                    {
                        replacement = pin;
                        break;
                    }
                }

                if( replacement )
                    break;
            }

            BOOST_REQUIRE( replacement );
            const KIID replacementId = replacement->m_Uuid;
            wxString replacementRef = replacement->GetParentSymbol()->GetRef( &path );
            const wxString replacementNumber = replacement->GetNumber();
            BOOST_CHECK( !chains.ReplaceNetChainTerminalPin( { "TERM_OVERRIDE", 1, originalA, path.Path() } ) );
            BOOST_REQUIRE( chains.ReplaceNetChainTerminalPin( { "TERM_OVERRIDE", 1, replacementId, path.Path() } ) );
            BOOST_CHECK( chain->GetTerminalPinA() == originalA );
            BOOST_CHECK( chain->GetTerminalPinB() == replacementId );
            BOOST_REQUIRE( chains.ReplaceNetChainTerminalPin( { "TERM_OVERRIDE", 1, originalB, path.Path() } ) );
            BOOST_REQUIRE( chains.ReplaceNetChainTerminalPin( { "TERM_OVERRIDE", 0, replacementId, path.Path() } ) );
            BOOST_CHECK_EQUAL( chain->GetTerminalRef( 0 ), replacementRef );
            BOOST_CHECK( chain->GetTerminalPath( 0 ) == path.Path() );
            BOOST_CHECK( !chains.ReplaceNetChainTerminalPin( { "TERM_OVERRIDE", 2, originalA, path.Path() } ) );

            for( int pass = 0; pass < 2; ++pass )
            {
                schematic->RebuildConnectivity();
                BOOST_CHECK( chains.GetNetChainByName( "TERM_OVERRIDE" ) == chain );
                BOOST_CHECK( chain->GetTerminalPinA() == replacementId );
                BOOST_CHECK( chain->GetTerminalPinB() == originalB );
                BOOST_CHECK( chain->GetTerminalPath( 0 ) == path.Path() );
                BOOST_CHECK_EQUAL( chain->GetTerminalRef( 0 ), replacementRef );
                BOOST_CHECK_EQUAL( chain->GetTerminalPinNum( 0 ), replacementNumber );
            }

            auto* replacementSymbol = static_cast<SCH_SYMBOL*>( replacement->GetParentSymbol() );
            auto library = replacementSymbol->GetLibSymbolRef()->Flatten();
            replacementSymbol->SetLibSymbol( library.release() );
            schematic->RebuildConnectivity();
            BOOST_CHECK( chain->GetTerminalPinA() == replacementId );
            BOOST_CHECK( chain->GetTerminalPath( 0 ) == path.Path() );
            BOOST_CHECK_EQUAL( chain->GetTerminalRef( 0 ), replacementRef );
            BOOST_CHECK_EQUAL( chain->GetNets().size(), 4u );

            replacementRef = "R900";
            static_cast<SCH_SYMBOL*>( replacement->GetParentSymbol() )->SetRef( &path, replacementRef );
            schematic->RebuildConnectivity();
            BOOST_CHECK( chains.GetNetChainByName( "TERM_OVERRIDE" ) == chain );
            BOOST_CHECK( chain->GetTerminalPinA() == replacementId );
            BOOST_CHECK_EQUAL( chain->GetTerminalRef( 0 ), replacementRef );
            BOOST_CHECK_EQUAL( chain->GetTerminalPinNum( 0 ), replacementNumber );

            replacementRef = "R901";
            static_cast<SCH_SYMBOL*>( replacement->GetParentSymbol() )->SetRef( &path, replacementRef );

            const wxString file = wxFileName::CreateTempFileName( "netchain-terminal-" );
            KI_TEST::DumpSchematicToFile( *schematic, *schematic->GetTopLevelSheet(), file.ToStdString() );
            std::ifstream stream( file.ToStdString() );
            BOOST_REQUIRE( stream.good() );
            auto reloaded = KI_TEST::ReadSchematicFromStream( stream, &schematic->Project() );
            BOOST_REQUIRE( reloaded );
            reloaded->RebuildConnectivity();
            auto* restored = reloaded->NetChains().GetNetChainByName( "TERM_OVERRIDE" );
            BOOST_REQUIRE( restored );
            BOOST_CHECK( restored->GetTerminalPinA() == replacementId );
            BOOST_CHECK( restored->GetTerminalPinB() == originalB );
            BOOST_CHECK_EQUAL( restored->GetTerminalRef( 0 ), replacementRef );
            BOOST_CHECK_EQUAL( restored->GetTerminalPinNum( 0 ), replacementNumber );
            stream.close();
            wxRemoveFile( file );

            auto* removedSymbol = static_cast<SCH_SYMBOL*>( replacement->GetParentSymbol() );
            path.LastScreen()->Remove( removedSymbol );
            std::unique_ptr<SCH_SYMBOL> removed( removedSymbol );
            static_cast<SCH_SYMBOL*>( original->GetParentSymbol() )->SetRef( &path, replacementRef );
            schematic->RebuildConnectivity();
            BOOST_CHECK( chain->GetTerminalPinA() == replacementId );
            BOOST_CHECK( chain->GetTerminalPath( 0 ) == path.Path() );
            BOOST_CHECK( !chain->GetSymbols().contains( removed.get() ) );
            BOOST_CHECK( chain->GetNets().empty() );
        }
    }
}
