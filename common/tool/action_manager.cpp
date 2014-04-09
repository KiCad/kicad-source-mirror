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
#include <tool/tool_event.h>
#include <tool/tool_action.h>
#include <boost/foreach.hpp>
#include <cassert>

ACTION_MANAGER::ACTION_MANAGER( TOOL_MANAGER* aToolManager ) :
    m_toolMgr( aToolManager )
{
}


ACTION_MANAGER::~ACTION_MANAGER()
{
    while( !m_actionIdIndex.empty() )
        UnregisterAction( m_actionIdIndex.begin()->second );
}


void ACTION_MANAGER::RegisterAction( TOOL_ACTION* aAction )
{
    // Check if the TOOL_ACTION was not registered before
    assert( aAction->GetId() == -1 );
    // TOOL_ACTIONs are supposed to be named [appName.]toolName.actionName (with dots between)
    // action name without specifying at least toolName is not valid
    assert( aAction->GetName().find( '.', 0 ) != std::string::npos );

    assert( m_actionNameIndex.find( aAction->m_name ) == m_actionNameIndex.end() );
    assert( m_actionIdIndex.find( aAction->m_id ) == m_actionIdIndex.end() );

    aAction->setId( MakeActionId( aAction->m_name ) );

    m_actionNameIndex[aAction->m_name] = aAction;
    m_actionIdIndex[aAction->m_id] = aAction;

    if( aAction->HasHotKey() )
        m_actionHotKeys[aAction->m_currentHotKey].push_back( aAction );
}


void ACTION_MANAGER::UnregisterAction( TOOL_ACTION* aAction )
{
    m_actionNameIndex.erase( aAction->m_name );
    m_actionIdIndex.erase( aAction->m_id );

    // Indicate that the ACTION_MANAGER no longer care about the object
    aAction->setId( -1 );

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


bool ACTION_MANAGER::RunAction( const std::string& aActionName ) const
{
    std::map<std::string, TOOL_ACTION*>::const_iterator it = m_actionNameIndex.find( aActionName );

    if( it == m_actionNameIndex.end() )
        return false; // no action with given name found

    RunAction( it->second );

    return true;
}


void ACTION_MANAGER::RunAction( const TOOL_ACTION* aAction ) const
{
    TOOL_EVENT event = aAction->MakeEvent();

    m_toolMgr->ProcessEvent( event );
}


bool ACTION_MANAGER::RunHotKey( int aHotKey ) const
{
    int key = std::toupper( aHotKey & ~MD_MODIFIER_MASK );
    int mod = aHotKey & MD_MODIFIER_MASK;

    HOTKEY_LIST::const_iterator it = m_actionHotKeys.find( key | mod );

    // If no luck, try without modifier, to handle keys that require a modifier
    // e.g. to get ? you need to press Shift+/ without US keyboard layout
    // Hardcoding ? as Shift+/ is a bad idea, as on another layout you may need to press a
    // different combination
    if( it == m_actionHotKeys.end() )
    {
        it = m_actionHotKeys.find( key );

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
            priority = m_toolMgr->GetPriority( tool->GetId() );

            if( priority >= 0 && priority > highestPriority )
            {
                highestPriority = priority;
                context = action;
            }
        }
    }

    if( !global && !context )   // currently there is no valid action to run
        return false;

    if( context )
        RunAction( context );
    else if( global )
        RunAction( global );

    return true;
}
