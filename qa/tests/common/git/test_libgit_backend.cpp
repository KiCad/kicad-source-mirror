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

#include <git/git_backend.h>
#include <git/libgit_backend.h>
#include <git/kicad_git_common.h>
#include <git/git_status_handler.h>
#include <git/project_git_utils.h>
#include <git/git_init_handler.h>
#include <git/git_pull_handler.h>
#include <git/git_push_handler.h>
#include <git/git_commit_handler.h>

#include <git2.h>

#include <fstream>

#include <wx/ffile.h>
#include <wx/filename.h>
#include <wx/textfile.h>
#include <wx/utils.h>


static const char* TEST_AUTHOR_NAME = "Test Author";
static const char* TEST_AUTHOR_EMAIL = "test@kicad.example";


/**
 * Build a temp directory tree containing a local working repo with one commit
 * and a bare "remote" repo cloned from it.  Helpers let individual tests
 * mutate this state (additional commits, upstream config, amends, etc.).
 */
struct GIT_BACKEND_FIXTURE
{
    GIT_BACKEND_FIXTURE()
    {
        m_backend = new LIBGIT_BACKEND();
        m_backend->Init();
        SetGitBackend( m_backend );

        m_tempBase = wxFileName::GetTempDir() + wxFileName::GetPathSeparator()
                     + wxString::Format( "kicad_libgit_backend_test_%ld_%ld", wxGetProcessId(), s_counter++ );
        wxFileName::Mkdir( m_tempBase, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );

        m_repoPath = m_tempBase + wxFileName::GetPathSeparator() + wxT( "repo" );
        m_remotePath = m_tempBase + wxFileName::GetPathSeparator() + wxT( "remote.git" );

        wxFileName::Mkdir( m_repoPath, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );

        m_ready = initRepoWithCommit() && initBareRemoteFromRepo();
    }


    ~GIT_BACKEND_FIXTURE()
    {
        if( wxFileName::DirExists( m_tempBase ) )
            wxFileName::Rmdir( m_tempBase, wxPATH_RMDIR_RECURSIVE );

        SetGitBackend( nullptr );
        m_backend->Shutdown();
        delete m_backend;
    }


    bool            ready() const { return m_ready; }
    const wxString& repoPath() const { return m_repoPath; }
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
        git_oid commitOid = {};

        wxString filePath = m_repoPath + wxFileName::GetPathSeparator() + aFile;
        {
            std::ofstream f( filePath.ToStdString() );
            f << aContent.ToStdString();
        }

        git_index* index = nullptr;

        if( git_repository_index( &index, aRepo ) != 0 )
            return commitOid;

        git_index_add_bypath( index, aFile.ToUTF8().data() );
        git_index_write( index );

        git_oid treeOid;
        git_index_write_tree( &treeOid, index );
        git_index_free( index );

        git_tree* tree = nullptr;

        if( git_tree_lookup( &tree, aRepo, &treeOid ) != 0 )
            return commitOid;

        git_signature* sig = nullptr;
        git_signature_now( &sig, TEST_AUTHOR_NAME, TEST_AUTHOR_EMAIL );

        // Parent is HEAD if it resolves, otherwise this becomes the root commit.
        git_commit*    parent = nullptr;
        git_reference* headRef = nullptr;

        if( git_repository_head( &headRef, aRepo ) == 0 )
        {
            git_reference_peel( (git_object**) &parent, headRef, GIT_OBJECT_COMMIT );
            git_reference_free( headRef );
        }

        const git_commit*  parents[1] = { parent };
        const git_commit** parentsPtr = parent ? parents : nullptr;
        size_t             parentsCount = parent ? 1 : 0;

        git_commit_create( &commitOid, aRepo, "HEAD", sig, sig, nullptr, aMessage.ToUTF8().data(), tree, parentsCount,
                           parentsPtr );

        if( parent )
            git_commit_free( parent );

        git_signature_free( sig );
        git_tree_free( tree );

        return commitOid;
    }


    /// Add an `origin` remote pointing at the bare remote on disk.
    bool addOrigin( git_repository* aRepo )
    {
        wxString    fileUrl = wxS( "file://" ) + m_remotePath;
        git_remote* remote = nullptr;
        int         rc = git_remote_create( &remote, aRepo, "origin", fileUrl.ToUTF8().data() );

        if( remote )
            git_remote_free( remote );

        return rc == 0;
    }


    /// Drop branch.<aBranch>.merge / .remote, simulating a repo where Add Version
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


    /// Set branch.<aBranch>.merge = refs/heads/<aBranch>, branch.<aBranch>.remote = origin.
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


private:
    bool initRepoWithCommit()
    {
        git_repository* repo = nullptr;

        // Force "master" as the initial branch so the tests are independent of the
        // user's init.defaultBranch (often "main") in system or global gitconfig.
        git_repository_init_options initOpts;
        git_repository_init_options_init( &initOpts, GIT_REPOSITORY_INIT_OPTIONS_VERSION );
        initOpts.initial_head = "master";

        if( git_repository_init_ext( &repo, m_repoPath.ToUTF8().data(), &initOpts ) != 0 )
            return false;

        git_config* cfg = nullptr;

        if( git_repository_config( &cfg, repo ) == 0 )
        {
            git_config_set_string( cfg, "user.name", TEST_AUTHOR_NAME );
            git_config_set_string( cfg, "user.email", TEST_AUTHOR_EMAIL );
            git_config_free( cfg );
        }

        createCommit( repo, wxT( "file.txt" ), wxT( "initial\n" ), wxT( "Initial commit" ) );
        git_repository_free( repo );
        return true;
    }


    bool initBareRemoteFromRepo()
    {
        git_clone_options opts;
        git_clone_init_options( &opts, GIT_CLONE_OPTIONS_VERSION );
        opts.bare = 1;

        wxString sourceUrl = wxS( "file://" ) + m_repoPath;

        git_repository* bare = nullptr;
        int             rc = git_clone( &bare, sourceUrl.ToUTF8().data(), m_remotePath.ToUTF8().data(), &opts );

        if( bare )
            git_repository_free( bare );

        return rc == 0;
    }


    LIBGIT_BACKEND* m_backend;
    wxString        m_tempBase;
    wxString        m_repoPath;
    wxString        m_remotePath;
    bool            m_ready;

    static long s_counter;
};

long GIT_BACKEND_FIXTURE::s_counter = 0;


BOOST_FIXTURE_TEST_SUITE( LibgitBackend, GIT_BACKEND_FIXTURE )


/**
 * The commit/amend checklists scope files to the project directory so that
 * unrelated files elsewhere in the repository do not leak into the commit
 * (issue #15910).  IsWithinProjectPath is the single scope predicate shared by
 * every checklist path in the project manager.
 */
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

    // An empty project path matches nothing rather than the whole repository.
    BOOST_CHECK( !KIGIT::PROJECT_GIT_UTILS::IsWithinProjectPath( wxT( "/repo/proj/f.txt" ), wxEmptyString ) );
}


/**
 * Regression for issue #15910: the Amend Last Commit checklist is seeded from a
 * tree diff of the previous commit.  Files that commit touched outside the
 * current project directory must be filtered out with IsWithinProjectPath,
 * exactly as the amend handler does, instead of leaking into the dialog.
 */
BOOST_AUTO_TEST_CASE( AmendFileList_ExcludesFilesOutsideProject )
{
    BOOST_TEST_REQUIRE( ready() );

    git_repository* repo = openRepo();
    BOOST_TEST_REQUIRE( repo );

    // The commit to be amended touches both a project-subdir file and an outside file, so
    // the HEAD-vs-parent diff contains one in-scope and one out-of-scope path.
    wxFileName::Mkdir( repoPath() + wxFileName::GetPathSeparator() + wxT( "proj" ), wxS_DIR_DEFAULT,
                       wxPATH_MKDIR_FULL );
    {
        std::ofstream f( ( repoPath() + wxFileName::GetPathSeparator() + wxT( "proj/proj.kicad_sch" ) ).ToStdString() );
        f << "sch\n";
    }
    {
        std::ofstream f( ( repoPath() + wxFileName::GetPathSeparator() + wxT( "outside.txt" ) ).ToStdString() );
        f << "unrelated\n";
    }

    {
        git_index* index = nullptr;
        BOOST_TEST_REQUIRE( git_repository_index( &index, repo ) == 0 );
        git_index_add_bypath( index, "proj/proj.kicad_sch" );
        git_index_add_bypath( index, "outside.txt" );
        git_index_write( index );

        git_oid treeOid;
        git_index_write_tree( &treeOid, index );
        git_index_free( index );

        git_tree* tree = nullptr;
        BOOST_TEST_REQUIRE( git_tree_lookup( &tree, repo, &treeOid ) == 0 );

        git_signature* sig = nullptr;
        git_signature_now( &sig, TEST_AUTHOR_NAME, TEST_AUTHOR_EMAIL );

        git_reference* headRefForParent = nullptr;
        git_repository_head( &headRefForParent, repo );

        git_commit* parent = nullptr;
        git_reference_peel( (git_object**) &parent, headRefForParent, GIT_OBJECT_COMMIT );
        git_reference_free( headRefForParent );

        const git_commit* parents[1] = { parent };
        git_oid           commitOid;
        git_commit_create( &commitOid, repo, "HEAD", sig, sig, nullptr, "touch both", tree, 1, parents );

        git_commit_free( parent );
        git_signature_free( sig );
        git_tree_free( tree );
    }

    KIGIT_COMMON common( repo );
    common.SetProjectDir( repoPath() + wxFileName::GetPathSeparator() );

    GIT_STATUS_HANDLER statusHandler( &common );
    wxString           repoWorkDir = statusHandler.GetWorkingDirectory();
    wxString projectPath = repoWorkDir + wxT( "proj" ) + wxFileName::GetPathSeparator();

    std::map<wxString, int> modifiedFiles;

    // Mirror the amend handler's diff-merge (HEAD vs parent), including the scope filter.
    git_reference* headRef = nullptr;
    BOOST_TEST_REQUIRE( git_repository_head( &headRef, repo ) == GIT_OK );

    git_commit* lastCommit = nullptr;
    git_reference_peel( (git_object**) &lastCommit, headRef, GIT_OBJECT_COMMIT );

    git_commit* parentCommit = nullptr;
    git_commit_parent( &parentCommit, lastCommit, 0 );

    git_tree* parentTree = nullptr;
    git_tree* lastTree = nullptr;
    git_commit_tree( &parentTree, parentCommit );
    git_commit_tree( &lastTree, lastCommit );

    git_diff* diff = nullptr;
    git_diff_tree_to_tree( &diff, repo, parentTree, lastTree, nullptr );

    size_t deltas = git_diff_num_deltas( diff );

    for( size_t ii = 0; ii < deltas; ++ii )
    {
        const git_diff_delta* delta = git_diff_get_delta( diff, ii );
        const char*           path = delta->new_file.path ? delta->new_file.path : delta->old_file.path;

        if( !path )
            continue;

        wxString absPath = repoWorkDir + wxString::FromUTF8( path );

        if( !KIGIT::PROJECT_GIT_UTILS::IsWithinProjectPath( absPath, projectPath ) )
            continue;

        modifiedFiles[wxString::FromUTF8( path )] |= GIT_STATUS_INDEX_MODIFIED;
    }

    BOOST_CHECK_MESSAGE( modifiedFiles.count( wxT( "outside.txt" ) ) == 0,
                         "outside.txt must not leak into the amend checklist" );
    BOOST_CHECK_MESSAGE( modifiedFiles.count( wxT( "proj/proj.kicad_sch" ) ) == 1,
                         "the project file should still appear in the amend checklist" );

    git_diff_free( diff );
    git_tree_free( lastTree );
    git_tree_free( parentTree );
    git_commit_free( parentCommit );
    git_commit_free( lastCommit );
    git_reference_free( headRef );
    git_repository_free( repo );
}



/**
 * When the local branch has no upstream configured, GetUpstreamShorthand must
 * synthesise <remote>/<branch> so the pull fallback and first-push paths have
 * a meaningful target to display and act on.
 */
BOOST_AUTO_TEST_CASE( GetUpstreamShorthand_NoUpstreamFallsBackToRemoteSlashBranch )
{
    BOOST_TEST_REQUIRE( ready() );

    git_repository* repo = openRepo();
    BOOST_TEST_REQUIRE( repo );

    // No upstream config, no remote-tracking ref.  Should still produce a useful
    // shorthand by combining the default remote name with the current branch.
    clearUpstreamConfig( repo, wxT( "master" ) );

    KIGIT_COMMON common( repo );
    BOOST_CHECK_EQUAL( common.GetUpstreamShorthand(), wxString( "origin/master" ) );

    git_repository_free( repo );
}


/**
 * Regression for the spurious clock icon after a message-only amend: when HEAD
 * and the upstream tracking ref point at different commit OIDs but the commits
 * share an identical tree, no file should be reported as AHEAD or BEHIND.
 *
 * Without the content-aware filter in GetDifferentFiles, the AHEAD set would
 * contain every file touched by the local commit relative to the merge-base.
 */
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


/**
 * Positive control: if the amend genuinely changes the file content, the file
 * should still appear in the AHEAD set.  Verifies the content-aware filter
 * doesn't over-filter.
 */
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


/**
 * Regression for the original issue #24332 surface: pull on a fresh repo where
 * Add Version Control wired up `origin` but never set per-branch upstream
 * tracking used to fail with "Could not lookup commit" because FETCH_HEAD has
 * no merge-marked entry.  The fallback in PerformPull resolves the implicit
 * target via refs/remotes/origin/<branch> and persists the upstream config.
 */
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
 * Push -u behavior: after the first successful push from a branch with no
 * upstream configured, branch.<name>.merge and .remote should be populated so
 * subsequent pull/push hit the FETCH_HEAD path instead of the fallback.
 */
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


/**
 * Amend rewrites HEAD with a new commit OID.  For a message-only amend, the
 * tree (and therefore the working tree) is unchanged.
 */
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


/**
 * Amend with new files: the tree changes and includes the newly staged content.
 */
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


/**
 * Amending an unborn branch (a fresh repo with no commits yet) must fail
 * cleanly rather than crash or produce a bogus commit.
 */
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


/**
 * InitializeRepository seeds a project-level .gitignore with KiCad-specific
 * transient paths (.history/, *-backups/, _autosave-*, fp-info-cache, ~*.lck).
 */
BOOST_AUTO_TEST_CASE( InitializeRepository_SeedsGitignoreWithKicadEntries )
{
    BOOST_TEST_REQUIRE( ready() );

    wxString freshDir = repoPath() + wxT( "_fresh" );
    wxFileName::Mkdir( freshDir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );

    KIGIT_COMMON     common( nullptr );
    GIT_INIT_HANDLER handler( &common );

    InitResult result = handler.InitializeRepository( freshDir );
    BOOST_CHECK_EQUAL( static_cast<int>( result ), static_cast<int>( InitResult::Success ) );

    wxFileName gitignoreFile( freshDir, wxT( ".gitignore" ) );
    BOOST_TEST_REQUIRE( gitignoreFile.FileExists() );

    wxString contents;
    {
        wxFFile f( gitignoreFile.GetFullPath(), wxT( "r" ) );
        f.ReadAll( &contents );
    }

    BOOST_CHECK( contents.Contains( wxT( ".history/" ) ) );
    BOOST_CHECK( contents.Contains( wxT( "*-backups/" ) ) );
    BOOST_CHECK( contents.Contains( wxT( "_autosave-*" ) ) );
    BOOST_CHECK( contents.Contains( wxT( "fp-info-cache" ) ) );
    BOOST_CHECK( contents.Contains( wxT( "~*.lck" ) ) );

    if( wxFileName::DirExists( freshDir ) )
        wxFileName::Rmdir( freshDir, wxPATH_RMDIR_RECURSIVE );
}


/**
 * InitializeRepository must not duplicate entries that the user (or a template)
 * has already written.  An existing `.history/` line should be left alone while
 * the other four KiCad defaults are appended.
 */
BOOST_AUTO_TEST_CASE( InitializeRepository_GitignoreDoesNotDuplicateEntries )
{
    BOOST_TEST_REQUIRE( ready() );

    wxString freshDir = repoPath() + wxT( "_dedup" );
    wxFileName::Mkdir( freshDir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );

    wxFileName gitignoreFile( freshDir, wxT( ".gitignore" ) );
    {
        wxFFile f( gitignoreFile.GetFullPath(), wxT( "w" ) );
        f.Write( wxT( "# my custom ignores\n.history/\nbuild/\n" ) );
    }

    KIGIT_COMMON     common( nullptr );
    GIT_INIT_HANDLER handler( &common );

    InitResult result = handler.InitializeRepository( freshDir );
    BOOST_CHECK_EQUAL( static_cast<int>( result ), static_cast<int>( InitResult::Success ) );

    wxTextFile tf;
    BOOST_TEST_REQUIRE( tf.Open( gitignoreFile.GetFullPath() ) );

    int historyCount = 0;
    int buildCount = 0;
    int fpInfoCount = 0;

    for( size_t i = 0; i < tf.GetLineCount(); ++i )
    {
        wxString line = tf.GetLine( i );
        line.Trim().Trim( false );

        if( line == wxT( ".history/" ) )
            historyCount++;
        else if( line == wxT( "build/" ) )
            buildCount++;
        else if( line == wxT( "fp-info-cache" ) )
            fpInfoCount++;
    }

    BOOST_CHECK_EQUAL( historyCount, 1 ); // was already present; not duplicated
    BOOST_CHECK_EQUAL( buildCount, 1 );   // user's custom entry preserved
    BOOST_CHECK_EQUAL( fpInfoCount, 1 );  // KiCad default that was missing got appended

    if( wxFileName::DirExists( freshDir ) )
        wxFileName::Rmdir( freshDir, wxPATH_RMDIR_RECURSIVE );
}


BOOST_AUTO_TEST_SUITE_END()
