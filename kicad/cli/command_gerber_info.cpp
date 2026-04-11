/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "command_gerber_info.h"
#include <cli/exit_codes.h>
#include <jobs/job_gerber_info.h>
#include <kiface_base.h>
#include <string_utils.h>
#include <macros.h>
#include <wx/crt.h>


#define ARG_FORMAT "--format"
#define ARG_UNITS "--units"
#define ARG_CALCULATE_AREA "--area"
#define ARG_STRICT "--strict"


CLI::GERBER_INFO_COMMAND::GERBER_INFO_COMMAND() :
        COMMAND( "info" )
{
    addCommonArgs( true, false, IO_TYPE::FILE, IO_TYPE::FILE );

    m_argParser.add_description( UTF8STDSTR( _( "Display information about a Gerber or Excellon file" ) ) );

    m_argParser.add_argument( ARG_FORMAT )
            .default_value( std::string( "text" ) )
            .help( UTF8STDSTR( _( "Output format, options: text, json" ) ) )
            .metavar( "FORMAT" );

    m_argParser.add_argument( ARG_UNITS )
            .default_value( std::string( "mm" ) )
            .help( UTF8STDSTR( _( "Output units, options: mm, inch, mils (default: mm)" ) ) )
            .metavar( "UNITS" );

    m_argParser.add_argument( ARG_CALCULATE_AREA )
            .help( UTF8STDSTR( _( "Calculate and display copper area" ) ) )
            .flag();

    m_argParser.add_argument( ARG_STRICT ).help( UTF8STDSTR( _( "Fail on any parse warnings or errors" ) ) ).flag();
}


int CLI::GERBER_INFO_COMMAND::doPerform( KIWAY& aKiway )
{
    std::unique_ptr<JOB_GERBER_INFO> infoJob = std::make_unique<JOB_GERBER_INFO>();

    infoJob->m_inputFile = m_argInput;
    infoJob->m_calculateArea = m_argParser.get<bool>( ARG_CALCULATE_AREA );
    infoJob->m_strict = m_argParser.get<bool>( ARG_STRICT );

    wxString format = From_UTF8( m_argParser.get<std::string>( ARG_FORMAT ).c_str() );

    if( format == wxS( "text" ) )
    {
        infoJob->m_outputFormat = JOB_GERBER_INFO::OUTPUT_FORMAT::TEXT;
    }
    else if( format == wxS( "json" ) )
    {
        infoJob->m_outputFormat = JOB_GERBER_INFO::OUTPUT_FORMAT::JSON;
    }
    else
    {
        wxFprintf( stderr, _( "Invalid output format: %s\n" ), format );
        return EXIT_CODES::ERR_ARGS;
    }

    wxString units = From_UTF8( m_argParser.get<std::string>( ARG_UNITS ).c_str() );

    if( units == wxS( "mm" ) )
    {
        infoJob->m_units = JOB_GERBER_INFO::UNITS::MM;
    }
    else if( units == wxS( "inch" ) )
    {
        infoJob->m_units = JOB_GERBER_INFO::UNITS::INCH;
    }
    else if( units == wxS( "mils" ) )
    {
        infoJob->m_units = JOB_GERBER_INFO::UNITS::MILS;
    }
    else
    {
        wxFprintf( stderr, _( "Invalid units: %s\n" ), units );
        return EXIT_CODES::ERR_ARGS;
    }

    int exitCode = aKiway.ProcessJob( KIWAY::FACE_GERBVIEW, infoJob.get() );

    return exitCode;
}
