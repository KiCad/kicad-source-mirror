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
 * @brief Pcbnew PLUGIN for Allegro *.brd and *. format.
 */

#include "pcb_io_allegro.h"

#include <board.h>
#include <reporter.h>
#include <io/io_utils.h>

#include <convert/allegro_parser.h>
#include <allegro_builder.h>

#include <core/profile.h>


static bool checkFileHeader( const wxString& aFileName )
{
    // The files start with a version string that can vary (a bit),
    // But the header seems always to contain the string "all" at the
    // start of the version string at 0xF8.
    static const std::vector<uint8_t> allegroVString = { 'a', 'l', 'l' };
    static const size_t               allegroVStringOffset = 0xf8;

    return IO_UTILS::fileHasBinaryHeader( aFileName, allegroVString, allegroVStringOffset );
}


static std::map<wxString, PCB_LAYER_ID>
allegroDefaultLayerMappingCallback( const std::vector<INPUT_LAYER_DESC>& aInputLayerDescriptionVector )
{
    std::map<wxString, PCB_LAYER_ID> retval;

    // Just return the auto-mapped layers
    for( const INPUT_LAYER_DESC& layerDesc : aInputLayerDescriptionVector )
    {
        retval.insert( { layerDesc.Name, layerDesc.AutoMapLayer } );
    }

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

    std::ifstream fin( aFileName, std::ios::binary );

    ALLEGRO::FILE_STREAM allegroStream( fin );

    ALLEGRO::PARSER parser( allegroStream, m_progressReporter );

    // When parsing a file "for real", encountering an unknown block is fatal, as we then
    // cannot know how long that block is, and thus can't proceed to find any later blocks.
    parser.EndAtUnknownBlock( false );

    PROF_TIMER timer( "allegro_file_parse" );

    // Import phase 1: turn the file into the C++ structs
    std::unique_ptr<ALLEGRO::RAW_BOARD> rawBoard = parser.Parse();

    m_reporter->Report( wxString::Format( "Phase 1 parse took %fms", timer.msecs() ), RPT_SEVERITY_DEBUG ); // format:allow
    // Import Phase 2: turn the C++ structs into the KiCad BOARD
    ALLEGRO::BOARD_BUILDER builder( *rawBoard, *m_board, *m_reporter, m_progressReporter );

    const bool phase2Ok = builder.BuildBoard();

    m_reporter->Report( wxString::Format( "Phase 2 parse took %fms", timer.msecs( true ) ), RPT_SEVERITY_DEBUG ); // format:allow

    return m_board;
}
