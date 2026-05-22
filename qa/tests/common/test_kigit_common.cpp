/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <git/kicad_git_common.h>
#include <git/kicad_git_memory.h>

#include <git2.h>

#include <wx/filename.h>
#include <wx/string.h>
#include <wx/stdpaths.h>

#include <chrono>
#include <fstream>
#include <memory>


namespace
{

struct GitInitGuard
{
    GitInitGuard()  { git_libgit2_init(); }
    ~GitInitGuard() { git_libgit2_shutdown(); }
};


struct ScopedTempDir
{
    wxString path;

    ScopedTempDir()
    {
        auto now = std::chrono::steady_clock::now().time_since_epoch();
        long long ticks = std::chrono::duration_cast<std::chrono::nanoseconds>( now ).count();

        wxString base = wxFileName::GetTempDir();
        wxFileName fn;
        fn.AssignDir( base );
        fn.AppendDir( wxString::Format( "kicad-qa-git-%lld-%p", ticks, this ) );
        wxFileName::Mkdir( fn.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );
        path = fn.GetPath();
    }

    ~ScopedTempDir()
    {
        if( !path.IsEmpty() )
            wxFileName::Rmdir( path, wxPATH_RMDIR_RECURSIVE );
    }
};


// Initialize a git repository at the given path with one initial commit on "main"
git_repository* makeRepoWithCommit( const wxString& aRepoPath, const wxString& aFileName,
                                    const std::string& aFileContents )
{
    git_repository* repo = nullptr;
    git_repository_init_options init_opts = GIT_REPOSITORY_INIT_OPTIONS_INIT;
    init_opts.flags = GIT_REPOSITORY_INIT_MKPATH;
    init_opts.initial_head = "main";

    BOOST_REQUIRE_EQUAL( git_repository_init_ext( &repo, aRepoPath.utf8_string().c_str(),
                                                  &init_opts ),
                         GIT_OK );

    // Write a file
    wxFileName filePath( aRepoPath, aFileName );
    {
        std::ofstream f( filePath.GetFullPath().utf8_string() );
        f << aFileContents;
    }

    // Configure user
    git_config* cfg = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_config( &cfg, repo ), GIT_OK );
    KIGIT::GitConfigPtr cfgPtr( cfg );
    git_config_set_string( cfg, "user.name", "QA Test" );
    git_config_set_string( cfg, "user.email", "qa@example.com" );

    // Stage and commit
    git_index* index = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_index( &index, repo ), GIT_OK );
    KIGIT::GitIndexPtr indexPtr( index );
    BOOST_REQUIRE_EQUAL( git_index_add_bypath( index, aFileName.utf8_string().c_str() ), GIT_OK );
    BOOST_REQUIRE_EQUAL( git_index_write( index ), GIT_OK );

    git_oid tree_oid;
    BOOST_REQUIRE_EQUAL( git_index_write_tree( &tree_oid, index ), GIT_OK );

    git_tree* tree = nullptr;
    BOOST_REQUIRE_EQUAL( git_tree_lookup( &tree, repo, &tree_oid ), GIT_OK );
    KIGIT::GitTreePtr treePtr( tree );

    git_signature* sig = nullptr;
    BOOST_REQUIRE_EQUAL( git_signature_now( &sig, "QA Test", "qa@example.com" ), GIT_OK );
    KIGIT::GitSignaturePtr sigPtr( sig );

    git_oid commit_oid;
    BOOST_REQUIRE_EQUAL( git_commit_create( &commit_oid, repo, "HEAD", sig, sig, nullptr,
                                            "initial", tree, 0, nullptr ),
                         GIT_OK );

    return repo;
}

} // namespace


BOOST_AUTO_TEST_SUITE( KiGitCommon )


// GetGitRootDirectory must return the working directory (project root) and not the
// internal .git folder.  Older code returned git_repository_path() which produced a path
// ending in /.git/, breaking the version-control popup-menu detection logic for whether a
// project lives at the repository root.
BOOST_AUTO_TEST_CASE( GitRootDirectoryReturnsWorkdir )
{
    GitInitGuard libgit;
    ScopedTempDir tmp;

    git_repository* repo = makeRepoWithCommit( tmp.path, "file.txt", "data\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    KIGIT_COMMON common( repo );
    wxString root = common.GetGitRootDirectory();

    BOOST_CHECK( !root.IsEmpty() );
    BOOST_CHECK_MESSAGE( !root.Contains( wxS( "/.git" ) ) && !root.Contains( wxS( "\\.git" ) ),
                         "GetGitRootDirectory should return the working directory, got: "
                                 + root.ToStdString() );

    // Should be the repo path (with trailing separator).
    wxFileName rootFn;
    rootFn.AssignDir( root );
    wxFileName tmpFn;
    tmpFn.AssignDir( tmp.path );
    BOOST_CHECK_EQUAL( rootFn.GetFullPath().ToStdString(), tmpFn.GetFullPath().ToStdString() );
}


// HasPushAndPullRemote must report true for repositories whose remote uses a name other
// than "origin", because libgit2 allows arbitrary remote names and many users rename or
// add additional remotes.
BOOST_AUTO_TEST_CASE( HasPushAndPullRemoteAcceptsNonOriginName )
{
    GitInitGuard libgit;
    ScopedTempDir tmp;

    git_repository* repo = makeRepoWithCommit( tmp.path, "file.txt", "data\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    // No remotes configured.
    KIGIT_COMMON common( repo );
    BOOST_CHECK( !common.HasPushAndPullRemote() );

    // Add a remote with a non-default name.
    git_remote* remote = nullptr;
    BOOST_REQUIRE_EQUAL( git_remote_create( &remote, repo, "github",
                                            "git@github.com:example/repo.git" ),
                         GIT_OK );
    KIGIT::GitRemotePtr remotePtr( remote );

    BOOST_CHECK( common.HasPushAndPullRemote() );
}


BOOST_AUTO_TEST_CASE( HasPushAndPullRemoteFindsOrigin )
{
    GitInitGuard libgit;
    ScopedTempDir tmp;

    git_repository* repo = makeRepoWithCommit( tmp.path, "file.txt", "data\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    git_remote* remote = nullptr;
    BOOST_REQUIRE_EQUAL( git_remote_create( &remote, repo, "origin",
                                            "git@example.com:repo.git" ),
                         GIT_OK );
    KIGIT::GitRemotePtr remotePtr( remote );

    KIGIT_COMMON common( repo );
    BOOST_CHECK( common.HasPushAndPullRemote() );
}


// GetDifferentFiles previously walked unbounded history when no upstream OID was available
// and dumped every file in the root commit's tree into the modified set, falsely flagging
// every file as ahead of the remote.  With no upstream configured, both sets must be empty.
BOOST_AUTO_TEST_CASE( GetDifferentFilesEmptyWithoutUpstream )
{
    GitInitGuard libgit;
    ScopedTempDir tmp;

    git_repository* repo = makeRepoWithCommit( tmp.path, "file.txt", "data\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    KIGIT_COMMON common( repo );
    auto [local, remote] = common.GetDifferentFiles();

    BOOST_CHECK_MESSAGE( local.empty(),
                         "Expected no AHEAD files when no upstream is configured, got "
                                 + std::to_string( local.size() ) );
    BOOST_CHECK_MESSAGE( remote.empty(),
                         "Expected no BEHIND files when no upstream is configured, got "
                                 + std::to_string( remote.size() ) );
}


// Regression test for the root-commit dump that this commit fixes.  Set up an upstream
// tracking ref whose history is disjoint from the local HEAD's history (separate root
// commits, no shared ancestors).  Pre-fix, get_modified_files() walked the local history
// past the unrelated upstream and reached the local root commit.  Because the root commit
// has no parent, the code dumped *every* file in its tree into the AHEAD set, including
// files like "untouched.txt" that were neither created nor modified relative to the
// upstream's view.  The fix bails out before we can include these phantom changes.
BOOST_AUTO_TEST_CASE( GetDifferentFilesHandlesUnrelatedHistories )
{
    GitInitGuard libgit;
    ScopedTempDir tmp;

    // Local root commit with two files.  The bug previously listed both as AHEAD.
    git_repository* repo = makeRepoWithCommit( tmp.path, "untouched.txt", "stable\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    // Add a second file to local HEAD so we have more than the bare initial tree.
    {
        wxFileName extra( tmp.path, wxS( "second.txt" ) );
        std::ofstream f( extra.GetFullPath().utf8_string() );
        f << "more\n";
    }

    git_index* index = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_index( &index, repo ), GIT_OK );
    KIGIT::GitIndexPtr indexPtr( index );
    BOOST_REQUIRE_EQUAL( git_index_add_bypath( index, "second.txt" ), GIT_OK );
    BOOST_REQUIRE_EQUAL( git_index_write( index ), GIT_OK );

    git_oid newTreeOid;
    BOOST_REQUIRE_EQUAL( git_index_write_tree( &newTreeOid, index ), GIT_OK );

    git_tree* newTree = nullptr;
    BOOST_REQUIRE_EQUAL( git_tree_lookup( &newTree, repo, &newTreeOid ), GIT_OK );
    KIGIT::GitTreePtr newTreePtr( newTree );

    git_reference* head = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_head( &head, repo ), GIT_OK );
    KIGIT::GitReferencePtr headPtr( head );

    git_commit* parentCommit = nullptr;
    BOOST_REQUIRE_EQUAL(
            git_commit_lookup( &parentCommit, repo, git_reference_target( head ) ), GIT_OK );
    KIGIT::GitCommitPtr parentCommitPtr( parentCommit );

    git_signature* sig = nullptr;
    BOOST_REQUIRE_EQUAL( git_signature_now( &sig, "QA Test", "qa@example.com" ), GIT_OK );
    KIGIT::GitSignaturePtr sigPtr( sig );

    const git_commit* parents[1] = { parentCommit };
    git_oid           secondCommitOid;
    BOOST_REQUIRE_EQUAL( git_commit_create( &secondCommitOid, repo, "HEAD", sig, sig, nullptr,
                                            "second", newTree, 1, parents ),
                         GIT_OK );

    // Build a disjoint upstream history with its own root commit and a different file.
    git_treebuilder* tb = nullptr;
    BOOST_REQUIRE_EQUAL( git_treebuilder_new( &tb, repo, nullptr ), GIT_OK );

    git_oid blobOid;
    const std::string upstreamData = "upstream\n";
    BOOST_REQUIRE_EQUAL( git_blob_create_from_buffer( &blobOid, repo, upstreamData.data(),
                                                      upstreamData.size() ),
                         GIT_OK );
    BOOST_REQUIRE_EQUAL( git_treebuilder_insert( nullptr, tb, "remote_only.txt", &blobOid,
                                                 GIT_FILEMODE_BLOB ),
                         GIT_OK );

    git_oid remoteTreeOid;
    BOOST_REQUIRE_EQUAL( git_treebuilder_write( &remoteTreeOid, tb ), GIT_OK );
    git_treebuilder_free( tb );

    git_tree* remoteTree = nullptr;
    BOOST_REQUIRE_EQUAL( git_tree_lookup( &remoteTree, repo, &remoteTreeOid ), GIT_OK );
    KIGIT::GitTreePtr remoteTreePtr( remoteTree );

    git_oid remoteCommitOid;
    BOOST_REQUIRE_EQUAL( git_commit_create( &remoteCommitOid, repo, nullptr, sig, sig, nullptr,
                                            "upstream root", remoteTree, 0, nullptr ),
                         GIT_OK );

    git_reference* remoteRef = nullptr;
    BOOST_REQUIRE_EQUAL( git_reference_create( &remoteRef, repo, "refs/remotes/origin/main",
                                               &remoteCommitOid, true, nullptr ),
                         GIT_OK );
    KIGIT::GitReferencePtr remoteRefPtr( remoteRef );

    git_config* cfg = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_config( &cfg, repo ), GIT_OK );
    KIGIT::GitConfigPtr cfgPtr( cfg );
    BOOST_REQUIRE_EQUAL( git_config_set_string( cfg, "branch.main.remote", "origin" ),
                         GIT_OK );
    BOOST_REQUIRE_EQUAL( git_config_set_string( cfg, "branch.main.merge", "refs/heads/main" ),
                         GIT_OK );

    KIGIT_COMMON common( repo );
    auto [local, remote] = common.GetDifferentFiles();

    // With unrelated histories there is no merge base, so the implementation cannot
    // attribute changes to either side.  Pre-fix, the revwalk reached both root commits
    // and dumped both trees in full; the new merge-base approach short-circuits and
    // reports no AHEAD/BEHIND files in this ambiguous state.  In particular, the
    // never-touched-locally "untouched.txt" must not appear in the AHEAD set.
    BOOST_CHECK_MESSAGE( local.find( wxS( "untouched.txt" ) ) == local.end(),
                         "untouched.txt should not be reported as AHEAD when the local and "
                         "remote histories share no ancestor" );
    BOOST_CHECK_MESSAGE( remote.find( wxS( "untouched.txt" ) ) == remote.end(),
                         "untouched.txt should not be reported as BEHIND when the local "
                         "and remote histories share no ancestor" );
}


// When local has commits ahead of a real upstream, only files that actually changed in those
// local commits should be reported as AHEAD.  Files that exist in both trees unchanged must
// not show up as ahead.  This is the common-case behaviour the issue reproducer exercises.
BOOST_AUTO_TEST_CASE( GetDifferentFilesReportsOnlyTouchedFilesWhenAhead )
{
    GitInitGuard libgit;
    ScopedTempDir tmp;

    // Initial commit becomes the shared base.  Two files: only one will be modified later.
    git_repository* repo = makeRepoWithCommit( tmp.path, "untouched.txt", "stable\n" );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    {
        wxFileName extra( tmp.path, wxS( "touched.txt" ) );
        std::ofstream f( extra.GetFullPath().utf8_string() );
        f << "v1\n";
    }

    git_index* index = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_index( &index, repo ), GIT_OK );
    KIGIT::GitIndexPtr indexPtr( index );
    BOOST_REQUIRE_EQUAL( git_index_add_bypath( index, "touched.txt" ), GIT_OK );
    BOOST_REQUIRE_EQUAL( git_index_write( index ), GIT_OK );

    git_oid           treeOid;
    BOOST_REQUIRE_EQUAL( git_index_write_tree( &treeOid, index ), GIT_OK );

    git_tree* tree = nullptr;
    BOOST_REQUIRE_EQUAL( git_tree_lookup( &tree, repo, &treeOid ), GIT_OK );
    KIGIT::GitTreePtr treePtr( tree );

    git_reference* head = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_head( &head, repo ), GIT_OK );
    KIGIT::GitReferencePtr headPtr( head );

    git_commit* parent = nullptr;
    BOOST_REQUIRE_EQUAL( git_commit_lookup( &parent, repo, git_reference_target( head ) ),
                         GIT_OK );
    KIGIT::GitCommitPtr parentPtr( parent );

    git_signature* sig = nullptr;
    BOOST_REQUIRE_EQUAL( git_signature_now( &sig, "QA Test", "qa@example.com" ), GIT_OK );
    KIGIT::GitSignaturePtr sigPtr( sig );

    const git_commit* parents[1] = { parent };
    git_oid           base_oid;
    BOOST_REQUIRE_EQUAL( git_commit_create( &base_oid, repo, "HEAD", sig, sig, nullptr,
                                            "add touched", tree, 1, parents ),
                         GIT_OK );

    // Mark this commit as the upstream tip and configure a remote so libgit2 accepts the
    // branch.main.remote = origin pointer below.
    git_remote* origin = nullptr;
    BOOST_REQUIRE_EQUAL( git_remote_create( &origin, repo, "origin",
                                            "git@example.com:repo.git" ),
                         GIT_OK );
    KIGIT::GitRemotePtr originPtr( origin );

    git_reference* upstream_ref = nullptr;
    BOOST_REQUIRE_EQUAL( git_reference_create( &upstream_ref, repo, "refs/remotes/origin/main",
                                               &base_oid, true, nullptr ),
                         GIT_OK );
    KIGIT::GitReferencePtr upstreamRefPtr( upstream_ref );

    git_config* cfg = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_config( &cfg, repo ), GIT_OK );
    KIGIT::GitConfigPtr cfgPtr( cfg );
    BOOST_REQUIRE_EQUAL( git_config_set_string( cfg, "branch.main.remote", "origin" ),
                         GIT_OK );
    BOOST_REQUIRE_EQUAL( git_config_set_string( cfg, "branch.main.merge", "refs/heads/main" ),
                         GIT_OK );

    // Local-only commit modifying just touched.txt.
    {
        wxFileName extra( tmp.path, wxS( "touched.txt" ) );
        std::ofstream f( extra.GetFullPath().utf8_string() );
        f << "v2\n";
    }

    BOOST_REQUIRE_EQUAL( git_index_add_bypath( index, "touched.txt" ), GIT_OK );
    BOOST_REQUIRE_EQUAL( git_index_write( index ), GIT_OK );

    git_oid newTreeOid;
    BOOST_REQUIRE_EQUAL( git_index_write_tree( &newTreeOid, index ), GIT_OK );

    git_tree* newTree = nullptr;
    BOOST_REQUIRE_EQUAL( git_tree_lookup( &newTree, repo, &newTreeOid ), GIT_OK );
    KIGIT::GitTreePtr newTreePtr( newTree );

    git_commit* baseCommit = nullptr;
    BOOST_REQUIRE_EQUAL( git_commit_lookup( &baseCommit, repo, &base_oid ), GIT_OK );
    KIGIT::GitCommitPtr baseCommitPtr( baseCommit );

    const git_commit* aheadParents[1] = { baseCommit };
    git_oid           aheadOid;
    BOOST_REQUIRE_EQUAL( git_commit_create( &aheadOid, repo, "HEAD", sig, sig, nullptr,
                                            "modify touched", newTree, 1, aheadParents ),
                         GIT_OK );

    KIGIT_COMMON common( repo );
    auto [local, remote] = common.GetDifferentFiles();

    BOOST_CHECK( remote.empty() );
    BOOST_CHECK_MESSAGE( local.find( wxS( "touched.txt" ) ) != local.end(),
                         "touched.txt should be reported as AHEAD" );
    BOOST_CHECK_MESSAGE( local.find( wxS( "untouched.txt" ) ) == local.end(),
                         "untouched.txt should NOT be reported as AHEAD - this was the "
                         "regression in issue 21576" );
}


BOOST_AUTO_TEST_SUITE_END()
