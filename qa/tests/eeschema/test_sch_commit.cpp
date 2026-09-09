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
#include <schematic.h>
#include <connection_graph.h>
#include <schematic_utils/schematic_file_util.h>
#include <settings/settings_manager.h>

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

BOOST_AUTO_TEST_SUITE_END()

