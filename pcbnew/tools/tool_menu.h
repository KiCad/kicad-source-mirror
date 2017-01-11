/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 KiCad Developers, see CHANGELOG.txt for contributors.
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

#ifndef TOOLS_TOOL_MENU__H_
#define TOOLS_TOOL_MENU__H_

#include "conditional_menu.h"
#include "pcb_tool.h"

#include <vector>
#include <memory>

class CONTEXT_MENU;

/**
 * Class TOOL_MENU
 *
 * Manages a CONDITIONAL_MENU and some number of
 * CONTEXT_MENUs as sub-menus
 *
 * Each "top-level" interactive tool can have one of these,
 * and other tools can contribute CONTEXT_MENUS to it.
 *
 * There are also helper functions for adding common sets of
 * menu items, for example zoom and grid controls.
 */
class TOOL_MENU
{
public:

    /**
     * Function TOOL_MENU
     *
     * Construct a new TOOL_MENU for a specific tool. This menu
     * will be empty - it's up to the caller to add the relevant
     * items. This can be done directy, using the reference returned
     * by TOOL_MENU::GetMenu(), or the helpers for common command sets
     * can be used, or a combination of the two.
     */
    TOOL_MENU( TOOL_INTERACTIVE& aTool );

    /**
     * Destructor, also destructs any submenus created with
     * TOOL_MENU::CreateSubMenu().
     */
    ~TOOL_MENU();

    /**
     * Function GetMenu
     *
     * @return reference to the CONDITIONAL_MENU model, which can be
     * used tby tools to add their own commands to the menu.
     */
    CONDITIONAL_MENU& GetMenu();

    /**
     * Function CreateSubMenu
     *
     * Store a submenu of this menu model. This can be shared with
     * other menu models.
     *
     * It is the callers responsibility to add the submenu to
     * m_menu (via GetMenu() ) in the right way, as well
     * as to set the tool with SetTool(), since it's not a given
     * that the menu's tool is the tool that directly owns this
     * TOOL_MENU
     *
     * @param aSubMenu: a sub menu to add
     */
    void AddSubMenu( std::shared_ptr<CONTEXT_MENU> aSubMenu );

    /**
     * Function ShowContextMenu
     *
     * Helper function to set and immediately show a CONTEXT_MENU
     * based on the internal CONDITIONAL_MENU in concert with
     * the given SELECTION
     *
     * You don't have to use this function, if the caller has a
     * different way to show the menu, it can create one from
     * the reference returned by TOOL_MENU::GetMenu(), but it will
     * have to be managed externally to this class.
     */
    void ShowContextMenu( SELECTION& aSelection );

    /**
     * Function ShowContextMenu
     *
     * Helper function to show a context menu without any selection
     * for tools that can't make selections.
     */
    void ShowContextMenu();

    /**
     * Function CloseContextMenu
     *
     * Helper function to close a menu previously opened with
     * ShowContextMenu(), if a suitable event is received
     */
    void CloseContextMenu( OPT_TOOL_EVENT& evt );

    /**
     * Function CreateBasicMenu
     *
     * Construct a "basic" menu for a tool, containing only items
     * that apply to all tools (e.g. zoom and grid)
     */
    void AddStandardSubMenus( EDA_DRAW_FRAME& aFrame );

private:

    /*!
     * Helper function for factories to abe able to easily add
     * their own new sub menus. This sets the tool to the TOOL_MENUs
     * owner and adds to the store.
     *
     * Note, this won't share the menu between multiple invocations
     * of the factory. But if different top-level tools are using the
     * same factory, which one would be used for SetTool()?
     */
    template <typename T, typename ... Args>
    std::shared_ptr<T> createOwnSubMenu( Args&& ... args )
    {
        auto subMenuPtr = std::make_shared<T>( args ... );

        subMenuPtr->SetTool( &m_tool );
        AddSubMenu( subMenuPtr );

        return subMenuPtr;
    }

    /**
     * The conditional model of the menu displayed by the tool
     */
    CONDITIONAL_MENU m_menu;

    /**
     * The actual menu displayed by the tool
     */
    std::unique_ptr<CONTEXT_MENU> m_contextMenu;

    /**
     * The tool that owns this menu
     */
    TOOL_INTERACTIVE& m_tool;

    /**
     * Lifetime-managing container of submenus
     */
    std::vector<std::shared_ptr<CONTEXT_MENU> > m_subMenus;
};

#endif    // TOOLS_TOOL_MENU__H_
