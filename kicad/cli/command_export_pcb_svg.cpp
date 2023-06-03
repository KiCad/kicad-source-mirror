/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
 * Copyright (C) 1992-2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "command_export_pcb_svg.h"
#include <cli/exit_codes.h>
#include "command_export_pcb_base.h"
#include "jobs/job_export_pcb_svg.h"
#include <kiface_base.h>
#include <layer_ids.h>
#include <regex>
#include <wx/crt.h>

#include <macros.h>
#include <wx/tokenzr.h>

#define ARG_EXCLUDE_DRAWING_SHEET "--exclude-drawing-sheet"
#define ARG_PAGE_SIZE "--page-size-mode"


CLI::EXPORT_PCB_SVG_COMMAND::EXPORT_PCB_SVG_COMMAND() : EXPORT_PCB_BASE_COMMAND( "svg" )
{
    addLayerArg( true );

    m_argParser.add_argument( "-m", ARG_MIRROR )
            .help( UTF8STDSTR( _( "Mirror the board (useful for trying to show bottom layers)" ) ) )
            .implicit_value( true )
            .default_value( false );

    m_argParser.add_argument( "-t", ARG_THEME )
            .default_value( std::string() )
            .help( UTF8STDSTR( _( "Color theme to use (will default to pcbnew settings)" ) ) );

    m_argParser.add_argument( ARG_NEGATIVE_SHORT, ARG_NEGATIVE )
            .help( UTF8STDSTR( _( ARG_NEGATIVE_DESC ) ) )
            .implicit_value( true )
            .default_value( false );

    m_argParser.add_argument( ARG_BLACKANDWHITE )
            .help( UTF8STDSTR( _( ARG_BLACKANDWHITE_DESC ) ) )
            .implicit_value( true )
            .default_value( false );

    m_argParser.add_argument( ARG_PAGE_SIZE )
            .help( UTF8STDSTR( _( "Set page sizing mode (0 = page with frame and title block, 1 = "
                                "current page size, 2 = board area only)" ) ) )
            .scan<'i', int>()
            .default_value( 0 );

    m_argParser.add_argument( ARG_EXCLUDE_DRAWING_SHEET )
            .help( UTF8STDSTR( _( "No drawing sheet" ) ) )
            .implicit_value( true )
            .default_value( false );
}


int CLI::EXPORT_PCB_SVG_COMMAND::doPerform( KIWAY& aKiway )
{
    int baseExit = EXPORT_PCB_BASE_COMMAND::doPerform( aKiway );
    if( baseExit != EXIT_CODES::OK )
        return baseExit;

    std::unique_ptr<JOB_EXPORT_PCB_SVG> svgJob( new JOB_EXPORT_PCB_SVG( true ) );

    svgJob->m_mirror = m_argParser.get<bool>( ARG_MIRROR );
    svgJob->m_blackAndWhite = m_argParser.get<bool>( ARG_BLACKANDWHITE );
    svgJob->m_pageSizeMode = m_argParser.get<int>( ARG_PAGE_SIZE );
    svgJob->m_negative = m_argParser.get<bool>( ARG_NEGATIVE );

    svgJob->m_filename = FROM_UTF8( m_argParser.get<std::string>( ARG_INPUT ).c_str() );
    svgJob->m_outputFile = FROM_UTF8( m_argParser.get<std::string>( ARG_OUTPUT ).c_str() );
    svgJob->m_colorTheme = FROM_UTF8( m_argParser.get<std::string>( ARG_THEME ).c_str() );
    svgJob->m_plotDrawingSheet = !m_argParser.get<bool>( ARG_EXCLUDE_DRAWING_SHEET );

    if( !wxFile::Exists( svgJob->m_filename ) )
    {
        wxFprintf( stderr, _( "Board file does not exist or is not accessible\n" ) );
        return EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    svgJob->m_printMaskLayer = m_selectedLayers;

    int exitCode = aKiway.ProcessJob( KIWAY::FACE_PCB, svgJob.get() );

    return exitCode;
}
