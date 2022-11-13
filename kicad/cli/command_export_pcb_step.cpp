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

#include <macros.h>

#define ARG_DRILL_ORIGIN "--drill-origin"
#define ARG_GRID_ORIGIN "--grid-origin"
#define ARG_NO_VIRTUAL "--no-virtual"
#define ARG_SUBST_MODELS "--subst-models"
#define ARG_FORCE "--force"
#define ARG_OUTPUT "--output"
#define ARG_INPUT "input"
#define ARG_MIN_DISTANCE "--min-distance"
#define ARG_USER_ORIGIN "--user-origin"
#define ARG_BOARD_ONLY "--board-only"

#define REGEX_QUANTITY "([\\s]*[+-]?[\\d]*[.]?[\\d]*)"
#define REGEX_DELIMITER "(?:[\\s]*x)"
#define REGEX_UNIT "([m]{2}|(?:in))"

CLI::EXPORT_PCB_STEP_COMMAND::EXPORT_PCB_STEP_COMMAND() : COMMAND( "step" )
{
    m_argParser.add_argument( ARG_DRILL_ORIGIN )
            .help( "Use Drill Origin for output origin" )
            .implicit_value( true )
            .default_value( false );

    m_argParser.add_argument( ARG_GRID_ORIGIN )
            .help( "Use Grid Origin for output origin" )
            .implicit_value( true )
            .default_value( false );

    m_argParser.add_argument( ARG_NO_VIRTUAL )
            .help( "Exclude 3D models for components with 'virtual' attribute" )
            .implicit_value( true )
            .default_value( false );

    m_argParser.add_argument( "--subst-models" )
            .help( "Substitute STEP or IGS models with the same name in place of VRML models" )
            .implicit_value( true )
            .default_value( false );

    m_argParser.add_argument( ARG_FORCE, "-f" )
            .help( "overwrite output file" )
            .implicit_value( true )
            .default_value( false );

    m_argParser.add_argument( ARG_BOARD_ONLY )
            .help( "only generate a board with no components" )
            .implicit_value( true )
            .default_value( false );

    m_argParser.add_argument( ARG_MIN_DISTANCE )
            .default_value( std::string() )
            .help( "Minimum distance between points to treat them as separate ones (default 0.01mm)" );

    m_argParser.add_argument( ARG_USER_ORIGIN )
            .default_value( std::string() )
            .help( "User-specified output origin ex. 1x1in, 1x1inch, 25.4x25.4mm (default mm)" );

    m_argParser.add_argument( "-o", ARG_OUTPUT )
            .default_value( std::string() )
            .help( "output file name" );

    m_argParser.add_argument( ARG_INPUT ).help( "input file" );
}

int CLI::EXPORT_PCB_STEP_COMMAND::Perform( KIWAY& aKiway )
{
    std::unique_ptr<JOB_EXPORT_PCB_STEP> step( new JOB_EXPORT_PCB_STEP( true ) );

    step->m_useDrillOrigin = m_argParser.get<bool>( ARG_DRILL_ORIGIN );
    step->m_useGridOrigin = m_argParser.get<bool>( ARG_GRID_ORIGIN );
    step->m_includeExcludedBom = !m_argParser.get<bool>( ARG_NO_VIRTUAL );
    step->m_substModels = m_argParser.get<bool>( ARG_SUBST_MODELS );
    step->m_overwrite = m_argParser.get<bool>( ARG_FORCE );
    step->m_filename = FROM_UTF8( m_argParser.get<std::string>( ARG_INPUT ).c_str() );
    step->m_outputFile = FROM_UTF8( m_argParser.get<std::string>( ARG_OUTPUT ).c_str() );
    step->m_boardOnly = m_argParser.get<bool>( ARG_BOARD_ONLY );

    wxString userOrigin = FROM_UTF8( m_argParser.get<std::string>( ARG_USER_ORIGIN ).c_str() );
    if( !userOrigin.IsEmpty() )
    {
        std::regex  re_pattern( REGEX_QUANTITY REGEX_DELIMITER REGEX_QUANTITY REGEX_UNIT,
                                std::regex_constants::icase );
        std::smatch sm;
        std::string str( userOrigin.ToUTF8() );
        std::regex_search( str, sm, re_pattern );
        step->m_xOrigin = atof( sm.str( 1 ).c_str() );
        step->m_yOrigin = atof( sm.str( 2 ).c_str() );

        std::string tunit( sm[3] );

        if( tunit.size() > 0 ) // No unit accepted ( default = mm )
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
        step->m_minDistance = atof( sm.str( 1 ).c_str() );

        std::string tunit( sm[2] );

        if( tunit.size() > 0 ) // No unit accepted ( default = mm )
        {
            if( !tunit.compare( "in" ) || !tunit.compare( "inch" ) )
            {
                step->m_minDistance *= 25.4;
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
