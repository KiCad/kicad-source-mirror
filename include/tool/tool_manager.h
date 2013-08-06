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

#ifndef __TOOL_MANAGER_H
#define __TOOL_MANAGER_H

#include <cstdio>
#include <map>
#include <vector>
#include <deque>

#include <boost/unordered_map.hpp>

#include <math/vector2d.h>

#include <tool/tool_event.h>
#include <tool/tool_base.h>

class TOOL_BASE;
class CONTEXT_MENU;
class wxWindow;

/**
 * Class TOOL_MANAGER.
 * Master controller class:
 * - registers editing tools
 * - pumps UI events to tools requesting them
 * - manages tool state machines (transitions and wait requests)
 */
class TOOL_MANAGER
{
	public:
		
		TOOL_MANAGER();
		~TOOL_MANAGER();

		/**
		 * Generates an unique ID from for a tool with given name.
		 */
		static TOOL_ID MakeToolId( const std::string& aToolName );

		/**
    	 * Function RegisterTool()
	     * Adds a tool to the manager set and sets it up. Called once for
	     * each tool during application initialization.
     	 * @param aTool: tool to be added. Ownership is transferred.
     	 */
		void RegisterTool( TOOL_BASE* aTool );

		/**
		 * Function InvokeTool()
		 * Calls a tool by sending a tool activation event to tool of given ID or name.
		 * An user-defined parameter object can be also passed 
		 */	
		void InvokeTool( TOOL_ID aToolId );
		void InvokeTool( const std::string& name );

		template <class Parameters>
			void InvokeTool( const std::string& name, const Parameters& aToolParams );


		/**
		 * Function FindTool()
		 * Searches for a tool with given name or ID 
		 */
		TOOL_BASE *FindTool( int aId );
		TOOL_BASE *FindTool( const std::string& aName );

		/**
		 * Resets the state of a given tool by clearing its wait and
		 * transition lists and calling tool's internal Reset() method.
		 */
		void ResetTool( TOOL_BASE *aTool );

		/**
		 * Takes an event from the TOOL_DISPATCHER and propagates it to 
		 * tools that requested events of matching type(s)
		 */
		bool ProcessEvent( TOOL_EVENT& aEvent );
		
		/**
		 * Sets the work environment (model, view, view controls and the parent window).
		 * These are made available to the tool. Called by the parent frame (PCB_EDIT_FRAME)
		 * when the board is set up
		 */
		void SetEnvironment( EDA_ITEM* aModel, KiGfx::VIEW* aView,
		                     KiGfx::VIEW_CONTROLS* aViewControls, wxWindow* aFrame );

		/* Accessors for the environment objects (view, model, etc.) */
		KiGfx::VIEW* GetView()
		{
			return m_view;
		}

		KiGfx::VIEW_CONTROLS* GetViewControls()
		{
			return m_viewControls;
		}

		EDA_ITEM* GetModel()
		{
			return m_model;
		}

		wxWindow* GetEditFrame()
		{
			return m_editFrame;
		}

		/**
		 * Defines a state transition - the events that cause a given handler method in the tool 
		 * to be called. Called by TOOL_INTERACTIVE::Go(). May be called from a coroutine context.
		 */
		void ScheduleNextState( TOOL_BASE* aTool, TOOL_STATE_FUNC& aHandler,
		                        const TOOL_EVENT_LIST& aConditions );
		
		/**
		 * Pauses execution of a given tool until one or more events matching aConditions arrives.
		 * The pause/resume operation is done through COROUTINE object.
		 * Called only from coroutines.
		 */
		boost::optional<TOOL_EVENT> ScheduleWait( TOOL_BASE* aTool,
		                                          const TOOL_EVENT_LIST& aConditions );
		
		/**
		 * Sets behaviour of the tool's context popup menu. 
		 * @param aMenu - the menu structure, defined by the tool
		 * @param aTrigger - when the menu is activated: 
		 * CMENU_NOW: opens the menu right now
		 * CMENU_BUTTON: opens the menu when RMB is pressed
		 * CMENU_OFF: menu is disabled.
		 * May be called from a coroutine context.
		 */
		void ScheduleContextMenu( TOOL_BASE* aTool, CONTEXT_MENU* aMenu,
		                          TOOL_ContextMenuTrigger aTrigger );

	private:
		void dispatchInternal ( TOOL_EVENT& aEvent );

		struct ToolState;
		typedef std::pair<TOOL_EVENT_LIST, TOOL_STATE_FUNC> Transition;

		std::map<TOOL_BASE*, ToolState*> m_toolState;
		std::map<std::string, ToolState*> m_toolNameIndex;
		std::map<TOOL_ID, ToolState*> m_toolIdIndex;

		EDA_ITEM* m_model;
		KiGfx::VIEW* m_view;
		KiGfx::VIEW_CONTROLS* m_viewControls;
		wxWindow* m_editFrame;

		ToolState* m_currentTool;
};

#endif
