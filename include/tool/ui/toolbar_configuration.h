/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Ian McInerney
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

#ifndef TOOLBAR_CONFIGURATION_H_
#define TOOLBAR_CONFIGURATION_H_

#include <string>
#include <vector>

#include <settings/json_settings.h>
#include <settings/parameters.h>
#include <tool/action_toolbar.h>
#include <tool/tool_action.h>
#include <tool/ui/toolbar_context_menu_registry.h>

enum class TOOLBAR_ITEM_TYPE
{
    TOOL,
    TB_GROUP,
    SPACER,
    CONTROL,
    SEPARATOR
};

class KICOMMON_API TOOLBAR_ITEM
{
public:
    TOOLBAR_ITEM() :
        m_Type( TOOLBAR_ITEM_TYPE::TOOL ),
        m_Size( 0 )
    { }

    TOOLBAR_ITEM( TOOLBAR_ITEM_TYPE aType ) :
        m_Type( aType ),
        m_Size( 0 )
    { }

    TOOLBAR_ITEM( TOOLBAR_ITEM_TYPE aType, int aSize ) :
        m_Type( aType ),
        m_Size( aSize )
    {
        wxASSERT( aType == TOOLBAR_ITEM_TYPE::SPACER );
    }

    TOOLBAR_ITEM( TOOLBAR_ITEM_TYPE aType, std::string aName ) :
        m_Type( aType ),
        m_Size( 0 )
    {
        if( aType == TOOLBAR_ITEM_TYPE::CONTROL )
            m_ControlName = aName;
        else if( aType == TOOLBAR_ITEM_TYPE::TOOL )
            m_ActionName = aName;
    }

public:
    TOOLBAR_ITEM_TYPE m_Type;

    // Control properties
    std::string m_ControlName;

    // Tool properties
    std::string m_ActionName;

    // Spacer properties
    int m_Size;

    // Group properties
    wxString                  m_GroupName;
    std::vector<TOOLBAR_ITEM> m_GroupItems;
};

// Forward declaration for use in TOOLBAR_ITEM_REF
class TOOLBAR_GROUP_CONFIG;

/**
 * Helper class returned by TOOLBAR_CONFIGURATION::AppendAction() to allow
 * chaining of context menu registration.
 */
class KICOMMON_API TOOLBAR_ITEM_REF
{
public:
    TOOLBAR_ITEM_REF( class TOOLBAR_CONFIGURATION& aParent, TOOLBAR_ITEM& aItem ) :
            m_parent( aParent ),
            m_item( aItem )
    {
    }

    /**
     * Associate a context menu factory with this action.
     *
     * The factory is registered globally so JSON-loaded configurations
     * will also get this menu.
     *
     * @param aFactory Factory function that creates the context menu
     * @return Reference to parent configuration for continued chaining
     */
    TOOLBAR_CONFIGURATION& WithContextMenu(
            TOOLBAR_CONTEXT_MENU_REGISTRY::MENU_FACTORY aFactory );

    /// Allow implicit conversion back to TOOLBAR_CONFIGURATION for chaining
    operator TOOLBAR_CONFIGURATION&() { return m_parent; }

    // Forwarding methods to allow continued chaining after WithContextMenu or directly
    TOOLBAR_ITEM_REF AppendAction( const std::string& aActionName );
    TOOLBAR_ITEM_REF AppendAction( const TOOL_ACTION& aAction );
    TOOLBAR_CONFIGURATION& AppendSeparator();
    TOOLBAR_CONFIGURATION& AppendSpacer( int aSize );
    TOOLBAR_CONFIGURATION& AppendGroup( const TOOLBAR_GROUP_CONFIG& aGroup );
    TOOLBAR_CONFIGURATION& AppendControl( const std::string& aControlName );
    TOOLBAR_CONFIGURATION& AppendControl( const ACTION_TOOLBAR_CONTROL& aControl );

private:
    TOOLBAR_CONFIGURATION& m_parent;
    TOOLBAR_ITEM&          m_item;
};

class KICOMMON_API TOOLBAR_GROUP_CONFIG
{
public:
    TOOLBAR_GROUP_CONFIG( const wxString& aName ) :
        m_groupName( aName )
    {
    }

    const wxString& GetName() const
    {
        return m_groupName;
    }

    TOOLBAR_GROUP_CONFIG& AddAction( std::string aActionName )
    {

        m_groupItems.emplace_back( TOOLBAR_ITEM_TYPE::TOOL, aActionName );
        return *this;
    }

    TOOLBAR_GROUP_CONFIG& AddAction( const TOOL_ACTION& aAction )
    {
        m_groupItems.emplace_back( TOOLBAR_ITEM_TYPE::TOOL, aAction.GetName() );
        return *this;
    }

    /**
     * Associate a context menu factory with this group.
     *
     * The menu will be available for all actions in the group.
     * The factory is registered globally so JSON-loaded configurations
     * will also get this menu.
     *
     * @param aFactory Factory function that creates the context menu
     * @return Reference for continued chaining
     */
    TOOLBAR_GROUP_CONFIG& AddContextMenu(
            TOOLBAR_CONTEXT_MENU_REGISTRY::MENU_FACTORY aFactory )
    {
        // Register the factory globally using the group name
        TOOLBAR_CONTEXT_MENU_REGISTRY::RegisterGroupMenuFactory(
                m_groupName.ToStdString(), std::move( aFactory ) );
        return *this;
    }

    std::vector<TOOLBAR_ITEM> GetGroupItems() const
    {
        return m_groupItems;
    }

public:
    // These are public to write the JSON, but are lower-cased to encourage people not to directly
    // access them and treat them as private.
    wxString                  m_groupName;
    std::vector<TOOLBAR_ITEM> m_groupItems;
};

class KICOMMON_API TOOLBAR_CONFIGURATION
{
public:

    TOOLBAR_CONFIGURATION() {}
    virtual ~TOOLBAR_CONFIGURATION() {}

    TOOLBAR_ITEM_REF AppendAction( const std::string& aActionName )
    {
        m_toolbarItems.emplace_back( TOOLBAR_ITEM_TYPE::TOOL, aActionName );
        return TOOLBAR_ITEM_REF( *this, m_toolbarItems.back() );
    }

    TOOLBAR_ITEM_REF AppendAction( const TOOL_ACTION& aAction )
    {
        m_toolbarItems.emplace_back( TOOLBAR_ITEM_TYPE::TOOL, aAction.GetName() );
        return TOOLBAR_ITEM_REF( *this, m_toolbarItems.back() );
    }

    TOOLBAR_CONFIGURATION& AppendSeparator()
    {
        m_toolbarItems.emplace_back( TOOLBAR_ITEM_TYPE::SEPARATOR );
        return *this;
    }

    TOOLBAR_CONFIGURATION& AppendSpacer( int aSize )
    {
        m_toolbarItems.emplace_back( TOOLBAR_ITEM_TYPE::SPACER, aSize );
        return *this;
    }

    TOOLBAR_CONFIGURATION& AppendGroup( const TOOLBAR_GROUP_CONFIG& aGroup )
    {
        TOOLBAR_ITEM item( TOOLBAR_ITEM_TYPE::TB_GROUP );
        item.m_GroupName = aGroup.GetName();
        item.m_GroupItems = aGroup.GetGroupItems();

        m_toolbarItems.push_back( item );
        return *this;
    }

    TOOLBAR_CONFIGURATION& AppendControl( const std::string& aControlName )
    {
        m_toolbarItems.emplace_back( TOOLBAR_ITEM_TYPE::CONTROL, aControlName );
        return *this;
    }

    TOOLBAR_CONFIGURATION& AppendControl( const ACTION_TOOLBAR_CONTROL& aControl )
    {
        m_toolbarItems.emplace_back( TOOLBAR_ITEM_TYPE::CONTROL, aControl.GetName() );
        return *this;
    }

    std::vector<TOOLBAR_ITEM> GetToolbarItems() const
    {
        return m_toolbarItems;
    }

    void Clear()
    {
        m_toolbarItems.clear();
    }

public:
    // These are public to write the JSON, but are lower-cased to encourage people not to directly
    // access them and treat them as private.
    std::vector<TOOLBAR_ITEM> m_toolbarItems;
};


enum class TOOLBAR_LOC
{
    LEFT = 0,       ///< Toolbar on the left side of the canvas
    RIGHT,          ///< Toolbar on the right side of the canvas
    TOP_MAIN,       ///< Toolbar on the top of the canvas
    TOP_AUX         ///< Toolbar on the top of the canvas
};

class KICOMMON_API TOOLBAR_SETTINGS : public JSON_SETTINGS
{
public:
    TOOLBAR_SETTINGS( const wxString& aFilename );

    virtual ~TOOLBAR_SETTINGS() {}

    /**
     * Get the default tools to show on the specified canvas toolbar.
     */
    virtual std::optional<TOOLBAR_CONFIGURATION> DefaultToolbarConfig( TOOLBAR_LOC aToolbar )
    {
        return std::nullopt;
    }

    /**
     * Get the tools to show on the specified canvas toolbar.
     *
     * Returns the user-configured tools, and if not customized, the default tools.
     */
    std::optional<TOOLBAR_CONFIGURATION> GetToolbarConfig( TOOLBAR_LOC aToolbar, bool aAllowCustom = true );

    /**
     * Get the stored configuration for the given toolbar.
     */
    std::optional<TOOLBAR_CONFIGURATION> GetStoredToolbarConfig( TOOLBAR_LOC aToolbar );

    /**
     * Set the stored configuration for the given toolbar.
     */
    void SetStoredToolbarConfig( TOOLBAR_LOC aToolbar, const TOOLBAR_CONFIGURATION& aConfig )
    {
        m_toolbars[aToolbar] = aConfig;
    }

protected:
    // The toolbars
    std::map<TOOLBAR_LOC, TOOLBAR_CONFIGURATION> m_toolbars;
};

#endif /* TOOLBAR_CONFIGURATION_H_ */
