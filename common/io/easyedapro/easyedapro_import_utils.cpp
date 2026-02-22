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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "easyedapro_import_utils.h"
#include "easyedapro_parser.h"
#include "easyedapro_v3_parser.h"

#include <io/common/plugin_common_choose_project.h>

#include <algorithm>
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


namespace
{

static std::string ToStdString( const wxString& aStr )
{
    return std::string( aStr.ToUTF8() );
}


static nlohmann::json EmptyV3ProjectIndex()
{
    nlohmann::json project = nlohmann::json::object();
    project["schematics"] = nlohmann::json::object();
    project["boards"] = nlohmann::json::object();
    project["pcbs"] = nlohmann::json::object();
    project["symbols"] = nlohmann::json::object();
    project["footprints"] = nlohmann::json::object();
    project["devices"] = nlohmann::json::object();

    return project;
}


static const nlohmann::json* FindMetaRow( const EASYEDAPRO::V3_DOC_RAW& aDoc )
{
    for( const EASYEDAPRO::V3_ROW& row : aDoc.rows )
    {
        if( row.type == wxS( "META" ) )
            return &row.inner;
    }

    return nullptr;
}


static wxString MetaGetString( const EASYEDAPRO::V3_DOC_RAW& aDoc, const char* aKey,
                               const wxString& aDefault = wxEmptyString )
{
    if( const nlohmann::json* meta = FindMetaRow( aDoc ) )
        return EASYEDAPRO::V3GetString( *meta, aKey, aDefault );

    return aDefault;
}


static int MetaGetInt( const EASYEDAPRO::V3_DOC_RAW& aDoc, const char* aKey, int aDefault = 0 )
{
    if( const nlohmann::json* meta = FindMetaRow( aDoc ) )
        return EASYEDAPRO::V3GetInt( *meta, aKey, aDefault );

    return aDefault;
}


static nlohmann::json MetaGetValue( const EASYEDAPRO::V3_DOC_RAW& aDoc, const char* aKey,
                                    const nlohmann::json& aDefault )
{
    if( const nlohmann::json* meta = FindMetaRow( aDoc ) )
    {
        auto it = meta->find( aKey );

        if( it != meta->end() )
            return *it;
    }

    return aDefault;
}

} // namespace


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

    wxString key = !libName.empty() ? ( libName + ':' + libReference ) : libReference;

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


nlohmann::json EASYEDAPRO::BuildV3ProjectIndexFromRawDocs( const V3_DOC_PARSER& aParser,
                                                           bool                 aIncludeLibraryMetadata )
{
    nlohmann::json project = EmptyV3ProjectIndex();

    struct SHEET_INFO
    {
        wxString uuid;
        wxString name;
        int      zIndex = 0;
        int      order = 0;
    };

    std::map<wxString, std::vector<SHEET_INFO>> sheetsBySch;
    int                                          pageOrder = 0;

    for( const auto& [uuid, pageDoc] : aParser.GetRawDocs( wxS( "SCH_PAGE" ) ) )
    {
        wxString schematic = MetaGetString( pageDoc, "schematic" );

        if( schematic.empty() )
            schematic = V3GetString( pageDoc.head, "schematic" );

        if( schematic.empty() )
            continue;

        SHEET_INFO info;
        info.uuid = uuid;
        info.name = MetaGetString( pageDoc, "title", uuid );
        info.zIndex = MetaGetInt( pageDoc, "zIndex", 0 );
        info.order = pageOrder++;

        sheetsBySch[schematic].push_back( std::move( info ) );
    }

    std::map<wxString, wxString> schematicsByBoard;

    for( const auto& [uuid, schDoc] : aParser.GetRawDocs( wxS( "SCH" ) ) )
    {
        nlohmann::json sch = nlohmann::json::object();
        sch["name"] = ToStdString( MetaGetString( schDoc, "title", uuid ) );
        sch["sheets"] = nlohmann::json::array();

        auto pagesIt = sheetsBySch.find( uuid );

        if( pagesIt != sheetsBySch.end() )
        {
            auto& pages = pagesIt->second;

            std::sort( pages.begin(), pages.end(),
                       []( const SHEET_INFO& aLeft, const SHEET_INFO& aRight )
                       {
                           if( aLeft.zIndex != aRight.zIndex )
                               return aLeft.zIndex < aRight.zIndex;

                           return aLeft.order < aRight.order;
                       } );

            int sheetId = 1;

            for( const SHEET_INFO& page : pages )
            {
                sch["sheets"].push_back( nlohmann::json::object( {
                        { "id", sheetId++ },
                        { "name", ToStdString( page.name ) },
                        { "uuid", ToStdString( page.uuid ) }
                } ) );
            }
        }

        project["schematics"][ToStdString( uuid )] = std::move( sch );

        wxString board = MetaGetString( schDoc, "board" );

        if( !board.empty() )
            schematicsByBoard[board] = uuid;
    }

    std::map<wxString, wxString> boardTitles;

    for( const auto& [uuid, boardDoc] : aParser.GetRawDocs( wxS( "BOARD" ) ) )
        boardTitles[uuid] = MetaGetString( boardDoc, "title", uuid );

    std::map<wxString, wxString> pcbsByBoard;

    for( const auto& [uuid, pcbDoc] : aParser.GetRawDocs( wxS( "PCB" ) ) )
    {
        nlohmann::json pcb = nlohmann::json::object();
        pcb["title"] = ToStdString( MetaGetString( pcbDoc, "title", uuid ) );

        project["pcbs"][ToStdString( uuid )] = std::move( pcb );

        wxString board = MetaGetString( pcbDoc, "board" );

        if( !board.empty() )
            pcbsByBoard[board] = uuid;
    }

    std::set<wxString> allBoardRefs;

    for( const auto& [boardRef, schUuid] : schematicsByBoard )
        allBoardRefs.insert( boardRef );

    for( const auto& [boardRef, pcbUuid] : pcbsByBoard )
        allBoardRefs.insert( boardRef );

    for( const wxString& boardRef : allBoardRefs )
    {
        wxString boardName = boardRef;

        if( auto it = boardTitles.find( boardRef ); it != boardTitles.end() )
            boardName = it->second;

        if( boardName.empty() )
            boardName = boardRef;

        nlohmann::json board = nlohmann::json::object();
        auto           schIt = schematicsByBoard.find( boardRef );
        auto           pcbIt = pcbsByBoard.find( boardRef );

        board["schematic"] = schIt != schematicsByBoard.end() ? ToStdString( schIt->second ) : "";
        board["pcb"] = pcbIt != pcbsByBoard.end() ? ToStdString( pcbIt->second ) : "";

        project["boards"][ToStdString( boardName )] = std::move( board );
    }

    if( project["boards"].empty() && project["pcbs"].is_object() && project["pcbs"].size() == 1
        && project["schematics"].is_object() && project["schematics"].size() == 1 )
    {
        auto pcbIt = project["pcbs"].begin();
        auto schIt = project["schematics"].begin();

        wxString pcbId = wxString::FromUTF8( pcbIt.key() );
        wxString schId = wxString::FromUTF8( schIt.key() );
        wxString boardName = wxString::FromUTF8( pcbIt.value().value( "title", pcbIt.key() ) );

        if( boardName.empty() )
            boardName = schId;

        nlohmann::json board = nlohmann::json::object();
        board["schematic"] = ToStdString( schId );
        board["pcb"] = ToStdString( pcbId );

        project["boards"][ToStdString( boardName )] = std::move( board );
    }

    if( !aIncludeLibraryMetadata )
        return project;

    for( const auto& [uuid, symDoc] : aParser.GetRawDocs( wxS( "SYMBOL" ) ) )
    {
        nlohmann::json sym = nlohmann::json::object();
        sym["source"] = ToStdString( MetaGetString( symDoc, "source" ) );
        sym["description"] = ToStdString( MetaGetString( symDoc, "description" ) );
        sym["title"] = ToStdString( MetaGetString( symDoc, "title", uuid ) );
        sym["display_title"] = sym["title"];
        sym["version"] = "3";
        sym["type"] = MetaGetInt( symDoc, "docType", static_cast<int>( SYMBOL_TYPE::NORMAL ) );
        sym["tags"] = nlohmann::json::object();
        sym["custom_tags"] = nlohmann::json::object();

        project["symbols"][ToStdString( uuid )] = std::move( sym );
    }

    for( const auto& [uuid, fpDoc] : aParser.GetRawDocs( wxS( "FOOTPRINT" ) ) )
    {
        nlohmann::json fp = nlohmann::json::object();
        fp["source"] = ToStdString( MetaGetString( fpDoc, "source" ) );
        fp["description"] = ToStdString( MetaGetString( fpDoc, "description" ) );
        fp["title"] = ToStdString( MetaGetString( fpDoc, "title", uuid ) );
        fp["display_title"] = fp["title"];
        fp["version"] = "3";
        fp["type"] = static_cast<int>( FOOTPRINT_TYPE::NORMAL );
        fp["tags"] = nlohmann::json::object();
        fp["custom_tags"] = nlohmann::json::object();

        project["footprints"][ToStdString( uuid )] = std::move( fp );
    }

    for( const auto& [uuid, deviceDoc] : aParser.GetRawDocs( wxS( "DEVICE" ) ) )
    {
        nlohmann::json dev = nlohmann::json::object();
        dev["source"] = ToStdString( MetaGetString( deviceDoc, "source" ) );
        dev["description"] = ToStdString( MetaGetString( deviceDoc, "description" ) );
        dev["title"] = ToStdString( MetaGetString( deviceDoc, "title", uuid ) );
        dev["version"] = "3";
        dev["tags"] = nlohmann::json::object();
        dev["custom_tags"] = nlohmann::json::object();
        dev["attributes"] = MetaGetValue( deviceDoc, "attributes", nlohmann::json::object() );

        project["devices"][ToStdString( uuid )] = std::move( dev );
    }

    return project;
}
