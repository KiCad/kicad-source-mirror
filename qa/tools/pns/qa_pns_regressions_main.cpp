/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers.
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#define BOOST_TEST_NO_MAIN

#include <wx/cmdline.h>
#include <wx/stdstream.h>
#include <wx/wfstream.h>

#include <qa_utils/utility_registry.h>
#include <pcbnew_utils/board_test_utils.h>
#include <pcbnew_utils/board_file_utils.h>

#include "pns_log_file.h"
#include "pns_log_viewer_frame.h"

#include <boost/test/included/unit_test.hpp>

using namespace boost::unit_test;

#if 0

bool runSingleTest( REPORTER* aReporter, wxString name, wxString testDirPath )
{
    PNS_LOG_FILE   logFile;
    PNS_LOG_PLAYER player;


    wxString fn( testDirPath );
    fn = fn.Append( "/" );
    fn = fn.Append( name );
    fn = fn.Append( "/pns" );


    wxFileName fname( fn );

    aReporter->Report( wxString::Format( "Running test '%s' from '%s'", name, fname.GetFullPath() ),
                       RPT_SEVERITY_INFO );

    //fname.A

    if( !logFile.Load( fname, aReporter ) )
    {
        aReporter->Report(
                wxString::Format( "Failed to load test '%s' from '%s'", name, fname.GetFullPath() ),
                RPT_SEVERITY_ERROR );
    }

    player.SetReporter( aReporter );
    player.ReplayLog( &logFile, 0 );
    bool pass = player.CompareResults( &logFile );
    return pass;
}


int main( int argc, char* argv[] )
{


    int passed = 0, failed = 0;

    for( auto casename : testCases )
    {
        if( runSingleTest( &reporter, casename, cl_parser.GetParam( 0 ) ) )
            passed++;
        else
            failed++;
    }

    reporter.Report(
            wxString::Format( "SUMMARY: %d/%d tests cases PASSED", passed,
                              (int) testCases.size() ),
            RPT_SEVERITY_INFO );


    return failed ? -1 : 0;
}
#endif

struct PNS_TEST_CASE
{
    PNS_TEST_CASE( std::string name, std::string path ) : m_name( name ), m_dataPath( path ){};

    std::string GetDataPath() const { return m_dataPath; }
    std::string GetName() const { return m_name; }
    std::string m_dataPath;
    std::string m_name;
};


class FIXTURE_LOGGER
{
public:
    FIXTURE_LOGGER()
    {
        m_Log = new KI_TEST::CONSOLE_LOG;
        m_Reporter = new KI_TEST::CONSOLE_MSG_REPORTER( m_Log );
    }

    ~FIXTURE_LOGGER()
    {
        delete m_Reporter;
        delete m_Log;
    }

    KI_TEST::CONSOLE_LOG*          m_Log;
    KI_TEST::CONSOLE_MSG_REPORTER* m_Reporter;
};


class PNS_TEST_FIXTURE
{
public:
    static void RunTest( PNS_TEST_CASE* aTestData )
    {
        //  printf( "Run %s\n", aTestData->GetName().c_str() );

        PNS_LOG_FILE   logFile;
        PNS_LOG_PLAYER player;

        if( !logFile.Load( wxString( aTestData->GetDataPath() ), m_logger.m_Reporter ) )
        {
            m_logger.m_Reporter->Report( wxString::Format( "Failed to load test '%s' from '%s'",
                                                  aTestData->GetName(), aTestData->GetDataPath() ),
                                RPT_SEVERITY_ERROR );
        }

        player.SetReporter( m_logger.m_Reporter );
        player.ReplayLog( &logFile, 0 );
        bool pass = player.CompareResults( &logFile );

        if( !pass )
            BOOST_TEST_FAIL( "replay results inconsistent with reference reslts" );
    }

    static FIXTURE_LOGGER m_logger;
};


FIXTURE_LOGGER PNS_TEST_FIXTURE::m_logger;


std::vector<PNS_TEST_CASE*> createTestCases()
{
    std::string absPath = KI_TEST::GetPcbnewTestDataDir() + std::string( "/pns_regressions/" );
    std::vector<PNS_TEST_CASE*> testCases;

    wxFileName fnameList( absPath + "tests.lst" );
    wxTextFile fp( fnameList.GetFullPath() );

    if( !fp.Open() )
    {
        wxString str =
                wxString::Format( "Failed to load test list from '%s'.", fnameList.GetFullPath() );
        BOOST_TEST_ERROR( str.c_str().AsChar() );
        return testCases;
    }

    std::vector<wxString> lines;

    if( !fp.Eof() )
    {
        lines.push_back( fp.GetFirstLine() );

        while( !fp.Eof() )
        {
            auto l = fp.GetNextLine();
            if( l.Length() > 0 )
            {
                lines.push_back( l );
            }
        }
    }

    fp.Close();

    for( auto l : lines )
    {
        wxString fn( absPath );
        fn = fn.Append( wxT( "/" ) );
        fn = fn.Append( l );
        fn = fn.Append( wxT( "/pns" ) );

        testCases.push_back( new PNS_TEST_CASE( l.ToStdString(), fn.ToStdString() ) );
    }

    wxString str = wxString::Format( "Loaded %d test cases from '%s'.", (int) testCases.size(),
                                     fnameList.GetFullPath() );

    BOOST_TEST_MESSAGE( str.c_str().AsChar() );

    return testCases;
}

static test_suite* init_pns_test_suite( int argc, char* argv[] )
{
    test_suite* pnsTestSuite = BOOST_TEST_SUITE( "pns_regressions" );

    auto testCases = createTestCases();

    for( auto& c : testCases )
    {
        pnsTestSuite->add(
                BOOST_TEST_CASE_NAME( std::bind( &PNS_TEST_FIXTURE::RunTest, c ), c->GetName() ) );
    }

    framework::master_test_suite().add( pnsTestSuite );
    return 0;
}

int main( int argc, char* argv[] )
{
    return unit_test_main( init_pns_test_suite, argc, argv );
}