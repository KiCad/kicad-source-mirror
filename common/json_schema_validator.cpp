/*
* This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <fstream>
#include <wx/filename.h>
#include <wx/log.h>

#include <json_schema_validator.h>
#include <locale_io.h>


JSON_SCHEMA_VALIDATOR::JSON_SCHEMA_VALIDATOR( const wxFileName& aSchemaFile )
{
    std::ifstream schema_stream( aSchemaFile.GetFullPath().fn_str() );
    nlohmann::json schema;

    try
    {
        schema_stream >> schema;
        setRootSchema( schema );
    }
    catch( std::exception& e )
    {
        if( !aSchemaFile.FileExists() )
        {
            wxLogError( wxString::Format( _( "schema file '%s' not found" ),
                                          aSchemaFile.GetFullPath() ) );
        }
        else
        {
            wxLogError( wxString::Format( _( "Error loading schema: %s" ), e.what() ) );
        }
    }
}


JSON_SCHEMA_VALIDATOR::JSON_SCHEMA_VALIDATOR( const nlohmann::json& aSchema )
{
    try
    {
        setRootSchema( aSchema );
    }
    catch( std::exception& e )
    {
        wxLogError( wxString::Format( _( "Error loading schema: %s" ), e.what() ) );
    }
}


void JSON_SCHEMA_VALIDATOR::setRootSchema( const nlohmann::json& aSchema )
{
    // For some obscure reason on MINGW, using UCRT option,
    // m_schema_validator.set_root_schema() hangs without switching to locale "C"
#if defined(__MINGW32__) && defined(_UCRT)
    LOCALE_IO dummy;
#endif

    m_validator.set_root_schema( aSchema );
}


nlohmann::json JSON_SCHEMA_VALIDATOR::Validate( const nlohmann::json& aJson,
                             nlohmann::json_schema::error_handler& aErrorHandler,
                             const nlohmann::json_uri& aInitialUri ) const
{
    return m_validator.validate( aJson, aErrorHandler, aInitialUri );
}
