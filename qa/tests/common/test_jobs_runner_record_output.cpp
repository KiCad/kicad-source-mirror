/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <boost/test/unit_test.hpp>

#include <qa_utils/file_utils.h>

#include <jobs/job_special_execute.h>
#include <jobs/jobset.h>
#include <jobs/jobs_output.h>
#include <jobs_runner.h>
#include <reporter.h>

#include <filesystem>
#include <fstream>


namespace
{
namespace fs = std::filesystem;

// An Execute Command job that records its output, run into a folder destination
struct RECORD_OUTPUT_FIXTURE
{
    RECORD_OUTPUT_FIXTURE() :
            m_destDir( wxS( "kicad_jobs_runner_record" ) ),
            m_jobset( wxString::FromUTF8( ( m_destDir.Path() / "record.kicad_jobset" ).string().c_str() ) )
    {
        m_execute = new JOB_SPECIAL_EXECUTE();
        m_execute->m_command = wxS( "echo recorded" );
        m_execute->m_recordOutput = true;
        m_execute->SetConfiguredOutputPath( wxS( "data/" ) );

        m_jobset.AddNewJob( wxS( "special_execute" ), m_execute );
        m_job = &m_jobset.GetJobs().front();

        JOBSET_DESTINATION* destination = m_jobset.AddNewDestination( JOBSET_DESTINATION_T::FOLDER );
        destination->m_only = { m_job->m_id };
        destination->m_outputHandler->FromJson( { { "output_path", ( m_destDir.Path() / "out" ).string() } } );
    }

    bool Run( WX_STRING_REPORTER& aReporter )
    {
        JOBS_RUNNER runner( nullptr, &m_jobset, nullptr, aReporter, nullptr );

        return runner.RunJobsForDestination( &m_jobset.GetDestinations().front() );
    }

    KI_TEST::SCOPED_TEMP_DIR m_destDir;
    JOBSET                   m_jobset;
    JOBSET_JOB*              m_job = nullptr;
    JOB_SPECIAL_EXECUTE*     m_execute = nullptr;
};
} // namespace


BOOST_FIXTURE_TEST_SUITE( JobsRunnerRecordOutput, RECORD_OUTPUT_FIXTURE )


BOOST_AUTO_TEST_CASE( ExecuteCommandRecordOutputIntoFolder )
{
    WX_STRING_REPORTER reporter;

    BOOST_CHECK( Run( reporter ) );

    fs::path logFile = m_destDir.Path() / "out" / "data" / ( m_job->m_id.ToStdString() + ".log" );
    BOOST_REQUIRE( fs::is_regular_file( logFile ) );

    std::ifstream log( logFile );
    std::string   line;
    std::getline( log, line );
    BOOST_CHECK( line.starts_with( "recorded" ) );
}


// The default log name is not written back into the job a user saves
BOOST_AUTO_TEST_CASE( ExecuteCommandDefaultLogNotStored )
{
    m_execute->SetConfiguredOutputPath( wxEmptyString );

    WX_STRING_REPORTER reporter;

    BOOST_CHECK( Run( reporter ) );
    BOOST_CHECK( fs::is_regular_file( m_destDir.Path() / "out" / ( m_job->m_id.ToStdString() + ".log" ) ) );
    BOOST_CHECK_MESSAGE( m_execute->GetConfiguredOutputPath().IsEmpty(), m_execute->GetConfiguredOutputPath() );
}


// A log that cannot be created is named in the error
BOOST_AUTO_TEST_CASE( ExecuteCommandRecordOutputFailureNamesLog )
{
    fs::path blocker = m_destDir.Path() / "blocker";
    std::ofstream( blocker ).put( 'x' );

    wxString logPath = wxString::FromUTF8( ( blocker / "run.log" ).string().c_str() );
    m_execute->SetConfiguredOutputPath( logPath );

    WX_STRING_REPORTER reporter;

    BOOST_CHECK( !Run( reporter ) );
    BOOST_CHECK_MESSAGE( reporter.GetMessages().Contains( logPath ), reporter.GetMessages() );
}


// Only Linux has a device that accepts the open and rejects every write
#ifdef __linux__
BOOST_AUTO_TEST_CASE( ExecuteCommandRecordOutputWriteFailureNamesLog )
{
    m_execute->SetConfiguredOutputPath( wxS( "/dev/full" ) );

    WX_STRING_REPORTER reporter;

    BOOST_CHECK( !Run( reporter ) );
    BOOST_CHECK_MESSAGE( reporter.GetMessages().Contains( wxS( "/dev/full" ) ), reporter.GetMessages() );
}
#endif


BOOST_AUTO_TEST_SUITE_END()
