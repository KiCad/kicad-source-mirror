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

/*
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/24061 (and its duplicate
 * https://gitlab.com/kicad/code/kicad/-/issues/23704).
 *
 * The 3D/STEP export runs kicad-cli as a child process that reads the board from disk.  A board
 * with unsaved edits (the user deletes panel sections without saving) therefore exported its stale
 * on-disk contents instead of what the user sees.
 *
 * DIALOG_EXPORT_STEP::StageBoardForExport bridges the gap by serializing the live board to a
 * temporary file when it has unsaved modifications.  This test deletes footprints in memory, runs
 * that helper, and verifies the staged file reflects the deletions while the original on-disk file
 * is left untouched.
 */

#include <filesystem>
#include <fstream>
#include <vector>

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <boost/test/unit_test.hpp>

#include <pcbnew_utils/board_file_utils.h>
#include <dialogs/dialog_export_step.h>
#include <board.h>
#include <footprint.h>

#include <wx/filefn.h>
#include <wx/filename.h>
#include <wx/string.h>


namespace
{
/// A uniquely-named temporary directory removed on destruction, so concurrent runs and early
/// test failures cannot collide or leak.
class SCOPED_TEMP_DIR
{
public:
    SCOPED_TEMP_DIR( const wxString& aPrefix )
    {
        wxString reserved = wxFileName::CreateTempFileName( aPrefix );
        BOOST_REQUIRE( !reserved.IsEmpty() );
        BOOST_REQUIRE( wxRemoveFile( reserved ) );

        m_path = std::filesystem::path( std::string( reserved.utf8_str() ) );
        std::filesystem::create_directory( m_path );
    }

    ~SCOPED_TEMP_DIR()
    {
        std::error_code ec;
        std::filesystem::remove_all( m_path, ec );
    }

    const std::filesystem::path& Path() const { return m_path; }

private:
    std::filesystem::path m_path;
};
} // namespace


BOOST_AUTO_TEST_SUITE( StepExportModifiedBoard )


BOOST_AUTO_TEST_CASE( ModifiedBoardStagesCurrentState )
{
    const std::string sourceBoard = KI_TEST::GetPcbnewTestDataDir() + "issue23704/issue23704.kicad_pcb";

    BOOST_REQUIRE_MESSAGE( std::filesystem::exists( sourceBoard ), "Missing test board " << sourceBoard );

    std::unique_ptr<BOARD> board = KI_TEST::ReadBoardFromFileOrStream( sourceBoard );
    BOOST_REQUIRE( board );

    const size_t originalCount = board->Footprints().size();
    BOOST_REQUIRE_MESSAGE( originalCount > 1, "Panel board needs multiple footprints to delete" );

    // Stage the board's "saved" copy in an isolated temp directory so the helper writes its
    // temporary export file alongside it rather than into the shared test data tree.
    const SCOPED_TEMP_DIR       workDir( wxS( "kicad_step_export_23704_" ) );
    const std::filesystem::path onDisk = workDir.Path() / "panel.kicad_pcb";
    KI_TEST::DumpBoardToFile( *board, onDisk.string() );
    const wxString onDiskPath = wxString::FromUTF8( onDisk.string() );

    // A sibling project file must be staged too, so project-relative model and library paths
    // resolve identically during export.
    std::filesystem::path projectFile = onDisk;
    projectFile.replace_extension( ".kicad_pro" );

    {
        std::ofstream project( projectFile );
        BOOST_REQUIRE( project );
        project << "{ \"meta\": { \"version\": 1 } }\n";
    }

    // The user deletes every panel section but one, never saving the file.
    while( board->Footprints().size() > 1 )
        board->Remove( board->Footprints().back() );

    const size_t remainingCount = board->Footprints().size();
    BOOST_REQUIRE_EQUAL( remainingCount, 1u );

    // Drive the production helper exactly as the export dialog does for a modified board.
    wxString              inputPath;
    std::vector<wxString> tempFiles;
    wxString              errorDetail;
    const wxString        error =
            DIALOG_EXPORT_STEP::StageBoardForExport( onDiskPath, true, board.get(), inputPath, tempFiles, errorDetail );

    BOOST_REQUIRE_MESSAGE( error.IsEmpty(), "StageBoardForExport failed: " << error.ToStdString() );
    BOOST_REQUIRE( wxFileExists( inputPath ) );
    BOOST_CHECK_MESSAGE( inputPath != onDiskPath, "Modified board must stage a temporary copy, not the on-disk file" );

    std::unique_ptr<BOARD> staged = KI_TEST::ReadBoardFromFileOrStream( std::string( inputPath.utf8_str() ) );
    BOOST_REQUIRE( staged );

    // The staged file reflects the in-memory deletions, not the full on-disk panel.
    BOOST_CHECK_EQUAL( staged->Footprints().size(), remainingCount );

    // The sibling project file is staged next to the temporary board.
    std::filesystem::path stagedProject = std::filesystem::path( std::string( inputPath.utf8_str() ) );
    stagedProject.replace_extension( ".kicad_pro" );
    BOOST_CHECK( std::filesystem::exists( stagedProject ) );
    BOOST_CHECK_EQUAL( std::filesystem::file_size( stagedProject ), std::filesystem::file_size( projectFile ) );

    // The original on-disk file is never mutated by staging.
    std::unique_ptr<BOARD> onDiskReloaded = KI_TEST::ReadBoardFromFileOrStream( onDisk.string() );
    BOOST_REQUIRE( onDiskReloaded );
    BOOST_CHECK_EQUAL( onDiskReloaded->Footprints().size(), originalCount );

    for( const wxString& f : tempFiles )
    {
        if( wxFileExists( f ) )
            wxRemoveFile( f );
    }
}


/**
 * An unmodified, saved board stages its existing on-disk file directly with no temporary copy.
 */
BOOST_AUTO_TEST_CASE( UnmodifiedBoardStagesOnDiskFile )
{
    const std::string sourceBoard = KI_TEST::GetPcbnewTestDataDir() + "issue23704/issue23704.kicad_pcb";

    std::unique_ptr<BOARD> board = KI_TEST::ReadBoardFromFileOrStream( sourceBoard );
    BOOST_REQUIRE( board );

    const wxString onDiskPath = wxString::FromUTF8( sourceBoard );

    wxString              inputPath;
    std::vector<wxString> tempFiles;
    wxString              errorDetail;
    const wxString        error = DIALOG_EXPORT_STEP::StageBoardForExport( onDiskPath, false, board.get(), inputPath,
                                                                           tempFiles, errorDetail );

    BOOST_CHECK( error.IsEmpty() );
    BOOST_CHECK_EQUAL( inputPath, onDiskPath );
    BOOST_CHECK( tempFiles.empty() );
}


/**
 * An empty board path is rejected (the board must be saved at least once before STEP export).
 */
BOOST_AUTO_TEST_CASE( UnsavedBoardIsRejected )
{
    const std::string sourceBoard = KI_TEST::GetPcbnewTestDataDir() + "issue23704/issue23704.kicad_pcb";

    std::unique_ptr<BOARD> board = KI_TEST::ReadBoardFromFileOrStream( sourceBoard );
    BOOST_REQUIRE( board );

    wxString              inputPath;
    std::vector<wxString> tempFiles;
    wxString              errorDetail;
    const wxString        error = DIALOG_EXPORT_STEP::StageBoardForExport( wxEmptyString, true, board.get(), inputPath,
                                                                           tempFiles, errorDetail );

    BOOST_CHECK( !error.IsEmpty() );
    BOOST_CHECK( inputPath.IsEmpty() );
    BOOST_CHECK( tempFiles.empty() );
}


BOOST_AUTO_TEST_SUITE_END()
