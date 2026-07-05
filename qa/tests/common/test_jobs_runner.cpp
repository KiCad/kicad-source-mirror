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

#include <gestfich.h>

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
 * Drive the production ExecuteCommandThroughShell (the exact function jobsets use) and capture its
 * stdout so the shell-wrapping behaviour is tested directly rather than through a copy.
 */
static int executeViaShell( const wxString& aCmd, wxString& aOutput )
{
    wxProcess process;
    process.Redirect();

    int result = ExecuteCommandThroughShell( aCmd, &process );

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


#ifdef __WXMSW__
BOOST_AUTO_TEST_CASE( CommandWithQuotedArgument )
{
    // Regression test for a Windows-only bug where a user-supplied command containing
    // double-quoted arguments with spaces was corrupted by the argv-array form of wxExecute,
    // which re-escapes embedded " as \" (see wxExecuteImpl in wxWidgets' src/msw/utilsexc.cpp).
    // cmd.exe does not honour backslash-escaped quotes, so this broke any quoted executable
    // path containing spaces (e.g. "C:\Program Files\KiCad\10.0\bin\python.exe" --version).
    // Under the buggy code path, cmd.exe's `echo` would emit \"hello world\"; with the fix
    // the user's quoting is preserved and the output contains the original "hello world".
    wxString cmd = wxS( "echo \"hello world\"" );
    wxString output;
    int      result = executeViaShell( cmd, output );

    BOOST_CHECK_EQUAL( result, 0 );
    BOOST_CHECK_MESSAGE( output.Contains( wxS( "\"hello world\"" ) ),
                         "Quoted argument should pass through verbatim, got: " + output );
}
#endif


/**
 * Regression for https://gitlab.com/kicad/code/kicad/-/issues/24226
 *
 * An absolute path containing spaces, quoted by the user, must reach the interpreter intact. The
 * old array-form invocation on Windows let wxExecute backslash-escape the quotes and cmd.exe then
 * mangled the path. Run an interpreter located at a spaced absolute path against a script that
 * prints a sentinel and verify the sentinel comes back.
 */
BOOST_AUTO_TEST_CASE( AbsolutePathWithSpaces )
{
    wxString baseDir = wxFileName::GetTempDir() + wxFileName::GetPathSeparator()
                       + wxS( "kicad test 24226 " ) + wxString::Format( wxS( "%d" ), (int) getpid() );

    BOOST_REQUIRE( wxFileName::Mkdir( baseDir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) );

    wxString sentinel = wxS( "SENTINEL_24226" );

    // A second spaced absolute path passed as an argument exercises the nested-quote shape
    // cmd /d /s /c ""exe path" "arg path"". The script only emits the sentinel when it receives
    // this argument intact, so argument quoting is validated, not just the executable path.
    wxString argPath = baseDir + wxFileName::GetPathSeparator() + wxS( "script input.txt" );
    wxFile   argFile;
    BOOST_REQUIRE( argFile.Create( argPath, true ) );
    argFile.Close();

    wxFile script;

#ifdef __WXMSW__
    // Write a batch file at a spaced absolute path and invoke it through its quoted absolute path.
    wxString scriptPath = baseDir + wxFileName::GetPathSeparator() + wxS( "print sentinel.bat" );
    BOOST_REQUIRE( script.Create( scriptPath, true ) );
    BOOST_REQUIRE( script.Write( wxS( "@echo off\r\nif \"%~1\"==\"" ) + argPath + wxS( "\" echo " )
                                 + sentinel + wxS( "\r\n" ) ) );
    script.Close();

    wxString cmd = wxS( "\"" ) + scriptPath + wxS( "\" \"" ) + argPath + wxS( "\"" );
#else
    // Write a shell script at a spaced absolute path and invoke it through its quoted absolute path.
    wxString scriptPath = baseDir + wxFileName::GetPathSeparator() + wxS( "print sentinel.sh" );
    BOOST_REQUIRE( script.Create( scriptPath, true ) );
    BOOST_REQUIRE( script.Write( wxS( "#!/bin/sh\n[ \"$1\" = \"" ) + argPath + wxS( "\" ] && echo " )
                                 + sentinel + wxS( "\n" ) ) );
    script.Close();
    BOOST_REQUIRE( wxFileName( scriptPath ).SetPermissions( wxPOSIX_USER_READ | wxPOSIX_USER_WRITE
                                                            | wxPOSIX_USER_EXECUTE ) );

    wxString cmd = wxS( "\"" ) + scriptPath + wxS( "\" \"" ) + argPath + wxS( "\"" );
#endif

    wxString output;
    int      result = executeViaShell( cmd, output );

    BOOST_CHECK_EQUAL( result, 0 );
    BOOST_CHECK_MESSAGE( output.Contains( sentinel ),
                         "Quoted absolute paths with spaces should reach the interpreter intact, "
                         "got: " + output );

    wxRemoveFile( argPath );
    wxRemoveFile( scriptPath );
    wxRmdir( baseDir );
}


BOOST_AUTO_TEST_SUITE_END()
