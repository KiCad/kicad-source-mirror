/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <tool/action_manager.h>
#include <tool/tool_manager.h>
#include <tool/tool_action.h>
#include <draw_frame.h>

#include <hotkeys_basic.h>
#include <boost/foreach.hpp>
#include <boost/range/adaptor/map.hpp>
#include <cassert>

ACTION_MANAGER::ACTION_MANAGER( TOOL_MANAGER* aToolManager ) :
    m_toolMgr( aToolManager )
{
    // Register known actions
    std::list<TOOL_ACTION*>& actionList = GetActionList();
    BOOST_FOREACH( TOOL_ACTION* action, actionList )
        RegisterAction( action );
}


ACTION_MANAGER::~ACTION_MANAGER()
{
    while( !m_actionIdIndex.empty() )
        UnregisterAction( m_actionIdIndex.begin()->second );
}


void ACTION_MANAGER::RegisterAction( TOOL_ACTION* aAction )
{
    // TOOL_ACTIONs are supposed to be named [appName.]toolName.actionName (with dots between)
    // action name without specifying at least toolName is not valid
    assert( aAction->GetName().find( '.', 0 ) != std::string::npos );

    // TOOL_ACTIONs must have unique names & ids
    assert( m_actionNameIndex.find( aAction->m_name ) == m_actionNameIndex.end() );
    assert( m_actionIdIndex.find( aAction->m_id ) == m_actionIdIndex.end() );

    if( aAction->m_id == -1 )
        aAction->m_id = MakeActionId( aAction->m_name );

    m_actionNameIndex[aAction->m_name] = aAction;
    m_actionIdIndex[aAction->m_id] = aAction;
}


void ACTION_MANAGER::UnregisterAction( TOOL_ACTION* aAction )
{
    m_actionNameIndex.erase( aAction->m_name );
    m_actionIdIndex.erase( aAction->m_id );

    if( aAction->HasHotKey() )
    {
        std::list<TOOL_ACTION*>& actions = m_actionHotKeys[aAction->m_currentHotKey];
        std::list<TOOL_ACTION*>::iterator action = std::find( actions.begin(), actions.end(), aAction );

        if( action != actions.end() )
            actions.erase( action );
        else
            assert( false );
    }
}


int ACTION_MANAGER::MakeActionId( const std::string& aActionName )
{
    static int currentActionId = 1;

    return currentActionId++;
}


TOOL_ACTION* ACTION_MANAGER::FindAction( const std::string& aActionName ) const
{
    std::map<std::string, TOOL_ACTION*>::const_iterator it = m_actionNameIndex.find( aActionName );

    if( it != m_actionNameIndex.end() )
        return it->second;

    return NULL;
}


bool ACTION_MANAGER::RunHotKey( int aHotKey ) const
{
    int key = aHotKey & ~MD_MODIFIER_MASK;
    int mod = aHotKey & MD_MODIFIER_MASK;

    if( key >= 'a' && key <= 'z')
        key = std::toupper(key);

    HOTKEY_LIST::const_iterator it = m_actionHotKeys.find( key | mod );

    // If no luck, try without Shift, to handle keys that require it
    // e.g. to get ? you need to press Shift+/ without US keyboard layout
    // Hardcoding ? as Shift+/ is a bad idea, as on another layout you may need to press a
    // different combination
    if( it == m_actionHotKeys.end() )
    {
        it = m_actionHotKeys.find( key | ( mod & ~MD_SHIFT ) );

        if( it == m_actionHotKeys.end() )
            return false; // no appropriate action found for the hotkey
    }

    const std::list<TOOL_ACTION*>& actions = it->second;

    // Choose the action that has the highest priority on the active tools stack
    // If there is none, run the global action associated with the hot key
    int highestPriority = -1, priority = -1;
    const TOOL_ACTION* context = NULL;  // pointer to context action of the highest priority tool
    const TOOL_ACTION* global = NULL;   // pointer to global action, if there is no context action

    BOOST_FOREACH( const TOOL_ACTION* action, actions )
    {
        if( action->GetScope() == AS_GLOBAL )
        {
            // Store the global action for the hot key in case there was no possible
            // context actions to run
            assert( global == NULL );       // there should be only one global action per hot key
            global = action;
            continue;
        }

        TOOL_BASE* tool = m_toolMgr->FindTool( action->GetToolName() );

        if( tool )
        {
            // Choose the action that goes to the tool with highest priority
            // (i.e. is on the top of active tools stack)
            priority = m_toolMgr->GetPriority( tool->GetId() );

            if( priority >= 0 && priority > highestPriority )
            {
                highestPriority = priority;
                context = action;
            }
        }
    }

    if( context )
    {
        m_toolMgr->RunAction( *context, true );
        return true;
    }
    else if( global )
    {
        m_toolMgr->RunAction( *global, true );
        return true;
    }

    return false;
}


void ACTION_MANAGER::UpdateHotKeys()
{
    m_actionHotKeys.clear();
    std::list<TOOL_ACTION*>& actions = GetActionList();

    for( std::list<TOOL_ACTION*>::iterator it = actions.begin(); it != actions.end(); ++it )
    {
        TOOL_ACTION* aAction = *it;

        int hotkey = processHotKey( aAction, true );

        if( hotkey > 0 )
            m_actionHotKeys[hotkey].push_back( aAction );
    }

#ifndef NDEBUG
    // Check if there are two global actions assigned to the same hotkey
    BOOST_FOREACH( std::list<TOOL_ACTION*>& action_list, m_actionHotKeys | boost::adaptors::map_values )
    {
        int global_actions_cnt = 0;

        BOOST_FOREACH( TOOL_ACTION* action, action_list )
        {
            if( action->GetScope() == AS_GLOBAL )
                ++global_actions_cnt;
        }

        assert( global_actions_cnt <= 1 );
    }
#endif /* not NDEBUG */
}


int ACTION_MANAGER::processHotKey( TOOL_ACTION* aAction, bool aForceUpdate )
{
    int hotkey = 0;

    if( aForceUpdate )
        hotkey = aAction->getDefaultHotKey();
    else
        hotkey = aAction->GetHotKey();

    if( ( hotkey & TOOL_ACTION::LEGACY_HK ) )
    {
        hotkey = hotkey & ~TOOL_ACTION::LEGACY_HK;  // it leaves only HK_xxx identifier
        EDA_DRAW_FRAME* frame = static_cast<EDA_DRAW_FRAME*>( m_toolMgr->GetEditFrame() );
        EDA_HOTKEY* hk_desc = frame->GetHotKeyDescription( hotkey );

        if( hk_desc )
        {
            hotkey = hk_desc->m_KeyCode;

            // Convert modifiers to the ones used by the Tool Framework
            if( hotkey & GR_KB_CTRL )
            {
                hotkey &= ~GR_KB_CTRL;
                hotkey |= MD_CTRL;
            }

            if( hotkey & GR_KB_ALT )
            {
                hotkey &= ~GR_KB_ALT;
                hotkey |= MD_ALT;
            }

            if( hotkey & GR_KB_SHIFT )
            {
                hotkey &= ~GR_KB_SHIFT;
                hotkey |= MD_SHIFT;
            }
        }
        else
        {
            hotkey = 0;
        }

        aAction->setHotKey( hotkey );
    }

    return hotkey;
}
