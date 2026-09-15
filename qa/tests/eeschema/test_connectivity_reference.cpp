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
#include <advanced_config.h>
#include <connectivity/conn_dump.h>
#include <connectivity/conn_facade.h>
#include <json_common.h>
#include <project.h>
#include <project/project_file.h>
#include <project/net_settings.h>
#include <schematic_utils/schematic_file_util.h>
#include <schematic.h>
#include <sch_commit.h>
#include <sch_line.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_sheet_pin.h>
#include <settings/settings_manager.h>
#include <scoped_set_reset.h>
#include <tool/tool_manager.h>

#include <algorithm>
#include <array>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace
{
using NET_NAMES = std::map<std::pair<std::string, std::string>, std::string>;

// Legacy records each island's own driver while the engine records the whole net's winner
void EraseDrivers( nlohmann::json& aValue )
{
    if( aValue.is_object() )
    {
        aValue.erase( "driver" );
        aValue.erase( "driver_path" );
    }

    if( aValue.is_structured() )
    {
        for( auto& child : aValue )
            EraseDrivers( child );
    }
}

std::string UserVisible( const std::string& aDump )
{
    auto rows = nlohmann::json::parse( aDump );
    EraseDrivers( rows );
    return rows.dump( 2 );
}

std::string CommittedDump( SCHEMATIC& aSchematic )
{
    return UserVisible( ADVANCED_CFG::GetCfg().m_ConnectivityEngine
                                ? SCH_CONNECTIVITY::Dump( aSchematic, aSchematic.Connectivity() )
                                : SCH_CONNECTIVITY::Dump( aSchematic ) );
}

// Legacy runs compare against the engine rebuild too, so legacy stays the reference
std::string RebuiltDump( SCHEMATIC& aSchematic )
{
    SCH_CONNECTIVITY::FACADE full;
    full.Update( aSchematic, true );
    return UserVisible( SCH_CONNECTIVITY::Dump( aSchematic, full ) );
}

NET_NAMES NetNames( const std::string& aDump )
{
    NET_NAMES result;

    for( const auto& row : nlohmann::json::parse( aDump ) )
    {
        result.emplace( std::pair{ row.at( "path" ).get<std::string>(), row.at( "item" ).get<std::string>() },
                        row.at( "name" ).get<std::string>() );
    }

    return result;
}

std::string NetName( const NET_NAMES& aNames, const KIID_PATH& aPath, const KIID& aItem )
{
    const auto found = aNames.find( { aPath.AsString().ToStdString( wxConvUTF8 ),
                                      aItem.AsString().ToStdString( wxConvUTF8 ) } );
    BOOST_REQUIRE_MESSAGE( found != aNames.end(),
                           "Missing row " << aPath.AsString() << " " << aItem.AsString() );
    return found->second;
}
} // namespace

BOOST_AUTO_TEST_SUITE( ConnectivityReference )

BOOST_AUTO_TEST_CASE( NativeThreeInstancesPropagateChildShortAndRestore )
{
    auto& config = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() );
    SCOPED_SET_RESET engine( config.m_ConnectivityEngine, config.m_ConnectivityEngine );
    SCOPED_SET_RESET incremental( config.m_IncrementalConnectivity, true );

    for( bool enabled : { false, true } )
    {
        BOOST_TEST_CONTEXT( "new engine " << enabled )
        {
            config.m_ConnectivityEngine = enabled;
            SETTINGS_MANAGER settings;
            std::unique_ptr<SCHEMATIC> schematic;
            KI_TEST::LoadSchematic( settings, "netlists/issue14818/issue14818", schematic );
            const auto hierarchy = schematic->Hierarchy();
            BOOST_REQUIRE_EQUAL( hierarchy.size(), 4u );
            std::vector<SCH_SHEET_PATH> children;

            for( const SCH_SHEET_PATH& path : hierarchy )
            {
                if( path.size() == 2 )
                    children.push_back( path );
            }

            BOOST_REQUIRE_EQUAL( children.size(), 3u );
            std::sort( children.begin(), children.end(), []( const auto& lhs, const auto& rhs )
            {
                return lhs.Last()->GetName() < rhs.Last()->GetName();
            } );
            SCH_SCREEN* screen = children.front().LastScreen();
            SCH_LINE* bridge = nullptr;

            for( SCH_ITEM* item : screen->Items().OfType( SCH_LINE_T ) )
            {
                BOOST_REQUIRE( !bridge );
                bridge = static_cast<SCH_LINE*>( item );
            }

            BOOST_REQUIRE( bridge );
            BOOST_REQUIRE( bridge->IsWire() );
            BOOST_REQUIRE( bridge->m_Uuid == KIID( "af3971fe-9d70-4180-aec7-06849506172b" ) );
            TOOL_MANAGER manager;
            manager.SetEnvironment( schematic.get(), nullptr, nullptr, nullptr, nullptr );
            schematic->SetCurrentSheet( children.front() );
            schematic->RecalculateConnections( nullptr, GLOBAL_CLEANUP, &manager );
            const VECTOR2I originalEnd = bridge->GetEndPoint();
            const KIID_PATH root = hierarchy.front().Path();
            const auto check = [&]( bool joined )
            {
                const std::string committed = CommittedDump( *schematic );
                const NET_NAMES names = NetNames( committed );
                std::array<std::string, 6> parentNets;

                for( size_t index = 0; index < children.size(); ++index )
                {
                    const auto& path = children[index];
                    BOOST_REQUIRE( path.LastScreen() == screen );
                    BOOST_REQUIRE( path.Last()->GetName() == wxString::Format( "Sub%zu", index + 1 ) );
                    BOOST_REQUIRE_EQUAL( path.Last()->GetPins().size(), 2u );
                    std::set<wxString> texts;

                    for( const SCH_SHEET_PIN* pin : path.Last()->GetPins() )
                    {
                        BOOST_REQUIRE( pin->GetText() == wxString( "in" ) || pin->GetText() == wxString( "out" ) );
                        BOOST_REQUIRE( texts.insert( pin->GetText() ).second );
                        const bool input = pin->GetText() == wxString( "in" );
                        const KIID child( input ? "fbac9619-48e4-4bb4-ae58-b737c341dfb7"
                                                : "6b540c7f-cd51-4ae0-9753-e62094939d20" );
                        const std::string parentNet = NetName( names, root, pin->m_Uuid );
                        BOOST_CHECK( !parentNet.empty() );
                        BOOST_CHECK_EQUAL( parentNet, NetName( names, path.Path(), child ) );
                        parentNets[2 * index + !input] = parentNet;
                    }
                }

                for( size_t first = 0; first < parentNets.size(); ++first )
                {
                    for( size_t second = first + 1; second < parentNets.size(); ++second )
                    {
                        const bool parentBridge = ( first == 1 && second == 2 ) || ( first == 3 && second == 4 );
                        BOOST_CHECK_EQUAL( parentNets[first] == parentNets[second], joined || parentBridge );
                    }
                }

                BOOST_CHECK_EQUAL( committed, RebuiltDump( *schematic ) );
                return committed;
            };
            const std::string baseline = check( true );
            const auto edit = [&]( const VECTOR2I& end )
            {
                SCH_COMMIT commit( &manager );
                commit.Modify( bridge, screen );
                bridge->SetEndPoint( end );
                commit.Push( "Move Child Wire Endpoint", SKIP_UNDO );
            };
            edit( originalEnd + VECTOR2I( 0, 1270000 ) );
            BOOST_CHECK( check( false ) != baseline );
            edit( originalEnd );
            BOOST_CHECK_EQUAL( check( true ), baseline );
        }
    }
}


BOOST_AUTO_TEST_CASE( NativeGlobalBridgeCommitsRefreshUnchangedSibling )
{
    auto& config = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() );
    SCOPED_SET_RESET engine( config.m_ConnectivityEngine, config.m_ConnectivityEngine );
    SCOPED_SET_RESET incremental( config.m_IncrementalConnectivity, true );

    for( bool enabled : { false, true } )
    {
        BOOST_TEST_CONTEXT( "new engine " << enabled )
        {
            config.m_ConnectivityEngine = enabled;
            SETTINGS_MANAGER settings;
            std::unique_ptr<SCHEMATIC> schematic;
            KI_TEST::LoadSchematic( settings, "netlists/issue14657/issue14657", schematic );
            auto hierarchy = schematic->Hierarchy();
            BOOST_REQUIRE_EQUAL( hierarchy.size(), 3u );
            std::sort( hierarchy.begin(), hierarchy.end(), []( const auto& lhs, const auto& rhs )
            {
                return lhs.size() != rhs.size() ? lhs.size() < rhs.size()
                                               : lhs.Last()->GetName() < rhs.Last()->GetName();
            } );
            BOOST_REQUIRE_EQUAL( hierarchy[0].size(), 1u );
            BOOST_REQUIRE_EQUAL( hierarchy[1].size(), 2u );
            BOOST_REQUIRE_EQUAL( hierarchy[2].size(), 2u );
            BOOST_REQUIRE_EQUAL( hierarchy[1].Last()->GetName(), wxString( "subsheet1" ) );
            BOOST_REQUIRE_EQUAL( hierarchy[2].Last()->GetName(), wxString( "subsheet2" ) );
            SCH_SCREEN* screen = hierarchy[1].LastScreen();
            BOOST_REQUIRE( screen != hierarchy[0].LastScreen() );
            BOOST_REQUIRE( screen != hierarchy[2].LastScreen() );
            BOOST_REQUIRE( hierarchy[0].LastScreen() != hierarchy[2].LastScreen() );
            SCH_LINE* bridge = nullptr;

            for( SCH_ITEM* item : screen->Items().OfType( SCH_LINE_T ) )
            {
                if( item->m_Uuid == KIID( "7e736d0c-601c-4f96-b4d2-138dfebf5932" ) )
                    bridge = static_cast<SCH_LINE*>( item );
            }

            BOOST_REQUIRE( bridge );
            BOOST_REQUIRE( bridge->IsWire() );
            TOOL_MANAGER manager;
            manager.SetEnvironment( schematic.get(), nullptr, nullptr, nullptr, nullptr );
            schematic->SetCurrentSheet( hierarchy[1] );
            schematic->RecalculateConnections( nullptr, GLOBAL_CLEANUP, &manager );
            const VECTOR2I originalEnd = bridge->GetEndPoint();
            const KIID_PATH root = hierarchy[0].Path();
            const KIID_PATH source = hierarchy[1].Path();
            const KIID_PATH sibling = hierarchy[2].Path();
            const std::vector<std::pair<KIID_PATH, KIID>> recipients{
                { source, KIID( "24390c04-0f13-48b6-ae5a-ba48240c3289" ) },
                { root, KIID( "2c0a5cdb-9772-4ba5-a787-71f4817b1e17" ) },
                { root, KIID( "c87e9573-e811-44b1-99a7-05b54a8ac7d3" ) },
                { root, KIID( "7ea56b40-7e69-4f46-899d-916eb5319378" ) },
                { sibling, KIID( "837954e7-e02c-47dc-b569-f7787b8dce47" ) },
                { sibling, KIID( "59996eb8-69ab-4ef2-9ed0-6da263326213" ) },
                { sibling, KIID( "c836a852-d408-4f5b-9d40-88c91302bb01" ) },
                { sibling, KIID( "6e69c8b0-e3b4-4882-ae1e-33126ecb39b0" ) }
            };
            const auto check = [&]( bool joined )
            {
                const std::string committed = CommittedDump( *schematic );
                const NET_NAMES names = NetNames( committed );
                const std::string expected = joined ? "GPIO1_14__46" : "REG_ENABLE";
                BOOST_CHECK_EQUAL( NetName( names, source, KIID( "ab3b04ea-7320-437f-91d8-0dbd1001f3f9" ) ),
                                   "GPIO1_14__46" );

                for( const auto& [path, item] : recipients )
                    BOOST_CHECK_EQUAL( NetName( names, path, item ), expected );

                BOOST_CHECK_EQUAL( committed, RebuiltDump( *schematic ) );
                return committed;
            };
            const std::string baseline = check( true );
            const auto edit = [&]( const VECTOR2I& end )
            {
                SCH_COMMIT commit( &manager );
                commit.Modify( bridge, screen );
                bridge->SetEndPoint( end );
                commit.Push( "Move Global Bridge Endpoint", SKIP_UNDO );
            };
            edit( originalEnd - VECTOR2I( 1270000, 0 ) );
            BOOST_CHECK( check( false ) != baseline );
            edit( originalEnd );
            BOOST_CHECK_EQUAL( check( true ), baseline );
        }
    }
}


BOOST_AUTO_TEST_CASE( NativeCommittedMoveOutOfRuleAreaRefreshesNetclasses )
{
    auto& config = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() );
    SCOPED_SET_RESET engine( config.m_ConnectivityEngine, config.m_ConnectivityEngine );
    SCOPED_SET_RESET incremental( config.m_IncrementalConnectivity, true );

    for( bool enabled : { false, true } )
    {
        BOOST_TEST_CONTEXT( "new engine " << enabled )
        {
            config.m_ConnectivityEngine = enabled;
            SETTINGS_MANAGER settings;
            std::unique_ptr<SCHEMATIC> schematic;
            KI_TEST::LoadSchematic( settings, "netlists/multinetclasses/multinetclasses", schematic );
            SCH_SCREEN* screen = schematic->RootScreen();
            const SCH_SHEET_PATH path = schematic->Hierarchy().front();
            SCH_ITEM* stationaryArea = screen->GetConnectivityItem( KIID( "6ba20b94-0e46-4b8f-a023-c91a1d3b84ac" ) );
            BOOST_REQUIRE( stationaryArea );
            const VECTOR2I areaPosition = stationaryArea->GetPosition();
            std::vector<SCH_ITEM*> netItems;

            for( const char* uuid : { "47160e4c-50a1-41c1-9f26-94bda63350fa", "f1308710-8c5c-4584-a706-b3735972840b",
                                     "20fe2096-2c56-4c5a-be18-dd8ed4e8c24b", "8598689f-ce4f-414f-a720-b8fa4ecb6b41",
                                     "17b6ce67-6699-41d5-a096-6714796adaa3" } )
            {
                SCH_ITEM* item = screen->GetConnectivityItem( KIID( uuid ) );
                BOOST_REQUIRE( item );
                netItems.push_back( item );
            }

            TOOL_MANAGER manager;
            manager.SetEnvironment( schematic.get(), nullptr, nullptr, nullptr, nullptr );
            schematic->SetCurrentSheet( path );
            schematic->RecalculateConnections( nullptr, GLOBAL_CLEANUP, &manager );
            auto netSettings = schematic->Project().GetProjectFile().NetSettings();
            const auto originalAssignments = netSettings->GetNetclassLabelAssignments();
            auto movedAssignments = originalAssignments;
            BOOST_REQUIRE_EQUAL( movedAssignments.at( wxString( "/NET_3" ) ).erase( wxString( "CLASS3" ) ), 1u );
            BOOST_REQUIRE_EQUAL( movedAssignments.at( wxString( "/NET_3" ) ).erase( wxString( "CLASS4" ) ), 1u );
            BOOST_REQUIRE( movedAssignments.at( wxString( "/NET_3" ) ).contains( wxString( "CLASS_COMPLETE" ) ) );
            const auto move = [&]( const VECTOR2I& offset, const auto& assignments )
            {
                SCH_COMMIT commit( &manager );

                for( SCH_ITEM* item : netItems )
                {
                    commit.Modify( item, screen );
                    item->Move( offset );
                }

                commit.Push( "Move Net Out Of Rule Area", SKIP_UNDO );
                BOOST_CHECK( stationaryArea->GetPosition() == areaPosition );
                BOOST_CHECK( netSettings->GetNetclassLabelAssignments() == assignments );
                SCH_CONNECTIVITY::FACADE full;
                full.Update( *schematic, true );
                BOOST_CHECK_EQUAL( CommittedDump( *schematic ),
                                   UserVisible( SCH_CONNECTIVITY::Dump( *schematic, full ) ) );
                full.ApplyNetclasses( *netSettings );
                BOOST_CHECK( netSettings->GetNetclassLabelAssignments() == assignments );
            };
            move( VECTOR2I( 0, 508000 ), movedAssignments );
            move( VECTOR2I( 0, -508000 ), originalAssignments );
        }
    }
}


BOOST_AUTO_TEST_CASE( NativeCommittedDirectiveMoveReassignsNetclasses )
{
    auto& config = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() );
    SCOPED_SET_RESET engine( config.m_ConnectivityEngine, config.m_ConnectivityEngine );
    SCOPED_SET_RESET incremental( config.m_IncrementalConnectivity, true );

    for( bool enabled : { false, true } )
    {
        BOOST_TEST_CONTEXT( "new engine " << enabled )
        {
            config.m_ConnectivityEngine = enabled;
            SETTINGS_MANAGER settings;
            std::unique_ptr<SCHEMATIC> schematic;
            KI_TEST::LoadSchematic( settings, "netlists/multinetclasses/multinetclasses", schematic );
            SCH_SCREEN* screen = schematic->RootScreen();
            const SCH_SHEET_PATH path = schematic->Hierarchy().front();
            SCH_ITEM* directive = screen->GetConnectivityItem( KIID( "20fe2096-2c56-4c5a-be18-dd8ed4e8c24b" ) );
            SCH_ITEM* target = screen->GetConnectivityItem( KIID( "6145ae74-b637-4a72-a5de-ad08388fb892" ) );
            BOOST_REQUIRE( directive );
            BOOST_REQUIRE( target );
            BOOST_REQUIRE( directive->Type() == SCH_DIRECTIVE_LABEL_T );
            BOOST_REQUIRE( target->Type() == SCH_LABEL_T );
            const VECTOR2I original = directive->GetPosition();
            TOOL_MANAGER manager;
            manager.SetEnvironment( schematic.get(), nullptr, nullptr, nullptr, nullptr );
            schematic->SetCurrentSheet( path );
            schematic->RecalculateConnections( nullptr, GLOBAL_CLEANUP, &manager );
            const NET_NAMES names = NetNames( CommittedDump( *schematic ) );
            const wxString sourceName = wxString::FromUTF8( NetName( names, path.Path(), directive->m_Uuid ) );
            const wxString targetName = wxString::FromUTF8( NetName( names, path.Path(), target->m_Uuid ) );
            BOOST_REQUIRE( !sourceName.IsEmpty() );
            BOOST_REQUIRE( !targetName.IsEmpty() );
            BOOST_REQUIRE( sourceName != targetName );
            auto netSettings = schematic->Project().GetProjectFile().NetSettings();
            const auto originalAssignments = netSettings->GetNetclassLabelAssignments();
            BOOST_REQUIRE( originalAssignments.at( sourceName ).contains( wxString( "CLASS_COMPLETE" ) ) );
            BOOST_REQUIRE( !originalAssignments.at( targetName ).contains( wxString( "CLASS_COMPLETE" ) ) );
            auto movedAssignments = originalAssignments;
            movedAssignments.at( sourceName ).erase( wxString( "CLASS_COMPLETE" ) );
            movedAssignments.at( targetName ).insert( wxString( "CLASS_COMPLETE" ) );
            const auto move = [&]( const VECTOR2I& position, const auto& expected )
            {
                SCH_COMMIT commit( &manager );
                commit.Modify( directive, screen );
                directive->SetPosition( position );
                commit.Push( "Move Netclass Directive", SKIP_UNDO );
                BOOST_CHECK( netSettings->GetNetclassLabelAssignments() == expected );
                BOOST_CHECK_EQUAL( CommittedDump( *schematic ), RebuiltDump( *schematic ) );
            };
            move( target->GetPosition(), movedAssignments );
            move( original, originalAssignments );
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
