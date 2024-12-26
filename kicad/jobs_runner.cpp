/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <jobs_runner.h>
#include <jobs/job_registry.h>
#include <jobs/jobset.h>
#include <jobs/job_special_execute.h>
#include <kiway.h>
#include <kiway_express.h>
#include <reporter.h>
#include <wx/process.h>
#include <wx/txtstrm.h>
#include <wx/sstream.h>
#include <wx/wfstream.h>

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


bool JOBS_RUNNER::RunJobsAllOutputs( bool aBail )
{
    bool success = true;

    for( JOBSET_OUTPUT& output : m_jobsFile->GetOutputs() )
    {
        success &= RunJobsForOutput( &output, aBail );
    }

    return success;
}


int JOBS_RUNNER::runSpecialExecute( JOBSET_JOB* aJob )
{
    JOB_SPECIAL_EXECUTE* specialJob = static_cast<JOB_SPECIAL_EXECUTE*>( aJob->m_job.get() );

    // static cast required because wx uses `long` which is 64-bit on Linux but 32-bit on Windows
    wxProcess process;
    process.Redirect();

    int result = static_cast<int>( wxExecute( specialJob->m_command, wxEXEC_SYNC, &process ) );

    wxString output;

    if( specialJob->m_recordOutput )
    {
        wxFFileOutputStream procOutput( specialJob->GetFullOutputPath() );
        wxInputStream*      inputStream = process.GetInputStream();
        if( inputStream )
        {
            inputStream->Read( procOutput );
        }
        procOutput.Close();
    }

    if( specialJob->m_ignoreExitcode )
    {
        return 0;
    }

    return result;
}


bool JOBS_RUNNER::RunJobsForOutput( JOBSET_OUTPUT* aOutput, bool aBail )
{
    bool                       success = true;
    std::vector<JOBSET_JOB> jobsForOutput = m_jobsFile->GetJobsForOutput( aOutput );
    wxString msg;

    wxFileName tmp;
    tmp.AssignDir( wxFileName::GetTempDir() );
    tmp.AppendDir( KIID().AsString() );

    aOutput->m_lastRunSuccessMap.clear();

    for( auto& reporter : aOutput->m_lastRunReporters )
    {
        delete reporter.second;
    }
    aOutput->m_lastRunReporters.clear();

    wxString tempDirPath = tmp.GetFullPath();
    if( !wxFileName::Mkdir( tempDirPath, wxS_DIR_DEFAULT ) )
    {
		if( m_reporter )
		{
			msg = wxString::Format( wxT( "Failed to create temporary directory %s" ), tempDirPath );
			m_reporter->Report( msg, RPT_SEVERITY_ERROR );
		}

        aOutput->m_lastRunSuccess = false;

		return false;
    }

    bool continueOuput = aOutput->m_outputHandler->OutputPrecheck();

    if( !continueOuput )
    {
        if( m_reporter )
        {
            msg = wxString::Format( wxT( "Output precheck failed for output %s" ), aOutput->m_id );
            m_reporter->Report( msg, RPT_SEVERITY_ERROR );
        }
        aOutput->m_lastRunSuccess = false;
        return false;
    }

    if( m_reporter != nullptr )
    {
        msg += wxT( "|--------------------------------\n" );
        msg += wxT( "| " );
        msg += wxString::Format( "Performing jobs for output %s", aOutput->m_id );
        msg += wxT( "\n" );
        msg += wxT( "|--------------------------------\n" );

        msg += wxString::Format( wxT( "|%-5s | %-50s\n" ), wxT( "No." ), wxT( "Description" ) );

        int jobNum = 1;
        for( const JOBSET_JOB& job : jobsForOutput )
        {
            msg += wxString::Format( wxT( "|%-5d | %-50s\n" ), jobNum,
                                               job.m_job->GetDescription() );
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
    for( const JOBSET_JOB& job : jobsForOutput )
    {
        if( m_reporter != nullptr )
        {
            msg = wxT( "|--------------------------------\n" );
            msg += wxString::Format( wxT( "| Running job %d, %s" ), jobNum, job.m_job->GetDescription() );
            msg += wxT( "\n" );
            msg += wxT( "|--------------------------------\n" );

            m_reporter->Report( msg, RPT_SEVERITY_INFO );
        }

        KIWAY::FACE_T iface = JOB_REGISTRY::GetKifaceType( job.m_type );

        job.m_job->SetTempOutputDirectory( tempDirPath );

        REPORTER* reporterToUse = m_reporter;
        if( !reporterToUse || reporterToUse == &NULL_REPORTER::GetInstance() )
        {
            reporterToUse = new WX_STRING_REPORTER;
            aOutput->m_lastRunReporters[job.m_id] = reporterToUse;
        }

        int result = 0;
        if( iface < KIWAY::KIWAY_FACE_COUNT )
        {
            result = m_kiway->ProcessJob( iface, job.m_job.get(), reporterToUse );
        }
        else
        {
            // special jobs
            if( job.m_job->GetType() == "special_execute" )
			{
				JOB_SPECIAL_EXECUTE* specialJob = static_cast<JOB_SPECIAL_EXECUTE*>( job.m_job.get() );

                wxString cmd = ExpandEnvVarSubstitutions( specialJob->m_command, m_project );

                // static cast requried because wx used `long` which is 64-bit on linux but 32-bit on windows
                result = static_cast<int>( wxExecute( cmd ) );
			}
        }

        if( result == 0 )
        {
            aOutput->m_lastRunSuccessMap[job.m_id] = true;
        }
        else
        {
            aOutput->m_lastRunSuccessMap[job.m_id] = false;
        }

        if( m_reporter )
        {
            if( result == 0 )
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

        if( result != 0 )
        {
            if( aBail )
            {
                return result;
            }
            else
            {
                success = false;
            }
        }
    }

    if( success )
    {
        success = aOutput->m_outputHandler->HandleOutputs( tempDirPath, m_project, outputs );
    }

    aOutput->m_lastRunSuccess = success;

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