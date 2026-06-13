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

#include <fstream>
#include <map>

#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/utils.h>


static const char* TEST_AUTHOR_NAME  = "Test Author";
static const char* TEST_AUTHOR_EMAIL = "test@kicad.example";


/**
 * Build a temp working repo with two commits.  The first commit adds a.txt and
 * b.txt; the second modifies a.txt, deletes b.txt and adds c.txt.  This lets a
 * compare of HEAD~1..HEAD exercise modified / removed / added deltas.
 */
struct GIT_COMPARE_FIXTURE
{
    GIT_COMPARE_FIXTURE() :
            m_repo( nullptr ),
            m_ready( false )
    {
        git_libgit2_init();

        wxString tmp = wxStandardPaths::Get().GetTempDir() + wxFileName::GetPathSeparator()
                       + wxString::Format( wxS( "kicad_qa_cmp_%lu_%ld" ),
                                           static_cast<unsigned long>( wxGetProcessId() ),
                                           s_counter++ );

        wxFileName::Mkdir( tmp, 0700, wxPATH_MKDIR_FULL );
        m_repoPath = tmp;

        m_ready = initRepo();
    }


    ~GIT_COMPARE_FIXTURE()
    {
        if( m_repo )
            git_repository_free( m_repo );

        if( !m_repoPath.IsEmpty() )
            wxFileName::Rmdir( m_repoPath, wxPATH_RMDIR_RECURSIVE );

        git_libgit2_shutdown();
    }


    /// Write a file in the working tree (or remove it if aRemove is set).
    void writeFile( const wxString& aFile, const wxString& aContent )
    {
        wxString filePath = m_repoPath + wxFileName::GetPathSeparator() + aFile;
        std::ofstream f( filePath.ToStdString(), std::ios::trunc );
        f << aContent.ToStdString();
    }


    void removeFile( const wxString& aFile )
    {
        wxRemoveFile( m_repoPath + wxFileName::GetPathSeparator() + aFile );
    }


    /// Stage every change (including deletions) and create a commit on HEAD.
    bool commitAll( const wxString& aMessage )
    {
        git_index* index = nullptr;

        if( git_repository_index( &index, m_repo ) != 0 )
            return false;

        KIGIT::GitIndexPtr indexPtr( index );

        char* paths[] = { const_cast<char*>( "*" ) };
        git_strarray pathspec = { paths, 1 };

        if( git_index_add_all( index, &pathspec, GIT_INDEX_ADD_DEFAULT, nullptr, nullptr ) != 0 )
            return false;

        git_index_write( index );

        git_oid treeOid;

        if( git_index_write_tree( &treeOid, index ) != 0 )
            return false;

        git_tree* tree = nullptr;

        if( git_tree_lookup( &tree, m_repo, &treeOid ) != 0 )
            return false;

        KIGIT::GitTreePtr treePtr( tree );

        git_signature* sig = nullptr;
        git_signature_now( &sig, TEST_AUTHOR_NAME, TEST_AUTHOR_EMAIL );
        KIGIT::GitSignaturePtr sigPtr( sig );

        git_commit*    parent  = nullptr;
        git_reference* headRef = nullptr;

        if( git_repository_head( &headRef, m_repo ) == 0 )
        {
            git_reference_peel( reinterpret_cast<git_object**>( &parent ), headRef,
                                GIT_OBJECT_COMMIT );
            git_reference_free( headRef );
        }

        KIGIT::GitCommitPtr     parentPtr( parent );
        const git_commit*       parents[1]   = { parent };
        const git_commit**      parentsPtr   = parent ? parents : nullptr;
        size_t                  parentsCount = parent ? 1 : 0;

        git_oid commitOid;

        return git_commit_create( &commitOid, m_repo, "HEAD", sig, sig, nullptr,
                                  aMessage.ToUTF8().data(), tree, parentsCount, parentsPtr )
               == 0;
    }


    git_repository* repo() const { return m_repo; }
    bool            ready() const { return m_ready; }


private:
    bool initRepo()
    {
        // Pin the initial branch to "master" so assertions are independent of the
        // user's init.defaultBranch (often "main") in their gitconfig.
        git_repository_init_options initOpts;
        git_repository_init_options_init( &initOpts, GIT_REPOSITORY_INIT_OPTIONS_VERSION );
        initOpts.initial_head = "master";

        if( git_repository_init_ext( &m_repo, m_repoPath.ToUTF8().data(), &initOpts ) != 0 )
            return false;

        git_config* cfg = nullptr;

        if( git_repository_config( &cfg, m_repo ) == 0 )
        {
            git_config_set_string( cfg, "user.name", TEST_AUTHOR_NAME );
            git_config_set_string( cfg, "user.email", TEST_AUTHOR_EMAIL );
            git_config_free( cfg );
        }

        writeFile( wxT( "a.txt" ), wxT( "alpha\n" ) );
        writeFile( wxT( "b.txt" ), wxT( "bravo\n" ) );

        if( !commitAll( wxT( "First commit" ) ) )
            return false;

        writeFile( wxT( "a.txt" ), wxT( "alpha modified\n" ) );
        removeFile( wxT( "b.txt" ) );
        writeFile( wxT( "c.txt" ), wxT( "charlie\n" ) );

        return commitAll( wxT( "Second commit" ) );
    }


    git_repository* m_repo;
    wxString        m_repoPath;
    bool            m_ready;

    static long s_counter;
};

long GIT_COMPARE_FIXTURE::s_counter = 0;


BOOST_FIXTURE_TEST_SUITE( GitCompareHandler, GIT_COMPARE_FIXTURE )


BOOST_AUTO_TEST_CASE( ResolveRefToTree_Valid )
{
    BOOST_REQUIRE( ready() );

    KIGIT::GitTreePtr head( KIGIT::ResolveRefToTree( repo(), wxT( "HEAD" ) ) );
    BOOST_CHECK( head.get() != nullptr );

    KIGIT::GitTreePtr prev( KIGIT::ResolveRefToTree( repo(), wxT( "HEAD~1" ) ) );
    BOOST_CHECK( prev.get() != nullptr );

    // HEAD and its parent point at different trees.
    BOOST_CHECK( head.get() != prev.get() );
}


BOOST_AUTO_TEST_CASE( ResolveRefToTree_Invalid )
{
    BOOST_REQUIRE( ready() );

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
    BOOST_REQUIRE( ready() );

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
    BOOST_REQUIRE( ready() );

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
    BOOST_REQUIRE( ready() );

    BOOST_CHECK( KIGIT::CompareRefs( repo(), wxT( "HEAD" ), wxT( "nope" ) ).empty() );
    BOOST_CHECK( KIGIT::CompareRefs( nullptr, wxT( "HEAD~1" ), wxT( "HEAD" ) ).empty() );
}


BOOST_AUTO_TEST_SUITE_END()
