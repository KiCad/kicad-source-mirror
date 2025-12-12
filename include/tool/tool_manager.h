/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2023 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
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

#ifndef TOOL_MANAGER_H
#define TOOL_MANAGER_H

#include <list>
#include <map>
#include <stack>
#include <vector>
#include <typeinfo>
#include <type_traits>

#include <tool/tool_base.h>
#include <tool/tool_event.h>

namespace KIGFX
{
class VIEW_CONTROLS;
struct VC_SETTINGS;
}

class COMMIT;
class TOOLS_HOLDER;
class TOOL_ACTION;
class ACTION_MANAGER;
class ACTION_MENU;
class APP_SETTINGS_BASE;


/**
 * Master controller class:
 * - registers editing tools
 * - pumps UI events to tools requesting them
 * - manages tool state machines (transitions and wait requests)
 */
class TOOL_MANAGER
{
private:
    struct TOOL_STATE;

public:
    TOOL_MANAGER();

    ~TOOL_MANAGER();

    // Helper typedefs
    typedef std::map<TOOL_BASE*, TOOL_STATE*> TOOL_STATE_MAP;
    typedef std::map<std::string, TOOL_STATE*> NAME_STATE_MAP;
    typedef std::map<TOOL_ID, TOOL_STATE*> ID_STATE_MAP;
    typedef std::list<TOOL_ID> ID_LIST;

    /**
     * Generate a unique ID from for a tool with given name.
     */
    static TOOL_ID MakeToolId( const std::string& aToolName );

    /**
     * Add a tool to the manager set and sets it up. Called once for each tool during
     * application initialization.
     *
     * @param aTool: tool to be added. Ownership is transferred.
     */
    void RegisterTool( TOOL_BASE* aTool );

    /**
     * Call a tool by sending a tool activation event to tool of given ID.
     *
     * @param aToolId is the ID number of the requested tool.
     * @return True if the requested tool was invoked successfully.
     */
    bool InvokeTool( TOOL_ID aToolId );

    /**
     * Call a tool by sending a tool activation event to tool of given name.
     *
     * @param aToolName is the name of the requested tool.
     * @return True if the requested tool was invoked successfully.
     */
    bool InvokeTool( const std::string& aToolName );

    /**
     * Shutdown all tools with a currently registered event loop in this tool manager
     * by waking them up with a null event.
     */
    void ShutdownAllTools();

    /**
     * Shutdown the specified tool by waking it up with a null event to terminate
     * the processing loop.
     *
     * @param aTool is the tool to shutdown
     */
    void ShutdownTool( TOOL_BASE* aTool );

    /**
     * Shutdown the specified tool by waking it up with a null event to terminate
     * the processing loop.
     *
     * @param aToolId is the ID of the tool to shutdown
     */
    void ShutdownTool( TOOL_ID aToolId );

    /**
     * Shutdown the specified tool by waking it up with a null event to terminate
     * the processing loop.
     *
     * @param aToolName is name of the tool to shutdown
     */
    void ShutdownTool( const std::string& aToolName );

    /**
     * Run the specified action immediately, pausing the current action to run the new one.
     *
     * The common format for action names is "application.ToolName.Action".
     *
     * @note The type of the optional parameter must match exactly with the type the consuming
     *       action is expecting, otherwise an assert will occur when reading the parameter.
     *
     * @param aActionName is the name of action to be invoked.
     * @param aParam is an optional parameter that might be used by the invoked action. Its meaning
     *               depends on the action.
     * @return False if the action was not found.
     */
    template<typename T, std::enable_if_t<!std::is_convertible_v<T*, COMMIT*>>* = nullptr>
    bool RunAction( const std::string& aActionName, T aParam )
    {
        // Use a cast to ensure the proper type is stored inside the parameter
        ki::any a( static_cast<T>( aParam ) );

        return doRunAction( aActionName, true, a, nullptr );
    }

    bool RunAction( const std::string& aActionName )
    {
        // Default initialize the parameter argument to an empty ki_any
        ki::any a;

        return doRunAction( aActionName, true, a, nullptr );
    }

    /**
     * Run the specified action immediately, pausing the current action to run the new one.
     *
     * @note The type of the optional parameter must match exactly with the type the consuming
     *       action is expecting, otherwise an assert will occur when reading the parameter.
     *
     * @param aAction is the action to be invoked.
     * @param aParam is an optional parameter that might be used by the invoked action. Its meaning
     *               depends on the action.
     * @return True if the action was handled immediately.
     */
    template<typename T, std::enable_if_t<!std::is_convertible_v<T, COMMIT*>>* = nullptr>
    bool RunAction( const TOOL_ACTION& aAction, T aParam )
    {
        // Use a cast to ensure the proper type is stored inside the parameter
        ki::any a( static_cast<T>( aParam ) );

        return doRunAction( aAction, true, a, nullptr );
    }

    /**
     * Run the specified action immediately, pausing the current action to run the new one.
     *
     * @note The type of the optional parameter must match exactly with the type the consuming
     *       action is expecting, otherwise an assert will occur when reading the parameter.
     *
     * @param aAction is the action to be invoked.
     * @param aCommit is the commit object the tool handling the action should add the new edits to
     * @return True if the action was handled immediately
     */
    template<typename T>
    bool RunSynchronousAction( const TOOL_ACTION& aAction, COMMIT* aCommit, T aParam )
    {
        // Use a cast to ensure the proper type is stored inside the parameter
        ki::any a( static_cast<T>( aParam ) );

        return doRunAction( aAction, true, a, aCommit );
    }

    bool RunSynchronousAction( const TOOL_ACTION& aAction, COMMIT* aCommit )
    {
        // Default initialize the parameter argument to an empty ki_any
        ki::any a;

        return doRunAction( aAction, true, a, aCommit );
    }

    bool RunAction( const TOOL_ACTION& aAction )
    {
        // Default initialize the parameter argument to an empty ki_any
        ki::any a;

        return doRunAction( aAction, true, a, nullptr );
    }

    /**
     * Run the specified action after the current action (coroutine) ends.
     *
     * The common format for action names is "application.ToolName.Action".
     *
     * @note The type of the optional parameter must match exactly with the type the consuming
     *       action is expecting, otherwise an assert will occur when reading the parameter.
     *
     * @param aActionName is the name of action to be invoked.
     * @param aParam is an optional parameter that might be used by the invoked action. Its meaning
     *               depends on the action.
     * @return False if the action was not found.
     */
    template<typename T>
    bool PostAction( const std::string& aActionName, T aParam )
    {
        // Use a cast to ensure the proper type is stored inside the parameter
        ki::any a( static_cast<T>( aParam ) );

        return doRunAction( aActionName, false, a, nullptr );
    }

    bool PostAction( const std::string& aActionName )
    {
        // Default initialize the parameter argument to an empty ki_any
        ki::any a;

        return doRunAction( aActionName, false, a, nullptr );
    }

    /**
     * Run the specified action after the current action (coroutine) ends.
     *
     * @nite The type of the optional parameter must match exactly with the type the consuming
     *       action is expecting, otherwise an assert will occur when reading the parameter.
     *
     * @param aAction is the action to be invoked.
     * @param aParam is an optional parameter that might be used by the invoked action. Its meaning
     *               depends on the action.  The parameter is not allowed to be a COMMIT as we have
     *               no way of guaranteeing the lifetime of the COMMIT object.
     */
    template<typename T, std::enable_if_t<!std::is_convertible_v<T, COMMIT*>>* = nullptr>
    bool PostAction( const TOOL_ACTION& aAction, T aParam )
    {
        // Use a cast to ensure the proper type is stored inside the parameter
        ki::any a( static_cast<T>( aParam ) );

        return doRunAction( aAction, false, a, nullptr );
    }

    void PostAction( const TOOL_ACTION& aAction )
    {
        // Default initialize the parameter argument to an empty ki_any
        ki::any a;

        doRunAction( aAction, false, a, nullptr );
    }

    // TODO: we need a way to guarantee that user events can't be processed in
    // between API actions.  Otherwise the COMMITs will get tangled up and we'll
    // crash on bad pointers.
    bool PostAPIAction( const TOOL_ACTION& aAction, COMMIT* aCommit )
    {
        // Default initialize the parameter argument to an empty ki_any
        ki::any a;

        return doRunAction( aAction, false, a, aCommit, true );
    }

    /**
     * Send a cancel event to the tool currently at the top of the tool stack.
     */
    void CancelTool();

    /**
     * "Prime" a tool by sending a cursor left-click event with the mouse position set
     * to the passed in position.
     *
     * @param aPosition is the mouse position to use in the event
     */
    void PrimeTool( const VECTOR2D& aPosition );

    /// @copydoc ACTION_MANAGER::GetHotKey()
    int GetHotKey( const TOOL_ACTION& aAction ) const;

    ACTION_MANAGER* GetActionManager() const { return m_actionMgr; }

    /**
     * Search for a tool with given ID.
     *
     * @param aId is the ID number of the requested tool.
     * @return Pointer to the requested tool or NULL in case of failure.
     */
    TOOL_BASE* FindTool( int aId ) const;

    /**
     * Search for a tool with given name.
     *
     * @param aName is the name of the requested tool.
     * @return Pointer to the requested tool or NULL in case of failure.
     */
    TOOL_BASE* FindTool( const std::string& aName ) const;

    /*
     * Return the tool of given type or nullptr if there is no such tool registered.
     */
    template<typename T>
    T* GetTool()
    {
        std::map<const char*, TOOL_BASE*>::iterator tool = m_toolTypes.find( typeid( T ).name() );

        if( tool != m_toolTypes.end() )
            return static_cast<T*>( tool->second );

        return nullptr;
    }

    /*
     * Return all registered tools.
     */
    std::vector<TOOL_BASE*> Tools() { return m_toolOrder; }

    /**
     * Deactivate the currently active tool.
     */
    void DeactivateTool();


    /**
     * Return true if a tool with given id is active (executing)
     */
     bool IsToolActive( TOOL_ID aId ) const;


    /**
     * Reset all tools (i.e. calls their Reset() method).
     */
    void ResetTools( TOOL_BASE::RESET_REASON aReason );

    /**
     * Initialize all registered tools.
     *
     * If a tool fails during the initialization, it is deactivated and becomes unavailable
     * for further use. Initialization should be done only once.
     */
    void InitTools();

    /**
     * Propagate an event to tools that requested events of matching type(s).
     *
     * @param aEvent is the event to be processed.
     * @return true if the event is a managed hotkey
     */
    bool ProcessEvent( const TOOL_EVENT& aEvent );

    /**
     * Put an event to the event queue to be processed at the end of event processing cycle.
     *
     * @param aEvent is the event to be put into the queue.
     */
    void PostEvent( const TOOL_EVENT& aEvent );

    /**
     * Set the work environment (model, view, view controls and the parent window).
     *
     * These are made available to the tool. Called by the parent frame when it is set up.
     */
    void SetEnvironment( EDA_ITEM* aModel, KIGFX::VIEW* aView,
                         KIGFX::VIEW_CONTROLS* aViewControls, APP_SETTINGS_BASE* aSettings,
                         TOOLS_HOLDER* aFrame );

    /*
     * Accessors for the environment objects (view, model, etc.)
     */
    KIGFX::VIEW* GetView() const { return m_view; }

    KIGFX::VIEW_CONTROLS* GetViewControls() const { return m_viewControls; }

    VECTOR2D GetMousePosition() const;
    VECTOR2D GetCursorPosition() const;

    EDA_ITEM* GetModel() const { return m_model; }
    void      ClearModel() { m_model = nullptr; }

    APP_SETTINGS_BASE* GetSettings() const { return m_settings; }

    TOOLS_HOLDER* GetToolHolder() const { return m_frame; }

    /**
     * Return id of the tool that is on the top of the active tools stack (was invoked the
     * most recently).
     *
     * @return Id of the currently used tool.
     */
    inline int GetCurrentToolId() const
    {
        return m_activeTools.empty() ? -1 : m_activeTools.front();
    }

    /**
     * Return the tool that is on the top of the active tools stack (was invoked the most
     * recently).
     *
     * @return Pointer to the currently used tool.
     */
    inline TOOL_BASE* GetCurrentTool() const
    {
        return FindTool( GetCurrentToolId() );
    }

    /**
     * Return the #TOOL_STATE object representing the state of the active tool. If there are no
     * tools active, it returns nullptr.
     */
    TOOL_STATE* GetCurrentToolState() const
    {
        auto it = m_toolIdIndex.find( GetCurrentToolId() );
        return ( it != m_toolIdIndex.end() ) ? it->second : nullptr;
    }

    /**
     * Return priority of a given tool.
     *
     * Higher number means that the tool is closer to the beginning of the active tools
     * queue (i.e. receives events earlier, tools with lower priority receive events later).
     *
     * @param aToolId is the id of queried tool.
     * @return The priority of a given tool. If returned number is negative, then it means that
     *         the tool id is invalid or the tool is not active.
     */
    int GetPriority( int aToolId ) const;

    /**
     * Define a state transition.
     *
     * The events that cause a given handler method in the tool to be called. Called by
     * TOOL_INTERACTIVE::Go(). May be called from a coroutine context.
     */
    void ScheduleNextState( TOOL_BASE* aTool, TOOL_STATE_FUNC& aHandler,
                            const TOOL_EVENT_LIST& aConditions );

    /**
     * Clear the state transition map for a tool.
     *
     * @param aTool is the tool that should have the transition map cleared.
     */
    void ClearTransitions( TOOL_BASE* aTool );

    void RunMainStack( TOOL_BASE* aTool, std::function<void()> aFunc );

    /**
     * Update the status bar and synchronizes toolbars.
     */
    void UpdateUI( const TOOL_EVENT& aEvent );

    /**
     * Pause execution of a given tool until one or more events matching aConditions arrives.
     *
     * The pause/resume operation is done through #COROUTINE object.  Called only from coroutines.
     */
    TOOL_EVENT* ScheduleWait( TOOL_BASE* aTool, const TOOL_EVENT_LIST& aConditions );

    /**
     * Set behavior of the tool's context popup menu.
     *
     * @param aTool is the parent tool.
     * @param aMenu is the menu structure, defined by the tool.
     * @param aTrigger determines when the menu is activated:
     *  CMENU_NOW: opens the menu right now
     *  CMENU_BUTTON: opens the menu when RMB is pressed
     *  CMENU_OFF: menu is disabled.
     * May be called from a coroutine context.
     */
    void ScheduleContextMenu( TOOL_BASE* aTool, ACTION_MENU* aMenu, CONTEXT_MENU_TRIGGER aTrigger );

    /**
     * Return the view controls settings for the current tool or the general settings if there is
     * no active tool.
     */
    const KIGFX::VC_SETTINGS& GetCurrentToolVC() const;

    /**
     * True while processing a context menu.
     */
    bool IsContextMenuActive() const
    {
        return m_menuActive;
    }

    /**
     * Disable mouse warping after the current context menu is closed.
     *
     * This must be called before invoking each context menu.  It's a good idea to call this
     * from non-modal dialogs (e.g. DRC window).
     */
    void VetoContextMenuMouseWarp()
    {
        m_warpMouseAfterContextMenu = false;
    }

    /**
     * Normally we warp the mouse after the context menu action runs.  This works fine for
     * something like the properties dialog, where you can continue drawing after the warp.
     * However, if the menu command itself is a drawing action (such as "Attempt Finish",
     * it will run before the warp (ie: from the position the command was in on the context
     * menu).  This call allows an action to perform the warp before it is run.
     */
    void WarpAfterContextMenu();

    /**
     * Handle context menu related events.
     */
    void DispatchContextMenu( const TOOL_EVENT& aEvent );

    /**
     * Handle specific events, that are intended for TOOL_MANAGER rather than tools.
     *
     * @param aEvent is the event to be processed.
     * @return true if the event was processed and should not go any further.
     */
    bool DispatchHotKey( const TOOL_EVENT& aEvent );

    VECTOR2D GetMenuCursorPos() const
    {
        return m_menuCursor;
    }

private:
    typedef std::pair<TOOL_EVENT_LIST, TOOL_STATE_FUNC> TRANSITION;

    /**
     * Helper function to actually run an action.
     */
    bool doRunAction( const TOOL_ACTION& aAction, bool aNow, const ki::any& aParam,
                      COMMIT* aCommit, bool aFromAPI = false );
    bool doRunAction( const std::string& aActionName, bool aNow, const ki::any& aParam,
                      COMMIT* aCommit );

    /**
     * Pass an event at first to the active tools, then to all others.
     */
    bool dispatchInternal( TOOL_EVENT& aEvent );

    /**
     * Check if it is a valid activation event and invokes a proper tool.
     *
     * @param aEvent is an event to be tested.
     * @return True if a tool was invoked, false otherwise.
     */
    bool dispatchActivation( const TOOL_EVENT& aEvent );

    /**
     * Invoke a tool by sending a proper event (in contrary to runTool, which makes the tool run
     * for real).
     *
     * @param aTool is the tool to be invoked.
     */
    bool invokeTool( TOOL_BASE* aTool );

    /**
     * Make a tool active, so it can receive events and react to them.
     *
     * The activated tool is pushed on the active tools stack, so the last activated tool
     * receives events first.
     *
     * @param aTool is the tool to be run.
     */
    bool runTool( TOOL_BASE* aTool );

    /**
     * Deactivate a tool and does the necessary clean up.
     *
     * @param aState is the state variable of the tool to be stopped.
     * @return m_activeTools iterator. If the tool has been completely deactivated, it points
     *         to the next active tool on the list. Otherwise it is an iterator pointing to
     *         \a aState.
     */
    ID_LIST::iterator finishTool( TOOL_STATE* aState );

    /**
     * Return information about a tool registration status.
     *
     * @param aTool is the tool to be checked.
     * @return true if the tool is in the registered tools list, false otherwise.
     */
    bool isRegistered( TOOL_BASE* aTool ) const
    {
        return m_toolState.count( aTool ) > 0;
    }

    /**
     * Return information about a tool activation status.
     *
     * @param aTool is the tool to be checked.
     * @return True if the tool is on the active tools stack, false otherwise.
     */
    bool isActive( TOOL_BASE* aTool ) const;

    /**
     * Save the #VIEW_CONTROLS settings to the tool state object.
     *
     * If #VIEW_CONTROLS settings are affected by #TOOL_MANAGER, the original settings are saved.
     */
    void saveViewControls( TOOL_STATE* aState );

    /**
     * Apply #VIEW_CONTROLS settings stored in a #TOOL_STATE object.
     */
    void applyViewControls( const TOOL_STATE* aState );

    /**
     * Main function for event processing.
     *
     * @return true if a hotkey was handled.
     */
    bool processEvent( const TOOL_EVENT& aEvent );

    /**
     * Save the previous active state and sets a new one.
     *
     * @param aState is the new active state. Might be null to indicate there is no new
     *               active state.
     */
    void setActiveState( TOOL_STATE* aState );

private:
    /// List of tools in the order they were registered.
    std::vector<TOOL_BASE*> m_toolOrder;

    /// Index of registered tools current states, associated by tools' objects.
    TOOL_STATE_MAP m_toolState;

    /// Index of the registered tools current states, associated by tools' names.
    NAME_STATE_MAP m_toolNameIndex;

    /// Index of the registered tools current states, associated by tools' ID numbers.
    ID_STATE_MAP m_toolIdIndex;

    /// Index of the registered tools to easily lookup by their type.
    std::map<const char*, TOOL_BASE*> m_toolTypes;

    /// Stack of the active tools.
    ID_LIST m_activeTools;

    /// Instance of ACTION_MANAGER that handles TOOL_ACTIONs.
    ACTION_MANAGER* m_actionMgr;

    /// Original cursor position, if overridden by the context menu handler.
    std::map<TOOL_ID, std::optional<VECTOR2D>> m_cursorSettings;

    EDA_ITEM*             m_model;
    KIGFX::VIEW*          m_view;
    KIGFX::VIEW_CONTROLS* m_viewControls;
    TOOLS_HOLDER*         m_frame;
    APP_SETTINGS_BASE*    m_settings;

    /// Queue that stores events to be processed at the end of the event processing cycle.
    std::list<TOOL_EVENT> m_eventQueue;

    /// Right click context menu position.
    VECTOR2D m_menuCursor;

    bool m_warpMouseAfterContextMenu;

    /// Flag indicating whether a context menu is currently displayed.
    bool m_menuActive;

    /// Tool currently displaying a popup menu. It is negative when there is no menu displayed.
    TOOL_ID m_menuOwner;

    /// Pointer to the state object corresponding to the currently executed tool.
    TOOL_STATE* m_activeState;

    /// True if the tool manager is shutting down (don't process additional events)
    bool m_shuttingDown;
};

#endif // TOOL_MANAGER_H
