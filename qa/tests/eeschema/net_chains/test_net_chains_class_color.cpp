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
#include <netclass.h>
#include <project.h>
#include <project/project_file.h>
#include <project/net_settings.h>
#include <settings/settings_manager.h>
#include <locale_io.h>


// Test backdoor declared in connection_graph.h; defined in test_net_chain_save_root_only.cpp.
void boost_test_inject_committed_net_chain( CONNECTION_GRAPH& aGraph,
                                            std::unique_ptr<SCH_NETCHAIN> aChain );


struct SIGNALS_CLASS_COLOR_FIXTURE
{
    SIGNALS_CLASS_COLOR_FIXTURE() : m_settingsManager() {}

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
};


// Validate that the connection graph correctly applies parsed netclass and color
// overrides to committed net chains.  This exercises the writer/parser/loader
// glue added in the e3ea709483 / 769c2efb85 / dea7492211 / 3b3863a843 commit
// chain without touching the file-format details directly.
BOOST_FIXTURE_TEST_CASE( NetChain_ApplyClassAndColorOverrides, SIGNALS_CLASS_COLOR_FIXTURE )
{
    LOCALE_IO dummy;
    KI_TEST::LoadSchematic( m_settingsManager, wxString( "net_chains_four_nets" ),
                            m_schematic );

    CONNECTION_GRAPH* graph = m_schematic->ConnectionGraph();
    BOOST_REQUIRE( graph );

    graph->Recalculate( m_schematic->BuildSheetListSortedByPageNumbers(), true );

    // Seed override maps as if they had just been parsed from the schematic.
    std::map<wxString, wxString>      classes;
    std::map<wxString, KIGFX::COLOR4D> colors;
    classes[wxT( "MY_CHAIN" )] = wxT( "DDR_DATA" );
    colors[wxT( "MY_CHAIN" )]  = KIGFX::COLOR4D( 1.0, 0.5, 0.25, 1.0 );

    graph->SetNetChainNetClassOverrides( classes );
    graph->SetNetChainColorOverrides( colors );

    // Promote the first detected potential chain so it gets a name we control.
    const auto& potentials = graph->GetPotentialNetChains();
    BOOST_REQUIRE( !potentials.empty() );

    SCH_NETCHAIN* potential = potentials.front().get();
    BOOST_REQUIRE( potential );

    SCH_NETCHAIN* committed = graph->CreateNetChainFromPotential( potential, wxT( "MY_CHAIN" ) );
    BOOST_REQUIRE( committed );
    BOOST_CHECK_EQUAL( committed->GetName(), wxT( "MY_CHAIN" ) );

    // The override maps should have been consulted at promotion time and
    // copied onto the committed chain.
    BOOST_CHECK_EQUAL( committed->GetNetClass(), wxT( "DDR_DATA" ) );
    BOOST_CHECK( committed->GetColor() != KIGFX::COLOR4D::UNSPECIFIED );
    BOOST_CHECK_CLOSE( committed->GetColor().r, 1.0, 1e-6 );
    BOOST_CHECK_CLOSE( committed->GetColor().g, 0.5, 1e-6 );
    BOOST_CHECK_CLOSE( committed->GetColor().b, 0.25, 1e-6 );
    BOOST_CHECK_CLOSE( committed->GetColor().a, 1.0, 1e-6 );
}


// Validate that promoting a chain with no override leaves the chain in
// its default ("no override") state.  This guards against an off-by-one
// where the override application leaks default values into chains that
// shouldn't have any.
BOOST_FIXTURE_TEST_CASE( NetChain_NoOverrideStaysDefault, SIGNALS_CLASS_COLOR_FIXTURE )
{
    LOCALE_IO dummy;
    KI_TEST::LoadSchematic( m_settingsManager, wxString( "net_chains_four_nets" ),
                            m_schematic );

    CONNECTION_GRAPH* graph = m_schematic->ConnectionGraph();
    BOOST_REQUIRE( graph );

    graph->Recalculate( m_schematic->BuildSheetListSortedByPageNumbers(), true );

    // Make sure the override maps are empty.
    graph->SetNetChainNetClassOverrides( {} );
    graph->SetNetChainColorOverrides( {} );

    const auto& potentials = graph->GetPotentialNetChains();
    BOOST_REQUIRE( !potentials.empty() );

    SCH_NETCHAIN* committed = graph->CreateNetChainFromPotential( potentials.front().get(),
                                                                  wxT( "OTHER" ) );
    BOOST_REQUIRE( committed );

    BOOST_CHECK( committed->GetNetClass().IsEmpty() );
    BOOST_CHECK( committed->GetColor() == KIGFX::COLOR4D::UNSPECIFIED );
}


// Regression for https://gitlab.com/kicad/code/kicad/-/issues/24498
//
// A net chain may assign a netclass to every member net.  On the PCB side
// board_netlist_updater mirrors that override into NET_SETTINGS as chain-derived pattern
// assignments, so each member net resolves to the chain's netclass.  The schematic never
// built the equivalent assignments, so GetEffectiveNetClass() (and the status-bar "Resolved
// Netclass" entry) ignored the chain's class.  ApplyNetChainNetclasses() closes that gap.
BOOST_FIXTURE_TEST_CASE( NetChain_NetclassResolvesForMemberNets, SIGNALS_CLASS_COLOR_FIXTURE )
{
    LOCALE_IO dummy;
    KI_TEST::LoadSchematic( m_settingsManager, wxString( "net_chains_four_nets" ), m_schematic );

    CONNECTION_GRAPH* graph = m_schematic->ConnectionGraph();
    BOOST_REQUIRE( graph );

    std::shared_ptr<NET_SETTINGS> ns = m_schematic->Project().GetProjectFile().NetSettings();
    BOOST_REQUIRE( ns );

    // The override only applies if the netclass exists, matching the board-side gating.
    std::shared_ptr<NETCLASS> highSpeed = std::make_shared<NETCLASS>( wxT( "HighSpeed" ) );
    ns->SetNetclass( wxT( "HighSpeed" ), highSpeed );

    auto chain = std::make_unique<SCH_NETCHAIN>();
    chain->SetName( wxT( "DQ_CHAIN" ) );
    chain->AddNet( wxT( "/NET_A" ) );
    chain->AddNet( wxT( "/NET_B" ) );
    chain->SetNetClass( wxT( "HighSpeed" ) );
    boost_test_inject_committed_net_chain( *graph, std::move( chain ) );

    BOOST_CHECK_EQUAL( ns->GetEffectiveNetClass( wxT( "/NET_A" ) )->GetName(), wxString( NETCLASS::Default ) );

    graph->ApplyNetChainNetclasses();

    BOOST_CHECK_EQUAL( ns->GetEffectiveNetClass( wxT( "/NET_A" ) )->GetName(), wxString( wxT( "HighSpeed" ) ) );
    BOOST_CHECK_EQUAL( ns->GetEffectiveNetClass( wxT( "/NET_B" ) )->GetName(), wxString( wxT( "HighSpeed" ) ) );
}


// Synthetic per-run member keys (SYNTHETIC_NET_PREFIX) embed subgraph codes that never match a
// resolved net name, so ApplyNetChainNetclasses() must skip them rather than emit a dead pattern.
BOOST_FIXTURE_TEST_CASE( NetChain_SyntheticMemberNetSkipped, SIGNALS_CLASS_COLOR_FIXTURE )
{
    LOCALE_IO dummy;
    KI_TEST::LoadSchematic( m_settingsManager, wxString( "net_chains_four_nets" ), m_schematic );

    CONNECTION_GRAPH* graph = m_schematic->ConnectionGraph();
    BOOST_REQUIRE( graph );

    std::shared_ptr<NET_SETTINGS> ns = m_schematic->Project().GetProjectFile().NetSettings();
    BOOST_REQUIRE( ns );

    std::shared_ptr<NETCLASS> highSpeed = std::make_shared<NETCLASS>( wxT( "HighSpeed" ) );
    ns->SetNetclass( wxT( "HighSpeed" ), highSpeed );

    const wxString synthetic = wxString( SCH_NETCHAIN::SYNTHETIC_NET_PREFIX ) + wxT( "42" );

    auto chain = std::make_unique<SCH_NETCHAIN>();
    chain->SetName( wxT( "DQ_CHAIN" ) );
    chain->AddNet( wxT( "/NET_A" ) );
    chain->AddNet( synthetic );
    chain->SetNetClass( wxT( "HighSpeed" ) );
    boost_test_inject_committed_net_chain( *graph, std::move( chain ) );

    graph->ApplyNetChainNetclasses();

    BOOST_CHECK_EQUAL( ns->GetEffectiveNetClass( wxT( "/NET_A" ) )->GetName(), wxString( wxT( "HighSpeed" ) ) );
    BOOST_CHECK_EQUAL( ns->GetEffectiveNetClass( synthetic )->GetName(), wxString( NETCLASS::Default ) );
}


// A chain referencing a netclass that no longer exists must not fabricate an assignment; the
// member nets stay on the default netclass just as the board-side updater leaves them.
BOOST_FIXTURE_TEST_CASE( NetChain_UnknownNetclassNotApplied, SIGNALS_CLASS_COLOR_FIXTURE )
{
    LOCALE_IO dummy;
    KI_TEST::LoadSchematic( m_settingsManager, wxString( "net_chains_four_nets" ), m_schematic );

    CONNECTION_GRAPH* graph = m_schematic->ConnectionGraph();
    BOOST_REQUIRE( graph );

    std::shared_ptr<NET_SETTINGS> ns = m_schematic->Project().GetProjectFile().NetSettings();
    BOOST_REQUIRE( ns );

    auto chain = std::make_unique<SCH_NETCHAIN>();
    chain->SetName( wxT( "DQ_CHAIN" ) );
    chain->AddNet( wxT( "/NET_A" ) );
    chain->SetNetClass( wxT( "Ghost" ) );
    boost_test_inject_committed_net_chain( *graph, std::move( chain ) );

    graph->ApplyNetChainNetclasses();

    BOOST_CHECK_EQUAL( ns->GetEffectiveNetClass( wxT( "/NET_A" ) )->GetName(), wxString( NETCLASS::Default ) );
}


// A full connectivity recalculation must leave the chain-derived assignments in place, so the
// status bar is correct on load and after every edit without an explicit ApplyNetChainNetclasses()
// call.  This drives the production buildConnectionGraph() path end to end.
BOOST_FIXTURE_TEST_CASE( NetChain_NetclassSurvivesRecalculate, SIGNALS_CLASS_COLOR_FIXTURE )
{
    LOCALE_IO dummy;
    KI_TEST::LoadSchematic( m_settingsManager, wxString( "net_chains_four_nets" ), m_schematic );

    CONNECTION_GRAPH* graph = m_schematic->ConnectionGraph();
    BOOST_REQUIRE( graph );

    SCH_SHEET_LIST sheets = m_schematic->BuildSheetListSortedByPageNumbers();
    graph->Recalculate( sheets, true );

    std::shared_ptr<NET_SETTINGS> ns = m_schematic->Project().GetProjectFile().NetSettings();
    BOOST_REQUIRE( ns );

    std::shared_ptr<NETCLASS> highSpeed = std::make_shared<NETCLASS>( wxT( "HighSpeed" ) );
    ns->SetNetclass( wxT( "HighSpeed" ), highSpeed );

    std::map<wxString, wxString> overrides;
    overrides[wxT( "MY_CHAIN" )] = wxT( "HighSpeed" );
    graph->SetNetChainNetClassOverrides( overrides );

    const auto& potentials = graph->GetPotentialNetChains();
    BOOST_REQUIRE( !potentials.empty() );

    SCH_NETCHAIN* committed = graph->CreateNetChainFromPotential( potentials.front().get(),
                                                                 wxT( "MY_CHAIN" ) );
    BOOST_REQUIRE( committed );
    BOOST_REQUIRE_EQUAL( committed->GetNetClass(), wxString( wxT( "HighSpeed" ) ) );

    // Real (non-synthetic) member nets are the ones the resolver can match by name.
    std::vector<wxString> namedNets;

    for( const wxString& net : committed->GetNets() )
    {
        if( !net.StartsWith( SCH_NETCHAIN::SYNTHETIC_NET_PREFIX ) )
            namedNets.push_back( net );
    }

    BOOST_REQUIRE( !namedNets.empty() );

    // The recalculation rebuilds connectivity from scratch; the chain override must be
    // reapplied automatically.
    graph->Recalculate( sheets, true );

    for( const wxString& net : namedNets )
        BOOST_CHECK_EQUAL( ns->GetEffectiveNetClass( net )->GetName(), wxString( wxT( "HighSpeed" ) ) );
}


// Deleting the chain's netclass (e.g. on the Net Classes page) must not leave a stale
// chain-derived assignment behind.  Re-running ApplyNetChainNetclasses() clears the prior
// entry and re-gates on HasNetclass(), so the member net falls back to the default netclass
// instead of resolving to a phantom implicit class.
BOOST_FIXTURE_TEST_CASE( NetChain_DeletedNetclassClearsStaleAssignment, SIGNALS_CLASS_COLOR_FIXTURE )
{
    LOCALE_IO dummy;
    KI_TEST::LoadSchematic( m_settingsManager, wxString( "net_chains_four_nets" ), m_schematic );

    CONNECTION_GRAPH* graph = m_schematic->ConnectionGraph();
    BOOST_REQUIRE( graph );

    std::shared_ptr<NET_SETTINGS> ns = m_schematic->Project().GetProjectFile().NetSettings();
    BOOST_REQUIRE( ns );

    std::shared_ptr<NETCLASS> highSpeed = std::make_shared<NETCLASS>( wxT( "HighSpeed" ) );
    ns->SetNetclass( wxT( "HighSpeed" ), highSpeed );

    auto chain = std::make_unique<SCH_NETCHAIN>();
    chain->SetName( wxT( "DQ_CHAIN" ) );
    chain->AddNet( wxT( "/NET_A" ) );
    chain->SetNetClass( wxT( "HighSpeed" ) );
    boost_test_inject_committed_net_chain( *graph, std::move( chain ) );

    graph->ApplyNetChainNetclasses();
    BOOST_CHECK_EQUAL( ns->GetEffectiveNetClass( wxT( "/NET_A" ) )->GetName(),
                       wxString( wxT( "HighSpeed" ) ) );

    // The netclass is removed; re-deriving must drop the now-orphaned assignment.
    ns->ClearNetclasses();
    graph->ApplyNetChainNetclasses();

    BOOST_CHECK_EQUAL( ns->GetEffectiveNetClass( wxT( "/NET_A" ) )->GetName(),
                       wxString( NETCLASS::Default ) );
}
