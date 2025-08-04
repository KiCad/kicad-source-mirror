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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <boost/test/unit_test.hpp>

#include <hotkey_store.h>
#include <hotkeys_basic.h>
#include <tool/tool_action.h>
#include <tool/tool_event.h>

BOOST_AUTO_TEST_SUITE( HotkeyStore )

BOOST_AUTO_TEST_CASE( PersistenceAndDefaults )
{
    TOOL_ACTION action1( TOOL_ACTION_ARGS().Name( "common.test1" ).FriendlyName( "Test1" )
                         .Scope( AS_GLOBAL ).DefaultHotkey( 'A' ) );
    TOOL_ACTION action2( TOOL_ACTION_ARGS().Name( "common.test2" ).FriendlyName( "Test2" )
                         .Scope( AS_GLOBAL ).DefaultHotkey( 'B' ) );

    HOTKEY_STORE store;
    store.Init( { &action1, &action2 }, false );

    auto& sections = store.GetSections();
    BOOST_REQUIRE( !sections.empty() );

    HOTKEY& hk = sections[0].m_HotKeys[0];
    hk.m_EditKeycode = 'C';
    store.SaveAllHotkeys();
    BOOST_CHECK_EQUAL( action1.GetHotKey(), 'C' );

    store.ResetAllHotkeysToDefault();
    BOOST_CHECK_EQUAL( sections[0].m_HotKeys[0].m_EditKeycode, action1.GetDefaultHotKey() );
}

BOOST_AUTO_TEST_CASE( DuplicateRegistration )
{
    TOOL_ACTION action1( TOOL_ACTION_ARGS().Name( "common.test1" ).FriendlyName( "Test1" )
                         .Scope( AS_GLOBAL ).DefaultHotkey( 'A' ) );
    TOOL_ACTION action2( TOOL_ACTION_ARGS().Name( "common.test2" ).FriendlyName( "Test2" )
                         .Scope( AS_GLOBAL ).DefaultHotkey( 'B' ) );

    HOTKEY_STORE store;
    store.Init( { &action1, &action2 }, false );

    HOTKEY* conflict = nullptr;
    bool has = store.CheckKeyConflicts( &action1, action2.GetHotKey(), &conflict );
    BOOST_CHECK( has );
    BOOST_CHECK( conflict != nullptr );

    has = store.CheckKeyConflicts( &action1, 'Z', &conflict );
    BOOST_CHECK( !has );
}

BOOST_AUTO_TEST_CASE( KeycodeSerialization )
{
    int code = MD_CTRL + 'M';
    wxString name = KeyNameFromKeyCode( code );
    int round = KeyCodeFromKeyName( name );
    BOOST_CHECK_EQUAL( code, round );
}

BOOST_AUTO_TEST_SUITE_END()

