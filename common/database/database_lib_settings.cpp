/*
* This program source code file is part of KiCad, a free EDA CAD application.
*
* Copyright (C) 2022 Jon Evans <jon@craftyjon.com>
* Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <core/kicad_algo.h>
#include <json_common.h>
#include <algorithm>

#include <database/database_lib_settings.h>
#include <settings/parameters.h>
#include <wildcards_and_files_ext.h>


const int dblibSchemaVersion = 1;


DATABASE_FIELD_MAPPING::DATABASE_FIELD_MAPPING( const std::string& aColumn, const std::string& aName,
                                                bool aVisibleOnAdd, bool aVisibleInChooser,
                                                bool aShowName, bool aInheritProperties ) :
        column( aColumn ),
        name( aName ),
        name_wx( aName.c_str(), wxConvUTF8 ),
        visible_on_add( aVisibleOnAdd ),
        visible_in_chooser( aVisibleInChooser ),
        show_name( aShowName ),
        inherit_properties( aInheritProperties )
{
}


DATABASE_LIB_SETTINGS::DATABASE_LIB_SETTINGS( const std::string& aFilename ) :
        JSON_SETTINGS( aFilename, SETTINGS_LOC::NONE, dblibSchemaVersion )
{

    m_params.emplace_back( new PARAM<std::string>( "source.dsn", &m_Source.dsn, "" ) );

    m_params.emplace_back( new PARAM<std::string>( "source.username", &m_Source.username, "" ) );

    m_params.emplace_back( new PARAM<std::string>( "source.password", &m_Source.password, "" ) );

    m_params.emplace_back( new PARAM<std::string>( "source.connection_string",
                                                   &m_Source.connection_string, "" ) );

    m_params.emplace_back( new PARAM<int>( "source.timeout_seconds", &m_Source.timeout, 2 ) );

    m_params.emplace_back( new PARAM_LAMBDA<nlohmann::json>(
            "libraries",
            [&]() -> nlohmann::json
            {
                // TODO: implement this; libraries are read-only from KiCad at the moment
                return {};
            },
            [&]( const nlohmann::json aObj )
            {
                m_Tables.clear();

                if( !aObj.is_array() )
                    return;

                for( const nlohmann::json& entry : aObj )
                {
                    if( entry.empty() || !entry.is_object() )
                        continue;

                    DATABASE_LIB_TABLE table;

                    table.name           = entry["name"].get<std::string>();
                    table.table          = entry["table"].get<std::string>();
                    table.key_col        = entry["key"].get<std::string>();
                    table.symbols_col    = entry["symbols"].get<std::string>();
                    table.footprints_col = entry["footprints"].get<std::string>();

                    // Sanitize library display names; currently only `/` is removed because we
                    // use it as a separator and allow it in symbol names.
                    std::erase( table.name, '/' );

                    if( entry.contains( "properties" ) && entry["properties"].is_object() )
                    {
                        const nlohmann::json& propJson = entry["properties"];

                        table.properties.description =
                                    fetchOrDefault<std::string>( propJson, "description" );

                        table.properties.footprint_filters =
                                    fetchOrDefault<std::string>( propJson, "footprint_filters" );

                        table.properties.keywords =
                                    fetchOrDefault<std::string>( propJson, "keywords" );

                        table.properties.exclude_from_bom =
                                    fetchOrDefault<std::string>( propJson, "exclude_from_bom" );

                        table.properties.exclude_from_board =
                                    fetchOrDefault<std::string>( propJson, "exclude_from_board" );

                        table.properties.exclude_from_sim =
                                    fetchOrDefault<std::string>( propJson, "exclude_from_sim" );
                    }

                    if( entry.contains( "fields" ) && entry["fields"].is_array() )
                    {
                        for( const nlohmann::json& fieldJson : entry["fields"] )
                        {
                            if( fieldJson.empty() || !fieldJson.is_object() )
                                continue;

                            table.fields.emplace_back( DATABASE_FIELD_MAPPING(
                                    fetchOrDefault<std::string>( fieldJson, "column" ),
                                    fetchOrDefault<std::string>( fieldJson, "name" ),
                                    fetchOrDefault<bool>( fieldJson, "visible_on_add" ),
                                    fetchOrDefault<bool>( fieldJson, "visible_in_chooser" ),
                                    fetchOrDefault<bool>( fieldJson, "show_name" ),
                                    fetchOrDefault<bool>( fieldJson, "inherit_properties" ) ) );
                        }
                    }

                    m_Tables.emplace_back( std::move( table ) );
                }
            },
            {} ) );

    m_params.emplace_back( new PARAM<int>( "cache.max_size", &m_Cache.max_size, 256 ) );

    m_params.emplace_back( new PARAM<int>( "cache.max_age", &m_Cache.max_age, 10 ) );

    m_params.emplace_back( new PARAM<bool>( "globally_unique_keys", &m_GloballyUniqueKeys, false ) );

    registerMigration( 0, 1,
                       [&]() -> bool
                       {
                           /*
                            * Schema 0 -> 1
                            * Move internal symbol properties from fields with special names to
                            * a separate place in the schema.
                            */
                            if( !Contains( "libraries" ) || !At( "libraries" ).is_array() )
                                return true;

                            for( nlohmann::json& library : At( "libraries" ) )
                            {
                                if( !library.contains( "fields" ) )
                                    continue;

                                for( const nlohmann::json& field : library["fields"] )
                                {
                                    if( !field.contains( "name" ) || !field.contains( "column" ) )
                                        continue;

                                    std::string name = field["name"].get<std::string>();
                                    std::string col  = field["column"].get<std::string>();

                                    if( name == "ki_description" )
                                        library["properties"]["description"] = col;
                                    else if( name == "ki_fp_filters" )
                                        library["properties"]["footprint_filters"] = col;
                                }
                            }

                           return true;
                       } );
}


wxString DATABASE_LIB_SETTINGS::getFileExt() const
{
    return FILEEXT::DatabaseLibraryFileExtension;
}
