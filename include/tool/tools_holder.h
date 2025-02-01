/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef  TOOL_HOLDER_H
#define  TOOL_HOLDER_H

#include <vector>
#include <tool/selection.h>
#include <settings/common_settings.h>

struct ACTION_CONDITIONS;

class ACTIONS;
class TOOL_ACTION;
class TOOL_DISPATCHER;
class TOOL_EVENT;
class TOOL_MANAGER;

/*
 * A mix-in class which allows its owner to hold a set of tools from the tool framework.
 *
 * This is just the framework; the owner is responsible for registering individual tools,
 * creating the dispatcher, etc.
 */
class TOOLS_HOLDER
{
public:
    TOOLS_HOLDER();

    virtual ~TOOLS_HOLDER() { }

    /**
     * Return the MVC controller.
     */
    TOOL_MANAGER* GetToolManager() const { return m_toolManager; }

    TOOL_DISPATCHER* GetToolDispatcher() const { return m_toolDispatcher; }

    /**
     * Register an action's update conditions with the UI layer to allow the UI to appropriately
     * display the state of its controls.
     *
     * @param aAction is the action to register.
     * @param aConditions are the UI conditions to use for the control states.
     */
    virtual void RegisterUIUpdateHandler( const TOOL_ACTION& aAction,
                                          const ACTION_CONDITIONS& aConditions );

    /**
     * Register a UI update handler for the control with ID @c aID.
     *
     * @param aID is the control ID to register the handler for.
     * @param aConditions are the UI conditions to use for the control states.
     */
    virtual void RegisterUIUpdateHandler( int aID, const ACTION_CONDITIONS& aConditions )
    {}

    /**
     * Unregister a UI handler for an action that was registered using @c RegisterUIUpdateHandler.
     *
     * @param aAction is the action to unregister the handler for.
     */
    virtual void UnregisterUIUpdateHandler( const TOOL_ACTION& aAction );

    /**
     * Unregister a UI handler for a given ID that was registered using @c RegisterUIUpdateHandler.
     *
     * @param aID is the control ID to unregister the handler for.
     */
    virtual void UnregisterUIUpdateHandler( int aID )
    {}

    /**
     * Get the current selection from the canvas area.
     *
     * @return the current selection.
     */
    virtual SELECTION& GetCurrentSelection()
    {
        return m_dummySelection;
    }

    /**
     * NB: the definition of "tool" is different at the user level.
     *
     * The implementation uses a single TOOL_BASE derived class to implement several user
     * "tools", such as rectangle and circle, or wire and bus.  So each user-level tool is
     * actually a #TOOL_ACTION.
     */

    /**
     * @brief Pushes a tool to the stack.
     *
     * @param aEvent The event that is starting the tool to be pushed to the stack.
     */
    virtual void PushTool( const TOOL_EVENT& aEvent );

    /**
     * @brief Pops a tool from the stack.
     *
     * @param aEvent The event that started the tool that was pushed to the stack.
     */
    virtual void PopTool( const TOOL_EVENT& aEvent );

    bool ToolStackIsEmpty() { return m_toolStack.empty(); }

    std::string CurrentToolName() const;
    bool IsCurrentTool( const TOOL_ACTION& aAction ) const;

    virtual void DisplayToolMsg( const wxString& msg ) {};

    virtual void ShowChangedLanguage();

    /**
     * Indicate that hotkeys should perform an immediate action even if another tool is
     * currently active.  If false, the first hotkey should select the relevant tool.
     */
    bool GetDoImmediateActions() const { return m_immediateActions; }

    /**
     * Indicates whether a drag should draw a selection rectangle or drag selected (or unselected)
     * objects.
     */
    MOUSE_DRAG_ACTION GetDragAction() const { return m_dragAction; }

    /**
     * Indicate that a move operation should warp the mouse pointer to the origin of the
     * move object.  This improves snapping, but some users are allergic to mouse warping.
     */
    bool GetMoveWarpsCursor() const { return m_moveWarpsCursor; }

#define ENVVARS_CHANGED  0x0001
#define TEXTVARS_CHANGED 0x0002
#define HOTKEYS_CHANGED  0x0004

    /**
     * Notification event that some of the common (suite-wide) settings have changed.
     * Update hotkeys, preferences, etc.
     */
    virtual void CommonSettingsChanged( int aFlags = 0 );

    /**
     * Canvas access.
     */
    virtual wxWindow* GetToolCanvas() const = 0;
    virtual void RefreshCanvas() { }

    virtual wxString ConfigBaseName() { return wxEmptyString; }

protected:
    TOOL_MANAGER*     m_toolManager;
    ACTIONS*          m_actions;
    TOOL_DISPATCHER*  m_toolDispatcher;

    SELECTION         m_dummySelection;     // Empty dummy selection

    std::vector<std::string> m_toolStack;   // Stack of user-level "tools".  This is NOT a
                                            // stack of TOOL instances, because somewhat
                                            // confusingly most TOOLs implement more than one
                                            // user-level tool.  A user-level tool actually
                                            // equates to an ACTION handler, so this stack
                                            // stores ACTION names.

    bool              m_immediateActions;   // Preference for immediate actions.  If false,
                                            // the first invocation of a hotkey will just
                                            // select the relevant tool rather than executing
                                            // the tool's action.
    MOUSE_DRAG_ACTION m_dragAction;         // DRAG_ANY/DRAG_SELECTED/SELECT.

    bool              m_moveWarpsCursor;    // cursor is warped to move/drag origin
};

#endif  // TOOL_HOLDER_H
