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

#include <database/database_backend.h>

#include <database/database_lib_settings.h>
#include <wx/intl.h>

#include "odbc_backend.h"
#include "db_backend_support.h"
#include "sqlite_backend.h"

#ifdef _WIN32
#include "ace_backend.h"
#endif


DB_BACKEND_RESULT DATABASE_BACKEND::Create( const DATABASE_SOURCE& aSource, const wxString& aBasePath )
{
    const DATABASE_SOURCE source = aSource.Expanded( aBasePath );

    switch( source.type )
    {
    case DATABASE_SOURCE_TYPE::ODBC:
        return { std::make_unique<ODBC_BACKEND>( source.dsn, source.username, source.password,
                                                 source.connection_string, source.timeout ),
                 DB_FAILURE::UNKNOWN, std::string() };

    case DATABASE_SOURCE_TYPE::SQLITE:
        return { std::make_unique<SQLITE_BACKEND>( source.path, source.timeout ), DB_FAILURE::UNKNOWN,
                 std::string() };

    case DATABASE_SOURCE_TYPE::ACCESS:
#ifdef _WIN32
        // A probe still running is waited on once, by Connect
        if( ACE_BACKEND::ProviderState() == ACE_BACKEND::PROVIDER_STATE::MISSING )
            return { nullptr, DB_FAILURE::PROVIDER_MISSING, ACE_BACKEND::ProviderMissingMessage() };

        return { std::make_unique<ACE_BACKEND>( source.path, source.password, source.timeout ), DB_FAILURE::UNKNOWN,
                 std::string() };
#else
        return { nullptr, DB_FAILURE::NOT_SUPPORTED_ON_PLATFORM,
                 utf8( wxString::Format( _( "source type '%s' is only available on Windows" ), wxS( "access" ) ) ) };
#endif

    case DATABASE_SOURCE_TYPE::INVALID:
        break;
    }

    return { nullptr, DB_FAILURE::INVALID_TYPE,
             utf8( wxString::Format( _( "unknown source type '%s'" ), wxString::FromUTF8( source.type_name ) ) ) };
}


bool DATABASE_BACKEND::ListColumns( const std::string& aTable, std::vector<std::string>& aColumns )
{
    aColumns.clear();

    std::optional<DB_TABLE_INFO> info = DescribeTable( aTable );

    // DescribeTable clears the error first, so an absent table leaves none and lists no columns
    if( !info )
        return GetLastError().empty();

    for( const DB_COLUMN& column : info->columns )
        aColumns.push_back( column.name );

    return true;
}
