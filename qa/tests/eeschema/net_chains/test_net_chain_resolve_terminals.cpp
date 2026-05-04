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

#include <connection_graph.h>
#include <sch_netchain.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <map>
#include <memory>
#include <utility>
#include <vector>


// Friend shim into CONNECTION_GRAPH::resolvePotentialChainByTerminals so the test can
// exercise the resolver without exposing it on the public API. Mirrors the existing
// boost_test_inject_committed_net_chain pattern.
SCH_NETCHAIN* boost_test_resolve_potential_chain_by_terminals(
        const std::pair<std::pair<wxString, wxString>,
                        std::pair<wxString, wxString>>& aTerms,
        const std::map<std::pair<wxString, wxString>, wxString>& aRefPinToNet,
        const std::vector<std::unique_ptr<SCH_NETCHAIN>>& aPotentials,
        const wxString& aChainName )
{
    CONNECTION_GRAPH::CHAIN_TERMINAL_REFS termRefs{
        { aTerms.first.first, aTerms.first.second },
        { aTerms.second.first, aTerms.second.second }
    };

    return CONNECTION_GRAPH::resolvePotentialChainByTerminals( termRefs, aRefPinToNet,
                                                               aPotentials, aChainName );
}


/**
 * Regression for [H-5]. The chain restore path used to resolve only the saved 'from' terminal
 * and pick the first potential chain whose net set contained that single net. When two
 * potential chains shared a 'from' net but diverged at the 'to' end, the wrong chain was
 * promoted on reload. The fix resolves both endpoints and requires the candidate to span
 * BOTH endpoint nets.
 */
BOOST_AUTO_TEST_SUITE( NetChainRestoreResolveBothTerminals )


using REF_PIN_KEY = std::pair<wxString, wxString>;
using REF_PIN_TO_NET = std::map<REF_PIN_KEY, wxString>;
using TERM_PAIR = std::pair<REF_PIN_KEY, REF_PIN_KEY>;


static std::unique_ptr<SCH_NETCHAIN> makePotential( const wxString& aName,
                                                    std::initializer_list<wxString> aNets )
{
    auto sig = std::make_unique<SCH_NETCHAIN>();
    sig->SetName( aName );

    for( const wxString& n : aNets )
        sig->AddNet( n );

    return sig;
}


BOOST_AUTO_TEST_CASE( PicksChainContainingBothTerminalNets )
{
    REF_PIN_TO_NET refPinToNet;
    refPinToNet[{ wxT( "U1" ), wxT( "1" ) }] = wxT( "/SHARED" );
    refPinToNet[{ wxT( "U3" ), wxT( "5" ) }] = wxT( "/TARGET_TO" );

    std::vector<std::unique_ptr<SCH_NETCHAIN>> potentials;

    // First in vector shares the 'from' net but ends on a different 'to' net. Old logic
    // would have wrongly picked this one.
    potentials.push_back( makePotential( wxT( "potA" ), { wxT( "/SHARED" ), wxT( "/OTHER_TO" ) } ) );

    // Second in vector matches both endpoints. This is the correct candidate.
    potentials.push_back( makePotential( wxT( "potB" ), { wxT( "/SHARED" ), wxT( "/TARGET_TO" ) } ) );

    TERM_PAIR terms{
        { wxT( "U1" ), wxT( "1" ) },
        { wxT( "U3" ), wxT( "5" ) }
    };

    SCH_NETCHAIN* match = boost_test_resolve_potential_chain_by_terminals(
            terms, refPinToNet, potentials, wxT( "TEST_CHAIN" ) );

    BOOST_REQUIRE( match );
    BOOST_CHECK_EQUAL( match->GetName(), wxT( "potB" ) );
}


BOOST_AUTO_TEST_CASE( ReturnsNullWhenNoPotentialSpansBothEndpoints )
{
    REF_PIN_TO_NET refPinToNet;
    refPinToNet[{ wxT( "U1" ), wxT( "1" ) }] = wxT( "/SHARED" );
    refPinToNet[{ wxT( "U3" ), wxT( "5" ) }] = wxT( "/TARGET_TO" );

    std::vector<std::unique_ptr<SCH_NETCHAIN>> potentials;
    potentials.push_back( makePotential( wxT( "potA" ), { wxT( "/SHARED" ), wxT( "/OTHER" ) } ) );
    potentials.push_back( makePotential( wxT( "potB" ), { wxT( "/UNRELATED_FROM" ),
                                                          wxT( "/TARGET_TO" ) } ) );

    TERM_PAIR terms{
        { wxT( "U1" ), wxT( "1" ) },
        { wxT( "U3" ), wxT( "5" ) }
    };

    SCH_NETCHAIN* match = boost_test_resolve_potential_chain_by_terminals(
            terms, refPinToNet, potentials, wxT( "TEST_CHAIN" ) );

    BOOST_CHECK( match == nullptr );
}


BOOST_AUTO_TEST_CASE( ReturnsNullWhenEitherTerminalUnresolvable )
{
    REF_PIN_TO_NET refPinToNet;
    refPinToNet[{ wxT( "U1" ), wxT( "1" ) }] = wxT( "/SHARED" );

    std::vector<std::unique_ptr<SCH_NETCHAIN>> potentials;
    potentials.push_back( makePotential( wxT( "potA" ), { wxT( "/SHARED" ), wxT( "/X" ) } ) );

    TERM_PAIR terms{
        { wxT( "U1" ), wxT( "1" ) },
        { wxT( "MISSING" ), wxT( "9" ) }
    };

    SCH_NETCHAIN* match = boost_test_resolve_potential_chain_by_terminals(
            terms, refPinToNet, potentials, wxT( "TEST_CHAIN" ) );

    BOOST_CHECK( match == nullptr );
}


BOOST_AUTO_TEST_CASE( SwappedTerminalOrderStillResolves )
{
    REF_PIN_TO_NET refPinToNet;
    refPinToNet[{ wxT( "U1" ), wxT( "1" ) }] = wxT( "/A" );
    refPinToNet[{ wxT( "U2" ), wxT( "2" ) }] = wxT( "/B" );

    std::vector<std::unique_ptr<SCH_NETCHAIN>> potentials;
    potentials.push_back( makePotential( wxT( "pot" ), { wxT( "/A" ), wxT( "/B" ) } ) );

    TERM_PAIR terms{
        { wxT( "U2" ), wxT( "2" ) },
        { wxT( "U1" ), wxT( "1" ) }
    };

    SCH_NETCHAIN* match = boost_test_resolve_potential_chain_by_terminals(
            terms, refPinToNet, potentials, wxT( "pot" ) );

    BOOST_REQUIRE( match );
    BOOST_CHECK_EQUAL( match->GetName(), wxT( "pot" ) );
}


// Companion regression for [H-5]. RebuildNetChains constructs both the potential chain net
// set and the refPin->net lookup using the same normalization: when GetNetName() is empty
// or contains "<NO NET>", a synthetic key __SG_<code> is substituted. The lookup table here
// must use that same synthetic key for the unnamed endpoint, otherwise common
// one-named-one-unnamed chains fail to restore under the strict both-endpoint rule.
BOOST_AUTO_TEST_CASE( ResolvesWhenOneEndpointIsUnnamedSyntheticName )
{
    REF_PIN_TO_NET refPinToNet;
    refPinToNet[{ wxT( "U1" ), wxT( "1" ) }] = wxT( "/NAMED_SIDE" );
    refPinToNet[{ wxT( "U2" ), wxT( "2" ) }] = wxT( "__SG_42" );

    std::vector<std::unique_ptr<SCH_NETCHAIN>> potentials;
    potentials.push_back( makePotential( wxT( "pot" ),
                                         { wxT( "/NAMED_SIDE" ), wxT( "__SG_42" ) } ) );

    TERM_PAIR terms{
        { wxT( "U1" ), wxT( "1" ) },
        { wxT( "U2" ), wxT( "2" ) }
    };

    SCH_NETCHAIN* match = boost_test_resolve_potential_chain_by_terminals(
            terms, refPinToNet, potentials, wxT( "TEST_CHAIN" ) );

    BOOST_REQUIRE( match );
    BOOST_CHECK_EQUAL( match->GetName(), wxT( "pot" ) );
}


BOOST_AUTO_TEST_SUITE_END()
