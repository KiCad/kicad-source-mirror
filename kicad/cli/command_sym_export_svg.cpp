/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mark Roszko <mark.roszko@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include "command_sym_export_svg.h"
#include <cli/exit_codes.h>
#include "jobs/job_sym_export_svg.h"
#include <kiface_base.h>
#include <layer_ids.h>
#include <string_utils.h>
#include <wx/crt.h>

#include <macros.h>
#include <wx/tokenzr.h>


#define ARG_NO_BACKGROUND_COLOR "--no-background-color"
#define ARG_SYMBOL "--symbol"
#define ARG_INC_HIDDEN_PINS "--include-hidden-pins"
#define ARG_INC_HIDDEN_FIELDS "--include-hidden-fields"


CLI::SYM_EXPORT_SVG_COMMAND::SYM_EXPORT_SVG_COMMAND() : COMMAND( "svg" )
{
    addCommonArgs( true, true, false, true );

    m_argParser.add_description( UTF8STDSTR( _( "Exports the symbol or entire symbol library "
                                                "to SVG" ) ) );

    m_argParser.add_argument( "-t", ARG_THEME )
            .default_value( std::string() )
            .help( UTF8STDSTR( _( "Color theme to use (will default to symbol editor "
                                  "settings)" ) ) )
            .metavar( "THEME_NAME" );

    m_argParser.add_argument( "-s", ARG_SYMBOL )
            .default_value( std::string() )
            .help( UTF8STDSTR( _( "Specific symbol to export within the library" ) ) )
            .metavar( "SYMBOL" );

    m_argParser.add_argument( ARG_BLACKANDWHITE )
            .help( UTF8STDSTR( _( ARG_BLACKANDWHITE_DESC ) ) )
            .flag();

    m_argParser.add_argument( ARG_INC_HIDDEN_PINS )
            .help( UTF8STDSTR( _( "Include hidden pins" ) ) )
            .flag();

    m_argParser.add_argument( ARG_INC_HIDDEN_FIELDS )
            .help( UTF8STDSTR( _( "Include hidden fields" ) ) )
            .flag();
}


int CLI::SYM_EXPORT_SVG_COMMAND::doPerform( KIWAY& aKiway )
{
    std::unique_ptr<JOB_SYM_EXPORT_SVG> svgJob = std::make_unique<JOB_SYM_EXPORT_SVG>();

    svgJob->m_libraryPath = m_argInput;
    svgJob->m_outputDirectory = m_argOutput;
    svgJob->m_blackAndWhite = m_argParser.get<bool>( ARG_BLACKANDWHITE );
    svgJob->m_symbol = From_UTF8( m_argParser.get<std::string>( ARG_SYMBOL ).c_str() );
    svgJob->m_includeHiddenFields = m_argParser.get<bool>( ARG_INC_HIDDEN_FIELDS );
    svgJob->m_includeHiddenPins = m_argParser.get<bool>( ARG_INC_HIDDEN_PINS );

    if( !wxFile::Exists( svgJob->m_libraryPath ) )
    {
        wxFprintf( stderr, _( "Symbol file does not exist or is not accessible\n" ) );
        return EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    svgJob->m_colorTheme = From_UTF8( m_argParser.get<std::string>( ARG_THEME ).c_str() );

    int exitCode = aKiway.ProcessJob( KIWAY::FACE_SCH, svgJob.get() );

    return exitCode;
}
