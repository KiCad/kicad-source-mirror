/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcbnew_utils/board_test_utils.h>
#include <board.h>
#include <board_design_settings.h>
#include <eda_pattern_match.h>
#include <project/net_settings.h>
#include <board_connected_item.h>
#include <import_net_names.h>
#include <netinfo.h>
#include <reporter.h>
#include <settings/settings_manager.h>
#include <set>
#include <tuple>

struct IMPORT_NET_NAMES_FIXTURE
{
    IMPORT_NET_NAMES_FIXTURE()
    {
        KI_TEST::LoadBoard( settings, "issue11814", board );
        BOOST_REQUIRE( board );
    }

    SETTINGS_MANAGER settings;
    std::unique_ptr<BOARD> board;
};

BOOST_FIXTURE_TEST_SUITE( ImportedNetNames, IMPORT_NET_NAMES_FIXTURE )

BOOST_AUTO_TEST_CASE( RenamePreservesConnectedItemsAndBothNetIndexes )
{
    const wxString original = wxS( "Net-(Q6-Pad2)" );
    const wxString renamed = wxS( "Net-(Q6-B)" );
    NETINFO_ITEM* net = board->FindNet( original );
    BOOST_REQUIRE( net );
    const int code = net->GetNetCode();
    const unsigned netCount = board->GetNetInfo().GetNetCount();
    std::vector<std::tuple<BOARD_CONNECTED_ITEM*, NETINFO_ITEM*, int, wxString>> before;
    std::set<KICAD_T> renamedTypes;

    for( BOARD_CONNECTED_ITEM* item : board->AllConnectedItems() )
    {
        before.emplace_back( item, item->GetNet(), item->GetNetCode(), item->GetNetname() );

        if( item->GetNet() == net )
            renamedTypes.insert( item->Type() );
    }

    BOOST_REQUIRE( renamedTypes.count( PCB_PAD_T ) );
    BOOST_REQUIRE( renamedTypes.count( PCB_TRACE_T ) );
    BOOST_REQUIRE( renamedTypes.count( PCB_ZONE_T ) );
    WX_STRING_REPORTER reporter;
    BOOST_REQUIRE( ApplyImportedNetNameMap( *board,
            { { original, renamed }, { wxS( "N999999999" ), wxS( "Net-(Absent-Pad1)" ) } }, reporter ) );
    BOOST_CHECK( board->FindNet( original ) == nullptr );
    BOOST_CHECK( board->FindNet( renamed ) == net );
    BOOST_CHECK( board->FindNet( code ) == net );
    BOOST_CHECK_EQUAL( board->GetNetInfo().GetNetCount(), netCount );
    BOOST_CHECK( board->FindNet( wxS( "Net-(Absent-Pad1)" ) ) == nullptr );

    for( const auto& [item, oldNet, oldCode, oldName] : before )
    {
        BOOST_CHECK( item->GetNet() == oldNet );
        BOOST_CHECK_EQUAL( item->GetNetCode(), oldCode );
        BOOST_CHECK_EQUAL( item->GetNetname(), oldNet == net ? renamed : oldName );
    }
}

BOOST_AUTO_TEST_CASE( InvalidMappingsLeaveEveryNetUnchanged )
{
    const wxString first = wxS( "Net-(Q6-Pad2)" );
    const wxString second = wxS( "Net-(Q6-Pad3)" );
    BOOST_REQUIRE( board->FindNet( first ) );
    BOOST_REQUIRE( board->FindNet( second ) );
    BOOST_REQUIRE( board->FindNet( wxS( "GND" ) ) );
    std::vector<std::tuple<NETINFO_ITEM*, int, wxString>> before;

    for( NETINFO_ITEM* net : board->GetNetInfo() )
        before.emplace_back( net, net->GetNetCode(), net->GetNetname() );

    const std::vector<std::map<wxString, wxString>> invalid = {
        { { first, wxS( "Net-(Q6-B)" ) }, { second, wxS( "GND" ) } },
        { { first, wxS( "Net-(Q6-B)" ) }, { second, wxS( "Net-(Q6-B)" ) } },
        { { first, wxS( "Net-(Q6-B)" ) }, { second, wxString() } },
        { { wxString(), wxS( "Net-(Q6-B)" ) } }
    };

    for( const auto& names : invalid )
    {
        WX_STRING_REPORTER reporter;
        BOOST_CHECK( !ApplyImportedNetNameMap( *board, names, reporter ) );
        BOOST_CHECK( !reporter.GetMessages().IsEmpty() );
        BOOST_CHECK_EQUAL( board->GetNetInfo().GetNetCount(), before.size() );

        for( const auto& [net, code, name] : before )
        {
            BOOST_CHECK_EQUAL( net->GetNetname(), name );
            BOOST_CHECK_EQUAL( net->GetNetCode(), code );
            BOOST_CHECK( board->FindNet( name ) == net );
            BOOST_CHECK( board->FindNet( code ) == net );
        }
    }
}

BOOST_AUTO_TEST_CASE( SimultaneousNameSwapPreservesNetIdentity )
{
    const wxString first = wxS( "Net-(Q6-Pad2)" );
    const wxString second = wxS( "Net-(Q6-Pad3)" );
    NETINFO_ITEM* firstNet = board->FindNet( first );
    NETINFO_ITEM* secondNet = board->FindNet( second );
    BOOST_REQUIRE( firstNet && secondNet );
    const int firstCode = firstNet->GetNetCode();
    const int secondCode = secondNet->GetNetCode();
    WX_STRING_REPORTER reporter;
    BOOST_REQUIRE( ApplyImportedNetNameMap( *board, { { first, second }, { second, first } }, reporter ) );
    BOOST_CHECK( board->FindNet( first ) == secondNet );
    BOOST_CHECK( board->FindNet( second ) == firstNet );
    BOOST_CHECK( board->FindNet( firstCode ) == firstNet );
    BOOST_CHECK( board->FindNet( secondCode ) == secondNet );
}

BOOST_AUTO_TEST_CASE( ExactNetclassAssignmentsFollowRenamedNets )
{
    const wxString original = wxS( "/ATN_IN" );
    const wxString renamed = wxS( "/RENAMED_ATN_IN" );
    NETINFO_ITEM* net = board->FindNet( original );
    BOOST_REQUIRE( net );
    auto settings = board->GetDesignSettings().m_NetSettings;
    BOOST_REQUIRE( settings );
    const auto assignedClass = settings->GetEffectiveNetClass( original );
    BOOST_REQUIRE_EQUAL( assignedClass->GetName(), wxString( "HV" ) );
    BOOST_REQUIRE( settings->GetEffectiveNetClass( renamed ) == settings->GetDefaultNetclass() );
    const NETCLASS* netClass = net->GetNetClass();
    std::vector<std::pair<wxString, wxString>> expectedPatterns;

    for( const auto& [matcher, name] : settings->GetNetclassPatternAssignments() )
        expectedPatterns.emplace_back( matcher->GetPattern() == original ? renamed : matcher->GetPattern(), name );

    WX_STRING_REPORTER reporter;
    BOOST_REQUIRE( ApplyImportedNetNameMap( *board, { { original, renamed } }, reporter ) );
    BOOST_CHECK( net->GetNetClass() == netClass );
    BOOST_CHECK( settings->GetEffectiveNetClass( renamed ) == assignedClass );
    BOOST_CHECK( settings->GetEffectiveNetClass( original ) == settings->GetDefaultNetclass() );
    std::vector<std::pair<wxString, wxString>> actualPatterns;

    for( const auto& [matcher, name] : settings->GetNetclassPatternAssignments() )
        actualPatterns.emplace_back( matcher->GetPattern(), name );

    BOOST_CHECK( actualPatterns == expectedPatterns );
    settings->ClearAllCaches();
    BOOST_CHECK( settings->GetEffectiveNetClass( renamed ) == assignedClass );
}

BOOST_AUTO_TEST_CASE( NetColorAssignmentsFollowRenamedNets )
{
    const wxString original = wxS( "/ATN_IN" );
    const wxString renamed = wxS( "/RECOLOURED_ATN_IN" );
    auto settings = board->GetDesignSettings().m_NetSettings;
    BOOST_REQUIRE( settings );
    const KIGFX::COLOR4D colour( 0.25, 0.5, 0.75, 1.0 );
    settings->SetNetColorAssignment( original, colour );

    WX_STRING_REPORTER reporter;
    BOOST_REQUIRE( ApplyImportedNetNameMap( *board, { { original, renamed } }, reporter ) );

    const auto& colours = settings->GetNetColorAssignments();
    BOOST_CHECK_EQUAL( colours.count( original ), 0 );
    BOOST_REQUIRE_EQUAL( colours.count( renamed ), 1 );
    BOOST_CHECK( colours.at( renamed ) == colour );
}


BOOST_AUTO_TEST_SUITE_END()
