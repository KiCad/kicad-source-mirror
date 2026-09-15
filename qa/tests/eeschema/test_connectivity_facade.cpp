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
#include <connectivity/conn_facade.h>
#include <schematic_utils/schematic_file_util.h>
#include <schematic.h>
#include <sch_screen.h>
#include <sch_commit.h>
#include <sch_sheet.h>
#include <sch_symbol.h>
#include <bus_alias.h>
#include <sch_label.h>
#include <sch_line.h>
#include <sch_bus_entry.h>
#include <sch_pin.h>
#include <sch_netchain.h>
#include <connectivity/conn_netchain_manager.h>
#include <gal/color4d.h>
#include <sch_rule_area.h>
#include <project/project_file.h>
#include <project/net_settings.h>
#include <settings/settings_manager.h>
#include <algorithm>
#include <advanced_config.h>
#include <connection_graph.h>
#include <tool/tool_manager.h>
#include <scoped_set_reset.h>

using namespace SCH_CONNECTIVITY;

BOOST_AUTO_TEST_SUITE( ConnectivityFacade )

BOOST_AUTO_TEST_CASE( NativeSheetRenameEscapesPublishedNetPrefixes )
{
    SETTINGS_MANAGER settings;
    std::unique_ptr<SCHEMATIC> schematic;
    KI_TEST::LoadSchematic( settings, "netlists/multinetclasses/multinetclasses", schematic );
    auto* original = schematic->GetTopLevelSheet();
    original->SetName( "Power" );
    auto copy = std::unique_ptr<SCH_SHEET>( static_cast<SCH_SHEET*>( original->Clone() ) );
    const_cast<KIID&>( copy->m_Uuid ) = KIID();
    copy->SetName( "Other" );
    schematic->AddTopLevelSheet( copy.release() );
    const auto paths = schematic->Hierarchy();
    BOOST_REQUIRE_EQUAL( paths.size(), 2u );
    SCH_LABEL* label = nullptr;

    for( SCH_ITEM* item : original->GetScreen()->Items().OfType( SCH_LABEL_T ) )
    {
        auto* candidate = static_cast<SCH_LABEL*>( item );

        if( candidate->GetText() == wxString( "NET_1" ) )
            label = candidate;
    }

    BOOST_REQUIRE( label );
    auto& facade = schematic->Connectivity();

    auto netSettings = schematic->Project().GetProjectFile().NetSettings();
    facade.Update( *schematic );
    facade.ApplyNetclasses( *netSettings );
    const auto originalAssignments = netSettings->GetNetclassLabelAssignments();
    const std::set<wxString> expectedClasses{ wxString( "CLASS2" ), wxString( "CLASS3" ) };
    BOOST_REQUIRE( originalAssignments.at( wxString( "/Power/NET_1" ) ) == expectedClasses );
    BOOST_REQUIRE( originalAssignments.at( wxString( "/Other/NET_1" ) ) == expectedClasses );
    const KIGFX::COLOR4D powerColor( 1.0, 0.0, 0.0, 1.0 );
    const KIGFX::COLOR4D otherColor( 0.0, 0.0, 1.0, 1.0 );
    netSettings->SetNetColorAssignment( wxString( "/Power/NET_1" ), powerColor );
    netSettings->SetNetColorAssignment( wxString( "/Other/NET_1" ), otherColor );
    SCH_SHEET_PATH topLevel;
    topLevel.push_back( original );

    for( const auto& [name, prefix] : std::vector<std::pair<wxString, wxString>>{
                 { "Power", "/Power/" }, { "Power/Supply", "/Power{slash}Supply/" },
                 { "Power", "/Power/" } } )
    {
        const wxString oldPrefix = topLevel.PathHumanReadable( true, false, true );
        original->SetName( name );
        const wxString newPrefix = topLevel.PathHumanReadable( true, false, true );
        BOOST_CHECK_EQUAL( newPrefix, prefix );
        netSettings->RenameNetPathPrefix( oldPrefix, newPrefix );
        facade.Update( *schematic );
        facade.ApplyNetclasses( *netSettings );
        std::map<wxString, std::set<wxString>> expectedAssignments;

        for( const auto& [net, classes] : originalAssignments )
        {
            expectedAssignments[net.StartsWith( "/Power/" ) ? prefix + net.Mid( 7 ) : net] = classes;
        }

        const std::map<wxString, KIGFX::COLOR4D> expectedColors{
            { prefix + "NET_1", powerColor }, { "/Other/NET_1", otherColor }
        };
        BOOST_CHECK( netSettings->GetNetclassLabelAssignments() == expectedAssignments );
        BOOST_CHECK( netSettings->GetNetColorAssignments() == expectedColors );

        for( const auto& path : paths )
        {
            const wxString expected = path.Last() == original ? prefix + "NET_1" : wxString( "/Other/NET_1" );
            const auto view = facade.Connection( label->m_Uuid, path.Path() );
            BOOST_REQUIRE( view );
            BOOST_CHECK_EQUAL( view->Name(), expected );
        }

        const auto incremental = facade.Published().Rows();
        facade.Update( *schematic, true );
        facade.ApplyNetclasses( *netSettings );
        BOOST_CHECK( facade.Published().Rows() == incremental );
        BOOST_CHECK( netSettings->GetNetclassLabelAssignments() == expectedAssignments );
        BOOST_CHECK( netSettings->GetNetColorAssignments() == expectedColors );
    }
}

BOOST_AUTO_TEST_CASE( NativeTopLevelRootsRetainLocalAndGlobalScope )
{
    for( const wxString& fixture : { wxString( "issue7203" ), wxString( "issue22938/issue22938" ) } )
    {
        BOOST_TEST_CONTEXT( fixture )
        {
            SETTINGS_MANAGER settings;
            std::unique_ptr<SCHEMATIC> schematic;
            KI_TEST::LoadSchematic( settings, fixture, schematic );
            SCH_SHEET* original = schematic->GetTopLevelSheet();
            original->SetName( "First" );
            auto copy = std::unique_ptr<SCH_SHEET>( static_cast<SCH_SHEET*>( original->Clone() ) );
            const_cast<KIID&>( copy->m_Uuid ) = KIID();
            copy->SetName( "Second" );
            schematic->AddTopLevelSheet( copy.release() );
            std::vector<SCH_SHEET_PATH> roots;

            for( const SCH_SHEET_PATH& path : schematic->Hierarchy() )
            {
                if( path.size() == 1 )
                    roots.push_back( path );
            }

            BOOST_REQUIRE_EQUAL( roots.size(), 2 );
            BOOST_REQUIRE( roots[0].LastScreen() == roots[1].LastScreen() );
            SCH_SCREEN* screen = original->GetScreen();
            SCH_ITEM* local = nullptr;
            SCH_ITEM* global = nullptr;

            for( SCH_ITEM* item : screen->Items() )
            {
                if( item->Type() == SCH_LABEL_T )
                    local = item;
                else if( item->Type() == SCH_GLOBAL_LABEL_T )
                    global = item;
                else if( item->Type() == SCH_SYMBOL_T )
                {
                    auto* symbol = static_cast<SCH_SYMBOL*>( item );
                    const wxString value = symbol->GetField( FIELD_T::VALUE )->GetText();

                    if( symbol->IsLocalPower() && value == wxString( "+48V" ) )
                        local = symbol;
                    else if( symbol->IsGlobalPower() && value == wxString( "+12V" ) )
                        global = symbol;
                }
            }

            BOOST_REQUIRE( local );
            BOOST_REQUIRE( global );
            const auto text = []( SCH_ITEM* item ) -> EDA_TEXT&
            {
                if( item->Type() == SCH_SYMBOL_T )
                    return *static_cast<SCH_SYMBOL*>( item )->GetField( FIELD_T::VALUE );

                return *static_cast<SCH_LABEL_BASE*>( item );
            };
            const wxString localName = text( local ).GetText();
            const wxString globalName = text( global ).GetText();
            auto& facade = schematic->Connectivity();
            const auto check = [&]
            {
                facade.Update( *schematic );
                const auto row = [&]( SCH_ITEM* item, const SCH_SHEET_PATH& path ) -> const ITEM_RESULT&
                {
                    if( item->Type() == SCH_SYMBOL_T )
                    {
                        const auto pins = static_cast<SCH_SYMBOL*>( item )->GetPins( &path );
                        BOOST_REQUIRE_EQUAL( pins.size(), 1 );
                        item = pins.front();
                    }

                    const auto instance = facade.Keys().FindInstance( path.Path() );
                    BOOST_REQUIRE( instance );
                    return facade.Published().Rows().at( { item->m_Uuid, *instance } );
                };
                const auto& firstLocal = row( local, roots[0] );
                const auto& secondLocal = row( local, roots[1] );
                const auto& firstGlobal = row( global, roots[0] );
                const auto& secondGlobal = row( global, roots[1] );
                BOOST_CHECK( firstLocal.component != secondLocal.component );
                BOOST_CHECK( firstGlobal.component == secondGlobal.component );
                BOOST_CHECK( firstLocal.component != firstGlobal.component );
                BOOST_CHECK( secondLocal.component != secondGlobal.component );

                for( const SCH_SHEET_PATH& path : roots )
                {
                    BOOST_CHECK_EQUAL( facade.Keys().Name( row( local, path ).name ),
                                       "/" + path.Last()->GetName() + "/" + text( local ).GetText() );
                    BOOST_CHECK_EQUAL( facade.Keys().Name( row( global, path ).name ), text( global ).GetText() );
                }

                const auto incremental = facade.Published().Rows();
                facade.Update( *schematic, true );
                BOOST_CHECK( facade.Published().Rows() == incremental );
            };
            check();
            text( local ).SetText( "LOCAL_SCOPE_EDITED" );
            screen->Update( local, false );
            check();
            text( global ).SetText( "GLOBAL_SCOPE_EDITED" );
            screen->Update( global, false );
            check();
            text( local ).SetText( localName );
            text( global ).SetText( globalName );
            screen->Update( local, false );
            screen->Update( global, false );
            check();
        }
    }
}

BOOST_AUTO_TEST_CASE( AutomaticUpdateRefreshesUntrackedProjectVariables )
{
    SETTINGS_MANAGER settings;
    std::unique_ptr<SCHEMATIC> schematic;
    KI_TEST::LoadSchematic( settings, "issue7203", schematic );
    const auto paths = schematic->Hierarchy();
    SCH_SCREEN& screen = *paths.front().LastScreen();
    auto labels = screen.Items().OfType( SCH_LABEL_T );
    BOOST_REQUIRE( labels.begin() != labels.end() );
    auto* label = static_cast<SCH_LABEL_BASE*>( *labels.begin() );
    label->SetText( "${CACHE_NAME}" );
    label->SetConnectivityDirty();
    schematic->Project().GetTextVars()["CACHE_NAME"] = "FIRST";
    auto& facade = schematic->Connectivity();
    facade.Update( paths, 1, {} );
    auto view = facade.Connection( label->m_Uuid, paths.front().Path() );
    BOOST_REQUIRE( view );
    BOOST_CHECK_EQUAL( view->Name( true ), wxString( "FIRST" ) );
    schematic->Project().GetTextVars()["CACHE_NAME"] = "SECOND";
    facade.Update( *schematic );
    BOOST_CHECK_EQUAL( view->Name( true ), wxString( "SECOND" ) );
}

BOOST_AUTO_TEST_CASE( FailedUpdateClearsRetainedViews )
{
    SETTINGS_MANAGER settings;
    std::unique_ptr<SCHEMATIC> schematic;
    KI_TEST::LoadSchematic( settings, "issue7203", schematic );
    auto& facade = schematic->Connectivity();
    auto paths = schematic->Hierarchy();
    facade.Update( paths, 1, {} );
    BOOST_REQUIRE( !facade.Published().Rows().empty() );
    const auto key = facade.Published().Rows().begin()->first;
    const auto path = facade.Keys().Instance( key.inst );
    auto view = facade.Connection( key.item, path );
    BOOST_REQUIRE( view );
    paths.push_back( paths.front() );
    BOOST_CHECK_THROW( facade.Update( paths, 1, {} ), std::invalid_argument );
    BOOST_CHECK( facade.Published().Rows().empty() );
    BOOST_CHECK( !facade.Connection( key.item, path ) );
    BOOST_CHECK( view->Name().empty() );
    facade.Update( schematic->Hierarchy(), 1, {} );
    BOOST_CHECK( facade.Connection( key.item, path ) );
}

BOOST_AUTO_TEST_CASE( SharedScreensAndCrossInstanceDrivers )
{
    SETTINGS_MANAGER settings;
    std::unique_ptr<SCHEMATIC> schematic;
    KI_TEST::LoadSchematic( settings, "netlists/hierarchy_aliases/hierarchy_aliases", schematic );
    auto& facade = schematic->Connectivity();
    const auto paths = schematic->Hierarchy();
    BUS_ALIASES aliases;

    for( const auto& alias : schematic->GetAllBusAliases() )
        aliases.insert_or_assign( alias->GetName(), alias->Members() );

    facade.Update( paths, 1, aliases );
    std::map<SCH_SCREEN*, size_t> instances;

    for( const auto& path : paths )
        ++instances[path.LastScreen()];

    const auto shared = std::find_if( instances.begin(), instances.end(),
                                     []( const auto& entry ) { return entry.second > 1; } );
    BOOST_REQUIRE( shared != instances.end() );
    SCH_SCREEN* dirty = shared->first;
    size_t cleanViews = 0;
    size_t dirtyViews = 0;
    dirty->BumpConnectivityRevision();

    for( const auto& [key, row] : facade.Published().Rows() )
    {
        const auto& instance = facade.Keys().Instance( key.inst );
        const auto path = paths.GetSheetPathByKIIDPath( instance );
        BOOST_REQUIRE( path );
        auto view = facade.Connection( key.item, instance );

        if( path->LastScreen() == dirty )
        {
            BOOST_CHECK( !view );
            ++dirtyViews;
        }
        else
        {
            BOOST_REQUIRE( view );
            const auto expected = row.name == INVALID_ID ? wxString() : facade.Keys().Name( row.name );
            BOOST_CHECK_EQUAL( view->Name(), expected );
            ++cleanViews;
        }
    }

    BOOST_CHECK_GT( dirtyViews, 0u );
    BOOST_CHECK_GT( cleanViews, 0u );
    facade.Update( paths, 1, aliases );

    for( const auto& [key, row] : facade.Published().Rows() )
        BOOST_CHECK( facade.Connection( key.item, facade.Keys().Instance( key.inst ) ) );
}

BOOST_AUTO_TEST_CASE( RecalculationSkipsScreenlessSheets )
{
    SETTINGS_MANAGER settings;
    std::unique_ptr<SCHEMATIC> schematic;
    KI_TEST::LoadSchematic( settings, "net_chains_four_nets_labeled", schematic );
    auto sheet = std::unique_ptr<SCH_SHEET>(
            static_cast<SCH_SHEET*>( schematic->GetTopLevelSheet()->Clone() ) );
    const_cast<KIID&>( sheet->m_Uuid ) = KIID();
    sheet->SetScreen( nullptr );
    sheet->SetFileName( "net_chains_four_nets.kicad_sch" );
    sheet->SetParent( schematic->GetTopLevelSheet() );
    schematic->GetTopLevelSheet()->GetScreen()->Append( sheet.release() );
    schematic->RefreshHierarchy();
    auto& facade = schematic->Connectivity();
    BOOST_CHECK_NO_THROW( facade.Recalculate( *schematic, true ) );
    BOOST_CHECK( !facade.Published().Rows().empty() );
    BOOST_CHECK( !schematic->NetChains().GetPotentialNetChains().empty() );
}

BOOST_AUTO_TEST_CASE( RecalculationRestoresNativeDirectiveAttachments )
{
    SETTINGS_MANAGER settings;
    std::unique_ptr<SCHEMATIC> schematic;
    KI_TEST::LoadSchematic( settings, "netlists/multinetclasses/multinetclasses", schematic );
    const auto path = schematic->Hierarchy().front();
    std::map<SCH_DIRECTIVE_LABEL*, std::unordered_set<SCH_RULE_AREA*>> expected;

    for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_DIRECTIVE_LABEL_T ) )
    {
        auto* directive = static_cast<SCH_DIRECTIVE_LABEL*>( item );
        const auto areas = directive->GetConnectedRuleAreas();

        if( !areas.empty() )
        {
            expected.emplace( directive, areas );
            directive->ClearConnectedRuleAreas();
        }
    }

    BOOST_REQUIRE( !expected.empty() );
    auto& facade = schematic->Connectivity();
    facade.Update( *schematic );
    facade.Recalculate( *schematic, false );
    BOOST_REQUIRE( facade.Published().Changes().changedItems.empty() );

    for( const auto& [directive, areas] : expected )
    {
        BOOST_CHECK( directive->GetConnectedRuleAreas() == areas );
        BOOST_CHECK( !directive->IsDangling() );
    }
}

BOOST_AUTO_TEST_CASE( RecalculationRestoresNativeEndpointFlags )
{
    SETTINGS_MANAGER settings;
    std::unique_ptr<SCHEMATIC> schematic;
    KI_TEST::LoadSchematic( settings, "netlists/hierarchy_aliases/hierarchy_aliases", schematic );
    const auto paths = schematic->Hierarchy();
    schematic->ConnectionGraph()->Recalculate( paths, true );
    std::vector<std::tuple<SCH_LINE*, bool, bool>> lines;
    std::vector<std::tuple<SCH_BUS_ENTRY_BASE*, bool, bool>> entries;
    std::set<SCH_SCREEN*> seen;

    for( const auto& path : paths )
    {
        SCH_SCREEN* screen = path.LastScreen();

        if( !seen.insert( screen ).second )
            continue;

        for( SCH_ITEM* item : screen->Items() )
        {
            if( auto* line = dynamic_cast<SCH_LINE*>( item ); line && line->IsConnectable() )
            {
                lines.emplace_back( line, line->IsStartDangling(), line->IsEndDangling() );
                line->SetDanglingState( !line->IsStartDangling(), !line->IsEndDangling() );
            }
            else if( auto* entry = dynamic_cast<SCH_BUS_ENTRY_BASE*>( item ) )
            {
                entries.emplace_back( entry, entry->IsStartDangling(), entry->IsEndDangling() );
                entry->SetDanglingState( !entry->IsStartDangling(), !entry->IsEndDangling() );
            }
        }
    }

    BOOST_REQUIRE( !lines.empty() );
    BOOST_REQUIRE( !entries.empty() );
    schematic->Connectivity().Recalculate( *schematic );

    for( const auto& [line, start, end] : lines )
    {
        BOOST_CHECK_EQUAL( line->IsStartDangling(), start );
        BOOST_CHECK_EQUAL( line->IsEndDangling(), end );
    }

    for( const auto& [entry, start, end] : entries )
    {
        BOOST_CHECK_EQUAL( entry->IsStartDangling(), start );
        BOOST_CHECK_EQUAL( entry->IsEndDangling(), end );
    }
}

BOOST_AUTO_TEST_CASE( RecalculationUsesCurrentInstanceForSharedPinFlags )
{
    SETTINGS_MANAGER settings;
    std::unique_ptr<SCHEMATIC> schematic;
    KI_TEST::LoadSchematic( settings, "legacy_hierarchy/legacy_hierarchy", schematic );
    const auto paths = schematic->BuildSheetListSortedByPageNumbers();
    std::map<SCH_SCREEN*, std::vector<SCH_SHEET_PATH>> instances;

    for( const auto& path : paths )
        instances[path.LastScreen()].push_back( path );

    SCH_SYMBOL* symbol = nullptr;
    std::vector<SCH_SHEET_PATH> shared;

    for( const auto& [screen, candidates] : instances )
    {
        if( candidates.size() < 2 )
            continue;

        for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
        {
            auto* candidate = static_cast<SCH_SYMBOL*>( item );

            if( candidate->GetUnitCount() > 1 )
            {
                symbol = candidate;
                shared = candidates;
                break;
            }
        }

        if( symbol )
            break;
    }

    BOOST_REQUIRE( symbol );
    symbol->SetUnitSelection( &shared.front(), 1 );
    symbol->SetUnitSelection( &shared.back(), 2 );

    for( size_t selected : { size_t( 0 ), shared.size() - 1, size_t( 0 ) } )
    {
        const auto& path = shared[selected];
        const int unit = symbol->GetUnitSelection( &path );
        schematic->SetCurrentSheet( path );
        path.UpdateAllScreenReferences();
        schematic->ConnectionGraph()->Recalculate( paths, true );
        SCH_PIN* pin = nullptr;

        for( SCH_PIN* candidate : symbol->GetPins( &path ) )
        {
            if( candidate->GetLibPin()->GetUnit() == unit
                && candidate->GetType() != ELECTRICAL_PINTYPE::PT_NC
                && candidate->GetType() != ELECTRICAL_PINTYPE::PT_NIC )
            {
                pin = candidate;
                break;
            }
        }

        BOOST_REQUIRE( pin );
        const bool dangling = pin->IsDangling();
        pin->SetIsDangling( !dangling );
        BOOST_REQUIRE( pin->IsDangling() != dangling );
        schematic->Connectivity().Recalculate( *schematic );
        BOOST_CHECK_EQUAL( pin->IsDangling(), dangling );
    }
}

BOOST_AUTO_TEST_CASE( NativePassthroughCommitRebuildsNetChains )
{
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, false );
    SETTINGS_MANAGER settings;
    std::unique_ptr<SCHEMATIC> schematic;
    KI_TEST::LoadSchematic( settings, "net_chains_four_nets_labeled", schematic );
    enabled = true;
    schematic->RebuildConnectivity();
    const auto path = schematic->Hierarchy().front();
    SCH_SYMBOL* bridge = nullptr;

    for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
    {
        auto* symbol = static_cast<SCH_SYMBOL*>( item );

        if( symbol->GetRef( &path ) == wxS( "R2" ) )
            bridge = symbol;
    }

    BOOST_REQUIRE( bridge );
    const auto longestChain = [&]()
    {
        size_t nets = 0;

        for( const auto& chain : schematic->NetChains().GetPotentialNetChains() )
            nets = std::max( nets, chain->GetNets().size() );

        return nets;
    };
    const auto originalLength = longestChain();
    BOOST_REQUIRE_EQUAL( originalLength, 4 );
    TOOL_MANAGER manager;
    manager.SetEnvironment( schematic.get(), nullptr, nullptr, nullptr, nullptr );
    SCH_COMMIT commit( &manager );
    commit.Modify( bridge, path.LastScreen() );
    bridge->SetPassthroughMode( SCH_SYMBOL::PASSTHROUGH_MODE::BLOCK );
    commit.Push( "Block net chain", SKIP_UNDO );
    BOOST_CHECK_EQUAL( longestChain(), 2u );
    schematic->RebuildConnectivity();
    BOOST_CHECK_EQUAL( longestChain(), 2u );
    SCH_COMMIT restoreCommit( &manager );
    restoreCommit.Modify( bridge, path.LastScreen() );
    bridge->SetPassthroughMode( SCH_SYMBOL::PASSTHROUGH_MODE::DEFAULT );
    restoreCommit.Push( "Restore net chain", SKIP_UNDO );
    BOOST_CHECK_EQUAL( longestChain(), originalLength );
}

BOOST_AUTO_TEST_SUITE_END()
