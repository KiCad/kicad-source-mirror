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

#include "sch_easyeda_parser.h"
#include "sch_io_easyeda.h"

#include <font/fontconfig.h>
#include <kiplatform/environment.h>
#include <project_sch.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <schematic.h>
#include <string_utils.h>
#include <wildcards_and_files_ext.h>
#include <wx/log.h>
#include <wx/stdstream.h>
#include <wx/zipstrm.h>

#include <json_common.h>
#include <core/map_helpers.h>
#include <wx/wfstream.h>


static bool FindSchFileInStream( const wxString& aName, wxInputStream& aStream,
                                 nlohmann::json& aOut, EASYEDA::DOCUMENT& aDoc,
                                 EASYEDA::DOC_TYPE& aDocType )
{
    if( aName.Lower().EndsWith( wxS( ".json" ) ) )
    {
        wxStdInputStream sin( aStream );
        nlohmann::json   js = nlohmann::json::parse( sin, nullptr, false );

        if( js.is_discarded() )
            return false;

        EASYEDA::DOCUMENT doc = js.get<EASYEDA::DOCUMENT>();
        EASYEDA::DOC_TYPE type;

        if( doc.docType )
            type = *doc.docType;
        else
            type = doc.head.docType;

        if( type == EASYEDA::DOC_TYPE::SCHEMATIC_SHEET || type == EASYEDA::DOC_TYPE::SCHEMATIC_LIST
            || type == EASYEDA::DOC_TYPE::SYMBOL )
        {
            aOut = js;
            aDoc = doc;
            aDocType = type;
            return true;
        }
    }
    else if( aName.Lower().EndsWith( wxS( ".zip" ) ) )
    {
        std::shared_ptr<wxZipEntry> entry;
        wxZipInputStream            zip( aStream );

        if( !zip.IsOk() )
            return false;

        while( entry.reset( zip.GetNextEntry() ), entry.get() != NULL )
        {
            wxString name = entry->GetName();

            if( FindSchFileInStream( name, zip, aOut, aDoc, aDocType ) )
                return true;
        }
    }

    return false;
}


bool SCH_IO_EASYEDA::CanReadSchematicFile( const wxString& aFileName ) const
{
    if( !SCH_IO::CanReadSchematicFile( aFileName ) )
        return false;

    try
    {
        wxFFileInputStream in( aFileName );
        nlohmann::json     js;
        EASYEDA::DOCUMENT  doc;
        EASYEDA::DOC_TYPE  docType;

        return FindSchFileInStream( aFileName, in, js, doc, docType );
    }
    catch( nlohmann::json::exception& )
    {
    }
    catch( std::exception& )
    {
    }

    return false;
}


bool SCH_IO_EASYEDA::CanReadLibrary( const wxString& aFileName ) const
{
    return CanReadSchematicFile( aFileName );
}


int SCH_IO_EASYEDA::GetModifyHash() const
{
    return 0;
}


LIB_SYMBOL* loadSymbol( const wxString& aLibraryPath, nlohmann::json aFileData,
                        const wxString& aAliasName, const std::map<std::string, UTF8>* aProperties )
{
    SCH_EASYEDA_PARSER      parser( nullptr, nullptr );
    std::map<wxString, int> namesCounter;

    try
    {
        wxFFileInputStream in( aLibraryPath );
        nlohmann::json     js;
        EASYEDA::DOCUMENT  topDoc;
        EASYEDA::DOC_TYPE  topDocType;

        if( !FindSchFileInStream( aLibraryPath, in, js, topDoc, topDocType ) )
        {
            THROW_IO_ERROR( wxString::Format( _( "Unable to find a valid schematic file in '%s'" ),
                                              aLibraryPath ) );
        }

        if( topDocType == EASYEDA::DOC_TYPE::SCHEMATIC_SHEET
            || topDocType == EASYEDA::DOC_TYPE::SCHEMATIC_LIST )
        {
            EASYEDA::DOCUMENT_SCHEMATICS schDoc = js.get<EASYEDA::DOCUMENT_SCHEMATICS>();

            for( const EASYEDA::DOCUMENT& subDoc : *schDoc.schematics )
            {
                if( subDoc.docType )
                {
                    if( *subDoc.docType != EASYEDA::DOC_TYPE::SCHEMATIC_SHEET )
                        continue;
                }
                else
                {
                    if( subDoc.head.docType != EASYEDA::DOC_TYPE::SCHEMATIC_SHEET )
                        continue;
                }

                EASYEDA::DOCUMENT dataStrDoc = subDoc.dataStr->get<EASYEDA::DOCUMENT>();

                for( wxString shap : dataStrDoc.shape )
                {
                    if( !shap.Contains( wxS( "LIB" ) ) )
                        continue;

                    shap.Replace( wxS( "#@$" ), wxS( "\n" ) );
                    wxArrayString parts = wxSplit( shap, '\n', '\0' );

                    if( parts.size() < 1 )
                        continue;

                    wxArrayString paramsRoot = wxSplit( parts[0], '~', '\0' );

                    if( paramsRoot.size() < 1 )
                        continue;

                    wxString rootType = paramsRoot[0];

                    if( rootType == wxS( "LIB" ) )
                    {
                        if( paramsRoot.size() < 4 )
                            continue;

                        VECTOR2D origin( parser.Convert( paramsRoot[1] ),
                                         parser.Convert( paramsRoot[2] ) );

                        wxString symbolName = wxString::Format( wxS( "Unknown_%s_%s" ),
                                                                paramsRoot[1], paramsRoot[2] );

                        wxArrayString paramParts = wxSplit( paramsRoot[3], '`', '\0' );

                        std::map<wxString, wxString> paramMap;

                        for( size_t i = 1; i < paramParts.size(); i += 2 )
                        {
                            wxString key = paramParts[i - 1];
                            wxString value = paramParts[i];

                            if( key == wxS( "spiceSymbolName" ) && !value.IsEmpty() )
                                symbolName = value;

                            paramMap[key] = value;
                        }

                        int& serial = namesCounter[symbolName];

                        if( serial > 0 )
                            symbolName << wxS( "_" ) << serial;

                        serial++;

                        paramMap[wxS( "spiceSymbolName" )] = symbolName;

                        if( symbolName == aAliasName )
                        {
                            parts.RemoveAt( 0 );

                            LIB_SYMBOL* ksymbol = parser.ParseSymbol( origin, paramMap, parts );

                            // Clear reference numbers
                            SCH_FIELD& refField = ksymbol->GetReferenceField();
                            wxString   origRef = refField.GetText();
                            wxString   reference;

                            for( size_t i = 0; i < origRef.size() && !wxIsdigit( origRef[i] ); i++ )
                                reference << origRef[i];

                            refField.SetText( reference );

                            return ksymbol;
                        }
                    }
                }
            }
        }
        else if( topDocType == EASYEDA::DOC_TYPE::SYMBOL )
        {
            EASYEDA::DOCUMENT_SYM symDoc = js.get<EASYEDA::DOCUMENT_SYM>();

            wxString symbolName = wxS( "Unknown" );

            std::optional<std::map<wxString, wxString>> c_para;

            if( symDoc.c_para )
                c_para = symDoc.c_para;
            else if( topDoc.head.c_para )
                c_para = topDoc.head.c_para;

            if( !c_para )
                return nullptr;

            symbolName = get_def( *c_para, wxS( "name" ), symbolName );

            int& serial = namesCounter[symbolName];

            if( serial > 0 )
                symbolName << wxS( "_" ) << serial;

            serial++;

            if( symbolName != aAliasName )
                return nullptr;

            VECTOR2D origin( topDoc.head.x, topDoc.head.y );

            LIB_SYMBOL* ksymbol = parser.ParseSymbol( origin, *c_para, topDoc.shape );

            // Clear reference numbers
            SCH_FIELD& refField = ksymbol->GetReferenceField();
            wxString   origRef = refField.GetText();
            wxString   reference;

            for( size_t i = 0; i < origRef.size() && !wxIsdigit( origRef[i] ); i++ )
                reference << origRef[i];

            refField.SetText( reference );

            return ksymbol;
        }
    }
    catch( nlohmann::json::exception& e )
    {
        THROW_IO_ERROR( wxString::Format( _( "Error loading symbol '%s' from library '%s': %s" ),
                                          aAliasName, aLibraryPath, e.what() ) );
    }
    catch( std::exception& e )
    {
        THROW_IO_ERROR( wxString::Format( _( "Error loading symbol '%s' from library '%s': %s" ),
                                          aAliasName, aLibraryPath, e.what() ) );
    }

    return nullptr;
}


void SCH_IO_EASYEDA::EnumerateSymbolLib( wxArrayString&         aSymbolNameList,
                                         const wxString&        aLibraryPath,
                                         const std::map<std::string, UTF8>* aProperties )
{
    std::map<wxString, int> namesCounter;

    // Suppress font substitution warnings
    fontconfig::FONTCONFIG::SetReporter( nullptr );

    try
    {
        wxFFileInputStream in( aLibraryPath );
        nlohmann::json     js;
        EASYEDA::DOCUMENT  topDoc;
        EASYEDA::DOC_TYPE  topDocType;

        if( !FindSchFileInStream( aLibraryPath, in, js, topDoc, topDocType ) )
        {
            THROW_IO_ERROR( wxString::Format( _( "Unable to find a valid schematic file in '%s'" ),
                                              aLibraryPath ) );
        }

        if( topDocType == EASYEDA::DOC_TYPE::SCHEMATIC_SHEET
            || topDocType == EASYEDA::DOC_TYPE::SCHEMATIC_LIST )
        {
            EASYEDA::DOCUMENT_SCHEMATICS schDoc = js.get<EASYEDA::DOCUMENT_SCHEMATICS>();

            for( const EASYEDA::DOCUMENT& subDoc : *schDoc.schematics )
            {
                if( subDoc.docType )
                {
                    if( *subDoc.docType != EASYEDA::DOC_TYPE::SCHEMATIC_SHEET )
                        continue;
                }
                else
                {
                    if( subDoc.head.docType != EASYEDA::DOC_TYPE::SCHEMATIC_SHEET )
                        continue;
                }

                EASYEDA::DOCUMENT dataStrDoc = subDoc.dataStr->get<EASYEDA::DOCUMENT>();

                for( wxString shap : dataStrDoc.shape )
                {
                    if( !shap.Contains( wxS( "LIB" ) ) )
                        continue;

                    shap.Replace( wxS( "#@$" ), wxS( "\n" ) );
                    wxArrayString parts = wxSplit( shap, '\n', '\0' );

                    if( parts.size() < 1 )
                        continue;

                    wxArrayString paramsRoot = wxSplit( parts[0], '~', '\0' );

                    if( paramsRoot.size() < 1 )
                        continue;

                    wxString rootType = paramsRoot[0];

                    if( rootType == wxS( "LIB" ) )
                    {
                        if( paramsRoot.size() < 4 )
                            continue;

                        wxString symbolName = wxString::Format( wxS( "Unknown_%s_%s" ),
                                                                paramsRoot[1], paramsRoot[2] );

                        wxArrayString paramParts = wxSplit( paramsRoot[3], '`', '\0' );

                        std::map<wxString, wxString> paramMap;

                        for( size_t i = 1; i < paramParts.size(); i += 2 )
                        {
                            wxString key = paramParts[i - 1];
                            wxString value = paramParts[i];

                            if( key == wxS( "spiceSymbolName" ) && !value.IsEmpty() )
                                symbolName = value;

                            paramMap[key] = value;
                        }

                        int& serial = namesCounter[symbolName];

                        if( serial > 0 )
                            symbolName << wxS( "_" ) << serial;

                        serial++;

                        aSymbolNameList.Add( symbolName );
                    }
                }
            }
        }
        else if( topDocType == EASYEDA::DOC_TYPE::SYMBOL )
        {
            EASYEDA::DOCUMENT_SYM symDoc = js.get<EASYEDA::DOCUMENT_SYM>();

            wxString packageName = wxS( "Unknown" );

            if( symDoc.c_para )
            {
                packageName = get_def( *symDoc.c_para, wxS( "name" ), packageName );
            }
            else if( topDoc.head.c_para )
            {
                packageName = get_def( *topDoc.head.c_para, wxS( "name" ), packageName );
            }

            aSymbolNameList.Add( packageName );
        }
    }
    catch( nlohmann::json::exception& e )
    {
        THROW_IO_ERROR( wxString::Format( _( "Error enumerating symbol library '%s': %s" ),
                                          aLibraryPath, e.what() ) );
    }
    catch( std::exception& e )
    {
        THROW_IO_ERROR( wxString::Format( _( "Error enumerating symbol library '%s': %s" ),
                                          aLibraryPath, e.what() ) );
    }
}


void SCH_IO_EASYEDA::EnumerateSymbolLib( std::vector<LIB_SYMBOL*>& aSymbolList,
                                         const wxString&           aLibraryPath,
                                         const std::map<std::string, UTF8>*    aProperties )
{
    wxFFileInputStream in( aLibraryPath );
    nlohmann::json     js;
    EASYEDA::DOCUMENT  topDoc;
    EASYEDA::DOC_TYPE  topDocType;

    if( !FindSchFileInStream( aLibraryPath, in, js, topDoc, topDocType ) )
    {
        THROW_IO_ERROR( wxString::Format( _( "Unable to find a valid schematic file in '%s'" ),
                                          aLibraryPath ) );
    }

    try
    {
        wxArrayString symbolNameList;

        EnumerateSymbolLib( symbolNameList, aLibraryPath, aProperties );

        for( const wxString& symbolName : symbolNameList )
        {
            LIB_SYMBOL* sym = loadSymbol( aLibraryPath, js, symbolName, aProperties );

            if( sym )
                aSymbolList.push_back( sym );
        }
    }
    catch( nlohmann::json::exception& e )
    {
        THROW_IO_ERROR( wxString::Format( _( "Error enumerating symbol library '%s': %s" ),
                                          aLibraryPath, e.what() ) );
    }
    catch( std::exception& e )
    {
        THROW_IO_ERROR( wxString::Format( _( "Error enumerating symbol library '%s': %s" ),
                                          aLibraryPath, e.what() ) );
    }
}


LIB_SYMBOL* SCH_IO_EASYEDA::LoadSymbol( const wxString&        aLibraryPath,
                                        const wxString&        aAliasName,
                                        const std::map<std::string, UTF8>* aProperties )
{
    try
    {
        wxFFileInputStream in( aLibraryPath );
        nlohmann::json     js;
        EASYEDA::DOCUMENT  topDoc;
        EASYEDA::DOC_TYPE  topDocType;

        if( !FindSchFileInStream( aLibraryPath, in, js, topDoc, topDocType ) )
        {
            THROW_IO_ERROR( wxString::Format( _( "Unable to find a valid schematic file in '%s'" ),
                                              aLibraryPath ) );
        }

        return loadSymbol( aLibraryPath, js, aAliasName, aProperties );
    }
    catch( nlohmann::json::exception& e )
    {
        THROW_IO_ERROR( wxString::Format( _( "Error loading symbol '%s' from library '%s': %s" ),
                                          aAliasName, aLibraryPath, e.what() ) );
    }
    catch( std::exception& e )
    {
        THROW_IO_ERROR( wxString::Format( _( "Error loading symbol '%s' from library '%s': %s" ),
                                          aAliasName, aLibraryPath, e.what() ) );
    }

    return nullptr;
}


static void LoadSchematic( SCHEMATIC* aSchematic, SCH_SHEET* aRootSheet, const wxString& aFileName )
{
    SCH_EASYEDA_PARSER parser( nullptr, nullptr );

    try
    {
        wxFFileInputStream in( aFileName );
        nlohmann::json     js;
        EASYEDA::DOCUMENT  topDoc;
        EASYEDA::DOC_TYPE  topDocType;

        if( !FindSchFileInStream( aFileName, in, js, topDoc, topDocType ) )
        {
            THROW_IO_ERROR( wxString::Format( _( "Unable to find a valid schematic file in '%s'" ),
                                              aFileName ) );
        }

        if( topDocType == EASYEDA::DOC_TYPE::SCHEMATIC_SHEET
            || topDocType == EASYEDA::DOC_TYPE::SCHEMATIC_LIST )
        {
            int                          pageNum = 1;
            EASYEDA::DOCUMENT_SCHEMATICS schDoc = js.get<EASYEDA::DOCUMENT_SCHEMATICS>();

            for( const EASYEDA::DOCUMENT& subDoc : *schDoc.schematics )
            {
                if( subDoc.docType )
                {
                    if( *subDoc.docType != EASYEDA::DOC_TYPE::SCHEMATIC_SHEET )
                        continue;
                }
                else
                {
                    if( subDoc.head.docType != EASYEDA::DOC_TYPE::SCHEMATIC_SHEET )
                        continue;
                }

                EASYEDA::DOCUMENT dataStrDoc = subDoc.dataStr->get<EASYEDA::DOCUMENT>();

                if( schDoc.schematics->size() > 1 )
                {
                    wxString sheetTitle =
                            !subDoc.title.empty() ? subDoc.title : ( wxString() << pageNum );

                    wxString sheetBaseName = EscapeString( sheetTitle, CTX_FILENAME );

                    wxFileName sheetFname( aFileName );
                    sheetFname.SetFullName(
                            sheetBaseName + wxS( "." )
                            + wxString::FromUTF8( FILEEXT::KiCadSchematicFileExtension ) );

                    wxFileName relSheetPath( sheetFname );
                    relSheetPath.MakeRelativeTo(
                            wxFileName( aRootSheet->GetFileName() ).GetPath() );

                    std::unique_ptr<SCH_SHEET> subSheet = std::make_unique<SCH_SHEET>( aSchematic );
                    subSheet->SetFileName( relSheetPath.GetFullPath() );
                    subSheet->SetName( sheetTitle );

                    SCH_SCREEN* screen = new SCH_SCREEN( aSchematic );
                    screen->SetFileName( sheetFname.GetFullPath() );
                    screen->SetPageNumber( wxString() << pageNum );
                    subSheet->SetScreen( screen );

                    VECTOR2I pos;
                    pos.x = schIUScale.MilsToIU( 200 );
                    pos.y = schIUScale.MilsToIU( 200 )
                            + ( subSheet->GetSize().y + schIUScale.MilsToIU( 200 ) )
                                      * ( pageNum - 1 );

                    subSheet->SetPosition( pos );

                    SCH_SHEET_PATH sheetPath;
                    sheetPath.push_back( aRootSheet );
                    sheetPath.push_back( subSheet.get() );
                    sheetPath.SetPageNumber( wxString() << pageNum );
                    aSchematic->SetCurrentSheet( sheetPath );

                    parser.ParseSchematic( aSchematic, subSheet.get(), aFileName,
                                           dataStrDoc.shape );

                    aRootSheet->GetScreen()->Append( subSheet.release() );
                }
                else
                {
                    parser.ParseSchematic( aSchematic, aRootSheet, aFileName, dataStrDoc.shape );
                }

                pageNum++;
            }
        }
    }
    catch( nlohmann::json::exception& e )
    {
        THROW_IO_ERROR(
                wxString::Format( _( "Error loading schematic '%s': %s" ), aFileName, e.what() ) );
    }
    catch( std::exception& e )
    {
        THROW_IO_ERROR(
                wxString::Format( _( "Error loading schematic '%s': %s" ), aFileName, e.what() ) );
    }
}


SCH_SHEET* SCH_IO_EASYEDA::LoadSchematicFile( const wxString& aFileName, SCHEMATIC* aSchematic,
                                              SCH_SHEET*             aAppendToMe,
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
    //
    // wxCHECK_MSG( libTable, nullptr, wxS( "Could not load symbol lib table." ) );
    LoadSchematic( aSchematic, rootSheet, aFileName );
    aSchematic->CurrentSheet().UpdateAllScreenReferences();

    return rootSheet;
}
