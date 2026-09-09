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
#include <schematic.h>
#include <sch_sheet.h>
#include <sch_screen.h>
#include <netclass.h>
#include <sch_label.h>
#include <project.h>
#include <project/project_file.h>
#include <project/net_settings.h>
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

BOOST_FIXTURE_TEST_CASE( NetChain_AppendAdoptionKeepsCommittedChains, SIGNALS_TEST_FIXTURE )
{
    LOCALE_IO dummy;
    KI_TEST::LoadSchematic( m_settingsManager, wxString( "net_chains_four_nets_labeled" ), m_schematic );
    m_schematic->ConnectionGraph()->Recalculate( m_schematic->Hierarchy(), true );

    CONNECTION_GRAPH* graph = m_schematic->ConnectionGraph();
    BOOST_REQUIRE( !graph->GetPotentialNetChains().empty() );

    SCH_NETCHAIN* committed =
            graph->CreateNetChainFromPotential( graph->GetPotentialNetChains().front().get(), "APPEND_CHAIN" );
    BOOST_REQUIRE( committed );

    const std::set<wxString> nets = committed->GetNets();
    const SCH_SHEET_PATH     path = m_schematic->Hierarchy().front();

    SCHEMATIC_CONTENT content;
    content.targetSheet = path.Last();
    content.hierarchy = m_schematic->Hierarchy();
    content.currentSheet = path;
    content.connectionGraph = std::make_unique<CONNECTION_GRAPH>( m_schematic.get() );
    content.preserveNetChains = true;

    for( SCH_ITEM* item : path.LastScreen()->Items() )
        content.screenItems.insert( item );

    m_schematic->AdoptContent( std::move( content ) );

    graph = m_schematic->ConnectionGraph();
    BOOST_REQUIRE( graph->GetNetChainByName( "APPEND_CHAIN" ) == committed );

    graph->Recalculate( m_schematic->Hierarchy(), true );
    BOOST_CHECK( committed->GetNets() == nets );
    BOOST_CHECK( !committed->GetSymbols().empty() );
}



BOOST_FIXTURE_TEST_CASE( NetChain_TemporaryGraphPreservesProjectAssignments, SIGNALS_TEST_FIXTURE )
{
    LOCALE_IO dummy;
    KI_TEST::LoadSchematic( m_settingsManager, wxString( "net_chains_four_nets_labeled" ), m_schematic );
    m_schematic->ConnectionGraph()->Recalculate( m_schematic->Hierarchy(), true );

    CONNECTION_GRAPH* graph = m_schematic->ConnectionGraph();
    BOOST_REQUIRE( !graph->GetPotentialNetChains().empty() );

    SCH_NETCHAIN* committed =
            graph->CreateNetChainFromPotential( graph->GetPotentialNetChains().front().get(), "NETCLASS_CHAIN" );
    BOOST_REQUIRE( committed );

    std::shared_ptr<NET_SETTINGS> ns = m_schematic->Project().GetProjectFile().NetSettings();
    std::shared_ptr<NETCLASS>     highSpeed = std::make_shared<NETCLASS>( wxT( "HighSpeed" ) );
    ns->SetNetclass( wxT( "HighSpeed" ), highSpeed );
    committed->SetNetClass( wxT( "HighSpeed" ) );
    graph->ApplyNetChainNetclasses();
    BOOST_REQUIRE( ns->HasChainPatternAssignments( NET_CHAIN_SOURCE::SCHEMATIC ) );

    CONNECTION_GRAPH temporary( m_schematic.get() );
    temporary.Recalculate( m_schematic->Hierarchy(), true );
    BOOST_CHECK( ns->HasChainPatternAssignments( NET_CHAIN_SOURCE::SCHEMATIC ) );
}


BOOST_FIXTURE_TEST_CASE( RebuildSignals_SelectsLabelNameIndependentlyOfItemOrder, SIGNALS_TEST_FIXTURE )
{
    LOCALE_IO locale;
    KI_TEST::LoadSchematic( m_settingsManager, "net_chains_four_nets_labeled", m_schematic );
    SCH_SCREEN* screen = m_schematic->GetTopLevelSheet()->GetScreen();
    SCH_LABEL* original = nullptr;

    for( SCH_ITEM* item : screen->Items().OfType( SCH_LABEL_T ) )
    {
        original = static_cast<SCH_LABEL*>( item );
        break;
    }

    BOOST_REQUIRE( original );
    auto copy = std::unique_ptr<SCH_LABEL>( static_cast<SCH_LABEL*>( original->Clone() ) );
    const_cast<KIID&>( copy->m_Uuid ) = KIID();
    SCH_LABEL* second = copy.get();
    screen->Append( copy.release() );
    auto* graph = m_schematic->ConnectionGraph();

    for( bool reverse : { false, true } )
    {
        original->SetText( reverse ? "ZZZ" : "AAA" );
        second->SetText( reverse ? "AAA" : "ZZZ" );
        graph->Recalculate( m_schematic->Hierarchy(), true );
        const auto& chains = graph->GetPotentialNetChains();
        BOOST_REQUIRE_EQUAL( chains.size(), 1 );
        BOOST_CHECK_EQUAL( chains.front()->GetName(), wxString( "AAA" ) );
    }
}

BOOST_FIXTURE_TEST_CASE( RebuildSignals_DistinguishesSharedScreenInstances, SIGNALS_TEST_FIXTURE )
{
    LOCALE_IO locale;
    KI_TEST::LoadSchematic( m_settingsManager, "net_chains_four_nets_labeled", m_schematic );
    SCH_SHEET* original = m_schematic->GetTopLevelSheet();
    auto copy = std::unique_ptr<SCH_SHEET>( static_cast<SCH_SHEET*>( original->Clone() ) );
    const_cast<KIID&>( copy->m_Uuid ) = KIID();
    copy->SetName( "Second" );
    BOOST_REQUIRE( copy->GetScreen() == original->GetScreen() );
    m_schematic->AddTopLevelSheet( copy.release() );
    const SCH_SHEET_LIST sheets = m_schematic->Hierarchy();
    BOOST_REQUIRE_EQUAL( sheets.size(), 2 );
    auto& manager = m_schematic->NetChains();

    m_schematic->ConnectionGraph()->Recalculate( sheets, true );
    const auto& chains = manager.GetPotentialNetChains();
    BOOST_REQUIRE_EQUAL( chains.size(), 2 );
    BOOST_CHECK_EQUAL( chains[0]->GetNets().size(), 4 );
    BOOST_CHECK_EQUAL( chains[1]->GetNets().size(), 4 );

    for( const auto& chain : chains )
    {
        BOOST_CHECK_EQUAL( chain->GetName(), wxString( "SIG" ) );
        BOOST_CHECK_EQUAL( chain->GetSymbols().size(), 3u );
        std::set<wxString> references;

        for( SCH_SYMBOL* symbol : chain->GetSymbols() )
            references.insert( symbol->GetRef( &sheets[0] ) );

        const std::set<wxString> expected{ "R1", "R2", "R3" };
        BOOST_CHECK( references == expected );
    }

    for( const wxString& net : chains[0]->GetNets() )
        BOOST_CHECK( !chains[1]->GetNets().contains( net ) );

    BOOST_REQUIRE( !chains[0]->GetSymbols().empty() );
    auto* bridge = *chains[0]->GetSymbols().begin();
    const auto pins = bridge->GetPins( &sheets[0] );
    BOOST_REQUIRE_EQUAL( pins.size(), 2u );
    auto* first = manager.FindPotentialNetChainBetweenPins( pins[0], sheets[0], pins[1], sheets[0] );
    auto* second = manager.FindPotentialNetChainBetweenPins( pins[0], sheets[1], pins[1], sheets[1] );
    BOOST_REQUIRE( first );
    BOOST_REQUIRE( second );
    BOOST_CHECK( first != second );

    for( int endpoint : { 0, 1 } )
    {
        BOOST_CHECK( first->GetTerminalPath( endpoint ) == sheets[0].Path() );
        BOOST_CHECK( second->GetTerminalPath( endpoint ) == sheets[1].Path() );
    }

    BOOST_CHECK( !manager.FindPotentialNetChainBetweenPins( pins[0], sheets[0], pins[1], sheets[1] ) );

    auto* ambiguous = manager.CreateManualNetChain(
            "AMBIGUOUS_INSTANCE", first->GetSymbols(), first->GetNets(),
            first->GetTerminalPinA(), first->GetTerminalPinB(),
            first->GetTerminalRef( 0 ), first->GetTerminalPinNum( 0 ),
            first->GetTerminalRef( 1 ), first->GetTerminalPinNum( 1 ) );
    BOOST_CHECK( !ambiguous );

    if( ambiguous )
        manager.DeleteCommittedNetChain( "AMBIGUOUS_INSTANCE" );

    auto* firstCommitted = manager.CreateNetChainFromPotential( first, "FIRST_INSTANCE" );
    auto* secondCommitted = manager.CreateNetChainFromPotential( second, "SECOND_INSTANCE" );
    BOOST_REQUIRE( firstCommitted );
    BOOST_REQUIRE( secondCommitted );
    const auto firstNets = firstCommitted->GetNets();
    const auto secondNets = secondCommitted->GetNets();
    BOOST_CHECK( !manager.ReplaceNetChainTerminalPin(
            { "FIRST_INSTANCE", 0, pins[0]->m_Uuid, sheets[1].Path() } ) );
    BOOST_CHECK( firstCommitted->GetTerminalPath( 0 ) == sheets[0].Path() );
    BOOST_REQUIRE( manager.ReplaceNetChainTerminalPin(
            { "FIRST_INSTANCE", 0, pins[0]->m_Uuid, sheets[0].Path() } ) );
    BOOST_REQUIRE( manager.ReplaceNetChainTerminalPin(
            { "SECOND_INSTANCE", 0, pins[0]->m_Uuid, sheets[1].Path() } ) );
    m_schematic->ConnectionGraph()->Recalculate( sheets, true );
    BOOST_CHECK( firstCommitted->GetTerminalPath( 0 ) == sheets[0].Path() );
    BOOST_CHECK( secondCommitted->GetTerminalPath( 0 ) == sheets[1].Path() );
    BOOST_CHECK( firstCommitted->GetNets() == firstNets );
    BOOST_CHECK( secondCommitted->GetNets() == secondNets );
    BOOST_REQUIRE( manager.DeleteCommittedNetChain( "FIRST_INSTANCE" ) );
    BOOST_REQUIRE( manager.DeleteCommittedNetChain( "SECOND_INSTANCE" ) );
}

// EOF
