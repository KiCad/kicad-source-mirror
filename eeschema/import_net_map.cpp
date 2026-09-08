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

#include <import_net_map.h>
#include <json_common.h>
#include <reporter.h>
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/intl.h>
#include <stdexcept>
#include <limits>

namespace
{
using JSON = nlohmann::json;

JSON readDocument( const wxString& aPath )
{
    wxFile file( aPath );
    wxString text;

    if( !file.IsOpened() || !file.ReadAll( &text ) )
        throw std::runtime_error( "Cannot read companion file" );

    return JSON::parse( text.ToUTF8().data() );
}

template<typename T>
T identityNumber( const JSON& aJson )
{
    if( !aJson.is_number_integer()
        || ( !aJson.is_number_unsigned() && aJson.get<int64_t>() < 0 )
        || aJson.get<uint64_t>() > static_cast<uint64_t>( std::numeric_limits<T>::max() ) )
    {
        throw std::runtime_error( "Companion identity must be a nonnegative integer in range" );
    }

    return aJson.get<T>();
}

void checkHeader( const JSON& aJson, const KIID& aUuid )
{
    if( identityNumber<int>( aJson.at( "schemaVersion" ) ) != 1
        || aJson.at( "rootUuid" ).get<std::string>() != aUuid.AsString().ToStdString() )
    {
        throw std::runtime_error( "Companion version or root UUID does not match" );
    }
}

wxString stringValue( const JSON& aJson, const char* aKey )
{
    return wxString::FromUTF8( aJson.at( aKey ).get<std::string>() );
}

KIID uuidValue( const JSON& aJson )
{
    const wxString text = wxString::FromUTF8( aJson.get<std::string>() );

    if( !KIID::SniffTest( text ) )
        throw std::runtime_error( "Companion contains an invalid item UUID" );

    const KIID uuid( text );

    if( uuid.AsString().CmpNoCase( text ) != 0 )
        throw std::runtime_error( "Companion contains an invalid item UUID" );

    return uuid;
}
}

wxString ImportNetMapPath( const wxString& aRootPath )
{
    wxFileName path( aRootPath );
    path.SetFullName( path.GetName() + wxS( ".orcad-net-map.json" ) );
    return path.GetFullPath();
}

bool WriteImportNetMap( const IMPORT_NET_MAP& aMap, const KIID& aRootUuid,
                        const wxString& aRootPath, REPORTER& aReporter )
{
    const wxString path = ImportNetMapPath( aRootPath );

    try
    {
        if( wxFileExists( path ) )
        {
            std::optional<IMPORT_NET_MAP> existing = ReadImportNetMap( aRootUuid, aRootPath, aReporter );

            if( !existing )
                return false;

            if( existing->sourceDesignDigest != aMap.sourceDesignDigest )
                throw std::runtime_error( "Companion belongs to a different source design" );
        }

        JSON document = { { "schemaVersion", 1 }, { "rootUuid", aRootUuid.AsString().ToStdString() },
                          { "sourceDesignDigest", aMap.sourceDesignDigest.ToStdString( wxConvUTF8 ) },
                          { "entries", JSON::array() } };

        for( const IMPORT_NET_MAP_ENTRY& entry : aMap.entries )
        {
            JSON record = { { "view", entry.view.ToStdString( wxConvUTF8 ) },
                            { "occurrence", JSON::array() }, { "sourceNetId", entry.sourceNetId },
                            { "originalName", entry.originalName.ToStdString( wxConvUTF8 ) },
                            { "nameAtImport", entry.nameAtImport.ToStdString( wxConvUTF8 ) },
                            { "status", entry.status.ToStdString( wxConvUTF8 ) },
                            { "terminals", JSON::array() }, { "itemUuids", JSON::array() } };

            for( const wxString& component : entry.occurrence )
                record["occurrence"].push_back( component.ToStdString( wxConvUTF8 ) );

            for( const KIID& uuid : entry.itemUuids )
                record["itemUuids"].push_back( uuid.AsString().ToStdString() );

            for( const IMPORT_NET_TERMINAL& terminal : entry.terminals )
            {
                JSON pin = { { "symbolUuid", terminal.symbolUuid.AsString().ToStdString() },
                             { "unit", terminal.unit }, { "sourcePinId", terminal.sourcePinId },
                             { "pinNumber", terminal.pinNumber.ToStdString( wxConvUTF8 ) },
                             { "duplicateIndex", terminal.duplicateIndex } };

                if( terminal.pinUuid )
                    pin["pinUuid"] = terminal.pinUuid->AsString().ToStdString();

                record["terminals"].push_back( std::move( pin ) );
            }

            document["entries"].push_back( std::move( record ) );
        }

        const std::string data = document.dump( 2 ) + "\n";
        wxTempFile file( path );

        if( !file.IsOpened() || !file.Write( data.data(), data.size() ) || !file.Commit() )
            throw std::runtime_error( "Cannot atomically write companion file" );

        return true;
    }
    catch( const std::exception& error )
    {
        aReporter.Report( wxString::Format( _( "Could not save imported net-name map '%s': %s" ),
                                           path, wxString::FromUTF8( error.what() ) ), RPT_SEVERITY_WARNING );
        return false;
    }
}

std::optional<IMPORT_NET_MAP> ReadImportNetMap( const KIID& aRootUuid,
                                              const wxString& aRootPath, REPORTER& aReporter )
{
    const wxString path = ImportNetMapPath( aRootPath );

    if( !wxFileExists( path ) )
        return std::nullopt;

    try
    {
        JSON document = readDocument( path );
        checkHeader( document, aRootUuid );
        IMPORT_NET_MAP map;
        map.sourceDesignDigest = stringValue( document, "sourceDesignDigest" );

        if( !document.at( "entries" ).is_array() )
            throw std::runtime_error( "Companion entries must be an array" );

        for( const JSON& record : document.at( "entries" ) )
        {
            IMPORT_NET_MAP_ENTRY entry;
            entry.view = stringValue( record, "view" );
            entry.sourceNetId = identityNumber<uint32_t>( record.at( "sourceNetId" ) );
            entry.originalName = stringValue( record, "originalName" );
            entry.nameAtImport = stringValue( record, "nameAtImport" );
            entry.status = stringValue( record, "status" );

            if( !record.at( "occurrence" ).is_array() || !record.at( "itemUuids" ).is_array()
                || !record.at( "terminals" ).is_array() )
            {
                throw std::runtime_error( "Companion occurrence, items and terminals must be arrays" );
            }

            for( const JSON& component : record.at( "occurrence" ) )
                entry.occurrence.push_back( wxString::FromUTF8( component.get<std::string>() ) );

            for( const JSON& uuid : record.at( "itemUuids" ) )
                entry.itemUuids.push_back( uuidValue( uuid ) );

            for( const JSON& pin : record.at( "terminals" ) )
            {
                IMPORT_NET_TERMINAL terminal;
                terminal.symbolUuid = uuidValue( pin.at( "symbolUuid" ) );
                terminal.unit = identityNumber<int>( pin.at( "unit" ) );
                terminal.sourcePinId = identityNumber<uint32_t>( pin.at( "sourcePinId" ) );
                terminal.pinNumber = stringValue( pin, "pinNumber" );
                terminal.duplicateIndex = identityNumber<unsigned>( pin.at( "duplicateIndex" ) );

                if( pin.contains( "pinUuid" ) )
                    terminal.pinUuid = uuidValue( pin.at( "pinUuid" ) );

                entry.terminals.push_back( std::move( terminal ) );
            }

            map.entries.push_back( std::move( entry ) );
        }

        return map;
    }
    catch( const std::exception& error )
    {
        aReporter.Report( wxString::Format( _( "Could not load imported net-name map '%s': %s" ),
                                           path, wxString::FromUTF8( error.what() ) ), RPT_SEVERITY_WARNING );
        return std::nullopt;
    }
}
