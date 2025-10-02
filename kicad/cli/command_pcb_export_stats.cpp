/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 The KiCad Developers
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

#include "command_pcb_export_stats.h"

#include <cli/exit_codes.h>
#include <jobs/job_export_pcb_stats.h>
#include <string_utils.h>
#include <wx/crt.h>

#define ARG_EXCLUDE_NO_PADS "--exclude-footprints-without-pads"
#define ARG_SUBTRACT_HOLES "--subtract-holes-from-board"
#define ARG_SUBTRACT_HOLES_COPPER "--subtract-holes-from-copper"
#define ARG_FORMAT "--format"
#define ARG_UNITS "--units"

CLI::PCB_EXPORT_STATS_COMMAND::PCB_EXPORT_STATS_COMMAND() :
        PCB_EXPORT_BASE_COMMAND( "stats", false, false )
{
    m_argParser.add_description( UTF8STDSTR( _( "Generate a board statistics report" ) ) );

    m_argParser.add_argument( ARG_FORMAT )
            .default_value( std::string( "report" ) )
            .help( UTF8STDSTR( _( "Output file format, options: json, report" ) ) )
            .metavar( "FORMAT" );

    m_argParser.add_argument( ARG_UNITS )
            .default_value( std::string( "mm" ) )
            .help( UTF8STDSTR( _( "Report units; valid options: in, mm" ) ) )
            .metavar( "UNITS" );

    m_argParser.add_argument( ARG_EXCLUDE_NO_PADS )
            .help( UTF8STDSTR( _( "Exclude footprints without pads" ) ) )
            .flag();

    m_argParser.add_argument( ARG_SUBTRACT_HOLES )
            .help( UTF8STDSTR( _( "Subtract holes from the board area" ) ) )
            .flag();

    m_argParser.add_argument( ARG_SUBTRACT_HOLES_COPPER )
            .help( UTF8STDSTR( _( "Subtract holes from copper areas" ) ) )
            .flag();
}


int CLI::PCB_EXPORT_STATS_COMMAND::doPerform( KIWAY& aKiway )
{
    std::unique_ptr<JOB_EXPORT_PCB_STATS> statsJob( new JOB_EXPORT_PCB_STATS() );

    statsJob->m_filename = m_argInput;

    if( !wxFile::Exists( statsJob->m_filename ) )
    {
        wxFprintf( stderr, _( "Board file does not exist or is not accessible\n" ) );
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    statsJob->m_excludeFootprintsWithoutPads = m_argParser.get<bool>( ARG_EXCLUDE_NO_PADS );
    statsJob->m_subtractHolesFromBoardArea   = m_argParser.get<bool>( ARG_SUBTRACT_HOLES );
    statsJob->m_subtractHolesFromCopperAreas = m_argParser.get<bool>( ARG_SUBTRACT_HOLES_COPPER );

    wxString format = From_UTF8( m_argParser.get<std::string>( ARG_FORMAT ).c_str() );

    if( format == "report" )
        statsJob->m_format = JOB_EXPORT_PCB_STATS::OUTPUT_FORMAT::REPORT;
    else if( format == "json" )
        statsJob->m_format = JOB_EXPORT_PCB_STATS::OUTPUT_FORMAT::JSON;
    else
    {
        wxFprintf( stderr, _( "Invalid report format\n" ) );
        return EXIT_CODES::ERR_ARGS;
    }

    wxString units = From_UTF8( m_argParser.get<std::string>( ARG_UNITS ).c_str() );

    if( units == wxS( "mm" ) )
        statsJob->m_units = JOB_EXPORT_PCB_STATS::UNITS::MM;
    else if( units == wxS( "in" ) || units == wxS( "inch" ) )
        statsJob->m_units = JOB_EXPORT_PCB_STATS::UNITS::INCH;
    else
    {
        wxFprintf( stderr, _( "Invalid units specified\n" ) );
        return EXIT_CODES::ERR_ARGS;
    }

    statsJob->SetConfiguredOutputPath( m_argOutput );

    return aKiway.ProcessJob( KIWAY::FACE_PCB, statsJob.get() );
}
