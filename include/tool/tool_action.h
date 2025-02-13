/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2023 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <bitset>
#include <cassert>
#include <optional>
#include <string>
#include <string_view>

#include <ki_any.h>
#include <wx/string.h>

class TOOL_EVENT;

enum class BITMAPS : unsigned int;

/// Scope of tool actions
enum TOOL_ACTION_SCOPE
{
    AS_CONTEXT = 1,  ///< Action belongs to a particular tool (i.e. a part of a pop-up menu)
    AS_ACTIVE,       ///< All active tools
    AS_GLOBAL        ///< Global action (toolbar/main menu event, global shortcut)
};

/// Flags for tool actions
enum TOOL_ACTION_FLAGS
{
    AF_NONE     = 0,
    AF_ACTIVATE = 1,    ///< Action activates a tool
    AF_NOTIFY   = 2     ///< Action is a notification (it is by default passed to all tools)
};

/// Flags that control how actions appear and interact on the toolbar
enum class TOOLBAR_STATE
{
    HIDDEN = 0,         ///< Action is hidden from the toolbar
    TOGGLE = 1,         ///< Action is a toggle button on the toolbar
    CANCEL = 2,         ///< Action can be cancelled by clicking the toolbar button again

    ENUM_LENGTH = 3
};

// The length of the TOOLBAR_STATE enum as a size_t (used for bitset creation)
const size_t gToolbarStateNumber = static_cast<size_t>( TOOLBAR_STATE::ENUM_LENGTH );

typedef std::bitset<gToolbarStateNumber> TOOLBAR_STATE_FLAGS;

/**
 * Define a group that can be used to group actions (and their events) of similar operations.
 */
class TOOL_ACTION_GROUP
{
public:
    TOOL_ACTION_GROUP( std::string aName ) :
        m_name( aName )
    {
        // Assign a unique group ID to each group
        static int groupIDs = 0;
        m_groupID = ++groupIDs;
    };

    TOOL_ACTION_GROUP( const TOOL_ACTION_GROUP& aOther )
    {
        // Ensure a copy of a group is exactly the same as this one to get
        // proper comparisons
        m_name    = aOther.GetName();
        m_groupID = aOther.GetGroupID();
    }

    int                GetGroupID() const { return m_groupID; }
    const std::string& GetName()    const { return m_name; }

    bool operator==( const TOOL_ACTION_GROUP& aOther ) const
    {
        return m_groupID == aOther.m_groupID;
    }

private:
    int         m_groupID;
    std::string m_name;
};

/**
 * Build up the properties of a TOOL_ACTION in an incremental manner that is static-construction
 * safe.
 *
 * Note: This is meant to be constructed and immediately passed into the TOOL_ACTION constructor.
 *       Construction should not be delayed, since this only retains pointers to the strings used.
 */
class TOOL_ACTION_ARGS
{
public:
    TOOL_ACTION_ARGS() = default;

    /**
     * The name of the action, the convention is "app.tool.actionName".
     *
     * This is a required property.
     */
    TOOL_ACTION_ARGS& Name( const std::string_view& aName )
    {
        m_name = aName;
        return *this;
    }

    TOOL_ACTION_ARGS& FriendlyName( const std::string_view& aName )
    {
        m_friendlyName = aName;
        return *this;
    }

    /**
     * The scope of the action.
     */
    TOOL_ACTION_ARGS& Scope( TOOL_ACTION_SCOPE aScope )
    {
        m_scope = aScope;
        return *this;
    }

    /**
     * The default hotkey to assign to the action.
     */
    TOOL_ACTION_ARGS& DefaultHotkey( int aDefaultHotkey )
    {
        m_defaultHotKey = aDefaultHotkey;
        return *this;
    }

    /**
     * The default alternate hotkey to assign to the action.
     */
    TOOL_ACTION_ARGS& DefaultHotkeyAlt( int aDefaultHotkeyAlt )
    {
        m_defaultHotKeyAlt = aDefaultHotkeyAlt;
        return *this;
    }

    /**
     * The legacy hotkey name from the old system.
     *
     * This property is only needed for existing actions and shouldn't be used in new actions.
     */
    TOOL_ACTION_ARGS& LegacyHotkeyName( const std::string_view& aLegacyName )
    {
        m_legacyName = aLegacyName;
        return *this;
    }

    /**
     *The string to use when displaying the action in a menu.
     */
    TOOL_ACTION_ARGS& MenuText( const std::string_view& aMenuText )
    {
        m_menuText = aMenuText;
        return *this;
    }

    /**
     * The string to use as a tooltip for the action in menus and toolbars.
     */
    TOOL_ACTION_ARGS& Tooltip( const std::string_view& aTooltip )
    {
        m_tooltip = aTooltip;
        return *this;
    }

    /**
     * The description of the action.
     */
    TOOL_ACTION_ARGS& Description( const std::string_view& aDescription )
    {
        m_description = aDescription;
        return *this;
    }

    /**
     * The bitmap to use as the icon for the action in toolbars and menus.
     */
    TOOL_ACTION_ARGS& Icon( BITMAPS aIcon )
    {
        m_icon = aIcon;
        return *this;
    }

    /**
     * Flags describing the type of the action.
     */
    TOOL_ACTION_ARGS& Flags( TOOL_ACTION_FLAGS aFlags )
    {
        m_flags = aFlags;
        return *this;
    }

    /**
     * Custom parameter to pass information to the tool.
     */
    template<typename T>
    TOOL_ACTION_ARGS& Parameter( T aParam )
    {
        m_param = aParam;
        return *this;
    }

    /**
     * The ID number to use for the action when interacting with any UI elements.
     */
    TOOL_ACTION_ARGS& UIId( int aUIId )
    {
        m_uiid = aUIId;
        return *this;
    }

    TOOL_ACTION_ARGS& Group( const TOOL_ACTION_GROUP& aGroup )
    {
        m_group = aGroup;
        return *this;
    }

    TOOL_ACTION_ARGS& ToolbarState( TOOLBAR_STATE aState )
    {
        m_toolbarState = TOOLBAR_STATE_FLAGS();
        m_toolbarState.value().set( static_cast<size_t>( aState ) );
        return *this;
    }

    TOOL_ACTION_ARGS& ToolbarState( std::initializer_list<TOOLBAR_STATE> aState )
    {
        m_toolbarState = TOOLBAR_STATE_FLAGS();

        for( auto flag : aState )
            m_toolbarState.value().set( static_cast<size_t>( flag ) );

        return *this;
    }

protected:
    // Let the TOOL_ACTION constructor have direct access to the members here
    friend class TOOL_ACTION;

    std::optional<std::string_view>     m_name;
    std::optional<std::string_view>     m_friendlyName;
    std::optional<TOOL_ACTION_SCOPE>    m_scope;
    std::optional<TOOL_ACTION_FLAGS>    m_flags;

    std::optional<int>                  m_uiid;

    std::optional<int>                  m_defaultHotKey;
    std::optional<int>                  m_defaultHotKeyAlt;
    std::optional<std::string_view>     m_legacyName;

    std::optional<std::string_view>     m_menuText;
    std::optional<std::string_view>     m_tooltip;
    std::optional<std::string_view>     m_description;

    std::optional<BITMAPS>              m_icon;

    std::optional<TOOL_ACTION_GROUP>    m_group;

    std::optional<TOOLBAR_STATE_FLAGS>  m_toolbarState;

    ki::any                             m_param;
};

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
    TOOL_ACTION( const TOOL_ACTION_ARGS& aArgs );
    TOOL_ACTION( const std::string& aName, TOOL_ACTION_SCOPE aScope = AS_CONTEXT,
                 int aDefaultHotKey = 0, const std::string& aLegacyHotKeyName = "",
                 const wxString& aMenuText = wxEmptyString,
                 const wxString& aTooltip = wxEmptyString,
                 BITMAPS aIcon = static_cast<BITMAPS>( 0 ), TOOL_ACTION_FLAGS aFlags = AF_NONE );

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
    int GetDefaultHotKeyAlt() const { return m_defaultHotKeyAlt; }

    /**
     * Return the hotkey keycode which initiates the action.
     */
    int GetHotKey() const { return m_hotKey; }
    int GetHotKeyAlt() const { return m_hotKeyAlt; }
    void SetHotKey( int aKeycode, int aKeycodeAlt = 0 );

    /**
     * Return the unique id of the TOOL_ACTION object.
     *
     * It is valid only after registering the TOOL_ACTION by #ACTION_MANAGER.
     *
     * @return The unique identification number. If the number is negative, then it is not valid.
     */
    int GetId() const { return m_id; }

    /**
     * Return true if this action has a custom UI ID set.
     */
    bool HasCustomUIId() const { return m_uiid.has_value(); }

    /**
     * Get the unique ID for this action in the user interface system.
     *
     * This can be either set to a specific ID during creation or computed
     * by offsetting the action ID by @c ACTION_BASE_UI_ID.
     *
     * @return The unique ID number for use in the user interface system.
     */
    int GetUIId() const { return m_uiid.value_or( m_id + ACTION_BASE_UI_ID ); }

    /**
     * Get the base value used to offset the user interface IDs for the actions.
     */
    static int GetBaseUIId() { return ACTION_BASE_UI_ID; }

    /**
     * Return the event associated with the action (i.e. the event that will be sent after
     * activating the action).
     */
    TOOL_EVENT MakeEvent() const;

    /**
     * Return the translated label for the action.
    */
    wxString GetMenuLabel() const;
    wxString GetMenuItem() const;
    wxString GetTooltip( bool aIncludeHotkey = true ) const;
    wxString GetButtonTooltip() const;
    wxString GetDescription() const;

    /**
     * Return the translated user-friendly name of the action.
     */
    wxString GetFriendlyName() const;

    TOOL_ACTION_SCOPE GetScope() const { return m_scope; }

    /**
     * Return a non-standard parameter assigned to the action.
     */
    template<typename T>
    T GetParam() const
    {
        wxASSERT_MSG( m_param.has_value(),
                      "Attempted to get a parameter from an action with no parameter." );

        T param;

        try
        {
            param = ki::any_cast<T>( m_param );
        }
        catch( const ki::bad_any_cast& e )
        {
            wxASSERT_MSG( false,
                          wxString::Format( "Requested parameter type %s from action with "
                                            "parameter type %s.",
                                            typeid(T).name(), m_param.type().name() ) );
        }

        return param;
    }

    const std::optional<TOOL_ACTION_GROUP> GetActionGroup() const { return m_group; }

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
    BITMAPS GetIcon() const
    {
        // The value of 0 corresponds to BITMAPS::INVALID_BITMAP, but is just
        // used here to make it so we don't need to include the full list in this
        // header.
        return m_icon.value_or( static_cast<BITMAPS>( 0 ) );
    }

    /**
     * Check if a specific toolbar state is required for this action.
     *
     * @param aState The state (from TOOLBAR_STATE enum) to check
     * @return true if the state should be used for this action
     */
    bool CheckToolbarState( TOOLBAR_STATE aState ) const
    {
        return m_toolbarState[static_cast<std::size_t>( aState )];
    }


protected:
    TOOL_ACTION();

    friend class ACTION_MANAGER;

    /// Base ID to use inside the user interface system to offset the action IDs.
    static constexpr int ACTION_BASE_UI_ID = 20000;

    /// Name of the action (convention is "app.tool.actionName")
    std::string          m_name;
    TOOL_ACTION_SCOPE    m_scope;

    std::optional<TOOL_ACTION_GROUP> m_group; ///< Optional group for the action to belong to.

    const int         m_defaultHotKey;    ///< Default hot key.
    const int         m_defaultHotKeyAlt; ///< Default hot key alternate.
    int               m_hotKey;           ///< The current hotkey (post-user-settings-application).

    /// The alternate hotkey (post-user-settings-application).
    int               m_hotKeyAlt;
    const std::string m_legacyName;       ///< Name for reading legacy hotkey settings.

    wxString                m_friendlyName; ///< User-friendly name.
    std::optional<wxString> m_menuLabel;    ///< Menu label.
    wxString                m_tooltip;      ///< User facing tooltip help text.
    std::optional<wxString> m_description;  ///< Description of the action.

    std::optional<BITMAPS> m_icon; ///< Icon for the menu entry

    int                m_id;   ///< Unique ID for maps. Assigned by #ACTION_MANAGER.
    std::optional<int> m_uiid; ///< ID to use when interacting with the UI (if empty, generate one).

    TOOLBAR_STATE_FLAGS m_toolbarState;    ///< Toolbar state behavior for the action

    TOOL_ACTION_FLAGS m_flags;
    ki::any           m_param; ///< Generic parameter.
};

#endif
