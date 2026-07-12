/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * Based on the dsn2kicad reference implementation and on OrCAD file format
 * documentation from the OpenOrCadParser project (MIT licensed).
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file sch_io_orcad.cpp
 *
 * OrCAD Capture .dsn importer plugin shell: CFB navigation, stream parsing orchestration.
 */

#include <sch_io/orcad/sch_io_orcad.h>

#include <algorithm>
#include <cctype>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <wx/string.h>
#include <wx/translation.h>

#include <compoundfilereader.h>
#include <utf.h>

#include <io/altium/altium_binary_parser.h>
#include <io/io_utils.h>
#include <ki_exception.h>
#include <kiid.h>
#include <progress_reporter.h>

#include <schematic.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>

#include <sch_io/orcad/orcad_cache.h>
#include <sch_io/orcad/orcad_converter.h>
#include <sch_io/orcad/orcad_library.h>
#include <sch_io/orcad/orcad_page.h>
#include <sch_io/orcad/orcad_records.h>


namespace
{

std::vector<char> readStream( const ALTIUM_COMPOUND_FILE& aFile,
                              const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    std::vector<char> data( static_cast<size_t>( aEntry->size ) );

    if( !data.empty() )
        aFile.GetCompoundFileReader().ReadFile( aEntry, 0, data.data(), data.size() );

    return data;
}


// Direct children of storage, filtered to streams (aStreams) or sub-storages, in directory order.
std::vector<std::pair<std::string, const CFB::COMPOUND_FILE_ENTRY*>>
enumChildren( const ALTIUM_COMPOUND_FILE& aFile, const CFB::COMPOUND_FILE_ENTRY* aParent,
              bool aStreams )
{
    std::vector<std::pair<std::string, const CFB::COMPOUND_FILE_ENTRY*>> out;

    const CFB::CompoundFileReader& reader = aFile.GetCompoundFileReader();

    reader.EnumFiles( aParent, 1,
            [&]( const CFB::COMPOUND_FILE_ENTRY* aEntry, const CFB::utf16string&, int ) -> int
            {
                if( reader.IsStream( aEntry ) == aStreams )
                    out.emplace_back( UTF16ToUTF8( aEntry->name ), aEntry );

                return 0;
            } );

    return out;
}


std::string lowerCopy( const std::string& aText )
{
    std::string out = aText;

    std::transform( out.begin(), out.end(), out.begin(),
                    []( unsigned char c )
                    {
                        return static_cast<char>( std::tolower( c ) );
                    } );

    return out;
}

} // namespace


bool SCH_IO_ORCAD::CanReadSchematicFile( const wxString& aFileName ) const
{
    if( !SCH_IO::CanReadSchematicFile( aFileName ) )
        return false;

    // .dsn also names plain-text SPECCTRA session files; OrCAD design is OLE2/CFB compound doc
    if( !IO_UTILS::fileHasBinaryHeader( aFileName, IO_UTILS::COMPOUND_FILE_HEADER ) )
        return false;

    try
    {
        ALTIUM_COMPOUND_FILE cfbFile( aFileName );

        const CFB::CompoundFileReader&  reader = cfbFile.GetCompoundFileReader();
        const CFB::COMPOUND_FILE_ENTRY* root = reader.GetRootEntry();

        if( !root )
            return false;

        if( !cfbFile.FindStreamSingleLevel( root, "Library", true ) )
            return false;

        return cfbFile.FindStreamSingleLevel( root, "Views", false ) != nullptr
               || cfbFile.FindStreamSingleLevel( root, "Schematics", false ) != nullptr;
    }
    catch( const IO_ERROR& )
    {
        return false;
    }
    catch( const CFB::CFBException& )
    {
        return false;
    }
    catch( const std::exception& )
    {
        return false;
    }
}


SCH_SHEET* SCH_IO_ORCAD::LoadSchematicFile( const wxString& aFileName, SCHEMATIC* aSchematic,
                                            SCH_SHEET* aAppendToMe,
                                            const std::map<std::string, UTF8>* aProperties )
{
    wxASSERT( !aFileName.IsEmpty() && aSchematic );

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

        // Top-level sheet UUID must match schematic file UUID
        const_cast<KIID&>( rootSheet->m_Uuid ) = screen->GetUuid();
    }

    if( m_progressReporter )
    {
        m_progressReporter->Report( wxString::Format( _( "Loading %s..." ), aFileName ) );

        if( !m_progressReporter->KeepRefreshing() )
            THROW_IO_ERROR( _( "Open canceled by user." ) );
    }

    ORCAD_WARN_FN warnFn = [this]( const wxString& aMsg )
    {
        if( m_reporter )
            m_reporter->Report( aMsg, RPT_SEVERITY_WARNING );
    };

    ORCAD_DESIGN design;

    try
    {
        ALTIUM_COMPOUND_FILE cfbFile( aFileName );

        const CFB::CompoundFileReader&  reader = cfbFile.GetCompoundFileReader();
        const CFB::COMPOUND_FILE_ENTRY* root = reader.GetRootEntry();

        // 'Library' stream: version, fonts, string table
        const CFB::COMPOUND_FILE_ENTRY* libraryEntry =
                cfbFile.FindStreamSingleLevel( root, "Library", true );

        if( !libraryEntry )
        {
            THROW_IO_ERROR( _( "The file does not contain the 'Library' stream of an OrCAD "
                               "Capture design." ) );
        }

        design.library = OrcadParseLibrary( readStream( cfbFile, libraryEntry ) );

        // Designs saved before OrCAD 10.x (2003) use a different stream framing.
        if( design.library.versionMajor < 3 )
        {
            THROW_IO_ERROR( wxString::Format( _( "This file uses the pre-2003 OrCAD Capture "
                                                 "format (version %d.%d), which is not "
                                                 "supported yet." ),
                                              design.library.versionMajor,
                                              design.library.versionMinor ) );
        }

        // -- 'Cache' stream: symbol definitions and package pin maps ------------------

        if( const CFB::COMPOUND_FILE_ENTRY* cacheEntry =
                    cfbFile.FindStreamSingleLevel( root, "Cache", true ) )
        {
            OrcadParseCache( readStream( cfbFile, cacheEntry ), design.library.strings, warnFn,
                             design.symbols, design.packages );
        }
        else
        {
            warnFn( _( "The design has no 'Cache' stream; placeholder symbols will be "
                       "synthesized for all parts." ) );
        }

        // 'Packages/<name>' streams: locally modified parts, same framing
        if( const CFB::COMPOUND_FILE_ENTRY* packagesStorage =
                    cfbFile.FindStreamSingleLevel( root, "Packages", false ) )
        {
            for( const auto& [streamName, entry] : enumChildren( cfbFile, packagesStorage, true ) )
            {
                std::map<std::string, ORCAD_SYMBOL_DEF> extraSymbols;
                std::map<std::string, ORCAD_PACKAGE>    extraPackages;

                try
                {
                    OrcadParseCache( readStream( cfbFile, entry ), design.library.strings,
                                     warnFn, extraSymbols, extraPackages );
                }
                catch( const IO_ERROR& e )
                {
                    // CFB entry names UTF-16 in container, UTF-8 here
                    warnFn( wxString::Format( _( "Package stream '%s' could not be parsed: %s" ),
                                              wxString::FromUTF8( streamName ), e.What() ) );
                    continue;
                }

                OrcadMergeCacheStreams( design.symbols, design.packages,
                                        std::move( extraSymbols ), std::move( extraPackages ) );
            }
        }

        // 'Views/<folder>': one storage per schematic folder
        const CFB::COMPOUND_FILE_ENTRY* viewsStorage =
                cfbFile.FindStreamSingleLevel( root, "Views", false );

        if( !viewsStorage )
        {
            THROW_IO_ERROR( _( "The file does not contain a 'Views' storage; it is not a "
                               "supported OrCAD Capture design." ) );
        }

        std::vector<std::string>                                folders;
        std::map<std::string, const CFB::COMPOUND_FILE_ENTRY*> folderEntries;

        for( const auto& [folderName, entry] : enumChildren( cfbFile, viewsStorage, false ) )
        {
            if( folderEntries.emplace( folderName, entry ).second )
                folders.push_back( folderName );
        }

        std::sort( folders.begin(), folders.end() );

        if( folders.empty() )
            THROW_IO_ERROR( _( "The design contains no schematic folders." ) );

        // Root folder = folder matching Library schematic name (any case); others are
        // hierarchical children, skipped here
        std::string rootFolder;
        std::string schematicName = lowerCopy( design.library.schematicName );

        if( !schematicName.empty() )
        {
            for( const std::string& folder : folders )
            {
                if( lowerCopy( folder ) == schematicName )
                {
                    rootFolder = folder;
                    break;
                }
            }
        }

        if( rootFolder.empty() )
            rootFolder = folders.front();

        for( const std::string& folder : folders )
        {
            if( folder != rootFolder )
                design.skippedFolders.push_back( folder );
        }

        design.name = design.library.schematicName.empty() ? rootFolder
                                                           : design.library.schematicName;

        // -- page streams under 'Views/<root>/Pages' -----------------------------------

        const CFB::COMPOUND_FILE_ENTRY* folderEntry = folderEntries[rootFolder];
        const CFB::COMPOUND_FILE_ENTRY* pagesStorage =
                cfbFile.FindStreamSingleLevel( folderEntry, "Pages", false );

        std::vector<std::string>                                available;
        std::map<std::string, const CFB::COMPOUND_FILE_ENTRY*> pageEntries;

        if( pagesStorage )
        {
            for( const auto& [pageName, entry] : enumChildren( cfbFile, pagesStorage, true ) )
            {
                if( pageEntries.emplace( pageName, entry ).second )
                    available.push_back( pageName );
            }
        }

        // Display order comes from the folder's 'Schematic' stream; fall back to
        // name order when it is missing or unreadable.
        std::vector<std::string> ordered;
        bool                     orderKnown = false;

        if( const CFB::COMPOUND_FILE_ENTRY* orderEntry =
                    cfbFile.FindStreamSingleLevel( folderEntry, "Schematic", true ) )
        {
            try
            {
                for( const std::string& pageName :
                     OrcadParsePageOrder( readStream( cfbFile, orderEntry ) ) )
                {
                    if( pageEntries.count( pageName )
                        && std::find( ordered.begin(), ordered.end(), pageName )
                                   == ordered.end() )
                    {
                        ordered.push_back( pageName );
                    }
                }

                orderKnown = true;
            }
            catch( const IO_ERROR& e )
            {
                warnFn( wxString::Format( _( "The page display order could not be read (%s); "
                                             "pages are imported in name order." ),
                                          e.What() ) );
                ordered.clear();
            }
        }
        else
        {
            warnFn( _( "The schematic folder has no page-order stream; pages are imported in "
                       "name order." ) );
        }

        if( orderKnown )
        {
            for( const std::string& pageName : available )
            {
                if( std::find( ordered.begin(), ordered.end(), pageName ) == ordered.end() )
                    ordered.push_back( pageName );
            }
        }
        else
        {
            ordered = available;
            std::sort( ordered.begin(), ordered.end() );
        }

        for( const std::string& pageName : ordered )
        {
            try
            {
                design.pages.push_back( OrcadParsePage( readStream( cfbFile,
                                                                    pageEntries[pageName] ),
                                                        design.library.strings, warnFn ) );
            }
            catch( const IO_ERROR& e )
            {
                // CFB entry names are UTF-16 in the container, UTF-8 here.
                warnFn( wxString::Format( _( "Page '%s' could not be parsed and was "
                                             "skipped: %s" ),
                                          wxString::FromUTF8( pageName ), e.What() ) );
            }
        }

        if( design.pages.empty() )
            THROW_IO_ERROR( _( "No schematic pages could be read from the design." ) );

        // -- hierarchy detection --------------------------------------------------------
        //
        // The link table maps block instances to child folders; it is used only to
        // name the skipped children in the conversion warnings.

        std::map<uint32_t, std::string> hierarchyLinks;

        if( const CFB::COMPOUND_FILE_ENTRY* hierarchyEntry =
                    cfbFile.FindStream( folderEntry, { "Hierarchy", "Hierarchy" } ) )
        {
            hierarchyLinks = OrcadReadHierarchyLinks( readStream( cfbFile, hierarchyEntry ),
                                                      design.library.strings, warnFn );
        }

        for( ORCAD_RAW_PAGE& page : design.pages )
        {
            if( OrcadPageHasHierarchyBlocks( page ) )
                design.hasHierarchyBlocks = true;

            for( ORCAD_DRAWN_INSTANCE& block : page.blocks )
            {
                if( block.childName.empty() )
                {
                    auto it = hierarchyLinks.find( block.dbId );

                    if( it != hierarchyLinks.end() )
                        block.childName = it->second;
                }
            }
        }
    }
    catch( const CFB::CFBException& e )
    {
        THROW_IO_ERROR( e.what() );
    }

    ORCAD_CONVERTER converter( design, aSchematic, m_reporter, m_progressReporter );

    converter.Convert( rootSheet );

    aSchematic->CurrentSheet().UpdateAllScreenReferences();
    aSchematic->FixupJunctionsAfterImport();

    return rootSheet;
}
