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
 */

#include <boost/test/unit_test.hpp>

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <schematic_utils/schematic_file_util.h>

#include <connection_graph.h>
#include <schematic.h>
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

    size_t fourCount = 0;
    SCH_NETCHAIN* four = nullptr;
    for( const auto& sig : graph->GetPotentialNetChains() )
    {
        if( sig && sig->GetNets().size() == 4 )
        {
            four = sig.get();
            fourCount++;
        }
    }

    BOOST_REQUIRE_MESSAGE( fourCount >= 1, "Expected initial 4-net signal present" );

    wxString firstNet = *four->GetNets().begin();
    SCH_SCREEN* screen = m_schematic->CurrentSheet().LastScreen();

    auto effectiveNetNameForPin = [&]( SCH_PIN* aPin ) -> wxString {
        if( CONNECTION_SUBGRAPH* sg = graph->GetSubgraphForItem( aPin ) )
        {
            wxString n = sg->GetNetName();
            if( !n.IsEmpty() )
                return n;
        }
        return aPin->GetDefaultNetName( m_schematic->CurrentSheet() );
    };

    int disabledCount = 0;
    for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* sym = static_cast<SCH_SYMBOL*>( item );
        auto pins = sym->GetPins( &m_schematic->CurrentSheet() );
        if( pins.size() != 2 )
            continue;

        wxString nameA = effectiveNetNameForPin( pins[0] );
        wxString nameB = effectiveNetNameForPin( pins[1] );

        if( ( nameA == firstNet && nameB != firstNet ) || ( nameB == firstNet && nameA != firstNet ) )
        {
            sym->SetPassthrough( false );
            disabledCount++;
        }
    }

    BOOST_REQUIRE_MESSAGE( disabledCount > 0, "Test did not find any bridging 2-pin symbol to disable" );

    graph->Recalculate( sheets, /*aUnconditional=*/true );

    bool stillHasFour = false;
    for( const auto& sig : graph->GetPotentialNetChains() )
    {
        if( sig && sig->GetNets().size() == 4 )
        {
            stillHasFour = true;
            break;
        }
    }

    BOOST_CHECK_MESSAGE( !stillHasFour, "Expected removal to split the 4-net signal" );
}

// EOF
