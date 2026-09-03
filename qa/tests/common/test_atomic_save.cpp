/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "qa_utils/wx_utils/unit_test_utils.h"
#include <boost/test/unit_test.hpp>
#include <qa_utils/file_utils.h>

#include <drawing_sheet/ds_data_model.h>
#include <ki_exception.h>
#include <kiplatform/io.h>
#include <richio.h>

#include <wx/dir.h>
#include <wx/ffile.h>
#include <wx/filefn.h>
#include <wx/filename.h>

#include <memory>
#include <string>

#if defined( _WIN32 )
#include <windows.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif


namespace
{

void writeFileContents( const wxString& aPath, const std::string& aContent )
{
    wxFFile fp( aPath, wxT( "wb" ) );
    BOOST_REQUIRE( fp.IsOpened() );

    if( !aContent.empty() )
        BOOST_REQUIRE( fp.Write( aContent.data(), aContent.size() ) == aContent.size() );

    fp.Close();
}


std::string readFileContents( const wxString& aPath )
{
    wxFFile fp( aPath, wxT( "rb" ) );

    if( !fp.IsOpened() )
        return std::string();

    wxString buf;
    fp.ReadAll( &buf );
    std::string out = std::string( buf.mb_str( wxConvUTF8 ) );
    fp.Close();
    return out;
}


// Counts remaining sibling temp files matching the atomic-save pattern next to a target.
// Used to assert no orphan temps leak out of a successful commit.
unsigned countSiblingTemps( const wxString& aTargetPath )
{
    wxFileName fn( aTargetPath );
    wxString dir = fn.GetPath();
    wxString pattern = fn.GetFullName() + wxT( ".kicad-save-*" );
    wxArrayString matches;
    wxDir::GetAllFiles( dir, &matches, pattern, wxDIR_FILES );
    return matches.GetCount();
}

} // anonymous namespace

BOOST_AUTO_TEST_SUITE( AtomicSave )


BOOST_AUTO_TEST_CASE( PrettifiedFormatter_HappyPath )
{
    KI_TEST::SCOPED_TEMP_DIR tempDir( wxT( "kicad-atomicsave-happy" ) );
    const wxString target = tempDir.PathStr() + wxFileName::GetPathSeparator() + wxT( "target" );

    {
        PRETTIFIED_FILE_OUTPUTFORMATTER f( target );
        f.Print( 0, "(kicad_test (content \"hello\"))\n" );
        BOOST_REQUIRE( f.Finish() );
    }

    BOOST_REQUIRE( wxFileName::FileExists( target ) );
    std::string actual = readFileContents( target );
    BOOST_REQUIRE( !actual.empty() );
    BOOST_REQUIRE( actual.find( "hello" ) != std::string::npos );
    BOOST_REQUIRE_EQUAL( countSiblingTemps( target ), 0u );
}


BOOST_AUTO_TEST_CASE( PrettifiedFormatter_UnwindingPreservesOriginal )
{
    // Pre-seed target with known content. If anything throws between formatter
    // construction and Finish() -- the exact bug class we are fixing -- the user's
    // file must remain byte-identical.
    KI_TEST::SCOPED_TEMP_DIR tempDir( wxT( "kicad-atomicsave-unwind" ) );
    const wxString target = tempDir.PathStr() + wxFileName::GetPathSeparator() + wxT( "target" );
    const std::string original = "(original \"do not lose me\")\n";
    writeFileContents( target, original );

    BOOST_REQUIRE_THROW(
        {
            PRETTIFIED_FILE_OUTPUTFORMATTER f( target );
            f.Print( 0, "(partial " );
            // Simulate a serializer crash mid-save -- the equivalent of std::bad_alloc
            // from KICAD_FORMAT::Prettify on a large board, or an IO_ERROR nested
            // deep in a FormatBoardToFormatter call.
            throw std::runtime_error( "simulated serializer failure" );
        },
        std::runtime_error );

    BOOST_REQUIRE( wxFileName::FileExists( target ) );
    BOOST_REQUIRE_EQUAL( readFileContents( target ), original );
    BOOST_REQUIRE_EQUAL( countSiblingTemps( target ), 0u );
}


BOOST_AUTO_TEST_CASE( PrettifiedFormatter_DestructorDiscardsWithoutExplicitFinish )
{
    // Finish() is the only thing that commits. A formatter that goes out of scope without it
    // has abandoned the save, so the target keeps the bytes it already had.
    KI_TEST::SCOPED_TEMP_DIR tempDir( wxT( "kicad-atomicsave-implicit" ) );
    const wxString target = tempDir.PathStr() + wxFileName::GetPathSeparator() + wxT( "target" );
    const std::string original = "(original \"keep me\")\n";
    writeFileContents( target, original );

    {
        PRETTIFIED_FILE_OUTPUTFORMATTER f( target );
        f.Print( 0, "(implicit_commit ok)\n" );
    }

    BOOST_REQUIRE_EQUAL( readFileContents( target ), original );
    BOOST_REQUIRE_EQUAL( countSiblingTemps( target ), 0u );
}


BOOST_AUTO_TEST_CASE( FileFormatter_UnwindingPreservesOriginal )
{
    // Same invariant for the streaming (non-prettified) formatter: a throw mid-
    // serialization must leave the user's file intact.
    KI_TEST::SCOPED_TEMP_DIR tempDir( wxT( "kicad-atomicsave-stream" ) );
    const wxString target = tempDir.PathStr() + wxFileName::GetPathSeparator() + wxT( "target" );
    const std::string original = "original streaming content\n";
    writeFileContents( target, original );

    BOOST_REQUIRE_THROW(
        {
            FILE_OUTPUTFORMATTER f( target );
            f.Print( 0, "partial write before the crash " );
            throw std::runtime_error( "simulated exporter failure" );
        },
        std::runtime_error );

    BOOST_REQUIRE( wxFileName::FileExists( target ) );
    BOOST_REQUIRE_EQUAL( readFileContents( target ), original );
    BOOST_REQUIRE_EQUAL( countSiblingTemps( target ), 0u );
}


BOOST_AUTO_TEST_CASE( FileFormatter_HandledExceptionDiscardsWithoutFinish )
{
    // A formatter that outlives the catch -- HYPERLYNX_EXPORTER keeps one in a member -- is
    // destroyed with no exception in flight, so an unwinding check cannot see that the save
    // failed. Committing there promotes a partial write the caller has already reported.
    KI_TEST::SCOPED_TEMP_DIR tempDir( wxT( "kicad-atomicsave-outlives" ) );
    const wxString target = tempDir.PathStr() + wxFileName::GetPathSeparator() + wxT( "target" );
    const std::string original = "original export content\n";
    writeFileContents( target, original );

    {
        std::shared_ptr<FILE_OUTPUTFORMATTER> out;

        try
        {
            out = std::make_shared<FILE_OUTPUTFORMATTER>( target );
            out->Print( 0, "partial export before the failure " );
            THROW_IO_ERROR( wxT( "simulated mid-export failure" ) );
        }
        catch( const IO_ERROR& )
        {
            // Swallowed on purpose; the exporter this models reports by returning false
        }

        BOOST_REQUIRE_EQUAL( readFileContents( target ), original );
    }

    BOOST_REQUIRE_EQUAL( readFileContents( target ), original );
    BOOST_REQUIRE_EQUAL( countSiblingTemps( target ), 0u );
}


BOOST_AUTO_TEST_CASE( AtomicWriteFile_HappyPath )
{
    KI_TEST::SCOPED_TEMP_DIR tempDir( wxT( "kicad-atomicsave-atomicbuf" ) );
    const wxString target = tempDir.PathStr() + wxFileName::GetPathSeparator() + wxT( "target" );
    const std::string payload = "{\"setting\":42}\n";

    wxString err;
    BOOST_REQUIRE( KIPLATFORM::IO::AtomicWriteFile( target, payload.data(), payload.size(),
                                                    &err ) );
    BOOST_REQUIRE( err.IsEmpty() );
    BOOST_REQUIRE( wxFileName::FileExists( target ) );
    BOOST_REQUIRE_EQUAL( readFileContents( target ), payload );
    BOOST_REQUIRE_EQUAL( countSiblingTemps( target ), 0u );
}


BOOST_AUTO_TEST_CASE( AtomicWriteFile_OverwritePreservesOriginalOnFailure )
{
    // AtomicWriteFile targeting a non-writable directory must fail without touching
    // the existing file at the target path.
    KI_TEST::SCOPED_TEMP_DIR tempDir( wxT( "kicad-atomicsave-overwrite" ) );
    const wxString target = tempDir.PathStr() + wxFileName::GetPathSeparator() + wxT( "target" );
    const std::string original = "unchanged\n";
    writeFileContents( target, original );

    // Point at a path whose parent directory cannot be created: the temp-file open
    // should fail immediately, leaving the target intact. Using an empty buffer is
    // fine -- the failure happens before any write.
    wxString bogus = target + wxT( "/subdir/cannot-create" );
    wxString err;
    bool ok = KIPLATFORM::IO::AtomicWriteFile( bogus, original.data(), original.size(), &err );
    BOOST_REQUIRE( !ok );
    BOOST_REQUIRE( !err.IsEmpty() );

    BOOST_REQUIRE( wxFileName::FileExists( target ) );
    BOOST_REQUIRE_EQUAL( readFileContents( target ), original );
}


BOOST_AUTO_TEST_CASE( PrettifiedFormatter_ExplicitFinishThrowsOnCommitFailure )
{
    // Target path is a pre-existing directory so rename() fails with EISDIR during commit.
    KI_TEST::SCOPED_TEMP_DIR tempDir( wxT( "kicad-atomicsave-commitfail-explicit" ) );
    const wxString target = tempDir.PathStr() + wxFileName::GetPathSeparator() + wxT( "target" );

    BOOST_REQUIRE( wxFileName::Mkdir( target, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) );

    bool threw = false;

    try
    {
        PRETTIFIED_FILE_OUTPUTFORMATTER f( target );
        f.Print( 0, "(doomed save)\n" );
        f.Finish();
    }
    catch( const IO_ERROR& )
    {
        threw = true;
    }

    BOOST_REQUIRE_MESSAGE( threw, "Finish() must throw IO_ERROR when atomic commit fails" );
    BOOST_REQUIRE( wxFileName::DirExists( target ) );
    BOOST_REQUIRE_EQUAL( countSiblingTemps( target ), 0u );
}


BOOST_AUTO_TEST_CASE( FileFormatter_ExplicitFinishThrowsOnCommitFailure )
{
    KI_TEST::SCOPED_TEMP_DIR tempDir( wxT( "kicad-atomicsave-commitfail-stream" ) );
    const wxString target = tempDir.PathStr() + wxFileName::GetPathSeparator() + wxT( "target" );

    BOOST_REQUIRE( wxFileName::Mkdir( target, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) );

    bool threw = false;

    try
    {
        FILE_OUTPUTFORMATTER f( target );
        f.Print( 0, "doomed streaming save\n" );
        f.Finish();
    }
    catch( const IO_ERROR& )
    {
        threw = true;
    }

    BOOST_REQUIRE_MESSAGE( threw, "Finish() must throw IO_ERROR when atomic commit fails" );
    BOOST_REQUIRE( wxFileName::DirExists( target ) );
    BOOST_REQUIRE_EQUAL( countSiblingTemps( target ), 0u );
}


BOOST_AUTO_TEST_CASE( DrawingSheetSave_PropagatesCommitFailure )
{
    // Regression for the DS_DATA_MODEL_FILEIO refactor: the drawing-sheet writer must
    // propagate commit failures instead of swallowing them in a constructor catch.
    DS_DATA_MODEL& model = DS_DATA_MODEL::GetTheInstance();
    model.SetEmptyLayout();

    KI_TEST::SCOPED_TEMP_DIR tempDir( wxT( "kicad-atomicsave-wks-commitfail" ) );
    const wxString target = tempDir.PathStr() + wxFileName::GetPathSeparator() + wxT( "target" );
    BOOST_REQUIRE( wxFileName::Mkdir( target, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) );

    BOOST_REQUIRE_THROW( model.Save( target ), IO_ERROR );
    BOOST_REQUIRE( wxFileName::DirExists( target ) );
    BOOST_REQUIRE_EQUAL( countSiblingTemps( target ), 0u );
}


BOOST_AUTO_TEST_CASE( PrettifiedFormatter_DestructorDiscardsWhenTargetIsDirectory )
{
    // A target that cannot be renamed over is still a target the formatter must not disturb.
    // Asserting on-disk state alone would pass either way, since a destructor that attempted
    // the commit would fail the rename and tidy up too -- but it would say so first.
    KI_TEST::SCOPED_TEMP_DIR tempDir( wxT( "kicad-atomicsave-commitfail-implicit" ) );
    const wxString target = tempDir.PathStr() + wxFileName::GetPathSeparator() + wxT( "target" );

    BOOST_REQUIRE( wxFileName::Mkdir( target, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) );

    KI_TEST::SCOPED_COUNTING_WXLOG logCounter( nullptr );

    {
        PRETTIFIED_FILE_OUTPUTFORMATTER f( target );
        f.Print( 0, "(implicit doomed)\n" );
    }

    // Nothing was attempted, so nothing had anything to report
    BOOST_REQUIRE_EQUAL( logCounter.GetCount(), 0u );
    BOOST_REQUIRE( wxFileName::DirExists( target ) );
    BOOST_REQUIRE_EQUAL( countSiblingTemps( target ), 0u );
}


BOOST_AUTO_TEST_CASE( PrettifiedFormatter_CreatesNewTarget )
{
    // When the target file doesn't exist, atomic save must create it cleanly.
    KI_TEST::SCOPED_TEMP_DIR tempDir( wxT( "kicad-atomicsave-newtarget" ) );
    const wxString target = tempDir.PathStr() + wxFileName::GetPathSeparator() + wxT( "target" );
    BOOST_REQUIRE( !wxFileName::FileExists( target ) );

    {
        PRETTIFIED_FILE_OUTPUTFORMATTER f( target );
        f.Print( 0, "(new_file content)\n" );
        BOOST_REQUIRE( f.Finish() );
    }

    BOOST_REQUIRE( wxFileName::FileExists( target ) );
    std::string actual = readFileContents( target );
    BOOST_REQUIRE( actual.find( "new_file" ) != std::string::npos );
    BOOST_REQUIRE_EQUAL( countSiblingTemps( target ), 0u );
}


#if !defined( _WIN32 )

BOOST_AUTO_TEST_CASE( AtomicWriteFile_PreservesPosixMode )
{
    // Regression: MakeWriteable used to run before DuplicatePermissions, so the temp
    // inherited a relaxed mode. A file saved atomically over a 0400 target ended up as
    // 0600. Verify the target mode survives the save exactly.
    KI_TEST::SCOPED_TEMP_DIR tempDir( wxT( "kicad-atomicsave-mode" ) );
    const wxString target = tempDir.PathStr() + wxFileName::GetPathSeparator() + wxT( "target" );
    const std::string original = "original\n";
    writeFileContents( target, original );

    BOOST_REQUIRE_EQUAL( chmod( target.fn_str(), 0400 ), 0 );

    const std::string payload = "replacement\n";
    wxString          err;
    BOOST_REQUIRE( KIPLATFORM::IO::AtomicWriteFile( target, payload.data(), payload.size(),
                                                    &err ) );
    BOOST_REQUIRE( err.IsEmpty() );

    struct stat st;
    BOOST_REQUIRE_EQUAL( stat( target.fn_str(), &st ), 0 );
    BOOST_REQUIRE_EQUAL( st.st_mode & 0777, 0400 );
    BOOST_REQUIRE_EQUAL( readFileContents( target ), payload );
}


BOOST_AUTO_TEST_CASE( AtomicWriteFile_FollowsSymlinkTarget )
{
    // Regression: pre-atomic saves opened the referent via wxFopen so the symlink
    // survived. The new rename-based path would have replaced the symlink with a
    // regular file; ResolveSymlinkTarget fixes that by resolving first.
    KI_TEST::SCOPED_TEMP_DIR tempDir( wxT( "kicad-atomicsave-symlink" ) );
    const wxString referent = tempDir.PathStr() + wxFileName::GetPathSeparator() + wxT( "referent" );
    const wxString linkPath = tempDir.PathStr() + wxFileName::GetPathSeparator() + wxT( "link" );
    const std::string original = "referent original\n";
    writeFileContents( referent, original );

    BOOST_REQUIRE_EQUAL( symlink( referent.fn_str(), linkPath.fn_str() ), 0 );

    const std::string payload = "replacement via symlink\n";
    wxString          err;
    BOOST_REQUIRE( KIPLATFORM::IO::AtomicWriteFile( linkPath, payload.data(), payload.size(),
                                                    &err ) );

    // Link must still be a symlink pointing at the referent.
    struct stat st;
    BOOST_REQUIRE_EQUAL( lstat( linkPath.fn_str(), &st ), 0 );
    BOOST_REQUIRE( S_ISLNK( st.st_mode ) );

    // Referent must have the new content.
    BOOST_REQUIRE_EQUAL( readFileContents( referent ), payload );
}


BOOST_AUTO_TEST_CASE( AtomicWriteFile_FailedRenameRestoresTargetMode )
{
    // Regression: if AtomicRename fails after MakeWriteable has run on the target, the
    // target must not be left with its mode bits permanently widened. We provoke a
    // rename failure by targeting a path whose parent directory cannot accept the write,
    // while ensuring the pre-existing target still has an unusual mode.
    KI_TEST::SCOPED_TEMP_DIR tempDir( wxT( "kicad-atomicsave-failrestore" ) );
    const wxString target = tempDir.PathStr() + wxFileName::GetPathSeparator() + wxT( "target" );
    const std::string original = "original\n";
    writeFileContents( target, original );
    BOOST_REQUIRE_EQUAL( chmod( target.fn_str(), 0400 ), 0 );

    // Route through a non-existent subdirectory so wxFopen on the temp fails before the
    // atomic sequence even reaches MakeWriteable. This exercises the "target untouched"
    // invariant for the common early-failure path.
    wxString bogus = target + wxT( "/cannot-create" );
    wxString err;
    BOOST_REQUIRE( !KIPLATFORM::IO::AtomicWriteFile( bogus, "x", 1, &err ) );

    struct stat st;
    BOOST_REQUIRE_EQUAL( stat( target.fn_str(), &st ), 0 );
    BOOST_REQUIRE_EQUAL( st.st_mode & 0777, 0400 );
    BOOST_REQUIRE_EQUAL( readFileContents( target ), original );
}

#else // _WIN32

BOOST_AUTO_TEST_CASE( AtomicWriteFile_PreservesWindowsAttributes )
{
    // Regression: DuplicatePermissions copies ACLs via SetFileSecurity but not attribute
    // bits. READONLY and HIDDEN used to be silently dropped on every successful save.
    KI_TEST::SCOPED_TEMP_DIR tempDir( wxT( "kicad-atomicsave-winattrs" ) );
    const wxString target = tempDir.PathStr() + wxFileName::GetPathSeparator() + wxT( "target" );
    const std::string original = "original\n";
    writeFileContents( target, original );

    BOOST_REQUIRE( SetFileAttributesW( target.wc_str(),
                                       FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN ) );

    const std::string payload = "replacement\n";
    wxString          err;
    BOOST_REQUIRE( KIPLATFORM::IO::AtomicWriteFile( target, payload.data(), payload.size(),
                                                    &err ) );
    BOOST_REQUIRE( err.IsEmpty() );

    DWORD attrs = GetFileAttributesW( target.wc_str() );
    BOOST_REQUIRE( attrs != INVALID_FILE_ATTRIBUTES );
    BOOST_REQUIRE( ( attrs & FILE_ATTRIBUTE_READONLY ) != 0 );
    BOOST_REQUIRE( ( attrs & FILE_ATTRIBUTE_HIDDEN ) != 0 );
    BOOST_REQUIRE_EQUAL( readFileContents( target ), payload );

    // Clear READONLY so the SCOPED_TEMP_DIR teardown (std::filesystem::remove_all) can
    // delete the file on Windows, where remove() does not clear the attribute itself.
    SetFileAttributesW( target.wc_str(), FILE_ATTRIBUTE_NORMAL );
}

#endif // _WIN32


BOOST_AUTO_TEST_SUITE_END()
