/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mark Roszko <mark.roszko@gmail.com>
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

#include "command_export_sch_pdf.h"
#include <cli/exit_codes.h>
#include "jobs/job_export_sch_pdf.h"
#include <kiface_base.h>
#include <layer_ids.h>
#include <wx/crt.h>

#include <macros.h>
#include <wx/tokenzr.h>

#define ARG_EXCLUDE_DRAWING_SHEET "--exclude-drawing-sheet"
#define ARG_NO_BACKGROUND_COLOR "--no-background-color"

CLI::EXPORT_SCH_PDF_COMMAND::EXPORT_SCH_PDF_COMMAND() : EXPORT_PCB_BASE_COMMAND( "pdf" )
{
    m_argParser.add_argument( "-t", ARG_THEME )
            .default_value( std::string() )
            .help( UTF8STDSTR( _( "Color theme to use (will default to pcbnew settings)" ) ) );

    m_argParser.add_argument( ARG_BLACKANDWHITE )
            .help( UTF8STDSTR( _( "Black and white only" ) ) )
            .implicit_value( true )
            .default_value( false );

    m_argParser.add_argument( ARG_EXCLUDE_DRAWING_SHEET )
            .help( UTF8STDSTR( _( "No drawing sheet" ) ) )
            .implicit_value( true )
            .default_value( false );

    m_argParser.add_argument( ARG_NO_BACKGROUND_COLOR )
            .help( UTF8STDSTR( _( "Avoid setting a background color (regardless of theme)" ) ) )
            .implicit_value( true )
            .default_value( false );
}


int CLI::EXPORT_SCH_PDF_COMMAND::doPerform( KIWAY& aKiway )
{
    std::unique_ptr<JOB_EXPORT_SCH_PDF> pdfJob = std::make_unique<JOB_EXPORT_SCH_PDF>( true );

    pdfJob->m_filename = FROM_UTF8( m_argParser.get<std::string>( ARG_INPUT ).c_str() );
    pdfJob->m_outputFile = FROM_UTF8( m_argParser.get<std::string>( ARG_OUTPUT ).c_str() );
    pdfJob->m_blackAndWhite = m_argParser.get<bool>( ARG_BLACKANDWHITE );
    pdfJob->m_plotDrawingSheet = !m_argParser.get<bool>( ARG_EXCLUDE_DRAWING_SHEET );
    pdfJob->m_useBackgroundColor = !m_argParser.get<bool>( ARG_NO_BACKGROUND_COLOR );

    if( !wxFile::Exists( pdfJob->m_filename ) )
    {
        wxFprintf( stderr, _( "Schematic file does not exist or is not accessible\n" ) );
        return EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    pdfJob->m_colorTheme = FROM_UTF8( m_argParser.get<std::string>( ARG_THEME ).c_str() );

    int exitCode = aKiway.ProcessJob( KIWAY::FACE_SCH, pdfJob.get() );

    return exitCode;
}