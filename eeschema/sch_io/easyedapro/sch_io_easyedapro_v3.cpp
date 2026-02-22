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


SCH_IO_EASYEDAPRO_V3::SCH_IO_EASYEDAPRO_V3() :
        SCH_IO( wxS( "EasyEDA Pro (JLCEDA) Schematic v3" ) )
{
    m_reporter = &WXLOG_REPORTER::GetInstance();
}


bool SCH_IO_EASYEDAPRO_V3::CanReadSchematicFile( const wxString& aFileName ) const
{
    return EASYEDAPRO::V3_DOC_PARSER::IsV3Archive( aFileName );
}


SCH_SHEET* SCH_IO_EASYEDAPRO_V3::LoadSchematicFile( const wxString& aFileName,
                                                    SCHEMATIC* aSchematic,
                                                    SCH_SHEET* aAppendToMe,
                                                    const std::map<std::string, UTF8>* aProperties )
{
    wxCHECK( !aFileName.IsEmpty() && aSchematic, nullptr );

    FONTCONFIG_REPORTER_SCOPE fontconfigScope( &LOAD_INFO_REPORTER::GetInstance() );

    SCH_SHEET* rootSheet = nullptr;

    if( aAppendToMe )
    {
        wxCHECK_MSG( aSchematic->IsValid(), nullptr,
                     wxS( "Can't append to a schematic with no root!" ) );

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
        std::vector<IMPORT_PROJECT_DESC> chosen = m_choose_project_handler(
                EASYEDAPRO::ProjectToSelectorDialog( project, false, true ) );

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
        THROW_IO_ERROR( wxString::Format( _( "Schematic document '%s' not found in '%s'" ),
                                          schematicToLoad, aFileName ) );
    }

    wxFileName sourceName( aFileName );
    wxString   libName = EASYEDAPRO::ShortenLibName( sourceName.GetName() );

    wxFileName libFileName( sourceName.GetPath(), libName, FILEEXT::KiCadSymbolLibFileExtension );

    wxString rootBaseName = EscapeString( prjSchIt->second.name, CTX_FILENAME );

    wxFileName rootFname( aFileName );
    rootFname.SetFullName( rootBaseName + wxS( "." )
                           + wxString::FromUTF8( FILEEXT::KiCadSchematicFileExtension ) );

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
        THROW_IO_ERROR( wxString::Format( _( "Failed to parse EasyEDA Pro v3 symbol/device metadata: %s" ),
                                          e.what() ) );
    }

    std::map<wxString, EASYEDAPRO::SYM_INFO> symbols;
    std::map<wxString, EASYEDAPRO::BLOB>     blobs;

    for( const auto& [blobDocUuid, rawDoc] : v3.GetRawDocs( wxS( "BLOB" ) ) )
    {
        for( const EASYEDAPRO::V3_ROW& row : rawDoc.rows )
        {
            if( row.type != wxS( "BLOB" ) )
                continue;

            try
            {
                EASYEDAPRO::BLOB blob;
                blob.objectId = EASYEDAPRO::V3GetString( row.inner, "objectId" );
                blob.url = EASYEDAPRO::V3GetString( row.inner, "url" );
                blobs[blob.objectId] = blob;
            }
            catch( nlohmann::json::exception& e )
            {
                wxLogWarning( wxString::Format(
                        _( "EasyEDA Pro v3 blob in '%s' was skipped due to parse error: %s" ),
                        blobDocUuid, e.what() ) );
            }
        }
    }

    for( const auto& [symbolUuid, rawDoc] : v3.GetRawDocs( wxS( "SYMBOL" ) ) )
    {
        auto symMetaIt = prjSymbols.find( symbolUuid );

        if( symMetaIt == prjSymbols.end() )
            continue;

        wxString                     description;
        wxString                     customTags;
        std::map<wxString, wxString> deviceAttributes;
        wxString                     fpTitle;

        for( auto& [devUuid, device] : prjDevices )
        {
            auto val = get_opt( device.attributes, "Symbol" );

            if( !val || *val != symbolUuid )
                continue;

            description = device.description;
            deviceAttributes = device.attributes;

            if( device.custom_tags.is_string() )
                customTags = device.custom_tags.get<wxString>();

            if( auto fpUuid = get_opt( device.attributes, "Footprint" ) )
            {
                if( auto prjFp = get_opt( prjFootprints, *fpUuid ) )
                {
                    fpTitle = prjFp->title;
                    break;
                }
            }
        }

        EASYEDAPRO::SYM_INFO symInfo;

        try
        {
            symInfo = parser.ParseSymbol( rawDoc, deviceAttributes, blobs );
        }
        catch( nlohmann::json::exception& e )
        {
            wxLogWarning( wxString::Format(
                    _( "EasyEDA Pro v3 symbol '%s' was skipped due to parse error: %s" ),
                    symbolUuid, e.what() ) );
            continue;
        }

        if( !symInfo.libSymbol )
            continue;

        LIB_ID libID = EASYEDAPRO::ToKiCadLibID( libName, symMetaIt->second.title );
        symInfo.libSymbol->SetLibId( libID );
        symInfo.libSymbol->SetName( symMetaIt->second.title );
        symInfo.libSymbol->GetFootprintField().SetText( libName + wxS( ":" ) + fpTitle );

        wxString keywords = customTags;
        keywords.Replace( wxS( ":" ), wxS( " " ), true );
        symInfo.libSymbol->SetKeyWords( keywords );

        description.Replace( wxS( "\u2103" ), wxS( "\u00B0C" ), true );
        symInfo.libSymbol->SetDescription( description );

        symbols.emplace( symbolUuid, std::move( symInfo ) );
    }

    const std::vector<EASYEDAPRO::PRJ_SHEET>& prjSchematicSheets = prjSchIt->second.sheets;
    const int                                  schSheetsCount = prjSchematicSheets.size();

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

        if( schSheetsCount > 1 )
        {
            wxString sheetBaseName = sheetId + wxS( "_" )
                                     + EscapeString( prjSheet.name, CTX_FILENAME );

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

            int sheetIndex = std::max( prjSheet.id - 1, 0 );

            VECTOR2I pos;
            pos.x = schIUScale.MilsToIU( 200 );
            pos.y = schIUScale.MilsToIU( 200 )
                    + ( subSheet->GetSize().y + schIUScale.MilsToIU( 200 ) ) * sheetIndex;

            subSheet->SetPosition( pos );

            SCH_SHEET_PATH sheetPath;
            sheetPath.push_back( rootSheet );
            sheetPath.push_back( subSheet.get() );
            sheetPath.SetPageNumber( sheetId );
            aSchematic->SetCurrentSheet( sheetPath );

            try
            {
                parser.ParseSchematic( aSchematic, subSheet.get(), project, symbols, blobs,
                                       *pageRawDoc, libName );
            }
            catch( nlohmann::json::exception& e )
            {
                wxLogWarning( wxString::Format(
                        _( "EasyEDA Pro v3 schematic page '%s' was skipped due to parse error: %s" ),
                        prjSheet.uuid, e.what() ) );
                continue;
            }

            rootSheet->GetScreen()->Append( subSheet.release() );
        }
        else
        {
            try
            {
                parser.ParseSchematic( aSchematic, rootSheet, project, symbols, blobs,
                                       *pageRawDoc, libName );
            }
            catch( nlohmann::json::exception& e )
            {
                wxLogWarning( wxString::Format(
                        _( "EasyEDA Pro v3 schematic page '%s' was skipped due to parse error: %s" ),
                        prjSheet.uuid, e.what() ) );
            }
        }
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

    for( auto& [symbolUuid, symInfo] : symbols )
    {
        if( symInfo.libSymbol )
        {
            schPlugin->SaveSymbol( libFileName.GetFullPath(), symInfo.libSymbol.release(),
                                   &properties );
        }
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
