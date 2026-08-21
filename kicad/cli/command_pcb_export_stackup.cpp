/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2026 Krishna Swaroop <krishna.swaroop@pixxel.co.in>
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

#include "command_pcb_export_stackup.h"

#include <cli/exit_codes.h>
#include <jobs/job_export_pcb_stackup.h>
#include <string_utils.h>
#include <wx/crt.h>

#define ARG_FORMAT "--format"
#define ARG_UNITS "--units"
#define ARG_EXCLUDE_COLOR "--exclude-color"
#define ARG_EXCLUDE_MATERIAL "--exclude-material"
#define ARG_EXCLUDE_THICKNESS "--exclude-thickness"
#define ARG_EXCLUDE_EPSILON_R "--exclude-epsilon-r"
#define ARG_EXCLUDE_LOSS_TANGENT "--exclude-loss-tangent"
#define ARG_EXCLUDE_FINISH "--exclude-finish"
#define ARG_EXCLUDE_BOARD_OPTIONS "--exclude-board-options"

CLI::PCB_EXPORT_STACKUP_COMMAND::PCB_EXPORT_STACKUP_COMMAND() :
        PCB_EXPORT_BASE_COMMAND( "stackup", IO_TYPE::FILE, IO_TYPE::FILE )
{
    m_argParser.add_description( UTF8STDSTR( _( "Export a board stackup report" ) ) );

    m_argParser.add_argument( ARG_FORMAT )
            .default_value( std::string( "csv" ) )
            .help( UTF8STDSTR( _( "Output file format; valid options: csv, json" ) ) )
            .metavar( "FORMAT" );

    m_argParser.add_argument( ARG_UNITS )
            .default_value( std::string( "mm" ) )
            .help( UTF8STDSTR( _( "CSV units; valid options: in, mm" ) ) )
            .metavar( "UNITS" );

    m_argParser.add_argument( ARG_EXCLUDE_COLOR ).help( UTF8STDSTR( _( "Omit color from the CSV export" ) ) ).flag();

    m_argParser.add_argument( ARG_EXCLUDE_MATERIAL )
            .help( UTF8STDSTR( _( "Omit material from the CSV export" ) ) )
            .flag();

    m_argParser.add_argument( ARG_EXCLUDE_THICKNESS )
            .help( UTF8STDSTR( _( "Omit thickness from the CSV export" ) ) )
            .flag();

    m_argParser.add_argument( ARG_EXCLUDE_EPSILON_R )
            .help( UTF8STDSTR( _( "Omit Epsilon R from the CSV export" ) ) )
            .flag();

    m_argParser.add_argument( ARG_EXCLUDE_LOSS_TANGENT )
            .help( UTF8STDSTR( _( "Omit loss tangent from the CSV export" ) ) )
            .flag();

    m_argParser.add_argument( ARG_EXCLUDE_FINISH )
            .help( UTF8STDSTR( _( "Omit board finish from the CSV export" ) ) )
            .flag();

    m_argParser.add_argument( ARG_EXCLUDE_BOARD_OPTIONS )
            .help( UTF8STDSTR( _( "Omit board options from the CSV export" ) ) )
            .flag();
}


int CLI::PCB_EXPORT_STACKUP_COMMAND::doPerform( KIWAY& aKiway )
{
    std::unique_ptr<JOB_EXPORT_PCB_STACKUP> stackupJob( new JOB_EXPORT_PCB_STACKUP() );

    stackupJob->m_filename = m_argInput;

    if( !wxFile::Exists( stackupJob->m_filename ) )
    {
        wxFprintf( stderr, _( "Board file does not exist or is not accessible\n" ) );
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    wxString format = From_UTF8( m_argParser.get<std::string>( ARG_FORMAT ).c_str() );

    if( format == "json" )
    {
        stackupJob->m_format = JOB_EXPORT_PCB_STACKUP::OUTPUT_FORMAT::JSON;
    }
    else if( format == "csv" )
    {
        stackupJob->m_format = JOB_EXPORT_PCB_STACKUP::OUTPUT_FORMAT::CSV;
    }
    else
    {
        wxFprintf( stderr, _( "Invalid format\n" ) );
        return EXIT_CODES::ERR_ARGS;
    }

    wxString units = From_UTF8( m_argParser.get<std::string>( ARG_UNITS ).c_str() );

    if( units == wxS( "mm" ) )
    {
        stackupJob->m_units = JOB_EXPORT_PCB_STACKUP::UNITS::MM;
    }
    else if( units == wxS( "in" ) || units == wxS( "inch" ) )
    {
        stackupJob->m_units = JOB_EXPORT_PCB_STACKUP::UNITS::INCH;
    }
    else
    {
        wxFprintf( stderr, _( "Invalid units specified\n" ) );
        return EXIT_CODES::ERR_ARGS;
    }

    stackupJob->m_includeColor = !m_argParser.get<bool>( ARG_EXCLUDE_COLOR );
    stackupJob->m_includeMaterial = !m_argParser.get<bool>( ARG_EXCLUDE_MATERIAL );
    stackupJob->m_includeThickness = !m_argParser.get<bool>( ARG_EXCLUDE_THICKNESS );
    stackupJob->m_includeEpsilonR = !m_argParser.get<bool>( ARG_EXCLUDE_EPSILON_R );
    stackupJob->m_includeLossTangent = !m_argParser.get<bool>( ARG_EXCLUDE_LOSS_TANGENT );
    stackupJob->m_includeFinish = !m_argParser.get<bool>( ARG_EXCLUDE_FINISH );
    stackupJob->m_includeBoardOptions = !m_argParser.get<bool>( ARG_EXCLUDE_BOARD_OPTIONS );

    stackupJob->SetConfiguredOutputPath( m_argOutput );

    return aKiway.ProcessJob( KIWAY::FACE_PCB, stackupJob.get() );
}
