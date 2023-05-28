/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Roberto Fernandez Bautista <roberto.fer.bau@gmail.com>
 * Copyright (C) 2020-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file cadstar_pcb_archive_plugin.cpp
 * @brief Pcbnew PLUGIN for CADSTAR PCB Archive (*.cpa) format: an ASCII format
 *        based on S-expressions.
 */

#include <sch_plugins/cadstar/cadstar_sch_archive_loader.h>
#include <sch_plugins/cadstar/cadstar_sch_archive_plugin.h>

#include <lib_symbol.h>
#include <progress_reporter.h>
#include <string_utf8_map.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <schematic.h>
#include <sch_plugins/kicad/sch_sexpr_plugin.h>
#include <wildcards_and_files_ext.h>


const wxString CADSTAR_SCH_ARCHIVE_PLUGIN::GetName() const
{
    return wxT( "CADSTAR Schematic Archive" );
}


const wxString CADSTAR_SCH_ARCHIVE_PLUGIN::GetFileExtension() const
{
    return CadstarSchematicFileExtension;
}


const wxString CADSTAR_SCH_ARCHIVE_PLUGIN::GetLibraryFileExtension() const
{
    return CadstarPartsLibraryFileExtension;
}


int CADSTAR_SCH_ARCHIVE_PLUGIN::GetModifyHash() const
{
    return 0;
}


SCH_SHEET* CADSTAR_SCH_ARCHIVE_PLUGIN::Load( const wxString& aFileName, SCHEMATIC* aSchematic,
        SCH_SHEET* aAppendToMe, const STRING_UTF8_MAP* aProperties )
{
    wxCHECK( !aFileName.IsEmpty() && aSchematic, nullptr );

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
    }

    CADSTAR_SCH_ARCHIVE_LOADER csaLoader( aFileName, m_reporter, m_progressReporter );
    csaLoader.Load( aSchematic, rootSheet );

    // SAVE SYMBOLS TO PROJECT LIBRARY:
    SYMBOL_LIB_TABLE* libTable = aSchematic->Prj().SchSymbolLibTable();

    wxCHECK_MSG( libTable, nullptr, "Could not load symbol lib table." );

    // Lets come up with a nice library name
    wxString libName = aSchematic->Prj().GetProjectName();

    if( libName.IsEmpty() )
    {
        wxFileName fn( rootSheet->GetFileName() );
        libName = fn.GetName();
    }

    if( libName.IsEmpty() )
        libName = "noname";

    libName = LIB_ID::FixIllegalChars( libName, true );

    wxFileName libFileName( aSchematic->Prj().GetProjectPath(), libName,
                            KiCadSymbolLibFileExtension );

    SCH_PLUGIN::SCH_PLUGIN_RELEASER sch_plugin;
    sch_plugin.set( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD ) );

    if( !libTable->HasLibrary( libName ) )
    {
        // Create a new empty symbol library.
        sch_plugin->CreateSymbolLib( libFileName.GetFullPath() );
        wxString libTableUri = "${KIPRJMOD}/" + libFileName.GetFullName();

        // Add the new library to the project symbol library table.
        libTable->InsertRow(
                new SYMBOL_LIB_TABLE_ROW( libName, libTableUri, wxString( "KiCad" ) ) );

        // Save project symbol library table.
        wxFileName fn( aSchematic->Prj().GetProjectPath(),
                       SYMBOL_LIB_TABLE::GetSymbolLibTableFileName() );

        // So output formatter goes out of scope and closes the file before reloading.
        {
            FILE_OUTPUTFORMATTER formatter( fn.GetFullPath() );
            libTable->Format( &formatter, 0 );
        }

        // Relaod the symbol library table.
        aSchematic->Prj().SetElem( PROJECT::ELEM_SYMBOL_LIB_TABLE, NULL );
        aSchematic->Prj().SchSymbolLibTable();
    }

    // set properties to prevent save file on every symbol save
    STRING_UTF8_MAP properties;
    properties.emplace( SCH_SEXPR_PLUGIN::PropBuffering, "" );

    for( LIB_SYMBOL* const& symbol : csaLoader.GetLoadedSymbols() )
        sch_plugin->SaveSymbol( libFileName.GetFullPath(), symbol, &properties );

    sch_plugin->SaveLibrary( libFileName.GetFullPath() );

    // Link up all symbols in the design to the newly created library
    for( SCH_SHEET_PATH& sheet : aSchematic->GetSheets() )
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
    aSchematic->FixupJunctions();

    return rootSheet;
}


bool CADSTAR_SCH_ARCHIVE_PLUGIN::CheckHeader( const wxString& aFileName )
{
    // TODO: write a parser for the cpa header. For now assume it is valid
    // and throw exceptions when parsing
    return true;
}


void CADSTAR_SCH_ARCHIVE_PLUGIN::EnumerateSymbolLib( wxArrayString&         aSymbolNameList,
                                                     const wxString&        aLibraryPath,
                                                     const STRING_UTF8_MAP* aProperties )
{
    ensureLoadedLibrary( aLibraryPath, aProperties );

    for( auto& [libnameStr, libSymbol] : m_libCache )
        aSymbolNameList.Add( libSymbol->GetName() );
}


void CADSTAR_SCH_ARCHIVE_PLUGIN::EnumerateSymbolLib( std::vector<LIB_SYMBOL*>& aSymbolList,
                                                     const wxString&           aLibraryPath,
                                                     const STRING_UTF8_MAP* aProperties )
{
    ensureLoadedLibrary( aLibraryPath, aProperties );

    for( auto& [libnameStr, libSymbol] : m_libCache )
        aSymbolList.push_back( libSymbol.get() );
}


LIB_SYMBOL* CADSTAR_SCH_ARCHIVE_PLUGIN::LoadSymbol( const wxString&        aLibraryPath,
                                                    const wxString&        aAliasName,
                                                    const STRING_UTF8_MAP* aProperties )
{
    ensureLoadedLibrary( aLibraryPath, aProperties );

    if( m_libCache.count( aAliasName ) )
        return m_libCache.at( aAliasName ).get();

    return nullptr;
}


void CADSTAR_SCH_ARCHIVE_PLUGIN::GetAvailableSymbolFields( std::vector<wxString>& aNames )
{
    std::set<wxString> fieldNames;

    for( auto& [libnameStr, libSymbol] : m_libCache )
    {
        std::vector<LIB_FIELD*> fields;
        libSymbol->GetFields( fields );

        for( LIB_FIELD* field : fields )
        {
            if( field->IsMandatory() )
                continue;

            fieldNames.insert( field->GetName() );
        }
    }

    std::copy( fieldNames.begin(), fieldNames.end(), std::back_inserter( aNames ) );
}


void CADSTAR_SCH_ARCHIVE_PLUGIN::SymbolLibOptions( STRING_UTF8_MAP* aListToAppendTo ) const
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


void CADSTAR_SCH_ARCHIVE_PLUGIN::ensureLoadedLibrary( const wxString& aLibraryPath,
                                                      const STRING_UTF8_MAP* aProperties )
{
    wxFileName csafn;
    wxString   fplibname = "cadstarpcblib";

    if( aProperties->count( "csa" ) )
    {
        csafn = wxFileName( aProperties->at( "csa" ) );
    }
    else
    {
        // If none specified, use
        // symbol.csa in same folder as the .lib
        csafn = wxFileName( aLibraryPath );
        csafn.SetName( "symbol" );
        csafn.SetExt( "csa" );
    }

    if( aProperties->count( "fplib" ) )
    {
        fplibname = aProperties->at( "fplib" );
    }

    // Get timestamp
    long long  timestamp = 0;
    wxFileName fn( aLibraryPath );

    if( fn.IsFileReadable() )
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

