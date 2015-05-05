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

#ifndef __TOOL_ACTION_H
#define __TOOL_ACTION_H

#include <string>
#include <cassert>

#include <tool/tool_event.h>

struct BITMAP_OPAQUE;

/**
 * Class TOOL_ACTION
 *
 * Represents a single user action. For instance:
 * - changing layer to top by pressing PgUp
 * - running the DRC from the menu
 * and so on, and so forth....
 * Action class groups all necessary properties of an action, including explanation,
 * icons, hotkeys, menu items, etc.
 */
class TOOL_ACTION
{
public:
    TOOL_ACTION( const std::string& aName, TOOL_ACTION_SCOPE aScope = AS_CONTEXT,
            int aDefaultHotKey = 0, const wxString aMenuItem = wxEmptyString,
            const wxString& aMenuDesc = wxEmptyString, const BITMAP_OPAQUE* aIcon = NULL,
            TOOL_ACTION_FLAGS aFlags = AF_NONE, void* aParam = NULL );

    ~TOOL_ACTION();

    bool operator==( const TOOL_ACTION& aRhs ) const
    {
        return m_id == aRhs.m_id;
    }

    bool operator!=( const TOOL_ACTION& aRhs ) const
    {
        return m_id != aRhs.m_id;
    }

    /**
     * Function GetName()
     * Returns name of the action. It is the same one that is contained in TOOL_EVENT that is
     * sent by activating the TOOL_ACTION.
     *
     * @return Name of the action.
     */
    const std::string& GetName() const
    {
        return m_name;
    }

    /**
     * Function GetId()
     * Returns the unique id of the TOOL_ACTION object. It is valid only after registering the
     * TOOL_ACTION by ACTION_MANAGER.
     *
     * @return The unique identification number. If the number is negative, then it is not valid.
     */
    int GetId() const
    {
        return m_id;
    }

    /**
     * Function GetHotKey()
     * Returns the associated hot key.
     */
    int GetHotKey() const
    {
        return m_currentHotKey;
    }

    /**
     * Function HasHotKey()
     * Checks if the action has a hot key assigned.
     *
     * @return True if there is a hot key assigned, false otherwise.
     */
    bool HasHotKey() const
    {
        return m_currentHotKey != 0;
    }

    /**
     * Function MakeEvent()
     * Returns the event associated with the action (i.e. the event that will be sent after
     * activating the action).
     *
     * @return The event associated with the action.
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

    const wxString& GetMenuItem() const
    {
        return m_menuItem;
    }

    void SetMenuItem( const wxString& aItem )
    {
        m_menuItem = aItem;
    }

    const wxString& GetDescription() const
    {
        return m_menuDescription;
    }

    void SetDescription( const wxString& aDescription )
    {
        m_menuDescription = aDescription;
    }

    TOOL_ACTION_SCOPE GetScope() const
    {
        return m_scope;
    }

    /**
     * Returns name of the tool associated with the action. It is basically the action name
     * stripped of the last part (e.g. for "pcbnew.InteractiveDrawing.drawCircle" it is
     * "pcbnew.InteractiveDrawing").
     */
    std::string GetToolName() const;

    /**
     * Returns true if the action is intended to activate a tool.
     */
    bool IsActivation() const
    {
        return m_flags & AF_ACTIVATE;
    }

    /**
     * Returns true if the action is a notification.
     */
    bool IsNotification() const
    {
        return m_flags & AF_NOTIFY;
    }

    /**
     * Returns an icon associated with the action. It is used in context menu.
     */
    const BITMAP_OPAQUE* GetIcon() const
    {
        return m_icon;
    }

    /**
     * Creates a hot key code that refers to a legacy hot key setting, instead of a particular key.
     * @param aHotKey is an ID of hot key to be referred (see @hotkeys.h).
     */
    inline static int LegacyHotKey( int aHotKey )
    {
        assert( ( aHotKey & LEGACY_HK ) == 0 );

        return aHotKey | LEGACY_HK;
    }
private:
    friend class ACTION_MANAGER;

    /// Returns the hot key assigned in the object definition. It may refer to a legacy hot key setting
    /// (if LEGACY_HK flag is set).
    int getDefaultHotKey()
    {
        return m_defaultHotKey;
    }

    /// Changes the assigned hot key.
    void setHotKey( int aHotKey )
    {
        // it is not the right moment to use legacy hot key, it should be given in the object definition
        assert( ( aHotKey & LEGACY_HK ) == 0 );

        m_currentHotKey = aHotKey;
    }

    /// Name of the action (convention is: app.[tool.]action.name)
    std::string m_name;

    /// Scope of the action
    TOOL_ACTION_SCOPE m_scope;

    /// Default hot key that activates the action.
    const int m_defaultHotKey;

    /// Custom assigned hot key that activates the action.
    int m_currentHotKey;

    /// Menu entry text
    wxString m_menuItem;

    /// Pop-up help
    wxString m_menuDescription;

    // Icon for menu entry
    const BITMAP_OPAQUE* m_icon;

    /// Unique ID for fast matching. Assigned by ACTION_MANAGER.
    int m_id;

    /// Action flags
    TOOL_ACTION_FLAGS m_flags;

    /// Generic parameter
    void* m_param;

    /// Flag to determine the hot key settings is not a particular key, but a reference to legacy
    /// hot key setting.
    static const int LEGACY_HK = 0x800000;
};

#endif
