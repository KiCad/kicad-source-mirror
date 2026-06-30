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
 * or you may search the http://www.gnu.org website for the version 32 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file
 * Issue #9673: highlight and net navigator must treat {MIXED_BUS} and its expansion
 * {FOO BAR HAM EGGS} as the same bus when different sheets use different label forms.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <schematic_utils/schematic_file_util.h>

#include <connection_graph.h>
#include <schematic.h>
#include <sch_sheet.h>
#include <sch_screen.h>
#include <settings/settings_manager.h>
#include <locale_io.h>

struct ISSUE9673_FIXTURE
{
    ISSUE9673_FIXTURE()
    { }

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
};


// Equivalents must carry the sheet-path prefix the highlight/navigator code passes in.
BOOST_FIXTURE_TEST_CASE( Issue9673BusAliasEquivalence, ISSUE9673_FIXTURE )
{
    LOCALE_IO dummy;

    KI_TEST::LoadSchematic( m_settingsManager, "issue9673/issue9673", m_schematic );

    CONNECTION_GRAPH* graph = m_schematic->ConnectionGraph();

    std::vector<wxString> equivAlias = graph->GetEquivalentBusNames( wxT( "/{MIXED_BUS}" ) );
    BOOST_REQUIRE_EQUAL( equivAlias.size(), 1u );
    BOOST_CHECK_EQUAL( equivAlias[0], wxT( "/{FOO BAR HAM EGGS}" ) );

    std::vector<wxString> equivExpanded = graph->GetEquivalentBusNames( wxT( "/{FOO BAR HAM EGGS}" ) );
    BOOST_REQUIRE_EQUAL( equivExpanded.size(), 1u );
    BOOST_CHECK_EQUAL( equivExpanded[0], wxT( "/{MIXED_BUS}" ) );

    // A plain net and an undefined alias have no equivalents.
    BOOST_CHECK( graph->GetEquivalentBusNames( wxT( "/FOO" ) ).empty() );
    BOOST_CHECK( graph->GetEquivalentBusNames( wxT( "/{UNKNOWN_BUS}" ) ).empty() );
}


// Each form's equivalent must resolve to the other form's subgraphs; that cross-form reach is
// what the highlight and navigator rely on.
BOOST_FIXTURE_TEST_CASE( Issue9673EquivalentResolvesToSubgraphs, ISSUE9673_FIXTURE )
{
    LOCALE_IO dummy;

    KI_TEST::LoadSchematic( m_settingsManager, "issue9673/issue9673", m_schematic );

    CONNECTION_GRAPH* graph = m_schematic->ConnectionGraph();

    // Both written forms exist as separate subgraphs.
    BOOST_REQUIRE( !graph->GetAllSubgraphs( wxT( "/{MIXED_BUS}" ) ).empty() );
    BOOST_REQUIRE( !graph->GetAllSubgraphs( wxT( "/{FOO BAR HAM EGGS}" ) ).empty() );

    // Before the fix the equivalent list was empty for path-qualified names, so the cross-form
    // highlight missed.
    std::vector<wxString> equivAlias = graph->GetEquivalentBusNames( wxT( "/{MIXED_BUS}" ) );
    BOOST_REQUIRE_EQUAL( equivAlias.size(), 1u );
    BOOST_CHECK( !graph->GetAllSubgraphs( equivAlias[0] ).empty() );

    std::vector<wxString> equivExpanded = graph->GetEquivalentBusNames( wxT( "/{FOO BAR HAM EGGS}" ) );
    BOOST_REQUIRE_EQUAL( equivExpanded.size(), 1u );
    BOOST_CHECK( !graph->GetAllSubgraphs( equivExpanded[0] ).empty() );
}
