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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef ACTION_MANAGER_H_
#define ACTION_MANAGER_H_

#include <list>
#include <map>
#include <string>
#include <set>

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

    ACTION_CONDITIONS& Show( const SELECTION_CONDITION& aCondition )
    {
        showCondition = aCondition;
        return *this;
    }

    SELECTION_CONDITION checkCondition;     ///< Returns true if the UI control should be checked
    SELECTION_CONDITION enableCondition;    ///< Returns true if the UI control should be enabled
    SELECTION_CONDITION showCondition;      ///< Returns true if the UI control should be shown
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
