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

#include <boost/test/unit_test.hpp>
#include <tool/tool_manager.h>
#include <sch_commit.h>
#include <sch_group.h>
#include <sch_text.h>
#include <sch_line.h>
#include <sch_screen.h>
#include <sch_label.h>
#include <sch_symbol.h>
#include <sch_pin.h>
#include <sch_sheet_pin.h>
#include <schematic.h>
#include <connection_graph.h>
#include <advanced_config.h>
#include <connectivity/conn_dump.h>
#include <connectivity/conn_facade.h>
#include <schematic_utils/schematic_file_util.h>
#include <settings/settings_manager.h>
#include <scoped_set_reset.h>

BOOST_AUTO_TEST_SUITE( SchCommit )

BOOST_AUTO_TEST_CASE( RecursesThroughGroups )
{
    TOOL_MANAGER mgr;
    SCH_COMMIT commit( &mgr );

    SCH_TEXT t1;
    SCH_TEXT t2;
    SCH_GROUP group;
    group.AddItem( &t1 );
    group.AddItem( &t2 );

    commit.Stage( &group, CHT_MODIFY, nullptr, RECURSE_MODE::RECURSE );

    BOOST_CHECK_EQUAL( commit.GetStatus( &t1 ), CHT_MODIFY );
    BOOST_CHECK_EQUAL( commit.GetStatus( &t2 ), CHT_MODIFY );
}

BOOST_AUTO_TEST_CASE( ClearsSelectedByDragFlag )
{
    TOOL_MANAGER mgr;
    SCH_COMMIT commit( &mgr );

    SCH_TEXT text;
    text.SetFlags( SELECTED_BY_DRAG );
    text.SetSelected();

    commit.Stage( &text, CHT_MODIFY );

    BOOST_CHECK( text.IsSelected() );
    BOOST_CHECK_EQUAL( commit.GetStatus( &text ), CHT_MODIFY );
}

BOOST_AUTO_TEST_CASE( CommitCanDeferConnectivityUntilExplicitRebuild )
{
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, enabled );

    for( bool useEngine : { false, true } )
    {
        BOOST_TEST_CONTEXT( "engine=" << useEngine )
        {
            enabled = false;
            SETTINGS_MANAGER settings;
            std::unique_ptr<SCHEMATIC> schematic;
            KI_TEST::LoadSchematic( settings, "netlists/multinetclasses/multinetclasses", schematic );
            const SCH_SHEET_PATH path = schematic->Hierarchy().front();
            SCH_SCREEN* screen = path.LastScreen();
            SCH_LABEL* label = nullptr;

            for( SCH_ITEM* item : screen->Items().OfType( SCH_LABEL_T ) )
            {
                auto* candidate = static_cast<SCH_LABEL*>( item );

                if( candidate->GetText() == "NET_2" )
                    label = candidate;
            }

            BOOST_REQUIRE( label );
            schematic->ConnectionGraph()->Reset();
            schematic->Connectivity().Clear();
            enabled = useEngine;
            TOOL_MANAGER manager;
            manager.SetEnvironment( schematic.get(), nullptr, nullptr, nullptr, nullptr );
            SCH_COMMIT commit( &manager );
            commit.Modify( label, screen );
            label->SetText( "DEFERRED_CONNECTIVITY" );
            commit.Push( "Rename label", SKIP_UNDO | SKIP_CONNECTIVITY );
            BOOST_CHECK( commit.Empty() );
            BOOST_CHECK_EQUAL( label->GetText(), "DEFERRED_CONNECTIVITY" );
            BOOST_CHECK( schematic->ConnectionGraph()->GetNetMap().empty() );
            schematic->RecalculateConnections( nullptr, GLOBAL_CLEANUP, &manager );
            const auto name = label->GetConnectionName( &path );
            BOOST_REQUIRE( name );
            BOOST_CHECK_EQUAL( *name, "/DEFERRED_CONNECTIVITY" );
        }
    }
}

BOOST_AUTO_TEST_CASE( RevertingCleanupRestoresMergedWireBeforeItsMove )
{
    SETTINGS_MANAGER settings;
    std::unique_ptr<SCHEMATIC> schematic;
    KI_TEST::LoadSchematic( settings, "issue12505", schematic );
    SCH_SCREEN* screen = schematic->GetCurrentScreen();
    auto* wire = dynamic_cast<SCH_LINE*>(
            screen->GetConnectivityItem( KIID( "8d2c5f9b-5bc9-4d6f-9ddd-e2fce9531195" ) ) );
    auto* other = dynamic_cast<SCH_LINE*>(
            screen->GetConnectivityItem( KIID( "06af46a6-6f60-4f17-95ae-2a85a89f02a6" ) ) );
    BOOST_REQUIRE( wire && other );
    const VECTOR2I start = wire->GetStartPoint();
    const VECTOR2I end = wire->GetEndPoint();
    TOOL_MANAGER manager;
    manager.SetEnvironment( schematic.get(), nullptr, nullptr, nullptr, nullptr );
    SCH_COMMIT move( &manager );
    move.Modify( wire, screen );
    wire->Move( other->GetStartPoint() - start );
    screen->Update( wire );
    schematic->CleanUp( &move, screen );
    BOOST_REQUIRE( !screen->CheckIfOnDrawList( wire ) );

    move.Revert();
    BOOST_CHECK( screen->CheckIfOnDrawList( wire ) );
    BOOST_CHECK( wire->GetStartPoint() == start );
    BOOST_CHECK( wire->GetEndPoint() == end );
    BOOST_CHECK( !wire->HasFlag( STRUCT_DELETED ) );
    BOOST_CHECK( screen->GetConnectivityItem( wire->m_Uuid ) == wire );
}

BOOST_AUTO_TEST_CASE( HeadlessCommitsRetainDirtyScreensAfterCleanup )
{
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, enabled );

    for( const auto& [backend, skipDirty] : { std::pair{ false, false }, std::pair{ true, false },
                                             std::pair{ false, true }, std::pair{ true, true } } )
    {
        BOOST_TEST_CONTEXT( "engine=" << backend << ", skip dirty=" << skipDirty )
        {
            enabled = backend;
            SETTINGS_MANAGER settings;
            std::unique_ptr<SCHEMATIC> schematic;
            KI_TEST::LoadSchematic( settings, "net_chains_four_nets", schematic );
            SCH_SCREEN* screen = schematic->Hierarchy().front().LastScreen();
            auto* wire = new SCH_LINE( VECTOR2I( -10000000, -10000000 ), LAYER_WIRE );
            wire->SetEndPoint( VECTOR2I( -9000000, -10000000 ) );
            screen->Append( wire );
            schematic->RebuildConnectivity();
            TOOL_MANAGER manager;
            manager.SetEnvironment( schematic.get(), nullptr, nullptr, nullptr, nullptr );
            screen->SetContentModified( false );
            SCH_COMMIT commit( &manager );
            commit.Modify( wire, screen );
            wire->Move( VECTOR2I( 100000, 0 ) );
            commit.Push( "Move wire", SKIP_UNDO | ( skipDirty ? SKIP_SET_DIRTY : 0 ) );
            BOOST_CHECK_EQUAL( screen->IsContentModified(), !skipDirty );
        }
    }
}

BOOST_AUTO_TEST_CASE( NoOpWireCommitRestoresConnectivityFlags )
{
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, enabled );

    for( bool useEngine : { false, true } )
    {
        BOOST_TEST_CONTEXT( "engine=" << useEngine )
        {
            enabled = useEngine;
            SETTINGS_MANAGER           settings;
            std::unique_ptr<SCHEMATIC> schematic;
            KI_TEST::LoadSchematic( settings, "net_chains_four_nets", schematic );
            schematic->RebuildConnectivity();
            SCH_SCREEN* screen = schematic->Hierarchy().front().LastScreen();
            SCH_LINE*   wire = nullptr;

            for( SCH_ITEM* item : screen->Items().OfType( SCH_LINE_T ) )
            {
                auto* line = static_cast<SCH_LINE*>( item );

                if( line->IsWire() && !line->IsDangling() )
                {
                    wire = line;
                    break;
                }
            }

            BOOST_REQUIRE( wire );
            const auto start = wire->GetStartPoint();
            const auto end = wire->GetEndPoint();
            const auto dump = [&]()
            {
                return useEngine ? SCH_CONNECTIVITY::Dump( *schematic, schematic->Connectivity() )
                                 : SCH_CONNECTIVITY::Dump( *schematic );
            };
            const auto   before = dump();
            TOOL_MANAGER manager;
            manager.SetEnvironment( schematic.get(), nullptr, nullptr, nullptr, nullptr );
            SCH_COMMIT unchanged( &manager );
            unchanged.Modify( wire, screen );
            unchanged.Push( "Edit Wire", SKIP_UNDO );
            BOOST_CHECK_EQUAL( dump(), before );

            SCH_COMMIT commit( &manager );
            commit.Modify( wire, screen );
            // Mirrors SCH_MOVE_TOOL's preview and final update of the selected item
            wire->SetDanglingState( true, true );
            wire->SetConnectivityDirty();
            commit.Push( "Move", SKIP_UNDO );
            BOOST_CHECK( wire->GetStartPoint() == start );
            BOOST_CHECK( wire->GetEndPoint() == end );
            BOOST_CHECK( !wire->IsConnectivityDirty() );
            BOOST_CHECK( !wire->IsStartDangling() );
            BOOST_CHECK( !wire->IsEndDangling() );
            BOOST_CHECK_EQUAL( dump(), before );

            SCH_SYMBOL* symbol = nullptr;

            for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
            {
                auto* candidate = static_cast<SCH_SYMBOL*>( item );

                if( !candidate->GetPins().empty() )
                {
                    symbol = candidate;
                    break;
                }
            }

            BOOST_REQUIRE( symbol );
            const auto position = symbol->GetPosition();
            std::vector<bool> dangling;
            SCH_COMMIT symbolCommit( &manager );
            symbolCommit.Modify( symbol, screen );

            for( SCH_PIN* pin : symbol->GetPins() )
            {
                dangling.push_back( pin->IsDangling() );
                pin->SetIsDangling( !pin->IsDangling() );
            }

            symbol->SetConnectivityDirty();
            symbolCommit.Push( "Move", SKIP_UNDO );
            BOOST_CHECK( symbol->GetPosition() == position );
            BOOST_CHECK( !symbol->IsConnectivityDirty() );

            for( size_t i = 0; i < dangling.size(); ++i )
                BOOST_CHECK_EQUAL( symbol->GetPins()[i]->IsDangling(), dangling[i] );

            BOOST_CHECK_EQUAL( dump(), before );

            SCH_COMMIT deferred( &manager );
            deferred.Modify( wire, screen );
            wire->SetDanglingState( true, true );
            wire->SetConnectivityDirty();
            deferred.Push( "Move", SKIP_UNDO | SKIP_CONNECTIVITY );
            BOOST_CHECK( wire->IsConnectivityDirty() );
            BOOST_CHECK( wire->IsStartDangling() );
            BOOST_CHECK( wire->IsEndDangling() );
            schematic->RebuildConnectivity();
            BOOST_CHECK_EQUAL( dump(), before );

            auto graphic = std::unique_ptr<SCH_LINE>( static_cast<SCH_LINE*>( wire->Duplicate( false ) ) );
            graphic->SetLayer( LAYER_NOTES );
            BOOST_REQUIRE( !graphic->IsConnectable() );
            SCH_LINE* drawing = graphic.get();
            screen->Append( graphic.release() );
            schematic->RebuildConnectivity();
            SCH_COMMIT drawingEdit( &manager );
            drawingEdit.Modify( drawing, screen );
            drawingEdit.Push( "Edit Line", SKIP_UNDO );
            BOOST_CHECK_EQUAL( dump(), before );

            SCH_COMMIT drawingRemoval( &manager );
            drawingRemoval.Remove( drawing, screen );
            drawingRemoval.Push( "Delete Line", SKIP_UNDO | DELETE_REMOVED_ITEMS );
            BOOST_CHECK_EQUAL( dump(), before );
        }
    }
}

BOOST_AUTO_TEST_CASE( SheetPinCommitMatchesFullRebuild )
{
    auto& config = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() );
    SCOPED_SET_RESET restoreEngine( config.m_ConnectivityEngine, config.m_ConnectivityEngine );
    SCOPED_SET_RESET restoreIncremental( config.m_IncrementalConnectivity, config.m_IncrementalConnectivity );
    const auto check = [&]( const char* fixture, const wxString& pinText, size_t expectedParentInstances )
    {
        BOOST_TEST_CONTEXT( fixture )
        {
            for( bool useEngine : { false, true } )
            {
                BOOST_TEST_CONTEXT( "engine=" << useEngine )
                {
                    config.m_ConnectivityEngine = useEngine;
                    config.m_IncrementalConnectivity = true;
                    SETTINGS_MANAGER settings;
                    std::unique_ptr<SCHEMATIC> schematic;
                    KI_TEST::LoadSchematic( settings, fixture, schematic );
                    TOOL_MANAGER manager;
                    manager.SetEnvironment( schematic.get(), nullptr, nullptr, nullptr, nullptr );
                    SCH_SCREEN* screen = nullptr;
                    SCH_SHEET_PIN* pin = nullptr;
                    size_t parentInstances = 0;

                    for( const SCH_SHEET_PATH& path : schematic->Hierarchy() )
                    {
                        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SHEET_T ) )
                        {
                            for( SCH_SHEET_PIN* candidate : static_cast<SCH_SHEET*>( item )->GetPins() )
                            {
                                if( candidate->GetText() == pinText )
                                {
                                    if( pin )
                                        BOOST_REQUIRE( candidate == pin );
                                    else
                                        schematic->SetCurrentSheet( path );

                                    pin = candidate;
                                    screen = path.LastScreen();
                                    ++parentInstances;
                                }
                            }
                        }
                    }

                    BOOST_REQUIRE( pin );
                    BOOST_REQUIRE_EQUAL( parentInstances, expectedParentInstances );
                    schematic->RecalculateConnections( nullptr, GLOBAL_CLEANUP, &manager );
                    SCH_SHEET* owner = pin->GetParent();
                    const wxString originalText = pin->GetText();
                    const auto hierarchy = schematic->Hierarchy();
                    const auto dump = [&]()
                    {
                        return useEngine ? SCH_CONNECTIVITY::Dump( *schematic, schematic->Connectivity() )
                                         : SCH_CONNECTIVITY::Dump( *schematic );
                    };
                    const auto baseline = dump();

                    const VECTOR2I originalPosition = pin->GetPosition();
                    bool connectedEndpoint = false;

                    for( SCH_ITEM* item : screen->Items().OfType( SCH_LINE_T ) )
                    {
                        const auto* line = static_cast<SCH_LINE*>( item );

                        if( ( line->IsWire() || line->IsBus() )
                            && ( line->GetStartPoint() == originalPosition
                                 || line->GetEndPoint() == originalPosition ) )
                        {
                            connectedEndpoint = true;
                        }
                    }

                    BOOST_REQUIRE( connectedEndpoint );
                    enum OPERATION { RENAME, MOVE, ADD_PIN };

                    for( OPERATION operation : { RENAME, MOVE, ADD_PIN } )
                    {
                        if( operation == ADD_PIN && ( !useEngine || expectedParentInstances != 1 ) )
                            continue;

                        SCH_SHEET_PIN* addedPin = nullptr;
                        std::unique_ptr<SCH_SHEET_PIN> removedPin;
                        BOOST_TEST_CONTEXT( "operation=" << operation )
                        {
                            for( bool restoring : { false, true } )
                            {
                                BOOST_TEST_CONTEXT( "restoring=" << restoring )
                                {
                                    SCH_COMMIT commit( &manager );
                                    commit.Modify( owner, screen );

                                    if( operation == RENAME )
                                    {
                                        pin->SetText( restoring ? originalText
                                                                : wxString( "UNMATCHED_NATIVE_PORT" ) );
                                    }
                                    else if( operation == MOVE )
                                    {
                                        const VECTOR2I delta( 1000000, 1000000 );
                                        owner->Move( delta * ( restoring ? -1 : 1 ) );
                                        BOOST_CHECK_EQUAL( pin->GetPosition() == originalPosition, restoring );
                                    }
                                    else if( restoring )
                                    {
                                        owner->RemovePin( addedPin );
                                        removedPin.reset();
                                    }
                                    else
                                    {
                                        auto duplicate = std::unique_ptr<SCH_SHEET_PIN>(
                                                static_cast<SCH_SHEET_PIN*>( pin->Duplicate( false ) ) );
                                        BOOST_REQUIRE( duplicate->m_Uuid != pin->m_Uuid );
                                        addedPin = duplicate.get();
                                        owner->AddPin( duplicate.release() );
                                    }

                                    commit.Push( "Edit Sheet Pin Properties", SKIP_UNDO );

                                    BOOST_CHECK( schematic->Hierarchy() == hierarchy );
                                    const auto edited = dump();

                                    if( restoring )
                                        BOOST_CHECK_EQUAL( edited, baseline );
                                    else
                                        BOOST_CHECK( edited != baseline );

                                    schematic->RebuildConnectivity();
                                    BOOST_CHECK_EQUAL( dump(), edited );
                                }
                            }
                        }
                    }
                }
            }
        }
    };
    check( "issue9673/issue9673", wxString( "INT" ), 1 );
    check( "netlists/hierarchy_aliases/hierarchy_aliases", wxString( "{ALIAS2}" ), 2 );
}

BOOST_AUTO_TEST_SUITE_END()
