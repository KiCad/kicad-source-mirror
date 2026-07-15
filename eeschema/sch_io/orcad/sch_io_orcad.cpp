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

#include <lib_symbol.h>
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
    const CFB::CompoundFileReader& reader = aFile.GetCompoundFileReader();

    // Stream cannot exceed file; corrupt entry claiming more must not drive huge allocation
    uint64_t size = reader.GetStreamSize( aEntry );

    if( size > reader.GetBufferLen() )
        THROW_IO_ERROR( wxS( "OrCAD stream size exceeds the compound file" ) );

    std::vector<char> data( static_cast<size_t>( size ) );

    if( !data.empty() )
        reader.ReadFile( aEntry, 0, data.data(), data.size() );

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

        // Pre-2003 designs use pre-preamble framing; pages via v2 reader, symbol bodies
        // (v2 cache framing undecoded) synthesized as placeholders
        bool isV2 = design.library.versionMajor < 3;

        // 'Cache' stream: symbol defs and package pin maps
        if( isV2 )
        {
            warnFn( _( "This is a pre-2003 OrCAD design; symbol graphics are synthesized as "
                       "placeholders (the legacy symbol cache format is not decoded)." ) );
        }
        else if( const CFB::COMPOUND_FILE_ENTRY* cacheEntry =
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
                    !isV2 ? cfbFile.FindStreamSingleLevel( root, "Packages", false ) : nullptr )
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

        design.name = design.library.schematicName.empty() ? rootFolder
                                                           : design.library.schematicName;

        // Hierarchical designs split pages across sibling folders linked by block instances;
        // import all as flat pages (root first) so nothing dropped. Hierarchy streams yield
        // occurrence designators and block->child-folder links, merged design-wide
        std::map<uint32_t, std::string> hierarchyLinks;

        auto parseFolderPages = [&]( const std::string&              aFolderName,
                                     const CFB::COMPOUND_FILE_ENTRY* aFolderEntry,
                                     std::vector<ORCAD_RAW_PAGE>&    aOutPages )
        {
            const CFB::COMPOUND_FILE_ENTRY* pagesStorage =
                    cfbFile.FindStreamSingleLevel( aFolderEntry, "Pages", false );

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

            // Display order from folder's 'Schematic' stream; fall back to name order if absent
            std::vector<std::string> ordered;
            bool                     orderKnown = false;

            if( const CFB::COMPOUND_FILE_ENTRY* orderEntry =
                        cfbFile.FindStreamSingleLevel( aFolderEntry, "Schematic", true ) )
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
                    warnFn( wxString::Format(
                            _( "The page display order for schematic folder '%s' could not be "
                               "read (%s); pages are imported in name order." ),
                            wxString::FromUTF8( aFolderName ), e.What() ) );
                    ordered.clear();
                }
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
                    std::vector<char> pageData = readStream( cfbFile, pageEntries[pageName] );

                    aOutPages.push_back( isV2 ? OrcadParsePageV2( pageData, design.library.strings,
                                                                  warnFn )
                                              : OrcadParsePage( pageData, design.library.strings,
                                                                warnFn ) );
                }
                catch( const IO_ERROR& e )
                {
                    warnFn( wxString::Format( _( "Page '%s' could not be parsed and was "
                                                 "skipped: %s" ),
                                              wxString::FromUTF8( pageName ), e.What() ) );
                }
            }
        };

        parseFolderPages( rootFolder, folderEntries[rootFolder], design.pages );

        // Root folder's Hierarchy stream holds whole occurrence tree (part refdes + nested blocks)
        if( const CFB::COMPOUND_FILE_ENTRY* hierarchyEntry =
                    cfbFile.FindStream( folderEntries[rootFolder], { "Hierarchy", "Hierarchy" } ) )
        {
            std::vector<char> hierarchyData = readStream( cfbFile, hierarchyEntry );

            design.occurrenceRoot = OrcadReadOccurrenceTree( hierarchyData, warnFn );

            if( design.occurrenceRoot.blocks.empty() && design.occurrenceRoot.partRefs.empty() )
            {
                // Legacy/unreadable tree: scan-recover flat root-scope refs and block links,
                // rebuild flat block occurrences so converter still schedules each child once
                hierarchyLinks = OrcadReadHierarchyLinks( hierarchyData, design.library.strings,
                                                          warnFn, &design.occurrenceRoot.partRefs );

                for( const auto& [dbId, childName] : hierarchyLinks )
                {
                    ORCAD_OCC_BLOCK block;
                    block.targetDbId = dbId;
                    block.childFolder = childName;
                    design.occurrenceRoot.blocks.push_back( std::move( block ) );
                }
            }

            // Block instance dbId -> child folder name, from occurrence tree
            std::function<void( const ORCAD_OCC_SCOPE& )> collectLinks =
                    [&]( const ORCAD_OCC_SCOPE& aScope )
            {
                for( const ORCAD_OCC_BLOCK& block : aScope.blocks )
                {
                    hierarchyLinks[block.targetDbId] = block.childFolder;
                    collectLinks( block.scope );
                }
            };

            collectLinks( design.occurrenceRoot );
        }

        if( design.pages.empty() )
            THROW_IO_ERROR( _( "No schematic pages could be read from the design." ) );

        // Parse pages of every block-reachable folder once; instantiated per block occurrence
        // during conversion. Unreferenced alternate/backup folders left out
        std::map<std::string, std::string> folderByLowerName;

        for( const std::string& folder : folders )
            folderByLowerName.emplace( lowerCopy( folder ), folder );

        for( const auto& [dbId, childName] : hierarchyLinks )
        {
            std::string key = lowerCopy( childName );

            if( key == lowerCopy( rootFolder ) || design.childFolderPages.count( key ) )
                continue;

            auto childIt = folderByLowerName.find( key );

            if( childIt != folderByLowerName.end() )
                parseFolderPages( childIt->second, folderEntries[childIt->second],
                                  design.childFolderPages[key] );
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
