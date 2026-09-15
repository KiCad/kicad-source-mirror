/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <connectivity/conn_navigation.h>
#include <connectivity/conn_facade.h>
#include <schematic_utils/schematic_file_util.h>
#include <schematic.h>
#include <settings/settings_manager.h>
#include <advanced_config.h>
#include <connection_graph.h>
#include <sch_item.h>
#include <sch_label.h>
#include <sch_line.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_symbol.h>
#include <algorithm>
#include <set>
#include <scoped_set_reset.h>

using namespace SCH_CONNECTIVITY;

BOOST_AUTO_TEST_SUITE( ConnectivityNavigation )

BOOST_AUTO_TEST_CASE( AliasMemberQueriesMatchNativeMembership )
{
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, false );
    SETTINGS_MANAGER settings;
    std::unique_ptr<SCHEMATIC> schematic;
    KI_TEST::LoadSchematic( settings, "netlists/hierarchy_aliases/hierarchy_aliases", schematic );
    schematic->RebuildConnectivity();
    const wxString name = wxS( "/S0.BOOT.SDA" );
    const auto legacyNames = NAVIGATION_QUERY( *schematic ).NetNames();
    const auto direct = NAVIGATION_QUERY( *schematic ).NetItems( name, false );
    const auto withBuses = NAVIGATION_QUERY( *schematic ).NetItems( name, true );
    BOOST_REQUIRE( !direct.empty() );
    BOOST_REQUIRE( !withBuses.empty() );
    size_t directCount = 0;
    size_t expandedCount = 0;

    for( const auto& [path, items] : direct )
        directCount += items.size();

    for( const auto& [path, items] : withBuses )
    {
        expandedCount += items.size();
        BOOST_CHECK_EQUAL( std::set<SCH_ITEM*>( items.begin(), items.end() ).size(), items.size() );
    }

    BOOST_REQUIRE_GT( expandedCount, directCount );
    const auto root = schematic->Hierarchy().front();
    std::optional<wxString> busName;

    for( SCH_ITEM* item : root.LastScreen()->Items().OfType( SCH_LABEL_T ) )
    {
        if( static_cast<SCH_LABEL*>( item )->GetText() == wxS( "S0{ALIAS1}" ) )
            busName = item->GetConnectionName( &root );
    }

    BOOST_REQUIRE( busName );
    const auto busItems = NAVIGATION_QUERY( *schematic ).NetItems( *busName, true );
    BOOST_REQUIRE( !busItems.empty() );
    enabled = true;
    schematic->RebuildConnectivity();
    BOOST_CHECK( NAVIGATION_QUERY( *schematic ).NetItems( name, false ) == direct );
    BOOST_CHECK( NAVIGATION_QUERY( *schematic ).NetItems( name, true ) == withBuses );
    BOOST_CHECK( NAVIGATION_QUERY( *schematic ).NetItems( *busName, true ) == busItems );
    const auto names = NAVIGATION_QUERY( *schematic ).NetNames();
    BOOST_CHECK( names == legacyNames );
    BOOST_CHECK( std::ranges::find( names, name ) != names.end() );
}

BOOST_AUTO_TEST_CASE( SharedScreenLocalNetsKeepTheirInstanceMembership )
{
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, false );
    SETTINGS_MANAGER settings;
    std::unique_ptr<SCHEMATIC> schematic;
    KI_TEST::LoadSchematic( settings, "net_chains_four_nets_labeled", schematic );
    auto* original = schematic->GetTopLevelSheet();
    auto copy = std::unique_ptr<SCH_SHEET>( static_cast<SCH_SHEET*>( original->Clone() ) );
    const_cast<KIID&>( copy->m_Uuid ) = KIID();
    copy->SetName( wxS( "Second" ) );
    schematic->AddTopLevelSheet( copy.release() );
    const auto paths = schematic->Hierarchy();
    BOOST_REQUIRE_EQUAL( paths.size(), 2u );
    BOOST_REQUIRE( paths[0].LastScreen() == paths[1].LastScreen() );
    SCH_LABEL* label = nullptr;

    for( SCH_ITEM* item : original->GetScreen()->Items().OfType( SCH_LABEL_T ) )
    {
        auto* candidate = static_cast<SCH_LABEL*>( item );

        if( candidate->GetText() == wxS( "SIG" ) )
            label = candidate;
    }

    BOOST_REQUIRE( label );

    for( bool backend : { false, true } )
    {
        enabled = backend;
        schematic->RebuildConnectivity();

        for( const auto& path : paths )
        {
            const auto name = label->GetConnectionName( &path );
            BOOST_REQUIRE( name );
            const auto result = NAVIGATION_QUERY( *schematic ).NetItems( *name, false );
            BOOST_REQUIRE_EQUAL( result.size(), 1u );
            BOOST_CHECK( result.begin()->first.Path() == path.Path() );
            const auto& items = result.begin()->second;
            BOOST_CHECK( std::ranges::find( items, label ) != items.end() );
        }
    }
}

BOOST_AUTO_TEST_CASE( UnprefixedAliasAndExpandedBusReachTheSamePhysicalItems )
{
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, false );
    SETTINGS_MANAGER settings;
    std::unique_ptr<SCHEMATIC> schematic;
    KI_TEST::LoadSchematic( settings, "issue9673/issue9673", schematic );
    NET_ITEMS_BY_SHEET legacy;

    for( bool backend : { false, true } )
    {
        enabled = backend;
        schematic->RebuildConnectivity();
        const NAVIGATION_QUERY query( *schematic );
        const wxString aliasName = wxS( "/{MIXED_BUS}" );
        const auto alias = query.NetItems( aliasName, true );
        const auto expanded = query.NetItems( wxS( "/{FOO BAR HAM EGGS}" ), true );
        const auto direct = query.NetItems( aliasName, false );
        BOOST_REQUIRE( !direct.empty() );
        BOOST_CHECK_GT( alias.size(), direct.size() );
        BOOST_CHECK( alias == expanded );

        if( backend )
            BOOST_CHECK( alias == legacy );
        else
            legacy = alias;
    }
}

BOOST_AUTO_TEST_CASE( StaleRootPreservesLiveChildMembership )
{
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, true );
    SETTINGS_MANAGER settings;
    std::unique_ptr<SCHEMATIC> schematic;
    KI_TEST::LoadSchematic( settings, "netlists/hierarchy_aliases/hierarchy_aliases", schematic );
    schematic->RebuildConnectivity();
    const wxString name = wxS( "/S0.BOOT.SDA" );
    auto expected = NAVIGATION_QUERY( *schematic ).NetItems( name, false );
    const auto root = schematic->Hierarchy().front();
    BOOST_REQUIRE_EQUAL( expected.erase( root ), 1u );
    BOOST_REQUIRE( !expected.empty() );
    root.LastScreen()->BumpConnectivityRevision();
    const NAVIGATION_QUERY query( *schematic );
    BOOST_CHECK( query.NetItems( name, false ) == expected );
    const auto names = query.NetNames();
    BOOST_CHECK( std::ranges::find( names, name ) != names.end() );
}

BOOST_AUTO_TEST_CASE( SharedScreenGlobalNetKeepsBothSheetRows )
{
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, false );
    SETTINGS_MANAGER settings;
    std::unique_ptr<SCHEMATIC> schematic;
    KI_TEST::LoadSchematic( settings, "NoConnectOnLineWithGlobalLabel", schematic );
    auto* original = schematic->GetTopLevelSheet();
    auto copy = std::unique_ptr<SCH_SHEET>( static_cast<SCH_SHEET*>( original->Clone() ) );
    const_cast<KIID&>( copy->m_Uuid ) = KIID();
    copy->SetName( wxS( "Second" ) );
    schematic->AddTopLevelSheet( copy.release() );
    const auto paths = schematic->Hierarchy();
    BOOST_REQUIRE_EQUAL( paths.size(), 2u );
    BOOST_REQUIRE( paths[0].LastScreen() == paths[1].LastScreen() );
    SCH_ITEM* label = nullptr;

    for( SCH_ITEM* item : original->GetScreen()->Items().OfType( SCH_GLOBAL_LABEL_T ) )
    {
        if( static_cast<SCH_GLOBALLABEL*>( item )->GetText() == wxS( "test_OK" ) )
            label = item;
    }

    BOOST_REQUIRE( label );

    for( bool backend : { false, true } )
    {
        enabled = backend;
        schematic->RebuildConnectivity();
        const auto name = label->GetConnectionName( &paths.front() );
        BOOST_REQUIRE( name );
        const auto result = NAVIGATION_QUERY( *schematic ).NetItems( *name, false );
        BOOST_REQUIRE_EQUAL( result.size(), 2u );
        BOOST_REQUIRE( result.contains( paths[0] ) );
        BOOST_REQUIRE( result.contains( paths[1] ) );
        BOOST_CHECK( result.at( paths[0] ) == result.at( paths[1] ) );
        const auto& items = result.at( paths[0] );
        BOOST_CHECK( std::ranges::find( items, label ) != items.end() );
    }
}

BOOST_AUTO_TEST_CASE( BusHighlightAndCrossProbeResolveMembersWithoutLegacyRows )
{
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, false );
    SETTINGS_MANAGER settings;
    std::unique_ptr<SCHEMATIC> schematic;
    KI_TEST::LoadSchematic( settings, "netlists/hierarchy_aliases/hierarchy_aliases", schematic );
    schematic->RebuildConnectivity();
    const auto root = schematic->Hierarchy().front();
    SCH_ITEM* busLabel = nullptr;

    for( SCH_ITEM* item : root.LastScreen()->Items().OfType( SCH_LABEL_T ) )
    {
        if( static_cast<SCH_LABEL*>( item )->GetText() == wxS( "S0{ALIAS1}" ) )
            busLabel = item;
    }

    BOOST_REQUIRE( busLabel );
    const auto busName = busLabel->GetConnectionName( &root );
    BOOST_REQUIRE( busName );
    const std::vector<wxString> expectedSignals{ wxS( "/S0.BOOT.SCL" ), wxS( "/S0.BOOT.SDA" ) };
    const NAVIGATION_QUERY legacyQuery( *schematic );
    BOOST_CHECK( legacyQuery.SignalNames( *busName ) == expectedSignals );
    const auto expectedItems = legacyQuery.NetItems( *busName, true, true );
    const auto busItems = legacyQuery.NetItems( *busName, true );
    BOOST_REQUIRE( expectedItems != busItems );
    enabled = true;
    schematic->ConnectionGraph()->Reset();
    schematic->RebuildConnectivity();
    const NAVIGATION_QUERY query( *schematic );
    BOOST_CHECK( query.SignalNames( *busName ) == expectedSignals );
    BOOST_CHECK( query.NetItems( *busName, true, true ) == expectedItems );
    BOOST_CHECK( query.SignalNames( expectedSignals.front() )
                 == std::vector<wxString>{ expectedSignals.front() } );
}

BOOST_AUTO_TEST_CASE( DisplayingSharedScreenReferencesPreservesPublishedConnectivity )
{
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, true );
    SETTINGS_MANAGER settings;
    std::unique_ptr<SCHEMATIC> schematic;
    KI_TEST::LoadSchematic( settings, "net_chains_four_nets_labeled", schematic );
    auto* original = schematic->GetTopLevelSheet();
    auto copy = std::unique_ptr<SCH_SHEET>( static_cast<SCH_SHEET*>( original->Clone() ) );
    const_cast<KIID&>( copy->m_Uuid ) = KIID();
    copy->SetName( wxS( "Second" ) );
    schematic->AddTopLevelSheet( copy.release() );
    const auto paths = schematic->Hierarchy();
    BOOST_REQUIRE_EQUAL( paths.size(), 2u );

    for( SCH_ITEM* item : original->GetScreen()->Items().OfType( SCH_SYMBOL_T ) )
    {
        auto* symbol = static_cast<SCH_SYMBOL*>( item );
        symbol->SetRef( &paths[1], symbol->GetRef( &paths[0] ) + wxS( "0" ) );
    }

    schematic->RebuildConnectivity();
    const auto names = NAVIGATION_QUERY( *schematic ).NetNames();
    BOOST_REQUIRE( !names.empty() );
    std::map<wxString, NET_ITEMS_BY_SHEET> expected;

    for( const wxString& name : names )
        expected.emplace( name, NAVIGATION_QUERY( *schematic ).NetItems( name, false ) );

    for( const auto& path : paths )
    {
        schematic->SetCurrentSheet( path );
        path.UpdateAllScreenReferences();
        const NAVIGATION_QUERY query( *schematic );
        BOOST_CHECK( query.NetNames() == names );

        for( const auto& [name, items] : expected )
            BOOST_CHECK( query.NetItems( name, false ) == items );
    }
}

BOOST_AUTO_TEST_CASE( NetclassAssignmentCandidatesUsePublishedNets )
{
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, enabled );

    for( const wxString& fixture : { wxString( "net_chains_four_nets_labeled" ),
                                    wxString( "netlists/hierarchy_aliases/hierarchy_aliases" ) } )
    {
        enabled = false;
        SETTINGS_MANAGER settings;
        std::unique_ptr<SCHEMATIC> schematic;
        KI_TEST::LoadSchematic( settings, fixture, schematic );
        SCH_SCREEN* screen = schematic->Hierarchy().front().LastScreen();
        SCH_LINE* wire = nullptr;
        SCH_LABEL* label = nullptr;

        for( SCH_ITEM* item : screen->Items().OfType( SCH_LINE_T ) )
        {
            if( static_cast<SCH_LINE*>( item )->IsWire() )
            {
                wire = static_cast<SCH_LINE*>( item->Clone() );
                break;
            }
        }

        for( SCH_ITEM* item : screen->Items().OfType( SCH_LABEL_T ) )
        {
            label = static_cast<SCH_LABEL*>( item->Clone() );
            break;
        }

        BOOST_REQUIRE( wire );
        BOOST_REQUIRE( label );
        const_cast<KIID&>( wire->m_Uuid ) = KIID();
        const_cast<KIID&>( label->m_Uuid ) = KIID();

        // Far from the fixture geometry so the wire stays undriven and the vector label drives only itself
        wire->Move( VECTOR2I( 0, 10000000 ) );
        label->Move( VECTOR2I( 0, 20000000 ) );
        label->SetText( wxString( "CANDIDATE_BUS[0..1]" ) );
        screen->Append( wire );
        screen->Append( label );
        schematic->RebuildConnectivity();
        const auto expected = schematic->GetNetClassAssignmentCandidates();
        BOOST_REQUIRE( !expected.empty() );

        if( fixture == wxString( "net_chains_four_nets_labeled" ) )
            BOOST_CHECK( expected.contains( wxString( "/SIG" ) ) );

        enabled = true;
        schematic->RebuildConnectivity();
        schematic->ConnectionGraph()->Reset();
        const auto actual = schematic->GetNetClassAssignmentCandidates();
        BOOST_CHECK( actual == expected );

        size_t buses = 0;

        for( const auto& group : schematic->Connectivity().GetNetMap() )
        {
            if( !group.instances.empty() && group.instances.front().IsBus() )
            {
                BOOST_CHECK( !actual.contains( group.name ) );
                ++buses;
            }
        }

        if( fixture == wxString( "netlists/hierarchy_aliases/hierarchy_aliases" ) )
            BOOST_CHECK_GT( buses, 0u );
    }
}

BOOST_AUTO_TEST_SUITE_END()
