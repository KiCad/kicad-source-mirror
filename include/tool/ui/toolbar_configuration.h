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


class KICOMMON_API TOOLBAR_GROUP_CONFIG
{
public:

    TOOLBAR_GROUP_CONFIG( std::string aName ) :
        m_groupName( aName )
    {
        wxASSERT_MSG( aName.starts_with( "group" ), "Toolbar group names must start with \"group\"" );
    }

    const std::string& GetName() const
    {
        return m_groupName;
    }

    TOOLBAR_GROUP_CONFIG& AddAction( std::string aActionName )
    {
        m_groupItems.push_back( aActionName );
        return *this;
    }

    TOOLBAR_GROUP_CONFIG& AddAction( const TOOL_ACTION& aAction )
    {
        m_groupItems.push_back( aAction.GetName() );
        return *this;
    }

    std::vector<std::string> GetGroupItems() const
    {
        return m_groupItems;
    }

public:
    // These are public to write the JSON, but are lower-cased to encourage people not to directly
    // access them and treat them as private.
    std::string              m_groupName;
    std::vector<std::string> m_groupItems;
};

class KICOMMON_API TOOLBAR_CONFIGURATION
{
public:

    TOOLBAR_CONFIGURATION() {}
    virtual ~TOOLBAR_CONFIGURATION() {}

    TOOLBAR_CONFIGURATION& AppendAction( std::string aActionName )
    {
        m_toolbarItems.push_back( aActionName );
        return *this;
    }

    TOOLBAR_CONFIGURATION& AppendAction( const TOOL_ACTION& aAction )
    {
        m_toolbarItems.push_back( aAction.GetName() );
        return *this;
    }

    TOOLBAR_CONFIGURATION& AppendSeparator()
    {
        m_toolbarItems.push_back( "separator" );
        return *this;
    }

    TOOLBAR_CONFIGURATION& AppendSpacer( int aSize )
    {
        m_toolbarItems.push_back( "spacer:" + std::to_string( aSize )  );
        return *this;
    }

    TOOLBAR_CONFIGURATION& AppendGroup( const TOOLBAR_GROUP_CONFIG& aGroup )
    {
        m_toolbarGroups.push_back( aGroup );
        m_toolbarItems.push_back( aGroup.GetName() );
        return *this;
    }


    TOOLBAR_CONFIGURATION& AppendControl( std::string aControlName )
    {
        m_toolbarItems.push_back( aControlName );
        return *this;
    }

    TOOLBAR_CONFIGURATION& AppendControl( const ACTION_TOOLBAR_CONTROL& aControl )
    {
        m_toolbarItems.push_back( aControl.GetName() );
        return *this;
    }

    std::vector<std::string> GetToolbarItems() const
    {
        return m_toolbarItems;
    }

    const TOOLBAR_GROUP_CONFIG* GetGroup( const std::string& aGroupName ) const
    {
        for( const TOOLBAR_GROUP_CONFIG& group : m_toolbarGroups )
        {
            if( group.GetName() == aGroupName )
                return &group;
        }

        return nullptr;
    }

    void Clear()
    {
        m_toolbarItems.clear();
        m_toolbarGroups.clear();
    }

public:
    // These are public to write the JSON, but are lower-cased to encourage people not to directly
    // access them and treat them as private.
    std::vector<std::string>            m_toolbarItems;
    std::vector<TOOLBAR_GROUP_CONFIG>   m_toolbarGroups;
};


enum class TOOLBAR_LOC
{
    LEFT,           ///< Toolbar on the left side of the canvas
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
    std::optional<TOOLBAR_CONFIGURATION> GetToolbarConfig( TOOLBAR_LOC aToolbar, bool aForceDefault );

protected:
    // The toolbars - only public to aid in JSON serialization/deserialization
    std::map<TOOLBAR_LOC, TOOLBAR_CONFIGURATION> m_toolbars;
};

#endif /* TOOLBAR_CONFIGURATION_H_ */
