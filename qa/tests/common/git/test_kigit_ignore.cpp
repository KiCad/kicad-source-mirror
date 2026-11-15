/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 */

/**
 * @file
 * Tests for the KIGIT ignore helpers, which write the project's .gitignore and report what
 * git would do with a rule for a path.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <git/git_status_handler.h>
#include <git/kicad_git_common.h>
#include <git/kicad_git_memory.h>
#include <git/kigit_ignore.h>

#include <git2.h>

#include <map>

#include <qa_utils/git_test_fixture.h>

#include <wx/ffile.h>
#include <wx/filename.h>
#include <wx/textfile.h>


static const char* TEST_AUTHOR_NAME = "Test Author";
static const char* TEST_AUTHOR_EMAIL = "test@kicad.example";


/// A repository holding one committed file, with the project at its root.
struct GIT_IGNORE_FIXTURE : KI_TEST::GIT_TEST_FIXTURE
{
    GIT_IGNORE_FIXTURE() :
            m_repoPath( TempPath() )
    {
        m_repo = KI_TEST::InitGitRepository( m_repoPath, "master", TEST_AUTHOR_NAME, TEST_AUTHOR_EMAIL );

        writeFile( wxT( "board.kicad_pcb" ), wxT( "(kicad_pcb)\n" ) );
        KI_TEST::CommitAllGitFiles( m_repo.get(), wxT( "First commit" ) );
    }


    void writeFile( const wxString& aFile, const wxString& aContent )
    {
        KI_TEST::WriteGitFile( m_repoPath, aFile, aContent.utf8_string() );
    }


    /// What the status reports, keyed by repository relative path.
    std::map<wxString, FileStatus> reported( KIGIT_COMMON& aCommon, bool aChangedOnly )
    {
        GIT_STATUS_HANDLER             handler( &aCommon );
        const wxString                 workDir = handler.GetWorkingDirectory();
        std::map<wxString, FileStatus> status = handler.GetFileStatus( wxEmptyString, aChangedOnly );
        std::map<wxString, FileStatus> paths;

        for( const auto& [path, fileStatus] : status )
            paths.emplace( path.StartsWith( workDir ) ? path.Mid( workDir.length() ) : path, fileStatus );

        return paths;
    }


    /// What the commit list offers, which is changes only.
    std::map<wxString, FileStatus> changes( KIGIT_COMMON& aCommon ) { return reported( aCommon, true ); }


    /// What the project tree is built from, ignored paths included.
    std::map<wxString, FileStatus> treeStatus( KIGIT_COMMON& aCommon ) { return reported( aCommon, false ); }


    wxFileName ignoreFile() const { return KIGIT::ProjectIgnoreFile( projectPath() ); }

    /// The project directory, which is where its ignore file belongs.
    virtual wxString projectPath() const { return m_repoPath + wxFileName::GetPathSeparator(); }

    git_repository* repo() const { return m_repo.get(); }

    virtual ~GIT_IGNORE_FIXTURE() = default;

    wxString rules() const
    {
        wxTextFile file( ignoreFile().GetFullPath() );
        wxString   contents;

        BOOST_REQUIRE( file.Open() );

        for( size_t ii = 0; ii < file.GetLineCount(); ++ii )
            contents += file[ii] + wxT( "\n" );

        return contents;
    }

protected:
    KIGIT::GitRepositoryPtr m_repo;
    wxString                m_repoPath;
};


BOOST_FIXTURE_TEST_SUITE( KiGitIgnore, GIT_IGNORE_FIXTURE )


BOOST_AUTO_TEST_CASE( RuleNamesOnlyThePathPicked )
{
    BOOST_CHECK_EQUAL( KIGIT::PathIgnoreRule( wxT( "gerbers" ), true ), wxT( "/gerbers/" ) );
    BOOST_CHECK_EQUAL( KIGIT::PathIgnoreRule( wxT( "out/top.gbr" ), false ), wxT( "/out/top.gbr" ) );
    BOOST_CHECK_EQUAL( KIGIT::PathIgnoreRule( wxT( "board[1].gbr" ), false ), wxT( "/board\\[1].gbr" ) );
    BOOST_CHECK_EQUAL( KIGIT::PathIgnoreRule( wxEmptyString, false ), wxEmptyString );

    BOOST_CHECK_EQUAL( KIGIT::ExtensionIgnoreRule( wxT( "out/top.gbr" ) ), wxT( "*.gbr" ) );
    BOOST_CHECK_EQUAL( KIGIT::ExtensionIgnoreRule( wxT( "fp-info-cache" ) ), wxEmptyString );
}


// The tree still hears about an ignored file, so it can dim the name instead of dropping it.
BOOST_AUTO_TEST_CASE( IgnoredFileLeavesTheCommitList )
{
    writeFile( wxT( "out/top.gbr" ), wxT( "gerber\n" ) );

    KIGIT_COMMON common( repo(), projectPath() );

    BOOST_REQUIRE( changes( common ).count( wxT( "out/top.gbr" ) ) == 1 );
    BOOST_CHECK( !KIGIT::IgnoreState( common, wxT( "out/top.gbr" ), false ).ignored );

    wxString error;
    BOOST_REQUIRE_MESSAGE( KIGIT::AddIgnoreRules( ignoreFile(), { wxT( "/out/top.gbr" ) }, wxEmptyString, error ),
                           error );

    BOOST_CHECK( changes( common ).count( wxT( "out/top.gbr" ) ) == 0 );
    BOOST_CHECK( KIGIT::IgnoreState( common, wxT( "out/top.gbr" ), false ).ignored );

    const std::map<wxString, FileStatus> tree = treeStatus( common );
    const auto                           entry = tree.find( wxT( "out/top.gbr" ) );

    BOOST_REQUIRE( entry != tree.end() );
    BOOST_CHECK( entry->second.status == KIGIT_COMMON::GIT_STATUS::GIT_STATUS_IGNORED );
    BOOST_CHECK( !entry->second.isDirectory );
}


// Git names an ignored folder alone, which is why the tree dims its children itself.
BOOST_AUTO_TEST_CASE( IgnoredFolderLeavesTheCommitList )
{
    writeFile( wxT( "gerbers/top.gbr" ), wxT( "gerber\n" ) );
    writeFile( wxT( "gerbers/bottom.gbr" ), wxT( "gerber\n" ) );

    KIGIT_COMMON common( repo(), projectPath() );

    BOOST_REQUIRE( changes( common ).count( wxT( "gerbers/top.gbr" ) ) == 1 );

    wxString error;
    BOOST_REQUIRE_MESSAGE( KIGIT::AddIgnoreRules( ignoreFile(), { wxT( "/gerbers/" ) }, wxEmptyString, error ), error );

    const std::map<wxString, FileStatus> changed = changes( common );

    BOOST_CHECK( changed.count( wxT( "gerbers/top.gbr" ) ) == 0 );
    BOOST_CHECK( changed.count( wxT( "gerbers/bottom.gbr" ) ) == 0 );
    BOOST_CHECK( KIGIT::IgnoreState( common, wxT( "gerbers" ), true ).ignored );

    const std::map<wxString, FileStatus> tree = treeStatus( common );
    const auto                           folder = tree.find( wxT( "gerbers" ) );

    BOOST_REQUIRE_MESSAGE( folder != tree.end(), "the tree needs the folder itself to dim it" );
    BOOST_CHECK( folder->second.status == KIGIT_COMMON::GIT_STATUS::GIT_STATUS_IGNORED );
    BOOST_CHECK( folder->second.isDirectory );
    BOOST_CHECK( tree.count( wxT( "gerbers/top.gbr" ) ) == 0 );
}


BOOST_AUTO_TEST_CASE( TrackedPathReportsThatARuleWouldNotApply )
{
    writeFile( wxT( "docs/notes.txt" ), wxT( "notes\n" ) );
    KI_TEST::CommitAllGitFiles( repo(), wxT( "Add notes" ) );

    KIGIT_COMMON common( repo(), projectPath() );

    BOOST_CHECK( KIGIT::IgnoreState( common, wxT( "board.kicad_pcb" ), false ).tracked );
    BOOST_CHECK( KIGIT::IgnoreState( common, wxT( "docs/notes.txt" ), false ).tracked );

    // A folder holding a tracked file cannot be ignored into untracking it either.
    BOOST_CHECK( KIGIT::IgnoreState( common, wxT( "docs" ), true ).tracked );

    writeFile( wxT( "out/top.gbr" ), wxT( "gerber\n" ) );
    BOOST_CHECK( !KIGIT::IgnoreState( common, wxT( "out/top.gbr" ), false ).tracked );
    BOOST_CHECK( !KIGIT::IgnoreState( common, wxT( "out" ), true ).tracked );
}


BOOST_AUTO_TEST_CASE( AddingKeepsTheFileTheUserHad )
{
    writeFile( wxT( ".gitignore" ), wxT( "# my own rules\n*.bak\n" ) );

    wxString error;
    BOOST_REQUIRE_MESSAGE( KIGIT::AddIgnoreRules( ignoreFile(), { wxT( "/out/top.gbr" ) }, wxEmptyString, error ),
                           error );
    BOOST_REQUIRE_MESSAGE(
            KIGIT::AddIgnoreRules( ignoreFile(), { wxT( "/out/top.gbr" ), wxT( "*.bak" ) }, wxEmptyString, error ),
            error );

    BOOST_CHECK_EQUAL( rules(), wxT( "# my own rules\n*.bak\n/out/top.gbr\n" ) );
}


// Without this the first new rule would land on the end of the user's last one.
BOOST_AUTO_TEST_CASE( AddingAfterAnUnterminatedLastLine )
{
    wxFFile out( ignoreFile().GetFullPath(), wxT( "w" ) );
    BOOST_REQUIRE( out.IsOpened() );
    BOOST_REQUIRE( out.Write( wxT( "*.bak" ) ) );
    BOOST_REQUIRE( out.Close() );

    wxString error;
    BOOST_REQUIRE_MESSAGE( KIGIT::AddIgnoreRules( ignoreFile(), { wxT( "/out/top.gbr" ) }, wxEmptyString, error ),
                           error );

    BOOST_CHECK_EQUAL( rules(), wxT( "*.bak\n/out/top.gbr\n" ) );
}


BOOST_AUTO_TEST_CASE( RemovingTakesOnlyTheRuleNamed )
{
    writeFile( wxT( ".gitignore" ), wxT( "# my own rules\n*.gbr\n\n/out/top.gbr\n/gerbers/\n" ) );

    wxString error;
    BOOST_REQUIRE_MESSAGE( KIGIT::RemoveIgnoreRules( ignoreFile(), { wxT( "/out/top.gbr" ) }, error ), error );

    BOOST_CHECK_EQUAL( rules(), wxT( "# my own rules\n*.gbr\n\n/gerbers/\n" ) );

    // A rule the file does not hold is not an error, and changes nothing.
    BOOST_REQUIRE_MESSAGE( KIGIT::RemoveIgnoreRules( ignoreFile(), { wxT( "/nowhere.gbr" ) }, error ), error );
    BOOST_CHECK_EQUAL( rules(), wxT( "# my own rules\n*.gbr\n\n/gerbers/\n" ) );

    // The broader rule is still there, so the file it covers is still ignored.
    KIGIT_COMMON common( repo(), projectPath() );
    BOOST_CHECK( KIGIT::IgnoreState( common, wxT( "out/top.gbr" ), false ).ignored );
}


BOOST_AUTO_TEST_SUITE_END()


/// The project is a folder inside a larger repository, where rules have to be anchored to the
/// project rather than the repository root.
struct GIT_IGNORE_SUBDIR_FIXTURE : GIT_IGNORE_FIXTURE
{
    wxString projectPath() const override
    {
        return m_repoPath + wxFileName::GetPathSeparator() + wxT( "proj" ) + wxFileName::GetPathSeparator();
    }
};


BOOST_FIXTURE_TEST_SUITE( KiGitIgnoreSubdirectory, GIT_IGNORE_SUBDIR_FIXTURE )


BOOST_AUTO_TEST_CASE( RulesApplyToTheProjectOnly )
{
    writeFile( wxT( "proj/out/top.gbr" ), wxT( "gerber\n" ) );
    writeFile( wxT( "other/out/top.gbr" ), wxT( "gerber\n" ) );

    KIGIT_COMMON common( repo(), projectPath() );

    wxString error;
    BOOST_REQUIRE_MESSAGE( KIGIT::AddIgnoreRules( ignoreFile(), { wxT( "/out/" ) }, wxEmptyString, error ), error );

    BOOST_CHECK_EQUAL( rules(), wxT( "/out/\n" ) );
    BOOST_CHECK( KIGIT::IgnoreState( common, wxT( "out" ), true ).ignored );
    BOOST_CHECK( KIGIT::IgnoreState( common, wxT( "out/top.gbr" ), false ).ignored );

    const std::map<wxString, FileStatus> changed = changes( common );

    BOOST_CHECK( changed.count( wxT( "proj/out/top.gbr" ) ) == 0 );
    BOOST_CHECK( changed.count( wxT( "other/out/top.gbr" ) ) == 1 );
}


BOOST_AUTO_TEST_SUITE_END()
