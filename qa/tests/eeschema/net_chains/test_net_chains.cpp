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
#include <sch_sheet.h>
#include <settings/settings_manager.h>
#include <locale_io.h>

struct SIGNALS_TEST_FIXTURE
{
    SIGNALS_TEST_FIXTURE() : m_settingsManager() {}

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
};

BOOST_FIXTURE_TEST_CASE( RebuildSignals_GroupsFourNetsIntoOneSignal, SIGNALS_TEST_FIXTURE )
{
    LOCALE_IO dummy;
    KI_TEST::LoadSchematic( m_settingsManager, wxString( "net_chains_four_nets" ), m_schematic );
    SCH_SHEET_LIST sheets = m_schematic->BuildSheetListSortedByPageNumbers();
    CONNECTION_GRAPH* graph = m_schematic->ConnectionGraph();
    graph->Recalculate( sheets, /*aUnconditional=*/true );

    const auto& netChains = graph->GetPotentialNetChains();
    bool foundFourNetSignal = false;
    for( const auto& sig : netChains )
    {
        if( sig && sig->GetNets().size() == 4 )
        {
            foundFourNetSignal = true;
            break;
        }
    }

    BOOST_CHECK_MESSAGE( foundFourNetSignal,
                         "Expected at least one signal composed of exactly 4 nets to be built" );
}

BOOST_FIXTURE_TEST_CASE( RebuildSignals_RespectsSignalLabelAndKeepsGrouping, SIGNALS_TEST_FIXTURE )
{
    LOCALE_IO dummy;
    KI_TEST::LoadSchematic( m_settingsManager, wxString( "net_chains_four_nets_labeled" ), m_schematic );
    SCH_SHEET_LIST sheets = m_schematic->BuildSheetListSortedByPageNumbers();
    CONNECTION_GRAPH* graph = m_schematic->ConnectionGraph();
    graph->Recalculate( sheets, /*aUnconditional=*/true );

    const auto& netChains = graph->GetPotentialNetChains();
    bool foundLabeled = false;
    for( const auto& sig : netChains )
    {
        if( !sig )
            continue;

        wxString name = sig->GetName();
        if( name.StartsWith( wxString( "/" ) ) )
            name = name.Mid( 1 );

        if( sig->GetNets().size() == 4 && name == wxString( "SIG" ) )
        {
            foundLabeled = true;
            break;
        }
    }

    BOOST_CHECK_MESSAGE( foundLabeled,
                         "Expected a 4-net signal named 'SIG' to be built from label" );
}

BOOST_FIXTURE_TEST_CASE( RebuildSignals_WithPullupBranch_ExcludesPowerBranch, SIGNALS_TEST_FIXTURE )
{
    LOCALE_IO dummy;
    KI_TEST::LoadSchematic( m_settingsManager, wxString( "net_chains_with_pullup" ), m_schematic );
    SCH_SHEET_LIST sheets = m_schematic->BuildSheetListSortedByPageNumbers();
    CONNECTION_GRAPH* graph = m_schematic->ConnectionGraph();
    graph->Recalculate( sheets, /*aUnconditional=*/true );

    const auto& netChains = graph->GetPotentialNetChains();

    // Pullup fixture has two resistors driving a single net through a pullup to VCC. The chain
    // walker should produce a multi-net chain that does NOT pull VCC into the group, since power
    // nets are sinks rather than chain participants.
    bool mainSignalExcludesVCC = false;
    for( const auto& sig : netChains )
    {
        if( !sig )
            continue;

        const auto& nets = sig->GetNets();

        if( nets.size() < 2 )
            continue;

        bool containsVCC = false;
        for( const wxString& n : nets )
        {
            wxString nn = n;

            if( nn.StartsWith( wxString( "/" ) ) )
                nn = nn.Mid( 1 );

            if( nn.CmpNoCase( wxString( "VCC" ) ) == 0 )
            {
                containsVCC = true;
                break;
            }
        }

        if( !containsVCC )
        {
            mainSignalExcludesVCC = true;
            break;
        }
    }

    BOOST_CHECK_MESSAGE( mainSignalExcludesVCC,
                         "Expected at least one multi-net signal that does not include VCC "
                         "(power branch should not be merged into the main signal)" );
}

BOOST_FIXTURE_TEST_CASE( RebuildSignals_WithBypassCap_ExcludesPowerBranch, SIGNALS_TEST_FIXTURE )
{
    LOCALE_IO dummy;
    KI_TEST::LoadSchematic( m_settingsManager, wxString( "net_chains_with_bypass" ), m_schematic );
    SCH_SHEET_LIST sheets = m_schematic->BuildSheetListSortedByPageNumbers();
    CONNECTION_GRAPH* graph = m_schematic->ConnectionGraph();
    graph->Recalculate( sheets, /*aUnconditional=*/true );

    const auto& netChains = graph->GetPotentialNetChains();

    // Bypass-cap fixture has a signal path with a decoupling capacitor to GND. The chain walker
    // should produce a multi-net chain that does NOT pull GND into the group, since power nets
    // are sinks rather than chain participants.
    bool mainSignalExcludesGND = false;
    for( const auto& sig : netChains )
    {
        if( !sig )
            continue;

        const auto& nets = sig->GetNets();

        if( nets.size() < 2 )
            continue;

        bool containsGND = false;
        for( const wxString& n : nets )
        {
            wxString nn = n;

            if( nn.StartsWith( wxString( "/" ) ) )
                nn = nn.Mid( 1 );

            if( nn.CmpNoCase( wxString( "GND" ) ) == 0 )
            {
                containsGND = true;
                break;
            }
        }

        if( !containsGND )
        {
            mainSignalExcludesGND = true;
            break;
        }
    }

    BOOST_CHECK_MESSAGE( mainSignalExcludesGND,
                         "Expected at least one multi-net signal that does not include GND "
                         "(power branch should not be merged into the main signal)" );
}

// EOF
