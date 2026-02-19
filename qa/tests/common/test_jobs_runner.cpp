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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <boost/test/unit_test.hpp>

#include <wx/process.h>
#include <wx/txtstrm.h>
#include <wx/filename.h>
#include <wx/dir.h>
#include <wx/file.h>

#ifdef _WIN32
#include <process.h>
#endif


BOOST_AUTO_TEST_SUITE( JobsRunner )


/**
 * Helper that mirrors the shell-wrapping execution pattern used in JOBS_RUNNER::runSpecialExecute.
 * Returns the exit code and captures stdout into aOutput.
 */
static int executeViaShell( const wxString& aCmd, wxString& aOutput )
{
    wxProcess process;
    process.Redirect();

#ifdef __WXMSW__
    const wxString shell = wxS( "cmd.exe" );
    const wxString shellFlag = wxS( "/c" );
#else
    const wxString shell = wxS( "/bin/sh" );
    const wxString shellFlag = wxS( "-c" );
#endif

    const wchar_t* argv[] = { shell.wc_str(), shellFlag.wc_str(), aCmd.wc_str(), nullptr };

    int result = static_cast<int>(
            wxExecute( const_cast<wchar_t**>( argv ), wxEXEC_SYNC, &process ) );

    wxInputStream* inputStream = process.GetInputStream();

    if( inputStream )
    {
        wxTextInputStream textStream( *inputStream );

        while( !inputStream->Eof() )
        {
            wxString line = textStream.ReadLine();

            if( !line.IsEmpty() || !inputStream->Eof() )
            {
                if( !aOutput.IsEmpty() )
                    aOutput += wxS( "\n" );

                aOutput += line;
            }
        }
    }

    return result;
}


BOOST_AUTO_TEST_CASE( SimpleCommand )
{
    wxString output;
    int result = executeViaShell( wxS( "echo hello" ), output );

    BOOST_CHECK_EQUAL( result, 0 );
    BOOST_CHECK( output.Contains( wxS( "hello" ) ) );
}

#ifdef _WIN32
#define getpid _getpid
#endif

BOOST_AUTO_TEST_CASE( GlobExpansion )
{
    wxString tempDir = wxFileName::GetTempDir() + wxFileName::GetPathSeparator()
                       + wxS( "kicad_test_glob_" ) + wxString::Format( wxS( "%d" ), (int) getpid() );

    BOOST_REQUIRE( wxFileName::Mkdir( tempDir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) );

    wxFile file1;
    wxFile file2;
    wxString path1 = tempDir + wxFileName::GetPathSeparator() + wxS( "file_a.txt" );
    wxString path2 = tempDir + wxFileName::GetPathSeparator() + wxS( "file_b.txt" );

    file1.Create( path1 );
    file1.Write( wxS( "content_a" ) );
    file1.Close();

    file2.Create( path2 );
    file2.Write( wxS( "content_b" ) );
    file2.Close();

    // Use ls with glob. Without shell interpretation, the glob would not expand.
#ifdef __WXMSW__
    wxString cmd = wxString::Format( wxS( "dir /b \"%s\\*.txt\"" ), tempDir );
#else
    wxString cmd = wxString::Format( wxS( "ls %s/*.txt" ), tempDir );
#endif

    wxString output;
    int result = executeViaShell( cmd, output );

    BOOST_CHECK_EQUAL( result, 0 );
    BOOST_CHECK_MESSAGE( output.Contains( wxS( "file_a.txt" ) ),
                         "Glob should expand to include file_a.txt, got: " + output );
    BOOST_CHECK_MESSAGE( output.Contains( wxS( "file_b.txt" ) ),
                         "Glob should expand to include file_b.txt, got: " + output );

    wxRemoveFile( path1 );
    wxRemoveFile( path2 );
    wxRmdir( tempDir );
}


BOOST_AUTO_TEST_CASE( PipeSupport )
{
#ifdef __WXMSW__
    wxString cmd = wxS( "echo hello world | findstr hello" );
#else
    wxString cmd = wxS( "echo hello world | grep hello" );
#endif

    wxString output;
    int result = executeViaShell( cmd, output );

    BOOST_CHECK_EQUAL( result, 0 );
    BOOST_CHECK( output.Contains( wxS( "hello" ) ) );
}


BOOST_AUTO_TEST_CASE( ExitCodePropagation )
{
#ifdef __WXMSW__
    wxString cmd = wxS( "cmd /c exit 42" );
#else
    wxString cmd = wxS( "exit 42" );
#endif

    wxString output;
    int result = executeViaShell( cmd, output );

    BOOST_CHECK_EQUAL( result, 42 );
}


BOOST_AUTO_TEST_CASE( CommandWithSingleQuotes )
{
#ifndef __WXMSW__
    wxString cmd = wxS( "echo \"it's working\"" );
    wxString output;
    int result = executeViaShell( cmd, output );

    BOOST_CHECK_EQUAL( result, 0 );
    BOOST_CHECK_MESSAGE( output.Contains( wxS( "it's working" ) ),
                         "Should handle single quotes in output, got: " + output );
#endif
}


BOOST_AUTO_TEST_SUITE_END()
