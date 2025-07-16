/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Roberto Fernandez Bautista <roberto.fer.bau@gmail.com>
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

#include <sch_io/cadstar/cadstar_sch_archive_loader.h>
#include <sch_io/cadstar/sch_io_cadstar_archive.h>
#include <io/cadstar/cadstar_parts_lib_parser.h>

#include <font/fontconfig.h>
#include <lib_symbol.h>
#include <progress_reporter.h>
#include <project_sch.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <schematic.h>
#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr.h>
#include <symbol_lib_table.h>
#include <wildcards_and_files_ext.h>
#include <wx_filename.h>
#include <libraries/library_table.h>
#include <libraries/symbol_library_adapter.h>
#include <wx/dir.h>
#include <wx/wfstream.h>
#include <wx/txtstrm.h>


bool SCH_IO_CADSTAR_ARCHIVE::CanReadLibrary( const wxString& aFileName ) const
{
    if( !SCH_IO::CanReadLibrary( aFileName ) )
        return false;

    try
    {
        CADSTAR_PARTS_LIB_PARSER p;
        return p.CheckFileHeader( aFileName.utf8_string() );
    }
    catch( const std::runtime_error& )
    {
    }

    return false;
}


int SCH_IO_CADSTAR_ARCHIVE::GetModifyHash() const
{
    return 0;
}


SCH_SHEET* SCH_IO_CADSTAR_ARCHIVE::LoadSchematicFile( const wxString&        aFileName,
                                                      SCHEMATIC*             aSchematic,
                                                      SCH_SHEET*             aAppendToMe,
                                                      const std::map<std::string, UTF8>* aProperties )
{
    wxCHECK( !aFileName.IsEmpty() && aSchematic, nullptr );

    // Show the font substitution warnings
    fontconfig::FONTCONFIG::SetReporter( &WXLOG_REPORTER::GetInstance() );

    SCH_SHEET* rootSheet = nullptr;


    if( aAppendToMe )
    {
        wxCHECK_MSG( aSchematic->IsValid(), nullptr, "Can't append to a schematic with no root!" );
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

    CADSTAR_SCH_ARCHIVE_LOADER csaLoader( aFileName, m_reporter, m_progressReporter );
    csaLoader.Load( aSchematic, rootSheet );

    // Save symbols to project library
    SYMBOL_LIBRARY_ADAPTER* adapter = PROJECT_SCH::SymbolLibAdapter( &aSchematic->Project() );
    LIBRARY_TABLE* table = adapter->ProjectTable().value_or( nullptr );
    wxCHECK_MSG( table, nullptr, "Could not load symbol lib table." );

    wxFileName prj_fn = aSchematic->Project().GetProjectFullName();
    wxString libName = CADSTAR_SCH_ARCHIVE_LOADER::CreateLibName( prj_fn, nullptr );

    wxFileName libFileName( aSchematic->Project().GetProjectPath(), libName,
                            FILEEXT::KiCadSymbolLibFileExtension );

    IO_RELEASER<SCH_IO> sch_plugin( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD ) );

    if( !table->HasRow( libName ) )
    {
        // Create a new empty symbol library.
        sch_plugin->CreateLibrary( libFileName.GetFullPath() );
        wxString libTableUri = "${KIPRJMOD}/" + libFileName.GetFullName();

        // Add the new library to the project symbol library table.
        LIBRARY_TABLE_ROW& row = table->InsertRow();
        row.SetNickname( libName );
        row.SetURI( libTableUri );
        row.SetType( "KiCad" );

        adapter->Manager().Save( table );

        adapter->LoadOne( libName );
    }

    // set properties to prevent save file on every symbol save
    std::map<std::string, UTF8> properties;
    properties.emplace( SCH_IO_KICAD_SEXPR::PropBuffering, "" );

    for( LIB_SYMBOL* const& symbol : csaLoader.GetLoadedSymbols() )
        sch_plugin->SaveSymbol( libFileName.GetFullPath(), symbol, &properties );

    sch_plugin->SaveLibrary( libFileName.GetFullPath() );

    // Link up all symbols in the design to the newly created library
    for( SCH_SHEET_PATH& sheet : aSchematic->Hierarchy() )
    {
        for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* sym = static_cast<SCH_SYMBOL*>( item );

            if( sym->GetLibId().IsLegacy() )
            {
                LIB_ID libid = sym->GetLibId();
                libid.SetLibNickname( libName );
                sym->SetLibId( libid );
            }
        };
    }

    // Need to fix up junctions after import to retain connectivity
    aSchematic->FixupJunctionsAfterImport();

    return rootSheet;
}


void SCH_IO_CADSTAR_ARCHIVE::EnumerateSymbolLib( wxArrayString&         aSymbolNameList,
                                                 const wxString&        aLibraryPath,
                                                 const std::map<std::string, UTF8>* aProperties )
{
    ensureLoadedLibrary( aLibraryPath, aProperties );

    for( auto& [libnameStr, libSymbol] : m_libCache )
        aSymbolNameList.Add( libSymbol->GetName() );
}


void SCH_IO_CADSTAR_ARCHIVE::EnumerateSymbolLib( std::vector<LIB_SYMBOL*>& aSymbolList,
                                                 const wxString&           aLibraryPath,
                                                 const std::map<std::string, UTF8>* aProperties )
{
    ensureLoadedLibrary( aLibraryPath, aProperties );

    for( auto& [libnameStr, libSymbol] : m_libCache )
        aSymbolList.push_back( libSymbol.get() );
}


LIB_SYMBOL* SCH_IO_CADSTAR_ARCHIVE::LoadSymbol( const wxString&        aLibraryPath,
                                                const wxString&        aAliasName,
                                                const std::map<std::string, UTF8>* aProperties )
{
    ensureLoadedLibrary( aLibraryPath, aProperties );

    if( m_libCache.count( aAliasName ) )
        return m_libCache.at( aAliasName ).get();

    return nullptr;
}


void SCH_IO_CADSTAR_ARCHIVE::GetAvailableSymbolFields( std::vector<wxString>& aNames )
{
    std::set<wxString> fieldNames;

    for( auto& [libnameStr, libSymbol] : m_libCache )
    {
        std::vector<SCH_FIELD*> fields;
        libSymbol->GetFields( fields );

        for( SCH_FIELD* field : fields )
        {
            if( field->IsMandatory() )
                continue;

            fieldNames.insert( field->GetName() );
        }
    }

    std::copy( fieldNames.begin(), fieldNames.end(), std::back_inserter( aNames ) );
}


void SCH_IO_CADSTAR_ARCHIVE::GetLibraryOptions( std::map<std::string, UTF8>* aListToAppendTo ) const
{
    ( *aListToAppendTo )["csa"] =
            UTF8( _( "Path to the CADSTAR schematic archive (*.csa) file related to this CADSTAR "
                     "parts library. If none specified it is assumed to be 'symbol.csa' in the "
                     "same folder." ) );

    ( *aListToAppendTo )["fplib"] =
            UTF8( _( "Name of the footprint library related to the symbols in this library. You "
                     "should create a separate entry for the CADSTAR PCB Archive (*.cpa) file in "
                     "the footprint library tables. If none specified, 'cadstarpcblib' is assumed." ) );
}


void SCH_IO_CADSTAR_ARCHIVE::ensureLoadedLibrary( const wxString& aLibraryPath,
                                                  const std::map<std::string, UTF8>* aProperties )
{
    wxFileName csafn;
    wxString   fplibname = "cadstarpcblib";

    // Suppress font substitution warnings
    fontconfig::FONTCONFIG::SetReporter( nullptr );

    if( aProperties && aProperties->contains( "csa" ) )
    {
        csafn = wxFileName( aProperties->at( "csa" ) );

        if( !csafn.IsAbsolute() )
        {
            wxFileName libDir( aLibraryPath );
            libDir.ClearExt();
            libDir.SetName( "" );
            // wxPATH_NORM_ALL is deprecated in wxWidgets 3.1, so use our flags
            csafn.Normalize( FN_NORMALIZE_FLAGS, libDir.GetAbsolutePath() );
        }
    }
    else
    {
        // If none specified, look for the
        // .csa file in same folder as the .lib
        csafn = wxFileName( aLibraryPath );
        csafn.SetExt( "csa" );

        if( !csafn.FileExists() )
        {
            csafn.SetName( "symbol" );

            if( !csafn.FileExists() )
            {
                csafn = wxDir::FindFirst( csafn.GetPath(), wxS( "*.csa" ),
                                          wxDIR_FILES | wxDIR_HIDDEN );

                if( !csafn.FileExists() )
                {
                    THROW_IO_ERROR( wxString::Format(
                            _( "Cannot find the .csa file corresponding to library '%s'." ),
                            aLibraryPath ) );
                }
            }
        }
    }

    if( aProperties && aProperties->contains( "fplib" ) )
    {
        fplibname = wxString::FromUTF8( aProperties->at( "fplib" ) );
    }

    // Get timestamp
    long long  timestamp = 0;
    wxFileName fn( aLibraryPath );

    if( fn.IsFileReadable() && fn.GetModificationTime().IsValid() )
        timestamp = fn.GetModificationTime().GetValue().GetValue();

    if( fn.IsFileReadable()
        && m_cachePath == aLibraryPath
        && m_cachecsafn.GetFullPath() == csafn.GetFullPath()
        && m_cachefplibname == fplibname
        && m_cacheTimestamp == timestamp )
    {
        return;
    }

    // Update cache
    m_libCache.clear();

    CADSTAR_SCH_ARCHIVE_LOADER csaLoader( csafn.GetFullPath(), m_reporter, m_progressReporter );
    csaLoader.SetFpLibName( fplibname );

    std::vector<LIB_SYMBOL*> symbols = csaLoader.LoadPartsLib( aLibraryPath );

    for( LIB_SYMBOL* sym : symbols )
    {
        m_libCache.insert( { sym->GetName(), std::unique_ptr<LIB_SYMBOL>( sym ) } );
    }

    m_cachePath = aLibraryPath;
    m_cachecsafn = csafn;
    m_cachefplibname = fplibname;
    m_cacheTimestamp = timestamp;
}

