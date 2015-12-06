/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#include <string>

#include <tool/tool_event.h>
#include <tool/tool_manager.h>
#include <tool/tool_interactive.h>
#include <tool/context_menu.h>

TOOL_INTERACTIVE::TOOL_INTERACTIVE( TOOL_ID aId, const std::string& aName ) :
    TOOL_BASE( INTERACTIVE, aId, aName )
{
}


TOOL_INTERACTIVE::TOOL_INTERACTIVE( const std::string& aName ) :
    TOOL_BASE( INTERACTIVE, TOOL_MANAGER::MakeToolId( aName ), aName )
{
}


TOOL_INTERACTIVE::~TOOL_INTERACTIVE()
{
}


void TOOL_INTERACTIVE::Activate()
{
    m_toolMgr->InvokeTool( m_toolId );
}


OPT_TOOL_EVENT TOOL_INTERACTIVE::Wait( const TOOL_EVENT_LIST& aEventList )
{
    return m_toolMgr->ScheduleWait( this, aEventList );
}


void TOOL_INTERACTIVE::goInternal( TOOL_STATE_FUNC& aState, const TOOL_EVENT_LIST& aConditions )
{
    m_toolMgr->ScheduleNextState( this, aState, aConditions );
}


void TOOL_INTERACTIVE::SetContextMenu( CONTEXT_MENU* aMenu, CONTEXT_MENU_TRIGGER aTrigger )
{
    if( aMenu )
        aMenu->SetTool( this );

    m_toolMgr->ScheduleContextMenu( this, aMenu, aTrigger );
}
