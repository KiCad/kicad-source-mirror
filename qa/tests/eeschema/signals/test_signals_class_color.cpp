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
#include <sch_signal.h>
#include <sch_sheet.h>
#include <settings/settings_manager.h>
#include <locale_io.h>

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
    KI_TEST::LoadSchematic( m_settingsManager, wxString( "signals_four_nets" ),
                            m_schematic );

    CONNECTION_GRAPH* graph = m_schematic->ConnectionGraph();
    BOOST_REQUIRE( graph );

    graph->Recalculate( m_schematic->BuildSheetListSortedByPageNumbers(), true );

    // Seed override maps as if they had just been parsed from the schematic.
    std::map<wxString, wxString>      classes;
    std::map<wxString, KIGFX::COLOR4D> colors;
    classes[wxT( "MY_CHAIN" )] = wxT( "DDR_DATA" );
    colors[wxT( "MY_CHAIN" )]  = KIGFX::COLOR4D( 1.0, 0.5, 0.25, 1.0 );

    graph->SetSignalNetClassOverrides( classes );
    graph->SetSignalColorOverrides( colors );

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
    KI_TEST::LoadSchematic( m_settingsManager, wxString( "signals_four_nets" ),
                            m_schematic );

    CONNECTION_GRAPH* graph = m_schematic->ConnectionGraph();
    BOOST_REQUIRE( graph );

    graph->Recalculate( m_schematic->BuildSheetListSortedByPageNumbers(), true );

    // Make sure the override maps are empty.
    graph->SetSignalNetClassOverrides( {} );
    graph->SetSignalColorOverrides( {} );

    const auto& potentials = graph->GetPotentialNetChains();
    BOOST_REQUIRE( !potentials.empty() );

    SCH_NETCHAIN* committed = graph->CreateNetChainFromPotential( potentials.front().get(),
                                                                  wxT( "OTHER" ) );
    BOOST_REQUIRE( committed );

    BOOST_CHECK( committed->GetNetClass().IsEmpty() );
    BOOST_CHECK( committed->GetColor() == KIGFX::COLOR4D::UNSPECIFIED );
}
