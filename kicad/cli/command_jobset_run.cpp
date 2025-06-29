/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 Mark Roszko <mark.roszko@gmail.com>
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

#include "command_jobset_run.h"
#include <cli/exit_codes.h>
#include <kiface_base.h>
#include <layer_ids.h>
#include <string_utils.h>
#include <wx/crt.h>
#include <jobs/jobset.h>
#include <jobs/job_registry.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>

#include <macros.h>
#include <wx/tokenzr.h>
#include <jobs_runner.h>
#include <reporter.h>

#define ARG_STOP_ON_ERROR "--stop-on-error"
#define ARG_JOB_FILE "--file"
#define ARG_OUTPUT "--output"

CLI::JOBSET_RUN_COMMAND::JOBSET_RUN_COMMAND() : COMMAND( "run" )
{
    addCommonArgs( true, false, false, false );

    m_argParser.add_description( UTF8STDSTR( _( "Runs a jobset file" ) ) );

    m_argParser.add_argument( ARG_STOP_ON_ERROR )
            .help( UTF8STDSTR(
                    _( "Stops processing jobs as they are executed sequentially on the first failure of a job" ) ) )
            .flag();

    m_argParser.add_argument( ARG_JOB_FILE, "-f" )
            .help( UTF8STDSTR( _( "Jobset file to be run" ) ) )
            .default_value( std::string( "" ) )
            .metavar( "JOB_FILE" );

    m_argParser.add_argument( ARG_OUTPUT )
            .help( UTF8STDSTR( _( "Jobset file output to generate, leave blank for all outputs defined in the jobset" ) ) )
            .default_value( std::string( "" ) )
            .metavar( "OUTPUT" );
}


int CLI::JOBSET_RUN_COMMAND::doPerform( KIWAY& aKiway )
{
    bool bail = m_argParser.get<bool>( ARG_STOP_ON_ERROR );
    wxString jobsFilePath = From_UTF8( m_argParser.get<std::string>( ARG_JOB_FILE ).c_str() );
    wxString  projectFile = m_argInput.ToStdString();

    wxString outputKey = From_UTF8( m_argParser.get<std::string>( ARG_OUTPUT ).c_str() );

    if( !Pgm().GetSettingsManager().LoadProject( projectFile ) )
    {
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    PROJECT* project = Pgm().GetSettingsManager().GetProject( projectFile );

    JOBSET jobFile( jobsFilePath.ToStdString() );

    jobFile.LoadFromFile();

    JOBS_RUNNER jobsRunner( &aKiway, &jobFile, project, CLI_REPORTER::GetInstance(), nullptr );

    int return_code = CLI::EXIT_CODES::SUCCESS;

    if( !outputKey.IsEmpty() )
    {
        JOBSET_DESTINATION* destination = jobFile.FindDestination( outputKey );

        if( destination == nullptr || !jobsRunner.RunJobsForDestination( destination, bail ) )
            return_code = CLI::EXIT_CODES::ERR_JOBS_RUN_FAILED;
    }
    else
    {
        if( !jobsRunner.RunJobsAllDestinations( bail ) )
            return_code = CLI::EXIT_CODES::ERR_JOBS_RUN_FAILED;
    }

    return return_code;
}
