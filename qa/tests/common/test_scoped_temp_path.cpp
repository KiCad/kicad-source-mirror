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

#include <wx/ffile.h>
#include <wx/filename.h>
#include <wx/log.h>

#ifndef __WINDOWS__
#include <wx/scopeguard.h>
#include <sys/stat.h>
#include <unistd.h>
#endif


BOOST_AUTO_TEST_SUITE( ScopedTempPath )


BOOST_AUTO_TEST_CASE( RemovesWhatItCreated )
{
    wxString directory;
    wxString file;

    {
        SCOPED_TEMP_PATH tempDirectory;
        SCOPED_TEMP_PATH tempFile;
        BOOST_REQUIRE( tempDirectory.MakeDirectory( wxS( "kicad_qa_scoped" ) ) );
        BOOST_REQUIRE( tempFile.MakeFile( wxS( "kicad_qa_scoped" ) ) );
        directory = tempDirectory.Path();
        file = tempFile.Path();
        BOOST_CHECK( wxFileName::DirExists( directory ) );
        BOOST_CHECK( wxFileName::FileExists( file ) );

        const wxString nested = directory + wxS( "/nested/child" );
        BOOST_REQUIRE( wxFileName::Mkdir( nested, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) );
        wxFFile content( wxFileName( nested, wxS( "content" ) ).GetFullPath(), wxS( "w" ) );
        BOOST_REQUIRE( content.IsOpened() );
    }

    BOOST_CHECK( !wxFileName::DirExists( directory ) );
    BOOST_CHECK( !wxFileName::FileExists( file ) );
}


BOOST_AUTO_TEST_CASE( DirectoriesAreUniqueAndCanBePrivate )
{
#ifndef __WINDOWS__
    const mode_t previousMask = umask( 0 );
    wxON_BLOCK_EXIT1( umask, previousMask );
#endif

    SCOPED_TEMP_PATH first;
    SCOPED_TEMP_PATH second;
    BOOST_REQUIRE( first.MakeDirectory( wxS( "kicad_qa_scoped" ), 0700 ) );
    BOOST_REQUIRE( second.MakeDirectory( wxS( "kicad_qa_scoped" ), 0700 ) );
    BOOST_CHECK( first.Path() != second.Path() );
    BOOST_CHECK( wxFileName::DirExists( first.Path() ) );
    BOOST_CHECK( wxFileName::DirExists( second.Path() ) );

#ifndef __WINDOWS__
    for( const wxString& path : { first.Path(), second.Path() } )
    {
        struct stat status;
        BOOST_REQUIRE_EQUAL( stat( path.fn_str(), &status ), 0 );
        BOOST_CHECK_EQUAL( status.st_mode & 0777, 0700 );
    }
#endif
}


BOOST_AUTO_TEST_CASE( FailedDirectoryCreationLeavesExistingFileAlone )
{
    SCOPED_TEMP_PATH existing;
    BOOST_REQUIRE( existing.MakeFile( wxS( "kicad_qa_scoped" ) ) );

    {
        wxFFile content( existing.Path(), wxS( "w" ) );
        BOOST_REQUIRE( content.IsOpened() );
        BOOST_REQUIRE( content.Write( wxS( "keep this file" ) ) );
    }

    {
        wxLogNull        noLog;
        SCOPED_TEMP_PATH failed;
        BOOST_CHECK( !failed.MakeDirectory( existing.Path() + wxS( "/scratch_" ), 0700 ) );
        BOOST_CHECK( failed.Path().IsEmpty() );
    }

    wxFFile  content( existing.Path(), wxS( "r" ) );
    wxString text;
    BOOST_REQUIRE( content.IsOpened() );
    BOOST_REQUIRE( content.ReadAll( &text ) );
    BOOST_CHECK_EQUAL( text.ToStdString(), "keep this file" );
}


#ifndef __WINDOWS__
BOOST_AUTO_TEST_CASE( DirectoryCleanupLeavesSymlinkTargetAlone )
{
    SCOPED_TEMP_PATH outside;
    BOOST_REQUIRE( outside.MakeDirectory( wxS( "kicad_qa_scoped" ), 0700 ) );
    const wxString sentinel = wxFileName( outside.Path(), wxS( "sentinel" ) ).GetFullPath();

    {
        wxFFile content( sentinel, wxS( "w" ) );
        BOOST_REQUIRE( content.IsOpened() );
        BOOST_REQUIRE( content.Write( wxS( "keep this directory" ) ) );
    }

    wxString scratchPath;

    {
        SCOPED_TEMP_PATH scratch;
        BOOST_REQUIRE( scratch.MakeDirectory( wxS( "kicad_qa_scoped" ), 0700 ) );
        scratchPath = scratch.Path();
        const wxString link = wxFileName( scratchPath, wxS( "outside" ) ).GetFullPath();
        BOOST_REQUIRE_EQUAL( symlink( outside.Path().fn_str(), link.fn_str() ), 0 );
    }

    BOOST_CHECK( !wxFileName::DirExists( scratchPath ) );
    BOOST_CHECK( wxFileName::DirExists( outside.Path() ) );
    wxFFile  content( sentinel, wxS( "r" ) );
    wxString text;
    BOOST_REQUIRE( content.IsOpened() );
    BOOST_REQUIRE( content.ReadAll( &text ) );
    BOOST_CHECK_EQUAL( text.ToStdString(), "keep this directory" );
}
#endif


BOOST_AUTO_TEST_SUITE_END()
