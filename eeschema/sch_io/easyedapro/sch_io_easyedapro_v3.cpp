/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include "sch_io_easyedapro_v3.h"

#include "sch_easyedapro_v3_parser.h"

#include <io/easyedapro/easyedapro_import_utils.h>
#include <io/easyedapro/easyedapro_parser.h>
#include <io/easyedapro/easyedapro_v3_parser.h>

#include <font/fontconfig.h>
#include <kiplatform/environment.h>
#include <libraries/library_table.h>
#include <libraries/symbol_library_adapter.h>
#include <project_sch.h>
#include <reporter.h>
#include <schematic.h>
#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr.h>
#include <sch_io/sch_io_mgr.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <string_utils.h>
#include <wildcards_and_files_ext.h>

#include <core/map_helpers.h>

#include <wx/filename.h>
#include <wx/log.h>

#include <set>


static long long getFileTimestamp( const wxString& aFileName )
{
    wxFileName fileName( aFileName );

    if( fileName.IsFileReadable() && fileName.GetModificationTime().IsValid() )
        return fileName.GetModificationTime().GetValue().GetValue();

    return 0;
}


static LIB_SYMBOL* LoadV3LibrarySymbolItem( const EASYEDAPRO::V3_DOC_PARSER&      aParser,
                                            const EASYEDAPRO::V3_SYMBOL_LIB_ITEM& aItem, const wxString& aLibName,
                                            const wxString&                             aSymbolName,
                                            const std::map<wxString, EASYEDAPRO::BLOB>& aBlobs,
                                            SCH_EASYEDAPRO_V3_PARSER&                   aSymbolParser )
{
    const EASYEDAPRO::V3_DOC_RAW* rawDoc = aParser.FindRawDoc( wxS( "SYMBOL" ), aItem.symbolUuid );

    if( !rawDoc )
        return nullptr;

    static const std::map<wxString, wxString> emptyAttributes;
    const EASYEDAPRO::PRJ_DEVICE*             device = aItem.hasDevice ? &aItem.device : nullptr;
    const std::map<wxString, wxString>&       attributes = aItem.hasDevice ? aItem.device.attributes : emptyAttributes;

    EASYEDAPRO::SYM_INFO symInfo = aSymbolParser.ParseSymbol( *rawDoc, attributes, aBlobs );

    if( !symInfo.libSymbol )
        return nullptr;

    LIB_SYMBOL& symbol = *symInfo.libSymbol;

    symbol.SetLibId( EASYEDAPRO::ToKiCadLibID( aLibName, aSymbolName ) );
    symbol.SetName( aSymbolName );

    if( device )
    {
        if( auto fpUuid = get_opt( device->attributes, "Footprint" ) )
        {
            wxString              fpTitle = *fpUuid;
            const nlohmann::json& index = aParser.GetLibraryIndex();

            if( index.contains( "footprints" ) && index.at( "footprints" ).is_object() )
            {
                const nlohmann::json& footprints = index.at( "footprints" );
                const std::string     key( fpUuid->ToUTF8() );

                if( footprints.contains( key ) )
                    fpTitle = EASYEDAPRO::GetV3LibraryItemTitle( footprints.at( key ), *fpUuid );
            }

            symbol.GetFootprintField().SetText( aLibName + wxS( ":" ) + fpTitle );
        }

        symbol.SetDescription( EASYEDAPRO::NormalizeEasyEDAText( device->description ) );

        wxString keywords = EASYEDAPRO::KeywordsFromV3Tags( device->tags );

        if( !keywords.empty() )
            symbol.SetKeyWords( keywords );
    }

    return symInfo.libSymbol.release();
}


SCH_IO_EASYEDAPRO_V3::SCH_IO_EASYEDAPRO_V3() :
        SCH_IO( wxS( "EasyEDA Pro (JLCEDA) Schematic v3" ) )
{
    m_reporter = &WXLOG_REPORTER::GetInstance();
}


SCH_IO_EASYEDAPRO_V3::~SCH_IO_EASYEDAPRO_V3() = default;


const EASYEDAPRO::V3_DOC_PARSER& SCH_IO_EASYEDAPRO_V3::getCachedLibraryParser( const wxString& aLibraryPath ) const
{
    long long timestamp = getFileTimestamp( aLibraryPath );

    if( !m_cachedLibraryParser || m_cachedLibraryPath != aLibraryPath || m_cachedLibraryTimestamp != timestamp )
    {
        m_cachedLibraryParser = std::make_unique<EASYEDAPRO::V3_DOC_PARSER>( aLibraryPath );
        m_cachedLibraryParser->LoadLibrary();
        m_cachedLibraryPath = aLibraryPath;
        m_cachedLibraryTimestamp = timestamp;
    }

    return *m_cachedLibraryParser;
}


bool SCH_IO_EASYEDAPRO_V3::CanReadSchematicFile( const wxString& aFileName ) const
{
    return EASYEDAPRO::V3_DOC_PARSER::IsV3Archive( aFileName );
}


bool SCH_IO_EASYEDAPRO_V3::CanReadLibrary( const wxString& aFileName ) const
{
    if( !SCH_IO::CanReadLibrary( aFileName ) )
        return false;

    return EASYEDAPRO::V3_DOC_PARSER::IsV3Library( aFileName, wxS( "SYMBOL" ) );
}


void SCH_IO_EASYEDAPRO_V3::EnumerateSymbolLib( wxArrayString& aSymbolNameList, const wxString& aLibraryPath,
                                               const std::map<std::string, UTF8>* aProperties )
{
    FONTCONFIG_REPORTER_SCOPE fontconfigScope( nullptr );

    const EASYEDAPRO::V3_DOC_PARSER& v3 = getCachedLibraryParser( aLibraryPath );

    for( const auto& [name, item] : EASYEDAPRO::BuildV3SymbolLibraryMap( v3 ) )
    {
        aSymbolNameList.Add( name );
    }
}


void SCH_IO_EASYEDAPRO_V3::EnumerateSymbolLib( std::vector<LIB_SYMBOL*>& aSymbolList, const wxString& aLibraryPath,
                                               const std::map<std::string, UTF8>* aProperties )
{
    FONTCONFIG_REPORTER_SCOPE fontconfigScope( nullptr );

    const EASYEDAPRO::V3_DOC_PARSER&           v3 = getCachedLibraryParser( aLibraryPath );
    std::map<wxString, EASYEDAPRO::V3_SYMBOL_LIB_ITEM> items = EASYEDAPRO::BuildV3SymbolLibraryMap( v3 );
    std::map<wxString, EASYEDAPRO::BLOB>      blobs = EASYEDAPRO::BuildV3BlobMap( v3 );

    wxString libName = LIB_ID::FixIllegalChars( wxFileName( aLibraryPath ).GetName(), true );

    SCH_EASYEDAPRO_V3_PARSER parser( nullptr, nullptr );

    for( const auto& [symbolName, item] : items )
    {
        try
        {
            if( LIB_SYMBOL* symbol = LoadV3LibrarySymbolItem( v3, item, libName, symbolName, blobs, parser ) )
                aSymbolList.push_back( symbol );
        }
        catch( nlohmann::json::exception& e )
        {
            wxLogWarning(
                    wxString::Format( _( "EasyEDA Pro v3 symbol '%s' in '%s' was skipped due to parse error: %s" ),
                                      symbolName, aLibraryPath, e.what() ) );
        }
    }
}


LIB_SYMBOL* SCH_IO_EASYEDAPRO_V3::LoadSymbol( const wxString& aLibraryPath, const wxString& aAliasName,
                                              const std::map<std::string, UTF8>* aProperties )
{
    FONTCONFIG_REPORTER_SCOPE fontconfigScope( nullptr );

    const EASYEDAPRO::V3_DOC_PARSER&                   v3 = getCachedLibraryParser( aLibraryPath );
    std::map<wxString, EASYEDAPRO::V3_SYMBOL_LIB_ITEM> items = EASYEDAPRO::BuildV3SymbolLibraryMap( v3 );

    auto itemIt = items.find( aAliasName );

    if( itemIt == items.end() )
        return nullptr;

    wxString libName = LIB_ID::FixIllegalChars( wxFileName( aLibraryPath ).GetName(), true );

    SCH_EASYEDAPRO_V3_PARSER parser( nullptr, nullptr );

    try
    {
        return LoadV3LibrarySymbolItem( v3, itemIt->second, libName, aAliasName,
                                        EASYEDAPRO::BuildV3BlobMap( v3 ), parser );
    }
    catch( nlohmann::json::exception& e )
    {
        THROW_IO_ERROR(
                wxString::Format( _( "Cannot load symbol '%s' from '%s': %s" ), aAliasName, aLibraryPath, e.what() ) );
    }
}


SCH_SHEET* SCH_IO_EASYEDAPRO_V3::LoadSchematicFile( const wxString& aFileName, SCHEMATIC* aSchematic,
                                                    SCH_SHEET*                         aAppendToMe,
                                                    const std::map<std::string, UTF8>* aProperties )
{
    wxCHECK( !aFileName.IsEmpty() && aSchematic, nullptr );

    FONTCONFIG_REPORTER_SCOPE fontconfigScope( &LOAD_INFO_REPORTER::GetInstance() );

    SCH_SHEET* rootSheet = nullptr;

    if( aAppendToMe )
    {
        wxCHECK_MSG( aSchematic->IsValid(), nullptr, wxS( "Can't append to a schematic with no root!" ) );

        rootSheet = aAppendToMe;
    }
    else
    {
        rootSheet = new SCH_SHEET( aSchematic );
        rootSheet->SetFileName( aFileName );
        aSchematic->SetTopLevelSheets( { rootSheet } );
    }

    if( !rootSheet->GetScreen() )
    {
        SCH_SCREEN* screen = new SCH_SCREEN( aSchematic );

        screen->SetFileName( aFileName );
        rootSheet->SetScreen( screen );

        const_cast<KIID&>( rootSheet->m_Uuid ) = screen->GetUuid();
    }

    EASYEDAPRO::V3_DOC_PARSER v3( aFileName );
    v3.Load();

    nlohmann::json project = EASYEDAPRO::BuildV3ProjectIndexFromRawDocs( v3 );

    std::map<wxString, EASYEDAPRO::PRJ_SCHEMATIC> prjSchematics = project.at( "schematics" );

    wxString schematicToLoad;

    if( aProperties && aProperties->contains( "sch_id" ) )
    {
        schematicToLoad = wxString::FromUTF8( aProperties->at( "sch_id" ) );
    }
    else if( prjSchematics.size() == 1 )
    {
        schematicToLoad = prjSchematics.begin()->first;
    }
    else if( m_choose_project_handler )
    {
        std::vector<IMPORT_PROJECT_DESC> chosen =
                m_choose_project_handler( EASYEDAPRO::ProjectToSelectorDialog( project, false, true ) );

        if( !chosen.empty() )
            schematicToLoad = chosen[0].SchematicId;
    }
    else if( !prjSchematics.empty() )
    {
        schematicToLoad = prjSchematics.begin()->first;
    }

    if( schematicToLoad.empty() )
        return nullptr;

    auto prjSchIt = prjSchematics.find( schematicToLoad );

    if( prjSchIt == prjSchematics.end() )
    {
        THROW_IO_ERROR(
                wxString::Format( _( "Schematic document '%s' not found in '%s'" ), schematicToLoad, aFileName ) );
    }

    wxFileName sourceName( aFileName );
    wxString   libName = EASYEDAPRO::ShortenLibName( sourceName.GetName() );

    wxFileName libFileName( sourceName.GetPath(), libName, FILEEXT::KiCadSymbolLibFileExtension );

    wxString rootBaseName = EscapeString( prjSchIt->second.name, CTX_FILENAME );

    wxFileName rootFname( aFileName );
    rootFname.SetFullName( rootBaseName + wxS( "." ) + wxString::FromUTF8( FILEEXT::KiCadSchematicFileExtension ) );

    rootSheet->SetName( prjSchIt->second.name );
    rootSheet->SetFileName( rootFname.GetFullPath() );
    rootSheet->GetScreen()->SetFileName( rootFname.GetFullPath() );

    SCH_EASYEDAPRO_V3_PARSER parser( nullptr, nullptr );

    std::map<wxString, EASYEDAPRO::PRJ_SYMBOL>    prjSymbols;
    std::map<wxString, EASYEDAPRO::PRJ_FOOTPRINT> prjFootprints;
    std::map<wxString, EASYEDAPRO::PRJ_DEVICE>    prjDevices;

    try
    {
        if( project.contains( "symbols" ) )
            prjSymbols = project.at( "symbols" );

        if( project.contains( "footprints" ) )
            prjFootprints = project.at( "footprints" );

        if( project.contains( "devices" ) )
            prjDevices = project.at( "devices" );
    }
    catch( nlohmann::json::exception& e )
    {
        THROW_IO_ERROR(
                wxString::Format( _( "Failed to parse EasyEDA Pro v3 symbol/device metadata: %s" ), e.what() ) );
    }

    // Schematic components reference Symbol (geometry) and Device (BOM attrs) separately.
    // Build geometry by symbol UUID; assign stable project-lib names from each Device.
    std::set<wxString>           usedLibNames;
    std::map<wxString, wxString> deviceLibNames =
            EASYEDAPRO::BuildV3DeviceLibNames( project, prjDevices, &usedLibNames );

    std::map<wxString, EASYEDAPRO::SYM_INFO> symbols;
    std::map<wxString, EASYEDAPRO::BLOB>     blobs = EASYEDAPRO::BuildV3BlobMap( v3 );

    for( const auto& [symbolUuid, rawDoc] : v3.GetRawDocs( wxS( "SYMBOL" ) ) )
    {
        auto symMetaIt = prjSymbols.find( symbolUuid );

        if( symMetaIt == prjSymbols.end() )
            continue;

        EASYEDAPRO::SYM_INFO symInfo;

        try
        {
            // Geometry only.  Device BOM fields come from the component Device ATTR.
            symInfo = parser.ParseSymbol( rawDoc, {}, blobs );
        }
        catch( nlohmann::json::exception& e )
        {
            wxLogWarning( wxString::Format( _( "EasyEDA Pro v3 symbol '%s' was skipped due to parse error: %s" ),
                                            symbolUuid, e.what() ) );
            continue;
        }

        if( !symInfo.libSymbol )
            continue;

        LIB_ID libID = EASYEDAPRO::ToKiCadLibID( libName, symMetaIt->second.title );
        symInfo.libSymbol->SetLibId( libID );
        symInfo.libSymbol->SetName( symMetaIt->second.title );

        symInfo.libSymbol->SetDescription( EASYEDAPRO::NormalizeEasyEDAText( symMetaIt->second.desc ) );

        wxString keywords = EASYEDAPRO::KeywordsFromV3Tags( symMetaIt->second.tags );

        if( !keywords.empty() )
            symInfo.libSymbol->SetKeyWords( keywords );

        symbols.emplace( symbolUuid, std::move( symInfo ) );
    }

    const std::vector<EASYEDAPRO::PRJ_SHEET>& prjSchematicSheets = prjSchIt->second.sheets;

    bool rootAssigned = false;
    int  subSheetIndex = 0;

    for( const EASYEDAPRO::PRJ_SHEET& prjSheet : prjSchematicSheets )
    {
        const EASYEDAPRO::V3_DOC_RAW* pageRawDoc = v3.FindRawDoc( wxS( "SCH_PAGE" ), prjSheet.uuid );

        if( !pageRawDoc )
        {
            wxLogWarning( wxString::Format( _( "EasyEDA Pro v3 schematic page '%s' was not found and was skipped." ),
                                            prjSheet.uuid ) );
            continue;
        }

        wxString sheetId = wxString::Format( "%d", prjSheet.id );

        // The first page fills the root sheet so no empty index sheet is left behind;
        // any remaining pages become subsheets hanging off the root.
        if( !rootAssigned )
        {
            SCH_SHEET_PATH sheetPath;
            sheetPath.push_back( rootSheet );
            sheetPath.SetPageNumber( sheetId );
            aSchematic->SetCurrentSheet( sheetPath );

            try
            {
                parser.ParseSchematic( aSchematic, rootSheet, project, symbols, blobs, *pageRawDoc, libName );
            }
            catch( nlohmann::json::exception& e )
            {
                wxLogWarning(
                        wxString::Format( _( "EasyEDA Pro v3 schematic page '%s' was skipped due to parse error: %s" ),
                                          prjSheet.uuid, e.what() ) );
                continue;
            }

            rootSheet->SetName( prjSheet.name );
            rootSheet->GetScreen()->SetPageNumber( sheetId );
            rootAssigned = true;
            continue;
        }

        wxString sheetBaseName = sheetId + wxS( "_" ) + EscapeString( prjSheet.name, CTX_FILENAME );

        wxFileName sheetFname( aFileName );
        sheetFname.SetFullName( sheetBaseName + wxS( "." )
                                + wxString::FromUTF8( FILEEXT::KiCadSchematicFileExtension ) );

        wxFileName relSheetPath( sheetFname );
        relSheetPath.MakeRelativeTo( rootFname.GetPath() );

        std::unique_ptr<SCH_SHEET> subSheet = std::make_unique<SCH_SHEET>( aSchematic );
        subSheet->SetFileName( relSheetPath.GetFullPath() );
        subSheet->SetName( prjSheet.name );

        SCH_SCREEN* screen = new SCH_SCREEN( aSchematic );
        screen->SetFileName( sheetFname.GetFullPath() );
        screen->SetPageNumber( sheetId );
        subSheet->SetScreen( screen );

        VECTOR2I pos;
        pos.x = schIUScale.MilsToIU( 200 );
        pos.y = schIUScale.MilsToIU( 200 ) + ( subSheet->GetSize().y + schIUScale.MilsToIU( 200 ) ) * subSheetIndex;

        subSheet->SetPosition( pos );

        SCH_SHEET_PATH sheetPath;
        sheetPath.push_back( rootSheet );
        sheetPath.push_back( subSheet.get() );
        sheetPath.SetPageNumber( sheetId );
        aSchematic->SetCurrentSheet( sheetPath );

        try
        {
            parser.ParseSchematic( aSchematic, subSheet.get(), project, symbols, blobs, *pageRawDoc, libName );
        }
        catch( nlohmann::json::exception& e )
        {
            wxLogWarning(
                    wxString::Format( _( "EasyEDA Pro v3 schematic page '%s' was skipped due to parse error: %s" ),
                                      prjSheet.uuid, e.what() ) );
            continue;
        }

        rootSheet->GetScreen()->Append( subSheet.release() );
        subSheetIndex++;
    }

    IO_RELEASER<SCH_IO> schPlugin( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD ) );

    SYMBOL_LIBRARY_ADAPTER* symAdapter = PROJECT_SCH::SymbolLibAdapter( &aSchematic->Project() );
    LIBRARY_TABLE*          table = symAdapter->ProjectTable().value_or( nullptr );
    wxCHECK_MSG( table, nullptr, "Could not load symbol lib table." );

    if( !table->HasRow( libName ) )
    {
        schPlugin->CreateLibrary( libFileName.GetFullPath() );
        wxString libTableUri = wxS( "${KIPRJMOD}/" ) + libFileName.GetFullName();

        LIBRARY_TABLE_ROW& row = table->InsertRow();
        row.SetNickname( libName );
        row.SetURI( libTableUri );
        row.SetType( "KiCad" );

        table->Save();
        symAdapter->LoadOne( libName );
    }

    std::map<std::string, UTF8> properties;
    properties.emplace( SCH_IO_KICAD_SEXPR::PropBuffering, wxEmptyString );

    std::set<wxString> symbolsSavedViaDevices;

    // Project library entries are devices: geometry from Symbol + BOM fields from Device.
    for( const auto& [devUuid, device] : prjDevices )
    {
        auto symbolAttr = get_opt( device.attributes, "Symbol" );

        if( !symbolAttr )
            continue;

        const EASYEDAPRO::V3_DOC_RAW* symbolDoc = v3.FindRawDoc( wxS( "SYMBOL" ), *symbolAttr );

        if( !symbolDoc )
            continue;

        EASYEDAPRO::SYM_INFO deviceSymInfo;

        try
        {
            deviceSymInfo = parser.ParseSymbol( *symbolDoc, device.attributes, blobs );
        }
        catch( nlohmann::json::exception& e )
        {
            wxLogWarning( wxString::Format( _( "EasyEDA Pro v3 device '%s' was skipped due to parse error: %s" ),
                                            device.title, e.what() ) );
            continue;
        }

        if( !deviceSymInfo.libSymbol )
            continue;

        auto nameIt = deviceLibNames.find( devUuid );
        wxString itemName = ( nameIt != deviceLibNames.end() ) ? nameIt->second : device.title;

        LIB_ID libID = EASYEDAPRO::ToKiCadLibID( libName, itemName );
        deviceSymInfo.libSymbol->SetLibId( libID );
        deviceSymInfo.libSymbol->SetName( itemName );

        if( auto fpUuid = get_opt( device.attributes, "Footprint" ) )
        {
            if( auto prjFp = get_opt( prjFootprints, *fpUuid ) )
                deviceSymInfo.libSymbol->GetFootprintField().SetText( libName + wxS( ":" ) + prjFp->title );
        }

        wxString description = device.description;

        if( description.empty() )
            description = get_def( device.attributes, wxS( "Description" ), wxEmptyString );

        if( !description.empty() )
        {
            deviceSymInfo.libSymbol->SetDescription( EASYEDAPRO::NormalizeEasyEDAText(
                    EASYEDAPRO::ResolveDeviceFieldVariables( description, device.attributes ) ) );
        }

        wxString keywords = EASYEDAPRO::KeywordsFromV3Tags( device.tags );

        if( !keywords.empty() )
            deviceSymInfo.libSymbol->SetKeyWords( keywords );

        schPlugin->SaveSymbol( libFileName.GetFullPath(), deviceSymInfo.libSymbol.release(), &properties );
        symbolsSavedViaDevices.insert( *symbolAttr );
    }

    // Keep geometry-only entries for symbols not referenced by any device (e.g. some helpers).
    for( auto& [symbolUuid, symInfo] : symbols )
    {
        if( !symInfo.libSymbol || symbolsSavedViaDevices.contains( symbolUuid ) )
            continue;

        wxString itemName = EASYEDAPRO::MakeUniqueLibName( usedLibNames, symInfo.libSymbol->GetName(),
                                                           symbolUuid );
        LIB_ID   libID = EASYEDAPRO::ToKiCadLibID( libName, itemName );
        symInfo.libSymbol->SetLibId( libID );
        symInfo.libSymbol->SetName( itemName );

        schPlugin->SaveSymbol( libFileName.GetFullPath(), symInfo.libSymbol.release(), &properties );
    }

    schPlugin->SaveLibrary( libFileName.GetFullPath() );

    aSchematic->CurrentSheet().UpdateAllScreenReferences();
    aSchematic->FixupJunctionsAfterImport();

    if( v3.GetSkippedCount() > 0 )
    {
        wxLogWarning( wxString::Format( _( "EasyEDA (JLCEDA) Pro v3 import skipped %d unsupported object(s)." ),
                                        v3.GetSkippedCount() ) );
    }

    return rootSheet;
}
