/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#ifndef __TOOL_MANAGER_H
#define __TOOL_MANAGER_H

#include <deque>
#include <typeinfo>
#include <map>

#include <tool/tool_base.h>

class TOOL_BASE;
class ACTION_MANAGER;
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
     * Generates a unique ID from for a tool with given name.
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
     * Calls a tool by sending a tool activation event to tool of given ID.
     *
     * @param aToolId is the ID number of the requested tool.
     * @return True if the requested tool was invoked successfully.
     */
    bool InvokeTool( TOOL_ID aToolId );

    /**
     * Function InvokeTool()
     * Calls a tool by sending a tool activation event to tool of given name.
     *
     * @param aToolName is the name of the requested tool.
     * @return True if the requested tool was invoked successfully.
     */
    bool InvokeTool( const std::string& aToolName );

    /**
     * Function RegisterAction()
     * Registers an action that can be used to control tools (eg. invoke, trigger specific
     * behaviours).
     *
     * @param aAction is the action to be registered.
     */
    void RegisterAction( TOOL_ACTION* aAction );

    /**
     * Function UnregisterAction()
     * Unregisters an action, so it is no longer active.
     *
     * @param aAction is the action to be unregistered.
     */
    void UnregisterAction( TOOL_ACTION* aAction );

    /**
     * Function RunAction()
     * Runs the specified action. The common format for action names is "application.ToolName.Action".
     *
     * @param aActionName is the name of action to be invoked.
     * @param aNow decides if the action has to be run immediately or after the current coroutine
     * is preemptied.
     * @param aParam is an optional parameter that might be used by the invoked action. Its meaning
     * depends on the action.
     * @return False if the action was not found.
     */
    template<typename T>
    bool RunAction( const std::string& aActionName, bool aNow = false, T aParam = NULL )
    {
        return RunAction( aActionName, aNow, reinterpret_cast<void*>( aParam ) );
    }

    bool RunAction( const std::string& aActionName, bool aNow, void* aParam );

    bool RunAction( const std::string& aActionName, bool aNow = false )
    {
        return RunAction( aActionName, aNow, (void*) NULL );
    }

    /**
     * Function RunAction()
     * Runs the specified action.
     *
     * @param aAction is the action to be invoked.
     * @param aNow decides if the action has to be run immediately or after the current coroutine
     * is preemptied.
     * @param aParam is an optional parameter that might be used by the invoked action. Its meaning
     * depends on the action.
     */
    template<typename T>
    void RunAction( const TOOL_ACTION& aAction, bool aNow = false, T aParam = NULL )
    {
        RunAction( aAction, aNow, reinterpret_cast<void*>( aParam ) );
    }

    void RunAction( const TOOL_ACTION& aAction, bool aNow, void* aParam );

    void RunAction( const TOOL_ACTION& aAction, bool aNow = false )
    {
        RunAction( aAction, aNow, (void*) NULL );
    }

    /**
     * Function FindTool()
     * Searches for a tool with given ID.
     *
     * @param aId is the ID number of the requested tool.
     * @return Pointer to the requested tool or NULL in case of failure.
     */
    TOOL_BASE* FindTool( int aId ) const;

    /**
     * Function FindTool()
     * Searches for a tool with given name.
     *
     * @param aName is the name of the requested tool.
     * @return Pointer to the requested tool or NULL in case of failure.
     */
    TOOL_BASE* FindTool( const std::string& aName ) const;

    /*
     * Function GetTool()
     * Returns the tool of given type or NULL if there is no such tool registered.
     */
    template<typename T>
    T* GetTool()
    {
        std::map<const char*, TOOL_BASE*>::iterator tool = m_toolTypes.find( typeid( T ).name() );

        if( tool != m_toolTypes.end() )
            return static_cast<T*>( tool->second );

        return NULL;
    }

    /**
     * Function ResetTools()
     * Resets all tools (i.e. calls their Reset() method).
     */
    void ResetTools( TOOL_BASE::RESET_REASON aReason );

    /**
     * Propagates an event to tools that requested events of matching type(s).
     * @param aEvent is the event to be processed.
     */
    bool ProcessEvent( const TOOL_EVENT& aEvent );

    /**
     * Puts an event to the event queue to be processed at the end of event processing cycle.
     * @param aEvent is the event to be put into the queue.
     */
    inline void PostEvent( const TOOL_EVENT& aEvent )
    {
        m_eventQueue.push_back( aEvent );
    }

    /**
     * Sets the work environment (model, view, view controls and the parent window).
     * These are made available to the tool. Called by the parent frame (PCB_EDIT_FRAME)
     * when the board is set up.
     */
    void SetEnvironment( EDA_ITEM* aModel, KIGFX::VIEW* aView,
            KIGFX::VIEW_CONTROLS* aViewControls, wxWindow* aFrame );

    /* Accessors for the environment objects (view, model, etc.) */
    KIGFX::VIEW* GetView() const
    {
        return m_view;
    }

    inline KIGFX::VIEW_CONTROLS* GetViewControls() const
    {
        return m_viewControls;
    }

    inline EDA_ITEM* GetModel() const
    {
        return m_model;
    }

    inline wxWindow* GetEditFrame() const
    {
        return m_editFrame;
    }

    /**
     * Returns id of the tool that is on the top of the active tools stack
     * (was invoked the most recently).
     * @return Id of the currently used tool.
     */
    inline int GetCurrentToolId() const
    {
        return m_activeTools.front();
    }

    /**
     * Returns the tool that is on the top of the active tools stack
     * (was invoked the most recently).
     * @return Pointer to the currently used tool.
     */
    inline TOOL_BASE* GetCurrentTool() const
    {
        return FindTool( GetCurrentToolId() );
    }

    /**
     * Returns priority of a given tool. Higher number means that the tool is closer to the
     * beginning of the active tools queue (i.e. receives events earlier, tools with lower
     * priority receive events later).
     * @param aToolId is the id of queried tool.
     * @return The priority of a given tool. If returned number is negative, then it means that
     * the tool id is invalid or the tool is not active.
     */
    int GetPriority( int aToolId ) const;

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
     * @param aTool - the parent tool
     * @param aMenu - the menu structure, defined by the tool
     * @param aTrigger - when the menu is activated:
     *  CMENU_NOW: opens the menu right now
     *  CMENU_BUTTON: opens the menu when RMB is pressed
     *  CMENU_OFF: menu is disabled.
     * May be called from a coroutine context.
     */
    void ScheduleContextMenu( TOOL_BASE* aTool, CONTEXT_MENU* aMenu,
            CONTEXT_MENU_TRIGGER aTrigger );

    /**
     * Allows a tool to pass the already handled event to the next tool on the stack.
     */
    void PassEvent()
    {
        m_passEvent = true;
    }

    /**
     * Stores an information to the system clipboard.
     * @param aText is the information to be stored.
     * @return False if error occured.
     */
    bool SaveClipboard( const std::string& aText );

    /**
     * Returns the information currently stored in the system clipboard. If data stored in the
     * clipboard is in non-text format, empty string is returned.
     */
    std::string GetClipboard() const;

    /**
     * Returns list of TOOL_ACTIONs. TOOL_ACTIONs add themselves to the list upon their
     * creation.
     * @return List of TOOL_ACTIONs.
     */
    static std::list<TOOL_ACTION*>& GetActionList()
    {
        // TODO I am afraid this approach won't work when we reach multitab version of kicad.
        static std::list<TOOL_ACTION*> actionList;

        return actionList;
    }

private:
    struct TOOL_STATE;
    typedef std::pair<TOOL_EVENT_LIST, TOOL_STATE_FUNC> TRANSITION;

    /**
     * Function dispatchInternal
     * Passes an event at first to the active tools, then to all others.
     */
    void dispatchInternal( const TOOL_EVENT& aEvent );

    /**
     * Function dispatchStandardEvents()
     * Handles specific events, that are intended for TOOL_MANAGER rather than tools.
     * @param aEvent is the event to be processed.
     * @return False if the event was processed and should not go any further.
     */
    bool dispatchStandardEvents( const TOOL_EVENT& aEvent );

    /**
     * Function dispatchActivation()
     * Checks if it is a valid activation event and invokes a proper tool.
     * @param aEvent is an event to be tested.
     * @return True if a tool was invoked, false otherwise.
     */
    bool dispatchActivation( const TOOL_EVENT& aEvent );

    /**
     * Function dispatchContextMenu()
     * Handles context menu related events.
     */
    void dispatchContextMenu( const TOOL_EVENT& aEvent );

    /**
     * Function invokeTool()
     * Invokes a tool by sending a proper event (in contrary to runTool, which makes the tool run
     * for real).
     * @param aTool is the tool to be invoked.
     */
    bool invokeTool( TOOL_BASE* aTool );

    /**
     * Function runTool()
     * Makes a tool active, so it can receive events and react to them. Activated tool is pushed
     * on the active tools stack, so the last activated tool receives events first.
     *
     * @param aToolId is the ID number of tool to be run.
     */
    bool runTool( TOOL_ID aToolId );

    /**
     * Function runTool()
     * Makes a tool active, so it can receive events and react to them. Activated tool is pushed
     * on the active tools stack, so the last activated tool receives events first.
     *
     * @param aName is the name of tool to be run.
     */
    bool runTool( const std::string& aName );

    /**
     * Function runTool()
     * Makes a tool active, so it can receive events and react to them. Activated tool is pushed
     * on the active tools stack, so the last activated tool receives events first.
     *
     * @param aTool is the tool to be run.
     */
    bool runTool( TOOL_BASE* aTool );

    template <class Parameters>
    void invokeTool( const std::string& aName, const Parameters& aToolParams );

    /**
     * Function finishTool()
     * Deactivates a tool and does the necessary clean up.
     *
     * @param aState is the state variable of the tool to be stopped.
     */
    void finishTool( TOOL_STATE* aState );

    /**
     * Function isRegistered()
     * Returns information about a tool registration status.
     *
     * @param aTool is the tool to be checked.
     * @return true if the tool is in the registered tools list, false otherwise.
     */
    bool isRegistered( TOOL_BASE* aTool ) const
    {
        return m_toolState.count( aTool ) > 0;
    }

    /**
     * Function isActive()
     * Returns information about a tool activation status.
     *
     * @param aTool is the tool to be checked.
     * @return True if the tool is on the active tools stack, false otherwise.
     */
    bool isActive( TOOL_BASE* aTool );

    /// Index of registered tools current states, associated by tools' objects.
    std::map<TOOL_BASE*, TOOL_STATE*> m_toolState;

    /// Index of the registered tools current states, associated by tools' names.
    std::map<std::string, TOOL_STATE*> m_toolNameIndex;

    /// Index of the registered tools to easily lookup by their type.
    std::map<const char*, TOOL_BASE*> m_toolTypes;

    /// Index of the registered tools current states, associated by tools' ID numbers.
    std::map<TOOL_ID, TOOL_STATE*> m_toolIdIndex;

    /// Stack of the active tools
    std::deque<TOOL_ID> m_activeTools;

    /// Instance of ACTION_MANAGER that handles TOOL_ACTIONs
    ACTION_MANAGER* m_actionMgr;

    EDA_ITEM* m_model;
    KIGFX::VIEW* m_view;
    KIGFX::VIEW_CONTROLS* m_viewControls;
    wxWindow* m_editFrame;

    /// Queue that stores events to be processed at the end of the event processing cycle.
    std::list<TOOL_EVENT> m_eventQueue;

    /// Flag saying if the currently processed event should be passed to other tools.
    bool m_passEvent;
};

#endif
