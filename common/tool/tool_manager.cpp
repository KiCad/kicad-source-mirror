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

#include <map>
#include <deque>

#include <boost/foreach.hpp>
#include <boost/optional.hpp>
#include <boost/range/adaptor/map.hpp>

#include <wx/event.h>

#include <view/view.h>

#include <tool/tool_base.h>
#include <tool/tool_interactive.h>
#include <tool/tool_manager.h>
#include <tool/context_menu.h>
#include <tool/coroutine.h>

#include <wxPcbStruct.h>
#include <class_drawpanel_gal.h>

using boost::optional;
using namespace std;

struct TOOL_MANAGER::ToolState
{
	TOOL_BASE* theTool;
	
	bool idle;
	bool pendingWait;
	bool pendingContextMenu;

	CONTEXT_MENU* contextMenu;
	TOOL_ContextMenuTrigger contextMenuTrigger;

	COROUTINE<int, TOOL_EVENT&>* cofunc;
	
	TOOL_EVENT wakeupEvent;
	TOOL_EVENT_LIST waitEvents;

	std::vector<Transition> transitions;
};


TOOL_MANAGER::TOOL_MANAGER()
{
}


void TOOL_MANAGER::RegisterTool( TOOL_BASE* aTool )
{
	ToolState* st = new ToolState;

	st->theTool = aTool;
	st->idle = true;
	st->pendingWait = false;
	st->pendingContextMenu = false;
	st->cofunc = NULL;
	st->contextMenuTrigger = CMENU_OFF;

	m_toolState[aTool] = st;
	m_toolNameIndex[aTool->GetName()] = st;
	m_toolIdIndex[aTool->GetId()] = st;
	
	aTool->m_toolMgr = this;
	
	if( aTool->GetType() == TOOL_Interactive )
		static_cast<TOOL_INTERACTIVE*>( aTool )->Reset();
}


void TOOL_MANAGER::InvokeTool( TOOL_ID aToolId )
{
    TOOL_BASE* tool = FindTool( aToolId );

    if( tool && tool->GetType() == TOOL_Interactive )
        static_cast<TOOL_INTERACTIVE*>( tool )->Reset();

    TOOL_EVENT evt( TC_Command, TA_ActivateTool, tool->GetName() );
    ProcessEvent( evt );
}


void TOOL_MANAGER::InvokeTool( const std::string& aName )
{
    TOOL_BASE* tool = FindTool( aName );

    if( tool )
        InvokeTool( tool->GetId() );
}


TOOL_BASE* TOOL_MANAGER::FindTool( int aId ) const
{
    std::map<TOOL_ID, ToolState*>::const_iterator it = m_toolIdIndex.find( aId );

    if( it != m_toolIdIndex.end() )
        return it->second->theTool;

    return NULL;
}


TOOL_BASE* TOOL_MANAGER::FindTool( const std::string& aName ) const
{
    std::map<std::string, ToolState*>::const_iterator it = m_toolNameIndex.find( aName );

    if( it != m_toolNameIndex.end() )
        return it->second->theTool;

    return NULL;
}


void TOOL_MANAGER::ScheduleNextState( TOOL_BASE* aTool, TOOL_STATE_FUNC& aHandler,
                                      const TOOL_EVENT_LIST& aConditions )
{
	ToolState* st = m_toolState[aTool];
	st->transitions.push_back( Transition( aConditions, aHandler ) );
}


optional<TOOL_EVENT> TOOL_MANAGER::ScheduleWait( TOOL_BASE* aTool,
                                                 const TOOL_EVENT_LIST& aConditions )
{
	ToolState* st = m_toolState[aTool];
	
	st->pendingWait = true;
	st->waitEvents = aConditions;
	st->cofunc->Yield();

	return st->wakeupEvent;
}


void TOOL_MANAGER::dispatchInternal( TOOL_EVENT& aEvent )
{
	// iterate over all registered tools
	BOOST_FOREACH( ToolState* st, m_toolState | boost::adaptors::map_values )
	{
		// the tool state handler is waiting for events (i.e. called Wait() method)
		if( st->pendingWait )
		{
			if( st->waitEvents.Matches( aEvent ) )
			{
				// got matching event? clear wait list and wake up the coroutine
				st->wakeupEvent = aEvent;
				st->pendingWait = false;
				st->waitEvents.clear();
				st->cofunc->Resume();
				if( !st->cofunc->Running() )
				{
					delete st->cofunc;
					st->cofunc = NULL;
				}
			}
		}
		else
		{
			// no state handler in progress - check if there are any transitions (defined by
			// Go() method that match the event.

			if( st->transitions.size() )
			{
				BOOST_FOREACH( Transition tr, st->transitions )
				{
					if( tr.first.Matches( aEvent ) )
					{
						st->transitions.clear();

						if( !st->cofunc )
							st->cofunc = new COROUTINE<int, TOOL_EVENT&>( tr.second );
						else 
							st->cofunc->SetEntry( tr.second );
							
						// got match? Run the handler.
						st->cofunc->Call( aEvent );
						
						if( !st->cofunc->Running() )
						{
							delete st->cofunc;
							st->cofunc = NULL;
						}
					}
				}
			}
		}
	}	
}


bool TOOL_MANAGER::ProcessEvent( TOOL_EVENT& aEvent )
{
	printf( "process: %s\n", aEvent.Format().c_str() );

	dispatchInternal( aEvent );
	
	BOOST_FOREACH( ToolState* st, m_toolState | boost::adaptors::map_values )
	{
		if( st->contextMenuTrigger == CMENU_NOW )
		{
			st->pendingWait = true;
			st->waitEvents = TOOL_EVENT( TC_Any, TA_Any );
			st->contextMenuTrigger = CMENU_OFF;
			GetEditFrame()->PopupMenu( st->contextMenu->GetMenu() );

			TOOL_EVENT evt( TC_Command, TA_ContextMenuChoice );
			dispatchInternal( evt );

			break;
		}
	}

	if( m_view->IsDirty() )
	{
		PCB_EDIT_FRAME* f = static_cast<PCB_EDIT_FRAME*>( GetEditFrame() );
		f->GetGalCanvas()->Refresh(); // fixme: ugly hack, provide a method in TOOL_DISPATCHER.
	}

	return false;
}


void TOOL_MANAGER::ScheduleContextMenu( TOOL_BASE* aTool, CONTEXT_MENU* aMenu,
                                        TOOL_ContextMenuTrigger aTrigger )
{
	ToolState* st = m_toolState[aTool];

	st->contextMenu = aMenu;
	st->contextMenuTrigger = aTrigger;
	
	if( aTrigger == CMENU_NOW )
		st->cofunc->Yield();	
}


TOOL_ID TOOL_MANAGER::MakeToolId( const std::string& aToolName )
{
	static int currentId;
	return currentId++;
}


void TOOL_MANAGER::SetEnvironment( EDA_ITEM* aModel, KiGfx::VIEW* aView,
                                   KiGfx::VIEW_CONTROLS* aViewControls, wxWindow* aFrame )
{
	m_model = aModel;
	m_view = aView;
	m_viewControls = aViewControls;
	m_editFrame = aFrame;

	// Reset state of the registered tools
	BOOST_FOREACH( TOOL_BASE* tool, m_toolState | boost::adaptors::map_keys )
	{
	    if( tool->GetType() == TOOL_Interactive )
	        static_cast<TOOL_INTERACTIVE*>( tool )->Reset();
	}
}
