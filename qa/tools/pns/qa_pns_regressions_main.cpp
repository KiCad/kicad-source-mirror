/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020-2022 KiCad Developers.
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

#include <wx/cmdline.h>
#include <wx/stdstream.h>
#include <wx/wfstream.h>

#include <qa_utils/utility_registry.h>
#include <pcbnew_utils/board_test_utils.h>

#include "pns_log_file.h"
#include "pns_log_viewer_frame.h"


static const wxCmdLineEntryDesc g_cmdLineDesc[] = {
    {
            wxCMD_LINE_SWITCH,
            "h",
            "help",
            _( "displays help on the command line parameters" ).mb_str(),
            wxCMD_LINE_VAL_NONE,
            wxCMD_LINE_OPTION_HELP,
    },
    {
            wxCMD_LINE_OPTION,
            "i",
            "iteration-limit",
            _( "Max number of iterations" ).mb_str(),
            wxCMD_LINE_VAL_NUMBER,
            wxCMD_LINE_PARAM_OPTIONAL,
    },
    {
            wxCMD_LINE_OPTION,
            "s",
            "steps-limit",
            _( "execute log up to steps-limit events" ).mb_str(),
            wxCMD_LINE_VAL_NUMBER,
            wxCMD_LINE_PARAM_OPTIONAL,
    },
    {
            wxCMD_LINE_PARAM,
            "directory with tests",
            "directory with tests",
            _( "directory with tests (containing tests.lst)" ).mb_str(),
            wxCMD_LINE_VAL_STRING,
            wxCMD_LINE_OPTION_MANDATORY,
    },
    { wxCMD_LINE_NONE }
};

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
    wxCmdLineParser cl_parser( argc, argv );

    cl_parser.SetDesc( g_cmdLineDesc );
    cl_parser.AddUsageText( _( "P&S Regression Test Suite. Compares live router results against "
                               "prerecorded references." ) );

    int cmd_parsed_ok = cl_parser.Parse();

    if( cl_parser.Found( "help" ) )
    {
        return 0;
    }

    if( cmd_parsed_ok != 0 )
    {
        printf( "P&S Regression Test Suite. For command line options, call %s -h.\n\n", argv[0] );
        // Help and invalid input both stop here
        return ( cmd_parsed_ok == -1 ) ? KI_TEST::RET_CODES::OK : KI_TEST::RET_CODES::BAD_CMDLINE;
    }

    wxString dirname;

    long iter_limit = 256;
    long steps_limit = -1;
    cl_parser.Found( "iteration-limit", &iter_limit );
    cl_parser.Found( "steps-limit", &steps_limit );

    wxFileName fname_list( cl_parser.GetParam( 0 ), "tests.lst" );

    KI_TEST::CONSOLE_LOG          log;
    KI_TEST::CONSOLE_MSG_REPORTER reporter( &log );


    wxTextFile fp( fname_list.GetFullPath() );


    if( !fp.Open() )
    {
        reporter.Report(
                wxString::Format( "Failed to load test list from '%s'.", fname_list.GetFullPath() ),
                RPT_SEVERITY_ERROR );
        return -1;
    }

    std::vector<wxString> testCases;

    if( !fp.Eof() )
    {
        testCases.push_back( fp.GetFirstLine() );
        while( !fp.Eof() )
        {
            auto l = fp.GetNextLine();
            if( l.Length() > 0 )
                testCases.push_back( l );
        }
    }

    fp.Close();

    reporter.Report( wxString::Format( "Loaded %d test cases from '%s'.", (int) testCases.size(),
                                       fname_list.GetFullPath() ),
                     RPT_SEVERITY_INFO );

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
