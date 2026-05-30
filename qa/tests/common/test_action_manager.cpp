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

#include <vector>

#include <tool/action_manager.h>
#include <tool/selection.h>
#include <tool/selection_conditions.h>
#include <tool/tool_action.h>

BOOST_AUTO_TEST_SUITE( ActionManagerDispatch )

BOOST_AUTO_TEST_CASE( UserBoundDetection_DefaultUntouched )
{
    TOOL_ACTION action(
            TOOL_ACTION_ARGS().Name( "common.test" ).FriendlyName( "Test" ).Scope( AS_GLOBAL ).DefaultHotkey( 'A' ) );

    BOOST_CHECK( !action.IsHotKeyUserBound( 'A' ) );
}

BOOST_AUTO_TEST_CASE( UserBoundDetection_CustomValue )
{
    TOOL_ACTION action(
            TOOL_ACTION_ARGS().Name( "common.test" ).FriendlyName( "Test" ).Scope( AS_GLOBAL ).DefaultHotkey( 'A' ) );

    action.SetHotKey( 'B' );

    BOOST_CHECK( action.IsHotKeyUserBound( 'B' ) );
    BOOST_CHECK( !action.IsHotKeyUserBound( 'A' ) );
}

BOOST_AUTO_TEST_CASE( UserBoundDetection_IdentityRebindIsIndistinguishable )
{
    // Known heuristic limit: explicitly assigning the default value cannot
    // be distinguished from never touching the action. Document via test.
    TOOL_ACTION action(
            TOOL_ACTION_ARGS().Name( "common.test" ).FriendlyName( "Test" ).Scope( AS_GLOBAL ).DefaultHotkey( 'A' ) );

    action.SetHotKey( 'A' );

    BOOST_CHECK( !action.IsHotKeyUserBound( 'A' ) );
}

BOOST_AUTO_TEST_CASE( UserBoundDetection_AltSlotIndependent )
{
    TOOL_ACTION action(
            TOOL_ACTION_ARGS().Name( "common.test" ).FriendlyName( "Test" ).Scope( AS_GLOBAL ).DefaultHotkey( 'A' ) );

    action.SetHotKey( 'A', 'B' );

    BOOST_CHECK( !action.IsHotKeyUserBound( 'A' ) );
    BOOST_CHECK( action.IsHotKeyUserBound( 'B' ) );
}

BOOST_AUTO_TEST_CASE( UserBoundDetection_SharedDefaultCollisionPreserved )
{
    // common.* vs frame.* both shipped with the same default key (e.g. 'D'
    // for common.showDatasheet and pcbnew.Drag45Degree). Neither is
    // user-bound, so promotion must not fire and original ordering stays.
    TOOL_ACTION commonAction( TOOL_ACTION_ARGS()
                                      .Name( "common.showDatasheet" )
                                      .FriendlyName( "Show Datasheet" )
                                      .Scope( AS_GLOBAL )
                                      .DefaultHotkey( 'D' ) );
    TOOL_ACTION frameAction( TOOL_ACTION_ARGS()
                                     .Name( "pcbnew.dragMove45" )
                                     .FriendlyName( "Drag 45" )
                                     .Scope( AS_GLOBAL )
                                     .DefaultHotkey( 'D' ) );

    BOOST_CHECK( !commonAction.IsHotKeyUserBound( 'D' ) );
    BOOST_CHECK( !frameAction.IsHotKeyUserBound( 'D' ) );
}

BOOST_AUTO_TEST_CASE( Promotion_UserBoundFrameActionWins )
{
    TOOL_ACTION commonCopy( TOOL_ACTION_ARGS()
                                    .Name( "common.Interactive.copy" )
                                    .FriendlyName( "Copy" )
                                    .Scope( AS_GLOBAL )
                                    .DefaultHotkey( 'C' ) );
    TOOL_ACTION frameCopy( TOOL_ACTION_ARGS()
                                   .Name( "pcbnew.InteractiveMove.copyWithReference" )
                                   .FriendlyName( "Copy With Reference" )
                                   .Scope( AS_GLOBAL )
                                   .DefaultHotkey( 'X' ) );

    // User re-binds the frame action onto the key common.Interactive.copy ships with.
    frameCopy.SetHotKey( 'C' );

    std::vector<const TOOL_ACTION*> global{ &commonCopy, &frameCopy };

    ACTION_MANAGER::PromoteUserBoundFrameAction( global, FRAME_PCB_EDITOR, 'C' );

    BOOST_CHECK_EQUAL( global.front()->GetName(), "pcbnew.InteractiveMove.copyWithReference" );
}

BOOST_AUTO_TEST_CASE( Promotion_OtherFrameLeavesOrderUnchanged )
{
    TOOL_ACTION commonCopy( TOOL_ACTION_ARGS()
                                    .Name( "common.Interactive.copy" )
                                    .FriendlyName( "Copy" )
                                    .Scope( AS_GLOBAL )
                                    .DefaultHotkey( 'C' ) );
    TOOL_ACTION frameCopy( TOOL_ACTION_ARGS()
                                   .Name( "pcbnew.InteractiveMove.copyWithReference" )
                                   .FriendlyName( "Copy With Reference" )
                                   .Scope( AS_GLOBAL )
                                   .DefaultHotkey( 'X' ) );

    frameCopy.SetHotKey( 'C' );

    std::vector<const TOOL_ACTION*> global{ &commonCopy, &frameCopy };

    ACTION_MANAGER::PromoteUserBoundFrameAction( global, FRAME_SCH, 'C' );

    BOOST_CHECK_EQUAL( global.front()->GetName(), "common.Interactive.copy" );
}

BOOST_AUTO_TEST_CASE( Promotion_DefaultBindingLeavesOrderUnchanged )
{
    TOOL_ACTION commonCopy( TOOL_ACTION_ARGS()
                                    .Name( "common.Interactive.copy" )
                                    .FriendlyName( "Copy" )
                                    .Scope( AS_GLOBAL )
                                    .DefaultHotkey( 'C' ) );
    TOOL_ACTION frameCopy( TOOL_ACTION_ARGS()
                                   .Name( "pcbnew.InteractiveMove.copyWithReference" )
                                   .FriendlyName( "Copy With Reference" )
                                   .Scope( AS_GLOBAL )
                                   .DefaultHotkey( 'C' ) );

    std::vector<const TOOL_ACTION*> global{ &commonCopy, &frameCopy };

    ACTION_MANAGER::PromoteUserBoundFrameAction( global, FRAME_PCB_EDITOR, 'C' );

    BOOST_CHECK_EQUAL( global.front()->GetName(), "common.Interactive.copy" );
}

BOOST_AUTO_TEST_CASE( HotkeyCondition_FallsBackToEnableWhenUnset )
{
    // Without a separate hotkey condition, RunHotKey() must reuse enableCondition,
    // preserving behavior for the vast majority of actions.
    ACTION_CONDITIONS cond;
    cond.Enable( SELECTION_CONDITIONS::NotEmpty );

    SELECTION empty;

    BOOST_CHECK( !cond.enableCondition( empty ) );
    BOOST_CHECK( !cond.GetHotkeyCondition()( empty ) );
}

BOOST_AUTO_TEST_CASE( HotkeyCondition_DivergesFromEnableWhenSet )
{
    // The #13366 fix greys out the menu (enableCondition) on an empty selection while
    // immediate-mode rotate/mirror still fire (hotkeyCondition). The two conditions must
    // be evaluated independently for the same selection.
    auto alwaysTrue = []( const SELECTION& ) { return true; };

    ACTION_CONDITIONS cond;
    cond.Enable( SELECTION_CONDITIONS::NotEmpty ).HotkeyEnable( alwaysTrue );

    SELECTION empty;

    BOOST_CHECK( !cond.enableCondition( empty ) );  // menu item greyed out
    BOOST_CHECK( cond.GetHotkeyCondition()( empty ) );  // hotkey still fires
}

BOOST_AUTO_TEST_SUITE_END()
