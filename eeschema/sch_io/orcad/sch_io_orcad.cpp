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



#include <sch_io/orcad/sch_io_orcad.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
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
#include <sch_io/orcad/orcad_cis.h>
#include <sch_io/orcad/orcad_converter.h>
#include <sch_io/orcad/orcad_library.h>
#include <sch_io/orcad/orcad_page.h>
#include <sch_io/orcad/orcad_records.h>


std::string OrcadNormalizeCfbName( const std::string& aName )
{
    std::string name = aName;
    std::replace( name.begin(), name.end(), '\x02', '/' );
    std::replace( name.begin(), name.end(), '\x03', ':' );
    return name;
}


namespace
{

void assignPostImportUuids( SCHEMATIC* aSchematic, const std::string& aSourceId )
{
    std::set<SCH_SCREEN*> screens;
    size_t                screenOrdinal = 0;

    for( const SCH_SHEET_PATH& path : aSchematic->BuildSheetListSortedByPageNumbers() )
    {
        SCH_SCREEN* screen = path.LastScreen();

        if( !screen || !screens.insert( screen ).second )
            continue;

        std::map<std::string, size_t> ordinals;

        for( SCH_ITEM* item : screen->Items() )
        {
            std::string uuid = item->m_Uuid.AsStdString();

            if( uuid.size() > 14 && uuid[14] == '5' )
                continue;

            VECTOR2I    position = item->GetPosition();
            std::string role = std::to_string( static_cast<int>( item->Type() ) ) + ":" + std::to_string( position.x )
                               + ":" + std::to_string( position.y );
            size_t ordinal = ordinals[role]++;
            const_cast<KIID&>( item->m_Uuid ) =
                    KIID::FromName( "orcad-import:" + aSourceId + ":post:" + std::to_string( screenOrdinal ) + ":"
                                    + role + ":" + std::to_string( ordinal ) );
        }

        ++screenOrdinal;
    }
}

std::vector<char> readStream( const ALTIUM_COMPOUND_FILE& aFile, const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    const CFB::CompoundFileReader& reader = aFile.GetCompoundFileReader();

    // Stream cannot exceed file; corrupt entry claiming more must not drive huge allocation
    uint64_t size = reader.GetStreamSize( aEntry );

    if( size > reader.GetBufferLen() )
        THROW_IO_ERROR( _( "OrCAD stream size exceeds the compound file" ) );

    std::vector<char> data( static_cast<size_t>( size ) );

    if( !data.empty() )
        reader.ReadFile( aEntry, 0, data.data(), data.size() );

    return data;
}


bool isLongFramedPackageStream( const std::vector<char>& aData )
{
    if( aData.size() < 11 )
        return false;

    auto byte = [&]( size_t aOffset )
    {
        return static_cast<uint8_t>( aData[aOffset] );
    };
    uint32_t bodyLength = byte( 3 ) | static_cast<uint32_t>( byte( 4 ) ) << 8 | static_cast<uint32_t>( byte( 5 ) ) << 16
                          | static_cast<uint32_t>( byte( 6 ) ) << 24;

    return byte( 7 ) == 0 && byte( 8 ) == 0 && byte( 9 ) == 0 && byte( 10 ) == 0 && bodyLength <= aData.size() - 11;
}


// Direct children of storage, filtered to streams (aStreams) or sub-storages, in directory order.
std::vector<std::pair<std::string, const CFB::COMPOUND_FILE_ENTRY*>>
enumChildren( const ALTIUM_COMPOUND_FILE& aFile, const CFB::COMPOUND_FILE_ENTRY* aParent, bool aStreams )
{
    std::vector<std::pair<std::string, const CFB::COMPOUND_FILE_ENTRY*>> out;

    const CFB::CompoundFileReader& reader = aFile.GetCompoundFileReader();

    reader.EnumFiles( aParent, 1,
                      [&]( const CFB::COMPOUND_FILE_ENTRY* aEntry, const CFB::utf16string&, int ) -> int
                      {
                          if( reader.IsStream( aEntry ) == aStreams )
                              out.emplace_back( OrcadNormalizeCfbName( UTF16ToUTF8( aEntry->name ) ), aEntry );

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


void mergeCisProperties( ORCAD_OCC_SCOPE& aScope, uint32_t aOccurrence,
                         const std::map<std::string, std::string>& aProperties, bool& aMatched )
{
    if( aScope.partRefs.count( aOccurrence ) || aScope.partProps.count( aOccurrence ) )
    {
        auto& target = aScope.partProps[aOccurrence];

        for( const auto& [name, value] : aProperties )
            target.insert_or_assign( name, value );

        aMatched = true;
    }

    for( ORCAD_OCC_BLOCK& block : aScope.blocks )
        mergeCisProperties( block.scope, aOccurrence, aProperties, aMatched );
}


void applyCisVariant( const ALTIUM_COMPOUND_FILE& aFile, const CFB::COMPOUND_FILE_ENTRY* aRoot,
                      const std::map<std::string, UTF8>* aProperties, ORCAD_DESIGN& aDesign, REPORTER* aReporter )
{
    const CFB::COMPOUND_FILE_ENTRY* cisStorage = aFile.FindStreamSingleLevel( aRoot, "CIS", false );

    if( !cisStorage )
        return;

    const CFB::COMPOUND_FILE_ENTRY* variantStore = aFile.FindStreamSingleLevel( cisStorage, "VariantStore", false );

    if( !variantStore )
        return;

    const CFB::COMPOUND_FILE_ENTRY* bomStorage = aFile.FindStreamSingleLevel( variantStore, "BOM", false );

    if( !bomStorage )
        return;

    const CFB::COMPOUND_FILE_ENTRY* bomData = aFile.FindStreamSingleLevel( bomStorage, "BOMDataStream", true );

    if( !bomData )
        return;

    std::vector<std::string>   names = OrcadCisParseCountedList( readStream( aFile, bomData ), 0xF9 );
    std::optional<std::string> requested;

    if( aProperties )
    {
        auto request = aProperties->find( "orcad_cis_variant" );

        if( request != aProperties->end() )
            requested = request->second;
    }

    std::string selected = OrcadCisSelectVariant( names, requested );

    if( selected.empty() )
        return;

    auto applyVariantName = [&]( std::vector<ORCAD_RAW_PAGE>& aPages )
    {
        constexpr std::string_view placeholder = "<Core Design>";

        for( ORCAD_RAW_PAGE& page : aPages )
        {
            for( ORCAD_GRAPHIC_INST& titleBlock : page.titleBlocks )
            {
                for( auto& [name, value] : titleBlock.props )
                {
                    size_t offset = 0;

                    while( ( offset = value.find( placeholder, offset ) ) != std::string::npos )
                    {
                        value.replace( offset, placeholder.size(), selected );
                        offset += selected.size();
                    }
                }
            }
        }
    };

    applyVariantName( aDesign.pages );

    for( auto& [folder, pages] : aDesign.childFolderPages )
        applyVariantName( pages );

    for( auto& [folder, pages] : aDesign.unreferencedFolderPages )
        applyVariantName( pages );

    std::map<std::string, const CFB::COMPOUND_FILE_ENTRY*> variantEntries;

    for( const auto& [name, entry] : enumChildren( aFile, bomStorage, false ) )
        variantEntries.emplace( name, entry );

    auto selectedEntry = variantEntries.find( selected );

    if( selectedEntry == variantEntries.end() )
        THROW_IO_ERROR( _( "The selected OrCAD CIS variant has no definition storage." ) );

    const CFB::COMPOUND_FILE_ENTRY* definition = aFile.FindStreamSingleLevel( selectedEntry->second, selected, true );

    if( !definition )
        THROW_IO_ERROR( _( "The selected OrCAD CIS variant has no definition stream." ) );

    std::vector<std::string>        selectedGroups = OrcadCisParseCountedList( readStream( aFile, definition ), 0xF9 );
    const CFB::COMPOUND_FILE_ENTRY* groupsStorage = aFile.FindStreamSingleLevel( variantStore, "Groups", false );
    std::map<std::string, const CFB::COMPOUND_FILE_ENTRY*> updateStreams;
    std::map<std::string, std::string>                     schematicGroupNames;

    if( groupsStorage )
    {
        for( const auto& [groupName, groupEntry] : enumChildren( aFile, groupsStorage, false ) )
        {
            schematicGroupNames.emplace( groupName, groupName );

            if( const CFB::COMPOUND_FILE_ENTRY* update =
                        aFile.FindStreamSingleLevel( groupEntry, "UpdateStorageGroupDataStream", true ) )
            {
                updateStreams.emplace( groupName, update );
            }

            for( const auto& [subgroupName, subgroupEntry] : enumChildren( aFile, groupEntry, false ) )
            {
                schematicGroupNames.emplace( groupName + "_" + subgroupName, groupName + "-" + subgroupName );

                if( const CFB::COMPOUND_FILE_ENTRY* update =
                            aFile.FindStreamSingleLevel( subgroupEntry, "UpdateStorageSubGroupDataStream", true ) )
                {
                    updateStreams.emplace( groupName + "_" + subgroupName, update );
                }
            }
        }
    }

    for( const std::string& groupName : selectedGroups )
    {
        if( groupName == "Common" || groupName == "CommonNI" )
            continue;

        auto stream = updateStreams.find( groupName );

        if( stream == updateStreams.end() )
        {
            if( schematicGroupNames.count( groupName ) )
                continue;

            THROW_IO_ERROR( _( "The selected OrCAD CIS variant references an unknown property group." ) );
        }

        OrcadCisParsePropertyUpdates( readStream( aFile, stream->second ) );
    }

    ORCAD_CIS_SCHEMATIC_INFO        schematicInfo;
    const CFB::COMPOUND_FILE_ENTRY* viewsStorage = aFile.FindStreamSingleLevel( aRoot, "Views", false );

    if( viewsStorage )
    {
        for( const auto& [folderName, folderEntry] : enumChildren( aFile, viewsStorage, false ) )
        {
            const CFB::COMPOUND_FILE_ENTRY* cisSchematic =
                    aFile.FindStreamSingleLevel( folderEntry, "CISSchematic", false );

            if( !cisSchematic )
                continue;

            const CFB::COMPOUND_FILE_ENTRY* infoStorage =
                    aFile.FindStreamSingleLevel( cisSchematic, "SchematicInfoStorage", false );

            if( !infoStorage )
                continue;

            for( const auto& [streamName, streamEntry] : enumChildren( aFile, infoStorage, true ) )
            {
                if( streamName == "SchematicInfoStream" )
                    continue;

                ORCAD_CIS_SCHEMATIC_INFO pageInfo = OrcadCisParseSchematicInfo( readStream( aFile, streamEntry ) );

                for( auto& [groupName, occurrences] : pageInfo )
                {
                    auto& target = schematicInfo[groupName];

                    for( auto& [occurrence, properties] : occurrences )
                        target.insert_or_assign( occurrence, std::move( properties ) );
                }
            }
        }
    }

    for( const std::string& selectedGroup : selectedGroups )
    {
        auto groupName = schematicGroupNames.find( selectedGroup );

        if( groupName == schematicGroupNames.end() )
            continue;

        auto group = schematicInfo.find( groupName->second );

        if( group == schematicInfo.end() )
            continue;

        for( const auto& [occurrence, properties] : group->second )
        {
            bool matched = false;
            mergeCisProperties( aDesign.occurrenceRoot, occurrence, properties, matched );

            if( !matched )
            {
                auto& target = aDesign.occurrenceRoot.partProps[occurrence];

                for( const auto& [name, value] : properties )
                    target.insert_or_assign( name, value );
            }
        }
    }

    if( aReporter )
    {
        aReporter->Report( wxString::Format( _( "Using OrCAD CIS variant '%s'." ), wxString::FromUTF8( selected ) ),
                           RPT_SEVERITY_INFO );
    }
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


bool SCH_IO_ORCAD::CanReadLibrary( const wxString& aFileName ) const
{
    if( !SCH_IO::CanReadLibrary( aFileName )
        || !IO_UTILS::fileHasBinaryHeader( aFileName, IO_UTILS::COMPOUND_FILE_HEADER ) )
    {
        return false;
    }

    try
    {
        ALTIUM_COMPOUND_FILE            cfbFile( aFileName );
        const CFB::CompoundFileReader&  reader = cfbFile.GetCompoundFileReader();
        const CFB::COMPOUND_FILE_ENTRY* root = reader.GetRootEntry();

        return root && cfbFile.FindStreamSingleLevel( root, "Library", true );
    }
    catch( const std::exception& )
    {
        return false;
    }
}


SCH_SHEET* SCH_IO_ORCAD::LoadSchematicFile( const wxString& aFileName, SCHEMATIC* aSchematic, SCH_SHEET* aAppendToMe,
                                            const std::map<std::string, UTF8>* aProperties )
{
    wxASSERT( !aFileName.IsEmpty() && aSchematic );

    std::optional<wxString> sourceHash = IO_UTILS::fileHashMMH3( aFileName );

    if( !sourceHash )
        THROW_IO_ERROR( _( "The OrCAD file could not be read." ) );

    std::string sourceId( sourceHash->ToUTF8() );

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
        const_cast<KIID&>( screen->GetUuid() ) = KIID::FromName( "orcad-import:" + sourceId + ":screen:0" );
        screen->SetFileName( aFileName );
        rootSheet->SetScreen( screen );

        // Top-level sheet UUID must match schematic file UUID
        rootSheet->SyncUuidToScreen();
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
    design.sourceId = sourceId;

    try
    {
        ALTIUM_COMPOUND_FILE cfbFile( aFileName );

        const CFB::CompoundFileReader&  reader = cfbFile.GetCompoundFileReader();
        const CFB::COMPOUND_FILE_ENTRY* root = reader.GetRootEntry();

        // 'Library' stream: version, fonts, string table
        const CFB::COMPOUND_FILE_ENTRY* libraryEntry = cfbFile.FindStreamSingleLevel( root, "Library", true );

        if( !libraryEntry )
        {
            THROW_IO_ERROR( _( "The file does not contain the 'Library' stream of an OrCAD "
                               "Capture design." ) );
        }

        design.library = OrcadParseLibrary( readStream( cfbFile, libraryEntry ) );

        // Pre-2003 designs use pre-preamble framing; pages and the symbol cache each need
        // their own reader
        bool isV2 = design.library.versionMajor < 3;

        // 'Cache' stream: symbol defs and package pin maps
        if( const CFB::COMPOUND_FILE_ENTRY* cacheEntry = cfbFile.FindStreamSingleLevel( root, "Cache", true ) )
        {
            if( isV2 )
            {
                OrcadParseCacheV2( readStream( cfbFile, cacheEntry ), design.library.strings, warnFn, design.symbols,
                                   design.packages );
            }
            else
            {
                OrcadParseCache( readStream( cfbFile, cacheEntry ), design.library.strings, warnFn, design.symbols,
                                 design.packages );
            }
        }
        else
        {
            warnFn( _( "The design has no 'Cache' stream; placeholder symbols will be "
                       "synthesized for all parts." ) );
        }

        // 'Packages/<name>' streams: locally modified parts
        if( const CFB::COMPOUND_FILE_ENTRY* packagesStorage =
                    cfbFile.FindStreamSingleLevel( root, "Packages", false ) )
        {
            for( const auto& [streamName, entry] : enumChildren( cfbFile, packagesStorage, true ) )
            {
                std::map<std::string, ORCAD_SYMBOL_DEF> extraSymbols;
                std::map<std::string, ORCAD_PACKAGE>    extraPackages;

                try
                {
                    std::vector<char> data = readStream( cfbFile, entry );

                    if( isV2 || !isLongFramedPackageStream( data ) )
                    {
                        OrcadParseOlbPackageStreamV2( data, design.library.strings, extraSymbols, extraPackages,
                                                      design.library.versionMajor < 2 );
                    }
                    else
                    {
                        OrcadParsePackageStream( data, design.library.strings, extraSymbols, extraPackages );
                    }
                }
                catch( const IO_ERROR& e )
                {
                    // CFB entry names UTF-16 in container, UTF-8 here
                    warnFn( wxString::Format( _( "Package stream '%s' could not be parsed: %s" ),
                                              wxString::FromUTF8( streamName ), e.What() ) );
                    continue;
                }

                if( isV2 )
                    OrcadMergeSymbolGeneralProperties( design.symbols, extraSymbols );
                else
                    OrcadMergeCacheStreams( design.symbols, design.packages, std::move( extraSymbols ),
                                            std::move( extraPackages ) );
            }
        }

        // 'Views/<folder>': one storage per schematic folder
        const CFB::COMPOUND_FILE_ENTRY* viewsStorage = cfbFile.FindStreamSingleLevel( root, "Views", false );

        if( !viewsStorage )
        {
            THROW_IO_ERROR( _( "The file does not contain a 'Views' storage; it is not a "
                               "supported OrCAD Capture design." ) );
        }

        std::vector<std::string>                               folders;
        std::map<std::string, const CFB::COMPOUND_FILE_ENTRY*> folderEntries;

        for( const auto& [folderName, entry] : enumChildren( cfbFile, viewsStorage, false ) )
        {
            if( folderEntries.emplace( folderName, entry ).second )
                folders.push_back( folderName );
        }

        if( const CFB::COMPOUND_FILE_ENTRY* directoryEntry =
                    cfbFile.FindStreamSingleLevel( root, "Views Directory", true ) )
        {
            try
            {
                std::vector<std::string> visibleFolders;

                for( std::string folder : OrcadParseSchematicFolderOrder( readStream( cfbFile, directoryEntry ) ) )
                {
                    folder = OrcadNormalizeCfbName( folder );

                    if( folderEntries.count( folder )
                        && std::find( visibleFolders.begin(), visibleFolders.end(), folder ) == visibleFolders.end() )
                    {
                        visibleFolders.push_back( std::move( folder ) );
                    }
                }

                folders = std::move( visibleFolders );
            }
            catch( const IO_ERROR& e )
            {
                warnFn( wxString::Format( _( "The schematic folder directory could not be read (%s); all stored "
                                             "folders are imported." ),
                                          e.What() ) );
                std::sort( folders.begin(), folders.end() );
            }
        }
        else
        {
            std::sort( folders.begin(), folders.end() );
        }

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
        {
            // A missing or unmatched root name requires a warning because it changes the sheet hierarchy.
            if( design.library.schematicName.empty() )
            {
                warnFn( _( "The design does not name its root schematic; the first schematic "
                           "folder is used instead." ) );
            }
            else
            {
                warnFn( wxString::Format( _( "The design names '%s' as its root schematic, but no such "
                                             "folder is present; the first schematic folder is used "
                                             "instead." ),
                                          wxString::FromUTF8( design.library.schematicName ) ) );
            }

            rootFolder = folders.front();
        }

        design.name = design.library.schematicName.empty() ? rootFolder : design.library.schematicName;


        std::map<uint32_t, std::string> hierarchyLinks;

        auto parseFolderPages = [&]( const std::string& aFolderName, const CFB::COMPOUND_FILE_ENTRY* aFolderEntry,
                                     std::vector<ORCAD_RAW_PAGE>& aOutPages )
        {
            const CFB::COMPOUND_FILE_ENTRY* pagesStorage =
                    cfbFile.FindStreamSingleLevel( aFolderEntry, "Pages", false );

            std::vector<std::string>                               available;
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
                    std::vector<char> orderData = readStream( cfbFile, orderEntry );

                    for( const std::string& pageName : isV2 ? OrcadParsePageOrderV2( orderData, design.library.strings )
                                                            : OrcadParsePageOrder( orderData ) )
                    {
                        if( pageEntries.count( pageName )
                            && std::find( ordered.begin(), ordered.end(), pageName ) == ordered.end() )
                        {
                            ordered.push_back( pageName );
                        }
                    }

                    orderKnown = true;
                }
                catch( const IO_ERROR& e )
                {
                    warnFn( wxString::Format( _( "The page display order for schematic folder '%s' could not be "
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

            for( size_t pageIndex = 0; pageIndex < ordered.size(); ++pageIndex )
            {
                const std::string& pageName = ordered[pageIndex];

                try
                {
                    std::vector<char> pageData = readStream( cfbFile, pageEntries[pageName] );
                    ORCAD_RAW_PAGE   page = isV2 ? OrcadParsePageV2( pageData, design.library.strings, warnFn,
                                                                     design.library.versionMajor < 2 )
                                                       : OrcadParsePage( pageData, design.library.strings, warnFn );
                    page.sourcePageNumber = pageIndex + 1;
                    page.sourcePageCount = ordered.size();
                    aOutPages.push_back( std::move( page ) );
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

            design.occurrenceRoot = isV2 ? OrcadReadOccurrenceTreeV2( hierarchyData, design.library.strings )
                                         : OrcadReadOccurrenceTree( hierarchyData, design.library.strings, warnFn );

            // Block instance dbId -> child folder name, from occurrence tree
            std::function<void( const ORCAD_OCC_SCOPE& )> collectLinks = [&]( const ORCAD_OCC_SCOPE& aScope )
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
        // during conversion.
        std::map<std::string, std::string> folderByLowerName;

        for( const auto& folderEntry : folderEntries )
            folderByLowerName.emplace( lowerCopy( folderEntry.first ), folderEntry.first );

        for( const auto& [dbId, childName] : hierarchyLinks )
        {
            std::string key = lowerCopy( childName );

            if( key == lowerCopy( rootFolder ) || design.childFolderPages.count( key ) )
                continue;

            auto childIt = folderByLowerName.find( key );

            if( childIt != folderByLowerName.end() )
                parseFolderPages( childIt->second, folderEntries[childIt->second], design.childFolderPages[key] );
        }

        for( const std::string& folder : folders )
        {
            std::string key = lowerCopy( folder );

            if( key == lowerCopy( rootFolder ) || design.childFolderPages.count( key ) )
                continue;

            std::vector<ORCAD_RAW_PAGE>& pages = design.unreferencedFolderPages[key];
            parseFolderPages( folder, folderEntries[folder], pages );

            if( pages.empty() )
                design.unreferencedFolderPages.erase( key );
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

        applyCisVariant( cfbFile, root, aProperties, design, m_reporter );
    }
    catch( const CFB::CFBException& e )
    {
        THROW_IO_ERROR( e.what() );
    }

    ORCAD_CONVERTER converter( design, aSchematic, m_reporter, m_progressReporter );

    converter.Convert( rootSheet );

    aSchematic->Settings().m_ShowDNPMarkers = false;

    auto [dashRatio, gapRatio] = OrcadDashRatios( design.library.versionMajor );
    aSchematic->Settings().m_DashedLineDashRatio = dashRatio;
    aSchematic->Settings().m_DashedLineGapRatio = gapRatio;


    aSchematic->CurrentSheet().UpdateAllScreenReferences();
    aSchematic->FixupJunctionsAfterImport();
    assignPostImportUuids( aSchematic, sourceId );

    return rootSheet;
}


const std::vector<std::unique_ptr<LIB_SYMBOL>>& SCH_IO_ORCAD::loadOlbSymbols( const wxString& aLibraryPath )
{
    if( auto it = m_libCache.find( aLibraryPath ); it != m_libCache.end() )
        return it->second;

    ORCAD_WARN_FN warnFn = [this]( const wxString& aMsg )
    {
        if( m_reporter )
            m_reporter->Report( aMsg, RPT_SEVERITY_WARNING );
    };

    ORCAD_DESIGN design;

    try
    {
        ALTIUM_COMPOUND_FILE cfbFile( aLibraryPath );

        const CFB::CompoundFileReader&  reader = cfbFile.GetCompoundFileReader();
        const CFB::COMPOUND_FILE_ENTRY* root = reader.GetRootEntry();

        const CFB::COMPOUND_FILE_ENTRY* libraryEntry = cfbFile.FindStreamSingleLevel( root, "Library", true );

        if( !libraryEntry )
            THROW_IO_ERROR( _( "The file is not an OrCAD Capture library (no 'Library' stream)." ) );

        design.library = OrcadParseLibrary( readStream( cfbFile, libraryEntry ) );

        bool isV2 = design.library.versionMajor < 3;

        // Parse one stream, tolerating a single bad/oversized stream without aborting the
        // library. Modern streams use preamble-framed cache reader; v2.0 uses short-prefix readers
        bool shortDisplayProp = design.library.versionMajor < 2;

        auto parseStream = [&]( const CFB::COMPOUND_FILE_ENTRY* aEntry, const std::string& aStreamName,
                                bool aIsPackage, bool aIsCache = false )
        {
            std::map<std::string, ORCAD_SYMBOL_DEF> extraSymbols;
            std::map<std::string, ORCAD_PACKAGE>    extraPackages;

            try
            {
                std::vector<char> data = readStream( cfbFile, aEntry );

                if( aIsPackage && ( isV2 || !isLongFramedPackageStream( data ) ) )
                {
                    OrcadParseOlbPackageStreamV2( data, design.library.strings, extraSymbols, extraPackages,
                                                  shortDisplayProp );
                }
                else if( isV2 )
                {
                    OrcadParseOlbSymbolStreamV2( data, design.library.strings, extraSymbols, shortDisplayProp );
                }
                else if( aIsCache )
                {
                    OrcadParseCache( data, design.library.strings, warnFn, extraSymbols, extraPackages );
                }
                else if( aIsPackage )
                {
                    OrcadParsePackageStream( data, design.library.strings, extraSymbols, extraPackages );
                }
                else
                {
                    OrcadParseSymbolStream( data, design.library.strings, extraSymbols );
                }
            }
            catch( const std::exception& e )
            {
                // Single bad stream must not abort whole library, but a library that quietly
                // drops a part looks complete and is not.
                warnFn( wxString::Format( _( "The library stream '%s' could not be read and was skipped (%s)." ),
                                          wxString::FromUTF8( aStreamName ), wxString::FromUTF8( e.what() ) ) );
                return;
            }

            OrcadMergeCacheStreams( design.symbols, design.packages, std::move( extraSymbols ),
                                    std::move( extraPackages ) );
        };

        // Design 'Cache' usually empty in a library; read anyway for rare cached symbol
        if( !isV2 )
        {
            if( const CFB::COMPOUND_FILE_ENTRY* cacheEntry = cfbFile.FindStreamSingleLevel( root, "Cache", true ) )
            {
                parseStream( cacheEntry, "Cache", false, true );
            }
        }

        // Symbols and parts live one per stream under 'Symbols' and 'Packages' storages
        for( const char* storageName : { "Symbols", "Packages" } )
        {
            bool isPackage = std::string( storageName ) == "Packages";

            const CFB::COMPOUND_FILE_ENTRY* storage = cfbFile.FindStreamSingleLevel( root, storageName, false );

            if( storage )
            {
                for( const auto& [streamName, entry] : enumChildren( cfbFile, storage, true ) )
                {
                    // '$Types$' and similar helper streams are not symbol defs
                    if( !streamName.empty() && streamName.front() == '$' )
                        continue;

                    parseStream( entry, streamName, isPackage );
                }
            }
        }
    }
    catch( const CFB::CFBException& e )
    {
        THROW_IO_ERROR( e.what() );
    }
    catch( const IO_ERROR& )
    {
        // Reportable errors (e.g. pre-2003 version gate) propagate
        throw;
    }
    catch( const std::exception& e )
    {
        // Malformed Library stream can drive over-sized allocation; degrade to recovered symbols
        warnFn( wxString::Format( _( "The OrCAD library could not be fully parsed (%s); some "
                                     "symbols may be missing." ),
                                  wxString::FromUTF8( e.what() ) ) );
    }

    ORCAD_CONVERTER converter( design, nullptr, m_reporter, m_progressReporter );

    // Build fully before caching so mid-parse failure does not cache an empty library
    std::vector<std::unique_ptr<LIB_SYMBOL>> built;

    for( LIB_SYMBOL* symbol : converter.BuildSymbolLibrary() )
        built.emplace_back( symbol );

    return m_libCache[aLibraryPath] = std::move( built );
}


void SCH_IO_ORCAD::EnumerateSymbolLib( wxArrayString& aSymbolNameList, const wxString& aLibraryPath,
                                       const std::map<std::string, UTF8>* )
{
    for( const std::unique_ptr<LIB_SYMBOL>& symbol : loadOlbSymbols( aLibraryPath ) )
        aSymbolNameList.Add( symbol->GetName() );
}


void SCH_IO_ORCAD::EnumerateSymbolLib( std::vector<LIB_SYMBOL*>& aSymbolList, const wxString& aLibraryPath,
                                       const std::map<std::string, UTF8>* )
{
    for( const std::unique_ptr<LIB_SYMBOL>& symbol : loadOlbSymbols( aLibraryPath ) )
        aSymbolList.push_back( symbol.get() );
}


LIB_SYMBOL* SCH_IO_ORCAD::LoadSymbol( const wxString& aLibraryPath, const wxString& aAliasName,
                                      const std::map<std::string, UTF8>* )
{
    for( const std::unique_ptr<LIB_SYMBOL>& symbol : loadOlbSymbols( aLibraryPath ) )
    {
        if( symbol->GetName() == aAliasName )
            return symbol.get();
    }

    return nullptr;
}
