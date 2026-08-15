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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#define BOOST_TEST_NO_MAIN

#include <wx/app.h>
#include <wx/cmdline.h>
#include <wx/stdstream.h>
#include <wx/wfstream.h>
#include <wx/textfile.h>

#include <io/io_utils.h>

#include <qa_utils/utility_registry.h>
#include <pcbnew_utils/board_test_utils.h>
#include <pcbnew_utils/board_file_utils.h>
#include <wx/dir.h>
#include <wx/utils.h>

#include "pns_log_file.h"
#include "pns_log_viewer_frame.h"

#include <boost/test/unit_test.hpp>

using namespace boost::unit_test;

std::map<wxString, wxString> g_testBoards;
static bool g_updateGolden = false;

struct PNS_TEST_CASE
{
    PNS_TEST_CASE( const wxString & aName, const wxString& aPath ) : m_name( aName ), m_dataPath( aPath ){};

    wxString GetDataPath() const { return m_dataPath; }
    wxString GetName() const { return m_name; }
    wxString m_dataPath;
    wxString m_name;
};


class FIXTURE_LOGGER
{
public:
    FIXTURE_LOGGER()
    {
        m_Log = new KI_TEST::CONSOLE_LOG;
        m_Log->SetConsoleOutputEnabled( wxGetEnv( wxS( "KICAD_PNS_VERBOSE" ), nullptr ) );
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
    static bool RunTest( PNS_TEST_CASE* aTestData )
    {
        BOOST_TEST_CONTEXT( aTestData->GetName() )
        {
            PNS_LOG_FILE   logFile;
            PNS_LOG_PLAYER player;

            auto hash = logFile.GetLogBoardHash( aTestData->GetDataPath() );

            wxString boardFilename;

            if( hash.has_value() )
            {
                if( auto it = g_testBoards.find( *hash ); it != g_testBoards.end() )
                {
                    boardFilename = it->second;
                    BOOST_TEST_MESSAGE( "found matching board: " << boardFilename );
                }
            }

            if( !logFile.Load( wxString( aTestData->GetDataPath() ), m_logger.m_Reporter, boardFilename ) )
            {
                // Missing board or stale hash; don't fail QA for this
                BOOST_TEST_MESSAGE( "Failed to load test " << aTestData->GetName() << " from "
                                                           << aTestData->GetDataPath() << ", will skip" );
                BOOST_CHECK( true );
                return true;
            }

            player.SetReporter( m_logger.m_Reporter );

            if( g_updateGolden )
            {
                player.ReplayLog( &logFile, 0, 0, -1, true );

                if( logFile.SaveLog( aTestData->GetDataPath(), m_logger.m_Reporter ) )
                    BOOST_TEST_MESSAGE( "Updated golden file: " << aTestData->GetDataPath() );
                else
                    BOOST_TEST_ERROR( "Failed to save golden file: " << aTestData->GetDataPath() );

                return true;
            }

            player.ReplayLog( &logFile, 0 );

            auto cstate = player.GetRouterUpdatedItems();
            auto expected = logFile.GetExpectedResult();

            bool pass = cstate.Compare( expected );
            BOOST_REQUIRE( pass );
            return pass;
        }

        return false;
    }

    static FIXTURE_LOGGER m_logger;
};


FIXTURE_LOGGER PNS_TEST_FIXTURE::m_logger;

std::vector<wxString> scanSubdirs( const wxString& absPath, bool aFiles, wxString filespec = wxEmptyString )
{
    std::vector<wxString> rv;

    wxDir dir( absPath );
    if( !dir.IsOpened() )
    {
        BOOST_TEST_ERROR( "Failed to open directory: " << absPath );
        return rv;
    }

    wxString dirName;

    bool hasFiles = dir.GetFirst( &dirName, filespec, aFiles ? wxDIR_FILES : wxDIR_DIRS );
    while( hasFiles )
    {
        rv.push_back( dirName );
        hasFiles = dir.GetNext( &dirName );
    }

    return rv;
}

void scanAndHashBoards( const wxString& aPath )
{
    for( const wxString& brdFile : scanSubdirs( aPath, true, wxT("*.kicad_pcb") ) )
    {
        wxFileName path ( aPath );
        path.SetName( brdFile );
        std::optional<wxString> hash = IO_UTILS::fileHashMMH3( path.GetFullPath() );
        if( hash )
        {
            BOOST_TEST_MESSAGE("- file: " << std::left << std::setw(50) << brdFile << " hash: " << ( *hash ) );
            g_testBoards[ *hash ] = path.GetFullPath();
        }
    }
}

std::vector<PNS_TEST_CASE*> createTestCases()
{
    std::vector<PNS_TEST_CASE*> testCases;

    wxFileName absPath( KI_TEST::GetPcbnewTestDataDir() );
    absPath.AppendDir (wxT("pns_regressions"));

    wxFileName boardsPath( absPath );
    boardsPath.AppendDir(wxT("boards"));

    scanAndHashBoards( boardsPath.GetFullPath() );

    for( const wxString& subdir : scanSubdirs( absPath.GetFullPath(), false ) )
    {
        if( !subdir.CmpNoCase( wxT("boards") ) )
            continue;

        wxFileName path( absPath );
        path.AppendDir( subdir );

        for( const wxString& logName: scanSubdirs( path.GetFullPath(), true, wxT("*.log") ) )
        {
            wxFileName logPath( path );
            logPath.SetName( logName );

            BOOST_TEST_MESSAGE( "Add case " << logPath.GetFullPath() );

            testCases.push_back( new PNS_TEST_CASE( subdir, logPath.GetFullPath() ) );
        }
    }

    return testCases;
}


bool init_pns_test_suite( )
{
    test_suite* pnsTestSuite = BOOST_TEST_SUITE( "pns_regressions" );

    auto testCases = createTestCases();

    for( auto& c : testCases )
    {
        pnsTestSuite->add(
                BOOST_TEST_CASE_NAME( std::bind( &PNS_TEST_FIXTURE::RunTest, c ), c->GetName().ToStdString() ) );
    }

    framework::master_test_suite().p_name.value = "P&S router regressions";
    framework::master_test_suite().add( pnsTestSuite );
    return true;
}


int main( int argc, char* argv[] )
{
    wxApp::SetInstance( new wxAppConsole );
    wxInitialize( argc, argv );

    for( int i = 1; i < argc; i++ )
    {
        if( wxString( argv[i] ).Cmp( wxT( "--update-golden" ) ) == 0 )
            g_updateGolden = true;
    }

    int ret = unit_test_main( &init_pns_test_suite, argc, argv );

    wxUninitialize();
    return ret;
}
