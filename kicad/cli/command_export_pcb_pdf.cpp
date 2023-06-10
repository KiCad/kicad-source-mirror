/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 1992-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "command_export_pcb_pdf.h"
#include <cli/exit_codes.h>
#include "jobs/job_export_pcb_pdf.h"
#include <kiface_base.h>
#include <layer_ids.h>
#include <wx/crt.h>

#include <macros.h>
#include <wx/tokenzr.h>

#include <locale_io.h>


CLI::EXPORT_PCB_PDF_COMMAND::EXPORT_PCB_PDF_COMMAND() : EXPORT_PCB_BASE_COMMAND( "pdf" )
{
    addLayerArg( true );

    m_argParser.add_argument( "-m", ARG_MIRROR )
            .help( UTF8STDSTR( _( "Mirror the board (useful for trying to show bottom layers)" ) ) )
            .implicit_value( true )
            .default_value( false );

    m_argParser.add_argument( "--erd", ARG_EXCLUDE_REFDES )
            .help( UTF8STDSTR( _( "Exclude the reference designator text" ) ) )
            .implicit_value( true )
            .default_value( false );

    m_argParser.add_argument( "--ev", ARG_EXCLUDE_VALUE )
            .help( UTF8STDSTR( _( "Exclude the value text" ) ) )
            .implicit_value( true )
            .default_value( false );

    m_argParser.add_argument( "--ibt", ARG_INCLUDE_BORDER_TITLE )
            .help( UTF8STDSTR( _( "Include the border and title block" ) ) )
            .implicit_value( true )
            .default_value( false );

    m_argParser.add_argument( ARG_NEGATIVE_SHORT, ARG_NEGATIVE )
            .help( UTF8STDSTR( _( ARG_NEGATIVE_DESC ) ) )
            .implicit_value( true )
            .default_value( false );

    m_argParser.add_argument( ARG_BLACKANDWHITE )
            .help( UTF8STDSTR( _( ARG_BLACKANDWHITE_DESC ) ) )
            .implicit_value( true )
            .default_value( false );

    m_argParser.add_argument( "-t", ARG_THEME )
            .default_value( std::string() )
            .help( UTF8STDSTR( _( "Color theme to use (will default to PCB Editor settings)" ) ) );
}


int CLI::EXPORT_PCB_PDF_COMMAND::doPerform( KIWAY& aKiway )
{
    int baseExit = EXPORT_PCB_BASE_COMMAND::doPerform( aKiway );

    if( baseExit != EXIT_CODES::OK )
        return baseExit;

    std::unique_ptr<JOB_EXPORT_PCB_PDF> pdfJob( new JOB_EXPORT_PCB_PDF( true ) );

    pdfJob->m_filename = FROM_UTF8( m_argParser.get<std::string>( ARG_INPUT ).c_str() );
    pdfJob->m_outputFile = FROM_UTF8( m_argParser.get<std::string>( ARG_OUTPUT ).c_str() );

    if( !wxFile::Exists( pdfJob->m_filename ) )
    {
        wxFprintf( stderr, _( "Board file does not exist or is not accessible\n" ) );
        return EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    pdfJob->m_plotFootprintValues = !m_argParser.get<bool>( ARG_EXCLUDE_VALUE );
    pdfJob->m_plotRefDes = !m_argParser.get<bool>( ARG_EXCLUDE_REFDES );

    pdfJob->m_plotBorderTitleBlocks = m_argParser.get<bool>( ARG_INCLUDE_BORDER_TITLE );

    pdfJob->m_mirror = m_argParser.get<bool>( ARG_MIRROR );
    pdfJob->m_blackAndWhite = m_argParser.get<bool>( ARG_BLACKANDWHITE );
    pdfJob->m_colorTheme = FROM_UTF8( m_argParser.get<std::string>( ARG_THEME ).c_str() );
    pdfJob->m_negative = m_argParser.get<bool>( ARG_NEGATIVE );

    pdfJob->m_printMaskLayer = m_selectedLayers;

    LOCALE_IO dummy;    // Switch to "C" locale
    int exitCode = aKiway.ProcessJob( KIWAY::FACE_PCB, pdfJob.get() );

    return exitCode;
}
