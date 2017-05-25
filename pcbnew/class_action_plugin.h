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
 * @file  class_action_plugin.h
 * @brief Class PCBNEW_ACTION_PLUGINS
 */

#ifndef CLASS_ACTION_PLUGIN_H
#define CLASS_ACTION_PLUGIN_H
#include <vector>
#include <wxPcbStruct.h>

/**
 * Class ACTION_PLUGIN
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

public:
    ACTION_PLUGIN() : m_actionMenuId( 0 ) {}
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
 * Class ACTION_PLUGINS
 * Mainly static. Storing all plugins informations.
 */
class ACTION_PLUGINS
{
private:
    /**
     * ACTION_PLUGIN system wide static list
     */
    static std::vector<ACTION_PLUGIN*> m_actionsList;

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
    static ACTION_PLUGIN* GetAction( wxString aName );

    /**
     * Function SetActionMenu
     * Associate a menu id to an action plugin
     * @param aInded is the action index
     * @param idMenu is the associated menuitem id
     */
    static void SetActionMenu( int aIndex, int idMenu );


    /**
     * Function GetActionMenu
     * Provide menu id for a plugin index
     * @param aIndex is the action index
     * @return associated menuitem id
     */
    static int GetActionMenu( int aIndex );


    /**
     * Function GetActionByMenu
     * find action plugin associated to a menu id
     * @param aMenu is the menu id (defined with SetActionMenu)
     * @return the associated ACTION_PLUGIN (or null if not found)
     */
    static ACTION_PLUGIN* GetActionByMenu( int aMenu );


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
};

#endif /* PCBNEW_ACTION_PLUGINS_H */
