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
 */

#include <boost/test/unit_test.hpp>

#include <jobs/jobs_output_archive.h>

#include <wx/file.h>
#include <wx/filename.h>


BOOST_AUTO_TEST_SUITE( JobsOutputArchive )


/**
 * Scoped temporary directory used by the tests below. Removes the entire tree
 * on destruction so BOOST_REQUIRE failures cannot leak temp files.
 */
struct ScopedTempDir
{
    explicit ScopedTempDir( const wxString& aTag )
    {
        wxFileName fn;
        fn.AssignDir( wxFileName::GetTempDir() );
        fn.AppendDir( wxString::Format( wxS( "kicad_qa_%s_%p" ), aTag, this ) );

        BOOST_REQUIRE( wxFileName::Mkdir( fn.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) );
        path = fn.GetPath();
    }

    ~ScopedTempDir()
    {
        if( !path.IsEmpty() && wxFileName::DirExists( path ) )
            wxFileName::Rmdir( path, wxPATH_RMDIR_RECURSIVE );
    }

    wxString path;
};


/**
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/24162
 *
 * The archive output handler must create missing parent directories on demand,
 * matching the folder output handler. Previously, writing an archive to a path
 * such as "subdir1/subdir2/archive.zip" failed because the intermediate
 * directories did not exist when the file stream was opened.
 */
BOOST_AUTO_TEST_CASE( NestedDestinationPathCreatesParents )
{
    ScopedTempDir base( wxS( "archive_nested" ) );

    // Populate a temp source directory with a single file to be archived.
    wxString srcDir = base.path + wxFileName::GetPathSeparator() + wxS( "src" );
    BOOST_REQUIRE( wxFileName::Mkdir( srcDir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) );

    wxString srcFile = srcDir + wxFileName::GetPathSeparator() + wxS( "payload.txt" );
    {
        wxFile f;
        BOOST_REQUIRE( f.Create( srcFile, true ) );
        f.Write( wxS( "hello" ) );
    }

    // Destination uses two layers of nesting that do not yet exist.
    wxString nestedDir = base.path + wxFileName::GetPathSeparator() + wxS( "level1" )
                         + wxFileName::GetPathSeparator() + wxS( "level2" );
    wxString outPath = nestedDir + wxFileName::GetPathSeparator() + wxS( "archive.zip" );

    BOOST_REQUIRE( !wxFileName::DirExists( nestedDir ) );

    JOBS_OUTPUT_ARCHIVE archive;
    archive.SetOutputPath( outPath );

    std::vector<wxString>        noOverwriteList;
    std::vector<JOB_OUTPUT>      outputs;
    std::optional<wxString>      resolved;

    bool ok = archive.HandleOutputs( srcDir, nullptr, noOverwriteList, outputs, resolved );

    BOOST_CHECK_MESSAGE( ok, "Archive job should succeed with a nested destination path." );
    BOOST_CHECK_MESSAGE( wxFileName::DirExists( nestedDir ),
                         "Nested parent directory should have been created." );
    BOOST_REQUIRE( resolved.has_value() );
    BOOST_CHECK( wxFileName::FileExists( resolved.value() ) );
}


/**
 * Sanity check: an existing destination directory must still work after the
 * mkdir change.
 */
BOOST_AUTO_TEST_CASE( ExistingDestinationStillWorks )
{
    ScopedTempDir base( wxS( "archive_existing" ) );

    wxString srcDir = base.path + wxFileName::GetPathSeparator() + wxS( "src" );
    BOOST_REQUIRE( wxFileName::Mkdir( srcDir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) );

    wxString srcFile = srcDir + wxFileName::GetPathSeparator() + wxS( "payload.txt" );
    {
        wxFile f;
        BOOST_REQUIRE( f.Create( srcFile, true ) );
        f.Write( wxS( "hello" ) );
    }

    wxString outPath = base.path + wxFileName::GetPathSeparator() + wxS( "archive.zip" );

    JOBS_OUTPUT_ARCHIVE archive;
    archive.SetOutputPath( outPath );

    std::vector<wxString>        noOverwriteList;
    std::vector<JOB_OUTPUT>      outputs;
    std::optional<wxString>      resolved;

    bool ok = archive.HandleOutputs( srcDir, nullptr, noOverwriteList, outputs, resolved );

    BOOST_CHECK( ok );
    BOOST_REQUIRE( resolved.has_value() );
    BOOST_CHECK( wxFileName::FileExists( resolved.value() ) );
}


BOOST_AUTO_TEST_SUITE_END()
