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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <sch_io/easypc/sch_io_easypc.h>
#include <sch_io/easypc/easypc_sch_builder.h>
#include <sch_io/easypc/easypc_sch_items.h>

#include <io/easypc/easypc_classes_library.h>
#include <io/easypc/easypc_classes_project.h>
#include <io/easypc/easypc_document.h>

#include <ki_exception.h>
#include <lib_id.h>
#include <lib_symbol.h>
#include <progress_reporter.h>
#include <project.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <schematic.h>
#include <string_utils.h>
#include <wildcards_and_files_ext.h>

#include <wx/filename.h>


SCH_IO_EASYPC::SCH_IO_EASYPC() :
        SCH_IO( wxS( "Easy-PC / DesignSpark Schematic" ) )
{
    m_reporter = &WXLOG_REPORTER::GetInstance();
}


SCH_IO_EASYPC::~SCH_IO_EASYPC() = default;


bool SCH_IO_EASYPC::CanReadSchematicFile( const wxString& aFileName ) const
{
    if( !SCH_IO::CanReadSchematicFile( aFileName ) )
        return false;

    EASYPC::FILE_INFO info = EASYPC::Probe( aFileName );
    return info.IsProject || info.FileKind == EASYPC::FILE_KIND::SCHEMATIC_DESIGN;
}


bool SCH_IO_EASYPC::CanReadLibrary( const wxString& aFileName ) const
{
    if( !SCH_IO::CanReadLibrary( aFileName ) )
        return false;

    EASYPC::FILE_INFO info = EASYPC::Probe( aFileName );

    return info.FileKind == EASYPC::FILE_KIND::SCHEMATIC_SYMBOL_LIBRARY
           || info.FileKind == EASYPC::FILE_KIND::COMPONENT_LIBRARY;
}


SCH_IO_EASYPC::LIBRARY& SCH_IO_EASYPC::library( const wxString& aLibraryPath )
{
    const long long stamp = EASYPC::LIBRARY_FILE::Timestamp( aLibraryPath );
    auto            it = m_libraries.find( aLibraryPath );

    if( it != m_libraries.end() && it->second.Timestamp == stamp )
        return it->second;

    if( it != m_libraries.end() )
        m_libraries.erase( it );

    std::unique_ptr<EASYPC::LIBRARY_FILE> file = std::make_unique<EASYPC::LIBRARY_FILE>( aLibraryPath );
    LIBRARY&                              lib = m_libraries[aLibraryPath];

    for( const wxString& warning : file->Warnings() )
        m_reporter->Report( warning, RPT_SEVERITY_WARNING );

    lib.Timestamp = stamp;
    lib.File = std::move( file );
    return lib;
}


void SCH_IO_EASYPC::EnumerateSymbolLib( wxArrayString& aSymbolNameList, const wxString& aLibraryPath,
                                        const std::map<std::string, UTF8>* aProperties )
{
    // KiCad names carry the characters a LIB_ID forbids escaped, and LoadSymbol takes them back the same way
    for( const wxString& name : library( aLibraryPath ).File->ItemNames() )
        aSymbolNameList.Add( EscapeString( name, CTX_LIBID ) );
}


void SCH_IO_EASYPC::EnumerateSymbolLib( std::vector<LIB_SYMBOL*>& aSymbolList, const wxString& aLibraryPath,
                                        const std::map<std::string, UTF8>* aProperties )
{
    for( const wxString& name : library( aLibraryPath ).File->ItemNames() )
    {
        if( LIB_SYMBOL* symbol = LoadSymbol( aLibraryPath, EscapeString( name, CTX_LIBID ) ) )
            aSymbolList.push_back( symbol );
    }
}


/// Whether every package gives every gate terminal the same pin number
static bool packagesAgree( const EASYPC::SOURCE_SCM_COMPONENT&                     aScm,
                           const std::vector<const EASYPC::SOURCE_PCB_COMPONENT*>& aPackages )
{
    std::vector<EASYPC::SOURCE_GATE*> gates = EASYPC::TypedItems<EASYPC::SOURCE_GATE>( aScm.Gates );

    for( size_t g = 0; g < gates.size(); ++g )
    {
        for( int t = 0; t < static_cast<int>( gates[g]->PinNames.size() ); ++t )
        {
            EASYPC::GATE_TERMINAL first = EASYPC::ResolveTerminal( aPackages.front(), static_cast<int>( g ), t );

            for( const EASYPC::SOURCE_PCB_COMPONENT* pcb : aPackages )
            {
                EASYPC::GATE_TERMINAL other = EASYPC::ResolveTerminal( pcb, static_cast<int>( g ), t );

                if( other.Mapped != first.Mapped || other.Number != first.Number )
                    return false;
            }
        }
    }

    return true;
}


std::unique_ptr<LIB_SYMBOL> SCH_IO_EASYPC::convertComponent( const LIBRARY& aLib, const wxString& aLibraryPath,
                                                             const wxString& aName )
{
    std::unique_ptr<EASYPC::LIBRARY_ITEM>            item = aLib.File->LoadItem( aName );
    std::vector<const EASYPC::SOURCE_PCB_COMPONENT*> packages;
    EASYPC::SCH_COMPONENT_SOURCE                     src;

    for( const EASYPC::LIBRARY_STREAM& stream : item->Streams )
    {
        if( const EASYPC::SOURCE_SCM_COMPONENT* scm = dynamic_cast<const EASYPC::SOURCE_SCM_COMPONENT*>( stream.Item ) )
            src.Schematic = scm;
    }

    if( !src.Schematic )
    {
        THROW_IO_ERROR(
                wxString::Format( _( "Easy-PC component '%s' in '%s' has no schematic part" ), aName, aLibraryPath ) );
    }

    for( const EASYPC::LIBRARY_STREAM& stream : item->Streams )
    {
        const EASYPC::SOURCE_PCB_COMPONENT* pcb = dynamic_cast<const EASYPC::SOURCE_PCB_COMPONENT*>( stream.Item );

        if( !pcb )
            continue;

        // A package with no library names its footprint only, which LIB_ID allows
        const wxString nickname = wxFileName( pcb->Library, wxPATH_WIN ).GetName();
        const wxString footprint = nickname.IsEmpty() ? pcb->Symbol : nickname + wxS( ":" ) + pcb->Symbol;

        packages.push_back( pcb );
        src.FootprintFilters.Add( footprint );

        // A component whose named default package is missing has no default footprint
        if( stream.StreamName == src.Schematic->Package )
        {
            src.Board = pcb;
            src.Footprint = footprint;
        }
    }

    // Without a default package the pin numbers stand when every package numbers every gate pin alike
    if( !src.Board && !packages.empty() )
    {
        if( packagesAgree( *src.Schematic, packages ) )
        {
            src.Board = packages.front();
        }
        else
        {
            m_reporter->Report( wxString::Format( _( "Easy-PC component '%s' in '%s' has no default package and its "
                                                     "packages number the pins differently; its pins are left "
                                                     "unnumbered." ),
                                                  aName, aLibraryPath ),
                                RPT_SEVERITY_WARNING );
        }
    }

    // Gate symbols live in other libraries; each loaded item stays alive until the conversion is done
    std::vector<std::unique_ptr<EASYPC::LIBRARY_ITEM>> gateItems;
    const wxString                                     cmlName = wxFileName( aLibraryPath ).GetFullName();
    int                                                gateNumber = 0;

    for( const EASYPC::OBJECT* obj : EASYPC::ListItems( src.Schematic->Gates ) )
    {
        const EASYPC::SOURCE_GATE* gate = dynamic_cast<const EASYPC::SOURCE_GATE*>( obj );
        ++gateNumber;

        if( !gate )
        {
            THROW_IO_ERROR( wxString::Format( _( "Easy-PC component '%s' in '%s' has an empty gate slot" ), aName,
                                              aLibraryPath ) );
        }

        // Stored absolute paths may name another machine; resolve the filename beside this library.
        const wxString               sslName = wxFileName( gate->Library, wxPATH_WIN ).GetFullName();
        const wxString               sslPath = EASYPC::ResolveProjectItem( aLibraryPath, sslName );
        const EASYPC::SOURCE_SYMBOL* symbol = nullptr;
        wxString                     msg;

        if( !wxFileName::FileExists( sslPath ) )
        {
            msg = wxString::Format( _( "Component '%s': gate %d symbol library '%s' not found beside '%s'." ), aName,
                                    gateNumber, sslName, cmlName );
        }
        else
        {
            try
            {
                gateItems.push_back( library( sslPath ).File->LoadItem( gate->Symbol ) );

                if( !gateItems.back()->Streams.empty() )
                    symbol = dynamic_cast<const EASYPC::SOURCE_SYMBOL*>( gateItems.back()->Streams.front().Item );

                if( !symbol )
                {
                    msg = wxString::Format( _( "Component '%s': gate %d item '%s' in '%s' is not a symbol." ), aName,
                                            gateNumber, gate->Symbol, sslName );
                }
            }
            catch( const IO_ERROR& e )
            {
                msg = wxString::Format( _( "Component '%s': gate %d symbol '%s' could not be loaded from '%s' "
                                           "(%s)." ),
                                        aName, gateNumber, gate->Symbol, sslName, e.What() );
            }
        }

        if( !symbol )
        {
            m_reporter->Report( msg, RPT_SEVERITY_WARNING );
            THROW_IO_ERROR( msg );
        }

        src.Gates.push_back( symbol );
    }

    LIB_ID id( wxFileName( aLibraryPath ).GetName(), EscapeString( aName, CTX_LIBID ) );
    return EASYPC::ConvertComponentSymbol( src, id );
}


LIB_SYMBOL* SCH_IO_EASYPC::LoadSymbol( const wxString& aLibraryPath, const wxString& aPartName,
                                       const std::map<std::string, UTF8>* aProperties )
{
    LIBRARY& lib = library( aLibraryPath );

    if( auto it = lib.Symbols.find( aPartName ); it != lib.Symbols.end() )
        return it->second.get();

    const wxString item = UnescapeString( aPartName );

    if( !lib.File->HasItem( item ) )
        return nullptr;

    std::unique_ptr<LIB_SYMBOL> symbol;

    // lib is passed on, since looking the library up again could reload it and leave lib dangling
    if( lib.File->Kind() == EASYPC::FILE_KIND::COMPONENT_LIBRARY )
    {
        symbol = convertComponent( lib, aLibraryPath, item );
    }
    else if( lib.File->Kind() == EASYPC::FILE_KIND::SCHEMATIC_SYMBOL_LIBRARY )
    {
        std::unique_ptr<EASYPC::LIBRARY_ITEM> loaded = lib.File->LoadItem( item );
        const EASYPC::SOURCE_SYMBOL*          sym = nullptr;

        if( !loaded->Streams.empty() )
            sym = dynamic_cast<const EASYPC::SOURCE_SYMBOL*>( loaded->Streams.front().Item );

        if( !sym )
        {
            THROW_IO_ERROR(
                    wxString::Format( _( "Easy-PC library item '%s' in '%s' is not a symbol" ), item, aLibraryPath ) );
        }

        LIB_ID id( wxFileName( aLibraryPath ).GetName(), EscapeString( item, CTX_LIBID ) );
        symbol = EASYPC::ConvertSymbol( *sym, id );
    }
    else
    {
        THROW_IO_ERROR( wxString::Format( _( "'%s' is not an Easy-PC symbol or component library" ), aLibraryPath ) );
    }

    LIB_SYMBOL* result = symbol.get();
    lib.Symbols[aPartName] = std::move( symbol );
    return result;
}


SCH_SHEET* SCH_IO_EASYPC::LoadSchematicFile( const wxString& aFileName, SCHEMATIC* aSchematic, SCH_SHEET* aAppendToMe,
                                             const std::map<std::string, UTF8>* aProperties )
{
    wxCHECK( !aFileName.IsEmpty() && aSchematic, nullptr );

    // Without a sheet to fill, the load would replace the live top-level sheets
    if( !aAppendToMe && aProperties && aProperties->count( "hierarchical_sheet_load" ) )
    {
        THROW_IO_ERROR( wxString::Format( _( "'%s' is a complete Easy-PC schematic and cannot be loaded as a "
                                             "hierarchical sheet. Use File > Import > Non-KiCad Schematic... "
                                             "instead." ),
                                          aFileName ) );
    }

    if( m_progressReporter )
    {
        m_progressReporter->Report( wxString::Format( _( "Loading %s..." ), aFileName ) );

        if( !m_progressReporter->KeepRefreshing() )
            THROW_IO_CANCELLED();
    }

    std::vector<wxString> paths;

    if( wxFileName( aFileName ).GetExt().CmpNoCase( wxS( "prj" ) ) == 0 )
    {
        std::unique_ptr<EASYPC::PROJECT_DOCUMENT> prj = EASYPC::LoadProject( aFileName );
        for( const EASYPC::SOURCE_PROJECT_ITEM* item : prj->Project->Schematics )
            paths.push_back( EASYPC::ResolveProjectItem( aFileName, item->Name ) );

        if( paths.empty() )
            THROW_IO_ERROR( wxString::Format( _( "Project '%s' contains no schematics" ), aFileName ) );
    }
    else
    {
        paths.push_back( aFileName );
    }

    std::vector<std::unique_ptr<EASYPC::DESIGN_DOCUMENT>> docs;
    std::vector<const EASYPC::DESIGN_DOCUMENT*>           allDocs;
    EASYPC::SCH_NET_NAMES                                 names;

    for( const wxString& path : paths )
    {
        docs.push_back( EASYPC::LoadDesign( path ) );
        allDocs.push_back( docs.back().get() );

        if( docs.back()->Kind != EASYPC::DOC_KIND::SCHEMATIC )
            THROW_IO_ERROR( wxString::Format( _( "'%s' is not a schematic design" ), path ) );

        names.AddSheet( *docs.back() );
    }

    if( aAppendToMe )
    {
        wxCHECK_MSG( aSchematic->IsValid(), nullptr, wxS( "Can't append to a schematic with no root!" ) );

        if( docs.size() != 1 )
        {
            THROW_IO_ERROR( wxString::Format( _( "'%s' holds %zu schematics; only one can be appended to a sheet" ),
                                              aFileName, docs.size() ) );
        }

        if( !aAppendToMe->GetScreen() )
        {
            wxFileName fn( paths.front() );
            fn.SetExt( FILEEXT::KiCadSchematicFileExtension );

            aAppendToMe->SetScreen( new SCH_SCREEN( aSchematic ) );
            aAppendToMe->GetScreen()->SetFileName( fn.GetFullPath() );
            aAppendToMe->SyncUuidToScreen();
        }
    }

    // A real UUID, unlike the virtual root's nil one, so SetTopLevelSheets() keeps it
    SCH_SHEET* rootSheet = aAppendToMe ? aAppendToMe : new SCH_SHEET( aSchematic );

    if( !aAppendToMe )
        aSchematic->SetTopLevelSheets( { rootSheet } );

    std::vector<SCH_SHEET*> sheets;
    EASYPC::SCH_BUILDER     builder( aSchematic, names, m_reporter );

    builder.SetProjectSymbols( allDocs );

    // Every sheet of a project is a top-level KiCad sheet; no known file holds a block (hierarchical) symbol
    for( size_t i = 0; i < docs.size(); ++i )
    {
        SCH_SHEET*     sheet = sheets.empty() ? rootSheet : new SCH_SHEET( &aSchematic->Root() );
        SCH_SHEET_PATH path;
        path.push_back( sheet );
        sheets.push_back( sheet );

        // The sheet appended to keeps its own name, file and page
        if( sheet != aAppendToMe )
        {
            wxFileName fn( paths[i] );
            fn.SetExt( FILEEXT::KiCadSchematicFileExtension );

            sheet->SetScreen( new SCH_SCREEN( aSchematic ) );
            sheet->GetScreen()->SetFileName( fn.GetFullPath() );
            sheet->SetFileName( fn.GetFullName() );
            sheet->SetName( wxFileName( paths[i] ).GetName() );

            path.SetPageNumber( wxString::Format( wxS( "%zu" ), i + 1 ) );
            sheet->GetScreen()->SetPageNumber( wxString::Format( wxS( "%zu" ), i + 1 ) );
        }

        builder.BuildSheet( *docs[i], sheet, path );
    }

    if( aAppendToMe )
    {
        aSchematic->RefreshHierarchy();
        return aAppendToMe;
    }

    aSchematic->SetTopLevelSheets( sheets );
    aSchematic->RefreshHierarchy();

    // The foreign-import path does not create per-sheet instance data, so references would not resolve
    wxString projectName = aSchematic->Project().GetProjectName();

    if( projectName.IsEmpty() )
        projectName = wxFileName( aFileName ).GetName();

    SCH_SHEET_LIST list = aSchematic->BuildUnorderedSheetList();
    list.AddNewSymbolInstances( SCH_SHEET_PATH(), projectName );
    list.AddNewSheetInstances( SCH_SHEET_PATH(), 0 );

    return rootSheet;
}
