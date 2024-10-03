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

#include <jobs_runner.h>
#include <jobs/job_registry.h>
#include <jobs/jobset.h>
#include <kiway.h>
#include <kiway_express.h>
#include <reporter.h>

JOBS_RUNNER::JOBS_RUNNER( KIWAY* aKiway, JOBSET* aJobsFile,
                          REPORTER* aReporter ) :
        m_kiway( aKiway ),
        m_jobsFile( aJobsFile ),
        m_reporter( aReporter )
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


bool JOBS_RUNNER::RunJobsForOutput( JOBSET_OUTPUT* aOutput, bool aBail )
{
    bool                       success = true;
    std::vector<JOBSET_JOB> jobsForOutput = m_jobsFile->GetJobsForOutput( aOutput );
    wxString msg;

    wxFileName tmp;
    tmp.AssignDir( wxFileName::GetTempDir() );
    tmp.AppendDir( KIID().AsString() );

    wxString tempDirPath = tmp.GetFullPath();
    if( !wxFileName::Mkdir( tempDirPath, wxS_DIR_DEFAULT ) )
    {
		if( m_reporter )
		{
			msg = wxString::Format( wxT( "Failed to create temporary directory %s" ), tempDirPath );
			m_reporter->Report( msg, RPT_SEVERITY_ERROR );
		}

		return false;
    }

    if( m_reporter != nullptr )
    {
        msg += wxT( "|--------------------------------\n" );
        msg += wxT( "| " );
        msg += wxString::Format( "Performing jobs" );
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

        int result = m_kiway->ProcessJob( iface, job.m_job );

        if( m_reporter )
        {
            wxString msg_fmt = wxT( "\033[32;1m%s\033[0m\n" );

            if( result == 0 )
            {
                msg = wxString::Format( msg_fmt, _( "Job successful" ) );

                successCount++;
            }
            else
            {
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
        aOutput->m_outputHandler->HandleOutputs( tempDirPath, outputs );
    }

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