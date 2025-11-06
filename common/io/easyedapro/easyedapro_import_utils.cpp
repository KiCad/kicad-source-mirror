/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Alex Shvartzkop <dudesuchamazing@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include "easyedapro_import_utils.h"
#include "easyedapro_parser.h"

#include <io/common/plugin_common_choose_project.h>

#include <ki_exception.h>
#include <string_utils.h>
#include <json_common.h>
#include <core/json_serializers.h>

#include <wx/log.h>
#include <wx/stream.h>
#include <wx/zipstrm.h>
#include <wx/wfstream.h>
#include <wx/mstream.h>
#include <wx/txtstrm.h>


wxString EASYEDAPRO::ShortenLibName( wxString aProjectName )
{
    wxString shortenedName = aProjectName;
    shortenedName.Replace( wxS( "ProProject_" ), wxS( "" ) );
    shortenedName.Replace( wxS( "ProDocument_" ), wxS( "" ) );
    shortenedName = shortenedName.substr( 0, 10 );

    return LIB_ID::FixIllegalChars( shortenedName + wxS( "-easyedapro" ), true );
}


LIB_ID EASYEDAPRO::ToKiCadLibID( const wxString& aLibName, const wxString& aLibReference )
{
    wxString libName = LIB_ID::FixIllegalChars( aLibName, true );
    wxString libReference = EscapeString( aLibReference, CTX_LIBID );

    wxString key = !aLibName.empty() ? ( aLibName + ':' + libReference ) : libReference;

    LIB_ID libId;
    libId.Parse( key, true );

    return libId;
}


std::vector<IMPORT_PROJECT_DESC>
EASYEDAPRO::ProjectToSelectorDialog( const nlohmann::json& aProject, bool aPcbOnly, bool aSchOnly )
{
    std::vector<IMPORT_PROJECT_DESC> result;

    std::map<wxString, EASYEDAPRO::PRJ_SCHEMATIC> prjSchematics = aProject.at( "schematics" );
    std::map<wxString, EASYEDAPRO::PRJ_BOARD>     prjBoards = aProject.at( "boards" );

    std::map<wxString, wxString>       prjPcbNames;
    std::map<wxString, nlohmann::json> prjPcbs = aProject.at( "pcbs" );

    for( const auto& [pcbUuid, pcbJsonEntry] : prjPcbs )
    {
        if( pcbJsonEntry.is_string() )
            prjPcbNames.emplace( pcbUuid, pcbJsonEntry );
        else if( pcbJsonEntry.is_object() )
            prjPcbNames.emplace( pcbUuid, pcbJsonEntry.at( "title" ) );
    }

    for( const auto& [prjName, board] : prjBoards )
    {
        IMPORT_PROJECT_DESC desc;
        desc.ComboName = desc.ComboId = prjName;
        desc.PCBId = board.pcb;
        desc.SchematicId = board.schematic;

        auto pcbNameIt = prjPcbNames.find( desc.PCBId );
        if( pcbNameIt != prjPcbNames.end() )
        {
            desc.PCBName = pcbNameIt->second;

            if( desc.PCBName.empty() )
                desc.PCBName = pcbNameIt->first;

            prjPcbNames.erase( pcbNameIt );
        }

        auto schIt = prjSchematics.find( desc.SchematicId );
        if( schIt != prjSchematics.end() )
        {
            desc.SchematicName = schIt->second.name;

            if( desc.SchematicName.empty() )
                desc.SchematicName = schIt->first;

            prjSchematics.erase( schIt );
        }

        result.emplace_back( desc );
    }

    if( !aSchOnly )
    {
        for( const auto& [pcbId, pcbName] : prjPcbNames )
        {
            IMPORT_PROJECT_DESC desc;
            desc.PCBId = pcbId;
            desc.PCBName = pcbName;

            if( desc.PCBName.empty() )
                desc.PCBName = pcbId;

            result.emplace_back( desc );
        }
    }

    if( !aPcbOnly )
    {
        for( const auto& [schId, schData] : prjSchematics )
        {
            IMPORT_PROJECT_DESC desc;
            desc.SchematicId = schId;
            desc.SchematicName = schData.name;

            if( desc.SchematicName.empty() )
                desc.SchematicName = schId;

            result.emplace_back( desc );
        }
    }

    return result;
}


nlohmann::json EASYEDAPRO::FindJsonFile( const wxString&           aZipFileName,
                                         const std::set<wxString>& aFileNames )
{
    std::shared_ptr<wxZipEntry> entry;
    wxFFileInputStream          in( aZipFileName );
    wxZipInputStream            zip( in );

    while( entry.reset( zip.GetNextEntry() ), entry.get() != NULL )
    {
        wxString name = entry->GetName();

        try
        {
            if( aFileNames.find( name ) != aFileNames.end() )
            {
                wxMemoryOutputStream memos;
                memos << zip;
                wxStreamBuffer* buf = memos.GetOutputStreamBuffer();

                wxString str =
                        wxString::FromUTF8( (char*) buf->GetBufferStart(), buf->GetBufferSize() );

                return nlohmann::json::parse( str );
            }
        }
        catch( nlohmann::json::exception& e )
        {
            THROW_IO_ERROR(
                    wxString::Format( _( "JSON error reading '%s': %s" ), name, e.what() ) );
        }
        catch( std::exception& e )
        {
            THROW_IO_ERROR( wxString::Format( _( "Error reading '%s': %s" ), name, e.what() ) );
        }
    }

    return nlohmann::json{};
}


nlohmann::json EASYEDAPRO::ReadProjectOrDeviceFile( const wxString& aZipFileName )
{
    static const std::set<wxString> c_files = { wxS( "project.json" ), wxS( "device.json" ),
                                                wxS( "footprint.json" ), wxS( "symbol.json" ) };

    nlohmann::json j = FindJsonFile( aZipFileName, c_files );

    if( !j.is_null() )
        return j;

    THROW_IO_ERROR( wxString::Format(
            _( "'%s' does not appear to be a valid EasyEDA (JLCEDA) Pro "
               "project or library file. Cannot find project.json or device.json." ),
            aZipFileName ) );
}


void EASYEDAPRO::IterateZipFiles(
        const wxString&                                                         aFileName,
        std::function<bool( const wxString&, const wxString&, wxInputStream& )> aCallback )
{
    std::shared_ptr<wxZipEntry> entry;
    wxFFileInputStream          in( aFileName );
    wxZipInputStream            zip( in );

    if( !zip.IsOk() )
    {
        THROW_IO_ERROR( wxString::Format( _( "Cannot read ZIP archive '%s'" ), aFileName ) );
    }

    while( entry.reset( zip.GetNextEntry() ), entry.get() != NULL )
    {
        wxString name = entry->GetName();
        wxString baseName = name.AfterLast( '\\' ).AfterLast( '/' ).BeforeFirst( '.' );

        try
        {
            if( aCallback( name, baseName, zip ) )
                break;
        }
        catch( nlohmann::json::exception& e )
        {
            THROW_IO_ERROR(
                    wxString::Format( _( "JSON error reading '%s': %s" ), name, e.what() ) );
        }
        catch( std::exception& e )
        {
            THROW_IO_ERROR( wxString::Format( _( "Error reading '%s': %s" ), name, e.what() ) );
        }
    }
}


std::vector<nlohmann::json> EASYEDAPRO::ParseJsonLines( wxInputStream&  aInput,
                                                        const wxString& aSource )
{
    wxTextInputStream txt( aInput, wxS( " " ), wxConvUTF8 );

    int currentLine = 1;

    std::vector<nlohmann::json> lines;
    while( aInput.CanRead() )
    {
        try
        {
            wxString line = txt.ReadLine();

            if( !line.IsEmpty() )
            {
                nlohmann::json js = nlohmann::json::parse( line );
                lines.emplace_back( js );
            }
            else
            {
                lines.emplace_back( nlohmann::json() );
            }
        }
        catch( nlohmann::json::exception& e )
        {
            wxLogWarning( wxString::Format( _( "Cannot parse JSON line %d in '%s': %s" ),
                                            currentLine, aSource, e.what() ) );
        }

        currentLine++;
    }

    return lines;
}


std::vector<std::vector<nlohmann::json>>
EASYEDAPRO::ParseJsonLinesWithSeparation( wxInputStream& aInput, const wxString& aSource )
{
    wxTextInputStream txt( aInput, wxS( " " ), wxConvUTF8 );

    int currentLine = 1;

    std::vector<std::vector<nlohmann::json>> lineBlocks;
    lineBlocks.emplace_back();

    while( aInput.CanRead() )
    {
        try
        {
            wxString line = txt.ReadLine();

            if( !line.IsEmpty() )
            {
                nlohmann::json js = nlohmann::json::parse( line );
                lineBlocks.back().emplace_back( js );
            }
            else
            {
                lineBlocks.emplace_back();
            }
        }
        catch( nlohmann::json::exception& e )
        {
            wxLogWarning( wxString::Format( _( "Cannot parse JSON line %d in '%s': %s" ),
                                            currentLine, aSource, e.what() ) );
        }

        currentLine++;
    }

    return lineBlocks;
}


std::map<wxString, wxString>
EASYEDAPRO::AnyMapToStringMap( const std::map<wxString, nlohmann::json>& aInput )
{
    std::map<wxString, wxString> stringMap;

    for( auto& [key, value] : aInput )
    {
        if( value.is_string() )
            stringMap[key] = value.get<wxString>();
        else if( value.is_number() )
            stringMap[key] = wxString::FromCDouble( value.get<double>() );
    }

    return stringMap;
}
