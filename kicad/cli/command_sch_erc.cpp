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

#include "command_sch_erc.h"
#include <cli/exit_codes.h>
#include "jobs/job_sch_erc.h"
#include <kiface_base.h>
#include <layer_ids.h>
#include <string_utils.h>
#include <wx/crt.h>

#include <macros.h>
#include <wx/tokenzr.h>

#define ARG_FORMAT "--format"
#define ARG_UNITS "--units"
#define ARG_SEVERITY_ALL "--severity-all"
#define ARG_SEVERITY_ERROR "--severity-error"
#define ARG_SEVERITY_WARNING "--severity-warning"
#define ARG_SEVERITY_EXCLUSIONS "--severity-exclusions"
#define ARG_EXIT_CODE_VIOLATIONS "--exit-code-violations"

CLI::SCH_ERC_COMMAND::SCH_ERC_COMMAND() : COMMAND( "erc" )
{
    addCommonArgs( true, true, false, false );
    addDefineArg();

    m_argParser.add_description( UTF8STDSTR( _( "Runs the Electrical Rules Check (ERC) on the "
                                                "schematic and creates a report" ) ) );

    m_argParser.add_argument( ARG_FORMAT )
            .default_value( std::string( "report" ) )
            .help( UTF8STDSTR( _( "Output file format, options: json, report" ) ) );

    m_argParser.add_argument( ARG_UNITS )
            .default_value( std::string( "mm" ) )
            .help( UTF8STDSTR(
                    _( "Report units; valid options: in, mm, mils" ) ) );

    m_argParser.add_argument( ARG_SEVERITY_ALL )
            .help( UTF8STDSTR( _( "Report all ERC violations, this is equivalent to including "
                                  "all the other severity arguments" ) ) )
            .flag();

    m_argParser.add_argument( ARG_SEVERITY_ERROR )
            .help( UTF8STDSTR( _( "Report all ERC error level violations, this can be combined "
                                  "with the other severity arguments" ) ) )
            .flag();

    m_argParser.add_argument( ARG_SEVERITY_WARNING )
            .help( UTF8STDSTR( _( "Report all ERC warning level violations, this can be combined "
                                  "with the other severity arguments" ) ) )
            .flag();

    m_argParser.add_argument( ARG_SEVERITY_EXCLUSIONS )
            .help( UTF8STDSTR( _( "Report all excluded ERC violations, this can be combined "
                                  "with the other severity arguments" ) ) )
            .flag();

    m_argParser.add_argument( ARG_EXIT_CODE_VIOLATIONS )
            .help( UTF8STDSTR( _( "Return a nonzero exit code if ERC violations exist" ) ) )
            .flag();
}


int CLI::SCH_ERC_COMMAND::doPerform( KIWAY& aKiway )
{
    std::unique_ptr<JOB_SCH_ERC> ercJob( new JOB_SCH_ERC() );

    ercJob->SetConfiguredOutputPath( m_argOutput );
    ercJob->m_filename = m_argInput;
    ercJob->m_exitCodeViolations = m_argParser.get<bool>( ARG_EXIT_CODE_VIOLATIONS );
    ercJob->SetVarOverrides( m_argDefineVars );

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
        ercJob->m_severity = severity;

    wxString units = From_UTF8( m_argParser.get<std::string>( ARG_UNITS ).c_str() );

    if( units == wxS( "mm" ) )
    {
        ercJob->m_units = JOB_SCH_ERC::UNITS::MM;
    }
    else if( units == wxS( "in" ) )
    {
        ercJob->m_units = JOB_SCH_ERC::UNITS::INCH;
    }
    else if( units == wxS( "mils" ) )
    {
        ercJob->m_units = JOB_SCH_ERC::UNITS::MILS;
    }
    else if( !units.IsEmpty() )
    {
        wxFprintf( stderr, _( "Invalid units specified\n" ) );
        return EXIT_CODES::ERR_ARGS;
    }

    wxString format = From_UTF8( m_argParser.get<std::string>( ARG_FORMAT ).c_str() );

    if( format == "report" )
    {
        ercJob->m_format = JOB_SCH_ERC::OUTPUT_FORMAT::REPORT;
    }
    else if( format == "json" )
    {
        ercJob->m_format = JOB_SCH_ERC::OUTPUT_FORMAT::JSON;
    }
    else
    {
        wxFprintf( stderr, _( "Invalid report format\n" ) );
        return EXIT_CODES::ERR_ARGS;
    }

    int exitCode = aKiway.ProcessJob( KIWAY::FACE_SCH, ercJob.get() );

    return exitCode;
}
