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

#include <magic_enum.hpp>
#include <nlohmann/json.hpp>

#include <tool/action_toolbar.h>
#include <tool/ui/toolbar_configuration.h>

///! Update the schema version whenever a migration is required
const int toolbarSchemaVersion = 1;


void to_json( nlohmann::json& aJson, const TOOLBAR_CONFIGURATION& aConfig )
{
    nlohmann::json groups = nlohmann::json::array();

    // Serialize the group object
    for( const TOOLBAR_GROUP_CONFIG& grp : aConfig.m_toolbarGroups )
    {
        nlohmann::json jsGrp = {
            { "name", grp.m_groupName }
        };

        nlohmann::json grpItems = nlohmann::json::array();

        for( const auto& it : grp.m_groupItems )
            grpItems.push_back( it );

        jsGrp["items"] = grpItems;

        groups.push_back( jsGrp );
    }

    // Serialize the items
    nlohmann::json tbItems = nlohmann::json::array();

    for( const auto& it : aConfig.m_toolbarItems )
        tbItems.push_back( it );

    aJson = {
        { "groups", groups },
        { "items",  tbItems }
    };
}


void from_json( const nlohmann::json& aJson, TOOLBAR_CONFIGURATION& aConfig )
{
    if( aJson.empty() )
        return;

    aConfig.m_toolbarItems.clear();
    aConfig.m_toolbarGroups.clear();

    // Deserialize the groups
    if( aJson.contains( "groups" ) && aJson.at( "groups" ).is_array())
    {
        for( const nlohmann::json& grp : aJson.at( "groups" ) )
        {
            std::string name = "";

            if( grp.contains( "name" ) )
                name = grp.at( "name" ).get<std::string>();

            TOOLBAR_GROUP_CONFIG cfg( name );

            // Deserialize the items
            if( grp.contains( "items" ) )
            {
                for( const nlohmann::json& it : grp.at( "items" ) )
                {
                    if( it.is_string() )
                        cfg.m_groupItems.push_back( it.get<std::string>() );
                }
            }
            aConfig.m_toolbarGroups.push_back( cfg );
        }
    }

    // Deserialize the items
    if( aJson.contains( "items" ) )
    {
        for( const nlohmann::json& it : aJson.at( "items" ) )
        {
            if( it.is_string() )
            aConfig.m_toolbarItems.push_back( it.get<std::string>() );
        }
    }
}


TOOLBAR_SETTINGS::TOOLBAR_SETTINGS( const wxString& aFullPath ) :
        JSON_SETTINGS( aFullPath, SETTINGS_LOC::NONE, toolbarSchemaVersion )
{
    m_params.emplace_back( new PARAM_LAMBDA<nlohmann::json>( "toolbars",
        [&]() -> nlohmann::json
        {
            // Serialize the toolbars
            nlohmann::json js = nlohmann::json::array();

            for( const auto& [loc, tb] : m_toolbars )
            {
                js.push_back( nlohmann::json( { { "name", magic_enum::enum_name( loc ) },
                                                  { "contents", tb } } ) );
            }

            return js;
        },
        [&]( const nlohmann::json& aObj )
        {
            // Deserialize the toolbars
            m_toolbars.clear();

            if( !aObj.is_array() )
                return;

            for( const auto& entry : aObj )
            {
                if( entry.empty() || !entry.is_object() )
                    continue;

                auto loc = magic_enum::enum_cast<TOOLBAR_LOC>( entry["name"].get<std::string>(),
                                                               magic_enum::case_insensitive );

                if( loc.has_value() )
                {
                    m_toolbars.emplace(
                        std::make_pair( loc.value(),
                                        entry["contents"].get<TOOLBAR_CONFIGURATION>() ) );
                }
            }
        },
        nlohmann::json::array() ) );
}


std::optional<TOOLBAR_CONFIGURATION> TOOLBAR_SETTINGS::GetToolbarConfig( TOOLBAR_LOC aToolbar, bool aAllowCustom )
{
    // If custom is allowed, look for if a toolbar exists
    if( aAllowCustom )
    {
        auto tb = m_toolbars.find( aToolbar );

        if( tb != m_toolbars.end() )
            return tb->second;
    }

    return DefaultToolbarConfig( aToolbar );
}
