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
#include <connectivity/conn_netchain_manager.h>
#include <schematic.h>
#include <sch_netchain.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_symbol.h>
#include <settings/settings_manager.h>
#include <locale_io.h>

struct SIGNALS_REMOVE_TEST_FIXTURE
{
    SIGNALS_REMOVE_TEST_FIXTURE() : m_settingsManager() {}

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
};

BOOST_FIXTURE_TEST_CASE( RemoveFromSignal_DisablesPropagationAndSplitsGroup, SIGNALS_REMOVE_TEST_FIXTURE )
{
    LOCALE_IO dummy;
    KI_TEST::LoadSchematic( m_settingsManager, wxString( "net_chains_four_nets" ), m_schematic );

    SCH_SHEET_LIST sheets = m_schematic->BuildSheetListSortedByPageNumbers();
    CONNECTION_GRAPH* graph = m_schematic->ConnectionGraph();
    graph->Recalculate( sheets, /*aUnconditional=*/true );

    auto& manager = m_schematic->NetChains();
    SCH_NETCHAIN* four = nullptr;

    for( const auto& sig : manager.GetPotentialNetChains() )
    {
        if( sig && sig->GetNets().size() == 4 )
            four = sig.get();
    }

    BOOST_REQUIRE_MESSAGE( four, "Expected initial 4-net signal present" );

    SCH_NETCHAIN* chain = manager.CreateNetChainFromPotential( four, wxS( "REMOVE" ) );
    BOOST_REQUIRE( chain );

    const wxString net = *chain->GetNets().begin();
    const auto     bridges = manager.GetBridgeSymbols( *chain, net );

    BOOST_REQUIRE_MESSAGE( !bridges.empty(), "No bridging symbol reported for a chain member" );
    BOOST_CHECK( manager.GetBridgeSymbols( *chain, wxS( "NOT_A_MEMBER" ) ).empty() );

    for( const auto& [symbol, screen] : bridges )
    {
        BOOST_CHECK( screen == m_schematic->CurrentSheet().LastScreen() );
        BOOST_CHECK( chain->GetSymbols().contains( symbol ) );
        symbol->SetPassthroughMode( SCH_SYMBOL::PASSTHROUGH_MODE::BLOCK );
    }

    graph->Recalculate( sheets, /*aUnconditional=*/true );

    for( const auto& sig : manager.GetPotentialNetChains() )
    {
        BOOST_CHECK_MESSAGE( sig->GetNets().size() < 4, "Expected removal to split the 4-net signal" );
        BOOST_CHECK_MESSAGE( !sig->GetNets().contains( net ), "Removed net still bridged into a chain" );
    }
}

// EOF
