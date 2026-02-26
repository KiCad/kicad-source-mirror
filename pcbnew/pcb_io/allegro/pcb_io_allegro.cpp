/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright Quilter
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @brief Pcbnew PLUGIN for Allegro brd format.
 */

#include "pcb_io_allegro.h"

#include <board.h>
#include <reporter.h>
#include <fstream>
#include <io/io_utils.h>
#include <kiplatform/io.h>

#include <convert/allegro_parser.h>
#include <allegro_builder.h>

#include <core/profile.h>

#include <stdexcept>


static const wxChar* const traceAllegroPerf = wxT( "KICAD_ALLEGRO_PERF" );


static bool checkFileHeader( const wxString& aFileName )
{
    // Pre-v18 files contain the string "all" at offset 0xF8 (start of version string)
    static const std::vector<uint8_t> allegroVString = { 'a', 'l', 'l' };
    static const size_t               allegroVStringOffset = 0xf8;

    if( IO_UTILS::fileHasBinaryHeader( aFileName, allegroVString, allegroVStringOffset ) )
        return true;

    // Files processed by Cadence dbdoctor replace the version string at 0xF8 with a
    // database version string (e.g. "dbd..."), so the "all" check above fails.
    // Detect these by checking the magic number at offset 0. The upper two bytes
    // of the little-endian magic identify the major Allegro format family:
    //   0x0013 = v16.x, 0x0014 = v17.x, 0x0015 = v18+
    static const std::vector<uint8_t> v16Magic = { 0x13, 0x00 };
    static const std::vector<uint8_t> v17Magic = { 0x14, 0x00 };
    static const std::vector<uint8_t> v18Magic = { 0x15, 0x00 };
    static const size_t               magicMajorOffset = 2;

    if( IO_UTILS::fileHasBinaryHeader( aFileName, v16Magic, magicMajorOffset ) )
        return true;

    if( IO_UTILS::fileHasBinaryHeader( aFileName, v17Magic, magicMajorOffset ) )
        return true;

    return IO_UTILS::fileHasBinaryHeader( aFileName, v18Magic, magicMajorOffset );
}


static std::map<wxString, PCB_LAYER_ID>
allegroDefaultLayerMappingCallback( const std::vector<INPUT_LAYER_DESC>& aInputLayerDescriptionVector )
{
    std::map<wxString, PCB_LAYER_ID> retval;

    for( const INPUT_LAYER_DESC& layerDesc : aInputLayerDescriptionVector )
        retval.insert( { layerDesc.Name, layerDesc.AutoMapLayer } );

    return retval;
}


PCB_IO_ALLEGRO::PCB_IO_ALLEGRO() : PCB_IO( wxS( "Allegro" ) )
{
    m_reporter = &WXLOG_REPORTER::GetInstance();

    LAYER_MAPPABLE_PLUGIN::RegisterCallback( allegroDefaultLayerMappingCallback );
}


bool PCB_IO_ALLEGRO::CanReadBoard( const wxString& aFileName ) const
{
    if( !PCB_IO::CanReadBoard( aFileName ) )
        return false;

    return checkFileHeader( aFileName );
}


bool PCB_IO_ALLEGRO::CanReadLibrary( const wxString& aFileName ) const
{
    if( !PCB_IO::CanReadLibrary( aFileName ) )
        return false;

    return false;
}


BOARD* PCB_IO_ALLEGRO::LoadBoard( const wxString& aFileName, BOARD* aAppendToMe,
                                  const std::map<std::string, UTF8>* aProperties, PROJECT* aProject )
{
    m_props = aProperties;
    m_board = aAppendToMe ? aAppendToMe : new BOARD();

    if( !aAppendToMe )
        m_board->SetFileName( aFileName );

    std::unique_ptr<BOARD> deleter( aAppendToMe ? nullptr : m_board );

    std::unique_ptr<KIPLATFORM::IO::MAPPED_FILE> mappedFile;

    try
    {
        mappedFile = std::make_unique<KIPLATFORM::IO::MAPPED_FILE>( aFileName );
    }
    catch( const std::runtime_error& e )
    {
        THROW_IO_ERROR( wxString::Format( wxS( "%s" ), e.what() ) );
    }

    if( !mappedFile->Data() || mappedFile->Size() == 0 )
        THROW_IO_ERROR( wxString::Format( wxS( "File is empty: %s" ), aFileName ) );

    if( !LoadBoardFromData( mappedFile->Data(), mappedFile->Size(), *m_board ) )
        return nullptr;

    (void) deleter.release();
    return m_board;
}


bool PCB_IO_ALLEGRO::LoadBoardFromData( const uint8_t* aData, size_t aSize, BOARD& aBoard )
{
    ALLEGRO::FILE_STREAM allegroStream( aData, aSize );

    ALLEGRO::PARSER parser( allegroStream, m_progressReporter );

    // When parsing a file "for real", encountering an unknown block is fatal, as we then
    // cannot know how long that block is, and thus can't proceed to find any later blocks.
    parser.EndAtUnknownBlock( false );

    PROF_TIMER totalTimer;

    wxLogTrace( traceAllegroPerf, wxT( "=== Allegro Import Performance ===" ) );

    // Import phase 1: turn the file into the C++ structs
    PROF_TIMER phaseTimer;
    std::unique_ptr<ALLEGRO::BRD_DB> brdDb = parser.Parse();
    phaseTimer.Stop();

    wxLogTrace( traceAllegroPerf, wxT( "Phase 1 (binary parse): %.3f ms" ), phaseTimer.msecs() ); //format:allow

    // Import Phase 2: turn the C++ structs into the KiCad BOARD
    ALLEGRO::BOARD_BUILDER builder( *brdDb, aBoard, *m_reporter, m_progressReporter, m_layer_mapping_handler );

    phaseTimer.Start();
    const bool phase2Ok = builder.BuildBoard();
    phaseTimer.Stop();

    wxLogTrace( traceAllegroPerf, wxT( "Phase 2 (board construction): %.3f ms" ), phaseTimer.msecs() ); //format:allow

    if( !phase2Ok )
    {
        wxLogTrace( wxT( "KICAD_ALLEGRO" ), "Phase 2 board construction failed" );
        m_reporter->Report( _( "Failed to build board from Allegro data" ), RPT_SEVERITY_ERROR );
        return false;
    }

    wxLogTrace( wxT( "KICAD_ALLEGRO" ), "Board construction completed successfully" );
    wxLogTrace( traceAllegroPerf, wxT( "LoadBoard total (Phase 1 + Phase 2): %.3f ms" ), totalTimer.msecs() ); //format:allow

    aBoard.m_LegacyNetclassesLoaded = true;
    aBoard.m_LegacyDesignSettingsLoaded = true;

    return true;
}
