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
 * Regression tests for the shared KIGIT git helpers ResolveRefToTree /
 * CollectDiffDeltas and the CompareRefs façade built on top of them.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <git/git_compare_handler.h>
#include <git/kicad_git_common.h>
#include <git/kicad_git_memory.h>

#include <git2.h>

#include <map>

#include <qa_utils/git_test_fixture.h>

#include <wx/filename.h>


static const char* TEST_AUTHOR_NAME  = "Test Author";
static const char* TEST_AUTHOR_EMAIL = "test@kicad.example";


/**
 * Build a temp working repo with two commits.  The first commit adds a.txt and
 * b.txt. The second modifies a.txt, deletes b.txt and adds c.txt. This lets a
 * compare of HEAD~1..HEAD exercise modified / removed / added deltas.
 */
struct GIT_COMPARE_FIXTURE : KI_TEST::GIT_TEST_FIXTURE
{
    GIT_COMPARE_FIXTURE() :
            m_repoPath( TempPath() )
    {
        initRepo();
    }


    /// Write a file in the working tree.
    void writeFile( const wxString& aFile, const wxString& aContent )
    {
        KI_TEST::WriteGitFile( m_repoPath, aFile, aContent.utf8_string() );
    }


    void removeFile( const wxString& aFile )
    {
        wxRemoveFile( m_repoPath + wxFileName::GetPathSeparator() + aFile );
    }


    /// Stage every change, including deletions, and create a commit on HEAD.
    void commitAll( const wxString& aMessage ) { KI_TEST::CommitAllGitFiles( m_repo.get(), aMessage ); }


    git_repository* repo() const { return m_repo.get(); }


private:
    void initRepo()
    {
        m_repo = KI_TEST::InitGitRepository( m_repoPath, "master", TEST_AUTHOR_NAME, TEST_AUTHOR_EMAIL );

        writeFile( wxT( "a.txt" ), wxT( "alpha\n" ) );
        writeFile( wxT( "b.txt" ), wxT( "bravo\n" ) );
        commitAll( wxT( "First commit" ) );

        writeFile( wxT( "a.txt" ), wxT( "alpha modified\n" ) );
        removeFile( wxT( "b.txt" ) );
        writeFile( wxT( "c.txt" ), wxT( "charlie\n" ) );
        commitAll( wxT( "Second commit" ) );
    }


    KIGIT::GitRepositoryPtr m_repo;
    wxString                m_repoPath;
};


BOOST_FIXTURE_TEST_SUITE( GitCompareHandler, GIT_COMPARE_FIXTURE )


BOOST_AUTO_TEST_CASE( ResolveRefToTree_Valid )
{
    KIGIT::GitTreePtr head( KIGIT::ResolveRefToTree( repo(), wxT( "HEAD" ) ) );
    BOOST_CHECK( head.get() != nullptr );

    KIGIT::GitTreePtr prev( KIGIT::ResolveRefToTree( repo(), wxT( "HEAD~1" ) ) );
    BOOST_CHECK( prev.get() != nullptr );

    // HEAD and its parent point at different trees.
    BOOST_CHECK( head.get() != prev.get() );
}


BOOST_AUTO_TEST_CASE( ResolveRefToTree_Invalid )
{
    BOOST_CHECK( KIGIT::ResolveRefToTree( repo(), wxT( "does-not-exist" ) ) == nullptr );
    BOOST_CHECK( KIGIT::ResolveRefToTree( nullptr, wxT( "HEAD" ) ) == nullptr );
}


BOOST_AUTO_TEST_CASE( CollectDiffDeltas_NullDiffIsNoOp )
{
    int calls = 0;
    KIGIT::CollectDiffDeltas( nullptr, [&calls]( const git_diff_delta& ) { ++calls; } );
    BOOST_CHECK_EQUAL( calls, 0 );
}


BOOST_AUTO_TEST_CASE( CollectDiffDeltas_WalksEveryDelta )
{
    KIGIT::GitTreePtr baseTree( KIGIT::ResolveRefToTree( repo(), wxT( "HEAD~1" ) ) );
    KIGIT::GitTreePtr headTree( KIGIT::ResolveRefToTree( repo(), wxT( "HEAD" ) ) );

    BOOST_REQUIRE( baseTree.get() );
    BOOST_REQUIRE( headTree.get() );

    git_diff*        diff = nullptr;
    git_diff_options opts = GIT_DIFF_OPTIONS_INIT;

    BOOST_REQUIRE_EQUAL(
            git_diff_tree_to_tree( &diff, repo(), baseTree.get(), headTree.get(), &opts ), 0 );

    KIGIT::GitDiffPtr diffPtr( diff );

    std::map<wxString, git_delta_t> seen;
    KIGIT::CollectDiffDeltas( diff,
            [&seen]( const git_diff_delta& aDelta )
            {
                const char* path = aDelta.new_file.path ? aDelta.new_file.path
                                                        : aDelta.old_file.path;
                seen[wxString::FromUTF8( path )] = aDelta.status;
            } );

    BOOST_CHECK_EQUAL( seen.size(), 3u );
    BOOST_CHECK( seen.at( wxT( "a.txt" ) ) == GIT_DELTA_MODIFIED );
    BOOST_CHECK( seen.at( wxT( "b.txt" ) ) == GIT_DELTA_DELETED );
    BOOST_CHECK( seen.at( wxT( "c.txt" ) ) == GIT_DELTA_ADDED );
}


BOOST_AUTO_TEST_CASE( CompareRefs_ReportsExpectedChanges )
{
    std::vector<KIGIT::CHANGED_FILE> changes =
            KIGIT::CompareRefs( repo(), wxT( "HEAD~1" ), wxT( "HEAD" ) );

    std::map<wxString, KIGIT::FILE_CHANGE_STATUS> byPath;

    for( const KIGIT::CHANGED_FILE& f : changes )
        byPath[f.path] = f.status;

    BOOST_CHECK_EQUAL( byPath.size(), 3u );
    BOOST_CHECK( byPath.at( wxT( "a.txt" ) ) == KIGIT::FILE_CHANGE_STATUS::MODIFIED );
    BOOST_CHECK( byPath.at( wxT( "b.txt" ) ) == KIGIT::FILE_CHANGE_STATUS::REMOVED );
    BOOST_CHECK( byPath.at( wxT( "c.txt" ) ) == KIGIT::FILE_CHANGE_STATUS::ADDED );
}


BOOST_AUTO_TEST_CASE( CompareRefs_InvalidRefReturnsEmpty )
{
    BOOST_CHECK( KIGIT::CompareRefs( repo(), wxT( "HEAD" ), wxT( "nope" ) ).empty() );
    BOOST_CHECK( KIGIT::CompareRefs( nullptr, wxT( "HEAD~1" ), wxT( "HEAD" ) ).empty() );
}


// An empty base ref compares against the empty tree, so the initial commit (which has no parent
// to name) reports all of its files as added instead of returning nothing.
BOOST_AUTO_TEST_CASE( CompareRefs_EmptyBaseComparesAgainstEmptyTree )
{
    std::vector<KIGIT::CHANGED_FILE> changes = KIGIT::CompareRefs( repo(), wxEmptyString, wxT( "HEAD~1" ) );

    std::map<wxString, KIGIT::FILE_CHANGE_STATUS> byPath;

    for( const KIGIT::CHANGED_FILE& f : changes )
        byPath[f.path] = f.status;

    BOOST_REQUIRE_EQUAL( changes.size(), 2u );
    BOOST_REQUIRE( byPath.count( wxT( "a.txt" ) ) );
    BOOST_REQUIRE( byPath.count( wxT( "b.txt" ) ) );
    BOOST_CHECK( byPath[wxT( "a.txt" )] == KIGIT::FILE_CHANGE_STATUS::ADDED );
    BOOST_CHECK( byPath[wxT( "b.txt" )] == KIGIT::FILE_CHANGE_STATUS::ADDED );
}


BOOST_AUTO_TEST_SUITE_END()
