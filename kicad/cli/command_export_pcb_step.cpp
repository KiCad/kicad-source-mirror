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

#include "command_export_pcb_step.h"
#include <cli/exit_codes.h>
#include "jobs/job_export_pcb_step.h"
#include <kiface_base.h>
#include <regex>
#include <locale_io.h>

#include <macros.h>

#define ARG_DRILL_ORIGIN "--drill-origin"
#define ARG_GRID_ORIGIN "--grid-origin"
#define ARG_NO_UNSPECIFIED "--no-unspecified"
#define ARG_NO_DNP "--no-dnp"
#define ARG_SUBST_MODELS "--subst-models"
#define ARG_FORCE "--force"
#define ARG_OUTPUT "--output"
#define ARG_INPUT "input"
#define ARG_MIN_DISTANCE "--min-distance"
#define ARG_USER_ORIGIN "--user-origin"
#define ARG_BOARD_ONLY "--board-only"
#define ARG_EXPORT_TRACKS "--export-tracks"

#define REGEX_QUANTITY "([\\s]*[+-]?[\\d]*[.]?[\\d]*)"
#define REGEX_DELIMITER "(?:[\\s]*x)"
#define REGEX_UNIT "([m]{2}|(?:in))"

CLI::EXPORT_PCB_STEP_COMMAND::EXPORT_PCB_STEP_COMMAND() : COMMAND( "step" )
{
    m_argParser.add_argument( ARG_DRILL_ORIGIN )
            .help( UTF8STDSTR( _( "Use Drill Origin for output origin" ) ) )
            .implicit_value( true )
            .default_value( false );

    m_argParser.add_argument( ARG_GRID_ORIGIN )
            .help( UTF8STDSTR( _( "Use Grid Origin for output origin" ) ) )
            .implicit_value( true )
            .default_value( false );

    m_argParser.add_argument( ARG_NO_UNSPECIFIED )
            .help( UTF8STDSTR( _( "Exclude 3D models for components with 'Unspecified' footprint type" ) ) )
            .implicit_value( true )
            .default_value( false );

    m_argParser.add_argument( ARG_NO_DNP )
            .help( UTF8STDSTR( _( "Exclude 3D models for components with 'Do not populate' attribute" ) ) )
            .implicit_value( true )
            .default_value( false );

    m_argParser.add_argument( "--subst-models" )
            .help( UTF8STDSTR( _( "Substitute STEP or IGS models with the same name in place of VRML models" ) ) )
            .implicit_value( true )
            .default_value( false );

    m_argParser.add_argument( ARG_FORCE, "-f" )
            .help( UTF8STDSTR( _( "Overwrite output file" ) ) )
            .implicit_value( true )
            .default_value( false );

    m_argParser.add_argument( ARG_BOARD_ONLY )
            .help( UTF8STDSTR( _( "Only generate a board with no components" ) ) )
            .implicit_value( true )
            .default_value( false );

    m_argParser.add_argument( ARG_EXPORT_TRACKS )
            .help( UTF8STDSTR( _( "Export tracks (extremely time consuming)" ) ) )
            .implicit_value( true )
            .default_value( false );

    m_argParser.add_argument( ARG_MIN_DISTANCE )
            .default_value( std::string( "0.01mm" ) )
            .help( UTF8STDSTR( _( "Minimum distance between points to treat them as separate ones" ) ) );

    m_argParser.add_argument( ARG_USER_ORIGIN )
            .default_value( std::string() )
            .help( UTF8STDSTR( _( "User-specified output origin ex. 1x1in, 1x1inch, 25.4x25.4mm (default unit mm)" ) ) );

    m_argParser.add_argument( "-o", ARG_OUTPUT )
            .default_value( std::string() )
            .help( UTF8STDSTR( _( "Output file name" ) ) );

    m_argParser.add_argument( ARG_INPUT ).help( UTF8STDSTR( _( "Input file" ) ) );
}

int CLI::EXPORT_PCB_STEP_COMMAND::doPerform( KIWAY& aKiway )
{
    std::unique_ptr<JOB_EXPORT_PCB_STEP> step( new JOB_EXPORT_PCB_STEP( true ) );

    step->m_useDrillOrigin = m_argParser.get<bool>( ARG_DRILL_ORIGIN );
    step->m_useGridOrigin = m_argParser.get<bool>( ARG_GRID_ORIGIN );
    step->m_includeUnspecified = !m_argParser.get<bool>( ARG_NO_UNSPECIFIED );
    step->m_includeDNP = !m_argParser.get<bool>( ARG_NO_DNP );
    step->m_substModels = m_argParser.get<bool>( ARG_SUBST_MODELS );
    step->m_overwrite = m_argParser.get<bool>( ARG_FORCE );
    step->m_filename = FROM_UTF8( m_argParser.get<std::string>( ARG_INPUT ).c_str() );
    step->m_outputFile = FROM_UTF8( m_argParser.get<std::string>( ARG_OUTPUT ).c_str() );
    step->m_boardOnly = m_argParser.get<bool>( ARG_BOARD_ONLY );
    step->m_exportTracks = m_argParser.get<bool>( ARG_EXPORT_TRACKS );

    wxString userOrigin = FROM_UTF8( m_argParser.get<std::string>( ARG_USER_ORIGIN ).c_str() );

    LOCALE_IO dummy;    // Switch to "C" locale

    if( !userOrigin.IsEmpty() )
    {
        std::regex  re_pattern( REGEX_QUANTITY REGEX_DELIMITER REGEX_QUANTITY REGEX_UNIT,
                                std::regex_constants::icase );
        std::smatch sm;
        std::string str( userOrigin.ToUTF8() );
        std::regex_search( str, sm, re_pattern );
        step->m_xOrigin = atof( sm.str( 1 ).c_str() );
        step->m_yOrigin = atof( sm.str( 2 ).c_str() );

        // Default unit for m_xOrigin and m_yOrigin is mm.
        // Convert in to board units. If the value is given in inches, it will be converted later
        step->m_xOrigin = pcbIUScale.mmToIU( step->m_xOrigin );
        step->m_yOrigin = pcbIUScale.mmToIU( step->m_yOrigin );

        std::string tunit( sm[3] );

        if( tunit.size() > 0 ) // unit specified ( default = mm, but can be in, inch or mm )
        {
            if( ( !sm.str( 1 ).compare( " " ) || !sm.str( 2 ).compare( " " ) )
                || ( sm.size() != 4 ) )
            {
                std::cout << m_argParser;
                return CLI::EXIT_CODES::ERR_ARGS;
            }

            // only in, inch and mm are valid:
            if( !tunit.compare( "in" ) || !tunit.compare( "inch" ) )
            {
                step->m_xOrigin *= 25.4;
                step->m_yOrigin *= 25.4;
            }
            else if( tunit.compare( "mm" ) )
            {
                std::cout << m_argParser;
                return CLI::EXIT_CODES::ERR_ARGS;
            }
        }
    }

    wxString minDistance = FROM_UTF8( m_argParser.get<std::string>( ARG_MIN_DISTANCE ).c_str() );

    if( !minDistance.IsEmpty() )
    {
        std::regex  re_pattern( REGEX_QUANTITY REGEX_UNIT,
                                std::regex_constants::icase );
        std::smatch sm;
        std::string str( minDistance.ToUTF8() );
        std::regex_search( str, sm, re_pattern );
        step->m_BoardOutlinesChainingEpsilon = atof( sm.str( 1 ).c_str() );

        std::string tunit( sm[2] );

        if( tunit.size() > 0 ) // No unit accepted ( default = mm )
        {
            if( !tunit.compare( "in" ) || !tunit.compare( "inch" ) )
            {
                step->m_BoardOutlinesChainingEpsilon *= 25.4;
            }
            else if( tunit.compare( "mm" ) )
            {
                std::cout << m_argParser;
                return CLI::EXIT_CODES::ERR_ARGS;
            }
        }
    }

    int exitCode = aKiway.ProcessJob( KIWAY::FACE_PCB, step.get() );

    return exitCode;
}
