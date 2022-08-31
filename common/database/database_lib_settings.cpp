/*
* This program source code file is part of KiCad, a free EDA CAD application.
*
* Copyright (C) 2022 Jon Evans <jon@craftyjon.com>
* Copyright (C) 2022 KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <nlohmann/json.hpp>

#include <database/database_lib_settings.h>
#include <settings/parameters.h>
#include <wildcards_and_files_ext.h>


const int dblibSchemaVersion = 0;


DATABASE_LIB_SETTINGS::DATABASE_LIB_SETTINGS( const std::string& aFilename ) :
        JSON_SETTINGS( aFilename, SETTINGS_LOC::NONE, dblibSchemaVersion )
{

    m_params.emplace_back( new PARAM<std::string>( "source.dsn", &m_Source.dsn, "" ) );

    m_params.emplace_back( new PARAM<std::string>( "source.username", &m_Source.username, "" ) );

    m_params.emplace_back( new PARAM<std::string>( "source.password", &m_Source.password, "" ) );

    m_params.emplace_back(
            new PARAM<std::string>( "source.connection_string", &m_Source.connection_string, "" ) );

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

                    if( entry.contains( "fields" ) && entry["fields"].is_array() )
                    {
                        for( const nlohmann::json& fieldJson : entry["fields"] )
                        {
                            if( fieldJson.empty() || !fieldJson.is_object() )
                                continue;

                            table.fields.emplace_back(
                                    DATABASE_FIELD_MAPPING(
                                    {
                                        fieldJson["column"].get<std::string>(),
                                        fieldJson["name"].get<std::string>(),
                                        fieldJson["visible_on_add"].get<bool>(),
                                        fieldJson["visible_in_chooser"].get<bool>()
                                    } ) );
                        }
                    }

                    m_Tables.emplace_back( std::move( table ) );
                }
            },
            {} ) );

    m_params.emplace_back( new PARAM<int>( "cache.max_size", &m_Cache.max_size, 256 ) );

    m_params.emplace_back( new PARAM<int>( "cache.max_age", &m_Cache.max_age, 10 ) );
}


wxString DATABASE_LIB_SETTINGS::getFileExt() const
{
    return DatabaseLibraryFileExtension;
}
