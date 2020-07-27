/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2017 CERN
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
#include <functional>

#include <wx/menu.h>
#include <wx/textentry.h>
#include <tool/tool_action.h>

class KIFACE_I;
class TOOL_INTERACTIVE;

/**
 * Defines the structure of a menu based on ACTIONs.
 */
class ACTION_MENU : public wxMenu
{
public:
    ///> Default constructor
    ACTION_MENU( bool isContextMenu, TOOL_INTERACTIVE* aTool = nullptr );

    ~ACTION_MENU() override;

    ACTION_MENU( const ACTION_MENU& aMenu ) = delete;
    ACTION_MENU& operator=( const ACTION_MENU& aMenu ) = delete;

    /**
     * Sets title for the menu. The title is shown as a text label shown on the top of
     * the menu.
     *
     * @param aTitle is the new title.
     */
    void SetTitle( const wxString& aTitle ) override;

    /**
     * Decides whether a title for a pop up menu should be displayed.
     */
    void DisplayTitle( bool aDisplay = true );

    /**
     * Assigns an icon for the entry.
     *
     * @param aIcon is the icon to be assigned. NULL is used to remove icon.
     */
    void SetIcon( const BITMAP_OPAQUE* aIcon );

    /**
     * Adds a wxWidgets-style entry to the menu. After highlighting/selecting the entry,
     * a wxWidgets event is generated.
     */
    wxMenuItem* Add( const wxString& aLabel, int aId, const BITMAP_OPAQUE* aIcon );
    wxMenuItem* Add( const wxString& aLabel, const wxString& aToolTip, int aId,
                     const BITMAP_OPAQUE* aIcon,  bool aIsCheckmarkEntry = false );

    /**
     * Adds an entry to the menu, basing on the TOOL_ACTION object. After selecting the entry,
     * a TOOL_EVENT command containing name of the action is sent.
     *
     * @param aAction is the action to be added to menu entry.
     */
    wxMenuItem* Add( const TOOL_ACTION& aAction, bool aIsCheckmarkEntry = false );

    /**
     * Adds an action menu as a submenu. The difference between this function and
     * wxMenu::AppendSubMenu() is the capability to handle icons.
     *
     * @param aMenu is the submenu to be added.
     */
    wxMenuItem* Add( ACTION_MENU* aMenu );

    /**
     * Add a standard close item to the menu with the accelerator key CTRL-W.
     * Emits the wxID_CLOSE event.
     *
     * @param aAppname is the application name to append to the tooltip
     */
    void AddClose( wxString aAppname = "" );

    /**
     * Adds either a standard Quit or Close item to the menu. If aKiface is NULL or in
     * single-instance then Quite (wxID_QUIT) is used, otherwise Close (wxID_CLOSE) is used.
     *
     * @param aAppname is the application name to append to the tooltip
     */
    void AddQuitOrClose( KIFACE_I* aKiface, wxString aAppname = "" );

    /**
     * Removes all the entries from the menu (as well as its title). It leaves the menu in the
     * initial state.
     */
    void Clear();

    /**
     * Returns true if the menu has any enabled items
     */
    bool HasEnabledItems() const;

    /**
     * Returns the position of selected item. If the returned value is negative, that means that
     * menu was dismissed.
     *
     * @return The position of selected item in the action menu.
     */
    inline int GetSelected() const
    {
        return m_selected;
    }

    /**
     * Runs update handlers for the menu and its submenus.
     */
    void UpdateAll();

    /**
     * Clears the dirty flag on the menu and all descendants.
     */
    void ClearDirty();
    void SetDirty();

    /**
     * Sets a tool that is the creator of the menu.
     *
     * @param aTool is the tool that created the menu.
     */
    void SetTool( TOOL_INTERACTIVE* aTool );

    /**
     * Creates a deep, recursive copy of this ACTION_MENU.
     */
    ACTION_MENU* Clone() const;

    void OnMenuEvent( wxMenuEvent& aEvent );
    void OnIdle( wxIdleEvent& event );

    static constexpr bool CHECK = true;

protected:
    ///> Returns an instance of this class. It has to be overridden in inheriting classes.
    virtual ACTION_MENU* create() const;

    ///> Returns an instance of TOOL_MANAGER class.
    TOOL_MANAGER* getToolManager() const;

    /**
     * Update menu state stub. It is called before a menu is shown, in order to update its state.
     * Here you can tick current settings, enable/disable entries, etc.
     */
    virtual void update()
    {
    }

    /**
     * Event handler stub. It should be used if you want to generate a TOOL_EVENT from a wxMenuEvent.
     * It will be called when a menu entry is clicked.
     */
    virtual OPT_TOOL_EVENT eventHandler( const wxMenuEvent& )
    {
        return OPT_TOOL_EVENT();
    }

    /**
     * Copies another menus data to this instance. Old entries are preserved, and ones form aMenu
     * are copied.
     */
    void copyFrom( const ACTION_MENU& aMenu );

protected:
    /**
     * Function appendCopy
     * Appends a copy of wxMenuItem.
     */
    wxMenuItem* appendCopy( const wxMenuItem* aSource );

    ///> Initializes handlers for events.
    void setupEvents();

    ///> Updates hot key settings for TOOL_ACTIONs in this menu.
    void updateHotKeys();

    ///> Traverses the submenus tree looking for a submenu capable of handling a particular menu
    ///> event. In case it is handled, it is returned the aToolEvent parameter.
    void runEventHandlers( const wxMenuEvent& aMenuEvent, OPT_TOOL_EVENT& aToolEvent );

    ///> Runs a function on the menu and all its submenus.
    void runOnSubmenus( std::function<void(ACTION_MENU*)> aFunction );

    ///> Checks if any of submenus contains a TOOL_ACTION with a specific ID.
    OPT_TOOL_EVENT findToolAction( int aId );

    bool    m_isForcedPosition;
    wxPoint m_forcedPosition;

    bool m_dirty;               // Menu requires update before display

    bool m_titleDisplayed;
    bool m_isContextMenu;

    ///> Menu title
    wxString m_title;

    ///> Optional icon
    const BITMAP_OPAQUE* m_icon;

    ///> Stores the id number of selected item.
    int m_selected;

    ///> Creator of the menu
    TOOL_INTERACTIVE* m_tool;

    ///> Associates tool actions with menu item IDs. Non-owning.
    std::map<int, const TOOL_ACTION*> m_toolActions;

    ///> List of submenus.
    std::list<ACTION_MENU*> m_submenus;

    friend class TOOL_INTERACTIVE;
};

#endif
