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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <tool/ui/toolbar_context_menu_registry.h>


std::map<std::string, TOOLBAR_CONTEXT_MENU_REGISTRY::MENU_FACTORY>&
TOOLBAR_CONTEXT_MENU_REGISTRY::getActionMenus()
{
    static std::map<std::string, MENU_FACTORY> s_actionMenus;
    return s_actionMenus;
}


std::map<std::string, TOOLBAR_CONTEXT_MENU_REGISTRY::MENU_FACTORY>&
TOOLBAR_CONTEXT_MENU_REGISTRY::getGroupMenus()
{
    static std::map<std::string, MENU_FACTORY> s_groupMenus;
    return s_groupMenus;
}


KICOMMON_API void TOOLBAR_CONTEXT_MENU_REGISTRY::RegisterMenuFactory( const std::string& aActionName,
                                                                      MENU_FACTORY aFactory )
{
    getActionMenus()[aActionName] = std::move( aFactory );
}


KICOMMON_API void TOOLBAR_CONTEXT_MENU_REGISTRY::RegisterGroupMenuFactory( const std::string& aGroupName,
                                                                           MENU_FACTORY aFactory )
{
    getGroupMenus()[aGroupName] = std::move( aFactory );
}


KICOMMON_API TOOLBAR_CONTEXT_MENU_REGISTRY::MENU_FACTORY
TOOLBAR_CONTEXT_MENU_REGISTRY::GetMenuFactory( const std::string& aActionName )
{
    auto& menus = getActionMenus();
    auto it = menus.find( aActionName );

    if( it != menus.end() )
        return it->second;

    return nullptr;
}


KICOMMON_API TOOLBAR_CONTEXT_MENU_REGISTRY::MENU_FACTORY
TOOLBAR_CONTEXT_MENU_REGISTRY::GetGroupMenuFactory( const std::string& aGroupName )
{
    auto& menus = getGroupMenus();
    auto it = menus.find( aGroupName );

    if( it != menus.end() )
        return it->second;

    return nullptr;
}
