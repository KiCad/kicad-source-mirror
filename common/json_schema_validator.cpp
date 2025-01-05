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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
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
        // For some obscure reason on MINGW, using UCRT option,
        // m_schema_validator.set_root_schema() hangs without switching to locale "C"
#if defined(__MINGW32__) && defined(_UCRT)
        LOCALE_IO dummy;
#endif

        schema_stream >> schema;
        m_validator.set_root_schema( schema );
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


nlohmann::json JSON_SCHEMA_VALIDATOR::Validate( const nlohmann::json& aJson,
                             nlohmann::json_schema::error_handler& aErrorHandler,
                             const nlohmann::json_uri& aInitialUri ) const
{
    return m_validator.validate( aJson, aErrorHandler, aInitialUri );
}
