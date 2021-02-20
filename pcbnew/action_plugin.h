/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 KiCad Developers, see AUTHORS.txt for contributors.
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


/**
 * @file  action_plugin.h
 * @brief Class PCBNEW_ACTION_PLUGINS
 */

#ifndef CLASS_ACTION_PLUGIN_H
#define CLASS_ACTION_PLUGIN_H
#include <vector>
#include <pcb_edit_frame.h>

/**
 * ACTION_PLUGIN
 * This is the parent class from where any action plugin class must
 * derive
 */
class ACTION_PLUGIN
{
public:
    // association between the plugin and its menu id
    // m_actionMenuId set to 0 means the corresponding menuitem to call this
    // action is not yet created
    int m_actionMenuId;
    // Same for button id
    int m_actionButtonId;
    // Icon for the action button and menu entry
    wxBitmap iconBitmap;
    // If show_on_toolbar is true a button will be added to top toolbar
    bool show_on_toolbar;

public:
    ACTION_PLUGIN() : m_actionMenuId( 0 ), m_actionButtonId( 0 ),
                      show_on_toolbar( false ) {}
    virtual ~ACTION_PLUGIN();

    /**
     * Function GetCategoryName
     * @return the category name of the action (to be able to group action under the same submenu)
     */
    virtual wxString GetCategoryName() = 0;

    /**
     * Function GetName
     * @return the name of the action
     */

    virtual wxString GetName() = 0;

    /**
     * Function GetDescription
     * @return a description of the action plugin
     */
    virtual wxString GetDescription() = 0;

    /**
     * Function GetShowToolbarButton
     * @return true if button should be shown on top toolbar
     */
    virtual bool GetShowToolbarButton() = 0;

    /**
     * Function GetIconFileName
     * @return a path to icon for the action plugin button
     */
    virtual wxString GetIconFileName() = 0;

    /**
     * Function GetPluginPath
     * @return a path this plugin was loaded from
     */
    virtual wxString GetPluginPath() = 0;

    /**
     * Function GetObject
     * This method gets the pointer to the object from where this action constructs
     * @return  it's a void pointer, as it could be a PyObject or any other
     */
    virtual void* GetObject() = 0;

    /**
     * Function Run
     * This method the the action
     */
    virtual void Run() = 0;

    /**
     * Function register_action
     * It's the standard method of a "ACTION_PLUGIN" to register itself into
     * the ACTION_PLUGINS singleton manager
     */
    void register_action();
};


/**
 * ACTION_PLUGINS
 * Mainly static. Storing all plugins informations.
 */
class ACTION_PLUGINS
{
private:
    /**
     * ACTION_PLUGIN system wide static list
     */
    static std::vector<ACTION_PLUGIN*> m_actionsList;
    static bool m_actionRunning;
public:
    /**
     * Function register_action
     * An action calls this static method when it wants to register itself
     * into the system actions
     *
     * @param aAction is the action plugin to be registered
     */
    static void register_action( ACTION_PLUGIN* aAction );

    /**
     * Function deregister_object
     * Anyone calls this method to deregister an object which builds a action,
     * it will lookup on the vector calling GetObject until find, then removed
     * and deleted
     *
     * @param aObject is the action plugin object to be deregistered
     */
    static bool deregister_object( void* aObject );

    /**
     * Function GetAction
     * @param aName is the action plugin name
     * @return a action object by it's name or NULL if it isn't available.
     */
    static ACTION_PLUGIN* GetAction( const wxString& aName );

    /**
     * Function SetActionMenu
     * Associate a menu id to an action plugin
     * @param aIndex is the action index
     * @param idMenu is the associated menuitem id
     */
    static void SetActionMenu( int aIndex, int idMenu );

    /**
     * Function GetActionByMenu
     * find action plugin associated to a menu id
     * @param aMenu is the menu id (defined with SetActionMenu)
     * @return the associated ACTION_PLUGIN (or null if not found)
     */
    static ACTION_PLUGIN* GetActionByMenu( int aMenu );

    /**
     * Function SetActionButton
     * Associate a button id to an action plugin
     * @param aAction is the action
     * @param idButton is the associated menuitem id
     */
    static void SetActionButton( ACTION_PLUGIN* aAction, int idButton );

    /**
     * Function GetActionByButton
     * find action plugin associated to a button id
     * @param aButton is the button id (defined with SetActionButton)
     * @return the associated ACTION_PLUGIN (or null if not found)
     */
    static ACTION_PLUGIN* GetActionByButton( int aButton );

    /**
     * Function GetActionByPath
     * find action plugin by module path
     * @param aPath the path of plugin
     * @return the corresponding ACTION_PLUGIN (or null if not found)
     */
    static ACTION_PLUGIN* GetActionByPath( const wxString& aPath );

    /**
     * Function GetAction
     * @return a action object by it's number or NULL if it isn't available.
     * @param  aIndex is the action index in list
     */
    static ACTION_PLUGIN* GetAction( int aIndex );

    /**
     * Function GetActionsCount
     * @return the number of actions available into the system
     */
    static int GetActionsCount();

    /**
     * Function IsActionRunning
     * @return Is an action running right now
     */
    static bool IsActionRunning();

    /**
     * Function SetActionRunning
     * @param aRunning sets whether an action is running now.
     */
    static void SetActionRunning( bool aRunning );

    /**
     * Unloads (deregisters) all action plugins
     */
    static void UnloadAll();
};

#endif /* PCBNEW_ACTION_PLUGINS_H */
