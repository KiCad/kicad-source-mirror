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

#include <boost/test/unit_test.hpp>

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <schematic_utils/schematic_file_util.h>

#include <connection_graph.h>
#include <connectivity/conn_netchain_input.h>
#include <sch_netchain.h>
#include <sch_label.h>
#include <sch_screen.h>
#include <sch_symbol.h>
#include <schematic.h>
#include <settings/settings_manager.h>
#include <locale_io.h>

#include <algorithm>
#include <functional>
#include <stdexcept>


struct NETCHAIN_ROLLBACK_FIXTURE
{
    NETCHAIN_ROLLBACK_FIXTURE() : m_settingsManager() {}

    ~NETCHAIN_ROLLBACK_FIXTURE()
    {
        // Defensive cleanup so a failing test never leaks a hook into a sibling test.
        CONNECTION_GRAPH::RebuildNetChainsTestHook() = nullptr;
    }

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
};


// A failed candidate must not publish newly committed chains or signal readiness.
BOOST_FIXTURE_TEST_CASE( NetChain_RebuildFailureDiscardsCandidate,
                         NETCHAIN_ROLLBACK_FIXTURE )
{
    LOCALE_IO dummy;
    KI_TEST::LoadSchematic( m_settingsManager, wxString( "net_chains_four_nets" ),
                            m_schematic );

    CONNECTION_GRAPH* graph = m_schematic->ConnectionGraph();
    BOOST_REQUIRE( graph );

    SCH_SHEET_LIST sheets = m_schematic->BuildSheetListSortedByPageNumbers();

    graph->Recalculate( sheets, /*aUnconditional=*/true );
    BOOST_REQUIRE( graph->NetChainsBuilt() );

    const std::size_t baselineCount = graph->GetCommittedNetChains().size();
    std::map<SCH_SYMBOL*, wxString> symbolNames;

    for( const SCH_SHEET_PATH& path : sheets )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            auto* symbol = static_cast<SCH_SYMBOL*>( item );
            symbolNames.emplace( symbol, symbol->GetNetChainName() );
        }
    }

    BOOST_REQUIRE( std::any_of( symbolNames.begin(), symbolNames.end(),
                               []( const auto& entry ) { return !entry.second.IsEmpty(); } ) );

    bool hookFired = false;
    CONNECTION_GRAPH::RebuildNetChainsTestHook() =
            [&]( SCH_CONNECTIVITY::NETCHAIN_MANAGER& candidate )
            {
                hookFired = true;
                BOOST_CHECK_THROW( candidate.Rebuild( {} ), std::logic_error );

                BOOST_REQUIRE( !candidate.GetPotentialNetChains().empty() );
                SCH_NETCHAIN* stray = candidate.CreateNetChainFromPotential(
                        candidate.GetPotentialNetChains().front().get(), "ROLLBACK_PARTIAL" );
                BOOST_REQUIRE( stray );
                BOOST_REQUIRE_GT( candidate.GetCommittedNetChains().size(), 0u );

                throw std::runtime_error( "rollback test injected throw" );
            };

    BOOST_CHECK_THROW( graph->Recalculate( sheets, true ), std::runtime_error );

    CONNECTION_GRAPH::RebuildNetChainsTestHook() = nullptr;

    BOOST_CHECK( hookFired );
    BOOST_CHECK_EQUAL( graph->GetCommittedNetChains().size(), baselineCount );
    BOOST_CHECK( !graph->NetChainsBuilt() );
    BOOST_CHECK( graph->GetNetChainByName( "ROLLBACK_PARTIAL" ) == nullptr );
    BOOST_CHECK( !m_schematic->NetChains().GetNetChainMemberNetOverrides().contains( "ROLLBACK_PARTIAL" ) );
    BOOST_CHECK( !m_schematic->NetChains().GetNetChainTerminalRefOverrides().contains( "ROLLBACK_PARTIAL" ) );

    for( const auto& [symbol, name] : symbolNames )
        BOOST_CHECK_EQUAL( symbol->GetNetChainName(), name );
}


// Unconditional graph reset leaves derived state empty until a rebuild succeeds.
BOOST_FIXTURE_TEST_CASE( NetChain_RebuildFailureLeavesResetGraphUnbuilt,
                         NETCHAIN_ROLLBACK_FIXTURE )
{
    LOCALE_IO dummy;
    KI_TEST::LoadSchematic( m_settingsManager, wxString( "net_chains_four_nets" ),
                            m_schematic );

    CONNECTION_GRAPH* graph = m_schematic->ConnectionGraph();
    BOOST_REQUIRE( graph );

    SCH_SHEET_LIST sheets = m_schematic->BuildSheetListSortedByPageNumbers();

    BOOST_REQUIRE( graph->NetChainsBuilt() );
    const std::size_t baselineCount = graph->GetCommittedNetChains().size();

    bool hookFired = false;
    CONNECTION_GRAPH::RebuildNetChainsTestHook() =
            [&]( SCH_CONNECTIVITY::NETCHAIN_MANAGER& )
            {
                hookFired = true;
                throw std::runtime_error( "no-growth rollback test throw" );
            };

    BOOST_CHECK_THROW( graph->Recalculate( sheets, true ), std::runtime_error );

    CONNECTION_GRAPH::RebuildNetChainsTestHook() = nullptr;

    BOOST_CHECK( hookFired );

    BOOST_CHECK( !graph->NetChainsBuilt() );
    BOOST_CHECK_EQUAL( graph->GetCommittedNetChains().size(), baselineCount );
    BOOST_CHECK( graph->GetPotentialNetChains().empty() );
}


BOOST_FIXTURE_TEST_CASE( NetChain_RebuildPublishesExistingAndNewChains, NETCHAIN_ROLLBACK_FIXTURE )
{
    LOCALE_IO locale;
    KI_TEST::LoadSchematic( m_settingsManager, "net_chains_four_nets", m_schematic );
    auto& chains = m_schematic->NetChains();
    BOOST_REQUIRE( !chains.GetPotentialNetChains().empty() );
    auto* original = chains.CreateNetChainFromPotential( chains.GetPotentialNetChains().front().get(),
                                                       "ORIGINAL_CHAIN" );
    BOOST_REQUIRE( original );
    CONNECTION_GRAPH::RebuildNetChainsTestHook() = [&]( SCH_CONNECTIVITY::NETCHAIN_MANAGER& candidate )
    {
        BOOST_REQUIRE( !candidate.GetPotentialNetChains().empty() );
        BOOST_REQUIRE( candidate.CreateNetChainFromPotential( candidate.GetPotentialNetChains().front().get(),
                                                              "ADDED_CHAIN" ) );
    };
    m_schematic->ConnectionGraph()->Recalculate( m_schematic->Hierarchy(), true );
    CONNECTION_GRAPH::RebuildNetChainsTestHook() = nullptr;
    BOOST_CHECK( chains.GetNetChainByName( "ORIGINAL_CHAIN" ) == original );
    BOOST_CHECK( chains.GetNetChainByName( "ADDED_CHAIN" ) );
    BOOST_CHECK( chains.NetChainsBuilt() );
}
