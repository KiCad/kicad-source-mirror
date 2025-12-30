/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Thomas Pointhuber <thomas.pointhuber@gmx.at>
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

/**
 * @brief Pcbnew PLUGIN for Altium *.PcbDoc format.
 */

#include <wx/string.h>

#include <font/fontconfig.h>
#include <pcb_io_altium_designer.h>
#include <altium_pcb.h>
#include <altium_pcb_compound_file.h>
#include <io/io_utils.h>
#include <io/altium/altium_binary_parser.h>
#include <pcb_io/pcb_io.h>
#include <reporter.h>

#include <board.h>

#include <compoundfilereader.h>
#include <utf.h>

PCB_IO_ALTIUM_DESIGNER::PCB_IO_ALTIUM_DESIGNER() :
        PCB_IO( wxS( "Altium Designer" ) )
{
    m_reporter = &WXLOG_REPORTER::GetInstance();

    RegisterCallback( PCB_IO_ALTIUM_DESIGNER::DefaultLayerMappingCallback );
}


PCB_IO_ALTIUM_DESIGNER::~PCB_IO_ALTIUM_DESIGNER()
{
}


std::map<wxString, PCB_LAYER_ID> PCB_IO_ALTIUM_DESIGNER::DefaultLayerMappingCallback(
        const std::vector<INPUT_LAYER_DESC>& aInputLayerDescriptionVector )
{
    std::map<wxString, PCB_LAYER_ID> retval;

    // Just return a the auto-mapped layers
    for( INPUT_LAYER_DESC layerDesc : aInputLayerDescriptionVector )
    {
        retval.insert( { layerDesc.Name, layerDesc.AutoMapLayer } );
    }

    return retval;
}


bool PCB_IO_ALTIUM_DESIGNER::checkFileHeader( const wxString& aFileName )
{
    // Compound File Binary Format header
    return IO_UTILS::fileStartsWithBinaryHeader( aFileName, IO_UTILS::COMPOUND_FILE_HEADER );
}


bool PCB_IO_ALTIUM_DESIGNER::CanReadBoard( const wxString& aFileName ) const
{
    if( !PCB_IO::CanReadBoard( aFileName ) )
        return false;

    return checkFileHeader( aFileName );
}


bool PCB_IO_ALTIUM_DESIGNER::CanReadLibrary( const wxString& aFileName ) const
{
    if( !PCB_IO::CanReadLibrary( aFileName ) )
        return false;

    return checkFileHeader( aFileName );
}


BOARD* PCB_IO_ALTIUM_DESIGNER::LoadBoard( const wxString& aFileName, BOARD* aAppendToMe,
                                          const std::map<std::string, UTF8>* aProperties, PROJECT* aProject )
{

    m_props = aProperties;

    m_board = aAppendToMe ? aAppendToMe : new BOARD();

    // Collect the font substitution warnings (RAII - automatically reset on scope exit)
    FONTCONFIG_REPORTER_SCOPE fontconfigScope( &LOAD_INFO_REPORTER::GetInstance() );

    // Give the filename to the board if it's new
    if( !aAppendToMe )
        m_board->SetFileName( aFileName );

    // clang-format off
    const std::map<ALTIUM_PCB_DIR, std::string> mapping = {
            { ALTIUM_PCB_DIR::FILE_HEADER, "FileHeader" },
            { ALTIUM_PCB_DIR::ARCS6, "Arcs6" },
            { ALTIUM_PCB_DIR::BOARD6, "Board6" },
            { ALTIUM_PCB_DIR::BOARDREGIONS, "BoardRegions" },
            { ALTIUM_PCB_DIR::CLASSES6, "Classes6" },
            { ALTIUM_PCB_DIR::COMPONENTS6, "Components6" },
            { ALTIUM_PCB_DIR::COMPONENTBODIES6, "ComponentBodies6" },
            { ALTIUM_PCB_DIR::DIMENSIONS6, "Dimensions6" },
            { ALTIUM_PCB_DIR::EXTENDPRIMITIVEINFORMATION, "ExtendedPrimitiveInformation" },
            { ALTIUM_PCB_DIR::FILLS6, "Fills6" },
            { ALTIUM_PCB_DIR::MODELS, "Models" },
            { ALTIUM_PCB_DIR::NETS6, "Nets6" },
            { ALTIUM_PCB_DIR::PADS6, "Pads6" },
            { ALTIUM_PCB_DIR::POLYGONS6, "Polygons6" },
            { ALTIUM_PCB_DIR::REGIONS6, "Regions6" },
            { ALTIUM_PCB_DIR::RULES6, "Rules6" },
            { ALTIUM_PCB_DIR::SHAPEBASEDREGIONS6, "ShapeBasedRegions6" },
            { ALTIUM_PCB_DIR::TEXTS6, "Texts6" },
            { ALTIUM_PCB_DIR::TRACKS6, "Tracks6" },
            { ALTIUM_PCB_DIR::VIAS6, "Vias6" },
            { ALTIUM_PCB_DIR::WIDESTRINGS6, "WideStrings6" }
    };
    // clang-format on

    ALTIUM_PCB_COMPOUND_FILE altiumPcbFile( aFileName );

    try
    {
        // Parse File
        ALTIUM_PCB pcb( m_board, m_progressReporter, m_layer_mapping_handler, m_reporter );
        pcb.Parse( altiumPcbFile, mapping );
    }
    catch( CFB::CFBException& exception )
    {
        THROW_IO_ERROR( exception.what() );
    }

    return m_board;
}


long long PCB_IO_ALTIUM_DESIGNER::GetLibraryTimestamp( const wxString& aLibraryPath ) const
{
    // File hasn't been loaded yet.
    if( aLibraryPath.IsEmpty() )
        return 0;

    wxFileName fn( aLibraryPath );

    if( fn.IsFileReadable() && fn.GetModificationTime().IsValid() )
        return fn.GetModificationTime().GetValue().GetValue();
    else
        return 0;
}


void PCB_IO_ALTIUM_DESIGNER::loadAltiumLibrary( const wxString& aLibraryPath )
{
    // Suppress font substitution warnings (RAII - automatically restored on scope exit)
    FONTCONFIG_REPORTER_SCOPE fontconfigScope( nullptr );

    long long timestamp = GetLibraryTimestamp( aLibraryPath );

    if( m_fplibFiles.contains( aLibraryPath ) && m_fplibFiles[aLibraryPath].m_Timestamp == timestamp )
        return; // Already loaded

    try
    {
        ALTIUM_FILE_CACHE& libFiles = m_fplibFiles[aLibraryPath];

        if( aLibraryPath.Lower().EndsWith( wxS( ".pcblib" ) ) )
        {
            libFiles.m_Files.emplace_back( std::make_unique<ALTIUM_PCB_COMPOUND_FILE>( aLibraryPath ) );
        }
        else if( aLibraryPath.Lower().EndsWith( wxS( ".intlib" ) ) )
        {
            std::unique_ptr<ALTIUM_PCB_COMPOUND_FILE> lib = std::make_unique<ALTIUM_PCB_COMPOUND_FILE>( aLibraryPath );

            for( const auto& [pcbLibName, pcbCfe] : lib->EnumDir( L"PCBLib" ) )
            {
                std::unique_ptr<ALTIUM_PCB_COMPOUND_FILE> libFile = std::make_unique<ALTIUM_PCB_COMPOUND_FILE>();

                if( lib->DecodeIntLibStream( *pcbCfe, libFile.get() ) )
                    libFiles.m_Files.emplace_back( std::move( libFile ) );
            }
        }

        libFiles.m_Timestamp = timestamp;
    }
    catch( CFB::CFBException& exception )
    {
        THROW_IO_ERROR( exception.what() );
    }
}


void PCB_IO_ALTIUM_DESIGNER::FootprintEnumerate( wxArrayString&  aFootprintNames,
                                                 const wxString& aLibraryPath, bool aBestEfforts,
                                                 const std::map<std::string, UTF8>* aProperties )
{
    loadAltiumLibrary( aLibraryPath );

    if( !m_fplibFiles.contains( aLibraryPath ) )
        return; // No footprint libraries in file, ignore.

    try
    {
        for( std::unique_ptr<ALTIUM_PCB_COMPOUND_FILE>& altiumLibFile : m_fplibFiles[aLibraryPath].m_Files )
        {
            // Map code-page-dependent names to unicode names
            CASE_INSENSITIVE_MAP<wxString> patternMap = altiumLibFile->ListLibFootprints();

            const std::vector<std::string>  streamName = { "Library", "Data" };
            const CFB::COMPOUND_FILE_ENTRY* libraryData = altiumLibFile->FindStream( streamName );

            if( libraryData == nullptr )
            {
                THROW_IO_ERROR( wxString::Format( _( "File not found: '%s'." ),
                                                  FormatPath( streamName ) ) );
            }

            ALTIUM_BINARY_PARSER parser( *altiumLibFile, libraryData );

            std::map<wxString, wxString> properties = parser.ReadProperties();

            uint32_t numberOfFootprints = parser.Read<uint32_t>();

            // The number of footprints might be bogus. Following it is a list of index strings to
            // find the footprints, and this list might actually be longer than how many footprints
            // actually are in the library.
            // If this case was detected, truncate the list to the number of footprints.
            // Sample in #18452 shows that by simply truncating can get the correct footprints.
            // Fixes https://gitlab.com/kicad/code/kicad/issues/18452.
            // After truncation we shall no longer detect if the stream is fully parsed.
            bool footprintListNotTruncated = true;

            if ( patternMap.size() < numberOfFootprints )
            {
                numberOfFootprints = patternMap.size();
                footprintListNotTruncated = false;
            }

            aFootprintNames.Alloc( numberOfFootprints );

            for( size_t i = 0; i < numberOfFootprints; i++ )
            {
                parser.ReadAndSetSubrecordLength();

                wxScopedCharBuffer charBuffer = parser.ReadCharBuffer();
                wxString           fpPattern( charBuffer, wxConvISO8859_1 );

                auto it = patternMap.find( fpPattern );

                if( it != patternMap.end() )
                {
                    aFootprintNames.Add( it->second ); // Proper unicode name
                }
                else
                {
                    THROW_IO_ERROR( wxString::Format( "Component name not found: '%s'", fpPattern ) );
                }

                parser.SkipSubrecord();
            }

            if( parser.HasParsingError() )
            {
                THROW_IO_ERROR( wxString::Format( "%s stream was not parsed correctly",
                                                  FormatPath( streamName ) ) );
            }

            if( footprintListNotTruncated && parser.GetRemainingBytes() != 0 )
            {
                THROW_IO_ERROR( wxString::Format( "%s stream is not fully parsed",
                                                  FormatPath( streamName ) ) );
            }
        }
    }
    catch( CFB::CFBException& exception )
    {
        THROW_IO_ERROR( exception.what() );
    }
}


FOOTPRINT* PCB_IO_ALTIUM_DESIGNER::FootprintLoad( const wxString& aLibraryPath,
                                                  const wxString& aFootprintName, bool aKeepUUID,
                                                  const std::map<std::string, UTF8>* aProperties )
{
    loadAltiumLibrary( aLibraryPath );

    if( !m_fplibFiles.contains( aLibraryPath ) )
        THROW_IO_ERROR( wxString::Format( _( "No footprints in library '%s'" ), aLibraryPath ) );

    try
    {
        for( std::unique_ptr<ALTIUM_PCB_COMPOUND_FILE>& altiumLibFile : m_fplibFiles[aLibraryPath].m_Files )
        {
            altiumLibFile->CacheLibModels();
            auto [dirName, fpCfe] = altiumLibFile->FindLibFootprintDirName( aFootprintName );

            if( dirName.IsEmpty() )
                continue;

            // Parse File
            ALTIUM_PCB pcb( m_board, nullptr, m_layer_mapping_handler, m_reporter, aLibraryPath, aFootprintName );
            return pcb.ParseFootprint( *altiumLibFile, aFootprintName );
        }
    }
    catch( CFB::CFBException& exception )
    {
        THROW_IO_ERROR( exception.what() );
    }

    THROW_IO_ERROR( wxString::Format( _( "Footprint '%s' not found in '%s'." ),
                                      aFootprintName,
                                      aLibraryPath ) );
}


std::vector<FOOTPRINT*> PCB_IO_ALTIUM_DESIGNER::GetImportedCachedLibraryFootprints()
{
    std::vector<FOOTPRINT*> footprints;

    for( FOOTPRINT* fp : m_board->Footprints() )
        footprints.push_back( fp );

    return footprints;
}
