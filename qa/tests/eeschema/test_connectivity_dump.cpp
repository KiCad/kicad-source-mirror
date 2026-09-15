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
#include <boost/test/data/test_case.hpp>
#include <schematic_utils/schematic_file_util.h>

#include <connectivity/conn_dump.h>
#include <connectivity/conn_facade.h>
#include <project/project_file.h>
#include <project/net_settings.h>
#include <connection_graph.h>
#include <advanced_config.h>
#include <sch_commit.h>
#include <sch_label.h>
#include <sch_line.h>
#include <sch_pin.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_symbol.h>
#include <schematic.h>
#include <settings/settings_manager.h>
#include <tool/tool_manager.h>

#include <json_common.h>
#include <algorithm>
#include <map>
#include <set>
#include <scoped_set_reset.h>


namespace
{
// Keep this exhaustive corpus small because every item edit compares two full rebuilds
// Native fixtures from the disabled entries in test_incremental_netlister.cpp's RemoveAddItems
const char* const ORACLE_FIXTURES[] = {
    "incremental_test", "issue10430", "issue10926_1", "issue11926", "issue12505", "issue12814",
    "issue13112", "issue13162", "issue13212", "issue13431", "issue13591", "issue16223", "issue6588", "issue9367"
};

const char* const PARITY_FIXTURES[] = {
    "incremental_test", "issue10430", "issue10926_1", "issue11926", "issue12505", "issue12814",
    "issue13112", "issue13162", "issue13212", "issue13431", "issue13591", "issue16223", "issue6588", "issue9367",
    "issue18346", "unconnected_bus_entry_qa", "netlist_exporter_unit_metadata_per_unit",
    "netlists/multinetclasses/multinetclasses", "issue22286/bugtest", "issue7203", "netlists/jumpers/jumpers",
    "issue23058/issue23058", "netlists/bus_junctions/bus_junctions",
    "netlists/prefix_bus_alias/prefix_bus_alias",
    "issue9673/issue9673", "erc_directive_label_not_connected", "netlists/legacy_power/legacy_power",
    "same_local_global_power", "issue1768/issue1768", "issue18092/issue18092", "issue22864/Test_Move_Grid",
    "issue23840/BusAndVectors"
};

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
}


struct CONNECTIVITY_DUMP_FIXTURE
{
    SETTINGS_MANAGER           settings;
    std::unique_ptr<SCHEMATIC> schematic;
};


BOOST_FIXTURE_TEST_SUITE( ConnectivityDump, CONNECTIVITY_DUMP_FIXTURE )

BOOST_AUTO_TEST_CASE( ShadowDumpResolvesClassesWithoutLegacyAssignments )
{
    KI_TEST::LoadSchematic( settings, "netlists/multinetclasses/multinetclasses", schematic );
    auto& facade = schematic->Connectivity();
    facade.Update( *schematic );
    const auto legacy = SCH_CONNECTIVITY::Dump( *schematic );
    const auto engine = SCH_CONNECTIVITY::Dump( *schematic, facade );
    auto classes = schematic->Project().GetProjectFile().NetSettings();
    classes->SetNetclassLabelAssignment( "/NET_1", { "CORRUPTED" } );
    classes->ClearCacheForNet( "/NET_1" );

    // Parity would be vacuous if the engine Dump read the cached assignments legacy rows use
    BOOST_REQUIRE_NE( SCH_CONNECTIVITY::Dump( *schematic ), legacy );
    BOOST_CHECK_EQUAL( SCH_CONNECTIVITY::Dump( *schematic, facade ), engine );
}

BOOST_DATA_TEST_CASE_F( CONNECTIVITY_DUMP_FIXTURE,
                       LegacyAndEngineDumpsMatch,
                       boost::unit_test::data::make( PARITY_FIXTURES ), fixture )
{
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, false );

    BOOST_TEST_CONTEXT( fixture )
    {
        KI_TEST::LoadSchematic( settings, fixture, schematic );
        auto& facade = schematic->Connectivity();
        facade.Update( *schematic );
        auto legacy = nlohmann::json::parse( SCH_CONNECTIVITY::Dump( *schematic ) );
        auto engine = nlohmann::json::parse( SCH_CONNECTIVITY::Dump( *schematic, facade ) );
        EraseDrivers( legacy );
        EraseDrivers( engine );
        BOOST_CHECK_MESSAGE( legacy == engine, nlohmann::json::diff( legacy, engine ).dump( 2 ) );
    }
}

BOOST_AUTO_TEST_CASE( NativeBusNamesPreserveMembership )
{
    using namespace SCH_CONNECTIVITY;
    using KEY = std::pair<std::string, std::string>;
    KI_TEST::LoadSchematic( settings, "issue9673/issue9673", schematic );
    auto& facade = schematic->Connectivity();
    facade.Update( *schematic );
    const auto& keys = facade.Keys();
    const auto legacy = nlohmann::json::parse( SCH_CONNECTIVITY::Dump( *schematic ) );
    const auto shadow = nlohmann::json::parse( SCH_CONNECTIVITY::Dump( *schematic, facade ) );
    std::map<KEY, std::set<KEY>> legacyBuses;
    std::map<int, std::set<KEY>> legacyNets;
    std::map<KEY, const nlohmann::json*> shadowRows;

    for( const auto& row : shadow )
        shadowRows.emplace( KEY{ row.at( "path" ), row.at( "item" ) }, &row );

    for( const auto& row : legacy )
    {
        const KEY key{ row.at( "path" ), row.at( "item" ) };
        const auto path = schematic->Hierarchy().GetSheetPathByKIIDPath(
                KIID_PATH( wxString::FromUTF8( key.first ) ) );
        BOOST_REQUIRE( path );
        const auto* item = path->LastScreen()->GetConnectivityItem( KIID( key.second ) );
        BOOST_REQUIRE( item );
        const auto* connection = item->Connection( &*path );

        if( !connection || !connection->Driver() )
            continue;

        if( connection->IsBus() )
            legacyBuses[KEY{ row.at( "driver_path" ), row.at( "driver" ) }].insert( key );
        else if( connection->IsNet() )
        {
            BOOST_REQUIRE_GT( connection->NetCode(), 0 );
            legacyNets[connection->NetCode()].insert( key );
            const auto after = shadowRows.find( key );
            BOOST_REQUIRE( after != shadowRows.end() );
            BOOST_CHECK( row.at( "name" ) == after->second->at( "name" ) );
            BOOST_CHECK( row.at( "type" ) == after->second->at( "type" ) );
        }
    }

    const auto itemKeys = [&]( const auto& items )
    {
        std::set<KEY> result;

        for( const ITEM_KEY& item : items )
            result.emplace( keys.Instance( item.inst ).AsString().ToStdString(),
                            item.item.AsString().ToStdString() );

        return result;
    };
    const auto checkFields = []( const auto& before, const auto& after )
    {
        for( const char* field : { "name", "local_name", "type", "netclasses" } )
            BOOST_CHECK_MESSAGE( before.at( field ) == after.at( field ), field );
    };
    const auto checkMembers = [&]( const auto& self, const auto& before, const auto& after ) -> void
    {
        BOOST_REQUIRE_EQUAL( before.size(), after.size() );

        for( size_t i = 0; i < before.size(); ++i )
        {
            checkFields( before[i], after[i] );
            self( self, before[i].at( "members" ), after[i].at( "members" ) );
        }
    };

    for( const char* name : { "/{FOO BAR HAM EGGS}", "/{MIXED_BUS}", "/A[0..4]", "/D[0..7]" } )
    {
        const auto row = std::find_if( legacy.begin(), legacy.end(),
                                      [&]( const auto& value ) { return value.at( "name" ) == name; } );
        BOOST_REQUIRE( row != legacy.end() );
        const auto path = schematic->Hierarchy().GetSheetPathByKIIDPath(
                KIID_PATH( wxString::FromUTF8( row->at( "path" ).get<std::string>() ) ) );
        BOOST_REQUIRE( path );
        const KIID itemId( row->at( "item" ).get<std::string>() );
        const auto instance = keys.FindInstance( path->Path() );
        BOOST_REQUIRE( instance );
        BOOST_TEST_CONTEXT( name )
        {
            const auto result = facade.Published().Rows().find( ITEM_KEY{ itemId, *instance } );
            BOOST_REQUIRE( result != facade.Published().Rows().end() );
            const auto& component = facade.Published().Components().at( result->second.component );
            BOOST_REQUIRE( component.name != INVALID_ID );
            const auto actual = itemKeys( component.content->items );
            const auto& expectedItems = legacyBuses.at( KEY{ row->at( "driver_path" ), row->at( "driver" ) } );
            BOOST_CHECK_MESSAGE( actual == expectedItems, nlohmann::json( actual ).dump()
                                  << " expected " << nlohmann::json( expectedItems ).dump() );
            BOOST_CHECK_EQUAL( keys.Name( component.name ).ToStdString( wxConvUTF8 ), name );

            for( const auto& before : legacy )
            {
                const KEY key{ before.at( "path" ), before.at( "item" ) };

                if( !expectedItems.contains( key ) )
                    continue;

                const auto after = shadowRows.find( key );
                BOOST_REQUIRE( after != shadowRows.end() );
                BOOST_REQUIRE( !before.at( "members" ).empty() );
                checkFields( before, *after->second );
                checkMembers( checkMembers, before.at( "members" ), after->second->at( "members" ) );
            }
        }
    }

    for( const auto& [code, items] : legacyNets )
    {
        const KEY& first = *items.begin();
        const auto instance = keys.FindInstance( KIID_PATH( wxString::FromUTF8( first.first ) ) );
        BOOST_REQUIRE( instance );
        BOOST_TEST_CONTEXT( code )
        {
            const auto row = facade.Published().Rows().find( ITEM_KEY{ KIID( first.second ), *instance } );
            BOOST_REQUIRE( row != facade.Published().Rows().end() );
            const auto& component = facade.Published().Components().at( row->second.component );
            BOOST_CHECK( itemKeys( component.content->items ) == items );
        }
    }

    BOOST_CHECK_GT( legacyNets.size(), 0u );
}

BOOST_AUTO_TEST_CASE( NativeMemberPriorityPreservesScalarPartitions )
{
    using namespace SCH_CONNECTIVITY;
    using KEY = std::pair<std::string, std::string>;

    for( const char* fixture : { "netlists/hierarchy_aliases/hierarchy_aliases",
                                 "netlists/multinetclasses/multinetclasses" } )
    {
        BOOST_TEST_CONTEXT( fixture )
        {
            KI_TEST::LoadSchematic( settings, fixture, schematic );
            auto& facade = schematic->Connectivity();
            facade.Update( *schematic );
            const auto& keys = facade.Keys();
            const auto legacy = nlohmann::json::parse( SCH_CONNECTIVITY::Dump( *schematic ) );
            const auto shadow = nlohmann::json::parse( SCH_CONNECTIVITY::Dump( *schematic, facade ) );
            BOOST_REQUIRE_EQUAL( legacy.size(), shadow.size() );

            for( size_t i = 0; i < legacy.size(); ++i )
            {
                BOOST_REQUIRE( legacy[i].at( "path" ) == shadow[i].at( "path" ) );
                BOOST_REQUIRE( legacy[i].at( "item" ) == shadow[i].at( "item" ) );
            }

            const auto paths = schematic->Hierarchy();
            std::map<int, std::set<KEY>> nets;
            std::map<int, std::string> names;

            for( const auto& row : legacy )
            {
                const KEY key{ row.at( "path" ), row.at( "item" ) };
                const auto path = paths.GetSheetPathByKIIDPath( KIID_PATH( wxString::FromUTF8( key.first ) ) );
                BOOST_REQUIRE( path );
                const auto* item = path->LastScreen()->GetConnectivityItem( KIID( key.second ) );
                BOOST_REQUIRE( item );
                const auto* connection = item->Connection( &*path );

                if( connection && connection->IsNet() && connection->Driver() )
                {
                    BOOST_REQUIRE_GT( connection->NetCode(), 0 );
                    nets[connection->NetCode()].insert( key );
                    names[connection->NetCode()] = connection->Name().ToStdString( wxConvUTF8 );
                }
            }

            for( const auto& [code, items] : nets )
            {
                const auto& first = *items.begin();
                const auto instance = keys.FindInstance( KIID_PATH( wxString::FromUTF8( first.first ) ) );
                BOOST_REQUIRE( instance );
                const auto row = facade.Published().Rows().find( ITEM_KEY{ KIID( first.second ), *instance } );
                BOOST_REQUIRE( row != facade.Published().Rows().end() );
                const auto& component = facade.Published().Components().at( row->second.component );
                std::set<KEY> actual;

                for( const ITEM_KEY& item : component.content->items )
                    actual.emplace( keys.Instance( item.inst ).AsString().ToStdString(),
                                    item.item.AsString().ToStdString() );

                BOOST_CHECK_MESSAGE( actual == items, "Net code " << code );
                BOOST_REQUIRE( component.name != INVALID_ID );
                BOOST_CHECK_EQUAL( keys.Name( component.name ).ToStdString( wxConvUTF8 ), names.at( code ) );
            }
        }
    }
}

BOOST_AUTO_TEST_CASE( HeadlessCommitPublishesCompletedCleanup )
{
    auto& config = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() );
    SCOPED_SET_RESET restoreEngine( config.m_ConnectivityEngine, config.m_ConnectivityEngine );

    for( bool engine : { false, true } )
    {
        BOOST_TEST_CONTEXT( "new engine " << engine )
        {
            config.m_ConnectivityEngine = engine;
            KI_TEST::LoadSchematic( settings, "issue12505", schematic );
            schematic->RebuildConnectivity();
            SCH_SCREEN* screen = schematic->GetCurrentScreen();
            const KIID junction( "2934c2df-d750-4949-9bb5-2d5234f61386" );
            auto* wire = dynamic_cast<SCH_LINE*>(
                    screen->GetConnectivityItem( KIID( "06af46a6-6f60-4f17-95ae-2a85a89f02a6" ) ) );
            auto* upper = dynamic_cast<SCH_LINE*>(
                    screen->GetConnectivityItem( KIID( "ac6a6b32-a43c-4495-b132-b2f68491aae2" ) ) );
            auto* lower = dynamic_cast<SCH_LINE*>(
                    screen->GetConnectivityItem( KIID( "f145de0f-02ed-4631-85ea-7126ccf81633" ) ) );
            BOOST_REQUIRE( wire && upper && lower );
            SCH_ITEM* dot = screen->GetConnectivityItem( junction );
            BOOST_REQUIRE( dot );
            const VECTOR2I position = dot->GetPosition();
            BOOST_REQUIRE( wire->IsEndPoint( position ) && upper->IsEndPoint( position )
                           && lower->IsEndPoint( position ) );
            BOOST_REQUIRE_EQUAL( screen->GetBusesAndWires( position, false ).size(), 3u );
            const std::set<int> directions{ wire->GetAngleFrom( position ), upper->GetAngleFrom( position ),
                                            lower->GetAngleFrom( position ) };
            BOOST_REQUIRE_EQUAL( directions.size(), 3u );
            const VECTOR2I start = upper->GetStartPoint();
            const VECTOR2I end = lower->GetEndPoint();
            int notifications = 0;
            auto subscription = schematic->Connectivity().Subscribe(
                    [&]( const SCH_CONNECTIVITY::CHANGE_SET& )
                    {
                        ++notifications;
                        BOOST_CHECK( !screen->GetConnectivityItem( junction ) );
                    } );
            TOOL_MANAGER manager;
            manager.SetEnvironment( schematic.get(), nullptr, nullptr, nullptr, nullptr );
            SCH_COMMIT move( &manager );
            schematic->CleanUp( &move, screen );
            BOOST_REQUIRE( move.Empty() );
            BOOST_REQUIRE( screen->GetConnectivityItem( junction ) == dot );
            BOOST_CHECK( dot->GetPosition() == position );
            move.Modify( wire, screen );
            wire->Move( VECTOR2I( 10000000, 10000000 ) );
            BOOST_REQUIRE( !wire->IsEndPoint( position ) );
            BOOST_REQUIRE( upper->IsEndPoint( position ) && lower->IsEndPoint( position ) );
            BOOST_REQUIRE_NE( upper->GetAngleFrom( position ), lower->GetAngleFrom( position ) );
            BOOST_REQUIRE_EQUAL( screen->GetBusesAndWires( position, false ).size(), 2u );
            move.Push( "Disconnect native junction", SKIP_UNDO );
            BOOST_CHECK( !screen->GetConnectivityItem( junction ) );
            BOOST_CHECK_EQUAL( notifications, engine ? 1 : 0 );
            size_t merged = 0;

            for( SCH_ITEM* item : screen->Items() )
            {
                BOOST_CHECK( !item->HasFlag( STRUCT_DELETED ) );

                if( auto* line = dynamic_cast<SCH_LINE*>( item );
                    line && line->IsEndPoint( start ) && line->IsEndPoint( end ) )
                    ++merged;
            }

            BOOST_CHECK_EQUAL( merged, 1u );
            const auto dump = [&]()
            {
                return engine ? SCH_CONNECTIVITY::Dump( *schematic, schematic->Connectivity() )
                              : SCH_CONNECTIVITY::Dump( *schematic );
            };
            const std::string incremental = dump();
            schematic->RebuildConnectivity();
            BOOST_CHECK_EQUAL( incremental, dump() );
        }
    }
}


BOOST_DATA_TEST_CASE_F( CONNECTIVITY_DUMP_FIXTURE,
                       PreviouslyDisabledFixturesMatchFullAndFreshRebuildsAfterCommits,
                       boost::unit_test::data::make( ORACLE_FIXTURES ), fixture )
{
    auto& config = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() );
    SCOPED_SET_RESET restoreEngine( config.m_ConnectivityEngine, config.m_ConnectivityEngine );
    SCOPED_SET_RESET restoreIncremental( config.m_IncrementalConnectivity, config.m_IncrementalConnectivity );
    config.m_ConnectivityEngine = true;
    config.m_IncrementalConnectivity = true;

    KI_TEST::LoadSchematic( settings, fixture, schematic );
    TOOL_MANAGER manager;
    manager.SetEnvironment( schematic.get(), nullptr, nullptr, nullptr, nullptr );
    SCH_COMMIT preparation( &manager );
    schematic->CleanUpConnections( &preparation, GLOBAL_CLEANUP );
    preparation.Push( "Prepare native oracle geometry", SKIP_UNDO | SKIP_CONNECTIVITY | DELETE_REMOVED_ITEMS );
    schematic->RebuildConnectivity();
    SCH_SYMBOL* symbol = nullptr;

    for( const SCH_SHEET_PATH& path : schematic->Hierarchy() )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            auto* candidate = static_cast<SCH_SYMBOL*>( item );

            if( !candidate->IsPower() && !candidate->GetPins( &path ).empty() )
            {
                symbol = candidate;
                break;
            }
        }

        if( symbol )
            break;
    }

    BOOST_REQUIRE( symbol );
    BOOST_REQUIRE( !symbol->GetParentGroup() );
    std::set<SCH_SCREEN*> seenScreens;
    const auto isGeometry = []( const SCH_ITEM* item )
    {
        return item->IsConnectable()
               && ( item->Type() == SCH_LINE_T || item->Type() == SCH_BUS_WIRE_ENTRY_T
                    || item->Type() == SCH_BUS_BUS_ENTRY_T || item->Type() == SCH_JUNCTION_T
                    || item->Type() == SCH_NO_CONNECT_T );
    };
    std::vector<std::pair<SCH_SCREEN*, std::unique_ptr<SCH_ITEM>>> geometry;

    for( const SCH_SHEET_PATH& path : schematic->Hierarchy() )
    {
        if( !seenScreens.insert( path.LastScreen() ).second )
            continue;

        for( SCH_ITEM* item : path.LastScreen()->Items() )
        {
            if( isGeometry( item ) )
            {
                BOOST_REQUIRE( !item->GetParentGroup() );
                geometry.emplace_back( path.LastScreen(), static_cast<SCH_ITEM*>( item->Clone() ) );
            }
        }
    }

    std::sort( geometry.begin(), geometry.end(),
               []( const auto& a, const auto& b ) { return a.second->m_Uuid < b.second->m_Uuid; } );
    const VECTOR2I offset( 10000000, 10000000 );
    SCH_CONNECTIVITY::FACADE rebuilt;
    rebuilt.Update( *schematic, true );
    const auto verify = [&]()
    {
        for( SCH_SCREEN* screen : seenScreens )
        {
            for( SCH_ITEM* item : screen->Items() )
            {
                BOOST_REQUIRE_MESSAGE( !item->HasFlag( STRUCT_DELETED ),
                                       "Cleanup left a deleted item on the screen: "
                                               << item->m_Uuid.AsString().ToStdString() );
            }
        }

        const std::string incremental = SCH_CONNECTIVITY::Dump( *schematic, schematic->Connectivity() );
        rebuilt.Update( *schematic, true );
        BOOST_CHECK_EQUAL( incremental, SCH_CONNECTIVITY::Dump( *schematic, rebuilt ) );
        // A separate fresh engine also detects stale content-keyed caches retained by a full rebuild
        SCH_CONNECTIVITY::FACADE fresh;
        fresh.Update( *schematic, true );
        BOOST_CHECK_EQUAL( incremental, SCH_CONNECTIVITY::Dump( *schematic, fresh ) );
        std::map<int, int> toFresh;
        std::map<int, int> fromFresh;
        const auto& current = schematic->Connectivity();

        for( const auto& [key, row] : current.Published().Rows() )
        {
            const auto& path = current.Keys().Instance( key.inst );
            const auto actual = current.Connection( key.item, path );
            const auto full = rebuilt.Connection( key.item, path );
            const auto empty = fresh.Connection( key.item, path );
            BOOST_REQUIRE( actual && full && empty );
            BOOST_CHECK_EQUAL( actual->NetCode(), full->NetCode() );
            BOOST_CHECK_EQUAL( actual->NetCode() == 0, empty->NetCode() == 0 );
            const auto forward = toFresh.emplace( actual->NetCode(), empty->NetCode() ).first;
            const auto reverse = fromFresh.emplace( empty->NetCode(), actual->NetCode() ).first;
            BOOST_CHECK_EQUAL( forward->second, empty->NetCode() );
            BOOST_CHECK_EQUAL( reverse->second, actual->NetCode() );
        }
    };

    const std::string original = SCH_CONNECTIVITY::Dump( *schematic, schematic->Connectivity() );
    const auto restoreGeometry = [&]()
    {
        // A wire edit can remove junctions and merge other wires. Restore all native geometry
        // through a commit, preserving engine history while recovering the exact source state
        SCH_COMMIT restore( &manager );
        std::vector<SCH_ITEM*> removed;
        std::vector<std::pair<SCH_SCREEN*, std::unique_ptr<SCH_ITEM>>> added;

        for( SCH_SCREEN* screen : seenScreens )
        {
            for( SCH_ITEM* item : screen->Items() )
            {
                if( isGeometry( item ) )
                {
                    restore.Remove( item, screen );
                    removed.push_back( item );
                }
            }
        }

        for( const auto& [screen, source] : geometry )
        {
            auto copy = std::unique_ptr<SCH_ITEM>( static_cast<SCH_ITEM*>( source->Clone() ) );
            BOOST_REQUIRE( copy && copy->m_Uuid == source->m_Uuid );
            restore.Add( copy.get(), screen );
            added.emplace_back( screen, std::move( copy ) );
        }

        restore.Push( "Restore native oracle geometry", SKIP_UNDO );

        for( auto& [screen, item] : added )
        {
            BOOST_REQUIRE( screen->CheckIfOnDrawList( item.get() ) );
            item.release();
        }

        for( SCH_ITEM* item : removed )
            delete item;

        verify();
        BOOST_CHECK_EQUAL( original, SCH_CONNECTIVITY::Dump( *schematic, schematic->Connectivity() ) );
    };

    for( const VECTOR2I& movement : { offset, VECTOR2I( -offset.x, -offset.y ) } )
    {
        SCH_SCREEN* screen = symbol->GetParentScreen();
        SCH_COMMIT commit( &manager );
        commit.Modify( symbol, screen );
        symbol->Move( movement );
        commit.Push( "Move native oracle symbol", SKIP_UNDO );
        BOOST_CHECK( !schematic->Connectivity().Published().Changes().Empty() );
        verify();
    }

    restoreGeometry();
    SCH_SCREEN* symbolScreen = symbol->GetParentScreen();
    // Restore the same item and pin identities after destroying the original object
    auto symbolReplacement = std::make_unique<SCH_SYMBOL>( *symbol );
    SCH_COMMIT removal( &manager );
    removal.Remove( symbol, symbolScreen );
    removal.Push( "Remove native oracle symbol", SKIP_UNDO );
    BOOST_REQUIRE( !symbolScreen->CheckIfOnDrawList( symbol ) );
    delete symbol;
    verify();
    BOOST_CHECK_NE( original, SCH_CONNECTIVITY::Dump( *schematic, schematic->Connectivity() ) );

    SCH_COMMIT addition( &manager );
    addition.Add( symbolReplacement.get(), symbolScreen );
    addition.Push( "Restore native oracle symbol", SKIP_UNDO );
    BOOST_REQUIRE( symbolScreen->CheckIfOnDrawList( symbolReplacement.get() ) );
    symbolReplacement.release();
    verify();
    restoreGeometry();

    std::vector<SCH_LABEL_BASE*> labels;

    for( SCH_SCREEN* screen : seenScreens )
    {
        for( SCH_ITEM* item : screen->Items() )
        {
            if( item->Type() == SCH_LABEL_T || item->Type() == SCH_GLOBAL_LABEL_T
                || item->Type() == SCH_HIER_LABEL_T )
                labels.push_back( static_cast<SCH_LABEL_BASE*>( item ) );
        }
    }

    std::sort( labels.begin(), labels.end(),
               []( const auto* a, const auto* b ) { return a->m_Uuid < b->m_Uuid; } );

    for( SCH_LABEL_BASE* label : labels )
    {
        const wxString originalText = label->GetText();

        for( const wxString& text : { wxString( "ORACLE_" ) + originalText, originalText } )
        {
            SCH_COMMIT rename( &manager );
            rename.Modify( label, label->GetParentScreen() );
            label->SetText( text );
            rename.Push( "Rename native oracle label", SKIP_UNDO );
            verify();
        }
    }

    for( const auto& [screen, source] : geometry )
    {
        const KIID& id = source->m_Uuid;

        BOOST_TEST_CONTEXT( "geometry item " << id.AsString().ToStdString() )
        {
            SCH_ITEM* item = screen->GetConnectivityItem( id );
            BOOST_REQUIRE( item );
            BOOST_REQUIRE( !item->GetParentGroup() );
            const VECTOR2I expectedPosition = item->GetPosition() + offset;
            SCH_COMMIT move( &manager );
            move.Modify( item, screen );
            item->Move( offset );
            BOOST_CHECK( item->GetPosition() == expectedPosition );
            move.Push( "Move native oracle geometry", SKIP_UNDO );
            BOOST_CHECK( !schematic->Connectivity().Published().Changes().Empty() );
            verify();
            restoreGeometry();

            item = screen->GetConnectivityItem( id );
            BOOST_REQUIRE( item );
            SCH_COMMIT removal( &manager );
            removal.Remove( item, screen );
            removal.Push( "Remove native oracle geometry", SKIP_UNDO );
            BOOST_REQUIRE( !screen->CheckIfOnDrawList( item ) );
            delete item;

            verify();
            BOOST_CHECK_NE( original, SCH_CONNECTIVITY::Dump( *schematic, schematic->Connectivity() ) );

            restoreGeometry();
        }
    }

    BOOST_TEST_MESSAGE( fixture << ": checked " << labels.size() << " native label renames and restores, and "
                               << geometry.size() << " native geometry move/remove/restore sequences" );
    const std::string incremental = SCH_CONNECTIVITY::Dump( *schematic, schematic->Connectivity() );
    BOOST_CHECK_EQUAL( original, incremental );
    schematic->RebuildConnectivity();
    BOOST_CHECK_EQUAL( incremental, SCH_CONNECTIVITY::Dump( *schematic, schematic->Connectivity() ) );
}


BOOST_DATA_TEST_CASE_F( CONNECTIVITY_DUMP_FIXTURE, NativeLabelBatchMatchesFullRebuildAndRestores,
                       boost::unit_test::data::make( { "issue7203", "netlists/hierarchy_aliases/hierarchy_aliases" } ),
                       fixture )
{
    KI_TEST::LoadSchematic( settings, fixture, schematic );
    const auto hierarchy = schematic->Hierarchy();
    SCH_SCREEN* screen = hierarchy.front().LastScreen();

    for( const auto& path : hierarchy )
    {
        SCH_SCREEN* candidate = path.LastScreen();
        auto labels = candidate->Items().OfType( SCH_LABEL_T );
        const auto instances = std::ranges::count_if( hierarchy, [&]( const auto& other )
        {
            return other.LastScreen() == candidate;
        } );

        if( instances > 1 && labels.begin() != labels.end() )
            screen = candidate;
    }

    std::vector<SCH_SHEET_PATH> affected;

    for( const auto& path : hierarchy )
    {
        if( path.LastScreen() == screen )
            affected.push_back( path );
    }

    BOOST_REQUIRE_EQUAL( affected.size(), hierarchy.size() == 1 ? 1u : 2u );
    std::vector<SCH_ITEM*> sources;

    for( SCH_ITEM* item : screen->Items().OfType( SCH_LABEL_T ) )
        sources.push_back( item );

    BOOST_REQUIRE( !sources.empty() );
    std::ranges::sort( sources, []( const SCH_ITEM* a, const SCH_ITEM* b ) { return a->m_Uuid < b->m_Uuid; } );
    SCH_CONNECTIVITY::FACADE incremental;
    incremental.Update( *schematic );
    const std::string baseline = SCH_CONNECTIVITY::Dump( *schematic, incremental );
    struct BATCH
    {
        SCH_SCREEN*                            screen;
        std::vector<std::unique_ptr<SCH_ITEM>> items;

        ~BATCH()
        {
            for( const auto& item : items )
            {
                if( screen->CheckIfOnDrawList( item.get() ) )
                    screen->Remove( item.get(), false );
            }
        }
    } batch{ screen, {} };
    batch.items.reserve( 500 );

    for( size_t i = 0; i < 500; ++i )
    {
        std::unique_ptr<SCH_ITEM> copy( sources[i % sources.size()]->Duplicate( false ) );
        copy->SetParentGroup( nullptr );
        batch.items.push_back( std::move( copy ) );
    }

    SCH_CONNECTIVITY::FACADE rebuilt;

    for( int cycle = 0; cycle < 2; ++cycle )
    {
        for( const auto& item : batch.items )
            screen->Append( item.get(), false );

        incremental.Update( *schematic );

        for( const auto& path : affected )
        {
            for( const auto& item : batch.items )
                BOOST_REQUIRE( incremental.Connection( item->m_Uuid, path.Path() ) );
        }

        const std::string edited = SCH_CONNECTIVITY::Dump( *schematic, incremental );
        BOOST_CHECK_NE( edited, baseline );
        rebuilt.Update( *schematic, true );
        BOOST_CHECK_EQUAL( edited, SCH_CONNECTIVITY::Dump( *schematic, rebuilt ) );

        for( const auto& item : batch.items )
            BOOST_REQUIRE( screen->Remove( item.get(), false ) );

        incremental.Update( *schematic );
        BOOST_CHECK_EQUAL( baseline, SCH_CONNECTIVITY::Dump( *schematic, incremental ) );

        for( const auto& path : affected )
        {
            for( const auto& item : batch.items )
                BOOST_CHECK( !incremental.Connection( item->m_Uuid, path.Path() ) );
        }

        rebuilt.Update( *schematic, true );
        BOOST_CHECK_EQUAL( baseline, SCH_CONNECTIVITY::Dump( *schematic, rebuilt ) );
    }
}

BOOST_AUTO_TEST_SUITE_END()
