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
#include <core/map_helpers.h>
#include <ki_exception.h>
#include <string_utils.h>
#include <vector>
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

// clang-format off
static const std::vector<wxString> c_deviceAttributesWhitelist = { wxS( "Value" ),
                                                                   wxS( "Datasheet" ),
                                                                   wxS( "Manufacturer Part" ),
                                                                   wxS( "Manufacturer" ),
                                                                   wxS( "BOM_Manufacturer Part" ),
                                                                   wxS( "BOM_Manufacturer" ),
                                                                   wxS( "Supplier Part" ),
                                                                   wxS( "Supplier" ),
                                                                   wxS( "BOM_Supplier Part" ),
                                                                   wxS( "BOM_Supplier" ),
                                                                   wxS( "LCSC Part Name" ) };
// clang-format on


static std::string ToStdString( const wxString& aStr )
{
    return std::string( aStr.ToUTF8() );
}


static wxString ToWxString( const std::string& aStr )
{
    return wxString::FromUTF8( aStr.c_str() );
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


nlohmann::json EASYEDAPRO::BuildV3ProjectIndexFromRawDocs( const V3_DOC_PARSER& aParser, bool aIncludeLibraryMetadata )
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
    int                                         pageOrder = 0;

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
                sch["sheets"].push_back( nlohmann::json::object( { { "id", sheetId++ },
                                                                   { "name", ToStdString( page.name ) },
                                                                   { "uuid", ToStdString( page.uuid ) } } ) );
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
        sym["tags"] = MetaGetValue( symDoc, "tags", nlohmann::json::object() );

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
        fp["tags"] = MetaGetValue( fpDoc, "tags", nlohmann::json::object() );

        project["footprints"][ToStdString( uuid )] = std::move( fp );
    }

    for( const auto& [uuid, deviceDoc] : aParser.GetRawDocs( wxS( "DEVICE" ) ) )
    {
        nlohmann::json dev = nlohmann::json::object();
        dev["source"] = ToStdString( MetaGetString( deviceDoc, "source" ) );
        dev["description"] = ToStdString( MetaGetString( deviceDoc, "description" ) );
        dev["title"] = ToStdString( MetaGetString( deviceDoc, "title", uuid ) );
        dev["version"] = "3";
        dev["tags"] = MetaGetValue( deviceDoc, "tags", nlohmann::json::object() );
        dev["attributes"] = MetaGetValue( deviceDoc, "attributes", nlohmann::json::object() );

        project["devices"][ToStdString( uuid )] = std::move( dev );
    }

    return project;
}


std::map<wxString, EASYEDAPRO::BLOB> EASYEDAPRO::BuildV3BlobMap( const V3_DOC_PARSER& aParser )
{
    std::map<wxString, BLOB> blobs;

    for( const auto& [blobDocUuid, rawDoc] : aParser.GetRawDocs( wxS( "BLOB" ) ) )
    {
        for( const V3_ROW& row : rawDoc.rows )
        {
            if( row.type != wxS( "BLOB" ) )
                continue;

            try
            {
                BLOB blob;
                blob.objectId = V3GetString( row.outer, "id" );
                blob.url = V3GetString( row.inner, "content" );
                blobs[blob.objectId] = blob;
            }
            catch( nlohmann::json::exception& e )
            {
                wxLogWarning( wxString::Format( _( "EasyEDA Pro v3 blob in '%s' was skipped due to parse error: %s" ),
                                                blobDocUuid, e.what() ) );
            }
        }
    }

    return blobs;
}


wxString EASYEDAPRO::GetV3LibraryItemTitle( const nlohmann::json& aMetadata, const wxString& aUuid )
{
    wxString title = EASYEDAPRO::V3GetString( aMetadata, "display_title" );

    if( title.empty() )
        title = EASYEDAPRO::V3GetString( aMetadata, "title" );

    if( title.empty() )
        title = aUuid;

    return title;
}


wxString EASYEDAPRO::KeywordsFromV3Tags( const nlohmann::json& aTags )
{
    if( !aTags.is_object() )
        return {};

    wxString keywords;

    auto appendTagName = [&]( const char* aKey )
    {
        if( !aTags.contains( aKey ) || !aTags.at( aKey ).is_object() )
            return;

        wxString name = V3GetString( aTags.at( aKey ), "name" );

        if( name.empty() )
            return;

        if( !keywords.empty() )
            keywords += wxS( " " );

        keywords += name;
    };

    appendTagName( "parent_tag" );
    appendTagName( "child_tag" );

    return keywords;
}


wxString EASYEDAPRO::ResolveDeviceFieldVariables( const wxString&                      aInput,
                                                  const std::map<wxString, wxString>& aDeviceAttributes )
{
    wxString inputText = aInput;
    wxString resolvedText;
    int      variableCount = 0;

    // Resolve variables: ={Variable1}text{Variable2}
    do
    {
        if( !inputText.StartsWith( wxS( "={" ) ) )
            return inputText;

        resolvedText.Clear();
        variableCount = 0;

        for( size_t i = 1; i < inputText.size(); )
        {
            wxUniChar c = inputText[i++];

            if( c == '{' )
            {
                wxString varName;
                bool     endFound = false;

                while( i < inputText.size() )
                {
                    c = inputText[i++];

                    if( c == '}' )
                    {
                        endFound = true;
                        break;
                    }

                    varName << c;
                }

                if( !endFound )
                    return inputText;

                wxString varValue =
                        get_def( aDeviceAttributes, varName, wxString::Format( wxS( "{%s!}" ), varName ) );

                resolvedText << varValue;
                variableCount++;
            }
            else
            {
                resolvedText << c;
            }
        }

        inputText = resolvedText;
    } while( variableCount > 0 );

    return resolvedText;
}


wxString EASYEDAPRO::NormalizeEasyEDAText( wxString aText )
{
    // ℃ -> °C
    aText.Replace( wxS( "\u2103" ), wxS( "\u00B0C" ), true );
    return aText;
}


wxString EASYEDAPRO::MakeUniqueLibName( std::set<wxString>& aUsedNames, const wxString& aName,
                                        const wxString& aFallback )
{
    wxString uniqueBase = aName.empty() ? aFallback : aName;
    wxString uniqueName = uniqueBase;
    int      suffix = 2;

    while( aUsedNames.contains( uniqueName ) )
        uniqueName = uniqueBase + wxString::Format( wxS( "_%d" ), suffix++ );

    aUsedNames.insert( uniqueName );
    return uniqueName;
}


std::map<wxString, wxString> EASYEDAPRO::BuildV3DeviceLibNames( nlohmann::json& aProject,
                                                               const std::map<wxString, PRJ_DEVICE>& aDevices,
                                                               std::set<wxString>* aUsedNames )
{
    std::map<wxString, wxString> deviceLibNames;
    std::set<wxString>           localUsedNames;
    std::set<wxString>&          usedLibNames = aUsedNames ? *aUsedNames : localUsedNames;
    nlohmann::json               deviceLibNamesJson = nlohmann::json::object();

    for( const auto& [devUuid, device] : aDevices )
    {
        wxString libNameForDevice = MakeUniqueLibName( usedLibNames, device.title, devUuid );
        deviceLibNames[devUuid] = libNameForDevice;
        deviceLibNamesJson[std::string( devUuid.ToUTF8() )] = std::string( libNameForDevice.ToUTF8() );
    }

    aProject["device_lib_names"] = std::move( deviceLibNamesJson );
    return deviceLibNames;
}


wxString EASYEDAPRO::LookupV3DeviceLibName( const nlohmann::json& aProject, const wxString& aDeviceUuid )
{
    if( aDeviceUuid.empty() || !aProject.contains( "device_lib_names" )
        || !aProject.at( "device_lib_names" ).is_object() )
    {
        return {};
    }

    std::string deviceIdUtf8 = std::string( aDeviceUuid.ToUTF8() );
    const auto& names = aProject.at( "device_lib_names" );

    if( names.contains( deviceIdUtf8 ) && names.at( deviceIdUtf8 ).is_string() )
        return names.at( deviceIdUtf8 ).get<wxString>();

    return {};
}


EASYEDAPRO::V3_DEVICE_DATA EASYEDAPRO::GetV3DeviceData( const nlohmann::json& aProject,
                                                        const wxString&       aDeviceUuid )
{
    V3_DEVICE_DATA data;

    if( aDeviceUuid.empty() || !aProject.contains( "devices" ) || !aProject.at( "devices" ).is_object() )
        return data;

    std::string deviceIdUtf8 = std::string( aDeviceUuid.ToUTF8() );

    if( !aProject.at( "devices" ).contains( deviceIdUtf8 ) )
        return data;

    const nlohmann::json& dev = aProject.at( "devices" ).at( deviceIdUtf8 );

    data.found = true;
    data.description = V3GetString( dev, "description" );

    if( dev.contains( "attributes" ) && dev.at( "attributes" ).is_object() )
    {
        for( const auto& [key, value] : dev.at( "attributes" ).items() )
            data.attributes[wxString::FromUTF8( key )] = V3JsonToString( value );
    }

    return data;
}


wxString EASYEDAPRO::ResolveV3DeviceValueText( const std::map<wxString, wxString>& aDeviceAttributes )
{
    wxString valueText = get_def( aDeviceAttributes, wxS( "Value" ), wxEmptyString );

    if( valueText.empty() )
        valueText = get_def( aDeviceAttributes, wxS( "Name" ), wxEmptyString );

    return NormalizeEasyEDAText( ResolveDeviceFieldVariables( valueText, aDeviceAttributes ) );
}


void EASYEDAPRO::ForEachImportedDeviceField(
        const std::map<wxString, wxString>& aDeviceAttributes, bool aIncludeValue,
        const std::function<void( const wxString& aKey, const wxString& aValue )>& aCallback )
{
    for( const wxString& attrKey : c_deviceAttributesWhitelist )
    {
        if( !aIncludeValue && attrKey == wxS( "Value" ) )
            continue;

        auto valOpt = get_opt( aDeviceAttributes, attrKey );

        if( !valOpt || valOpt->empty() )
            continue;

        aCallback( attrKey,
                   NormalizeEasyEDAText( ResolveDeviceFieldVariables( *valOpt, aDeviceAttributes ) ) );
    }
}


template <typename Map>
static wxString MakeUniqueV3LibraryName( const Map& aItems, const wxString& aName, const wxString& aFallback )
{
    wxString uniqueBase = aName.empty() ? aFallback : aName;
    wxString uniqueName = uniqueBase;

    int suffix = 2;
    while( aItems.contains( uniqueName ) )
        uniqueName = uniqueBase + wxString::Format( wxS( "_%d" ), suffix++ );

    return uniqueName;
}


std::map<wxString, EASYEDAPRO::V3_SYMBOL_LIB_ITEM> EASYEDAPRO::BuildV3SymbolLibraryMap( const V3_DOC_PARSER& aParser )
{
    std::map<wxString, V3_SYMBOL_LIB_ITEM> items;
    const nlohmann::json&                  index = aParser.GetLibraryIndex();

    const bool hasDevices = index.is_object() && index.contains( "devices" ) && index.at( "devices" ).is_object()
                            && !index.at( "devices" ).empty();

    if( hasDevices )
    {
        try
        {
            for( const auto& [devUuidKey, deviceJson] : index.at( "devices" ).items() )
            {
                if( !deviceJson.is_object() )
                    continue;

                PRJ_DEVICE device = deviceJson;
                auto       symbolIt = device.attributes.find( wxS( "Symbol" ) );

                if( symbolIt == device.attributes.end() || symbolIt->second.empty() )
                    continue;

                if( !aParser.FindRawDoc( wxS( "SYMBOL" ), symbolIt->second ) )
                    continue;

                wxString deviceUuid = V3GetString( deviceJson, "uuid", ToWxString( devUuidKey ) );
                wxString name = device.title.empty() ? GetV3LibraryItemTitle( deviceJson, deviceUuid ) : device.title;
                V3_SYMBOL_LIB_ITEM item;
                item.symbolUuid = symbolIt->second;
                item.device = std::move( device );
                item.hasDevice = true;

                items.emplace( MakeUniqueV3LibraryName( items, name, deviceUuid ), std::move( item ) );
            }
        }
        catch( nlohmann::json::exception& e )
        {
            wxLogWarning( wxString::Format( _( "Failed to parse EasyEDA Pro v3 device metadata: %s" ), e.what() ) );
            return {};
        }

        return items;
    }

    for( const auto& [name, symbolUuid] : BuildV3LibraryItemMap( aParser, "symbols", wxS( "SYMBOL" ) ) )
    {
        V3_SYMBOL_LIB_ITEM item;
        item.symbolUuid = symbolUuid;
        items.emplace( name, std::move( item ) );
    }

    return items;
}


std::map<wxString, wxString> EASYEDAPRO::BuildV3LibraryItemMap( const V3_DOC_PARSER& aParser, const char* aIndexKey,
                                                                const wxString& aDocType )
{
    std::map<wxString, wxString> titlesByUuid;
    const nlohmann::json&        index = aParser.GetLibraryIndex();

    if( index.is_object() && index.contains( aIndexKey ) && index.at( aIndexKey ).is_object() )
    {
        for( const auto& [uuidKey, metadata] : index.at( aIndexKey ).items() )
        {
            wxString uuid = V3GetString( metadata, "uuid", ToWxString( uuidKey ) );
            titlesByUuid[uuid] = GetV3LibraryItemTitle( metadata, uuid );
        }
    }

    std::map<wxString, wxString> nameToUuid;

    for( const auto& [uuid, rawDoc] : aParser.GetRawDocs( aDocType ) )
    {
        (void) rawDoc;

        wxString name = uuid;

        if( auto it = titlesByUuid.find( uuid ); it != titlesByUuid.end() )
            name = it->second;

        nameToUuid[MakeUniqueV3LibraryName( nameToUuid, name, uuid )] = uuid;
    }

    return nameToUuid;
}
