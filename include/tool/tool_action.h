/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2015 CERN
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
 *
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

#ifndef __TOOL_ACTION_H
#define __TOOL_ACTION_H

#include <string>
#include <cassert>

#include <tool/tool_event.h>

struct BITMAP_OPAQUE;

/**
 * Represent a single user action.
 *
 * For instance:
 * - changing layer to top by pressing PgUp
 * - running the DRC from the menu
 * and so on, and so forth....
 *
 * Action class groups all necessary properties of an action, including explanation,
 * icons, hotkeys, menu items, etc.
 */
class TOOL_ACTION
{
public:
    TOOL_ACTION( const std::string& aName, TOOL_ACTION_SCOPE aScope = AS_CONTEXT,
                 int aDefaultHotKey = 0, const std::string& aLegacyHotKeyName = "",
                 const wxString& aMenuText = wxEmptyString,
                 const wxString& aTooltip = wxEmptyString,
                 const BITMAP_OPAQUE* aIcon = nullptr, TOOL_ACTION_FLAGS aFlags = AF_NONE,
                 void* aParam = nullptr );

    ~TOOL_ACTION();

    // TOOL_ACTIONS are singletons; don't be copying them around....
    TOOL_ACTION( const TOOL_ACTION& ) = delete;
    TOOL_ACTION& operator= ( const TOOL_ACTION& ) = delete;

    bool operator==( const TOOL_ACTION& aRhs ) const
    {
        return m_id == aRhs.m_id;
    }

    bool operator!=( const TOOL_ACTION& aRhs ) const
    {
        return m_id != aRhs.m_id;
    }

    /**
     * Return name of the action.
     *
     * It is the same one that is contained in #TOOL_EVENT that is sent by activating the
     * TOOL_ACTION.  Convention is "app.tool.actionName".
     *
     * @return Name of the action.
     */
    const std::string& GetName() const { return m_name; }

    /**
     * Return the default hotkey (if any) for the action.
     */
    int GetDefaultHotKey() const { return m_defaultHotKey; }

    /**
     * Return the hotkey keycode which initiates the action.
     */
    int GetHotKey() const { return m_hotKey; }
    void SetHotKey( int aKeycode );

    /**
     * Return the unique id of the TOOL_ACTION object.
     *
     * It is valid only after registering the TOOL_ACTION by #ACTION_MANAGER.
     *
     * @return The unique identification number. If the number is negative, then it is not valid.
     */
    int GetId() const { return m_id; }

    /*
     * Get the unique ID for this action in the user interface system.
     *
     * This is simply the action ID offset by @c ACTION_BASE_UI_ID.
     *
     * @return The unique ID number for use in the user interface system.
     */
    int GetUIId() const { return m_id + ACTION_BASE_UI_ID; }

    /*
     * Get the base value used to offset the user interface IDs for the actions.
     */
    static int GetBaseUIId() { return ACTION_BASE_UI_ID; }

    /**
     * Return the event associated with the action (i.e. the event that will be sent after
     * activating the action).
     */
    TOOL_EVENT MakeEvent() const
    {
        if( IsActivation() )
            return TOOL_EVENT( TC_COMMAND, TA_ACTIVATE, m_name, m_scope, m_param );
        else if( IsNotification() )
            return TOOL_EVENT( TC_MESSAGE, TA_NONE, m_name, m_scope, m_param );
        else
            return TOOL_EVENT( TC_COMMAND, TA_ACTION, m_name, m_scope, m_param );
    }

    wxString GetLabel() const;
    wxString GetMenuItem() const;
    wxString GetDescription( bool aIncludeHotkey = true ) const;

    TOOL_ACTION_SCOPE GetScope() const { return m_scope; }

    void* GetParam() const { return m_param; }

    /**
     * Return name of the tool associated with the action. It is basically the action name
     * stripped of the last part (e.g. for "pcbnew.InteractiveDrawing.drawCircle" it is
     * "pcbnew.InteractiveDrawing").
     */
    std::string GetToolName() const;

    /**
     * Return true if the action is intended to activate a tool.
     */
    bool IsActivation() const
    {
        return m_flags & AF_ACTIVATE;
    }

    /**
     * Return true if the action is a notification.
     */
    bool IsNotification() const
    {
        return m_flags & AF_NOTIFY;
    }

    /**
     * Return an icon associated with the action.
     *
     * It is used in context menu.
     */
    const BITMAP_OPAQUE* GetIcon() const
    {
        return m_icon;
    }

protected:
    TOOL_ACTION();

    friend class ACTION_MANAGER;

    ///< Base ID to use inside the user interface system to offset the action IDs.
    static constexpr int ACTION_BASE_UI_ID = 20000;

    ///< Name of the action (convention is "app.tool.actionName")
    std::string          m_name;
    TOOL_ACTION_SCOPE    m_scope;

    const int            m_defaultHotKey;  // Default hot key
    int                  m_hotKey;         // The current hotkey (post-user-settings-application)
    const std::string    m_legacyName;     // Name for reading legacy hotkey settings

    wxString             m_label;
    wxString             m_tooltip;
    const BITMAP_OPAQUE* m_icon;           // Icon for the menu entry

    int                  m_id;             // Unique ID for maps. Assigned by ACTION_MANAGER.

    TOOL_ACTION_FLAGS    m_flags;
    void*                m_param;          // Generic parameter
};

#endif
