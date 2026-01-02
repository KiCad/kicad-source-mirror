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
#include <json_common.h>

#include <tool/action_toolbar.h>
#include <tool/ui/toolbar_configuration.h>
#include <tool/ui/toolbar_context_menu_registry.h>

///! Update the schema version whenever a migration is required
const int toolbarSchemaVersion = 1;

void to_json( nlohmann::json& aJson, const TOOLBAR_ITEM& aItem )
{
    aJson = { { "type", magic_enum::enum_name( aItem.m_Type ) } };

    switch( aItem.m_Type )
    {
    case TOOLBAR_ITEM_TYPE::SEPARATOR:
        // Nothing to add for a separator
        break;

    case TOOLBAR_ITEM_TYPE::SPACER:
        aJson["size"] = aItem.m_Size;
        break;

    case TOOLBAR_ITEM_TYPE::CONTROL:
        aJson["name"] = aItem.m_ControlName;
        break;

    case TOOLBAR_ITEM_TYPE::TOOL:
        aJson["name"] = aItem.m_ActionName;
        break;

    case TOOLBAR_ITEM_TYPE::TB_GROUP:
        aJson["group_name"] = aItem.m_GroupName;

        nlohmann::json grpItems = nlohmann::json::array();

        for( const auto& it : aItem.m_GroupItems )
            grpItems.push_back( it );

        aJson["group_items"] = grpItems;

        break;
    }
}


void from_json( const nlohmann::json& aJson, TOOLBAR_ITEM& aItem )
{
    if( aJson.empty() )
        return;

    if( aJson.contains( "type" ) )
    {
        auto type = magic_enum::enum_cast<TOOLBAR_ITEM_TYPE>( aJson["type"].get<std::string>(),
                                                            magic_enum::case_insensitive );

        if( type.has_value() )
            aItem.m_Type = type.value();
    }

    switch( aItem.m_Type )
    {
    case TOOLBAR_ITEM_TYPE::SEPARATOR:
        // Nothing to read for a separator
        break;

    case TOOLBAR_ITEM_TYPE::SPACER:
        if( aJson.contains( "size" ) )
            aItem.m_Size = aJson["size"].get<int>();

        break;

    case TOOLBAR_ITEM_TYPE::CONTROL:
        if( aJson.contains( "name" ) )
            aItem.m_ControlName = aJson["name"].get<std::string>();

        break;

    case TOOLBAR_ITEM_TYPE::TOOL:
        if( aJson.contains( "name" ) )
            aItem.m_ActionName = aJson["name"].get<std::string>();

        break;

    case TOOLBAR_ITEM_TYPE::TB_GROUP:
        if( aJson.contains( "group_name" ) )
            aItem.m_GroupName = aJson["group_name"].get<wxString>();

        if( aJson.contains( "group_items" ) )
        {
            for( const nlohmann::json& it : aJson.at( "group_items" ) )
                aItem.m_GroupItems.push_back( it.get<TOOLBAR_ITEM>() );
        }
        break;
    }
}


void to_json( nlohmann::json& aJson, const TOOLBAR_CONFIGURATION& aConfig )
{
    aJson = nlohmann::json::array();

    for( const TOOLBAR_ITEM& item : aConfig.m_toolbarItems )
        aJson.push_back( item );
}


void from_json( const nlohmann::json& aJson, TOOLBAR_CONFIGURATION& aConfig )
{
    if( aJson.empty() )
        return;

    aConfig.m_toolbarItems.clear();

    if( aJson.is_array() )
    {
        for( const nlohmann::json& item : aJson )
            aConfig.m_toolbarItems.push_back( item.get<TOOLBAR_ITEM>() );
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


std::optional<TOOLBAR_CONFIGURATION> TOOLBAR_SETTINGS::GetStoredToolbarConfig( TOOLBAR_LOC aToolbar )
{
    auto tb = m_toolbars.find( aToolbar );

    if( tb != m_toolbars.end() )
        return tb->second;

    // Return a nullopt if no toolbar is configured
    return std::nullopt;
}


TOOLBAR_CONFIGURATION& TOOLBAR_ITEM_REF::WithContextMenu(
        TOOLBAR_CONTEXT_MENU_REGISTRY::MENU_FACTORY aFactory )
{
    // Register the factory globally so JSON configs get the same menu
    TOOLBAR_CONTEXT_MENU_REGISTRY::RegisterMenuFactory( m_item.m_ActionName, std::move( aFactory ) );
    return m_parent;
}


// Forwarding methods for TOOLBAR_ITEM_REF to enable chaining

TOOLBAR_ITEM_REF TOOLBAR_ITEM_REF::AppendAction( const std::string& aActionName )
{
    return m_parent.AppendAction( aActionName );
}


TOOLBAR_ITEM_REF TOOLBAR_ITEM_REF::AppendAction( const TOOL_ACTION& aAction )
{
    return m_parent.AppendAction( aAction );
}


TOOLBAR_CONFIGURATION& TOOLBAR_ITEM_REF::AppendSeparator()
{
    return m_parent.AppendSeparator();
}


TOOLBAR_CONFIGURATION& TOOLBAR_ITEM_REF::AppendSpacer( int aSize )
{
    return m_parent.AppendSpacer( aSize );
}


TOOLBAR_CONFIGURATION& TOOLBAR_ITEM_REF::AppendGroup( const TOOLBAR_GROUP_CONFIG& aGroup )
{
    return m_parent.AppendGroup( aGroup );
}


TOOLBAR_CONFIGURATION& TOOLBAR_ITEM_REF::AppendControl( const std::string& aControlName )
{
    return m_parent.AppendControl( aControlName );
}


TOOLBAR_CONFIGURATION& TOOLBAR_ITEM_REF::AppendControl( const ACTION_TOOLBAR_CONTROL& aControl )
{
    return m_parent.AppendControl( aControl );
}
