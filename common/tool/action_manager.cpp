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
    assert( aAction->GetId() == -1 );    // Check if the TOOL_ACTION was not registered before
    assert( m_actionNameIndex.find( aAction->m_name ) == m_actionNameIndex.end() );
    assert( m_actionIdIndex.find( aAction->m_id ) == m_actionIdIndex.end() );

    aAction->setId( MakeActionId( aAction->m_name ) );

    m_actionNameIndex[aAction->m_name] = aAction;
    m_actionIdIndex[aAction->m_id] = aAction;

    if( aAction->HasHotKey() )
        m_actionHotKeys[aAction->m_currentHotKey] = aAction;

    aAction->setActionMgr( this );
}


void ACTION_MANAGER::UnregisterAction( TOOL_ACTION* aAction )
{
    // Indicate that the ACTION_MANAGER no longer care about the object
    aAction->setActionMgr( NULL );
    aAction->setId( -1 );

    m_actionNameIndex.erase( aAction->m_name );
    m_actionIdIndex.erase( aAction->m_id );

    if( aAction->HasHotKey() )
        m_actionHotKeys.erase( aAction->m_currentHotKey );
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

    runAction( it->second );

    return true;
}


bool ACTION_MANAGER::RunHotKey( int aHotKey ) const
{
    std::map<int, TOOL_ACTION*>::const_iterator it = m_actionHotKeys.find( aHotKey );

    if( it == m_actionHotKeys.end() )
        return false; // no appropriate action found for the hotkey

    runAction( it->second );

    return true;
}


void ACTION_MANAGER::runAction( const TOOL_ACTION* aAction ) const
{
    TOOL_EVENT event = aAction->MakeEvent();

    m_toolMgr->ProcessEvent( event );
}
