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

#include <common.h>
#include <cli/exit_codes.h>
#include <jobs_runner.h>
#include <jobs/job_registry.h>
#include <jobs/jobset.h>
#include <jobs/job_special_copyfiles.h>
#include <jobs/job_special_execute.h>
#include <kiway.h>
#include <kiway_express.h>
#include <reporter.h>
#include <wx/process.h>
#include <wx/txtstrm.h>
#include <wx/sstream.h>
#include <wx/wfstream.h>
#include <gestfich.h>

JOBS_RUNNER::JOBS_RUNNER( KIWAY* aKiway, JOBSET* aJobsFile, PROJECT* aProject,
                          REPORTER* aReporter ) :
        m_kiway( aKiway ),
        m_jobsFile( aJobsFile ),
        m_reporter( aReporter ),
        m_project( aProject )
{
    if( !m_reporter )
    {
        m_reporter = &NULL_REPORTER::GetInstance();
    }
}


bool JOBS_RUNNER::RunJobsAllDestinations( bool aBail )
{
    bool success = true;

    for( JOBSET_DESTINATION& destination : m_jobsFile->GetDestinations() )
        success &= RunJobsForDestination( &destination, aBail );

    return success;
}


int JOBS_RUNNER::runSpecialExecute( const JOBSET_JOB* aJob, PROJECT* aProject )
{
    JOB_SPECIAL_EXECUTE* specialJob = static_cast<JOB_SPECIAL_EXECUTE*>( aJob->m_job.get() );

    wxString             cmd = ExpandEnvVarSubstitutions( specialJob->m_command, m_project );

    // static cast required because wx uses `long` which is 64-bit on Linux but 32-bit on Windows
    wxProcess process;
    process.Redirect();

    int result = static_cast<int>( wxExecute( cmd, wxEXEC_SYNC, &process ) );

    if( specialJob->m_recordOutput )
    {
        if( specialJob->GetConfiguredOutputPath().IsEmpty() )
        {
            wxFileName fn( aJob->m_id );
            fn.SetExt( wxT( "log" ) );
            specialJob->SetConfiguredOutputPath( fn.GetFullPath() );
        }

        wxFFileOutputStream procOutput( specialJob->GetFullOutputPath( aProject ) );

        if( !procOutput.IsOk() )
            return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;

        wxInputStream*      inputStream = process.GetInputStream();
        if( inputStream )
        {
            inputStream->Read( procOutput );
        }
        procOutput.Close();
    }

    if( specialJob->m_ignoreExitcode )
    {
        return CLI::EXIT_CODES::OK;
    }

    return result;
}


int JOBS_RUNNER::runSpecialCopyFiles( const JOBSET_JOB* aJob, PROJECT* aProject )
{
    JOB_SPECIAL_COPYFILES* job = static_cast<JOB_SPECIAL_COPYFILES*>( aJob->m_job.get() );

    wxString source = ExpandEnvVarSubstitutions( job->m_source, aProject );

    if( source.IsEmpty() )
        return CLI::EXIT_CODES::ERR_ARGS;

    wxString   projectPath = aProject->GetProjectPath();
    wxFileName sourceFn( source );
    sourceFn.MakeAbsolute( projectPath );

    wxFileName destFn( job->GetFullOutputPath( aProject ) );

    if( !job->m_dest.IsEmpty() )
        destFn.AppendDir( job->m_dest );

    std::vector<wxString> exclusions;

    for( const JOBSET_DESTINATION& destination : m_jobsFile->GetDestinations() )
        exclusions.push_back( projectPath + destination.m_outputHandler->GetOutputPath() );

    wxString errors;
    int      copyCount = 0;
    bool     success = CopyFilesOrDirectory( sourceFn.GetFullPath(), destFn.GetFullPath(),
                                             errors, copyCount, exclusions );

    if( !success )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    if( job->m_generateErrorOnNoCopy && copyCount == 0 )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    return CLI::EXIT_CODES::OK;
}


class JOBSET_OUTPUT_REPORTER : public WX_STRING_REPORTER
{
public:
    JOBSET_OUTPUT_REPORTER( const wxString& aTempDirPath ) :
            m_tempDirPath( aTempDirPath )
    {
    }

    REPORTER& Report( const wxString& aText, SEVERITY aSeverity ) override
    {
        wxString text( aText );

        if( aSeverity == RPT_SEVERITY_ACTION )
            text.Replace( m_tempDirPath, wxEmptyString );

        return WX_STRING_REPORTER::Report( text, aSeverity );
    }

private:
    wxString m_tempDirPath;
};


bool JOBS_RUNNER::RunJobsForDestination( JOBSET_DESTINATION* aDestination, bool aBail )
{
    bool                    genOutputs = true;
    bool                    success = true;
    std::vector<JOBSET_JOB> jobsForDestination = m_jobsFile->GetJobsForDestination( aDestination );
    wxString msg;

    wxFileName tmp;
    tmp.AssignDir( wxFileName::GetTempDir() );
    tmp.AppendDir( KIID().AsString() );

    aDestination->m_lastRunSuccessMap.clear();

    for( auto& [name, reporter] : aDestination->m_lastRunReporters )
        delete reporter;

    aDestination->m_lastRunReporters.clear();

    wxString tempDirPath = tmp.GetFullPath();

    if( !wxFileName::Mkdir( tempDirPath, wxS_DIR_DEFAULT ) )
    {
		if( m_reporter )
		{
			msg = wxString::Format( wxT( "Failed to create temporary directory %s" ), tempDirPath );
			m_reporter->Report( msg, RPT_SEVERITY_ERROR );
		}

        aDestination->m_lastRunSuccess = false;

		return false;
    }

    bool continueOuput = aDestination->m_outputHandler->OutputPrecheck();

    if( !continueOuput )
    {
        if( m_reporter )
        {
            msg = wxString::Format( wxT( "Output precheck failed for output %s" ),
                                    aDestination->m_id );
            m_reporter->Report( msg, RPT_SEVERITY_ERROR );
        }

        aDestination->m_lastRunSuccess = false;
        return false;
    }

    if( m_reporter != nullptr )
    {
        msg += wxT( "|--------------------------------\n" );
        msg += wxT( "| " );
        msg += wxString::Format( "Performing jobs for output %s", aDestination->m_id );
        msg += wxT( "\n" );
        msg += wxT( "|--------------------------------\n" );

        msg += wxString::Format( wxT( "|%-5s | %-50s\n" ), wxT( "No." ), wxT( "Description" ) );

        int jobNum = 1;

        for( const JOBSET_JOB& job : jobsForDestination )
        {
            msg += wxString::Format( wxT( "|%-5d | %-50s\n" ), jobNum, job.GetDescription() );
            jobNum++;
        }

        msg += wxT( "|--------------------------------\n" );
        msg += wxT( "\n" );
        msg += wxT( "\n" );

        m_reporter->Report( msg, RPT_SEVERITY_INFO );
    }

    std::vector<JOB_OUTPUT> outputs;

    int jobNum = 1;
    int failCount = 0;
    int successCount = 0;

    for( const JOBSET_JOB& job : jobsForDestination )
    {
        if( m_reporter != nullptr )
        {
            msg = wxT( "|--------------------------------\n" );

            msg += wxString::Format( wxT( "| Running job %d, %s" ), jobNum, job.GetDescription() );
            jobNum++;

            msg += wxT( "\n" );
            msg += wxT( "|--------------------------------\n" );

            m_reporter->Report( msg, RPT_SEVERITY_INFO );
        }

        KIWAY::FACE_T iface = JOB_REGISTRY::GetKifaceType( job.m_type );

        job.m_job->SetTempOutputDirectory( tempDirPath );

        REPORTER* reporterToUse = m_reporter;

        if( !reporterToUse || reporterToUse == &NULL_REPORTER::GetInstance() )
        {
            reporterToUse = new JOBSET_OUTPUT_REPORTER( tempDirPath );
            aDestination->m_lastRunReporters[job.m_id] = reporterToUse;
        }

        int result = CLI::EXIT_CODES::SUCCESS;

        if( iface < KIWAY::KIWAY_FACE_COUNT )
        {
            result = m_kiway->ProcessJob( iface, job.m_job.get(), reporterToUse );
        }
        else
        {
            // special jobs
            if( job.m_job->GetType() == "special_execute" )
			{
                result = runSpecialExecute( &job, m_project );
            }
            else if( job.m_job->GetType() == "special_copyfiles" )
            {
                result = runSpecialCopyFiles( &job, m_project );
            }
        }

        aDestination->m_lastRunSuccessMap[job.m_id] = ( result == CLI::EXIT_CODES::SUCCESS );

        if( m_reporter )
        {
            if( result == CLI::EXIT_CODES::SUCCESS )
            {
                wxString msg_fmt = wxT( "\033[32;1m%s\033[0m\n" );
                msg = wxString::Format( msg_fmt, _( "Job successful" ) );

                successCount++;
            }
            else
            {
                wxString msg_fmt = wxT( "\033[31;1m%s\033[0m\n" );
                msg = wxString::Format( msg_fmt, _( "Job failed" ) );

                failCount++;
            }

            msg += wxT( "\n\n" );
            m_reporter->Report( msg, RPT_SEVERITY_INFO );
        }

        if( result == CLI::EXIT_CODES::ERR_RC_VIOLATIONS )
        {
            success = false;

            if( aBail )
                break;
        }
        else if( result != CLI::EXIT_CODES::SUCCESS )
        {
            genOutputs = false;
            success = false;

            if( aBail )
                break;
        }
    }

    if( genOutputs )
        success &= aDestination->m_outputHandler->HandleOutputs( tempDirPath, m_project, outputs );

    aDestination->m_lastRunSuccess = success;

    if( m_reporter )
    {
        msg = wxString::Format( wxT( "\n\n\033[33;1m%d %s, %d %s\033[0m\n" ),
                                successCount,
                                wxT( "jobs succeeded" ),
                                failCount,
                                wxT( "job failed" ) );

        m_reporter->Report( msg, RPT_SEVERITY_INFO );
    }

	return success;
}