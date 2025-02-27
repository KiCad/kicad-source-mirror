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
    aJson = nlohmann::json::array();

    for( const TOOLBAR_ITEM& item : aConfig.m_toolbarItems )
    {
        nlohmann::json jsItem = {
            { "type", magic_enum::enum_name( item.m_Type ) }
        };

        switch( item.m_Type )
        {
        case TOOLBAR_ITEM_TYPE::SEPARATOR:
            // Nothing to add for a separator
            break;

        case TOOLBAR_ITEM_TYPE::SPACER:
            jsItem["size"] = item.m_Size;
            break;

        case TOOLBAR_ITEM_TYPE::CONTROL:
            jsItem["name"] = item.m_ControlName;
            break;

        case TOOLBAR_ITEM_TYPE::TOOL:
            jsItem["name"] = item.m_ActionName;
            break;

        case TOOLBAR_ITEM_TYPE::GROUP:
            jsItem["group_name"] = item.m_GroupName;

            nlohmann::json grpItems = nlohmann::json::array();

            for( const auto& it : item.m_GroupItems )
                grpItems.push_back( it );

            jsItem["group_items"] = grpItems;

            break;
        }

        aJson.push_back( jsItem );
    }
}


void from_json( const nlohmann::json& aJson, TOOLBAR_CONFIGURATION& aConfig )
{
    if( aJson.empty() )
        return;

    aConfig.m_toolbarItems.clear();

    if( aJson.is_array() )
    {
        for( const nlohmann::json& item : aJson )
        {
            TOOLBAR_ITEM tbItem;

            if( item.contains( "type" ) )
            {
                auto type = magic_enum::enum_cast<TOOLBAR_ITEM_TYPE>( item["type"].get<std::string>(),
                                                                    magic_enum::case_insensitive );

                if( type.has_value() )
                    tbItem.m_Type = type.value();
            }

            switch( tbItem.m_Type )
            {
            case TOOLBAR_ITEM_TYPE::SEPARATOR:
                // Nothing to read for a separator
                break;

            case TOOLBAR_ITEM_TYPE::SPACER:
                if( item.contains( "size" ) )
                    tbItem.m_Size = item["size"].get<int>();

                break;

            case TOOLBAR_ITEM_TYPE::CONTROL:
                if( item.contains( "name" ) )
                    tbItem.m_ControlName = item["name"].get<std::string>();

                break;

            case TOOLBAR_ITEM_TYPE::TOOL:
                if( item.contains( "name" ) )
                    tbItem.m_ActionName = item["name"].get<std::string>();

                break;

            case TOOLBAR_ITEM_TYPE::GROUP:
                if( item.contains( "group_name" ) )
                    tbItem.m_GroupName = item["group_name"].get<wxString>();

                if( item.contains( "group_items" ) )
                {
                    for( const nlohmann::json& it : item["group_items"].at( "group_items" ) )
                    {
                        if( it.is_string() )
                            tbItem.m_GroupItems.push_back( it.get<std::string>() );
                    }
                }
                break;
            }

            // We just directly add the item to the config
            aConfig.m_toolbarItems.push_back( tbItem );
        }
    }
}


TOOLBAR_SETTINGS::TOOLBAR_SETTINGS( const wxString& aFullPath ) :
        JSON_SETTINGS( aFullPath, SETTINGS_LOC::TOOLBARS, toolbarSchemaVersion )
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
