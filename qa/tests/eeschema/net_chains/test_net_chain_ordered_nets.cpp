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

#include <algorithm>
#include <set>
#include <vector>

#include <connection_graph.h>
#include <kiid.h>
#include <locale_io.h>
#include <schematic.h>
#include <sch_netchain.h>
#include <settings/settings_manager.h>


struct ORDERED_NETS_FIXTURE
{
    ORDERED_NETS_FIXTURE() : m_settingsManager() {}

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;

    void Load( const wxString& aFixtureName )
    {
        m_settingsManager.LoadProject( wxString() );
        KI_TEST::LoadSchematic( m_settingsManager, aFixtureName, m_schematic );
        m_schematic->ConnectionGraph()->Recalculate(
                m_schematic->BuildSheetListSortedByPageNumbers(), /*aUnconditional=*/true );
    }

    // Locate the largest potential chain in the graph.  Most fixtures we use here
    // produce a single dominant chain; tying tests to a specific name keeps them
    // brittle, so we just take the biggest.
    SCH_NETCHAIN* FindLargestPotentialChain() const
    {
        SCH_NETCHAIN* best = nullptr;

        for( const auto& sig : m_schematic->ConnectionGraph()->GetPotentialNetChains() )
        {
            if( !sig )
                continue;

            if( !best || sig->GetNets().size() > best->GetNets().size() )
                best = sig.get();
        }

        return best;
    }
};


BOOST_FIXTURE_TEST_SUITE( SchNetChainOrderedNets, ORDERED_NETS_FIXTURE )


// A four-net linear chain (R-R-R between three nets in series, total four nets).
// GetOrderedNets must return every member, with terminal A's net at the head and
// terminal B's net at the tail.
BOOST_AUTO_TEST_CASE( LinearChainCoversEveryMemberWithTerminalsAtEnds )
{
    LOCALE_IO dummy;
    Load( wxS( "net_chains_four_nets" ) );

    SCH_NETCHAIN* chain = FindLargestPotentialChain();
    BOOST_REQUIRE_MESSAGE( chain, "Fixture must contain at least one inferred chain" );
    BOOST_REQUIRE_EQUAL( chain->GetNets().size(), 4u );

    CONNECTION_GRAPH* graph = m_schematic->ConnectionGraph();
    const std::vector<wxString>& ordered = chain->GetOrderedNets( graph );

    // Every member appears exactly once.
    BOOST_REQUIRE_EQUAL( ordered.size(), chain->GetNets().size() );

    std::set<wxString> seen( ordered.begin(), ordered.end() );
    BOOST_CHECK( seen == chain->GetNets() );

    // Terminal nets land at the ends.
    wxString termA = chain->GetTerminalNetName( 0, graph );
    wxString termB = chain->GetTerminalNetName( 1, graph );
    BOOST_REQUIRE( !termA.IsEmpty() );
    BOOST_REQUIRE( !termB.IsEmpty() );
    BOOST_CHECK_EQUAL( ordered.front(), termA );
    BOOST_CHECK_EQUAL( ordered.back(),  termB );
}


// Second call must return the cached vector unchanged (same content, same order).
BOOST_AUTO_TEST_CASE( SecondCallReturnsIdenticalCachedVector )
{
    LOCALE_IO dummy;
    Load( wxS( "net_chains_four_nets" ) );

    SCH_NETCHAIN* chain = FindLargestPotentialChain();
    BOOST_REQUIRE( chain );
    CONNECTION_GRAPH* graph = m_schematic->ConnectionGraph();

    std::vector<wxString> first = chain->GetOrderedNets( graph );
    std::vector<wxString> second = chain->GetOrderedNets( graph );

    BOOST_CHECK( first == second );
}


// Mutating the chain (AddNet) must invalidate the cache and surface the new net.
BOOST_AUTO_TEST_CASE( AddNetInvalidatesCache )
{
    LOCALE_IO dummy;
    Load( wxS( "net_chains_four_nets" ) );

    SCH_NETCHAIN* chain = FindLargestPotentialChain();
    BOOST_REQUIRE( chain );
    CONNECTION_GRAPH* graph = m_schematic->ConnectionGraph();

    const std::vector<wxString> before = chain->GetOrderedNets( graph );
    const wxString sentinel = wxS( "__ordered_nets_test_sentinel__" );

    chain->AddNet( sentinel );

    const std::vector<wxString>& after = chain->GetOrderedNets( graph );

    BOOST_CHECK( std::find( after.begin(), after.end(), sentinel ) != after.end() );
    BOOST_CHECK_GT( after.size(), before.size() );
}


// Branched chain (Y or T topology): the shortest path between the two terminals
// is the prefix; any off-path member nets are appended alphabetically.  Whatever
// the fixture's specific topology, the contract is:
//   1. every member appears exactly once
//   2. terminal A's net is at index 0
//   3. terminal B's net appears (somewhere) before any off-path nets
BOOST_AUTO_TEST_CASE( BranchedChainPlacesPathBeforeOffPathNets )
{
    LOCALE_IO dummy;
    Load( wxS( "net_chains_branching_longer" ) );

    SCH_NETCHAIN* chain = FindLargestPotentialChain();
    BOOST_REQUIRE( chain );
    BOOST_REQUIRE_GE( chain->GetNets().size(), 3u );

    CONNECTION_GRAPH* graph = m_schematic->ConnectionGraph();
    const std::vector<wxString>& ordered = chain->GetOrderedNets( graph );

    BOOST_REQUIRE_EQUAL( ordered.size(), chain->GetNets().size() );

    std::set<wxString> seen( ordered.begin(), ordered.end() );
    BOOST_CHECK( seen == chain->GetNets() );

    wxString termA = chain->GetTerminalNetName( 0, graph );
    wxString termB = chain->GetTerminalNetName( 1, graph );
    BOOST_REQUIRE( !termA.IsEmpty() );
    BOOST_REQUIRE( !termB.IsEmpty() );
    BOOST_CHECK_EQUAL( ordered.front(), termA );

    // termB must appear somewhere; off-path nets are appended after it.
    auto itB = std::find( ordered.begin(), ordered.end(), termB );
    BOOST_CHECK( itB != ordered.end() );
}


// Null graph and unset terminals must yield an empty result without caching, so
// that a subsequent call once state is valid still rebuilds.
BOOST_AUTO_TEST_CASE( NullGraphReturnsEmpty )
{
    LOCALE_IO dummy;
    Load( wxS( "net_chains_four_nets" ) );

    SCH_NETCHAIN* chain = FindLargestPotentialChain();
    BOOST_REQUIRE( chain );

    const std::vector<wxString>& empty = chain->GetOrderedNets( nullptr );
    BOOST_CHECK( empty.empty() );

    // A subsequent valid call must still populate.
    const std::vector<wxString>& populated = chain->GetOrderedNets( m_schematic->ConnectionGraph() );
    BOOST_CHECK( !populated.empty() );
}


// A standalone SCH_NETCHAIN with no symbols and no terminals returns empty and
// stays dirty so future, properly-set-up calls will populate.
BOOST_AUTO_TEST_CASE( StandaloneEmptyChainStaysDirty )
{
    SCH_NETCHAIN bare;
    const std::vector<wxString>& first = bare.GetOrderedNets( nullptr );
    BOOST_CHECK( first.empty() );

    bare.AddNet( wxS( "A" ) );
    bare.AddNet( wxS( "B" ) );

    // Still empty: no graph, no symbols, no terminals.  Returns empty without crashing.
    const std::vector<wxString>& second = bare.GetOrderedNets( nullptr );
    BOOST_CHECK( second.empty() );
}


BOOST_AUTO_TEST_SUITE_END()
