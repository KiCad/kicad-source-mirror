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

#include "command_pcb_drc.h"
#include <cli/exit_codes.h>
#include "jobs/job_pcb_drc.h"
#include <kiface_base.h>
#include <layer_ids.h>
#include <string_utils.h>
#include <wx/crt.h>

#include <macros.h>
#include <wx/tokenzr.h>

#define ARG_FORMAT "--format"
#define ARG_ALL_TRACK_ERRORS "--all-track-errors"
#define ARG_UNITS "--units"
#define ARG_SEVERITY_ALL "--severity-all"
#define ARG_SEVERITY_ERROR "--severity-error"
#define ARG_SEVERITY_WARNING "--severity-warning"
#define ARG_SEVERITY_EXCLUSIONS "--severity-exclusions"
#define ARG_EXIT_CODE_VIOLATIONS "--exit-code-violations"
#define ARG_PARITY "--schematic-parity"
#define ARG_ZONE_FILL "--refill-zones"
#define ARG_SAVE_BOARD "--save-board"

CLI::PCB_DRC_COMMAND::PCB_DRC_COMMAND() : COMMAND( "drc" )
{
    addCommonArgs( true, true, false, false );
    addDefineArg();

    m_argParser.add_description( UTF8STDSTR( _( "Runs the Design Rules Check (DRC) on the PCB "
                                                "and creates a report" ) ) );

    m_argParser.add_argument( ARG_FORMAT )
            .default_value( std::string( "report" ) )
            .help( UTF8STDSTR( _( "Output file format, options: json, report" ) ) )
            .metavar( "FORMAT" );

    m_argParser.add_argument( ARG_ALL_TRACK_ERRORS )
            .help( UTF8STDSTR( _( "Report all errors for each track" ) ) )
            .flag();

    m_argParser.add_argument( ARG_PARITY )
            .help( UTF8STDSTR( _( "Test for parity between PCB and schematic" ) ) )
            .flag();

    m_argParser.add_argument( ARG_UNITS )
            .default_value( std::string( "mm" ) )
            .help( UTF8STDSTR( _( "Report units; valid options: in, mm, mils" ) ) )
            .metavar( "UNITS" );

    m_argParser.add_argument( ARG_SEVERITY_ALL )
            .help( UTF8STDSTR( _( "Report all DRC violations, this is equivalent to including "
                                  "all the other severity arguments" ) ) )
            .flag();

    m_argParser.add_argument( ARG_SEVERITY_ERROR )
            .help( UTF8STDSTR( _( "Report all DRC error level violations, this can be combined "
                                  "with the other severity arguments" ) ) )
            .flag();

    m_argParser.add_argument( ARG_SEVERITY_WARNING )
            .help( UTF8STDSTR( _( "Report all DRC warning level violations, this can be combined "
                                  "with the other severity arguments" ) ) )
            .flag();

    m_argParser.add_argument( ARG_SEVERITY_EXCLUSIONS )
            .help( UTF8STDSTR( _( "Report all excluded DRC violations, this can be combined with "
                                  "the other severity arguments" ) ) )
            .flag();

    m_argParser.add_argument( ARG_EXIT_CODE_VIOLATIONS )
            .help( UTF8STDSTR( _( "Return a nonzero exit code if DRC violations exist" ) ) )
            .flag();

    m_argParser.add_argument( ARG_ZONE_FILL )
            .help( UTF8STDSTR( _( "Refill zones before running DRC" ) ) )
            .flag();

    m_argParser.add_argument( ARG_SAVE_BOARD )
            .help( UTF8STDSTR( _( "Save the board after DRC, must be used with --refill-zones" ) ) )
            .flag();
}


int CLI::PCB_DRC_COMMAND::doPerform( KIWAY& aKiway )
{
    std::unique_ptr<JOB_PCB_DRC> drcJob( new JOB_PCB_DRC() );

    drcJob->SetConfiguredOutputPath( m_argOutput );
    drcJob->m_filename = m_argInput;
    drcJob->SetVarOverrides( m_argDefineVars );
    drcJob->m_reportAllTrackErrors = m_argParser.get<bool>( ARG_ALL_TRACK_ERRORS );
    drcJob->m_exitCodeViolations = m_argParser.get<bool>( ARG_EXIT_CODE_VIOLATIONS );

    int severity = 0;
    if( m_argParser.get<bool>( ARG_SEVERITY_ALL ) )
    {
        severity = RPT_SEVERITY_ERROR | RPT_SEVERITY_WARNING | RPT_SEVERITY_EXCLUSION;
    }

    if( m_argParser.get<bool>( ARG_SEVERITY_ERROR ) )
    {
        severity |= RPT_SEVERITY_ERROR;
    }

    if( m_argParser.get<bool>( ARG_SEVERITY_WARNING ) )
    {
        severity |= RPT_SEVERITY_WARNING;
    }

    if( m_argParser.get<bool>( ARG_SEVERITY_EXCLUSIONS ) )
    {
        severity |= RPT_SEVERITY_EXCLUSION;
    }

    if( severity ) // override the default only if something we configured
        drcJob->m_severity = severity;

    drcJob->m_reportAllTrackErrors = m_argParser.get<bool>( ARG_ALL_TRACK_ERRORS );

    wxString units = From_UTF8( m_argParser.get<std::string>( ARG_UNITS ).c_str() );

    if( units == wxS( "mm" ) )
    {
        drcJob->m_units = JOB_PCB_DRC::UNITS::MM;
    }
    else if( units == wxS( "in" ) )
    {
        drcJob->m_units = JOB_PCB_DRC::UNITS::INCH;
    }
    else if( units == wxS( "mils" ) )
    {
        drcJob->m_units = JOB_PCB_DRC::UNITS::MILS;
    }
    else if( !units.IsEmpty() )
    {
        wxFprintf( stderr, _( "Invalid units specified\n" ) );
        return EXIT_CODES::ERR_ARGS;
    }

    wxString format = From_UTF8( m_argParser.get<std::string>( ARG_FORMAT ).c_str() );
    if( format == "report" )
    {
        drcJob->m_format = JOB_PCB_DRC::OUTPUT_FORMAT::REPORT;
    }
    else if( format == "json" )
    {
        drcJob->m_format = JOB_PCB_DRC::OUTPUT_FORMAT::JSON;
    }
    else
    {
        wxFprintf( stderr, _( "Invalid report format\n" ) );
        return EXIT_CODES::ERR_ARGS;
    }

    drcJob->m_parity = m_argParser.get<bool>( ARG_PARITY );
    drcJob->m_refillZones = m_argParser.get<bool>( ARG_ZONE_FILL );
    drcJob->m_saveBoard = m_argParser.get<bool>( ARG_SAVE_BOARD );

    int exitCode = aKiway.ProcessJob( KIWAY::FACE_PCB, drcJob.get() );

    return exitCode;
}
