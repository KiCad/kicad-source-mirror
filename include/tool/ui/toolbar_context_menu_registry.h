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

#ifndef TOOLBAR_CONTEXT_MENU_REGISTRY_H_
#define TOOLBAR_CONTEXT_MENU_REGISTRY_H_

#include <functional>
#include <map>
#include <memory>
#include <string>

#include <kicommon.h>

class ACTION_MENU;
class TOOL_MANAGER;

/**
 * Registry for toolbar context menu factories.
 *
 * This allows context menus to be associated with actions and groups by name,
 * so that JSON-loaded toolbar configurations can get the same menus as
 * code-defined default configurations.
 */
class KICOMMON_API TOOLBAR_CONTEXT_MENU_REGISTRY
{
public:
    /// Factory function type: takes TOOL_MANAGER, returns owned ACTION_MENU
    using MENU_FACTORY = std::function<std::unique_ptr<ACTION_MENU>( TOOL_MANAGER* )>;

    /**
     * Register a context menu factory for an action.
     *
     * @param aActionName The action name (from TOOL_ACTION::GetName())
     * @param aFactory Factory function that creates the menu
     */
    static void RegisterMenuFactory( const std::string& aActionName, MENU_FACTORY aFactory );

    /**
     * Register a context menu factory for a toolbar group.
     *
     * @param aGroupName The group name (from TOOLBAR_GROUP_CONFIG)
     * @param aFactory Factory function that creates the menu
     */
    static void RegisterGroupMenuFactory( const std::string& aGroupName, MENU_FACTORY aFactory );

    /**
     * Get the menu factory for an action, if one is registered.
     *
     * @param aActionName The action name to look up
     * @return The factory function, or nullptr if not registered
     */
    static MENU_FACTORY GetMenuFactory( const std::string& aActionName );

    /**
     * Get the menu factory for a group, if one is registered.
     *
     * @param aGroupName The group name to look up
     * @return The factory function, or nullptr if not registered
     */
    static MENU_FACTORY GetGroupMenuFactory( const std::string& aGroupName );

private:
    // Use Meyer's singleton to prevent SIOF
    static std::map<std::string, MENU_FACTORY>& getActionMenus();
    static std::map<std::string, MENU_FACTORY>& getGroupMenus();
};

#endif /* TOOLBAR_CONTEXT_MENU_REGISTRY_H_ */
