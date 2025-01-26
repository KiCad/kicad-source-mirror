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

#include "sch_easyedapro_parser.h"
#include "sch_io_easyedapro.h"

#include <font/fontconfig.h>
#include <schematic.h>
#include <sch_sheet.h>
#include <sch_screen.h>
#include <symbol_lib_table.h>
#include <kiplatform/environment.h>

#include <fstream>
#include <wx/txtstrm.h>
#include <wx/wfstream.h>
#include <wx/mstream.h>
#include <wx/zipstrm.h>
#include <wx/fs_zip.h>
#include <wx/log.h>

#include <json_common.h>
#include <string_utils.h>
#include <wildcards_and_files_ext.h>
#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr.h>
#include <io/easyedapro/easyedapro_import_utils.h>
#include <core/map_helpers.h>
#include <project_sch.h>


struct SCH_IO_EASYEDAPRO::PRJ_DATA
{
    std::map<wxString, EASYEDAPRO::SYM_INFO> m_Symbols;
    std::map<wxString, EASYEDAPRO::BLOB>     m_Blobs;
};


SCH_IO_EASYEDAPRO::SCH_IO_EASYEDAPRO() : SCH_IO( wxS( "EasyEDA Pro (JLCEDA) Schematic" ) )
{
    m_reporter = &WXLOG_REPORTER::GetInstance();
}


SCH_IO_EASYEDAPRO::~SCH_IO_EASYEDAPRO()
{
    if( m_projectData )
        delete m_projectData;
}


bool SCH_IO_EASYEDAPRO::CanReadSchematicFile( const wxString& aFileName ) const
{
    if( aFileName.Lower().EndsWith( wxS( ".epro" ) ) )
    {
        return true;
    }
    else if( aFileName.Lower().EndsWith( wxS( ".zip" ) ) )
    {
        std::shared_ptr<wxZipEntry> entry;
        wxFFileInputStream          in( aFileName );
        wxZipInputStream            zip( in );

        if( !zip.IsOk() )
            return false;

        while( entry.reset( zip.GetNextEntry() ), entry.get() != NULL )
        {
            wxString name = entry->GetName();

            if( name == wxS( "project.json" ) )
                return true;
        }

        return false;
    }

    return false;
}


int SCH_IO_EASYEDAPRO::GetModifyHash() const
{
    return 0;
}


static LIB_SYMBOL* loadSymbol( nlohmann::json project, const wxString& aLibraryPath,
                               const wxString& aAliasName, const std::map<std::string, UTF8>* aProperties )
{
    SCH_EASYEDAPRO_PARSER parser( nullptr, nullptr );
    LIB_SYMBOL*           symbol = nullptr;
    wxFileName            libFname( aLibraryPath );
    wxString              symLibName = LIB_ID::FixIllegalChars( libFname.GetName(), true );

    /*if( libFname.GetExt() == wxS( "esym" ) )
    {
        wxFFileInputStream ffis( aLibraryPath );
        wxTextInputStream  txt( ffis, wxS( " " ), wxConvUTF8 );

        bool                        loadThis = false;
        std::vector<nlohmann::json> lines;
        while( ffis.CanRead() )
        {
            nlohmann::json js = nlohmann::json::parse( txt.ReadLine() );
            lines.emplace_back( js );

            if( js.at( 0 ) == "ATTR" && js.at( 3 ) == "Symbol" )
            {
                if( js.at( 4 ).get<wxString>() == aAliasName )
                {
                    loadThis = true;
                }
            }
        }

        if( loadThis )
        {
            EASYEDAPRO::SYM_INFO symInfo = parser.ParseSymbol( lines );
            return symInfo.libSymbol.release();
        }
    }
    else */
    if( libFname.GetExt() == wxS( "elibz" ) || libFname.GetExt() == wxS( "epro" )
        || libFname.GetExt() == wxS( "zip" ) )
    {
        std::map<wxString, EASYEDAPRO::PRJ_SYMBOL>    prjSymbols = project.at( "symbols" );
        std::map<wxString, EASYEDAPRO::PRJ_FOOTPRINT> prjFootprints;
        std::map<wxString, EASYEDAPRO::PRJ_DEVICE>    prjDevices;

        if( project.contains( "footprints" ) )
            prjFootprints = project.at( "footprints" );

        if( project.contains( "devices" ) )
            prjDevices = project.at( "devices" );

        auto prjSymIt = std::find_if( prjSymbols.begin(), prjSymbols.end(),
                                      [&]( const auto& pair )
                                      {
                                          return pair.second.title == aAliasName;
                                      } );

        if( prjSymIt == prjSymbols.end() )
            return nullptr;

        wxString prjSymUuid = prjSymIt->first;

        wxString                     description;
        wxString                     customTags;
        std::map<wxString, wxString> deviceAttributes;
        wxString                     fpTitle;

        for( auto& [key, device] : prjDevices )
        {
            auto val = get_opt( device.attributes, "Symbol" );

            if( val && *val == prjSymUuid )
            {
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
        }

        auto cb = [&]( const wxString& name, const wxString& symUuid, wxInputStream& zip ) -> bool
        {
            if( !name.EndsWith( wxS( ".esym" ) ) )
                EASY_IT_CONTINUE;

            if( symUuid != prjSymUuid )
                EASY_IT_CONTINUE;

            wxTextInputStream txt( zip, wxS( " " ), wxConvUTF8 );

            std::vector<nlohmann::json> lines;
            while( zip.CanRead() )
            {
                nlohmann::json js = nlohmann::json::parse( txt.ReadLine() );
                lines.emplace_back( js );
            }

            EASYEDAPRO::SYM_INFO symInfo = parser.ParseSymbol( lines, deviceAttributes );

            if( !symInfo.libSymbol )
                EASY_IT_CONTINUE;

            LIB_ID libID = EASYEDAPRO::ToKiCadLibID( symLibName, aAliasName );
            symInfo.libSymbol->SetLibId( libID );
            symInfo.libSymbol->SetName( aAliasName );
            symInfo.libSymbol->GetFootprintField().SetText( symLibName + wxS( ":" ) + fpTitle );

            wxString keywords = customTags;
            keywords.Replace( wxS( ":" ), wxS( " " ), true );

            symInfo.libSymbol->SetKeyWords( keywords );

            description.Replace( wxS( "\u2103" ), wxS( "\u00B0C" ), true ); // ℃ -> °C

            symInfo.libSymbol->SetDescription( description );

            symbol = symInfo.libSymbol.release();

            EASY_IT_BREAK;
        };
        EASYEDAPRO::IterateZipFiles( aLibraryPath, cb );
    }

    return symbol;
}


void SCH_IO_EASYEDAPRO::EnumerateSymbolLib( wxArrayString&         aSymbolNameList,
                                            const wxString&        aLibraryPath,
                                            const std::map<std::string, UTF8>* aProperties )
{
    wxFileName fname( aLibraryPath );

    if( fname.GetExt() == wxS( "esym" ) )
    {
        wxFFileInputStream ffis( aLibraryPath );
        wxTextInputStream  txt( ffis, wxS( " " ), wxConvUTF8 );

        while( ffis.CanRead() )
        {
            wxString line = txt.ReadLine();

            if( !line.Contains( wxS( "ATTR" ) ) )
                continue; // Don't bother parsing

            nlohmann::json js = nlohmann::json::parse( line );
            if( js.at( 0 ) == "ATTR" && js.at( 3 ) == "Symbol" )
            {
                aSymbolNameList.Add( js.at( 4 ).get<wxString>() );
            }
        }
    }
    else if( fname.GetExt() == wxS( "elibz" ) || fname.GetExt() == wxS( "epro" )
             || fname.GetExt() == wxS( "zip" ) )
    {
        nlohmann::json project = EASYEDAPRO::ReadProjectOrDeviceFile( aLibraryPath );
        std::map<wxString, nlohmann::json> symbolMap = project.at( "symbols" );

        for( auto& [key, value] : symbolMap )
        {
            wxString title;

            if( value.contains( "display_title" ) )
                title = value.at( "display_title" ).get<wxString>();
            else
                title = value.at( "title" ).get<wxString>();

            aSymbolNameList.Add( title );
        }
    }
}


void SCH_IO_EASYEDAPRO::EnumerateSymbolLib( std::vector<LIB_SYMBOL*>& aSymbolList,
                                            const wxString&           aLibraryPath,
                                            const std::map<std::string, UTF8>*    aProperties )
{
    wxFileName     libFname( aLibraryPath );
    wxArrayString  symbolNameList;
    nlohmann::json project;

    EnumerateSymbolLib( symbolNameList, aLibraryPath, aProperties );

    if( libFname.GetExt() == wxS( "elibz" ) || libFname.GetExt() == wxS( "epro" )
        || libFname.GetExt() == wxS( "zip" ) )
    {
        project = EASYEDAPRO::ReadProjectOrDeviceFile( aLibraryPath );
    }

    for( const wxString& symbolName : symbolNameList )
    {
        LIB_SYMBOL* sym = loadSymbol( project, aLibraryPath, symbolName, aProperties );

        if( sym )
            aSymbolList.push_back( sym );
    }
}


void SCH_IO_EASYEDAPRO::LoadAllDataFromProject( const wxString& aProjectPath )
{
    if( m_projectData )
        delete m_projectData;

    m_projectData = new PRJ_DATA;

    SCH_EASYEDAPRO_PARSER parser( nullptr, nullptr );
    wxFileName            fname( aProjectPath );
    wxString              symLibName = EASYEDAPRO::ShortenLibName( fname.GetName() );

    if( fname.GetExt() != wxS( "epro" ) && fname.GetExt() != wxS( "zip" ) )
        return;

    nlohmann::json project = EASYEDAPRO::ReadProjectOrDeviceFile( aProjectPath );

    std::map<wxString, EASYEDAPRO::PRJ_SYMBOL>    prjSymbols = project.at( "symbols" );
    std::map<wxString, EASYEDAPRO::PRJ_FOOTPRINT> prjFootprints = project.at( "footprints" );
    std::map<wxString, EASYEDAPRO::PRJ_DEVICE>    prjDevices = project.at( "devices" );

    auto cb = [&]( const wxString& name, const wxString& baseName, wxInputStream& zip ) -> bool
    {
        if( !name.EndsWith( wxS( ".esym" ) ) && !name.EndsWith( wxS( ".eblob" ) ) )
            EASY_IT_CONTINUE;

        std::vector<nlohmann::json> lines = EASYEDAPRO::ParseJsonLines( zip, name );

        if( name.EndsWith( wxS( ".esym" ) ) )
        {
            wxString                     description;
            wxString                     customTags;
            std::map<wxString, wxString> deviceAttributes;
            wxString                     fpTitle;

            for( auto& [key, device] : prjDevices )
            {
                auto val = get_opt( device.attributes, "Symbol" );

                if( val && *val == baseName )
                {
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
            }

            EASYEDAPRO::PRJ_SYMBOL symData = prjSymbols.at( baseName );
            EASYEDAPRO::SYM_INFO   symInfo = parser.ParseSymbol( lines, deviceAttributes );

            if( !symInfo.libSymbol )
                EASY_IT_CONTINUE;

            LIB_ID libID = EASYEDAPRO::ToKiCadLibID( symLibName, symData.title );
            symInfo.libSymbol->SetLibId( libID );
            symInfo.libSymbol->SetName( symData.title );
            symInfo.libSymbol->GetFootprintField().SetText( symLibName + wxS( ":" ) + fpTitle );

            wxString keywords = customTags;
            keywords.Replace( wxS( ":" ), wxS( " " ), true );

            symInfo.libSymbol->SetKeyWords( keywords );

            description.Replace( wxS( "\u2103" ), wxS( "\u00B0C" ), true ); // ℃ -> °C

            symInfo.libSymbol->SetDescription( description );

            m_projectData->m_Symbols.emplace( baseName, std::move( symInfo ) );
        }
        else if( name.EndsWith( wxS( ".eblob" ) ) )
        {
            for( const nlohmann::json& line : lines )
            {
                if( line.at( 0 ) == "BLOB" )
                {
                    EASYEDAPRO::BLOB blob = line;
                    m_projectData->m_Blobs[blob.objectId] = blob;
                }
            }
        }

        EASY_IT_CONTINUE;
    };
    EASYEDAPRO::IterateZipFiles( aProjectPath, cb );
}


LIB_SYMBOL* SCH_IO_EASYEDAPRO::LoadSymbol( const wxString& aLibraryPath, const wxString& aAliasName,
                                           const std::map<std::string, UTF8>* aProperties )
{
    wxFileName     libFname( aLibraryPath );
    nlohmann::json project;

    if( libFname.GetExt() == wxS( "elibz" ) || libFname.GetExt() == wxS( "epro" )
        || libFname.GetExt() == wxS( "zip" ) )
    {
        project = EASYEDAPRO::ReadProjectOrDeviceFile( aLibraryPath );
    }

    return loadSymbol( project, aLibraryPath, aAliasName, aProperties );
}


SCH_SHEET* SCH_IO_EASYEDAPRO::LoadSchematicFile( const wxString& aFileName,
                                                 SCHEMATIC* aSchematic, SCH_SHEET* aAppendToMe,
                                                 const std::map<std::string, UTF8>* aProperties )
{
    wxCHECK( !aFileName.IsEmpty() && aSchematic, nullptr );

    // Show the font substitution warnings
    fontconfig::FONTCONFIG::SetReporter( &WXLOG_REPORTER::GetInstance() );

    SCH_SHEET* rootSheet = nullptr;

    if( aAppendToMe )
    {
        wxCHECK_MSG( aSchematic->IsValid(), nullptr,
                     wxS( "Can't append to a schematic with no root!" ) );

        rootSheet = &aSchematic->Root();
    }
    else
    {
        rootSheet = new SCH_SHEET( aSchematic );
        rootSheet->SetFileName( aFileName );
        aSchematic->SetRoot( rootSheet );
    }

    if( !rootSheet->GetScreen() )
    {
        SCH_SCREEN* screen = new SCH_SCREEN( aSchematic );

        screen->SetFileName( aFileName );
        rootSheet->SetScreen( screen );

        // Virtual root sheet UUID must be the same as the schematic file UUID.
        const_cast<KIID&>( rootSheet->m_Uuid ) = screen->GetUuid();
    }

    // TODO(JE) library tables
    // SYMBOL_LIB_TABLE* libTable = PROJECT_SCH::SchSymbolLibTable( &aSchematic->Project() );
    // wxCHECK_MSG( libTable, nullptr, wxS( "Could not load symbol lib table." ) );

    SCH_EASYEDAPRO_PARSER parser( nullptr, nullptr );
    wxFileName            fname( aFileName );
    wxString              libName = EASYEDAPRO::ShortenLibName( fname.GetName() );

    wxFileName libFileName( fname.GetPath(), libName, FILEEXT::KiCadSymbolLibFileExtension );

    if( fname.GetExt() != wxS( "epro" ) && fname.GetExt() != wxS( "zip" ) )
        return rootSheet;

    nlohmann::json project = EASYEDAPRO::ReadProjectOrDeviceFile( aFileName );

    std::map<wxString, EASYEDAPRO::PRJ_SCHEMATIC> prjSchematics = project.at( "schematics" );

    wxString schematicToLoad;

    if( aProperties && aProperties->contains( "sch_id" ) )
    {
        schematicToLoad = wxString::FromUTF8( aProperties->at( "sch_id" ) );
    }
    else
    {
        if( prjSchematics.size() == 1 )
        {
            schematicToLoad = prjSchematics.begin()->first;
        }
        else
        {
            std::vector<IMPORT_PROJECT_DESC> chosen = m_choose_project_handler(
                    EASYEDAPRO::ProjectToSelectorDialog( project, false, true ) );

            if( chosen.size() > 0 )
                schematicToLoad = chosen[0].SchematicId;
        }
    }

    if( schematicToLoad.empty() )
        return nullptr;

    wxString rootBaseName = EscapeString( prjSchematics[schematicToLoad].name, CTX_FILENAME );

    wxFileName rootFname( aFileName );
    rootFname.SetFullName( rootBaseName + wxS( "." )
                           + wxString::FromUTF8( FILEEXT::KiCadSchematicFileExtension ) );

    rootSheet->SetName( prjSchematics[schematicToLoad].name );
    rootSheet->SetFileName( rootFname.GetFullPath() );
    rootSheet->GetScreen()->SetFileName( rootFname.GetFullPath() );

    const std::vector<EASYEDAPRO::PRJ_SHEET>& prjSchematicSheets =
            prjSchematics[schematicToLoad].sheets;

    LoadAllDataFromProject( aFileName );

    if( !m_projectData )
        return nullptr;

    const int schSheetsCount = prjSchematicSheets.size();

    auto cbs = [&]( const wxString& name, const wxString& baseName, wxInputStream& zip ) -> bool
    {
        if( !name.EndsWith( wxS( ".esch" ) ) )
            EASY_IT_CONTINUE;

        wxArrayString nameParts = wxSplit( name, '\\', '\0' );

        if( nameParts.size() == 1 )
            nameParts = wxSplit( name, '/', '\0' );

        if( nameParts.size() < 3 )
            EASY_IT_CONTINUE;

        wxString schematicUuid = nameParts[1];
        wxString sheetFileName = nameParts[2];
        wxString sheetId = sheetFileName.BeforeLast( '.' );
        int      sheetId_i;
        sheetId.ToInt( &sheetId_i );

        if( schematicUuid != schematicToLoad )
            EASY_IT_CONTINUE;

        auto prjSheetIt = std::find_if( prjSchematicSheets.begin(), prjSchematicSheets.end(),
                                        [&]( const EASYEDAPRO::PRJ_SHEET& s )
                                        {
                                            return s.id == sheetId_i;
                                        } );

        if( prjSheetIt == prjSchematicSheets.end() )
            EASY_IT_CONTINUE;

        std::vector<nlohmann::json> lines = EASYEDAPRO::ParseJsonLines( zip, name );

        if( schSheetsCount > 1 )
        {
            wxString sheetBaseName =
                    sheetId + wxS( "_" ) + EscapeString( prjSheetIt->name, CTX_FILENAME );

            wxFileName sheetFname( aFileName );
            sheetFname.SetFullName( sheetBaseName + wxS( "." )
                                    + wxString::FromUTF8( FILEEXT::KiCadSchematicFileExtension ) );

            wxFileName relSheetPath( sheetFname );
            relSheetPath.MakeRelativeTo( rootFname.GetPath() );

            std::unique_ptr<SCH_SHEET> subSheet = std::make_unique<SCH_SHEET>( aSchematic );
            subSheet->SetFileName( relSheetPath.GetFullPath() );
            subSheet->SetName( prjSheetIt->name );

            SCH_SCREEN* screen = new SCH_SCREEN( aSchematic );
            screen->SetFileName( sheetFname.GetFullPath() );
            screen->SetPageNumber( sheetId );
            subSheet->SetScreen( screen );

            VECTOR2I pos;
            pos.x = schIUScale.MilsToIU( 200 );
            pos.y = schIUScale.MilsToIU( 200 )
                    + ( subSheet->GetSize().y + schIUScale.MilsToIU( 200 ) ) * ( sheetId_i - 1 );

            subSheet->SetPosition( pos );

            SCH_SHEET_PATH sheetPath;
            sheetPath.push_back( rootSheet );
            sheetPath.push_back( subSheet.get() );
            sheetPath.SetPageNumber( sheetId );
            aSchematic->SetCurrentSheet( sheetPath );

            parser.ParseSchematic( aSchematic, subSheet.get(), project, m_projectData->m_Symbols,
                                   m_projectData->m_Blobs, lines, libName );

            rootSheet->GetScreen()->Append( subSheet.release() );
        }
        else
        {
            parser.ParseSchematic( aSchematic, rootSheet, project, m_projectData->m_Symbols,
                                   m_projectData->m_Blobs, lines, libName );
        }

        EASY_IT_CONTINUE;
    };
    EASYEDAPRO::IterateZipFiles( aFileName, cbs );

    IO_RELEASER<SCH_IO> sch_plugin( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD ) );

    // TODO(JE) library tables
#if 0
    if( !libTable->HasLibrary( libName ) )
    {
        // Create a new empty symbol library.
        sch_plugin->CreateLibrary( libFileName.GetFullPath() );
        wxString libTableUri = wxS( "${KIPRJMOD}/" ) + libFileName.GetFullName();

        // Add the new library to the project symbol library table.
        libTable->InsertRow( new SYMBOL_LIB_TABLE_ROW( libName, libTableUri, wxS( "KiCad" ) ) );

        // Save project symbol library table.
        wxFileName fn( aSchematic->Project().GetProjectPath(),
                       SYMBOL_LIB_TABLE::GetSymbolLibTableFileName() );

        // So output formatter goes out of scope and closes the file before reloading.
        {
            FILE_OUTPUTFORMATTER formatter( fn.GetFullPath() );
            libTable->Format( &formatter, 0 );
        }

        // Relaod the symbol library table.
        aSchematic->Project().SetElem( PROJECT::ELEM::SYMBOL_LIB_TABLE, NULL );
        PROJECT_SCH::SchSymbolLibTable( &aSchematic->Project() );
    }
#endif

    // set properties to prevent save file on every symbol save
    std::map<std::string, UTF8> properties;
    properties.emplace( SCH_IO_KICAD_SEXPR::PropBuffering, wxEmptyString );

    for( auto& [symbolUuid, symInfo] : m_projectData->m_Symbols )
        sch_plugin->SaveSymbol( libFileName.GetFullPath(), symInfo.libSymbol.release(),
                                &properties );

    sch_plugin->SaveLibrary( libFileName.GetFullPath() );

    aSchematic->CurrentSheet().UpdateAllScreenReferences();
    aSchematic->FixupJunctionsAfterImport();

    return rootSheet;
}
