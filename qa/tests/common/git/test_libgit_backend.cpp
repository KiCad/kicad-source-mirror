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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file
 * Regression tests for LIBGIT_BACKEND, KIGIT_COMMON, GIT_INIT_HANDLER,
 * GIT_PULL_HANDLER, GIT_PUSH_HANDLER, GIT_COMMIT_HANDLER focused on the recent
 * Git integration bug fixes and the Amend Last Commit feature.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <dialogs/git/git_status_entries.h>
#include <git/git_backend.h>
#include <git/libgit_backend.h>
#include <git/kicad_git_common.h>
#include <git/kicad_git_memory.h>
#include <git/git_status_handler.h>
#include <git/project_git_utils.h>
#include <wildcards_and_files_ext.h>
#include <git/git_init_handler.h>
#include <git/git_branch_handler.h>
#include <git/git_pull_handler.h>
#include <git/git_push_handler.h>
#include <git/git_commit_handler.h>
#include <git/git_clone_handler.h>
#include <qa_utils/git_test_fixture.h>

#include <git2.h>

#include <project.h>
#include <project/project_local_settings.h>
#include <settings/settings_manager.h>

#ifndef __WINDOWS__
#include <sys/stat.h>
#include <unistd.h>
#endif

#include <fstream>
#include <future>
#include <set>
#include <thread>

#include <wx/ffile.h>
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/filesys.h>
#include <wx/textfile.h>


static const char* TEST_AUTHOR_NAME = "Test Author";
static const char* TEST_AUTHOR_EMAIL = "test@kicad.example";


/**
 * Build a temp directory tree containing a local working repo with one commit
 * and a bare "remote" repo cloned from it.  Helpers let individual tests
 * mutate this state (additional commits, upstream config, amends, etc.).
 */
struct GIT_BACKEND_FIXTURE : KI_TEST::GIT_TEST_FIXTURE
{
    GIT_BACKEND_FIXTURE()
    {
        m_repoPath = TempPath() + wxFileName::GetPathSeparator() + wxT( "repo" );
        m_remotePath = TempPath() + wxFileName::GetPathSeparator() + wxT( "remote.git" );
        m_ready = initRepoWithCommit() && initBareRemoteFromRepo();
    }


    bool            ready() const { return m_ready; }
    const wxString& repoPath() const { return m_repoPath; }
    const wxString& tempBase() const { return TempPath(); }
    const wxString& remotePath() const { return m_remotePath; }


    /// Open the working repo.  Caller frees with git_repository_free.
    git_repository* openRepo() const
    {
        git_repository* repo = nullptr;
        git_repository_open( &repo, m_repoPath.ToUTF8().data() );
        return repo;
    }


    /// Create a new commit on the current branch from a single (path, content) pair.
    git_oid createCommit( git_repository* aRepo, const wxString& aFile, const wxString& aContent,
                          const wxString& aMessage )
    {
        KI_TEST::WriteGitFile( m_repoPath, aFile, aContent.utf8_string() );
        return KI_TEST::CommitGitFile( aRepo, aFile, aMessage );
    }


    /// Create a commit on top of aParentOid setting aFile to aContent, without moving HEAD.
    git_oid createDetachedCommit( git_repository* aRepo, const git_oid& aParentOid, const wxString& aFile,
                                  const std::string& aContent, const char* aMessage )
    {
        git_oid result = {};

        git_commit* parentCommit = nullptr;
        BOOST_REQUIRE_EQUAL( git_commit_lookup( &parentCommit, aRepo, &aParentOid ), GIT_OK );

        git_oid blobOid;
        BOOST_REQUIRE_EQUAL( git_blob_create_from_buffer( &blobOid, aRepo, aContent.data(), aContent.size() ), GIT_OK );

        git_tree* parentTree = nullptr;
        BOOST_REQUIRE_EQUAL( git_commit_tree( &parentTree, parentCommit ), GIT_OK );

        git_treebuilder* builder = nullptr;
        BOOST_REQUIRE_EQUAL( git_treebuilder_new( &builder, aRepo, parentTree ), GIT_OK );
        BOOST_REQUIRE_EQUAL(
                git_treebuilder_insert( nullptr, builder, aFile.ToUTF8().data(), &blobOid, GIT_FILEMODE_BLOB ),
                GIT_OK );

        git_oid treeOid;
        BOOST_REQUIRE_EQUAL( git_treebuilder_write( &treeOid, builder ), GIT_OK );
        git_treebuilder_free( builder );
        git_tree_free( parentTree );

        git_tree* tree = nullptr;
        BOOST_REQUIRE_EQUAL( git_tree_lookup( &tree, aRepo, &treeOid ), GIT_OK );

        git_signature* sig = nullptr;
        BOOST_REQUIRE_EQUAL( git_signature_now( &sig, TEST_AUTHOR_NAME, TEST_AUTHOR_EMAIL ), GIT_OK );

        const git_commit* parents[1] = { parentCommit };
        BOOST_REQUIRE_EQUAL(
                git_commit_create( &result, aRepo, nullptr, sig, sig, nullptr, aMessage, tree, 1, parents ), GIT_OK );

        git_signature_free( sig );
        git_tree_free( tree );
        git_commit_free( parentCommit );
        return result;
    }

    /// Write .git/FETCH_HEAD naming aOid, as a fetch would leave it.
    void writeFetchHead( const git_oid& aOid )
    {
        char hex[GIT_OID_HEXSZ + 1] = { 0 };
        git_oid_tostr( hex, sizeof( hex ), &aOid );

        wxString      fetchHead = repoPath() + wxFileName::GetPathSeparator() + wxT( ".git" )
                                  + wxFileName::GetPathSeparator() + wxT( "FETCH_HEAD" );
        std::ofstream f( fetchHead.ToStdString() );
        f << hex << "\t\tbranch 'master' of test\n";
    }

    /// Add an `origin` remote pointing at the bare remote on disk.
    bool addOrigin( git_repository* aRepo )
    {
        git_remote* remote = nullptr;
        int rc = git_remote_create( &remote, aRepo, "origin", m_remotePath.ToUTF8().data() );

        if( remote )
            git_remote_free( remote );

        return rc == 0;
    }


    /// Drop branch.\<aBranch\>.merge / .remote, simulating a repo where Add Version
    /// Control wired up `origin` but never set per-branch upstream tracking.
    bool clearUpstreamConfig( git_repository* aRepo, const wxString& aBranch )
    {
        git_config* cfg = nullptr;

        if( git_repository_config( &cfg, aRepo ) != 0 )
            return false;

        wxString remoteKey = wxString::Format( "branch.%s.remote", aBranch );
        wxString mergeKey = wxString::Format( "branch.%s.merge", aBranch );

        git_config_delete_entry( cfg, remoteKey.ToUTF8().data() );
        git_config_delete_entry( cfg, mergeKey.ToUTF8().data() );

        git_config_free( cfg );
        return true;
    }


    /// Set branch.\<aBranch\>.merge = refs/heads/\<aBranch\>, branch.\<aBranch\>.remote = origin.
    bool setUpstreamConfig( git_repository* aRepo, const wxString& aBranch )
    {
        git_config* cfg = nullptr;

        if( git_repository_config( &cfg, aRepo ) != 0 )
            return false;

        wxString remoteKey = wxString::Format( "branch.%s.remote", aBranch );
        wxString mergeKey = wxString::Format( "branch.%s.merge", aBranch );
        wxString mergeVal = wxString::Format( "refs/heads/%s", aBranch );

        git_config_set_string( cfg, remoteKey.ToUTF8().data(), "origin" );
        git_config_set_string( cfg, mergeKey.ToUTF8().data(), mergeVal.ToUTF8().data() );
        git_config_free( cfg );
        return true;
    }


    /// Read a string value from the repo config, or "" if absent.
    wxString readConfig( git_repository* aRepo, const wxString& aKey )
    {
        git_config* cfg = nullptr;

        if( git_repository_config( &cfg, aRepo ) != 0 )
            return wxEmptyString;

        git_buf  buf = { nullptr, 0, 0 };
        wxString result;

        if( git_config_get_string_buf( &buf, cfg, aKey.ToUTF8().data() ) == 0 && buf.ptr )
            result = wxString::FromUTF8( buf.ptr );

        git_buf_dispose( &buf );
        git_config_free( cfg );
        return result;
    }


    void useEmptyGlobalIgnore( git_repository* aRepo )
    {
        KI_TEST::WriteGitFile( tempBase(), wxS( "global-ignore" ), "" );
        const wxString path = tempBase() + wxS( "/global-ignore" );
        git_config*    rawConfig = nullptr;
        BOOST_REQUIRE_EQUAL( git_repository_config( &rawConfig, aRepo ), GIT_OK );
        KIGIT::GitConfigPtr config( rawConfig );
        BOOST_REQUIRE_EQUAL( git_config_set_string( config.get(), "core.excludesfile", path.utf8_str() ), GIT_OK );
    }


private:
    bool initRepoWithCommit()
    {
        KIGIT::GitRepositoryPtr repo =
                KI_TEST::InitGitRepository( m_repoPath, "master", TEST_AUTHOR_NAME, TEST_AUTHOR_EMAIL );

        git_config* cfg = nullptr;
        BOOST_REQUIRE_EQUAL( git_repository_config( &cfg, repo.get() ), GIT_OK );
        KIGIT::GitConfigPtr config( cfg );
        BOOST_REQUIRE_EQUAL( git_config_set_bool( cfg, "pull.rebase", 0 ), GIT_OK );

        createCommit( repo.get(), wxT( "file.txt" ), wxT( "initial\n" ), wxT( "Initial commit" ) );
        return true;
    }


    bool initBareRemoteFromRepo()
    {
        git_clone_options opts;
        git_clone_init_options( &opts, GIT_CLONE_OPTIONS_VERSION );
        opts.bare = 1;

        git_repository* bare = nullptr;
        int rc = git_clone( &bare, m_repoPath.ToUTF8().data(), m_remotePath.ToUTF8().data(), &opts );

        if( bare )
            git_repository_free( bare );

        return rc == 0;
    }


    wxString m_repoPath;
    wxString m_remotePath;
    bool     m_ready;
};


BOOST_FIXTURE_TEST_SUITE( LibgitBackend, GIT_BACKEND_FIXTURE )


// The single scope predicate behind every checklist, so a file elsewhere in the repository
// cannot leak into a project commit (issue #15910).
// Getting changes keeps whichever FETCH_HEAD entry libgit2 marked to merge, without reading
// its name. That is only sound while at most one entry is ever marked, so the assumption is
// checked here rather than left for a reader to take on trust.
BOOST_AUTO_TEST_CASE( FetchHeadMarksAtMostOneBranchToMerge )
{
    BOOST_TEST_REQUIRE( ready() );

    git_repository* repo = openRepo();
    BOOST_TEST_REQUIRE( repo );
    KIGIT::GitRepositoryPtr repoPtr( repo );
    BOOST_REQUIRE( addOrigin( repo ) );

    // Two more branches on the remote, named either side of the current one alphabetically.
    {
        git_repository* bare = nullptr;
        BOOST_REQUIRE_EQUAL( git_repository_open( &bare, remotePath().ToUTF8().data() ), GIT_OK );
        KIGIT::GitRepositoryPtr barePtr( bare );

        git_oid head;
        BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &head, bare, "HEAD" ), GIT_OK );

        for( const char* name : { "refs/heads/aaa", "refs/heads/zzz" } )
        {
            git_reference* ref = nullptr;
            BOOST_REQUIRE_EQUAL( git_reference_create( &ref, bare, name, &head, 1, nullptr ), GIT_OK );
            git_reference_free( ref );
        }
    }

    struct MARKED
    {
        int                      total = 0;
        std::vector<std::string> names;
    };

    auto collect = []( git_repository* aRepo )
    {
        MARKED marked;

        auto cb = []( const char* aName, const char*, const git_oid*, unsigned int aIsMerge, void* aPayload ) -> int
        {
            MARKED* m = static_cast<MARKED*>( aPayload );
            m->total++;

            if( aIsMerge )
                m->names.push_back( std::string( aName ? aName : "" ) );

            return 0;
        };

        BOOST_REQUIRE_EQUAL( git_repository_fetchhead_foreach( aRepo, cb, &marked ), GIT_OK );
        return marked;
    };

    KIGIT_COMMON common( repo );

    // Add Version Control wires up origin without per-branch tracking. Nothing is marked, so the
    // by-name fallback below the callback is what chooses the branch.
    {
        BOOST_REQUIRE( clearUpstreamConfig( repo, wxT( "master" ) ) );

        GIT_PULL_HANDLER handler( &common );
        BOOST_REQUIRE_MESSAGE( handler.PerformFetch(), handler.GetErrorString() );

        MARKED marked = collect( repo );
        BOOST_CHECK_EQUAL( marked.total, 3 );
        BOOST_CHECK_EQUAL( marked.names.size(), 0u );
    }

    // With tracking configured exactly one is marked, and it is the branch being tracked, so
    // keeping the last one keeps the right one.
    {
        BOOST_REQUIRE( setUpstreamConfig( repo, wxT( "master" ) ) );

        GIT_PULL_HANDLER handler( &common );
        BOOST_REQUIRE_MESSAGE( handler.PerformFetch(), handler.GetErrorString() );

        MARKED marked = collect( repo );
        BOOST_CHECK_EQUAL( marked.total, 3 );
        BOOST_REQUIRE_EQUAL( marked.names.size(), 1u );
        BOOST_CHECK_EQUAL( marked.names[0], "refs/heads/master" );
    }
}


// KiCad cannot read or write Git LFS, so an operation that would put a pointer file where a
// design file belongs is refused. The remote can start using LFS at any time, so the commit
// being merged has to be judged, not the one already held.
BOOST_AUTO_TEST_CASE( PerformPull_RefusesACommitThatBringsGitLfs )
{
    BOOST_TEST_REQUIRE( ready() );

    git_repository* repo = openRepo();
    BOOST_TEST_REQUIRE( repo );
    KIGIT::GitRepositoryPtr repoPtr( repo );
    BOOST_REQUIRE( addOrigin( repo ) );
    BOOST_REQUIRE( setUpstreamConfig( repo, wxT( "master" ) ) );

    // The remote starts keeping design files in LFS, and moves master on to that commit.
    {
        git_repository* bare = nullptr;
        BOOST_REQUIRE_EQUAL( git_repository_open( &bare, remotePath().ToUTF8().data() ), GIT_OK );
        KIGIT::GitRepositoryPtr barePtr( bare );

        git_oid base;
        BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &base, bare, "refs/heads/master" ), GIT_OK );

        git_oid attrs = createDetachedCommit( bare, base, wxT( ".gitattributes" ), "*.kicad_pcb filter=lfs -text\n",
                                              "Track boards with LFS" );

        git_oid board =
                createDetachedCommit( bare, attrs, wxT( "board.kicad_pcb" ),
                                      "version https://git-lfs.github.com/spec/v1\n"
                                      "oid sha256:0000000000000000000000000000000000000000000000000000000000000000\n"
                                      "size 1\n",
                                      "Add the board" );

        git_reference* ref = nullptr;
        BOOST_REQUIRE_EQUAL( git_reference_create( &ref, bare, "refs/heads/master", &board, 1, nullptr ), GIT_OK );
        git_reference_free( ref );
    }

    KIGIT_COMMON     common( repo, repoPath() );
    GIT_PULL_HANDLER handler( &common );

    PullResult result = handler.PerformPull();

    BOOST_CHECK_MESSAGE( result != PullResult::Success && result != PullResult::FastForward,
                         "the pull should refuse a commit that brings Git LFS in" );
    const wxString error = handler.GetErrorString();

    BOOST_CHECK_MESSAGE( error.Upper().Contains( wxS( "LFS" ) ),
                         "the refusal should name Git LFS, got: " + error.ToStdString() );

    // Above all, the pointer text must not have been written where the board belongs.
    BOOST_CHECK( !wxFileName::FileExists( repoPath() + wxFileName::GetPathSeparator() + wxT( "board.kicad_pcb" ) ) );
}


// Cancel has to reach a push that is already writing objects, as it does a fetch that is
// already receiving them. libgit2 abandons the transfer when the progress callback refuses.
BOOST_AUTO_TEST_CASE( PushProgressStopsWhenTheOperationIsCancelled )
{
    BOOST_TEST_REQUIRE( ready() );

    git_repository* repo = openRepo();
    BOOST_TEST_REQUIRE( repo );
    KIGIT::GitRepositoryPtr repoPtr( repo );

    KIGIT_COMMON     common( repo );
    GIT_PUSH_HANDLER handler( &common );

    // Running normally, the push carries on.
    BOOST_CHECK_EQUAL( push_transfer_progress_cb( 1, 10, 128, &handler ), 0 );

    handler.CancelThisOperation();

    BOOST_CHECK_MESSAGE( push_transfer_progress_cb( 2, 10, 256, &handler ) != 0,
                         "a cancelled push should stop at its next progress tick" );
}


BOOST_AUTO_TEST_CASE( IsWithinProjectPath_ScopesToProjectDirectory )
{
    const wxString project = wxT( "/repo/proj/" );

    // Files inside the project directory are in scope.
    BOOST_CHECK( KIGIT::PROJECT_GIT_UTILS::IsWithinProjectPath( wxT( "/repo/proj/proj.kicad_sch" ), project ) );
    BOOST_CHECK( KIGIT::PROJECT_GIT_UTILS::IsWithinProjectPath( wxT( "/repo/proj/sub/board.kicad_pcb" ), project ) );

    // Files elsewhere in the repository are not.
    BOOST_CHECK( !KIGIT::PROJECT_GIT_UTILS::IsWithinProjectPath( wxT( "/repo/outside.txt" ), project ) );
    BOOST_CHECK( !KIGIT::PROJECT_GIT_UTILS::IsWithinProjectPath( wxT( "/repo/other/f.txt" ), project ) );

    // A sibling whose name merely shares the project's prefix must not match.
    BOOST_CHECK( !KIGIT::PROJECT_GIT_UTILS::IsWithinProjectPath( wxT( "/repo/proj-extra/f.txt" ), project ) );

    // The predicate normalizes a missing trailing separator before comparing, so the
    // prefix collision above is still rejected when the caller omits the separator.
    BOOST_CHECK( KIGIT::PROJECT_GIT_UTILS::IsWithinProjectPath( wxT( "/repo/proj/f.txt" ), wxT( "/repo/proj" ) ) );
    BOOST_CHECK( !KIGIT::PROJECT_GIT_UTILS::IsWithinProjectPath( wxT( "/repo/proj-extra/f.txt" ),
                                                                 wxT( "/repo/proj" ) ) );

#ifdef __WINDOWS__
    BOOST_CHECK( KIGIT::PROJECT_GIT_UTILS::IsWithinProjectPath( wxT( "C:\\repo\\proj/file.kicad_sch" ),
                                                                 wxT( "C:\\repo\\proj\\" ) ) );
    BOOST_CHECK( !KIGIT::PROJECT_GIT_UTILS::IsWithinProjectPath( wxT( "C:\\repo\\proj-extra/file.kicad_sch" ),
                                                                  wxT( "C:\\repo\\proj\\" ) ) );
#endif

    // An empty project path matches nothing rather than the whole repository.
    BOOST_CHECK( !KIGIT::PROJECT_GIT_UTILS::IsWithinProjectPath( wxT( "/repo/proj/f.txt" ), wxEmptyString ) );
}


// A message-only amend leaves HEAD and upstream at different OIDs with identical trees. Without
// the content-aware filter every file touched since the merge-base showed a clock icon.
BOOST_AUTO_TEST_CASE( GetDifferentFiles_SameTreeAmendHidesAheadFiles )
{
    BOOST_TEST_REQUIRE( ready() );

    git_repository* repo = openRepo();
    BOOST_TEST_REQUIRE( repo );

    // Add a second commit so the merge-base in GetDifferentFiles isn't the root.
    git_oid c1 = createCommit( repo, wxT( "file.txt" ), wxT( "edited\n" ), wxT( "Edit file" ) );

    // Simulate "pushed": create the remote-tracking ref pointing at C1.
    git_reference* trackingRef = nullptr;
    git_reference_create( &trackingRef, repo, "refs/remotes/origin/master", &c1, 1, nullptr );

    if( trackingRef )
        git_reference_free( trackingRef );

    addOrigin( repo );
    setUpstreamConfig( repo, wxT( "master" ) );

    // Amend C1 message-only.  Same tree, different commit OID.
    git_reference* headRef = nullptr;
    git_repository_head( &headRef, repo );

    git_commit* headCommit = nullptr;
    git_reference_peel( (git_object**) &headCommit, headRef, GIT_OBJECT_COMMIT );

    git_tree* tree = nullptr;
    git_commit_tree( &tree, headCommit );

    git_signature* sig = nullptr;
    git_signature_now( &sig, TEST_AUTHOR_NAME, TEST_AUTHOR_EMAIL );

    git_oid amendedOid;
    int     rc = git_commit_amend( &amendedOid, headCommit, "HEAD", sig, sig, nullptr, "Edit file (message-only amend)",
                                   tree );
    BOOST_TEST_REQUIRE( rc == 0 );

    git_tree_free( tree );
    git_commit_free( headCommit );
    git_reference_free( headRef );
    git_signature_free( sig );

    // Now: HEAD points at C1', upstream still at C1, both have the same tree.
    // Pre-filter: AHEAD = { file.txt } (touched in C1 relative to root).
    // Post-filter (the fix under test): AHEAD = {} because HEAD's blob == upstream's blob.
    KIGIT_COMMON common( repo );
    auto [ahead, behind] = common.GetDifferentFiles();

    BOOST_CHECK_MESSAGE( ahead.empty(), "Message-only amend should not report any AHEAD files; got "
                                                + std::to_string( ahead.size() ) );
    BOOST_CHECK_MESSAGE( behind.empty(), "Message-only amend should not report any BEHIND files; got "
                                                 + std::to_string( behind.size() ) );

    git_repository_free( repo );
}


// Positive control for the content-aware filter, which must not over-filter.
BOOST_AUTO_TEST_CASE( GetDifferentFiles_DifferentTreeAmendKeepsAheadFile )
{
    BOOST_TEST_REQUIRE( ready() );

    git_repository* repo = openRepo();
    BOOST_TEST_REQUIRE( repo );

    git_oid c1 = createCommit( repo, wxT( "file.txt" ), wxT( "edited\n" ), wxT( "Edit file" ) );

    git_reference* trackingRef = nullptr;
    git_reference_create( &trackingRef, repo, "refs/remotes/origin/master", &c1, 1, nullptr );

    if( trackingRef )
        git_reference_free( trackingRef );

    addOrigin( repo );
    setUpstreamConfig( repo, wxT( "master" ) );

    // Make a new commit on top with different content (replacing C1 effectively).
    // Use the same path so the file remains "the same file" to the diff.
    createCommit( repo, wxT( "file.txt" ), wxT( "edited again\n" ), wxT( "Re-edit file" ) );

    KIGIT_COMMON common( repo );
    auto [ahead, behind] = common.GetDifferentFiles();

    BOOST_CHECK_MESSAGE( ahead.count( wxT( "file.txt" ) ) == 1,
                         "AHEAD should contain file.txt when content differs from upstream" );

    git_repository_free( repo );
}


// origin without per-branch tracking leaves FETCH_HEAD unmarked, which used to fail (#24332).
BOOST_AUTO_TEST_CASE( PerformPull_NoUpstreamConfig_FallbackSucceedsAndWritesUpstream )
{
    BOOST_TEST_REQUIRE( ready() );

    git_repository* repo = openRepo();
    BOOST_TEST_REQUIRE( repo );

    addOrigin( repo );
    clearUpstreamConfig( repo, wxT( "master" ) );

    KIGIT_COMMON     common( repo );
    GIT_PULL_HANDLER handler( &common );

    PullResult result = handler.PerformPull();

    // Without the fallback, this would be PullResult::Error with "Could not lookup commit".
    BOOST_CHECK_MESSAGE( result == PullResult::UpToDate || result == PullResult::Success
                                 || result == PullResult::FastForward,
                         "Pull without upstream config should succeed via fallback; got "
                                 + std::to_string( static_cast<int>( result ) ) + ", err='"
                                 + handler.GetErrorString().ToStdString() + "'" );

    // The fallback also wires up tracking so subsequent pulls take the normal path.
    BOOST_CHECK_EQUAL( readConfig( repo, wxT( "branch.master.remote" ) ), wxString( "origin" ) );
    BOOST_CHECK_EQUAL( readConfig( repo, wxT( "branch.master.merge" ) ), wxString( "refs/heads/master" ) );

    git_repository_free( repo );
}


/**
 * With pull.rebase set, a pull over diverged history must replay the local commit onto the
 * remote one and leave no rebase in progress.
 */
BOOST_AUTO_TEST_CASE( PerformPull_RebaseConfigReplaysLocalCommit )
{
    BOOST_TEST_REQUIRE( ready() );

    git_repository* repo = openRepo();
    BOOST_TEST_REQUIRE( repo );

    addOrigin( repo );
    setUpstreamConfig( repo, wxT( "master" ) );

    git_object* initial = nullptr;
    BOOST_TEST_REQUIRE( git_revparse_single( &initial, repo, "HEAD" ) == 0 );

    KIGIT_COMMON common( repo );
    git_oid      remoteOid = createCommit( repo, wxT( "remote.txt" ), wxT( "remote\n" ), wxT( "Remote commit" ) );

    {
        GIT_PUSH_HANDLER pushHandler( &common );
        BOOST_TEST_REQUIRE( static_cast<int>( pushHandler.PerformPush() ) == static_cast<int>( PushResult::Success ) );
    }

    BOOST_TEST_REQUIRE( git_reset( repo, initial, GIT_RESET_HARD, nullptr ) == 0 );
    git_object_free( initial );

    createCommit( repo, wxT( "local.txt" ), wxT( "local\n" ), wxT( "Local commit" ) );

    git_config* cfg = nullptr;
    BOOST_TEST_REQUIRE( git_repository_config( &cfg, repo ) == 0 );
    git_config_set_bool( cfg, "pull.rebase", 1 );
    git_config_free( cfg );

    GIT_PULL_HANDLER handler( &common );
    PullResult       result = handler.PerformPull();

    BOOST_CHECK_MESSAGE( result == PullResult::Success,
                         "Rebase pull failed: " + handler.GetErrorString().ToStdString() );
    BOOST_CHECK_EQUAL( git_repository_state( repo ), static_cast<int>( GIT_REPOSITORY_STATE_NONE ) );

    git_commit* head = nullptr;
    BOOST_TEST_REQUIRE( git_revparse_single( (git_object**) &head, repo, "HEAD" ) == 0 );
    BOOST_CHECK( git_oid_equal( git_commit_parent_id( head, 0 ), &remoteOid ) );
    git_commit_free( head );

    git_repository_free( repo );
}


// Tracking config is what lets later operations take the FETCH_HEAD path.
BOOST_AUTO_TEST_CASE( Push_FirstPushSetsUpstreamTracking )
{
    BOOST_TEST_REQUIRE( ready() );

    git_repository* repo = openRepo();
    BOOST_TEST_REQUIRE( repo );

    // Add a new local commit so there is something to actually push beyond the
    // initial state shared with the bare remote.
    createCommit( repo, wxT( "file.txt" ), wxT( "second\n" ), wxT( "Second commit" ) );

    addOrigin( repo );
    clearUpstreamConfig( repo, wxT( "master" ) );

    KIGIT_COMMON     common( repo );
    GIT_PUSH_HANDLER handler( &common );

    PushResult result = handler.PerformPush();
    BOOST_TEST_REQUIRE( static_cast<int>( result ) == static_cast<int>( PushResult::Success ) );

    BOOST_CHECK_EQUAL( readConfig( repo, wxT( "branch.master.remote" ) ), wxString( "origin" ) );
    BOOST_CHECK_EQUAL( readConfig( repo, wxT( "branch.master.merge" ) ), wxString( "refs/heads/master" ) );

    git_repository_free( repo );
}


// A detached HEAD is not a branch, so its name is the literal "HEAD" and there is no branch on
// the remote for it to go to.
BOOST_AUTO_TEST_CASE( Push_RefusesOnADetachedHead )
{
    BOOST_TEST_REQUIRE( ready() );

    git_repository* repo = openRepo();
    BOOST_TEST_REQUIRE( repo );

    createCommit( repo, wxT( "file.txt" ), wxT( "second\n" ), wxT( "Second commit" ) );
    addOrigin( repo );

    git_oid headOid;
    BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &headOid, repo, "HEAD" ), GIT_OK );
    BOOST_REQUIRE_EQUAL( git_repository_set_head_detached( repo, &headOid ), GIT_OK );

    KIGIT_COMMON     common( repo );
    GIT_PUSH_HANDLER handler( &common );

    BOOST_CHECK_EQUAL( static_cast<int>( handler.PerformPush() ), static_cast<int>( PushResult::Error ) );
    const wxString error = handler.GetErrorString();

    BOOST_CHECK_MESSAGE( error.Contains( wxS( "branch" ) ),
                         "the refusal should say the project is not on a branch, got: " + error.ToStdString() );

    git_repository_free( repo );
}


// Behind the upstream. libgit2 reports it through the return code, not the message.
BOOST_AUTO_TEST_CASE( Push_ReportsNonFastForwardWhenRemoteIsAhead )
{
    BOOST_TEST_REQUIRE( ready() );

    git_repository* repo = openRepo();
    BOOST_TEST_REQUIRE( repo );

    {
        git_repository* remote = nullptr;
        BOOST_REQUIRE_EQUAL( git_repository_open( &remote, remotePath().ToUTF8().data() ), GIT_OK );

        git_oid remoteHead;
        BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &remoteHead, remote, "refs/heads/master" ), GIT_OK );

        git_oid ahead = createDetachedCommit( remote, remoteHead, wxT( "theirs.txt" ), "theirs\n", "Remote commit" );

        git_reference* ref = nullptr;
        BOOST_REQUIRE_EQUAL( git_reference_create( &ref, remote, "refs/heads/master", &ahead, 1, nullptr ), GIT_OK );
        git_reference_free( ref );
        git_repository_free( remote );
    }

    createCommit( repo, wxT( "file.txt" ), wxT( "second\n" ), wxT( "Local commit" ) );
    addOrigin( repo );

    KIGIT_COMMON     common( repo );
    GIT_PUSH_HANDLER handler( &common );

    PushResult result = handler.PerformPush();

    BOOST_CHECK_EQUAL( static_cast<int>( result ), static_cast<int>( PushResult::NonFastForward ) );

    git_repository_free( repo );
}


// A refusal the client-side check let through arrives only on the per-reference callback. A
// read-only refs directory provokes one, which directory permissions cannot do on Windows.
#ifndef __WINDOWS__
BOOST_AUTO_TEST_CASE( Push_ReportsErrorWhenRemoteRefusesTheUpdate )
{
    BOOST_TEST_REQUIRE( ready() );

    git_repository* repo = openRepo();
    BOOST_TEST_REQUIRE( repo );

    createCommit( repo, wxT( "file.txt" ), wxT( "second\n" ), wxT( "Local commit" ) );
    addOrigin( repo );

    wxString headsDir = remotePath() + wxFileName::GetPathSeparator() + wxT( "refs" ) + wxFileName::GetPathSeparator()
                        + wxT( "heads" );

    BOOST_REQUIRE( wxFileName::DirExists( headsDir ) );

    // Directory permissions do not bind root, so there the refusal cannot be staged.
    if( geteuid() == 0 )
        return;

    // Put the permissions back however the test ends, or the fixture cannot remove its directory.
    struct WRITABLE_AGAIN
    {
        wxString dir;
        ~WRITABLE_AGAIN() { chmod( dir.fn_str(), 0755 ); }
    } restore{ headsDir };

    chmod( headsDir.fn_str(), 0555 );

    KIGIT_COMMON     common( repo );
    GIT_PUSH_HANDLER handler( &common );

    PushResult result = handler.PerformPush();

    BOOST_CHECK_EQUAL( static_cast<int>( result ), static_cast<int>( PushResult::Error ) );

    // A reason that went missing reads back as the words "No error".
    const std::vector<wxString>& errors = handler.GetErrorStrings();

    BOOST_REQUIRE_EQUAL( errors.size(), 1u );
    BOOST_CHECK_MESSAGE( !errors.front().EndsWith( wxS( "No error" ) ),
                         "the refusal lost git's reason: " + errors.front().ToStdString() );

    git_repository_free( repo );
}
#endif


// Left mid-rebase, every later commit is refused until it is finished from the command line.
BOOST_AUTO_TEST_CASE( PerformPull_RebaseConflictLeavesRepositoryAsItWas )
{
    BOOST_TEST_REQUIRE( ready() );

    git_repository* repo = openRepo();
    BOOST_TEST_REQUIRE( repo );

    {
        git_repository* remote = nullptr;
        BOOST_REQUIRE_EQUAL( git_repository_open( &remote, remotePath().ToUTF8().data() ), GIT_OK );

        git_oid remoteHead;
        BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &remoteHead, remote, "refs/heads/master" ), GIT_OK );

        git_oid ahead = createDetachedCommit( remote, remoteHead, wxT( "file.txt" ), "remote\n", "Remote change" );

        git_reference* ref = nullptr;
        BOOST_REQUIRE_EQUAL( git_reference_create( &ref, remote, "refs/heads/master", &ahead, 1, nullptr ), GIT_OK );
        git_reference_free( ref );
        git_repository_free( remote );
    }

    git_oid localHead = createCommit( repo, wxT( "file.txt" ), wxT( "local\n" ), wxT( "Local change" ) );

    addOrigin( repo );
    setUpstreamConfig( repo, wxT( "master" ) );

    {
        git_config* cfg = nullptr;
        BOOST_REQUIRE_EQUAL( git_repository_config( &cfg, repo ), GIT_OK );
        git_config_set_bool( cfg, "pull.rebase", 1 );
        git_config_free( cfg );
    }

    KIGIT_COMMON     common( repo );
    GIT_PULL_HANDLER handler( &common );

    PullResult result = handler.PerformPull();

    BOOST_CHECK_EQUAL( static_cast<int>( result ), static_cast<int>( PullResult::Conflict ) );
    BOOST_CHECK_EQUAL( git_repository_state( repo ), GIT_REPOSITORY_STATE_NONE );

    git_oid headNow;
    BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &headNow, repo, "HEAD" ), GIT_OK );
    BOOST_CHECK( git_oid_equal( &headNow, &localHead ) );

    wxTextFile file( repoPath() + wxFileName::GetPathSeparator() + wxT( "file.txt" ) );
    BOOST_REQUIRE( file.Open() );
    BOOST_CHECK_EQUAL( file.GetFirstLine(), wxString( "local" ) );

    git_repository_free( repo );
}


// The fast-forward must follow branch.<name>.merge, not a remote branch of the same name.
BOOST_AUTO_TEST_CASE( PerformPull_FastForwardFollowsConfiguredUpstreamBranch )
{
    BOOST_TEST_REQUIRE( ready() );

    git_repository* repo = openRepo();
    BOOST_TEST_REQUIRE( repo );

    git_oid ahead;

    {
        git_repository* remote = nullptr;
        BOOST_REQUIRE_EQUAL( git_repository_open( &remote, remotePath().ToUTF8().data() ), GIT_OK );

        // The remote's branch is called main, while the local one stays master.
        git_reference* masterRef = nullptr;
        BOOST_REQUIRE_EQUAL( git_reference_lookup( &masterRef, remote, "refs/heads/master" ), GIT_OK );

        git_reference* mainRef = nullptr;
        BOOST_REQUIRE_EQUAL( git_reference_rename( &mainRef, masterRef, "refs/heads/main", 0, nullptr ), GIT_OK );
        git_reference_free( masterRef );
        BOOST_REQUIRE_EQUAL( git_repository_set_head( remote, "refs/heads/main" ), GIT_OK );

        git_oid remoteHead = *git_reference_target( mainRef );
        git_reference_free( mainRef );

        ahead = createDetachedCommit( remote, remoteHead, wxT( "theirs.txt" ), "theirs\n", "Remote commit" );

        git_reference* ref = nullptr;
        BOOST_REQUIRE_EQUAL( git_reference_create( &ref, remote, "refs/heads/main", &ahead, 1, nullptr ), GIT_OK );
        git_reference_free( ref );
        git_repository_free( remote );
    }

    addOrigin( repo );

    {
        git_config* cfg = nullptr;
        BOOST_REQUIRE_EQUAL( git_repository_config( &cfg, repo ), GIT_OK );
        git_config_set_string( cfg, "branch.master.remote", "origin" );
        git_config_set_string( cfg, "branch.master.merge", "refs/heads/main" );
        git_config_free( cfg );
    }

    KIGIT_COMMON     common( repo );
    GIT_PULL_HANDLER handler( &common );

    PullResult result = handler.PerformPull();

    BOOST_CHECK_EQUAL( static_cast<int>( result ), static_cast<int>( PullResult::FastForward ) );

    git_oid headNow;
    BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &headNow, repo, "HEAD" ), GIT_OK );
    BOOST_CHECK( git_oid_equal( &headNow, &ahead ) );

    git_repository_free( repo );
}


// Files are written before the branch moves, so a branch that cannot move must not leave the
// tree holding a commit HEAD does not point at.
BOOST_AUTO_TEST_CASE( PerformPull_FastForwardRollsBackWhenBranchCannotMove )
{
    BOOST_TEST_REQUIRE( ready() );

    git_repository* repo = openRepo();
    BOOST_TEST_REQUIRE( repo );

    git_oid before;
    BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &before, repo, "HEAD" ), GIT_OK );

    wxString trackedPath = repoPath() + wxFileName::GetPathSeparator() + wxT( "file.txt" );
    wxString contentBefore;
    {
        wxFFile f( trackedPath, wxT( "rb" ) );
        f.ReadAll( &contentBefore );
    }

    git_oid ahead;
    {
        git_repository* remote = nullptr;
        BOOST_REQUIRE_EQUAL( git_repository_open( &remote, remotePath().ToUTF8().data() ), GIT_OK );

        git_oid remoteHead;
        BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &remoteHead, remote, "refs/heads/master" ), GIT_OK );

        ahead = createDetachedCommit( remote, remoteHead, wxT( "file.txt" ), "changed by remote\n", "Remote edit" );

        git_reference* ref = nullptr;
        BOOST_REQUIRE_EQUAL( git_reference_create( &ref, remote, "refs/heads/master", &ahead, 1, nullptr ), GIT_OK );
        git_reference_free( ref );
        git_repository_free( remote );
    }

    addOrigin( repo );

    // Sits outside the paths the pull touches. Rolling back must leave it exactly as it is.
    wxString bystanderPath = repoPath() + wxFileName::GetPathSeparator() + wxT( "bystander.txt" );
    {
        wxFFile bystander( bystanderPath, wxT( "wb" ) );
        BOOST_TEST_REQUIRE( bystander.IsOpened() );
        bystander.Write( wxT( "written by something else\n" ) );
    }

    // A real reference lock, so the update fails after the checkout, with no production seam.
    wxString refLock = repoPath() + wxFileName::GetPathSeparator() + wxT( ".git" ) + wxFileName::GetPathSeparator()
                       + wxT( "refs" ) + wxFileName::GetPathSeparator() + wxT( "heads" )
                       + wxFileName::GetPathSeparator() + wxT( "master.lock" );
    {
        wxFFile lock( refLock, wxT( "wb" ) );
        BOOST_TEST_REQUIRE( lock.IsOpened() );
        lock.Write( wxT( "locked\n" ) );
    }

    KIGIT_COMMON     common( repo );
    GIT_PULL_HANDLER handler( &common );

    PullResult result = handler.PerformPull();

    BOOST_CHECK_EQUAL( static_cast<int>( result ), static_cast<int>( PullResult::Error ) );

    wxRemoveFile( refLock );
    git_repository_free( repo );

    // Reopen so nothing is answered from a cached index or reference.
    git_repository* fresh = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_open( &fresh, repoPath().ToUTF8().data() ), GIT_OK );

    git_oid headAfter;
    BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &headAfter, fresh, "HEAD" ), GIT_OK );
    BOOST_CHECK_MESSAGE( git_oid_equal( &headAfter, &before ), "HEAD must not have moved" );

    wxString contentAfter;
    {
        wxFFile f( trackedPath, wxT( "rb" ) );
        f.ReadAll( &contentAfter );
    }

    BOOST_CHECK_MESSAGE( contentAfter == contentBefore, "the working file must be back at the commit HEAD points at" );

    // A tree left at the incoming commit would show up here as an uncommitted change.
    git_status_options opts;
    git_status_init_options( &opts, GIT_STATUS_OPTIONS_VERSION );
    opts.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;

    git_status_list* list = nullptr;
    BOOST_REQUIRE_EQUAL( git_status_list_new( &list, fresh, &opts ), GIT_OK );
    size_t dirty = git_status_list_entrycount( list );
    git_status_list_free( list );

    BOOST_CHECK_MESSAGE( dirty == 0, "the rollback must leave no phantom local changes" );

    // Only the paths the pull wrote. Widening it would delete files it never touched.
    BOOST_CHECK_MESSAGE( wxFileName::FileExists( bystanderPath ),
                         "the rollback must not remove a file outside the paths the pull wrote" );

    git_repository_free( fresh );
}


// An untracked directory where the incoming commit wants a file blocks the checkout, and the
// rollback must not take the directory with it.
BOOST_AUTO_TEST_CASE( PerformPull_RollbackKeepsAnUntrackedDirectoryItCannotIdentify )
{
    BOOST_TEST_REQUIRE( ready() );

    git_repository* repo = openRepo();
    BOOST_TEST_REQUIRE( repo );

    // The remote adds a file called "notes".
    {
        git_repository* remote = nullptr;
        BOOST_REQUIRE_EQUAL( git_repository_open( &remote, remotePath().ToUTF8().data() ), GIT_OK );

        git_oid remoteHead;
        BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &remoteHead, remote, "refs/heads/master" ), GIT_OK );

        git_oid ahead = createDetachedCommit( remote, remoteHead, wxT( "notes" ), "from the remote\n", "Add notes" );

        git_reference* ref = nullptr;
        BOOST_REQUIRE_EQUAL( git_reference_create( &ref, remote, "refs/heads/master", &ahead, 1, nullptr ), GIT_OK );
        git_reference_free( ref );
        git_repository_free( remote );
    }

    addOrigin( repo );

    // An untracked directory does not make the tree dirty, and the collision check names the
    // file inside it, so the pull reaches the checkout.
    wxString notesDir = repoPath() + wxFileName::GetPathSeparator() + wxT( "notes" );
    BOOST_REQUIRE( wxFileName::Mkdir( notesDir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) );

    wxString keptFile = notesDir + wxFileName::GetPathSeparator() + wxT( "keep.txt" );
    {
        wxFFile f( keptFile, wxT( "wb" ) );
        BOOST_TEST_REQUIRE( f.IsOpened() );
        f.Write( wxT( "the only copy\n" ) );
    }

    KIGIT_COMMON     common( repo );
    GIT_PULL_HANDLER handler( &common );

    PullResult result = handler.PerformPull();

    // Every failure code is negative, and a checkout over a directory can only fail.
    BOOST_CHECK_MESSAGE( static_cast<int>( result ) < 0, "the checkout cannot succeed over a directory" );

    git_repository_free( repo );

    BOOST_CHECK_MESSAGE( wxFileName::DirExists( notesDir ),
                         "the rollback must not remove an untracked directory it did not write" );
    BOOST_REQUIRE_MESSAGE( wxFileName::FileExists( keptFile ), "the file inside it must survive" );

    wxString after;
    {
        wxFFile f( keptFile, wxT( "rb" ) );
        f.ReadAll( &after );
    }

    BOOST_CHECK_EQUAL( after.ToStdString(), std::string( "the only copy\n" ) );
}


BOOST_AUTO_TEST_CASE( PerformPull_AdoptRemoteHistoryLeavesUnbornBranchOnFailedCheckout )
{
    BOOST_TEST_REQUIRE( ready() );

    wxString emptyDir = tempBase() + wxFileName::GetPathSeparator() + wxT( "unborn_rollback" );
    wxFileName::Mkdir( emptyDir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );

    git_repository* fresh = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_init( &fresh, emptyDir.ToUTF8().data(), 0 ), GIT_OK );

    {
        git_remote* remote = nullptr;
        BOOST_REQUIRE_EQUAL( git_remote_create( &remote, fresh, "origin", remotePath().ToUTF8().data() ), GIT_OK );
        git_remote_free( remote );
    }

    // The checkout refuses, which this path must survive without leaving a branch behind.
    wxString collide = emptyDir + wxFileName::GetPathSeparator() + wxT( "file.txt" );
    {
        wxFFile f( collide, wxT( "wb" ) );
        f.Write( wxT( "mine\n" ) );
    }

    KIGIT_COMMON     common( fresh );
    GIT_PULL_HANDLER handler( &common );

    PullResult result = handler.PerformPull();

    BOOST_CHECK_MESSAGE( result != PullResult::Success && result != PullResult::FastForward,
                         "a colliding untracked file must not be overwritten" );

    git_repository_free( fresh );

    git_repository* reopened = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_open( &reopened, emptyDir.ToUTF8().data() ), GIT_OK );

    BOOST_CHECK_MESSAGE( git_repository_head_unborn( reopened ) == 1,
                         "the branch must still be unborn after a refused checkout" );

    wxString kept;
    {
        wxFFile f( collide, wxT( "rb" ) );
        f.ReadAll( &kept );
    }

    BOOST_CHECK_EQUAL( kept.ToStdString(), std::string( "mine\n" ) );

    git_repository_free( reopened );

    if( wxFileName::DirExists( emptyDir ) )
        wxFileName::Rmdir( emptyDir, wxPATH_RMDIR_RECURSIVE );
}


BOOST_AUTO_TEST_CASE( PerformPull_ExplainsWhenActionLockHeld )
{
    BOOST_TEST_REQUIRE( ready() );

    git_repository* repo = openRepo();
    BOOST_TEST_REQUIRE( repo );

    addOrigin( repo );

    KIGIT_COMMON     common( repo );
    GIT_PULL_HANDLER handler( &common );

    std::unique_lock<std::mutex> held = common.LockAction();

    PullResult result = std::async( std::launch::async,
                                    [&handler]()
                                    {
                                        return handler.PerformPull();
                                    } )
                                .get();

    BOOST_CHECK_EQUAL( static_cast<int>( result ), static_cast<int>( PullResult::Error ) );
    BOOST_CHECK( !handler.GetErrorStrings().empty() );

    held.unlock();
    git_repository_free( repo );
}


// The user has to be told which file, since the checklist may not even show it.
BOOST_AUTO_TEST_CASE( PerformPull_NamesUntrackedFileBlockingFastForward )
{
    BOOST_TEST_REQUIRE( ready() );

    git_repository* repo = openRepo();
    BOOST_TEST_REQUIRE( repo );

    {
        git_repository* remote = nullptr;
        BOOST_REQUIRE_EQUAL( git_repository_open( &remote, remotePath().ToUTF8().data() ), GIT_OK );

        git_oid remoteHead;
        BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &remoteHead, remote, "refs/heads/master" ), GIT_OK );

        git_oid ahead = createDetachedCommit( remote, remoteHead, wxT( "theirs.txt" ), "theirs\n", "Remote adds" );

        git_reference* ref = nullptr;
        BOOST_REQUIRE_EQUAL( git_reference_create( &ref, remote, "refs/heads/master", &ahead, 1, nullptr ), GIT_OK );
        git_reference_free( ref );
        git_repository_free( remote );
    }

    wxString collidingPath = repoPath() + wxFileName::GetPathSeparator() + wxT( "theirs.txt" );
    {
        std::ofstream f( collidingPath.ToStdString() );
        f << "mine, never committed\n";
    }

    addOrigin( repo );
    setUpstreamConfig( repo, wxT( "master" ) );

    git_oid headBefore;
    BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &headBefore, repo, "HEAD" ), GIT_OK );

    KIGIT_COMMON     common( repo );
    GIT_PULL_HANDLER handler( &common );

    PullResult result = handler.PerformPull();

    BOOST_CHECK_EQUAL( static_cast<int>( result ), static_cast<int>( PullResult::DirtyWorkdir ) );
    BOOST_CHECK( handler.GetErrorString().Contains( wxT( "theirs.txt" ) ) );

    git_oid headAfter;
    BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &headAfter, repo, "HEAD" ), GIT_OK );
    BOOST_CHECK( git_oid_equal( &headBefore, &headAfter ) );

    wxTextFile file( collidingPath );
    BOOST_REQUIRE( file.Open() );
    BOOST_CHECK_EQUAL( file.GetFirstLine(), wxString( "mine, never committed" ) );

    git_repository_free( repo );
}


// The same collision when the branches have diverged, which takes the merge path.
BOOST_AUTO_TEST_CASE( PerformPull_NamesUntrackedFileBlockingMerge )
{
    BOOST_TEST_REQUIRE( ready() );

    git_repository* repo = openRepo();
    BOOST_TEST_REQUIRE( repo );

    {
        git_repository* remote = nullptr;
        BOOST_REQUIRE_EQUAL( git_repository_open( &remote, remotePath().ToUTF8().data() ), GIT_OK );

        git_oid remoteHead;
        BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &remoteHead, remote, "refs/heads/master" ), GIT_OK );

        git_oid ahead = createDetachedCommit( remote, remoteHead, wxT( "theirs.txt" ), "theirs\n", "Remote adds" );

        git_reference* ref = nullptr;
        BOOST_REQUIRE_EQUAL( git_reference_create( &ref, remote, "refs/heads/master", &ahead, 1, nullptr ), GIT_OK );
        git_reference_free( ref );
        git_repository_free( remote );
    }

    git_oid localHead = createCommit( repo, wxT( "other.txt" ), wxT( "local\n" ), wxT( "Local change" ) );

    wxString collidingPath = repoPath() + wxFileName::GetPathSeparator() + wxT( "theirs.txt" );
    {
        std::ofstream f( collidingPath.ToStdString() );
        f << "mine, never committed\n";
    }

    addOrigin( repo );
    setUpstreamConfig( repo, wxT( "master" ) );

    KIGIT_COMMON     common( repo );
    GIT_PULL_HANDLER handler( &common );

    PullResult result = handler.PerformPull();

    BOOST_CHECK_EQUAL( static_cast<int>( result ), static_cast<int>( PullResult::DirtyWorkdir ) );
    BOOST_CHECK( handler.GetErrorString().Contains( wxT( "theirs.txt" ) ) );
    BOOST_CHECK_EQUAL( git_repository_state( repo ), GIT_REPOSITORY_STATE_NONE );

    git_oid headAfter;
    BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &headAfter, repo, "HEAD" ), GIT_OK );
    BOOST_CHECK( git_oid_equal( &localHead, &headAfter ) );

    git_repository_free( repo );
}


// With no commit of its own to merge from, getting changes adopts the remote history.
BOOST_AUTO_TEST_CASE( PerformPull_UnbornBranchAdoptsRemoteHistory )
{
    BOOST_TEST_REQUIRE( ready() );

    wxString freshPath = tempBase() + wxFileName::GetPathSeparator() + wxT( "fresh" );

    git_repository*             repo = nullptr;
    git_repository_init_options initOpts;
    git_repository_init_options_init( &initOpts, GIT_REPOSITORY_INIT_OPTIONS_VERSION );
    initOpts.flags = GIT_REPOSITORY_INIT_MKPATH;
    initOpts.initial_head = "master";
    BOOST_REQUIRE_EQUAL( git_repository_init_ext( &repo, freshPath.ToUTF8().data(), &initOpts ), GIT_OK );
    BOOST_REQUIRE_EQUAL( git_repository_head_unborn( repo ), 1 );

    addOrigin( repo );

    KIGIT_COMMON     common( repo );
    GIT_PULL_HANDLER handler( &common );

    PullResult result = handler.PerformPull();

    BOOST_CHECK_EQUAL( static_cast<int>( result ), static_cast<int>( PullResult::FastForward ) );
    BOOST_CHECK_EQUAL( git_repository_head_unborn( repo ), 0 );
    BOOST_CHECK_EQUAL( git_repository_head_detached( repo ), 0 );
    BOOST_CHECK( wxFileName::FileExists( freshPath + wxFileName::GetPathSeparator() + wxT( "file.txt" ) ) );

    git_repository_free( repo );
}


BOOST_AUTO_TEST_CASE( AbandonOperation_ClearsInterruptedMerge )
{
    BOOST_TEST_REQUIRE( ready() );

    git_repository* repo = openRepo();
    BOOST_TEST_REQUIRE( repo );

    git_oid base;
    BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &base, repo, "HEAD" ), GIT_OK );

    git_oid side = createDetachedCommit( repo, base, wxT( "file.txt" ), "side\n", "Side change" );
    git_oid localHead = createCommit( repo, wxT( "file.txt" ), wxT( "local\n" ), wxT( "Local change" ) );

    {
        git_annotated_commit* sideCommit = nullptr;
        BOOST_REQUIRE_EQUAL( git_annotated_commit_lookup( &sideCommit, repo, &side ), GIT_OK );

        const git_annotated_commit* heads[1] = { sideCommit };
        git_checkout_options        checkoutOpts = GIT_CHECKOUT_OPTIONS_INIT;
        checkoutOpts.checkout_strategy = GIT_CHECKOUT_SAFE | GIT_CHECKOUT_ALLOW_CONFLICTS;

        BOOST_REQUIRE_EQUAL( git_merge( repo, heads, 1, nullptr, &checkoutOpts ), GIT_OK );
        git_annotated_commit_free( sideCommit );
    }

    BOOST_REQUIRE_EQUAL( git_repository_state( repo ), GIT_REPOSITORY_STATE_MERGE );

    KIGIT_COMMON common( repo );
    wxString     error;

    // The panel names the operation it offers to abandon, so the session has to tell which it is.
    BOOST_CHECK( common.IsOperationInProgress() );
    BOOST_CHECK( common.OperationInProgress() == GitOperation::Merge );
    BOOST_CHECK( common.AbandonOperation( error ) );
    BOOST_CHECK( !common.IsOperationInProgress() );
    BOOST_CHECK( common.OperationInProgress() == GitOperation::None );
    BOOST_CHECK_EQUAL( git_repository_state( repo ), GIT_REPOSITORY_STATE_NONE );

    git_index* index = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_index( &index, repo ), GIT_OK );
    BOOST_CHECK( !git_index_has_conflicts( index ) );
    git_index_free( index );

    git_oid headNow;
    BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &headNow, repo, "HEAD" ), GIT_OK );
    BOOST_CHECK( git_oid_equal( &headNow, &localHead ) );

    wxTextFile file( repoPath() + wxFileName::GetPathSeparator() + wxT( "file.txt" ) );
    BOOST_REQUIRE( file.Open() );
    BOOST_CHECK_EQUAL( file.GetFirstLine(), wxString( "local" ) );

    git_repository_free( repo );
}


// An interrupted rebase also leaves HEAD detached. Abandoning it must put the branch back.
// A bisect is a search, not a half-finished change. Resetting and clearing its bookkeeping
// leaves the project on whichever commit the search had reached, with no way back to where it
// started.
// Abandoning goes back to the last commit for the files the operation touched. Uncommitted work
// in a file it never went near is somebody's own, and a repository can hold several projects.
BOOST_AUTO_TEST_CASE( AbandonOperation_KeepsWorkTheOperationDidNotTouch )
{
    BOOST_TEST_REQUIRE( ready() );

    git_repository* repo = openRepo();
    BOOST_TEST_REQUIRE( repo );

    // A second tracked file, committed before the branch point so neither side changes it.
    createCommit( repo, wxT( "bystander.txt" ), wxT( "keep\n" ), wxT( "Add bystander" ) );

    git_oid base;
    BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &base, repo, "HEAD" ), GIT_OK );

    git_oid sideEdit = createDetachedCommit( repo, base, wxT( "file.txt" ), "side\n", "Side change" );

    // The side also brings a file of its own, which the merge stages as an addition.
    git_oid side = createDetachedCommit( repo, sideEdit, wxT( "fromside.txt" ), "new\n", "Side addition" );

    createCommit( repo, wxT( "file.txt" ), wxT( "local\n" ), wxT( "Local change" ) );

    // The user's own edit, in a file the merge has no business in.
    wxString bystanderPath = repoPath() + wxFileName::GetPathSeparator() + wxT( "bystander.txt" );
    {
        std::ofstream f( bystanderPath.ToStdString() );
        f << "my own work\n";
    }

    {
        git_annotated_commit* sideCommit = nullptr;
        BOOST_REQUIRE_EQUAL( git_annotated_commit_lookup( &sideCommit, repo, &side ), GIT_OK );

        const git_annotated_commit* heads[1] = { sideCommit };
        git_checkout_options        checkoutOpts = GIT_CHECKOUT_OPTIONS_INIT;
        checkoutOpts.checkout_strategy = GIT_CHECKOUT_SAFE | GIT_CHECKOUT_ALLOW_CONFLICTS;

        BOOST_REQUIRE_EQUAL( git_merge( repo, heads, 1, nullptr, &checkoutOpts ), GIT_OK );
        git_annotated_commit_free( sideCommit );
    }

    BOOST_REQUIRE_EQUAL( git_repository_state( repo ), GIT_REPOSITORY_STATE_MERGE );

    KIGIT_COMMON common( repo );
    wxString     error;

    BOOST_REQUIRE_MESSAGE( common.AbandonOperation( error ), error );

    // The merge is gone and its file is back.
    BOOST_CHECK_EQUAL( git_repository_state( repo ), GIT_REPOSITORY_STATE_NONE );

    git_index* index = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_index( &index, repo ), GIT_OK );
    KIGIT::GitIndexPtr indexPtr( index );
    BOOST_CHECK( !git_index_has_conflicts( index ) );

    {
        std::ifstream f( ( repoPath() + wxFileName::GetPathSeparator() + wxT( "file.txt" ) ).ToStdString() );
        std::string   content( ( std::istreambuf_iterator<char>( f ) ), std::istreambuf_iterator<char>() );
        BOOST_CHECK_EQUAL( content, "local\n" );
    }

    // The file the merge brought in is gone again, as git's own abort leaves it.
    BOOST_CHECK( !wxFileName::FileExists( repoPath() + wxFileName::GetPathSeparator() + wxT( "fromside.txt" ) ) );

    // And the edit the merge never touched is still there.
    {
        std::ifstream f( bystanderPath.ToStdString() );
        std::string   content( ( std::istreambuf_iterator<char>( f ) ), std::istreambuf_iterator<char>() );
        BOOST_CHECK_EQUAL( content, "my own work\n" );
    }

    git_repository_free( repo );
}


BOOST_AUTO_TEST_CASE( AbandonOperation_RefusesDuringABisect )
{
    BOOST_TEST_REQUIRE( ready() );

    git_repository* repo = openRepo();
    BOOST_TEST_REQUIRE( repo );

    git_oid head;
    BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &head, repo, "HEAD" ), GIT_OK );

    // What `git bisect start` leaves behind, which is how git itself recognises the state.
    wxString gitDir = repoPath() + wxFileName::GetPathSeparator() + wxT( ".git" ) + wxFileName::GetPathSeparator();

    {
        std::ofstream log( ( gitDir + wxT( "BISECT_LOG" ) ).ToStdString() );
        log << "git bisect start\n";
    }

    {
        std::ofstream start( ( gitDir + wxT( "BISECT_START" ) ).ToStdString() );
        start << "master\n";
    }

    BOOST_REQUIRE_EQUAL( git_repository_state( repo ), GIT_REPOSITORY_STATE_BISECT );

    KIGIT_COMMON common( repo );
    wxString     error;

    BOOST_CHECK_MESSAGE( !common.AbandonOperation( error ), "a bisect must not be abandoned from here" );
    BOOST_CHECK( !error.IsEmpty() );
    BOOST_CHECK_MESSAGE( wxFileName::FileExists( gitDir + wxT( "BISECT_LOG" ) ),
                         "the bisect bookkeeping must survive" );

    wxRemoveFile( gitDir + wxT( "BISECT_LOG" ) );
    wxRemoveFile( gitDir + wxT( "BISECT_START" ) );
    git_repository_free( repo );
}


BOOST_AUTO_TEST_CASE( AbandonOperation_RefusesAnInteractiveRebaseInOurWords )
{
    BOOST_TEST_REQUIRE( ready() );

    git_repository* repo = openRepo();
    BOOST_TEST_REQUIRE( repo );

    const wxString sep = wxFileName::GetPathSeparator();

    // What `git rebase -i` leaves behind. libgit2 cannot open one, so the abort fails.
    const wxString rebaseDir = repoPath() + sep + wxT( ".git" ) + sep + wxT( "rebase-merge" );

    BOOST_REQUIRE( wxFileName::Mkdir( rebaseDir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) );

    git_oid head;
    BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &head, repo, "HEAD" ), GIT_OK );

    const std::string headHash = git_oid_tostr_s( &head );

    auto writeRebaseFile = [&]( const wxString& aName, const std::string& aText )
    {
        std::ofstream out( ( rebaseDir + sep + aName ).ToStdString() );
        out << aText << "\n";
    };

    writeRebaseFile( wxT( "interactive" ), "" );
    writeRebaseFile( wxT( "head-name" ), "refs/heads/master" );
    writeRebaseFile( wxT( "onto" ), headHash );
    writeRebaseFile( wxT( "orig-head" ), headHash );

    BOOST_REQUIRE_EQUAL( git_repository_state( repo ), GIT_REPOSITORY_STATE_REBASE_INTERACTIVE );

    KIGIT_COMMON common( repo );
    wxString     error;

    BOOST_CHECK( !common.AbandonOperation( error ) );
    BOOST_CHECK_MESSAGE( error.Contains( wxT( "another program" ) ), error );
    BOOST_CHECK_MESSAGE( !error.Lower().Contains( wxT( "not supported" ) ), error );

    for( const wxString& name : { wxT( "interactive" ), wxT( "head-name" ), wxT( "onto" ), wxT( "orig-head" ) } )
        wxRemoveFile( rebaseDir + sep + name );

    wxFileName::Rmdir( rebaseDir );
    git_repository_free( repo );
}


BOOST_AUTO_TEST_CASE( AbandonOperation_ClearsInterruptedRebase )
{
    BOOST_TEST_REQUIRE( ready() );

    git_repository* repo = openRepo();
    BOOST_TEST_REQUIRE( repo );

    git_oid base;
    BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &base, repo, "HEAD" ), GIT_OK );

    git_oid side = createDetachedCommit( repo, base, wxT( "file.txt" ), "side\n", "Side change" );
    git_oid localHead = createCommit( repo, wxT( "file.txt" ), wxT( "local\n" ), wxT( "Local change" ) );

    {
        git_annotated_commit* upstream = nullptr;
        BOOST_REQUIRE_EQUAL( git_annotated_commit_lookup( &upstream, repo, &side ), GIT_OK );

        git_rebase* rebase = nullptr;
        BOOST_REQUIRE_EQUAL( git_rebase_init( &rebase, repo, nullptr, upstream, nullptr, nullptr ), GIT_OK );

        git_rebase_operation* op = nullptr;
        BOOST_REQUIRE_EQUAL( git_rebase_next( &op, rebase ), GIT_OK );

        // Walk away mid-rebase, as a crash would.
        git_rebase_free( rebase );
        git_annotated_commit_free( upstream );
    }

    BOOST_REQUIRE_EQUAL( git_repository_state( repo ), GIT_REPOSITORY_STATE_REBASE_MERGE );
    BOOST_REQUIRE_EQUAL( git_repository_head_detached( repo ), 1 );

    KIGIT_COMMON common( repo );
    wxString     error;

    BOOST_CHECK( common.AbandonOperation( error ) );
    BOOST_CHECK_EQUAL( git_repository_state( repo ), GIT_REPOSITORY_STATE_NONE );
    BOOST_CHECK_EQUAL( git_repository_head_detached( repo ), 0 );

    git_oid headNow;
    BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &headNow, repo, "HEAD" ), GIT_OK );
    BOOST_CHECK( git_oid_equal( &headNow, &localHead ) );

    wxTextFile file( repoPath() + wxFileName::GetPathSeparator() + wxT( "file.txt" ) );
    BOOST_REQUIRE( file.Open() );
    BOOST_CHECK_EQUAL( file.GetFirstLine(), wxString( "local" ) );

    git_repository_free( repo );
}


// A fast-forward over a locally modified file must refuse, not move the branch past a change
// it could not apply. Otherwise the local edit reads as reverting the remote change.
BOOST_AUTO_TEST_CASE( PerformPull_FastForwardRefusesOverDirtyTrackedFile )
{
    BOOST_TEST_REQUIRE( ready() );

    git_repository* repo = openRepo();
    BOOST_TEST_REQUIRE( repo );

    {
        git_repository* remote = nullptr;
        BOOST_REQUIRE_EQUAL( git_repository_open( &remote, remotePath().ToUTF8().data() ), GIT_OK );

        git_oid remoteHead;
        BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &remoteHead, remote, "refs/heads/master" ), GIT_OK );

        git_oid ahead = createDetachedCommit( remote, remoteHead, wxT( "file.txt" ), "remote\n", "Remote change" );

        git_reference* ref = nullptr;
        BOOST_REQUIRE_EQUAL( git_reference_create( &ref, remote, "refs/heads/master", &ahead, 1, nullptr ), GIT_OK );
        git_reference_free( ref );
        git_repository_free( remote );
    }

    wxString filePath = repoPath() + wxFileName::GetPathSeparator() + wxT( "file.txt" );
    {
        std::ofstream f( filePath.ToStdString() );
        f << "local edit\n";
    }

    addOrigin( repo );
    setUpstreamConfig( repo, wxT( "master" ) );

    git_oid headBefore;
    BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &headBefore, repo, "HEAD" ), GIT_OK );

    KIGIT_COMMON     common( repo );
    GIT_PULL_HANDLER handler( &common );

    PullResult result = handler.PerformPull();

    BOOST_CHECK_EQUAL( static_cast<int>( result ), static_cast<int>( PullResult::DirtyWorkdir ) );

    git_oid headAfter;
    BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &headAfter, repo, "HEAD" ), GIT_OK );
    BOOST_CHECK( git_oid_equal( &headBefore, &headAfter ) );

    wxTextFile file( filePath );
    BOOST_REQUIRE( file.Open() );
    BOOST_CHECK_EQUAL( file.GetFirstLine(), wxString( "local edit" ) );

    git_repository_free( repo );
}


// Equivalent to git checkout HEAD with a path, so other changes and untracked files stay.
BOOST_AUTO_TEST_CASE( DiscardChangesRestoresOnlyTheGivenPaths )
{
    BOOST_TEST_REQUIRE( ready() );

    git_repository* repo = openRepo();
    BOOST_TEST_REQUIRE( repo );

    createCommit( repo, wxT( "keep.txt" ), wxT( "keep\n" ), wxT( "Add keep" ) );
    createCommit( repo, wxT( "gone.txt" ), wxT( "gone\n" ), wxT( "Add gone" ) );

    wxString workDir = wxString::FromUTF8( git_repository_workdir( repo ) );

    auto writeAll = []( const wxString& aPath, const std::string& aContent )
    {
        std::ofstream f( aPath.ToStdString(), std::ios::binary );
        f << aContent;
    };

    auto readAll = []( const wxString& aPath )
    {
        std::ifstream in( aPath.ToStdString(), std::ios::binary );
        return std::string( ( std::istreambuf_iterator<char>( in ) ), std::istreambuf_iterator<char>() );
    };

    writeAll( workDir + wxT( "file.txt" ), "accidental\n" );
    writeAll( workDir + wxT( "keep.txt" ), "still dirty\n" );
    writeAll( workDir + wxT( "new.txt" ), "never committed\n" );
    BOOST_TEST_REQUIRE( wxRemoveFile( workDir + wxT( "gone.txt" ) ) );

    KIGIT_COMMON common( repo );

    BOOST_CHECK( common.IsPathInHead( wxT( "file.txt" ) ) );
    BOOST_CHECK( !common.IsPathInHead( wxT( "new.txt" ) ) );

    wxString error;
    BOOST_REQUIRE_MESSAGE( common.DiscardChanges( { wxT( "file.txt" ), wxT( "gone.txt" ) }, error ), error );

    BOOST_CHECK_EQUAL( readAll( workDir + wxT( "file.txt" ) ), "initial\n" );
    BOOST_CHECK_EQUAL( readAll( workDir + wxT( "gone.txt" ) ), "gone\n" );
    BOOST_CHECK_EQUAL( readAll( workDir + wxT( "keep.txt" ) ), "still dirty\n" );
    BOOST_CHECK_EQUAL( readAll( workDir + wxT( "new.txt" ) ), "never committed\n" );

    git_repository_free( repo );
}


// libgit2 reads checkout paths as wildmatch patterns, so a name holding a glob character must
// be matched literally or the wrong file is restored.
BOOST_AUTO_TEST_CASE( DiscardChangesMatchesPathsLiterally )
{
    BOOST_TEST_REQUIRE( ready() );

    git_repository* repo = openRepo();
    BOOST_TEST_REQUIRE( repo );

    createCommit( repo, wxT( "board[1].txt" ), wxT( "committed\n" ), wxT( "Add bracketed" ) );
    createCommit( repo, wxT( "board1.txt" ), wxT( "other committed\n" ), wxT( "Add plain" ) );

    wxString workDir = wxString::FromUTF8( git_repository_workdir( repo ) );

    auto writeAll = []( const wxString& aPath, const std::string& aContent )
    {
        std::ofstream f( aPath.ToStdString(), std::ios::binary );
        f << aContent;
    };

    auto readAll = []( const wxString& aPath )
    {
        std::ifstream in( aPath.ToStdString(), std::ios::binary );
        return std::string( ( std::istreambuf_iterator<char>( in ) ), std::istreambuf_iterator<char>() );
    };

    writeAll( workDir + wxT( "board[1].txt" ), "edited\n" );
    writeAll( workDir + wxT( "board1.txt" ), "edited too\n" );

    KIGIT_COMMON common( repo );

    wxString error;
    BOOST_REQUIRE_MESSAGE( common.DiscardChanges( { wxT( "board[1].txt" ) }, error ), error );

    BOOST_CHECK_EQUAL( readAll( workDir + wxT( "board[1].txt" ) ), "committed\n" );
    BOOST_CHECK_EQUAL( readAll( workDir + wxT( "board1.txt" ) ), "edited too\n" );

    git_repository_free( repo );
}


// libgit2 reads a status pathspec as a wildmatch pattern, so a folder name holding a glob
// character must be matched literally or the wrong folder is listed.
BOOST_AUTO_TEST_CASE( GetFileStatusMatchesPathspecLiterally )
{
    BOOST_TEST_REQUIRE( ready() );

    git_repository* repo = openRepo();
    BOOST_TEST_REQUIRE( repo );

    wxString workDir = wxString::FromUTF8( git_repository_workdir( repo ) );

    auto writeAll = []( const wxString& aPath, const std::string& aContent )
    {
        wxFileName fn( aPath );
        fn.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );

        std::ofstream f( aPath.ToStdString(), std::ios::binary );
        f << aContent;
    };

    writeAll( workDir + wxT( "a[1]/x.txt" ), "one\n" );
    writeAll( workDir + wxT( "a1/y.txt" ), "two\n" );

    KIGIT_COMMON       common( repo );
    GIT_STATUS_HANDLER handler( &common );

    std::map<wxString, FileStatus> status = handler.GetFileStatus( wxT( "a[1]/" ), true );

    bool bracketed = false;
    bool plain = false;

    for( const auto& [path, fileStatus] : status )
    {
        bracketed |= path.EndsWith( wxT( "a[1]/x.txt" ) );
        plain |= path.EndsWith( wxT( "a1/y.txt" ) );
    }

    BOOST_CHECK_MESSAGE( bracketed, "the folder named in the pathspec was not listed" );
    BOOST_CHECK_MESSAGE( !plain, "a folder the pathspec does not name was listed" );

    git_repository_free( repo );
}


// The panel can only say the remote moved on if the session can tell.
BOOST_AUTO_TEST_CASE( HasUpstreamCommitsSeesTheRemoteMoveOn )
{
    BOOST_TEST_REQUIRE( ready() );

    git_repository* repo = openRepo();
    BOOST_TEST_REQUIRE( repo );

    KIGIT_COMMON common( repo );
    BOOST_REQUIRE( common.AddRemote( wxT( "origin" ), wxT( "https://example.com/x.git" ) ) );

    git_reference* head = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_head( &head, repo ), GIT_OK );

    wxString    branch = wxString::FromUTF8( git_reference_shorthand( head ) );
    std::string trackingRef = ( wxT( "refs/remotes/origin/" ) + branch ).utf8_string();
    std::string upstreamName = ( wxT( "origin/" ) + branch ).utf8_string();
    git_oid     base = *git_reference_target( head );

    git_reference* tracking = nullptr;
    BOOST_REQUIRE_EQUAL( git_reference_create( &tracking, repo, trackingRef.c_str(), &base, 0, nullptr ), GIT_OK );
    git_reference_free( tracking );
    BOOST_REQUIRE_EQUAL( git_branch_set_upstream( head, upstreamName.c_str() ), GIT_OK );

    // In step with the upstream.
    BOOST_CHECK( !common.HasUpstreamCommits() );

    // The upstream moves on.
    git_oid remoteTip = createDetachedCommit( repo, base, wxT( "file.txt" ), "remote\n", "Remote change" );
    BOOST_REQUIRE_EQUAL( git_reference_create( &tracking, repo, trackingRef.c_str(), &remoteTip, 1, nullptr ), GIT_OK );
    git_reference_free( tracking );

    BOOST_CHECK( common.HasUpstreamCommits() );

    git_reference_free( head );
    git_repository_free( repo );
}


// Rebuilding the index from HEAD would drop the conflict stages before libgit2's unmerged-index
// guard could see them, recording the markers as resolved content.
BOOST_AUTO_TEST_CASE( Commit_RefusesConflictedRepository )
{
    BOOST_TEST_REQUIRE( ready() );

    git_repository* repo = openRepo();
    BOOST_TEST_REQUIRE( repo );

    git_oid baseOid;
    BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &baseOid, repo, "HEAD" ), GIT_OK );

    createCommit( repo, wxT( "file.txt" ), wxT( "mine\n" ), wxT( "Local change" ) );

    git_oid remoteCommitOid = createDetachedCommit( repo, baseOid, wxT( "file.txt" ), "remote\n", "Remote change" );

    git_annotated_commit* theirs = nullptr;
    BOOST_REQUIRE_EQUAL( git_annotated_commit_lookup( &theirs, repo, &remoteCommitOid ), GIT_OK );

    const git_annotated_commit* heads[1] = { theirs };
    BOOST_REQUIRE_EQUAL( git_merge( repo, heads, 1, nullptr, nullptr ), GIT_OK );
    git_annotated_commit_free( theirs );

    git_index* index = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_index( &index, repo ), GIT_OK );
    BOOST_REQUIRE( git_index_has_conflicts( index ) );
    git_index_free( index );

    git_oid headBefore;
    BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &headBefore, repo, "HEAD" ), GIT_OK );

    wxString workDir = wxString::FromUTF8( git_repository_workdir( repo ) );

    auto readAll = []( const wxString& aPath )
    {
        std::ifstream in( aPath.ToStdString(), std::ios::binary );
        return std::string( ( std::istreambuf_iterator<char>( in ) ), std::istreambuf_iterator<char>() );
    };

    std::string conflictedBytes = readAll( workDir + wxT( "file.txt" ) );

    GIT_COMMIT_HANDLER handler( repo );

    BOOST_CHECK( handler.PerformCommit( { wxT( "file.txt" ) }, wxT( "Oops" ), TEST_AUTHOR_NAME, TEST_AUTHOR_EMAIL )
                 == CommitResult::Error );
    BOOST_CHECK( !handler.GetErrorString().IsEmpty() );

    BOOST_CHECK( handler.PerformAmend( { wxT( "file.txt" ) }, wxT( "Oops" ), TEST_AUTHOR_NAME, TEST_AUTHOR_EMAIL )
                 == CommitResult::Error );

    // A clean selected file elsewhere must not erase the unrelated conflict either.
    {
        std::ofstream f( ( workDir + wxT( "other.txt" ) ).ToStdString(), std::ios::binary );
        f << "clean\n";
    }

    BOOST_CHECK( handler.PerformCommit( { wxT( "other.txt" ) }, wxT( "Oops" ), TEST_AUTHOR_NAME, TEST_AUTHOR_EMAIL )
                 == CommitResult::Error );

    git_oid headAfter;
    BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &headAfter, repo, "HEAD" ), GIT_OK );
    BOOST_CHECK_EQUAL( git_oid_cmp( &headBefore, &headAfter ), 0 );

    BOOST_REQUIRE_EQUAL( git_repository_index( &index, repo ), GIT_OK );
    BOOST_CHECK( git_index_has_conflicts( index ) );
    git_index_free( index );

    BOOST_CHECK_EQUAL( git_repository_state( repo ), GIT_REPOSITORY_STATE_MERGE );
    BOOST_CHECK_EQUAL( readAll( workDir + wxT( "file.txt" ) ), conflictedBytes );

    // An in-progress merge whose conflicts were staged as resolved is still not an ordinary
    // commit: the single-parent path would record the wrong parent structure.
    BOOST_REQUIRE_EQUAL( git_repository_index( &index, repo ), GIT_OK );
    BOOST_REQUIRE_EQUAL( git_index_add_bypath( index, "file.txt" ), GIT_OK );
    BOOST_REQUIRE_EQUAL( git_index_write( index ), GIT_OK );
    git_index_free( index );

    BOOST_CHECK( handler.PerformCommit( { wxT( "file.txt" ) }, wxT( "Oops" ), TEST_AUTHOR_NAME, TEST_AUTHOR_EMAIL )
                 == CommitResult::Error );
    BOOST_CHECK_EQUAL( git_repository_state( repo ), GIT_REPOSITORY_STATE_MERGE );

    git_repository_free( repo );
}


// A surviving staged entry would leave a phantom New row in the checklist.
BOOST_AUTO_TEST_CASE( DiscardChangesDropsStagedEntryForNeverCommittedFile )
{
    BOOST_TEST_REQUIRE( ready() );

    git_repository* repo = openRepo();
    BOOST_TEST_REQUIRE( repo );

    wxString workDir = wxString::FromUTF8( git_repository_workdir( repo ) );

    {
        std::ofstream f( ( workDir + wxT( "staged.txt" ) ).ToStdString(), std::ios::binary );
        f << "staged outside\n";
    }

    git_index* index = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_index( &index, repo ), GIT_OK );
    BOOST_REQUIRE_EQUAL( git_index_add_bypath( index, "staged.txt" ), GIT_OK );
    BOOST_REQUIRE_EQUAL( git_index_write( index ), GIT_OK );
    git_index_free( index );

    BOOST_TEST_REQUIRE( wxRemoveFile( workDir + wxT( "staged.txt" ) ) );

    KIGIT_COMMON common( repo );
    wxString     error;

    BOOST_REQUIRE_MESSAGE( common.DiscardChanges( { wxT( "staged.txt" ) }, error ), error );
    BOOST_CHECK( !common.HasUncommittedTrackedChanges() );

    git_repository_free( repo );
}


// A second operation that cannot get the lock returns Busy without touching the cancellation
// of the one holding it, and a cancelled operation leaves none behind for the next.
BOOST_AUTO_TEST_CASE( BusyOperationDoesNotConsumeAnotherCancellation )
{
    BOOST_TEST_REQUIRE( ready() );

    git_repository* repo = openRepo();
    BOOST_TEST_REQUIRE( repo );

    addOrigin( repo );

    KIGIT_COMMON common( repo );

    GIT_PUSH_HANDLER           pushA( &common );
    KIGIT_COMMON::CANCEL_TOKEN tokenA = pushA.CancelToken();

    BOOST_REQUIRE( tokenA );
    BOOST_REQUIRE( !tokenA->load() );

    {
        // What the backend does for A once it owns the action lock.
        KIGIT_COMMON::OPERATION_GUARD operationA( &common, tokenA );

        tokenA->store( true );

        // A is holding the action lock, so B cannot start.
        std::unique_lock<std::mutex> heldByA = common.LockAction();

        GIT_PUSH_HANDLER           pushB( &common );
        KIGIT_COMMON::CANCEL_TOKEN tokenB = pushB.CancelToken();

        BOOST_CHECK_MESSAGE( tokenB != tokenA, "each operation must own its own token" );

        // From another thread, since trying a lock from the thread that holds it is not defined.
        PushResult resultB = std::async( std::launch::async,
                                         [&pushB]()
                                         {
                                             return pushB.PerformPush();
                                         } )
                                     .get();

        BOOST_CHECK_EQUAL( static_cast<int>( resultB ), static_cast<int>( PushResult::Busy ) );

        BOOST_CHECK_MESSAGE( tokenA->load(), "a Busy operation must not clear another's cancellation" );
        BOOST_CHECK_MESSAGE( !tokenB->load(), "a Busy operation must not inherit another's cancellation" );
        BOOST_CHECK_MESSAGE( common.GetRecentCommits().empty(),
                             "a Busy operation must leave the active read cancelled" );
    }

    BOOST_CHECK_MESSAGE( !common.GetRecentCommits().empty(),
                         "later reads must not inherit the finished operation's cancellation" );

    // A cancelled operation used to leave its token published for the next one to adopt.
    GIT_PUSH_HANDLER pushC( &common );

    BOOST_CHECK_MESSAGE( pushC.CancelToken() != tokenA, "a later operation must not inherit the retired token" );
    BOOST_CHECK_MESSAGE( !pushC.IsThisOperationCancelled(), "a later operation must start uncancelled" );

    git_repository_free( repo );
}


// Cancel from the first progress update.
class CLONE_CANCEL_REPORTER : public PROGRESS_REPORTER
{
public:
    void SetNumPhases( int ) override {}
    void AddPhases( int ) override {}
    void BeginPhase( int ) override {}
    void AdvancePhase() override {}
    void AdvancePhase( const wxString& ) override {}
    void Report( const wxString& ) override {}
    void SetCurrentProgress( double ) override {}
    void SetMaxProgress( int ) override {}
    void AdvanceProgress() override {}
    void SetTitle( const wxString& ) override {}
    bool IsCancelled() const override { return true; }

    bool KeepRefreshing( bool = false ) override
    {
        m_ticks++;
        return false;
    }

    int Ticks() const { return m_ticks; }

private:
    int m_ticks = 0;
};


// Cancelling in the progress dialog must stop the clone and remove its partial checkout.
BOOST_AUTO_TEST_CASE( Clone_CancelFromProgressReporterStopsTheClone )
{
    BOOST_TEST_REQUIRE( ready() );

    wxString dest = tempBase() + wxFileName::GetPathSeparator() + wxT( "clone_cancelled" );

    KIGIT_COMMON      common( nullptr );
    GIT_CLONE_HANDLER handler( &common );

    handler.SetRemote( wxFileSystem::FileNameToURL( wxFileName( remotePath() ) ) );
    handler.SetClonePath( dest );

    auto                   reporter = std::make_unique<CLONE_CANCEL_REPORTER>();
    CLONE_CANCEL_REPORTER* watcher = reporter.get();

    handler.SetProgressReporter( std::move( reporter ) );

    bool cloned = handler.PerformClone();

    BOOST_REQUIRE_MESSAGE( watcher->Ticks() > 0, "the clone must report progress at least once" );
    BOOST_CHECK_MESSAGE( handler.IsThisOperationCancelled(), "the clone must record the cancellation" );
    BOOST_CHECK_MESSAGE( !cloned, "a clone cancelled from the progress dialog must not report success" );
    BOOST_CHECK_MESSAGE( !wxDirExists( dest ), "a cancelled clone must not leave a directory that blocks a retry" );

    if( common.GetRepo() )
    {
        git_repository_free( common.GetRepo() );
        common.SetRepo( nullptr );
    }
}


// Cancel pressed while the task is still queued must stop it before any network work.
BOOST_AUTO_TEST_CASE( CancelRequestedWhileQueuedAbortsTransfer )
{
    BOOST_TEST_REQUIRE( ready() );

    git_repository* repo = openRepo();
    BOOST_TEST_REQUIRE( repo );

    createCommit( repo, wxT( "file.txt" ), wxT( "second\n" ), wxT( "Second commit" ) );
    addOrigin( repo );

    KIGIT_COMMON common( repo );

    // What the panel does: make the token, then cancel it before the worker starts.
    KIGIT_COMMON::CANCEL_TOKEN token = KIGIT_COMMON::NewCancelToken();
    token->store( true );

    GIT_PULL_HANDLER pullHandler( &common );
    pullHandler.SetCancelToken( token );

    BOOST_CHECK( !pullHandler.PerformFetch() );
    BOOST_CHECK( pullHandler.IsThisOperationCancelled() );

    GIT_PUSH_HANDLER pushHandler( &common );
    pushHandler.SetCancelToken( token );

    BOOST_CHECK_EQUAL( static_cast<int>( pushHandler.PerformPush() ), static_cast<int>( PushResult::Error ) );

    // The session was never shutting down, so it must not look like it afterwards.
    BOOST_CHECK( !common.IsShuttingDown() );

    git_repository_free( repo );
}


// The session-wide shutdown flag reaches an operation that starts after it was raised.
BOOST_AUTO_TEST_CASE( ShutdownRequestAbortsAnOperationThatStartsAfterIt )
{
    BOOST_TEST_REQUIRE( ready() );

    git_repository* repo = openRepo();
    BOOST_TEST_REQUIRE( repo );

    createCommit( repo, wxT( "file.txt" ), wxT( "second\n" ), wxT( "Second commit" ) );
    addOrigin( repo );

    KIGIT_COMMON common( repo );

    common.SetShuttingDown( true );

    GIT_PULL_HANDLER pullHandler( &common );
    BOOST_CHECK( !pullHandler.PerformFetch() );

    GIT_PUSH_HANDLER pushHandler( &common );
    PushResult       result = pushHandler.PerformPush();

    BOOST_CHECK_EQUAL( static_cast<int>( result ), static_cast<int>( PushResult::Error ) );
    BOOST_CHECK( common.IsShuttingDown() );

    git_repository_free( repo );
}


BOOST_AUTO_TEST_CASE( Amend_MessageOnlyRewritesHeadKeepsTree )
{
    BOOST_TEST_REQUIRE( ready() );

    git_repository* repo = openRepo();
    BOOST_TEST_REQUIRE( repo );

    git_reference* headRefBefore = nullptr;
    git_repository_head( &headRefBefore, repo );
    git_oid oidBefore = *git_reference_target( headRefBefore );

    git_commit* commitBefore = nullptr;
    git_reference_peel( (git_object**) &commitBefore, headRefBefore, GIT_OBJECT_COMMIT );
    git_oid treeBefore = *git_commit_tree_id( commitBefore );

    git_commit_free( commitBefore );
    git_reference_free( headRefBefore );

    GIT_COMMIT_HANDLER handler( repo );
    CommitResult result = handler.PerformAmend( {}, wxT( "Amended message" ), TEST_AUTHOR_NAME, TEST_AUTHOR_EMAIL );
    BOOST_CHECK_EQUAL( static_cast<int>( result ), static_cast<int>( CommitResult::Success ) );

    git_reference* headRefAfter = nullptr;
    git_repository_head( &headRefAfter, repo );
    git_oid oidAfter = *git_reference_target( headRefAfter );

    git_commit* commitAfter = nullptr;
    git_reference_peel( (git_object**) &commitAfter, headRefAfter, GIT_OBJECT_COMMIT );
    git_oid treeAfter = *git_commit_tree_id( commitAfter );

    BOOST_CHECK( !git_oid_equal( &oidBefore, &oidAfter ) );
    BOOST_CHECK( git_oid_equal( &treeBefore, &treeAfter ) );

    wxString amendedMsg = wxString::FromUTF8( git_commit_message( commitAfter ) );
    BOOST_CHECK_EQUAL( amendedMsg.Trim(), wxString( "Amended message" ) );

    git_commit_free( commitAfter );
    git_reference_free( headRefAfter );
    git_repository_free( repo );
}


BOOST_AUTO_TEST_CASE( Amend_StagedFileChangesTreeAndKeepsParent )
{
    BOOST_TEST_REQUIRE( ready() );

    git_repository* repo = openRepo();
    BOOST_TEST_REQUIRE( repo );

    git_reference* headRefBefore = nullptr;
    git_repository_head( &headRefBefore, repo );

    git_commit* commitBefore = nullptr;
    git_reference_peel( (git_object**) &commitBefore, headRefBefore, GIT_OBJECT_COMMIT );
    git_oid      treeBefore = *git_commit_tree_id( commitBefore );
    unsigned int parentCountBefore = git_commit_parentcount( commitBefore );

    git_commit_free( commitBefore );
    git_reference_free( headRefBefore );

    // Add a new file to the working tree so the amend has something to stage.
    wxString newFile = repoPath() + wxFileName::GetPathSeparator() + wxT( "new.txt" );
    {
        std::ofstream f( newFile.ToStdString() );
        f << "new content\n";
    }

    GIT_COMMIT_HANDLER handler( repo );
    CommitResult result = handler.PerformAmend( { wxT( "new.txt" ) }, wxT( "Amended with file" ), TEST_AUTHOR_NAME,
                                                TEST_AUTHOR_EMAIL );
    BOOST_CHECK_EQUAL( static_cast<int>( result ), static_cast<int>( CommitResult::Success ) );

    git_reference* headRefAfter = nullptr;
    git_repository_head( &headRefAfter, repo );

    git_commit* commitAfter = nullptr;
    git_reference_peel( (git_object**) &commitAfter, headRefAfter, GIT_OBJECT_COMMIT );
    git_oid      treeAfter = *git_commit_tree_id( commitAfter );
    unsigned int parentCountAfter = git_commit_parentcount( commitAfter );

    BOOST_CHECK( !git_oid_equal( &treeBefore, &treeAfter ) );
    BOOST_CHECK_EQUAL( parentCountAfter, parentCountBefore );

    // new.txt should now be in HEAD's tree.
    git_tree* treeObj = nullptr;
    git_commit_tree( &treeObj, commitAfter );

    const git_tree_entry* entry = git_tree_entry_byname( treeObj, "new.txt" );
    BOOST_CHECK( entry != nullptr );

    git_tree_free( treeObj );
    git_commit_free( commitAfter );
    git_reference_free( headRefAfter );
    git_repository_free( repo );
}


// A repository with no commits yet must fail cleanly rather than crash or write a bogus commit.
BOOST_AUTO_TEST_CASE( Amend_UnbornBranchReturnsError )
{
    BOOST_TEST_REQUIRE( ready() );

    wxString unbornDir = repoPath() + wxT( "_unborn" );
    wxFileName::Mkdir( unbornDir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );

    git_repository* unbornRepo = nullptr;
    BOOST_TEST_REQUIRE( git_repository_init( &unbornRepo, unbornDir.ToUTF8().data(), 0 ) == 0 );

    GIT_COMMIT_HANDLER handler( unbornRepo );
    CommitResult       result = handler.PerformAmend( {}, wxT( "should fail" ), TEST_AUTHOR_NAME, TEST_AUTHOR_EMAIL );

    BOOST_CHECK_EQUAL( static_cast<int>( result ), static_cast<int>( CommitResult::Error ) );
    BOOST_CHECK( !handler.GetErrorString().IsEmpty() );

    git_repository_free( unbornRepo );

    if( wxFileName::DirExists( unbornDir ) )
        wxFileName::Rmdir( unbornDir, wxPATH_RMDIR_RECURSIVE );
}


BOOST_AUTO_TEST_CASE( SetupRemote_PreservesSshSchemePortAndUsername )
{
    BOOST_TEST_REQUIRE( ready() );

    KIGIT::GitRepositoryPtr repo( openRepo() );
    BOOST_REQUIRE( repo );

    KIGIT_COMMON     common( repo.get() );
    GIT_INIT_HANDLER handler( &common );
    RemoteConfig     config;
    config.url = wxS( "ssh://runner@192.168.1.55:29022/srv/git/repos/l3-auth-30.git" );
    config.username = wxS( "saved-user" );
    config.password = wxS( "key-passphrase" );
    config.connType = KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_SSH;

    BOOST_REQUIRE( handler.SetupRemote( config ) );
    BOOST_CHECK_EQUAL( readConfig( repo.get(), wxS( "remote.origin.url" ) ).ToStdString(), config.url.ToStdString() );
    BOOST_CHECK_EQUAL( common.GetRemote().ToStdString(), config.url.ToStdString() );
    BOOST_CHECK_EQUAL( common.GetUsername().ToStdString(), "runner" );
    BOOST_CHECK_EQUAL( common.GetPassword().ToStdString(), "key-passphrase" );
    BOOST_CHECK( common.GetConnType() == KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_SSH );
}


BOOST_AUTO_TEST_CASE( SetupRemote_UpdatesExistingOriginWithScpAddress )
{
    BOOST_TEST_REQUIRE( ready() );

    KIGIT::GitRepositoryPtr repo( openRepo() );
    BOOST_REQUIRE( repo );
    BOOST_REQUIRE( addOrigin( repo.get() ) );

    KIGIT_COMMON     common( repo.get() );
    GIT_INIT_HANDLER handler( &common );
    RemoteConfig     config;
    config.url = wxS( "runner@host:path/repo" );
    config.username = wxS( "saved-user" );
    config.connType = KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_SSH;

    BOOST_REQUIRE( handler.SetupRemote( config ) );
    BOOST_CHECK_EQUAL( readConfig( repo.get(), wxS( "remote.origin.url" ) ).ToStdString(), config.url.ToStdString() );
    BOOST_CHECK_EQUAL( common.GetRemote().ToStdString(), config.url.ToStdString() );
    BOOST_CHECK_EQUAL( common.GetUsername().ToStdString(), "runner" );
    BOOST_CHECK( common.GetConnType() == KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_SSH );
}


BOOST_AUTO_TEST_CASE( SetupRemote_ExtractsHttpCredentialsWithoutPersistingThem )
{
    BOOST_TEST_REQUIRE( ready() );

    KIGIT::GitRepositoryPtr repo( openRepo() );
    BOOST_REQUIRE( repo );

    KIGIT_COMMON     common( repo.get() );
    GIT_INIT_HANDLER handler( &common );
    RemoteConfig     config;
    config.url = wxS( "https://url-user:pa@ss:word@example.com:8443/x/y@z.git" );
    config.username = wxS( "saved-user" );
    config.connType = KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_HTTPS;

    BOOST_REQUIRE( handler.SetupRemote( config ) );
    BOOST_CHECK_EQUAL( readConfig( repo.get(), wxS( "remote.origin.url" ) ).ToStdString(),
                       "https://example.com:8443/x/y@z.git" );
    BOOST_CHECK_EQUAL( common.GetRemote().ToStdString(), "https://example.com:8443/x/y@z.git" );
    BOOST_CHECK_EQUAL( common.GetUsername().ToStdString(), "url-user" );
    BOOST_CHECK_EQUAL( common.GetPassword().ToStdString(), "pa@ss:word" );
    BOOST_CHECK( common.GetConnType() == KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_HTTPS );

    // A password typed separately replaces the one pasted in the URL.
    config.password = wxS( "typed@pass:word" );
    BOOST_REQUIRE( handler.SetupRemote( config ) );
    BOOST_CHECK_EQUAL( readConfig( repo.get(), wxS( "remote.origin.url" ) ).ToStdString(),
                       "https://example.com:8443/x/y@z.git" );
    BOOST_CHECK_EQUAL( common.GetPassword().ToStdString(), "typed@pass:word" );
}


BOOST_AUTO_TEST_CASE( SetupRemote_KeepsSeparateHttpCredentialsForImmediateFetch )
{
    BOOST_TEST_REQUIRE( ready() );

    KIGIT::GitRepositoryPtr repo( openRepo() );
    BOOST_REQUIRE( repo );

    KIGIT_COMMON     common( repo.get() );
    GIT_INIT_HANDLER handler( &common );
    RemoteConfig     config;
    config.url = wxS( "http://example.com/repo.git" );
    config.username = wxS( "alice" );
    config.password = wxS( "typed@pass:word" );
    config.connType = KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_HTTPS;

    BOOST_REQUIRE( handler.SetupRemote( config ) );
    BOOST_CHECK_EQUAL( readConfig( repo.get(), wxS( "remote.origin.url" ) ).ToStdString(), config.url.ToStdString() );
    BOOST_CHECK_EQUAL( common.GetRemote().ToStdString(), config.url.ToStdString() );
    BOOST_CHECK_EQUAL( common.GetUsername().ToStdString(), "alice" );
    BOOST_CHECK_EQUAL( common.GetPassword().ToStdString(), "typed@pass:word" );
}


BOOST_AUTO_TEST_CASE( InitializeRepository_SeedsGitignoreWithKicadEntries )
{
    BOOST_TEST_REQUIRE( ready() );

    wxString freshDir = repoPath() + wxT( "_fresh" );
    wxFileName::Mkdir( freshDir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );

    KIGIT_COMMON     common( nullptr );
    GIT_INIT_HANDLER handler( &common );

    InitResult result = handler.InitializeRepository( freshDir );
    BOOST_REQUIRE_EQUAL( static_cast<int>( result ), static_cast<int>( InitResult::Success ) );

    wxFileName gitignoreFile( freshDir, wxT( ".gitignore" ) );
    BOOST_TEST_REQUIRE( gitignoreFile.FileExists() );

    KIGIT::GitRepositoryPtr repo( common.GetRepo() );
    common.SetRepo( nullptr );
    BOOST_REQUIRE( repo );
    useEmptyGlobalIgnore( repo.get() );

    const std::pair<const char*, bool> cases[] = {
        { "board.kicad_pcb", false },
        { "lib.kicad_sym", false },
        { "board.kicad_pcb-bak", true },
        { "board.lck", true },
        { "board.LCK", true },
        { "~board.kicad_pcb", true },
        { "proj.kicad_prl", true },
        { "upper.KiCaD_PrL", true },
        { "proj-backups/board.zip", true },
        { "data-proj-backups/board.kicad_pcb", true },
        { "nested/proj-backups/old/board.zip", true },
        { "proj-backups-old/board.kicad_pcb", false },
        { ".history/board.kicad_pcb", true },
        { ".history/.git/objects/ab/cdef", true },
        { ".history-notes/board.kicad_pcb", false },
        { "named/.history", false },
        { "named/proj-backups", false },
        { "_autosave-board.kicad_pcb", true },
        { "_autosave-design/board.kicad_pcb", true },
        { "fp-info-cache", true },
        { "fp-info-cache.kicad_pcb", false },
        { "cache.kicad_prl/board.kicad_pcb", true },
        { "board.kicad_pcb.mine", true },
        { "board.kicad_pcb.theirs", true },
        { "board.kicad_pcb.mine.1", true },
        { "board.kicad_pcb.theirs.17", true },
        { "board.kicad_pcb.mine.99", true },
        { "board.kicad_pcb.mine.100", false },
        { "board.kicad_pcb.mine.1notes", false },
        { "board.kicad_pcb.theirs.notes", false },
        { "board.mine.kicad_pcb", false },
        { "board.kicad_pcb.1", false },
    };

    for( const auto& [path, transient] : cases )
    {
        BOOST_TEST_CONTEXT( path )
        {
            KI_TEST::WriteGitFile( freshDir, wxString::FromUTF8( path ), "test file\n" );
            int ignored = 0;
            BOOST_REQUIRE_EQUAL( git_ignore_path_is_ignored( &ignored, repo.get(), path ), GIT_OK );
            BOOST_CHECK_EQUAL( ignored != 0, transient );
            BOOST_CHECK_EQUAL( KIGIT::PROJECT_GIT_UTILS::IsTransientGitFile( wxFileName( path ) ), transient );
        }
    }

    repo.reset();

    if( wxFileName::DirExists( freshDir ) )
        wxFileName::Rmdir( freshDir, wxPATH_RMDIR_RECURSIVE );
}


// A curated .gitignore must not be edited. Appending defaults dirtied a fresh clone.
BOOST_AUTO_TEST_CASE( InitializeRepository_LeavesExistingGitignoreUntouched )
{
    BOOST_TEST_REQUIRE( ready() );

    wxString freshDir = repoPath() + wxT( "_existing" );
    wxFileName::Mkdir( freshDir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );

    const wxString curated = wxT( "# my custom ignores\n.history/\nbuild/\n" );
    wxFileName     gitignoreFile( freshDir, wxT( ".gitignore" ) );

    {
        wxFFile f( gitignoreFile.GetFullPath(), wxT( "wb" ) );
        f.Write( curated );
    }

    KIGIT_COMMON     common( nullptr );
    GIT_INIT_HANDLER handler( &common );

    InitResult result = handler.InitializeRepository( freshDir );
    BOOST_CHECK_EQUAL( static_cast<int>( result ), static_cast<int>( InitResult::Success ) );

    wxString after;
    {
        wxFFile f( gitignoreFile.GetFullPath(), wxT( "rb" ) );
        f.ReadAll( &after );
    }

    BOOST_CHECK_EQUAL( after.ToStdString(), curated.ToStdString() );

    if( wxFileName::DirExists( freshDir ) )
        wxFileName::Rmdir( freshDir, wxPATH_RMDIR_RECURSIVE );
}


// An empty file is still the team's file, so it stays empty rather than being filled in.
BOOST_AUTO_TEST_CASE( InitializeRepository_LeavesEmptyGitignoreEmpty )
{
    BOOST_TEST_REQUIRE( ready() );

    wxString freshDir = repoPath() + wxT( "_emptyignore" );
    wxFileName::Mkdir( freshDir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );

    wxFileName gitignoreFile( freshDir, wxT( ".gitignore" ) );

    {
        wxFFile f( gitignoreFile.GetFullPath(), wxT( "wb" ) );
    }

    KIGIT_COMMON     common( nullptr );
    GIT_INIT_HANDLER handler( &common );

    BOOST_CHECK_EQUAL( static_cast<int>( handler.InitializeRepository( freshDir ) ),
                       static_cast<int>( InitResult::Success ) );

    BOOST_TEST_REQUIRE( gitignoreFile.FileExists() );
    BOOST_CHECK_EQUAL( gitignoreFile.GetSize().GetValue(), 0u );

    if( wxFileName::DirExists( freshDir ) )
        wxFileName::Rmdir( freshDir, wxPATH_RMDIR_RECURSIVE );
}


// LoadProject() needs a real project file with a meta section.
static wxString seedProjectFile( const wxString& aDir )
{
    wxString path = aDir + wxFileName::GetPathSeparator() + wxT( "proj.kicad_pro" );

    if( !wxFileName::FileExists( path ) )
    {
        wxFFile f( path, wxT( "w" ) );
        f.Write( wxT( "{\"meta\": {\"filename\": \"proj.kicad_pro\", \"version\": 3}}" ) );
    }

    return path;
}


BOOST_AUTO_TEST_CASE( ProjectGitCommon_LazyCreateAttachRelease )
{
    BOOST_TEST_REQUIRE( ready() );

    wxString projectPath = seedProjectFile( repoPath() );

    SETTINGS_MANAGER mgr;
    BOOST_TEST_REQUIRE( mgr.LoadProject( projectPath ) );

    PROJECT* project = mgr.GetProject( projectPath );
    BOOST_TEST_REQUIRE( project != nullptr );

    std::shared_ptr<KIGIT_COMMON> common = project->GetGitCommon();
    BOOST_TEST_REQUIRE( common != nullptr );
    BOOST_CHECK( common->GetRepo() == nullptr );
    BOOST_CHECK( common == project->GetGitCommon() );

    BOOST_TEST_REQUIRE( project->AttachGitRepository() );
    git_repository* repo = common->GetRepo();
    BOOST_TEST_REQUIRE( repo != nullptr );
    BOOST_CHECK( !common->GetProjectDir().IsEmpty() );

    BOOST_TEST_REQUIRE( project->AttachGitRepository() );
    BOOST_CHECK( common->GetRepo() == repo );

    project->ReleaseGitCommon();

    std::shared_ptr<KIGIT_COMMON> fresh = project->GetGitCommon();
    BOOST_TEST_REQUIRE( fresh != nullptr );
    BOOST_CHECK( fresh != common );
    BOOST_CHECK( fresh->GetRepo() == nullptr );
    BOOST_CHECK( common->GetRepo() == nullptr );

    // Unloading with the repository still attached exercises the destructor release path.
    BOOST_TEST_REQUIRE( project->AttachGitRepository() );
    BOOST_CHECK( mgr.UnloadProject( project, false ) );
}


BOOST_AUTO_TEST_CASE( ProjectGitCommon_ReleaseSkipsFreeWhenBackendGone )
{
    BOOST_TEST_REQUIRE( ready() );

    wxString projectPath = seedProjectFile( repoPath() );

    SETTINGS_MANAGER mgr;
    BOOST_TEST_REQUIRE( mgr.LoadProject( projectPath ) );

    PROJECT* project = mgr.GetProject( projectPath );
    BOOST_TEST_REQUIRE( project != nullptr );
    BOOST_TEST_REQUIRE( project->AttachGitRepository() );

    git_repository* repo = project->GetGitCommon()->GetRepo();
    BOOST_TEST_REQUIRE( repo != nullptr );

    // Simulate the process-exit ordering where the backend is torn down before the
    // last project. The release must clear the handle without freeing it, since a free
    // would call into a shut-down libgit2.
    GIT_BACKEND* backend = GetGitBackend();
    SetGitBackend( nullptr );

    project->ReleaseGitCommon();

    BOOST_CHECK( project->GetGitCommon()->GetRepo() == nullptr );

    SetGitBackend( backend );

    // The release skipped the free on purpose, so reclaim the handle now.
    git_repository_free( repo );
}


BOOST_AUTO_TEST_CASE( ProjectGitCommon_AttachOutsideRepositoryFails )
{
    BOOST_TEST_REQUIRE( ready() );

    wxFileName tempBase( repoPath(), wxEmptyString );
    tempBase.RemoveLastDir();
    wxString outsideDir = tempBase.GetPath() + wxFileName::GetPathSeparator() + wxT( "no_repo_here" );
    wxFileName::Mkdir( outsideDir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );

    wxString projectPath = seedProjectFile( outsideDir );

    SETTINGS_MANAGER mgr;
    BOOST_TEST_REQUIRE( mgr.LoadProject( projectPath ) );

    PROJECT* project = mgr.GetProject( projectPath );
    BOOST_TEST_REQUIRE( project != nullptr );

    BOOST_CHECK( !project->AttachGitRepository() );
    BOOST_CHECK( project->GetGitCommon()->GetRepo() == nullptr );
}


BOOST_AUTO_TEST_CASE( ProjectGitCommon_AttachHonorsDisabledSetting )
{
    BOOST_TEST_REQUIRE( ready() );

    wxString projectPath = seedProjectFile( repoPath() );

    SETTINGS_MANAGER mgr;
    BOOST_TEST_REQUIRE( mgr.LoadProject( projectPath ) );

    PROJECT* project = mgr.GetProject( projectPath );
    BOOST_TEST_REQUIRE( project != nullptr );

    project->GetLocalSettings().m_GitIntegrationDisabled = true;

    BOOST_CHECK( !project->AttachGitRepository() );
    BOOST_CHECK( project->GetGitCommon()->GetRepo() == nullptr );

    project->GetLocalSettings().m_GitIntegrationDisabled = false;

    BOOST_CHECK( project->AttachGitRepository() );
    BOOST_CHECK( project->GetGitCommon()->GetRepo() != nullptr );

    project->ReleaseGitCommon();
}


BOOST_AUTO_TEST_CASE( ProjectGitCommon_ReleaseDefersFreeWhileActionMutexHeld )
{
    BOOST_TEST_REQUIRE( ready() );

    wxString projectPath = seedProjectFile( repoPath() );

    SETTINGS_MANAGER mgr;
    BOOST_TEST_REQUIRE( mgr.LoadProject( projectPath ) );

    PROJECT* project = mgr.GetProject( projectPath );
    BOOST_TEST_REQUIRE( project != nullptr );

    BOOST_TEST_REQUIRE( project->AttachGitRepository() );
    std::shared_ptr<KIGIT_COMMON> common = project->GetGitCommon();

    std::promise<void> lockHeld;
    std::promise<void> allowUnlock;

    std::thread worker(
            [&]()
            {
                std::unique_lock<std::mutex> lock = common->LockAction();
                lockHeld.set_value();
                allowUnlock.get_future().wait();
            } );

    lockHeld.get_future().wait();

    // The worker holds the action mutex, so the release must defer the free to the
    // orphan registry instead of blocking. The repo handle must stay valid for the
    // running operation until it finishes.
    project->ReleaseGitCommon();

    BOOST_CHECK( common->GetRepo() != nullptr );

    allowUnlock.set_value();
    worker.join();

    BOOST_REQUIRE_EQUAL( GetGitBackend()->OrphanRegistry().JoinAll( std::chrono::seconds( 5 ) ), 0u );
    BOOST_CHECK( common->GetRepo() == nullptr );
}


// A listed path that is not a regular file is still a path. A symlink to a directory, and a
// submodule, both look absent to a regular-file test, and recording them as deletions drops
// them out of the project.
#ifndef __WINDOWS__
BOOST_AUTO_TEST_CASE( CommitKeepsAPathThatIsNotARegularFile )
{
    BOOST_TEST_REQUIRE( ready() );

    git_repository* repo = openRepo();
    BOOST_TEST_REQUIRE( repo );

    wxString sep = wxFileName::GetPathSeparator();
    BOOST_REQUIRE( wxFileName::Mkdir( repoPath() + sep + wxT( "shared" ), 0700, wxPATH_MKDIR_FULL ) );

    {
        std::ofstream f( ( repoPath() + sep + wxT( "shared" ) + sep + wxT( "lib.txt" ) ).ToStdString() );
        f << "shared\n";
    }

    BOOST_REQUIRE_EQUAL( symlink( "shared", ( repoPath() + sep + wxT( "link" ) ).ToUTF8().data() ), 0 );

    GIT_COMMIT_HANDLER handler( repo );

    BOOST_REQUIRE( handler.PerformCommit( { wxT( "link" ) }, wxT( "Add the shared library link" ), wxT( "Test Author" ),
                                          wxT( "test@kicad.example" ) )
                   == CommitResult::Success );

    git_oid headOid;
    BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &headOid, repo, "HEAD" ), GIT_OK );

    git_commit* head = nullptr;
    BOOST_REQUIRE_EQUAL( git_commit_lookup( &head, repo, &headOid ), GIT_OK );

    git_tree* tree = nullptr;
    BOOST_REQUIRE_EQUAL( git_commit_tree( &tree, head ), GIT_OK );

    git_tree_entry* entry = nullptr;
    BOOST_CHECK_MESSAGE( git_tree_entry_bypath( &entry, tree, "link" ) == GIT_OK,
                         "the commit dropped the symlink instead of recording it" );

    if( entry )
        git_tree_entry_free( entry );

    git_tree_free( tree );
    git_commit_free( head );
    git_repository_free( repo );
}
#endif


BOOST_AUTO_TEST_CASE( IsTransientGitFile_HandlesDosPaths )
{
    using KIGIT::PROJECT_GIT_UTILS;

    BOOST_CHECK( PROJECT_GIT_UTILS::IsTransientGitFile( wxFileName( "proj\\.history\\board.kicad_pcb", wxPATH_DOS ) ) );
    BOOST_CHECK(
            PROJECT_GIT_UTILS::IsTransientGitFile( wxFileName( "proj\\data-proj-backups\\board.zip", wxPATH_DOS ) ) );
    BOOST_CHECK( PROJECT_GIT_UTILS::IsTransientGitFile( wxFileName( "proj\\settings.KICAD_PRL", wxPATH_DOS ) ) );
    BOOST_CHECK( PROJECT_GIT_UTILS::IsTransientGitFile( wxFileName( "proj\\board.kicad_pcb.theirs.99", wxPATH_DOS ) ) );
    BOOST_CHECK( !PROJECT_GIT_UTILS::IsTransientGitFile(
            wxFileName( "proj\\.history-notes\\board.kicad_pcb", wxPATH_DOS ) ) );
    BOOST_CHECK( !PROJECT_GIT_UTILS::IsTransientGitFile( wxFileName( "proj\\board.kicad_pcb.mine.100", wxPATH_DOS ) ) );
}


BOOST_AUTO_TEST_CASE( BuildGitChangeEntries_KeepsTrackedAndStagedTransientFiles )
{
    BOOST_REQUIRE( ready() );
    KIGIT::GitRepositoryPtr repo( openRepo() );
    BOOST_REQUIRE( repo );
    useEmptyGlobalIgnore( repo.get() );

    // An ancestor outside the project must not hide its files.
    const wxString prefix = wxS( "workspace-backups/project/" );
    const wxString projectPath = repoPath() + wxS( "/" ) + prefix;
    KI_TEST::WriteGitFile( repoPath(), prefix + wxS( ".gitignore" ), "" );
    KI_TEST::WriteGitFile( repoPath(), prefix + wxS( "tracked.kicad_prl" ), "original settings\n" );
    KI_TEST::WriteGitFile( repoPath(), prefix + wxS( "deleted.kicad_pcb.mine" ), "original recovery\n" );
    KI_TEST::CommitAllGitFiles( repo.get(), wxS( "Track files the user chose to keep" ) );

    KI_TEST::WriteGitFile( repoPath(), prefix + wxS( "tracked.kicad_prl" ), "changed settings with more data\n" );
    BOOST_REQUIRE( wxRemoveFile( projectPath + wxS( "deleted.kicad_pcb.mine" ) ) );
    KI_TEST::WriteGitFile( repoPath(), prefix + wxS( "staged.kicad_prl" ), "staged settings\n" );

    git_index* rawIndex = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_index( &rawIndex, repo.get() ), GIT_OK );
    KIGIT::GitIndexPtr index( rawIndex );
    BOOST_REQUIRE_EQUAL( git_index_add_bypath( index.get(), ( prefix + wxS( "staged.kicad_prl" ) ).utf8_str() ),
                         GIT_OK );
    BOOST_REQUIRE_EQUAL( git_index_write( index.get() ), GIT_OK );

    const char* untracked[] = {
        "board.kicad_pcb",           "untracked.kicad_prl",       "board.kicad_pcb.theirs.17",
        "_autosave-board.kicad_pcb", "project-backups/board.zip", ".history/revisions/board.kicad_pcb",
    };

    for( const char* path : untracked )
        KI_TEST::WriteGitFile( repoPath(), prefix + wxString::FromUTF8( path ), "new file\n" );

    const std::set<wxString> expected = {
        prefix + wxS( "board.kicad_pcb" ),
        prefix + wxS( "tracked.kicad_prl" ),
        prefix + wxS( "deleted.kicad_pcb.mine" ),
        prefix + wxS( "staged.kicad_prl" ),
    };

    auto checkEntries = [&repo, &expected]( const wxString& aProjectPath )
    {
        KIGIT_COMMON                 common( repo.get(), aProjectPath );
        std::unique_lock<std::mutex> lock = common.LockAction();
        std::set<wxString>           paths;

        for( const FILE_CHECKLIST_ENTRY& entry : BuildGitChangeEntries( common, aProjectPath, false ) )
            paths.insert( entry.path );

        BOOST_CHECK_EQUAL_COLLECTIONS( paths.begin(), paths.end(), expected.begin(), expected.end() );
    };

    checkEntries( projectPath );

#ifndef __WINDOWS__
    const wxString alias = tempBase() + wxS( "/project-alias" );
    BOOST_REQUIRE_EQUAL( symlink( projectPath.utf8_str(), alias.utf8_str() ), 0 );
    checkEntries( alias );
#endif
}


// WriteTreeToDir must reproduce the full tree of a ref, including files in subdirectories, so a
// hierarchical schematic's sub-sheets are present when the committed side is diffed.
BOOST_AUTO_TEST_CASE( WriteTreeToDir_MaterializesSubdirectoryFiles )
{
    BOOST_TEST_REQUIRE( ready() );

    git_repository* repo = openRepo();
    BOOST_TEST_REQUIRE( repo );

    const wxString sep = wxFileName::GetPathSeparator();

    createCommit( repo, wxT( "root.kicad_sch" ), wxT( "root\n" ), wxT( "add root" ) );

    BOOST_REQUIRE( wxFileName::Mkdir( repoPath() + sep + wxT( "sub" ), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) );
    createCommit( repo, wxT( "sub/child.kicad_sch" ), wxT( "child\n" ), wxT( "add child" ) );

    // Under the fixture's own directory, so a failed run leaves nothing behind in the system temp.
    wxString dest = tempBase() + wxFileName::GetPathSeparator() + wxT( "tree_out" );

    BOOST_CHECK( KIGIT::PROJECT_GIT_UTILS::WriteTreeToDir( repo, wxT( "HEAD" ), dest ) );
    BOOST_CHECK( wxFileName::FileExists( dest + sep + wxT( "root.kicad_sch" ) ) );
    BOOST_CHECK_MESSAGE( wxFileName::FileExists( dest + sep + wxT( "sub" ) + sep + wxT( "child.kicad_sch" ) ),
                         "sub-sheet file in a subdirectory must be materialized alongside the root" );

    if( wxFileName::DirExists( dest ) )
        wxFileName::Rmdir( dest, wxPATH_RMDIR_RECURSIVE );

    git_repository_free( repo );
}


// The repository must stay untouched until the final commit, and that commit must also bring the
// working tree to the merged content.
BOOST_AUTO_TEST_CASE( MergeConflictRoundTrip )
{
    BOOST_TEST_REQUIRE( ready() );

    git_repository* repo = openRepo();
    BOOST_TEST_REQUIRE( repo );

    // Remember the shared base, then diverge: "mine" on HEAD, "remote" on a side commit.
    git_reference* headRef = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_head( &headRef, repo ), GIT_OK );
    git_oid baseOid = *git_reference_target( headRef );
    git_reference_free( headRef );

    createCommit( repo, wxT( "file.txt" ), wxT( "mine\n" ), wxT( "Local change" ) );

    // The remote-side commit shares the base but does not move HEAD.
    git_oid remoteCommitOid = createDetachedCommit( repo, baseOid, wxT( "file.txt" ), "remote\n", "Remote change" );

    // A pull would leave this in FETCH_HEAD.
    writeFetchHead( remoteCommitOid );

    KIGIT_COMMON common( repo );

    wxString                          stageDir = tempBase() + wxFileName::GetPathSeparator() + wxT( "stages" );
    wxString                          headOid, theirsOid, conflictsError;
    KIGIT_ERROR_MESSAGE               error;
    std::vector<KIGIT_CONFLICT_SIDES> conflicts;

    BOOST_REQUIRE_MESSAGE( common.GetMergeConflicts( stageDir, headOid, theirsOid, conflicts, conflictsError ),
                           "GetMergeConflicts failed: " + conflictsError.ToStdString() );
    BOOST_REQUIRE_EQUAL( conflicts.size(), 1u );
    BOOST_CHECK_EQUAL( conflicts[0].path.ToStdString(), "file.txt" );
    BOOST_CHECK( conflicts[0].hasAncestor && conflicts[0].hasOurs && conflicts[0].hasTheirs );

    // Reported so the resolutions can be pinned to it.
    {
        char expect[GIT_OID_HEXSZ + 1] = { 0 };
        git_oid_tostr( expect, sizeof( expect ), &remoteCommitOid );
        BOOST_CHECK_EQUAL( theirsOid.ToStdString(), std::string( expect ) );
    }

    auto readAll = []( const wxString& aPath )
    {
        std::ifstream in( aPath.ToStdString(), std::ios::binary );
        return std::string( ( std::istreambuf_iterator<char>( in ) ), std::istreambuf_iterator<char>() );
    };

    BOOST_CHECK_EQUAL( readAll( conflicts[0].oursFile ), "mine\n" );
    BOOST_CHECK_EQUAL( readAll( conflicts[0].theirsFile ), "remote\n" );
    BOOST_CHECK_EQUAL( readAll( conflicts[0].ancestorFile ), "initial\n" );

    // The repository must not have been touched by the conflict listing.
    BOOST_CHECK_EQUAL( git_repository_state( repo ), GIT_REPOSITORY_STATE_NONE );

    // Refuse to apply resolutions against a moved head.
    BOOST_CHECK_EQUAL(
            static_cast<int>( common.CreateMergedCommit( wxT( "0000000000000000000000000000000000000000" ), theirsOid,
                                                         { { conflicts[0].path, conflicts[0].oursFile } }, error ) ),
            static_cast<int>( MergeCommitResult::FailedCleanly ) );

    // A background fetch can move FETCH_HEAD while the user resolves. The pinned commit id must
    // win over whatever FETCH_HEAD says by then.
    writeFetchHead( baseOid );

    // A save landing while the user resolves must be refused, not reverted by the final
    // checkout.
    wxString trackedFile = repoPath() + wxFileName::GetPathSeparator() + wxT( "file.txt" );
    {
        std::ofstream f( trackedFile.ToStdString(), std::ios::binary );
        f << "edited during resolution\n";
    }

    BOOST_CHECK_EQUAL( static_cast<int>( common.CreateMergedCommit(
                               headOid, theirsOid, { { conflicts[0].path, conflicts[0].oursFile } }, error ) ),
                       static_cast<int>( MergeCommitResult::FailedCleanly ) );

    {
        std::ofstream f( trackedFile.ToStdString(), std::ios::binary );
        f << "mine\n";
    }

    // A checkout failure, here injected with git's own index lock, must leave the branch where
    // it was. Recording the merge first would make a later pull report up to date while the
    // files still hold the old content.
    wxString indexLock = repoPath() + wxFileName::GetPathSeparator() + wxT( ".git" ) + wxFileName::GetPathSeparator()
                         + wxT( "index.lock" );
    {
        std::ofstream f( indexLock.ToStdString() );
    }

    {
        wxString      lockedMerged = tempBase() + wxFileName::GetPathSeparator() + wxT( "locked_merged.txt" );
        std::ofstream f( lockedMerged.ToStdString(), std::ios::binary );
        f << "merged\n";
        f.close();

        // The lock also blocks the rollback, so the working files are left part way. The caller
        // keeps its recovery copies on this result.
        BOOST_CHECK_EQUAL( static_cast<int>( common.CreateMergedCommit(
                                   headOid, theirsOid, { { conflicts[0].path, lockedMerged } }, error ) ),
                           static_cast<int>( MergeCommitResult::FailedPartially ) );

        git_oid headNow;
        BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &headNow, repo, "HEAD" ), GIT_OK );

        char headNowStr[GIT_OID_HEXSZ + 1] = { 0 };
        git_oid_tostr( headNowStr, sizeof( headNowStr ), &headNow );
        BOOST_CHECK_EQUAL( std::string( headNowStr ), headOid.ToStdString() );
    }

    wxRemoveFile( indexLock );

#ifndef __WINDOWS__
    // A failure after the checkout, injected here by making the branch ref unwritable, must
    // roll the working files back automatically. The operation required a clean tree, so the
    // rollback is lossless by construction. Directory permissions do not bind root.
    if( geteuid() != 0 )
    {
        // The ref update writes a lock file next to the ref and renames it into place, so the
        // way to make it fail is denying writes on the containing directory.
        wxString refsDir = repoPath() + wxFileName::GetPathSeparator() + wxT( ".git" ) + wxFileName::GetPathSeparator()
                           + wxT( "refs" ) + wxFileName::GetPathSeparator() + wxT( "heads" );

        BOOST_REQUIRE( wxFileName::DirExists( refsDir ) );

        // Put the permissions back however this ends, or the fixture cannot remove its directory.
        struct WRITABLE_AGAIN
        {
            wxString dir;
            ~WRITABLE_AGAIN() { chmod( dir.fn_str(), 0755 ); }
        } restore{ refsDir };

        chmod( refsDir.fn_str(), 0555 );

        wxString refMerged = tempBase() + wxFileName::GetPathSeparator() + wxT( "ref_merged.txt" );
        {
            std::ofstream f( refMerged.ToStdString() );
            f << "merged\n";
        }

        BOOST_CHECK_EQUAL( static_cast<int>( common.CreateMergedCommit( headOid, theirsOid,
                                                                        { { conflicts[0].path, refMerged } }, error ) ),
                           static_cast<int>( MergeCommitResult::FailedCleanly ) );

        // The branch did not move and the working file is back at the pre-merge content.
        git_oid headNow;
        BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &headNow, repo, "HEAD" ), GIT_OK );

        char headNowStr[GIT_OID_HEXSZ + 1] = { 0 };
        git_oid_tostr( headNowStr, sizeof( headNowStr ), &headNow );
        BOOST_CHECK_EQUAL( std::string( headNowStr ), headOid.ToStdString() );

        std::ifstream in( trackedFile.ToStdString(), std::ios::binary );
        std::string   rolled( ( std::istreambuf_iterator<char>( in ) ), std::istreambuf_iterator<char>() );
        BOOST_CHECK_EQUAL( rolled, "mine\n" );
    }
#endif

    // The branch did not move, so the half-written file is an ordinary local modification. Put it
    // back the way a user cleaning up would, so the retry starts from a clean tree.
    {
        std::ofstream f( trackedFile.ToStdString(), std::ios::binary );
        f << "mine\n";
    }

    // Resolve with fresh content and finish the merge.
    wxString mergedFile = tempBase() + wxFileName::GetPathSeparator() + wxT( "merged.txt" );
    {
        std::ofstream f( mergedFile.ToStdString(), std::ios::binary );
        f << "merged\n";
    }

    BOOST_REQUIRE_MESSAGE( common.CreateMergedCommit( headOid, theirsOid, { { conflicts[0].path, mergedFile } }, error )
                                   == MergeCommitResult::Success,
                           "CreateMergedCommit failed: " + error.summary.ToStdString() );

    // HEAD is now a merge commit of both sides, the working file carries the resolution, and no
    // merge state is left behind.
    git_oid newHead;
    BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &newHead, repo, "HEAD" ), GIT_OK );

    git_commit* mergeCommit = nullptr;
    BOOST_REQUIRE_EQUAL( git_commit_lookup( &mergeCommit, repo, &newHead ), GIT_OK );
    BOOST_CHECK_EQUAL( git_commit_parentcount( mergeCommit ), 2u );

    // Merged against the pinned remote commit, not the one FETCH_HEAD moved to meanwhile.
    const git_oid* secondParent = git_commit_parent_id( mergeCommit, 1 );
    BOOST_REQUIRE( secondParent );
    BOOST_CHECK( git_oid_equal( secondParent, &remoteCommitOid ) );
    git_commit_free( mergeCommit );

    BOOST_CHECK_EQUAL( readAll( repoPath() + wxFileName::GetPathSeparator() + wxT( "file.txt" ) ), "merged\n" );
    BOOST_CHECK_EQUAL( git_repository_state( repo ), GIT_REPOSITORY_STATE_NONE );

    git_repository_free( repo );
}


// The panel writes the .mine or .theirs recovery copy into the working tree before finishing
// the merge, so an untracked file must not count as a change made during resolution.
BOOST_AUTO_TEST_CASE( MergeConflictFinishesWithAnUntrackedFilePresent )
{
    BOOST_TEST_REQUIRE( ready() );

    git_repository* repo = openRepo();
    BOOST_TEST_REQUIRE( repo );

    git_reference* headRef = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_head( &headRef, repo ), GIT_OK );
    git_oid baseOid = *git_reference_target( headRef );
    git_reference_free( headRef );

    createCommit( repo, wxT( "file.txt" ), wxT( "mine\n" ), wxT( "Local change" ) );
    git_oid remoteCommitOid = createDetachedCommit( repo, baseOid, wxT( "file.txt" ), "remote\n", "Remote change" );
    writeFetchHead( remoteCommitOid );

    KIGIT_COMMON common( repo );

    wxString                          stageDir = tempBase() + wxFileName::GetPathSeparator() + wxT( "stages" );
    wxString                          headOid, theirsOid, conflictsError;
    KIGIT_ERROR_MESSAGE               error;
    std::vector<KIGIT_CONFLICT_SIDES> conflicts;

    BOOST_REQUIRE_MESSAGE( common.GetMergeConflicts( stageDir, headOid, theirsOid, conflicts, conflictsError ),
                           "GetMergeConflicts failed: " + conflictsError.ToStdString() );
    BOOST_REQUIRE_EQUAL( conflicts.size(), 1u );

    // The recovery copy the panel leaves beside the file.
    {
        wxString      copy = repoPath() + wxFileName::GetPathSeparator() + wxT( "file.txt.mine" );
        std::ofstream f( copy.ToStdString() );
        f << "mine\n";
    }

    BOOST_CHECK_MESSAGE(
            common.CreateMergedCommit( headOid, theirsOid, { { conflicts[0].path, conflicts[0].theirsFile } }, error )
                    == MergeCommitResult::Success,
            "refused with an untracked file present: " + error.summary.ToStdString() );

    git_repository_free( repo );
}


// A rename on the remote arrives as two conflict entries, the old name with no remote side and the
// new name with no base. Taking theirs on both must leave the content on the new name only.
BOOST_AUTO_TEST_CASE( MergeConflictRenameTakeTheirsLandsOnNewPath )
{
    BOOST_TEST_REQUIRE( ready() );

    git_repository* repo = openRepo();
    BOOST_TEST_REQUIRE( repo );

    wxString base;

    for( int i = 1; i <= 12; ++i )
        base << wxString::Format( wxT( "line %d\n" ), i );

    git_oid baseOid = createCommit( repo, wxT( "file.txt" ), base, wxT( "Base" ) );

    wxString mine = base;
    mine.Replace( wxT( "line 6\n" ), wxT( "line 6 mine\n" ) );
    createCommit( repo, wxT( "file.txt" ), mine, wxT( "Local change" ) );

    // Theirs: file.txt removed, renamed.txt added with the same text except line 6.
    wxString theirs = base;
    theirs.Replace( wxT( "line 6\n" ), wxT( "line 6 theirs\n" ) );

    git_oid theirsOidRaw;
    {
        git_commit* baseCommit = nullptr;
        BOOST_REQUIRE_EQUAL( git_commit_lookup( &baseCommit, repo, &baseOid ), GIT_OK );
        git_tree* baseTree = nullptr;
        BOOST_REQUIRE_EQUAL( git_commit_tree( &baseTree, baseCommit ), GIT_OK );

        git_treebuilder* tb = nullptr;
        BOOST_REQUIRE_EQUAL( git_treebuilder_new( &tb, repo, baseTree ), GIT_OK );
        BOOST_REQUIRE_EQUAL( git_treebuilder_remove( tb, "file.txt" ), GIT_OK );

        std::string content = theirs.utf8_string();
        git_oid     blob;
        BOOST_REQUIRE_EQUAL( git_blob_create_from_buffer( &blob, repo, content.data(), content.size() ), GIT_OK );
        BOOST_REQUIRE_EQUAL( git_treebuilder_insert( nullptr, tb, "renamed.txt", &blob, GIT_FILEMODE_BLOB ), GIT_OK );

        git_oid treeOid;
        BOOST_REQUIRE_EQUAL( git_treebuilder_write( &treeOid, tb ), GIT_OK );
        git_tree* tree = nullptr;
        BOOST_REQUIRE_EQUAL( git_tree_lookup( &tree, repo, &treeOid ), GIT_OK );

        git_signature* sig = nullptr;
        BOOST_REQUIRE_EQUAL( git_signature_now( &sig, "QA", "qa@example.com" ), GIT_OK );
        const git_commit* parents[1] = { baseCommit };
        BOOST_REQUIRE_EQUAL( git_commit_create( &theirsOidRaw, repo, nullptr, sig, sig, nullptr, "Rename and edit",
                                                tree, 1, parents ),
                             GIT_OK );
        git_signature_free( sig );
        git_tree_free( tree );
        git_treebuilder_free( tb );
        git_tree_free( baseTree );
        git_commit_free( baseCommit );
    }

    writeFetchHead( theirsOidRaw );

    KIGIT_COMMON                      common( repo );
    wxString                          stageDir = tempBase() + wxFileName::GetPathSeparator() + wxT( "stages" );
    wxString                          headOid, theirsOid, conflictsError;
    KIGIT_ERROR_MESSAGE               error;
    std::vector<KIGIT_CONFLICT_SIDES> conflicts;

    BOOST_REQUIRE_MESSAGE( common.GetMergeConflicts( stageDir, headOid, theirsOid, conflicts, conflictsError ),
                           conflictsError );


    BOOST_REQUIRE_EQUAL( conflicts.size(), 2u );

    std::vector<std::pair<wxString, wxString>> resolved;

    for( const KIGIT_CONFLICT_SIDES& c : conflicts )
        resolved.emplace_back( c.path, c.hasTheirs ? c.theirsFile : wxString() );

    BOOST_REQUIRE_MESSAGE( common.CreateMergedCommit( headOid, theirsOid, resolved, error )
                                   == MergeCommitResult::Success,
                           error.summary );

    wxString workdir = wxString::FromUTF8( git_repository_workdir( repo ) );
    bool     renamedExists = wxFileExists( workdir + wxT( "renamed.txt" ) );
    bool     oldExists = wxFileExists( workdir + wxT( "file.txt" ) );

    BOOST_CHECK_MESSAGE( renamedExists, "theirs' content must land on the renamed path" );
    BOOST_CHECK_MESSAGE( !oldExists, "the old path must not survive a take-theirs of a rename" );

    git_repository_free( repo );
}


// Rebuilding from the parent would drop every file the amended commit held but the list omits.
BOOST_AUTO_TEST_CASE( Amend_KeepsFilesAlreadyInCommit )
{
    BOOST_TEST_REQUIRE( ready() );

    git_repository* repo = openRepo();
    BOOST_TEST_REQUIRE( repo );

    // The commit to amend carries second.txt, which the amend below does not mention.
    createCommit( repo, wxT( "second.txt" ), wxT( "two\n" ), wxT( "Add second" ) );

    {
        wxString      filePath = repoPath() + wxFileName::GetPathSeparator() + wxT( "file.txt" );
        std::ofstream f( filePath.ToStdString() );
        f << "amended\n";
    }

    GIT_COMMIT_HANDLER handler( repo );
    BOOST_REQUIRE( handler.PerformAmend( { wxT( "file.txt" ) }, wxT( "Amended" ), TEST_AUTHOR_NAME, TEST_AUTHOR_EMAIL )
                   == CommitResult::Success );

    git_oid headOid;
    BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &headOid, repo, "HEAD" ), GIT_OK );

    git_commit* head = nullptr;
    BOOST_REQUIRE_EQUAL( git_commit_lookup( &head, repo, &headOid ), GIT_OK );

    git_tree* tree = nullptr;
    BOOST_REQUIRE_EQUAL( git_commit_tree( &tree, head ), GIT_OK );

    BOOST_CHECK_MESSAGE( git_tree_entry_byname( tree, "second.txt" ) != nullptr,
                         "a file already in the amended commit must survive an amend that does not list it" );
    BOOST_CHECK( git_tree_entry_byname( tree, "file.txt" ) != nullptr );

    git_tree_free( tree );
    git_commit_free( head );
    git_repository_free( repo );
}


// A switch that would overwrite local changes must report a conflict and change nothing.
// A remote-tracking branch cannot be stood on. Checking one out could only attach HEAD to a
// local branch of the same name, which may be at another commit, so it is refused outright and
// nothing on disk is touched.
BOOST_AUTO_TEST_CASE( SwitchToBranch_RefusesARemoteBranch )
{
    BOOST_TEST_REQUIRE( ready() );

    git_repository* repo = openRepo();
    BOOST_TEST_REQUIRE( repo );

    git_oid localTip = createCommit( repo, wxT( "file.txt" ), wxT( "local\n" ), wxT( "local" ) );

    git_commit* tipCommit = nullptr;
    BOOST_REQUIRE_EQUAL( git_commit_lookup( &tipCommit, repo, &localTip ), GIT_OK );
    KIGIT::GitCommitPtr tipPtr( tipCommit );

    git_reference* localBranch = nullptr;
    BOOST_REQUIRE_EQUAL( git_branch_create( &localBranch, repo, "feature", tipCommit, 0 ), GIT_OK );
    git_reference_free( localBranch );

    // origin/feature further along than the local branch of the same name, as a fetch leaves it.
    git_oid        remoteTip = createDetachedCommit( repo, localTip, wxT( "file.txt" ), "remote\n", "remote" );
    git_reference* tracking = nullptr;
    BOOST_REQUIRE_EQUAL( git_reference_create( &tracking, repo, "refs/remotes/origin/feature", &remoteTip, 1, nullptr ),
                         GIT_OK );
    git_reference_free( tracking );

    KIGIT_COMMON       common( repo );
    GIT_BRANCH_HANDLER handler( &common );

    BOOST_CHECK( handler.SwitchToBranch( wxT( "origin/feature" ) ) == BranchResult::Error );
    const wxString error = handler.GetErrorString();

    BOOST_CHECK_MESSAGE( error.Contains( wxS( "remote" ) ),
                         "the refusal should say the branch is on the remote, got: " + error.ToStdString() );

    // Nothing moved and nothing was written.
    git_oid headNow;
    BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &headNow, repo, "HEAD" ), GIT_OK );
    BOOST_CHECK( git_oid_equal( &headNow, &localTip ) );

    std::ifstream f( ( repoPath() + wxFileName::GetPathSeparator() + wxT( "file.txt" ) ).ToStdString() );
    std::string   content( ( std::istreambuf_iterator<char>( f ) ), std::istreambuf_iterator<char>() );
    BOOST_CHECK_EQUAL( content, "local\n" );

    git_repository_free( repo );
}


BOOST_AUTO_TEST_CASE( SwitchToBranch_ReportsConflictOverLocalChanges )
{
    BOOST_TEST_REQUIRE( ready() );

    git_repository* repo = openRepo();
    BOOST_TEST_REQUIRE( repo );

    git_oid headOid;
    BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &headOid, repo, "HEAD" ), GIT_OK );

    git_commit* headCommit = nullptr;
    BOOST_REQUIRE_EQUAL( git_commit_lookup( &headCommit, repo, &headOid ), GIT_OK );

    git_reference* oldBranch = nullptr;
    BOOST_REQUIRE_EQUAL( git_branch_create( &oldBranch, repo, "old", headCommit, 0 ), GIT_OK );
    git_reference_free( oldBranch );
    git_commit_free( headCommit );

    git_oid advancedOid = createCommit( repo, wxT( "file.txt" ), wxT( "master2\n" ), wxT( "Advance master" ) );

    wxString workDir = wxString::FromUTF8( git_repository_workdir( repo ) );

    {
        std::ofstream f( ( workDir + wxT( "file.txt" ) ).ToStdString(), std::ios::binary );
        f << "local edit\n";
    }

    KIGIT_COMMON       common( repo );
    GIT_BRANCH_HANDLER handler( &common );

    BOOST_CHECK( handler.SwitchToBranch( wxT( "old" ) ) == BranchResult::Conflict );

    git_oid afterOid;
    BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &afterOid, repo, "HEAD" ), GIT_OK );
    BOOST_CHECK_EQUAL( git_oid_cmp( &afterOid, &advancedOid ), 0 );

    std::ifstream in( ( workDir + wxT( "file.txt" ) ).ToStdString(), std::ios::binary );
    std::string   content( ( std::istreambuf_iterator<char>( in ) ), std::istreambuf_iterator<char>() );
    BOOST_CHECK_EQUAL( content, "local edit\n" );

    git_repository_free( repo );
}


// A never-committed file the target branch carries must stop the switch too.
BOOST_AUTO_TEST_CASE( SwitchToBranch_ReportsConflictOverUntrackedCollision )
{
    BOOST_TEST_REQUIRE( ready() );

    git_repository* repo = openRepo();
    BOOST_TEST_REQUIRE( repo );

    git_oid headOid;
    BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &headOid, repo, "HEAD" ), GIT_OK );

    git_commit* headCommit = nullptr;
    BOOST_REQUIRE_EQUAL( git_commit_lookup( &headCommit, repo, &headOid ), GIT_OK );

    git_reference* plainBranch = nullptr;
    BOOST_REQUIRE_EQUAL( git_branch_create( &plainBranch, repo, "plain", headCommit, 0 ), GIT_OK );
    git_reference_free( plainBranch );
    git_commit_free( headCommit );

    createCommit( repo, wxT( "extra.txt" ), wxT( "committed\n" ), wxT( "Add extra" ) );

    KIGIT_COMMON       common( repo );
    GIT_BRANCH_HANDLER handler( &common );

    BOOST_REQUIRE( handler.SwitchToBranch( wxT( "plain" ) ) == BranchResult::Success );

    wxString workDir = wxString::FromUTF8( git_repository_workdir( repo ) );

    {
        std::ofstream f( ( workDir + wxT( "extra.txt" ) ).ToStdString(), std::ios::binary );
        f << "local\n";
    }

    BOOST_CHECK( handler.SwitchToBranch( wxT( "master" ) ) == BranchResult::Conflict );

    std::ifstream in( ( workDir + wxT( "extra.txt" ) ).ToStdString(), std::ios::binary );
    std::string   content( ( std::istreambuf_iterator<char>( in ) ), std::istreambuf_iterator<char>() );
    BOOST_CHECK_EQUAL( content, "local\n" );

    git_repository_free( repo );
}


// A file deleted from the working tree is offered in the checklist, so committing it must
// record the deletion instead of failing to add the missing path.
BOOST_AUTO_TEST_CASE( Commit_RecordsDeletedFile )
{
    BOOST_TEST_REQUIRE( ready() );

    git_repository* repo = openRepo();
    BOOST_TEST_REQUIRE( repo );

    createCommit( repo, wxT( "doomed.txt" ), wxT( "doomed\n" ), wxT( "Add doomed" ) );

    wxString workDir = wxString::FromUTF8( git_repository_workdir( repo ) );
    BOOST_TEST_REQUIRE( wxRemoveFile( workDir + wxT( "doomed.txt" ) ) );

    GIT_COMMIT_HANDLER handler( repo );
    CommitResult result = handler.PerformCommit( { wxT( "doomed.txt" ) }, wxT( "Remove doomed" ), TEST_AUTHOR_NAME,
                                                 TEST_AUTHOR_EMAIL );

    BOOST_REQUIRE_MESSAGE( result == CommitResult::Success, handler.GetErrorString() );

    git_oid headOid;
    BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &headOid, repo, "HEAD" ), GIT_OK );

    git_commit* head = nullptr;
    BOOST_REQUIRE_EQUAL( git_commit_lookup( &head, repo, &headOid ), GIT_OK );

    git_tree* tree = nullptr;
    BOOST_REQUIRE_EQUAL( git_commit_tree( &tree, head ), GIT_OK );

    BOOST_CHECK( git_tree_entry_byname( tree, "doomed.txt" ) == nullptr );
    BOOST_CHECK( git_tree_entry_byname( tree, "file.txt" ) != nullptr );

    git_tree_free( tree );
    git_commit_free( head );
    git_repository_free( repo );
}


// The selected entries must also advance to the committed content, or they linger afterwards as
// phantom staged changes.
BOOST_AUTO_TEST_CASE( Commit_PreservesIndexEntriesOfUnselectedFiles )
{
    BOOST_TEST_REQUIRE( ready() );

    git_repository* repo = openRepo();
    BOOST_TEST_REQUIRE( repo );

    createCommit( repo, wxT( "board.txt" ), wxT( "board v1\n" ), wxT( "Add board" ) );
    createCommit( repo, wxT( "doomed.txt" ), wxT( "doomed\n" ), wxT( "Add doomed" ) );

    wxString workDir = wxString::FromUTF8( git_repository_workdir( repo ) );

    auto writeAll = [&]( const wxString& aName, const std::string& aContent )
    {
        std::ofstream f( ( workDir + aName ).ToStdString(), std::ios::binary );
        f << aContent;
    };

    auto readAll = [&]( const wxString& aName )
    {
        std::ifstream in( ( workDir + aName ).ToStdString(), std::ios::binary );
        return std::string( ( std::istreambuf_iterator<char>( in ) ), std::istreambuf_iterator<char>() );
    };

    // Staged content of a path, or a marker for "no entry staged".
    auto stagedContent = [&]( const char* aPath )
    {
        std::string result = "<none>";
        git_index*  index = nullptr;
        BOOST_REQUIRE_EQUAL( git_repository_index( &index, repo ), GIT_OK );
        BOOST_REQUIRE_EQUAL( git_index_read( index, 1 ), GIT_OK );

        if( const git_index_entry* entry = git_index_get_bypath( index, aPath, 0 ) )
        {
            git_blob* blob = nullptr;
            BOOST_REQUIRE_EQUAL( git_blob_lookup( &blob, repo, &entry->id ), GIT_OK );
            result.assign( static_cast<const char*>( git_blob_rawcontent( blob ) ), git_blob_rawsize( blob ) );
            git_blob_free( blob );
        }

        git_index_free( index );
        return result;
    };

    auto committedContent = [&]( const char* aPath )
    {
        std::string result = "<none>";
        git_oid     headOid;
        BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &headOid, repo, "HEAD" ), GIT_OK );

        git_commit* head = nullptr;
        BOOST_REQUIRE_EQUAL( git_commit_lookup( &head, repo, &headOid ), GIT_OK );

        git_tree* tree = nullptr;
        BOOST_REQUIRE_EQUAL( git_commit_tree( &tree, head ), GIT_OK );

        git_tree_entry* entry = nullptr;

        if( git_tree_entry_bypath( &entry, tree, aPath ) == GIT_OK )
        {
            git_blob* blob = nullptr;
            BOOST_REQUIRE_EQUAL( git_blob_lookup( &blob, repo, git_tree_entry_id( entry ) ), GIT_OK );
            result.assign( static_cast<const char*>( git_blob_rawcontent( blob ) ), git_blob_rawsize( blob ) );
            git_blob_free( blob );
            git_tree_entry_free( entry );
        }

        git_tree_free( tree );
        git_commit_free( head );
        return result;
    };

    // Three kinds of staging the user did outside KiCad and did not select: a modification, a
    // brand new file, and a deletion.
    writeAll( wxT( "notes.txt" ), "staged A\n" );
    writeAll( wxT( "board.txt" ), "board staged\n" );

    git_index* index = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_index( &index, repo ), GIT_OK );
    BOOST_REQUIRE_EQUAL( git_index_add_bypath( index, "notes.txt" ), GIT_OK );
    BOOST_REQUIRE_EQUAL( git_index_add_bypath( index, "board.txt" ), GIT_OK );
    BOOST_REQUIRE_EQUAL( git_index_remove_bypath( index, "doomed.txt" ), GIT_OK );
    BOOST_REQUIRE_EQUAL( git_index_write( index ), GIT_OK );
    git_index_free( index );

    // The staged version and the working version differ, so a lost index entry cannot be
    // recovered from the working file.
    writeAll( wxT( "notes.txt" ), "working B\n" );
    writeAll( wxT( "board.txt" ), "board working\n" );

    // The selected file is a fourth path, untouched by any of the staging above.
    writeAll( wxT( "file.txt" ), "selected\n" );

    GIT_COMMIT_HANDLER handler( repo );
    CommitResult       result =
            handler.PerformCommit( { wxT( "file.txt" ) }, wxT( "Only file.txt" ), TEST_AUTHOR_NAME, TEST_AUTHOR_EMAIL );
    BOOST_REQUIRE_MESSAGE( result == CommitResult::Success, handler.GetErrorString() );

    // The commit holds HEAD plus the selected file only.
    BOOST_CHECK_EQUAL( committedContent( "file.txt" ), "selected\n" );
    BOOST_CHECK_EQUAL( committedContent( "board.txt" ), "board v1\n" );
    BOOST_CHECK_EQUAL( committedContent( "notes.txt" ), "<none>" );
    BOOST_CHECK_EQUAL( committedContent( "doomed.txt" ), "doomed\n" );

    // Every unselected staged decision survives, and the working files are untouched.
    BOOST_CHECK_EQUAL( stagedContent( "notes.txt" ), "staged A\n" );
    BOOST_CHECK_EQUAL( stagedContent( "board.txt" ), "board staged\n" );
    BOOST_CHECK_EQUAL( stagedContent( "doomed.txt" ), "<none>" );
    BOOST_CHECK_EQUAL( readAll( wxT( "notes.txt" ) ), "working B\n" );
    BOOST_CHECK_EQUAL( readAll( wxT( "board.txt" ) ), "board working\n" );

    // The selected path matches what was committed, so it is not left looking staged.
    BOOST_CHECK_EQUAL( stagedContent( "file.txt" ), "selected\n" );

    // A selected deletion behaves the same way, and still leaves the others alone.
    BOOST_TEST_REQUIRE( wxRemoveFile( workDir + wxT( "file.txt" ) ) );

    result = handler.PerformCommit( { wxT( "file.txt" ) }, wxT( "Remove file.txt" ), TEST_AUTHOR_NAME,
                                    TEST_AUTHOR_EMAIL );
    BOOST_REQUIRE_MESSAGE( result == CommitResult::Success, handler.GetErrorString() );

    BOOST_CHECK_EQUAL( committedContent( "file.txt" ), "<none>" );
    BOOST_CHECK_EQUAL( stagedContent( "file.txt" ), "<none>" );
    BOOST_CHECK_EQUAL( stagedContent( "notes.txt" ), "staged A\n" );
    BOOST_CHECK_EQUAL( stagedContent( "board.txt" ), "board staged\n" );
    BOOST_CHECK_EQUAL( stagedContent( "doomed.txt" ), "<none>" );

    // A message-only amend keeps the tree and must not disturb the staged entries either.
    result = handler.PerformAmend( {}, wxT( "Reworded" ), TEST_AUTHOR_NAME, TEST_AUTHOR_EMAIL );
    BOOST_REQUIRE_MESSAGE( result == CommitResult::Success, handler.GetErrorString() );

    BOOST_CHECK_EQUAL( stagedContent( "notes.txt" ), "staged A\n" );
    BOOST_CHECK_EQUAL( stagedContent( "board.txt" ), "board staged\n" );
    BOOST_CHECK_EQUAL( stagedContent( "doomed.txt" ), "<none>" );
    BOOST_CHECK_EQUAL( committedContent( "board.txt" ), "board v1\n" );

    // A failed commit must not cost the user any staged content either.
    writeAll( wxT( "late.txt" ), "late\n" );
    BOOST_CHECK( handler.PerformCommit( { wxT( "late.txt" ) }, wxT( "Bad author" ), wxEmptyString, wxEmptyString )
                 == CommitResult::Error );

    BOOST_CHECK_EQUAL( stagedContent( "notes.txt" ), "staged A\n" );
    BOOST_CHECK_EQUAL( stagedContent( "board.txt" ), "board staged\n" );
    BOOST_CHECK_EQUAL( stagedContent( "doomed.txt" ), "<none>" );

    git_repository_free( repo );
}


// libgit2 wants UTF-8 paths. A locale conversion would mangle or reject non-ASCII names, so
// check that they survive an add, a modify, an amend and a delete with byte-exact identity.
BOOST_AUTO_TEST_CASE( Commit_HandlesNonAsciiPaths )
{
    BOOST_TEST_REQUIRE( ready() );

    git_repository* repo = openRepo();
    BOOST_TEST_REQUIRE( repo );

    wxString workDir = wxString::FromUTF8( git_repository_workdir( repo ) );

    const wxString    umlaut = wxString::FromUTF8( "Gr\xc3\xb6\xc3\x9f"
                                                   "e.kicad_sch" );
    const wxString    cyrillic = wxString::FromUTF8( "\xd0\xbf\xd0\xbb\xd0\xb0\xd1\x82\xd0\xb0.txt" );
    const std::string umlautUtf8 = umlaut.utf8_string();
    const std::string cyrillicUtf8 = cyrillic.utf8_string();

    auto writeAll = [&]( const wxString& aName, const std::string& aContent )
    {
        KI_TEST::WriteGitFile( repoPath(), aName, aContent );
    };

    auto inHeadTree = [&]( const std::string& aPath )
    {
        git_oid headOid;
        BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &headOid, repo, "HEAD" ), GIT_OK );

        git_commit* head = nullptr;
        BOOST_REQUIRE_EQUAL( git_commit_lookup( &head, repo, &headOid ), GIT_OK );

        git_tree* tree = nullptr;
        BOOST_REQUIRE_EQUAL( git_commit_tree( &tree, head ), GIT_OK );

        git_tree_entry* entry = nullptr;
        bool            found = git_tree_entry_bypath( &entry, tree, aPath.c_str() ) == GIT_OK;

        if( found )
            git_tree_entry_free( entry );

        git_tree_free( tree );
        git_commit_free( head );
        return found;
    };

    writeAll( umlaut, "one\n" );
    writeAll( cyrillic, "one\n" );

    GIT_COMMIT_HANDLER handler( repo );
    CommitResult       result =
            handler.PerformCommit( { umlaut, cyrillic }, wxT( "Add" ), TEST_AUTHOR_NAME, TEST_AUTHOR_EMAIL );
    BOOST_REQUIRE_MESSAGE( result == CommitResult::Success, handler.GetErrorString() );

    // The recorded path must be the same bytes as the file on disk, not a locale transcoding.
    BOOST_CHECK( inHeadTree( umlautUtf8 ) );
    BOOST_CHECK( inHeadTree( cyrillicUtf8 ) );

    writeAll( umlaut, "two\n" );
    result = handler.PerformCommit( { umlaut }, wxT( "Modify" ), TEST_AUTHOR_NAME, TEST_AUTHOR_EMAIL );
    BOOST_REQUIRE_MESSAGE( result == CommitResult::Success, handler.GetErrorString() );
    BOOST_CHECK( inHeadTree( umlautUtf8 ) );

    writeAll( umlaut, "three\n" );
    result = handler.PerformAmend( { umlaut }, wxT( "Amended" ), TEST_AUTHOR_NAME, TEST_AUTHOR_EMAIL );
    BOOST_REQUIRE_MESSAGE( result == CommitResult::Success, handler.GetErrorString() );
    BOOST_CHECK( inHeadTree( umlautUtf8 ) );
    BOOST_CHECK( inHeadTree( cyrillicUtf8 ) );

    BOOST_TEST_REQUIRE( wxRemoveFile( workDir + umlaut ) );
    result = handler.PerformCommit( { umlaut }, wxT( "Delete" ), TEST_AUTHOR_NAME, TEST_AUTHOR_EMAIL );
    BOOST_REQUIRE_MESSAGE( result == CommitResult::Success, handler.GetErrorString() );

    BOOST_CHECK( !inHeadTree( umlautUtf8 ) );
    BOOST_CHECK( inHeadTree( cyrillicUtf8 ) );

    git_repository_free( repo );
}


// While another process holds .git/index.lock, KiCad's own index write must fail cleanly and
// leave that process's staging on disk untouched.
BOOST_AUTO_TEST_CASE( Commit_DoesNotDamageIndexWhileAnotherProcessHoldsTheLock )
{
    BOOST_TEST_REQUIRE( ready() );

    git_repository* repo = openRepo();
    BOOST_TEST_REQUIRE( repo );

    wxString workDir = wxString::FromUTF8( git_repository_workdir( repo ) );

    {
        std::ofstream f( ( workDir + wxT( "notes.txt" ) ).ToStdString(), std::ios::binary );
        f << "staged elsewhere\n";
    }

    git_repository* foreignRepo = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_open( &foreignRepo, repoPath().ToUTF8().data() ), GIT_OK );

    git_index* foreign = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_index( &foreign, foreignRepo ), GIT_OK );
    BOOST_REQUIRE_EQUAL( git_index_add_bypath( foreign, "notes.txt" ), GIT_OK );
    BOOST_REQUIRE_EQUAL( git_index_write( foreign ), GIT_OK );
    git_index_free( foreign );
    git_repository_free( foreignRepo );

    {
        std::ofstream f( ( workDir + wxT( "file.txt" ) ).ToStdString(), std::ios::binary );
        f << "selected\n";
    }

    // Stand in for another git process that is part way through its own index write.
    wxString indexLock = repoPath() + wxFileName::GetPathSeparator() + wxT( ".git" ) + wxFileName::GetPathSeparator()
                         + wxT( "index.lock" );
    {
        std::ofstream f( indexLock.ToStdString() );
        f << "held\n";
    }

    GIT_COMMIT_HANDLER handler( repo );
    CommitResult       result =
            handler.PerformCommit( { wxT( "file.txt" ) }, wxT( "Only file.txt" ), TEST_AUTHOR_NAME, TEST_AUTHOR_EMAIL );

    // The commit itself does not need the index lock, so it exists. Only the staged file list
    // could not be rewritten, which is a different outcome from a failed commit.
    BOOST_CHECK( result == CommitResult::SuccessIndexStale );
    BOOST_CHECK( !handler.GetErrorString().IsEmpty() );

    BOOST_TEST_REQUIRE( wxRemoveFile( indexLock ) );

    git_oid committed;
    BOOST_CHECK_MESSAGE( git_reference_name_to_id( &committed, repo, "HEAD" ) == GIT_OK,
                         "the commit must exist even though the index could not be rewritten" );

    // The other process's staging is still exactly as it left it.
    git_index* after = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_index( &after, repo ), GIT_OK );
    BOOST_REQUIRE_EQUAL( git_index_read( after, 1 ), GIT_OK );

    const git_index_entry* notes = git_index_get_bypath( after, "notes.txt", 0 );
    BOOST_REQUIRE_MESSAGE( notes, "the locked-out write damaged another process's staging" );

    git_blob* blob = nullptr;
    BOOST_REQUIRE_EQUAL( git_blob_lookup( &blob, repo, &notes->id ), GIT_OK );
    std::string content( static_cast<const char*>( git_blob_rawcontent( blob ) ), git_blob_rawsize( blob ) );
    BOOST_CHECK_EQUAL( content, "staged elsewhere\n" );

    git_blob_free( blob );
    git_index_free( after );
    git_repository_free( repo );
}


// Amend needs the same protection as Commit. Staging done outside KiCad must survive it.
BOOST_AUTO_TEST_CASE( Amend_KeepsStagingWrittenByAnotherProcess )
{
    BOOST_TEST_REQUIRE( ready() );

    git_repository* repo = openRepo();
    BOOST_TEST_REQUIRE( repo );

    wxString workDir = wxString::FromUTF8( git_repository_workdir( repo ) );

    git_index* cached = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_index( &cached, repo ), GIT_OK );
    KIGIT::GitIndexPtr cachedPtr( cached );

    {
        std::ofstream f( ( workDir + wxT( "notes.txt" ) ).ToStdString(), std::ios::binary );
        f << "staged elsewhere\n";
    }

    // A second handle on the same directory stands in for a terminal git process.
    git_repository* foreignRepo = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_open( &foreignRepo, repoPath().ToUTF8().data() ), GIT_OK );

    git_index* foreign = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_index( &foreign, foreignRepo ), GIT_OK );
    BOOST_REQUIRE_EQUAL( git_index_add_bypath( foreign, "notes.txt" ), GIT_OK );
    BOOST_REQUIRE_EQUAL( git_index_write( foreign ), GIT_OK );

    const git_index_entry* stagedBefore = git_index_get_bypath( foreign, "notes.txt", 0 );
    BOOST_REQUIRE( stagedBefore );
    git_oid stagedBlob = stagedBefore->id;

    git_index_free( foreign );
    git_repository_free( foreignRepo );

    {
        std::ofstream f( ( workDir + wxT( "file.txt" ) ).ToStdString(), std::ios::binary );
        f << "amended\n";
    }

    GIT_COMMIT_HANDLER handler( repo );
    CommitResult       result = handler.PerformAmend( { wxT( "file.txt" ) }, wxT( "Amended message" ), TEST_AUTHOR_NAME,
                                                      TEST_AUTHOR_EMAIL );
    BOOST_REQUIRE_MESSAGE( result == CommitResult::Success, handler.GetErrorString() );

    git_repository_free( repo );

    // Reopen so nothing is answered from a cached index.
    git_repository* fresh = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_open( &fresh, repoPath().ToUTF8().data() ), GIT_OK );

    git_index* after = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_index( &after, fresh ), GIT_OK );

    const git_index_entry* notes = git_index_get_bypath( after, "notes.txt", 0 );
    BOOST_REQUIRE_MESSAGE( notes, "the externally staged file must still be staged after an amend" );
    BOOST_CHECK_MESSAGE( git_oid_equal( &notes->id, &stagedBlob ),
                         "the externally staged blob must be the exact one the other process wrote" );

    // The amend must not have pulled notes.txt into the commit.
    git_oid headOid;
    BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &headOid, fresh, "HEAD" ), GIT_OK );

    git_commit* headCommit = nullptr;
    BOOST_REQUIRE_EQUAL( git_commit_lookup( &headCommit, fresh, &headOid ), GIT_OK );

    git_tree* headTree = nullptr;
    BOOST_REQUIRE_EQUAL( git_commit_tree( &headTree, headCommit ), GIT_OK );

    git_tree_entry* leaked = nullptr;
    BOOST_CHECK_MESSAGE( git_tree_entry_bypath( &leaked, headTree, "notes.txt" ) != GIT_OK,
                         "an unselected staged file must not ride into the amended commit" );
    git_tree_entry_free( leaked );

    git_tree_free( headTree );
    git_commit_free( headCommit );
    git_index_free( after );
    git_repository_free( fresh );
}


BOOST_AUTO_TEST_CASE( Commit_KeepsStagingWrittenByAnotherProcess )
{
    BOOST_TEST_REQUIRE( ready() );

    git_repository* repo = openRepo();
    BOOST_TEST_REQUIRE( repo );

    wxString workDir = wxString::FromUTF8( git_repository_workdir( repo ) );

    // Make the repository handle cache an index that knows nothing about the staging below.
    git_index* cached = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_index( &cached, repo ), GIT_OK );
    KIGIT::GitIndexPtr cachedPtr( cached );
    BOOST_REQUIRE_EQUAL( git_index_entrycount( cached ), 1 );

    {
        std::ofstream f( ( workDir + wxT( "notes.txt" ) ).ToStdString(), std::ios::binary );
        f << "staged elsewhere\n";
    }

    // A second repository handle on the same directory stands in for the terminal.
    git_repository* foreignRepo = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_open( &foreignRepo, repoPath().ToUTF8().data() ), GIT_OK );

    git_index* foreign = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_index( &foreign, foreignRepo ), GIT_OK );
    BOOST_REQUIRE_EQUAL( git_index_add_bypath( foreign, "notes.txt" ), GIT_OK );
    BOOST_REQUIRE_EQUAL( git_index_write( foreign ), GIT_OK );
    git_index_free( foreign );
    git_repository_free( foreignRepo );

    {
        std::ofstream f( ( workDir + wxT( "file.txt" ) ).ToStdString(), std::ios::binary );
        f << "selected\n";
    }

    GIT_COMMIT_HANDLER handler( repo );
    CommitResult       result =
            handler.PerformCommit( { wxT( "file.txt" ) }, wxT( "Only file.txt" ), TEST_AUTHOR_NAME, TEST_AUTHOR_EMAIL );
    BOOST_REQUIRE_MESSAGE( result == CommitResult::Success, handler.GetErrorString() );

    git_index* after = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_index( &after, repo ), GIT_OK );
    BOOST_REQUIRE_EQUAL( git_index_read( after, 1 ), GIT_OK );

    const git_index_entry* notes = git_index_get_bypath( after, "notes.txt", 0 );
    BOOST_REQUIRE_MESSAGE( notes, "staging written by another process was lost" );

    git_blob* blob = nullptr;
    BOOST_REQUIRE_EQUAL( git_blob_lookup( &blob, repo, &notes->id ), GIT_OK );
    std::string content( static_cast<const char*>( git_blob_rawcontent( blob ) ), git_blob_rawsize( blob ) );
    BOOST_CHECK_EQUAL( content, "staged elsewhere\n" );

    git_blob_free( blob );
    git_index_free( after );
    git_repository_free( repo );
}


// The panel promises to commit exactly the checked files. Content staged outside KiCad for
// an unchecked file must not ride into the commit.
BOOST_AUTO_TEST_CASE( Commit_IgnoresContentStagedElsewhere )
{
    BOOST_TEST_REQUIRE( ready() );

    git_repository* repo = openRepo();
    BOOST_TEST_REQUIRE( repo );

    createCommit( repo, wxT( "sneaky.txt" ), wxT( "clean\n" ), wxT( "Add sneaky" ) );

    wxString workDir = wxString::FromUTF8( git_repository_workdir( repo ) );

    // Stage a change to sneaky.txt the way a command line git add would, then change the
    // checked file too.
    {
        std::ofstream f( ( workDir + wxT( "sneaky.txt" ) ).ToStdString(), std::ios::binary );
        f << "staged elsewhere\n";
    }

    git_index* index = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_index( &index, repo ), GIT_OK );
    BOOST_REQUIRE_EQUAL( git_index_add_bypath( index, "sneaky.txt" ), GIT_OK );
    BOOST_REQUIRE_EQUAL( git_index_write( index ), GIT_OK );
    git_index_free( index );

    {
        std::ofstream f( ( workDir + wxT( "file.txt" ) ).ToStdString(), std::ios::binary );
        f << "wanted\n";
    }

    GIT_COMMIT_HANDLER handler( repo );
    CommitResult       result =
            handler.PerformCommit( { wxT( "file.txt" ) }, wxT( "Only file.txt" ), TEST_AUTHOR_NAME, TEST_AUTHOR_EMAIL );

    BOOST_REQUIRE_MESSAGE( result == CommitResult::Success, handler.GetErrorString() );

    git_oid headOid;
    BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &headOid, repo, "HEAD" ), GIT_OK );

    git_commit* head = nullptr;
    BOOST_REQUIRE_EQUAL( git_commit_lookup( &head, repo, &headOid ), GIT_OK );

    git_tree* tree = nullptr;
    BOOST_REQUIRE_EQUAL( git_commit_tree( &tree, head ), GIT_OK );

    const git_tree_entry* sneaky = git_tree_entry_byname( tree, "sneaky.txt" );
    BOOST_REQUIRE( sneaky );

    git_blob* blob = nullptr;
    BOOST_REQUIRE_EQUAL( git_blob_lookup( &blob, repo, git_tree_entry_id( sneaky ) ), GIT_OK );

    std::string sneakyContent( static_cast<const char*>( git_blob_rawcontent( blob ) ), git_blob_rawsize( blob ) );

    BOOST_CHECK_EQUAL( sneakyContent, "clean\n" );

    git_blob_free( blob );
    git_tree_free( tree );
    git_commit_free( head );
    git_repository_free( repo );
}


// The old version arrives as a new commit on top of the head, and never over uncommitted work.
BOOST_AUTO_TEST_CASE( RestoreVersionRoundTrip )
{
    BOOST_TEST_REQUIRE( ready() );

    git_repository* repo = openRepo();
    BOOST_TEST_REQUIRE( repo );

    git_oid secondOid = createCommit( repo, wxT( "file.txt" ), wxT( "second\n" ), wxT( "Second version" ) );

    git_object* firstObj = nullptr;
    BOOST_REQUIRE_EQUAL( git_revparse_single( &firstObj, repo, "HEAD~1" ), GIT_OK );
    git_oid firstOid = *git_object_id( firstObj );
    git_object_free( firstObj );

    char firstHash[GIT_OID_HEXSZ + 1] = { 0 };
    git_oid_tostr( firstHash, sizeof( firstHash ), &firstOid );

    KIGIT_COMMON common( repo, repoPath() );
    KIGIT_ERROR_MESSAGE error;

    // Refuses over uncommitted changes.
    wxString trackedFile = repoPath() + wxFileName::GetPathSeparator() + wxT( "file.txt" );
    {
        std::ofstream f( trackedFile.ToStdString(), std::ios::binary );
        f << "uncommitted\n";
    }

    BOOST_CHECK( common.RestoreVersion( wxString::FromUTF8( firstHash ), error ) == RestoreResult::Dirty );

    {
        std::ofstream f( trackedFile.ToStdString(), std::ios::binary );
        f << "second\n";
    }

    // Restores as a new commit whose parent is the current head.
    BOOST_REQUIRE_MESSAGE( common.RestoreVersion( wxString::FromUTF8( firstHash ), error ) == RestoreResult::Restored,
                           "RestoreVersion failed: " + error.summary.ToStdString() );

    git_oid newHead;
    BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &newHead, repo, "HEAD" ), GIT_OK );

    git_commit* restoreCommit = nullptr;
    BOOST_REQUIRE_EQUAL( git_commit_lookup( &restoreCommit, repo, &newHead ), GIT_OK );
    BOOST_CHECK_EQUAL( git_commit_parentcount( restoreCommit ), 1u );
    BOOST_CHECK( git_oid_equal( git_commit_parent_id( restoreCommit, 0 ), &secondOid ) );
    git_commit_free( restoreCommit );

    std::ifstream in( trackedFile.ToStdString() );
    std::string   content( ( std::istreambuf_iterator<char>( in ) ), std::istreambuf_iterator<char>() );
    BOOST_CHECK_EQUAL( content, "initial\n" );

    // Restoring the same version again is a no-op.
    BOOST_CHECK( common.RestoreVersion( wxString::FromUTF8( firstHash ), error ) == RestoreResult::AlreadyCurrent );

    git_repository_free( repo );
}


// Restoring one project in a shared repository must leave the siblings current.
BOOST_AUTO_TEST_CASE( RestoreScopesToProjectDirectory )
{
    BOOST_TEST_REQUIRE( ready() );

    KIGIT::GitRepositoryPtr repo( openRepo() );
    BOOST_REQUIRE( repo );

    git_oid rootOid;
    BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &rootOid, repo.get(), "HEAD" ), GIT_OK );
    createCommit( repo.get(), "projects/board/f.txt", "a1\n", "A v1" );
    git_oid bothV1 = createCommit( repo.get(), "projects/board2/g.txt", "b1\n", "B v1" );

    createCommit( repo.get(), "projects/board/f.txt", "a2\n", "A v2" );
    createCommit( repo.get(), "projects/board2/g.txt", "b2\n", "B v2" );

    char v1Hash[GIT_OID_HEXSZ + 1] = { 0 };
    git_oid_tostr( v1Hash, sizeof( v1Hash ), &bothV1 );

    const wxString projectPath = repoPath() + wxS( "/projects/board" );
    KIGIT_COMMON   common( repo.get(), projectPath );
    KIGIT_ERROR_MESSAGE error;

    BOOST_REQUIRE_MESSAGE( common.RestoreVersion( wxString::FromUTF8( v1Hash ), error ) == RestoreResult::Restored,
                           "RestoreVersion failed: " + error.summary.ToStdString() );

    auto readAll = []( const wxString& aPath )
    {
        std::ifstream in( aPath.ToStdString(), std::ios::binary );
        return std::string( ( std::istreambuf_iterator<char>( in ) ), std::istreambuf_iterator<char>() );
    };

    // The restored project went back, the sibling stayed current.
    BOOST_CHECK_EQUAL( readAll( repoPath() + wxS( "/projects/board/f.txt" ) ), "a1\n" );
    BOOST_CHECK_EQUAL( readAll( repoPath() + wxS( "/projects/board2/g.txt" ) ), "b2\n" );
    BOOST_CHECK_EQUAL( readAll( repoPath() + wxS( "/file.txt" ) ), "initial\n" );

    // A version from before the project's folder existed is refused, not emptied.
    char rootHash[GIT_OID_HEXSZ + 1] = { 0 };
    git_oid_tostr( rootHash, sizeof( rootHash ), &rootOid );

    BOOST_CHECK( common.RestoreVersion( wxString::FromUTF8( rootHash ), error ) == RestoreResult::Error );
    BOOST_CHECK_EQUAL( readAll( repoPath() + wxS( "/projects/board/f.txt" ) ), "a1\n" );

#ifndef __WINDOWS__
    // A symlink to the project must restore the same folder.
    const wxString projectLink = TempPath() + wxS( "/project-link" );
    BOOST_REQUIRE_EQUAL( symlink( projectPath.utf8_str(), projectLink.utf8_str() ), 0 );
    createCommit( repo.get(), "projects/board/f.txt", "a3\n", "A v3" );

    KIGIT_COMMON linkedProject( repo.get(), projectLink );
    BOOST_REQUIRE_MESSAGE( linkedProject.RestoreVersion( wxString::FromUTF8( v1Hash ), error )
                                   == RestoreResult::Restored,
                           "RestoreVersion failed: " + error.summary.ToStdString() );
    BOOST_CHECK_EQUAL( readAll( repoPath() + wxS( "/projects/board/f.txt" ) ), "a1\n" );
    BOOST_CHECK_EQUAL( readAll( repoPath() + wxS( "/projects/board2/g.txt" ) ), "b2\n" );
#endif
}


BOOST_AUTO_TEST_CASE( RestoreRefusesUnknownOrOutsideProject )
{
    BOOST_TEST_REQUIRE( ready() );

    KIGIT::GitRepositoryPtr repo( openRepo() );
    BOOST_REQUIRE( repo );

    git_oid firstOid;
    BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &firstOid, repo.get(), "HEAD" ), GIT_OK );
    git_oid currentOid = createCommit( repo.get(), "file.txt", "second\n", "Second version" );

    char firstHash[GIT_OID_HEXSZ + 1] = { 0 };
    git_oid_tostr( firstHash, sizeof( firstHash ), &firstOid );

    const wxString outsidePath = repoPath() + wxS( "2" );
    BOOST_REQUIRE( wxFileName::Mkdir( outsidePath ) );
    const std::vector<wxString> projectPaths = { wxEmptyString, TempPath(), outsidePath };

    for( const wxString& projectPath : projectPaths )
    {
        KIGIT_COMMON common( repo.get(), projectPath );
        KIGIT_ERROR_MESSAGE error;

        BOOST_CHECK( common.RestoreVersion( wxString::FromUTF8( firstHash ), error ) == RestoreResult::Error );
        BOOST_CHECK( !error.details.IsEmpty() );

        git_oid afterOid;
        BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &afterOid, repo.get(), "HEAD" ), GIT_OK );
        BOOST_CHECK( git_oid_equal( &currentOid, &afterOid ) );

        unsigned int status = 0;
        BOOST_REQUIRE_EQUAL( git_status_file( &status, repo.get(), "file.txt" ), GIT_OK );
        BOOST_CHECK_EQUAL( status, GIT_STATUS_CURRENT );

        wxString contents;
        wxFile   file( repoPath() + wxS( "/file.txt" ) );
        BOOST_REQUIRE( file.IsOpened() );
        BOOST_REQUIRE( file.ReadAll( &contents ) );
        BOOST_CHECK_EQUAL( contents.ToStdString(), "second\n" );
    }
}


// The colliding content was never committed, so nothing could bring it back.
BOOST_AUTO_TEST_CASE( RestoreRefusesUntrackedCollision )
{
    BOOST_TEST_REQUIRE( ready() );

    git_repository* repo = openRepo();
    BOOST_TEST_REQUIRE( repo );

    // A version that carries notes.txt, then a commit deleting it.
    git_oid withNotesOid = createCommit( repo, wxT( "notes.txt" ), wxT( "keep\n" ), wxT( "Add notes" ) );

    {
        git_index* index = nullptr;
        BOOST_REQUIRE_EQUAL( git_repository_index( &index, repo ), GIT_OK );
        BOOST_REQUIRE_EQUAL( git_index_remove_bypath( index, "notes.txt" ), GIT_OK );
        git_index_write( index );

        git_oid treeOid;
        BOOST_REQUIRE_EQUAL( git_index_write_tree( &treeOid, index ), GIT_OK );
        git_index_free( index );

        git_tree* tree = nullptr;
        BOOST_REQUIRE_EQUAL( git_tree_lookup( &tree, repo, &treeOid ), GIT_OK );

        git_commit* parent = nullptr;
        BOOST_REQUIRE_EQUAL( git_commit_lookup( &parent, repo, &withNotesOid ), GIT_OK );

        git_signature* sig = nullptr;
        BOOST_REQUIRE_EQUAL( git_signature_now( &sig, TEST_AUTHOR_NAME, TEST_AUTHOR_EMAIL ), GIT_OK );

        const git_commit* parents[1] = { parent };
        git_oid           deleteOid;
        BOOST_REQUIRE_EQUAL(
                git_commit_create( &deleteOid, repo, "HEAD", sig, sig, nullptr, "Delete notes", tree, 1, parents ),
                GIT_OK );

        git_signature_free( sig );
        git_commit_free( parent );
        git_tree_free( tree );
        wxRemoveFile( repoPath() + wxFileName::GetPathSeparator() + wxT( "notes.txt" ) );
    }

    char withNotesHash[GIT_OID_HEXSZ + 1] = { 0 };
    git_oid_tostr( withNotesHash, sizeof( withNotesHash ), &withNotesOid );

    KIGIT_COMMON common( repo, repoPath() );
    KIGIT_ERROR_MESSAGE error;

    // A new, never committed notes.txt collides with the version being restored.
    wxString notesFile = repoPath() + wxFileName::GetPathSeparator() + wxT( "notes.txt" );
    {
        std::ofstream f( notesFile.ToStdString(), std::ios::binary );
        f << "precious\n";
    }

    // An untracked file the old version does not carry must not block anything.
    wxString unrelatedFile = repoPath() + wxFileName::GetPathSeparator() + wxT( "unrelated.txt" );
    {
        std::ofstream f( unrelatedFile.ToStdString(), std::ios::binary );
        f << "scratch\n";
    }

    auto readAll = []( const wxString& aPath )
    {
        std::ifstream in( aPath.ToStdString(), std::ios::binary );
        return std::string( ( std::istreambuf_iterator<char>( in ) ), std::istreambuf_iterator<char>() );
    };

    BOOST_CHECK( common.RestoreVersion( wxString::FromUTF8( withNotesHash ), error ) == RestoreResult::Dirty );
    BOOST_CHECK_EQUAL( readAll( notesFile ), "precious\n" );

    // With the colliding file gone, the harmless untracked one alone does not block.
    wxRemoveFile( notesFile );

    BOOST_REQUIRE_MESSAGE( common.RestoreVersion( wxString::FromUTF8( withNotesHash ), error )
                                   == RestoreResult::Restored,
                           "RestoreVersion failed: " + error.summary.ToStdString() );
    BOOST_CHECK_EQUAL( readAll( notesFile ), "keep\n" );
    BOOST_CHECK_EQUAL( readAll( unrelatedFile ), "scratch\n" );

    git_repository_free( repo );
}


// A file deleted here and changed on the remote conflicts with no local side. Keeping the
// deletion must produce a merge commit without the file, not a zero byte one.
BOOST_AUTO_TEST_CASE( MergeConflictKeepsDeletion )
{
    BOOST_TEST_REQUIRE( ready() );

    git_repository* repo = openRepo();
    BOOST_TEST_REQUIRE( repo );

    git_reference* headRef = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_head( &headRef, repo ), GIT_OK );
    git_oid baseOid = *git_reference_target( headRef );
    git_reference_free( headRef );

    // Local side deletes the file.
    {
        git_index* index = nullptr;
        BOOST_REQUIRE_EQUAL( git_repository_index( &index, repo ), GIT_OK );
        BOOST_REQUIRE_EQUAL( git_index_remove_bypath( index, "file.txt" ), GIT_OK );
        git_index_write( index );

        git_oid treeOid;
        BOOST_REQUIRE_EQUAL( git_index_write_tree( &treeOid, index ), GIT_OK );
        git_index_free( index );

        git_tree* tree = nullptr;
        BOOST_REQUIRE_EQUAL( git_tree_lookup( &tree, repo, &treeOid ), GIT_OK );

        git_commit* baseCommit = nullptr;
        BOOST_REQUIRE_EQUAL( git_commit_lookup( &baseCommit, repo, &baseOid ), GIT_OK );

        git_signature* sig = nullptr;
        BOOST_REQUIRE_EQUAL( git_signature_now( &sig, TEST_AUTHOR_NAME, TEST_AUTHOR_EMAIL ), GIT_OK );

        const git_commit* parents[1] = { baseCommit };
        git_oid           deleteOid;
        BOOST_REQUIRE_EQUAL(
                git_commit_create( &deleteOid, repo, "HEAD", sig, sig, nullptr, "Delete file", tree, 1, parents ),
                GIT_OK );

        git_signature_free( sig );
        git_commit_free( baseCommit );
        git_tree_free( tree );
        wxRemoveFile( repoPath() + wxFileName::GetPathSeparator() + wxT( "file.txt" ) );
    }

    // Remote side modifies it, without moving HEAD.
    git_oid remoteCommitOid = createDetachedCommit( repo, baseOid, wxT( "file.txt" ), "remote\n", "Remote change" );

    writeFetchHead( remoteCommitOid );

    KIGIT_COMMON common( repo );

    wxString                          stageDir = tempBase() + wxFileName::GetPathSeparator() + wxT( "del_stages" );
    wxString                          headOid, theirsOid, conflictsError;
    KIGIT_ERROR_MESSAGE               error;
    std::vector<KIGIT_CONFLICT_SIDES> conflicts;

    BOOST_REQUIRE_MESSAGE( common.GetMergeConflicts( stageDir, headOid, theirsOid, conflicts, conflictsError ),
                           "GetMergeConflicts failed: " + conflictsError.ToStdString() );
    BOOST_REQUIRE_EQUAL( conflicts.size(), 1u );
    BOOST_CHECK( !conflicts[0].hasOurs );
    BOOST_CHECK( conflicts[0].hasTheirs );

    // Keep the deletion: an empty resolved path, not the extracted empty placeholder file.
    BOOST_REQUIRE_MESSAGE( common.CreateMergedCommit( headOid, theirsOid, { { conflicts[0].path, wxString() } }, error )
                                   == MergeCommitResult::Success,
                           "CreateMergedCommit failed: " + error.summary.ToStdString() );

    git_oid newHead;
    BOOST_REQUIRE_EQUAL( git_reference_name_to_id( &newHead, repo, "HEAD" ), GIT_OK );

    git_commit* mergeCommit = nullptr;
    BOOST_REQUIRE_EQUAL( git_commit_lookup( &mergeCommit, repo, &newHead ), GIT_OK );
    BOOST_CHECK_EQUAL( git_commit_parentcount( mergeCommit ), 2u );

    git_tree* mergedTree = nullptr;
    BOOST_REQUIRE_EQUAL( git_commit_tree( &mergedTree, mergeCommit ), GIT_OK );
    BOOST_CHECK_MESSAGE( git_tree_entry_byname( mergedTree, "file.txt" ) == nullptr,
                         "keeping the deletion must not commit the file" );
    git_tree_free( mergedTree );
    git_commit_free( mergeCommit );

    BOOST_CHECK( !wxFileName::FileExists( repoPath() + wxFileName::GetPathSeparator() + wxT( "file.txt" ) ) );

    git_repository_free( repo );
}


// Viewing a diff materializes a committed tree, and that must never replace what the user has
// staged.
BOOST_AUTO_TEST_CASE( WriteTreeToDir_LeavesIndexUntouched )
{
    BOOST_TEST_REQUIRE( ready() );

    git_repository* repo = openRepo();
    BOOST_TEST_REQUIRE( repo );

    // Stage content that differs from both HEAD and the working file.
    wxString filePath = repoPath() + wxFileName::GetPathSeparator() + wxT( "file.txt" );
    {
        std::ofstream f( filePath.ToStdString() );
        f << "staged-work-in-progress";
    }

    git_index* index = nullptr;
    BOOST_REQUIRE_EQUAL( git_repository_index( &index, repo ), GIT_OK );
    git_index_add_bypath( index, "file.txt" );
    git_index_write( index );

    const git_index_entry* before = git_index_get_bypath( index, "file.txt", 0 );
    BOOST_REQUIRE( before );
    git_oid stagedOid = before->id;

    // Under the fixture's own directory, so a failed run leaves nothing behind in the system temp.
    wxString dest = tempBase() + wxFileName::GetPathSeparator() + wxT( "tree_out" );

    BOOST_CHECK( KIGIT::PROJECT_GIT_UTILS::WriteTreeToDir( repo, wxT( "HEAD" ), dest ) );

    // Re-read the index from disk and confirm the staged blob is still the one we staged, not
    // the committed version that was checked out.
    git_index_read( index, 1 );
    const git_index_entry* after = git_index_get_bypath( index, "file.txt", 0 );
    BOOST_REQUIRE( after );
    BOOST_CHECK_MESSAGE( git_oid_equal( &stagedOid, &after->id ),
                         "WriteTreeToDir must not rewrite the repository index" );

    git_index_free( index );

    if( wxFileName::DirExists( dest ) )
        wxFileName::Rmdir( dest, wxPATH_RMDIR_RECURSIVE );

    git_repository_free( repo );
}


BOOST_AUTO_TEST_SUITE_END()
