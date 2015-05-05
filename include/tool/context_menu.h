/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2015 CERN
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

#ifndef __CONTEXT_MENU_H
#define __CONTEXT_MENU_H

#include <map>
#include <list>
#include <boost/function.hpp>

#include <wx/menu.h>
#include <tool/tool_action.h>

class TOOL_INTERACTIVE;

/**
 * Class CONTEXT_MENU
 *
 * Defines the structure of a context (usually right-click) popup menu
 * for a given tool.
 */
class CONTEXT_MENU : public wxMenu
{
public:
    ///> Default constructor
    CONTEXT_MENU();

    ///> Copy constructor
    CONTEXT_MENU( const CONTEXT_MENU& aMenu );

    virtual ~CONTEXT_MENU();

    CONTEXT_MENU& operator=( const CONTEXT_MENU& aMenu );

    /**
     * Function SetTitle()
     * Sets title for the context menu. The title is shown as a text label shown on the top of
     * the menu.
     * @param aTitle is the new title.
     */
    void SetTitle( const wxString& aTitle );

    /**
     * Function SetIcon()
     * Assigns an icon for the entry.
     * @param aIcon is the icon to be assigned. NULL is used to remove icon.
     */
    inline void SetIcon( const BITMAP_OPAQUE* aIcon )
    {
        m_icon = aIcon;
    }

    /**
     * Function Add()
     * Adds an entry to the menu. After highlighting/selecting the entry, a TOOL_EVENT command is
     * sent that contains ID of the entry.
     * @param aLabel is the text label show in the menu.
     * @param aId is the ID that is sent in the TOOL_EVENT. It should be unique for every entry.
     * @param aIcon is an optional icon.
     */
    wxMenuItem* Add( const wxString& aLabel, int aId, const BITMAP_OPAQUE* aIcon = NULL );

    /**
     * Function Add()
     * Adds an entry to the menu, basing on the TOOL_ACTION object. After selecting the entry,
     * a TOOL_EVENT command containing name of the action is sent.
     * @param aAction is the action to be added to menu entry.
     */
    wxMenuItem* Add( const TOOL_ACTION& aAction );

    /**
     * Function Add()
     * Adds a context menu as a submenu. The difference between this function and wxMenu::AppendSubMenu()
     * is the capability to handle icons.
     * @param aMenu is the submenu to be added.
     * @param aLabel is the caption displayed for the menu entry.
     * @param aExpand allows to add all entries from the menu as individual entries rather than
     *                add everything as a submenu.
     */
    std::list<wxMenuItem*> Add( CONTEXT_MENU* aMenu, const wxString& aLabel, bool aExpand = false );

    /**
     * Function Clear()
     * Removes all the entries from the menu (as well as its title). It leaves the menu in the
     * initial state.
     */
    void Clear();

    /**
     * Function GetSelected()
     * Returns the position of selected item. If the returned value is negative, that means that
     * menu was dismissed.
     * @return The position of selected item in the context menu.
     */
    inline int GetSelected() const
    {
        return m_selected;
    }

    /**
     * Function UpdateAll()
     * Runs update handlers for the menu and its submenus.
     */
    void UpdateAll();

    typedef boost::function<OPT_TOOL_EVENT(const wxMenuEvent&)> MENU_HANDLER;
    typedef boost::function<void()> UPDATE_HANDLER;

    /**
     * Function SetMenuHandler()
     * Sets the menu event handler to another function.
     */
    inline void SetMenuHandler( MENU_HANDLER aMenuHandler )
    {
        m_menu_handler = aMenuHandler;
    }

    /**
     * Function SetUpdateHandler()
     * Sets the update handler to a different function.
     */
    inline void SetUpdateHandler( UPDATE_HANDLER aUpdateHandler )
    {
        m_update_handler = aUpdateHandler;
    }

private:
    // Empty stubs used by the default constructor
    static OPT_TOOL_EVENT menuHandlerStub(const wxMenuEvent& );
    static void updateHandlerStub();

    /**
     * Function appendCopy
     * Appends a copy of wxMenuItem.
     */
    wxMenuItem* appendCopy( const wxMenuItem* aSource );

    ///> Common part of copy constructor and assignment operator.
    void copyFrom( const CONTEXT_MENU& aMenu );

    ///> Initializes handlers for events.
    void setupEvents();

    ///> The default menu event handler.
    void onMenuEvent( wxMenuEvent& aEvent );

    /**
     * Function setTool()
     * Sets a tool that is the creator of the menu.
     * @param aTool is the tool that created the menu.
     */
    void setTool( TOOL_INTERACTIVE* aTool );

    ///> Traverses the submenus tree looking for a submenu capable of handling a particular menu
    ///> event. In case it is handled, it is returned the aToolEvent parameter.
    void runEventHandlers( const wxMenuEvent& aMenuEvent, OPT_TOOL_EVENT& aToolEvent );

    ///> Runs a function on the menu and all its submenus.
    void runOnSubmenus( boost::function<void(CONTEXT_MENU*)> aFunction );

    ///> Flag indicating that the menu title was set up.
    bool m_titleSet;

    ///> Stores the id number of selected item.
    int m_selected;

    ///> Creator of the menu
    TOOL_INTERACTIVE* m_tool;

    ///> Menu items with ID higher than that are considered TOOL_ACTIONs
    static const int ACTION_ID = 30000;

    ///> Associates tool actions with menu item IDs. Non-owning.
    std::map<int, const TOOL_ACTION*> m_toolActions;

    ///> List of submenus.
    std::list<CONTEXT_MENU*> m_submenus;

    ///> Parent CONTEXT_MENU.
    CONTEXT_MENU* m_parent;

    ///> Optional icon
    const BITMAP_OPAQUE* m_icon;

    ///> Optional callback to translate wxMenuEvents to TOOL_EVENTs.
    MENU_HANDLER m_menu_handler;

    ///> Optional callback to update the menu state before it is displayed.
    UPDATE_HANDLER m_update_handler;

    friend class TOOL_INTERACTIVE;
};

#endif
