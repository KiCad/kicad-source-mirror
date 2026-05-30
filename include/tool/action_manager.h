/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef ACTION_MANAGER_H_
#define ACTION_MANAGER_H_

#include <list>
#include <map>
#include <optional>
#include <string>
#include <set>
#include <vector>

#include <frame_type.h>
#include <tool/selection_conditions.h>

class TOOL_BASE;
class TOOL_MANAGER;
class TOOL_ACTION;

/**
 * Functors that can be used to figure out how the action controls should be displayed in the UI
 * and if an action should be enabled given the current selection.
 *
 * @note @c checkCondition is also used for determining the state of a toggled toolbar item
 *       (the item is toggled when the condition is true).
 */
struct ACTION_CONDITIONS
{
    ACTION_CONDITIONS()
    {
        checkCondition  = SELECTION_CONDITIONS::ShowNever;      // Never check by default
        enableCondition = SELECTION_CONDITIONS::ShowAlways;     // Always enable by default
        showCondition   = SELECTION_CONDITIONS::ShowAlways;     // Always show by default
    }

    ACTION_CONDITIONS& Check( const SELECTION_CONDITION& aCondition )
    {
        checkCondition = aCondition;
        return *this;
    }

    ACTION_CONDITIONS& Enable( const SELECTION_CONDITION& aCondition )
    {
        enableCondition = aCondition;
        return *this;
    }

    /**
     * Set a separate condition for direct command dispatch (hotkeys and navlib buttons). When set,
     * it is used instead of enableCondition, allowing menu items to appear disabled while the
     * action still fires for immediate-mode operations like rotate and mirror.
     */
    ACTION_CONDITIONS& HotkeyEnable( const SELECTION_CONDITION& aCondition )
    {
        hotkeyCondition = aCondition;
        return *this;
    }

    ACTION_CONDITIONS& Show( const SELECTION_CONDITION& aCondition )
    {
        showCondition = aCondition;
        return *this;
    }

    /**
     * Return the condition that direct command dispatch should use, falling back to
     * enableCondition when no separate dispatch condition has been set.
     */
    const SELECTION_CONDITION& GetHotkeyCondition() const
    {
        if( hotkeyCondition )
            return *hotkeyCondition;

        return enableCondition;
    }

    SELECTION_CONDITION checkCondition;     ///< Returns true if the UI control should be checked
    SELECTION_CONDITION enableCondition;    ///< Returns true if the UI control should be enabled
    SELECTION_CONDITION showCondition;      ///< Returns true if the UI control should be shown

    /// Optional separate condition for hotkey dispatch (when empty, enableCondition is used)
    std::optional<SELECTION_CONDITION> hotkeyCondition;
};

/**
 * Manage #TOOL_ACTION objects.
 *
 * Registering them and allows one to run them using associated hot keys, names or ids.
 */
class ACTION_MANAGER
{
public:
    /**
     * @param aToolManager is a tool manager instance that is used to pass events to tools.
     */
    ACTION_MANAGER( TOOL_MANAGER* aToolManager );

    /**
     * Unregister every registered action.
     */
    ~ACTION_MANAGER();

    /**
     * Add a tool action to the manager and sets it up. After that it is possible to invoke
     * the action using hotkeys or sending a command event with its name.
     *
     * @param aAction: action to be added. Ownership is not transferred.
     */
    void RegisterAction( TOOL_ACTION* aAction );

    /**
     * Generate an unique ID from for an action with given name.
     */
    static int MakeActionId( const std::string& aActionName );

    /**
     * Get a list of currently-registered actions mapped by their name.
     */
    const std::map<std::string, TOOL_ACTION*>& GetActions() const;

    /**
     * Test if a UI ID corresponds to an action ID in our system.
     */
    bool IsActionUIId( int aId ) const;

    /**
     * Find an action with a given name (if there is one available).
     *
     * @param aActionName is the searched action.
     * @return Pointer to a TOOL_ACTION object or NULL if there is no such action.
     */
    TOOL_ACTION* FindAction( const std::string& aActionName ) const;

    /**
     * Run an action associated with a hotkey (if there is one available).
     *
     * @param aHotKey is the hotkey to be handled.
     * @return True if there was an action associated with the hotkey, false otherwise.
     */
    bool RunHotKey( int aHotKey ) const;

    /**
     * Return the action-name namespace prefix (e.g. "pcbnew.", "eeschema.") shared by the
     * actions native to a given frame, or an empty string when the frame has no namespace.
     */
    static std::string FrameNamespacePrefix( FRAME_T aFrameType );

    /**
     * Reorder global actions sharing a hotkey so that one native to the given frame is tried
     * first when the user has bound the matched slot away from its default.
     */
    static void PromoteUserBoundFrameAction( std::vector<const TOOL_ACTION*>& aGlobalActions, FRAME_T aFrameType,
                                             int aMatchedHotKey );

    /**
     * Return the hot key associated with a given action or 0 if there is none.
     *
     * @param aAction is the queried action.
     */
    int GetHotKey( const TOOL_ACTION& aAction ) const;

    /**
     * Optionally read the hotkey config files and then rebuilds the internal hotkey maps.
     */
    void UpdateHotKeys( bool aFullUpdate );

    /**
     * Return list of TOOL_ACTIONs.
     *
     * #TOOL_ACTIONs add themselves to the list upon their creation.
     *
     * @return List of TOOL_ACTIONs.
     */
    static std::list<TOOL_ACTION*>& GetActionList()
    {
        static std::list<TOOL_ACTION*> actionList;

        return actionList;
    }

    /**
     * Set the conditions the UI elements for activating a specific tool action should use
     * for determining the current UI state (e.g. checked, enabled, shown)
     *
     * @param aAction is the tool action using these conditions.
     * @param aConditions are the conditions to use for the action.
     */
    void SetConditions( const TOOL_ACTION& aAction, const ACTION_CONDITIONS& aConditions );

    /**
     * Get the conditions to use for a specific tool action.
     *
     * @param aAction is the tool action.
     * @return the action conditions, returns nullptr if no conditions are registered.
     */
    const ACTION_CONDITIONS* GetCondition( const TOOL_ACTION& aAction ) const;

private:
    // Resolve a hotkey by applying legacy and current settings over the action's
    // default hotkey.
    void processHotKey( TOOL_ACTION* aAction, const std::map<std::string, int>& aLegacyMap,
                        const std::map<std::string, std::pair<int, int>>& aHotKeyMap );

    ///< Tool manager needed to run actions
    TOOL_MANAGER* m_toolMgr;

    ///< Map for indexing actions by their names
    std::map<std::string, TOOL_ACTION*> m_actionNameIndex;

    ///< Map for recording actions that have custom UI IDs
    std::map<int, TOOL_ACTION*> m_customUIIdIndex;

    ///< Map for indexing actions by their hotkeys
    typedef std::map<int, std::list<TOOL_ACTION*> > HOTKEY_LIST;
    HOTKEY_LIST m_actionHotKeys;

    ///< Quick action<->hot key lookup
    std::map<int, int> m_hotkeys;

    /// Map the command ID that wx uses for the action to the UI conditions for the
    /// menu/toolbar items
    std::map<int, ACTION_CONDITIONS> m_uiConditions;
};

#endif /* ACTION_MANAGER_H_ */
