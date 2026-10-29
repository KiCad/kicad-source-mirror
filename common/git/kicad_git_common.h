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

#ifndef _GIT_COMMON_H_
#define _GIT_COMMON_H_

#include <git/git_backend.h>
#include <git/git_compare_handler.h>
#include <git/kicad_git_errors.h>
#include <git/kigit_known_hosts.h>
#include <import_export.h>

#include <git2.h>
#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <optional>
#include <set>
#include <utility>
#include <vector>

#include <wx/datetime.h>
#include <wx/string.h>

class LIBGIT_BACKEND;
struct KIGIT_AUTH_ATTEMPT;


/// What libgit2 reported for a failed call, read before anything else can replace it.
struct KIGIT_ERROR_INFO
{
    int      klass = 0; ///< git_error_t
    int      code = 0;  ///< The failed call's return value
    wxString message;
};


// One entry in the commit history, as shown by the advanced git window's History page.
struct KIGIT_COMMIT_INFO
{
    wxString              hash;    ///< Short commit hash.
    wxString              summary; ///< First line of the commit message.
    wxString              author;  ///< Author name.
    wxString              email;   ///< Author email, used to look up a web avatar.
    wxDateTime            when;    ///< Commit time.
    std::vector<wxString> refs;    ///< HEAD, branches and tags at this commit, in that order.
};


// What the Stashes page's detail pane shows about one stash.
struct KIGIT_STASH_DETAIL
{
    bool                  found = false;
    wxString              message;  ///< What was stashed, without the "WIP on branch:" prefix.
    wxString              branch;   ///< The branch the stash was taken on, as its message records it.
    wxString              baseHash; ///< Short hash of the commit the stash was taken on.
    wxDateTime            when;
    std::vector<wxString> files; ///< Paths the stash changes, new files included.
};


// A local or remote branch and its tip commit.
struct KIGIT_BRANCH_INFO
{
    wxString   name;
    bool       isRemote = false;
    bool       isCurrent = false;
    wxString   tipHash; ///< Short hash of the commit the branch points at.
    wxString   message; ///< Full message of the tip commit.
    wxDateTime when;    ///< That commit's time.
};


// Commit counts between two references.
struct KIGIT_AHEAD_BEHIND
{
    size_t ahead = 0;  ///< Commits only in the first reference.
    size_t behind = 0; ///< Commits only in the second reference.
};


// What the Branches page's detail pane shows about one branch.
struct KIGIT_BRANCH_DETAIL
{
    bool       found = false;
    wxString   name;
    wxString   tipHash;
    wxDateTime when;
    wxString   author;
    wxString   email;
    wxString   message;
    wxString   upstream; ///< The branch this one tracks, or empty.

    /// Comparison with upstream, if it could be read.
    std::optional<KIGIT_AHEAD_BEHIND> divergence;
};


// Which commits the History page walks.
struct KIGIT_HISTORY_FILTER
{
    wxString branch; ///< One branch, local or remote, or empty for every branch and HEAD.
    wxString search; ///< Keep only commits whose summary, author or hash contains this.
    wxString path;   ///< Keep only commits that changed this file or folder, relative to the root.
};


// One file a commit changed, with its line counts against the parent.
struct KIGIT_COMMIT_FILE
{
    wxString                  path;
    wxString                  oldPath; ///< For a rename or copy, where the file came from.
    KIGIT::FILE_CHANGE_STATUS status = KIGIT::FILE_CHANGE_STATUS::MODIFIED;
    int                       added = 0;
    int                       removed = 0;
    bool                      binary = false;
};


// Everything the History page's detail pane shows about one commit.
struct KIGIT_COMMIT_DETAIL
{
    bool                           found = false;
    wxString                       hash; ///< Full commit hash.
    wxString                       shortHash;
    wxString                       message; ///< Full message, body included.
    wxString                       author;
    wxString                       email;
    wxDateTime                     when;
    std::vector<wxString>          parents; ///< Short hashes.
    std::vector<wxString>          refs;    ///< As in KIGIT_COMMIT_INFO.
    std::vector<KIGIT_COMMIT_FILE> files;
    int                            added = 0;
    int                            removed = 0;
};


// One entry in the stash list, as shown by the advanced git window's Stashes page.
struct KIGIT_STASH_INFO
{
    size_t     index = 0; ///< Position in the stash list, which shifts as stashes come and go.
    wxString   id;        ///< Full hash of the stash commit, which does not.
    wxString   message;   ///< The line git writes, such as "WIP on main: 7f8e9d1 subject".
    wxDateTime when;      ///< Time the stash was created.
};


// One entry in the tag list, as shown by the advanced git window's Tags page.
struct KIGIT_TAG_INFO
{
    wxString   name;    ///< Tag name.
    wxString   hash;    ///< Short hash of the tagged commit.
    wxString   summary; ///< First line of the tagged commit message.
    wxDateTime when;    ///< When the tag was made, or the commit for a lightweight tag.
};


// What the Tags page's detail pane shows about one tag.
struct KIGIT_TAG_DETAIL
{
    bool       found = false; ///< False when no tag of that name points at a commit.
    wxString   name;          ///< The tag name.
    wxString   hash;          ///< Short hash of the tagged commit.
    wxDateTime when;          ///< When the tag was made, or the commit for a lightweight tag.
    wxString   author;        ///< Who made the tag, or the commit for a lightweight tag.
    wxString   email;         ///< Their email.
    wxString   message;       ///< The tag's own message, or the commit's for a lightweight tag.
};


// The kind of operation a repository can be left in the middle of.
enum class GitOperation
{
    None,
    Merge,
    Rebase,
    CherryPick,
    Revert,
    Other
};


// Outcome of restoring an old version as a new commit.
enum class RestoreResult
{
    Restored,       ///< A new commit carrying the old version's content was created.
    AlreadyCurrent, ///< The chosen version's content matches the current head.
    Dirty,          ///< Uncommitted changes exist, and restoring would overwrite them.
    Error           ///< The restore failed. See the error string.
};


// Each side of a conflicted file, extracted to a temp file. A side that does not exist is empty.
struct KIGIT_CONFLICT_SIDES
{
    wxString path;                ///< Repository-relative path of the conflicted file.
    wxString ancestorFile;        ///< Extracted common-ancestor version, an empty file when absent.
    wxString oursFile;            ///< Extracted local version, an empty file when absent.
    wxString theirsFile;          ///< Extracted remote version, an empty file when absent.
    bool     hasAncestor = false; ///< False when the file did not exist in the common ancestor.
    bool     hasOurs = false;     ///< False when the local side deleted the file.
    bool     hasTheirs = false;   ///< False when the remote side deleted the file.
};


/**
 * The project's git session. A project can be open before it is ever put under version
 * control, and integration can be turned off for it, so a session with no repository is a
 * normal state rather than a programming error. Every method answers empty, false or nothing
 * in that case.
 */
class APIEXPORT KIGIT_COMMON
{

public:
    /// aProjectPath is the absolute project directory. Leave it empty for handlers that
    /// do not need a project folder.
    KIGIT_COMMON( git_repository* aRepo, const wxString& aProjectPath = wxEmptyString );
    KIGIT_COMMON( const KIGIT_COMMON& aOther );
    ~KIGIT_COMMON();

    git_repository* GetRepo() const;

    void SetRepo( git_repository* aRepo )
    {
        m_repo = aRepo;
    }

    wxString GetCurrentBranchName() const;

    /// Full commit ID at HEAD, or empty for an unborn or missing HEAD. The caller holds the
    /// action lock when capturing this alongside the branch name for a later operation.
    wxString GetHeadCommitId() const;

    /// Local and remote branches with their tip commits. Symbolic aliases are omitted.
    /// Callers choose the display order.
    std::vector<KIGIT_BRANCH_INFO> GetBranches() const;

    // One branch's tip commit and how it stands to the branch it tracks. Not found leaves the
    // result's found flag clear.
    KIGIT_BRANCH_DETAIL GetBranchDetail( const wxString& aBranch ) const;

    /**
     * Return a vector of project files in the repository.  Sorted by the depth of
     * the project file in the directory tree
     *
     * @return std::vector<wxString> of project files
     */
    std::vector<wxString> GetProjectDirs();

    /**
     * Return a pair of sets of files that differ locally from the remote repository
     * The first set is files that have been committed locally but not pushed
     * The second set is files that have been committed remotely but not pulled
     */
    std::pair<std::set<wxString>,std::set<wxString>> GetDifferentFiles() const;

    enum class GIT_STATUS
    {
        GIT_STATUS_UNTRACKED,
        GIT_STATUS_CURRENT,
        GIT_STATUS_MODIFIED,        // File changed but not committed to local repository
        GIT_STATUS_ADDED,
        GIT_STATUS_DELETED,
        GIT_STATUS_BEHIND,          // File changed in remote repository but not in local
        GIT_STATUS_AHEAD,           // File changed in local repository but not in remote
        GIT_STATUS_CONFLICTED,
        GIT_STATUS_IGNORED,
        GIT_STATUS_LAST
    };

    enum class GIT_CONN_TYPE
    {
        GIT_CONN_HTTPS = 0,
        GIT_CONN_SSH,
        GIT_CONN_LOCAL,
        GIT_CONN_LAST
    };

    /// A transport address with any embedded credentials extracted.
    struct REMOTE_URL
    {
        wxString      url; ///< Keeps the SSH username, scheme and port, but never a password.
        wxString      username;
        wxString      password;
        wxString      hostname;
        int           port = 0; ///< 0 when the address names none.
        GIT_CONN_TYPE connType = GIT_CONN_TYPE::GIT_CONN_LOCAL;
    };

    static REMOTE_URL ParseRemoteURL( const wxString& aURL );

    wxString GetUsername() const
    {
        std::lock_guard<std::mutex> lock( m_credentialsMutex );
        return m_username;
    }

    wxString GetPassword();
    GIT_CONN_TYPE GetConnType() const;

    /// For a remote whose address names no transport, the user picks one in the prompt.
    void SetConnType( GIT_CONN_TYPE aConnType );

    void SetUsername( const wxString& aUsername )
    {
        std::lock_guard<std::mutex> lock( m_credentialsMutex );
        m_username = aUsername;
        m_secretFetched = false;
    }

    void SetPassword( const wxString& aPassword )
    {
        std::lock_guard<std::mutex> lock( m_credentialsMutex );
        m_password = aPassword;
        m_secretFetched = true;
    }
    void SetSSHKey( const wxString& aSSHKey );

    /// The key the project was set up with, even when its file is gone, otherwise the first key
    /// offered. Empty when none is known.
    wxString GetSSHKey() const;

    /// The key the next attempt will try first, for the credential prompt to show. Empty when no
    /// key file can be offered.
    wxString GetFirstOfferedSSHKey() const;

    /// The encrypted key the server last accepted for this remote, so the credential prompt asks for the
    /// right passphrase. Empty when no server has accepted one.
    wxString GetAcceptedSSHKey() const;
    void     SetAcceptedSSHKey( const wxString& aKey );

    /// The remote the credential prompt was opened for. Read as one, so a transfer syncing the
    /// URL on another thread cannot leave the fields describing two remotes.
    struct CREDENTIAL_IDENTITY
    {
        wxString      remote;
        GIT_CONN_TYPE connType = GIT_CONN_TYPE::GIT_CONN_LOCAL;
        wxString      username;
    };

    /// What the credential prompt collected. No username leaves the session's alone, for an SSH
    /// address that names it. An empty key keeps the current one.
    struct PROMPTED_CREDENTIALS
    {
        GIT_CONN_TYPE           connType = GIT_CONN_TYPE::GIT_CONN_LOCAL;
        std::optional<wxString> username;
        wxString                password;
        wxString                sshKey;
    };

    CREDENTIAL_IDENTITY GetCredentialIdentity() const;

    /**
     * Apply what the prompt collected, unless the session has moved to another remote since
     * aFor was taken. The check and the apply share one critical section, so a background fetch
     * cannot slip between them. The per-attempt state is reset with it, so the next transfer
     * starts over with these. Returns false, leaving everything alone, when the identity moved.
     */
    bool ApplyPromptedCredentials( const CREDENTIAL_IDENTITY& aFor, const PROMPTED_CREDENTIALS& aEntered );

    // Holds a temporary variable that can be used by the authentication callback
    // to remember which types of authentication have been tested so that we
    // don't loop forever.
    std::atomic<unsigned>& TestedTypes() { return m_testedTypes; }

    /// True when HEAD has commits its upstream does not, or has no upstream yet.
    /// False for a detached or unborn HEAD.
    bool HasLocalCommits() const;

    /// True when the upstream branch has commits the current branch does not.
    bool HasUpstreamCommits() const;

    /// True when the repository is set to rebase rather than merge when getting changes.
    bool IsPullRebaseConfigured() const;

    /// True when HEAD points at a commit rather than a branch, as after checking out a tag.
    bool IsHeadDetached() const;

    /// Commits in aBranch that HEAD does not contain. No value when either cannot be read.
    std::optional<size_t> GetUnmergedCommitCount( const wxString& aBranch ) const;

    // True when tracked files hold uncommitted changes, staged or not. Untracked files do not
    // count: a merge leaves them alone. aReadOk distinguishes a clean tree from a failed read.
    bool HasUncommittedTrackedChanges( bool* aReadOk = nullptr ) const;

    // The full messages of the newest commits from HEAD, newest first, with repeats dropped.
    std::vector<wxString> GetRecentCommitMessages( size_t aMax ) const;

    // The most recent commits aFilter selects, newest first, up to aMax of them.
    std::vector<KIGIT_COMMIT_INFO> GetRecentCommits( size_t aMax = 50, const KIGIT_HISTORY_FILTER& aFilter = {} ) const;

    // One commit in full, with the files it changed against its first parent. A root commit's
    // files all count as added. Not found leaves the result's found flag clear.
    KIGIT_COMMIT_DETAIL GetCommitDetail( const wxString& aHash ) const;

    // The full message of the commit HEAD points at, body and trailers included. The amend
    // prefill uses this, so accepting the suggestion does not truncate the message to its
    // first line.
    wxString GetLastCommitMessage() const;

    // The stashes in the repository, in the order git reports them (newest first).
    std::vector<KIGIT_STASH_INFO> GetStashes() const;

    // One stash in full, by its commit id. Not found leaves the result's found flag clear.
    KIGIT_STASH_DETAIL GetStashDetail( const wxString& aId ) const;

    // The tags in the repository, each resolved to the commit it points at.
    std::vector<KIGIT_TAG_INFO> GetTags() const;

    // One tag in full. Not found leaves the result's found flag clear.
    KIGIT_TAG_DETAIL GetTagDetail( const wxString& aName ) const;

    /// True while the repository has no commits, so HEAD names a branch that does not exist yet.
    bool IsHeadUnborn() const;

    // The caller holds the action lock, as with the query methods above.
    // Create aName at the tip of aFrom, a local or remote branch, or at HEAD when aFrom is empty.
    bool CreateBranch( const wxString& aName, const wxString& aFrom = wxEmptyString );

    /**
     * Point HEAD at a branch that does not exist yet, so the first commit creates it.
     *
     * Only meaningful while the repository has no commits. With no commit to start it at, a
     * branch cannot be created any other way.
     */
    bool StartUnbornBranch( const wxString& aName );
    bool DeleteBranch( const wxString& aName );

    /// Set the uncommitted tracked changes aside as a new stash, as git stash push would.
    bool StashSave( const wxString& aMessage );
    bool StashApply( size_t aIndex );
    bool StashPop( size_t aIndex );
    bool StashDrop( size_t aIndex );

    // Tag HEAD. With a message the tag is annotated and carries it, otherwise it is lightweight.
    bool CreateTag( const wxString& aName, const wxString& aMessage = wxEmptyString );
    bool DeleteTag( const wxString& aName );
    bool CheckoutTag( const wxString& aName );

    bool AddRemote( const wxString& aName, const wxString& aUrl );
    bool SetRemoteUrl( const wxString& aName, const wxString& aUrl );
    bool RemoveRemote( const wxString& aName );

    bool SetUserConfig( const wxString& aName, const wxString& aEmail );

    /**
     * Recompute the merge in memory and extract each conflicted file's three versions into
     * aStageDir, without touching the repository. aHeadOid pins HEAD for CreateMergedCommit.
     */
    bool GetMergeConflicts( const wxString& aStageDir, wxString& aHeadOid, wxString& aTheirsOid,
                            std::vector<KIGIT_CONFLICT_SIDES>& aConflicts, wxString& aError );

    /**
     * Restore the project as it was at aCommitHash, recorded as a new commit on top of the
     * current head. Only the session's project folder is restored. A failure leaves its reason
     * in aError's details, and a summary only when the next step is known.
     */
    RestoreResult RestoreVersion( const wxString& aCommitHash, KIGIT_ERROR_MESSAGE& aError );

    /**
     * Put the given working directory relative paths back to their committed state. A path
     * that is not in HEAD keeps its file and loses its staged entry.
     */
    bool DiscardChanges( const std::vector<wxString>& aPaths, wxString& aError );

    /// True when the working directory relative path exists in the HEAD commit's tree.
    bool IsPathInHead( const wxString& aPath );

    /// True while aPath is one of the files a merge or rebase stopped on.
    bool IsPathConflicted( const wxString& aPath ) const;

    /// True while a merge, rebase, cherry-pick or revert is in progress, which refuses commits.
    bool IsOperationInProgress() const;

    /// Which operation that is, so the panel can name it.
    GitOperation OperationInProgress() const;

    /// Drop that operation and return to the last commit. The caller holds the action lock.
    bool AbandonOperation( wxString& aError );

    /**
     * Redo the merge with each conflicted path replaced by its resolved file, then commit and
     * check out. All or nothing.
     *
     * aTheirsOid pins the remote commit, so a background fetch cannot change what gets merged.
     * An empty resolved-file path records "the file is deleted". A failure leaves its reason in
     * aError's details, and a summary only when the next step is known.
     */
    MergeCommitResult CreateMergedCommit( const wxString& aExpectedHeadOid, const wxString& aTheirsOid,
                                          const std::vector<std::pair<wxString, wxString>>& aResolvedFiles,
                                          KIGIT_ERROR_MESSAGE&                              aError );

    // Returns true if the repository has a remote that can be pushed to pulled from
    bool HasPushAndPullRemote() const;

    // Updates the password and remote information for the repository given the current branch
    void UpdateCurrentBranchInfo();

    /**
     * Bind the session to the URL a transfer is about to connect to. An unchanged URL keeps the
     * session credentials, so a password corrected in the prompt outlives a stale one in the URL.
     * A changed URL replaces the username and password with its own, clearing any it lacks.
     */
    void SyncRemoteUrl( const wxString& aRawUrl );

    /**
     * Bind the session to a URL that url.insteadOf rewrote, keeping the username, password and
     * key entered for the original.
     */
    void FollowRewrittenUrl( const wxString& aRawUrl );

    wxString GetGitRootDirectory() const;

    wxString GetRemotename() const;

    /// Returns every remote as a (name, URL) pair.
    std::vector<std::pair<wxString, wxString>> GetRemotes() const;

    // When the repository last fetched, from any remote. Invalid when it never has.
    wxDateTime GetLastFetchTime() const;

    // Lines added and removed in the working tree against HEAD, new files included. Only for
    // aPaths, relative to the repository root, so the count matches the list beside it.
    std::pair<int, int> GetWorkingTreeLineStats( const std::set<wxString>& aPaths ) const;

    /// Returns GetRemotename() when non-empty, otherwise "origin".  Centralizes the
    /// fallback policy used by push/pull/fetch paths.
    wxString GetRemoteNameOrDefault() const;

    /**
     * Keep symlinks in the repository path so Git status paths match the project tree.
     */
    void SetProjectDir( const wxString& aProjectDir ) { m_projectDir = aProjectDir; }

    /**
     * Return the path set by SetProjectDir(), or the repository's working directory.
     */
    wxString GetProjectDir() const;

    /// Project folder relative to the repository, with a trailing slash. An empty string
    /// means the repository root. No value means the folder is unknown.
    std::optional<wxString> GetProjectPrefix() const;

    /// Why binding a repository last failed, for the panels to show. Empty on success.
    void SetAttachError( const wxString& aError, const wxString& aTrustCommand = wxEmptyString )
    {
        m_attachError = aError;
        m_attachTrustCommand = aTrustCommand;
    }

    const wxString& GetAttachError() const { return m_attachError; }

    /// The command that trusts a repository another user owns, when that is why attaching failed.
    const wxString& GetAttachTrustCommand() const { return m_attachTrustCommand; }

    /// Start the next attempt from the first key, with the key that last connected to the current
    /// remote moved up.
    void ResetNextKey();

    /// Remember the key that connected for @p aAttempt, so the next connection offers it first.
    void RememberWorkingSSHKey( const KIGIT_AUTH_ATTEMPT& aAttempt );

    /// By value, so the caller never holds a pointer into the vector.
    wxString GetNextPublicKey();

    void SetRemote( const wxString& aRemote )
    {
        {
            std::lock_guard<std::mutex> lock( m_credentialsMutex );
            m_remote = aRemote;
            m_password.clear();
            m_secretFetched = false;
            m_acceptedSSHKey.clear();
            updateConnectionTypeUnlocked();
        }

        // Gathers the keys now, or the first transfer's sync would drop a key chosen after this
        SyncRemoteUrl( aRemote );
    }

    wxString GetRemote() const
    {
        std::lock_guard<std::mutex> lock( m_credentialsMutex );
        return m_remote;
    }

    /// Offer the agent's keys once, then the next key file, recording in aAttempt which file was offered.
    int HandleSSHKeyAuthentication( git_cred** aOut, const wxString& aUsername, KIGIT_AUTH_ATTEMPT& aAttempt );

    int HandlePlaintextAuthentication( git_cred** aOut, const wxString& aUsername );

    int HandleSSHAgentAuthentication( git_cred** aOut, const wxString& aUsername );

    /// libgit2's last message, without credentials from any URL it quotes, so it is safe to show or trace.
    static wxString GetLastGitError();

    /// The error libgit2 holds for a call that returned aCode. Read it straight after the call, because the
    /// callbacks and cleanup that follow can replace it.
    static KIGIT_ERROR_INFO GetLastGitErrorInfo( int aCode );

    /// One token per operation. Born uncancelled, so nothing has to be reset before an
    /// operation starts and one operation's cancel cannot reach another.
    using CANCEL_TOKEN = std::shared_ptr<std::atomic<bool>>;

    static CANCEL_TOKEN NewCancelToken() { return std::make_shared<std::atomic<bool>>( false ); }

    /// Publishes a token for as long as its operation runs. Constructed once the action lock
    /// is held, so Cancel can only reach the operation that owns the repository.
    class OPERATION_GUARD
    {
    public:
        OPERATION_GUARD( KIGIT_COMMON* aCommon, const CANCEL_TOKEN& aToken ) :
                m_common( aCommon ),
                m_token( aToken )
        {
            std::lock_guard<std::mutex> lock( m_common->m_operationMutex );
            m_common->m_activeCancel = m_token;

            // A shutdown asked for before this operation reached the lock still applies to it.
            if( m_token && m_common->m_shutdown )
                m_token->store( true );
        }

        /// Only while it is still the published one, or a slow operation finishing would
        /// drop a newer one's token.
        ~OPERATION_GUARD()
        {
            std::lock_guard<std::mutex> lock( m_common->m_operationMutex );

            if( m_common->m_activeCancel == m_token )
                m_common->m_activeCancel.reset();
        }

        OPERATION_GUARD( const OPERATION_GUARD& ) = delete;
        OPERATION_GUARD& operator=( const OPERATION_GUARD& ) = delete;

    private:
        KIGIT_COMMON* m_common;
        CANCEL_TOKEN  m_token;
    };

    /// Background timers ask this before starting work of their own.
    bool IsShuttingDown() const
    {
        std::lock_guard<std::mutex> lock( m_operationMutex );
        return m_shutdown;
    }

    /// Stop the operation running now and keep later ones from starting.
    void SetShuttingDown( bool aCancel )
    {
        std::lock_guard<std::mutex> lock( m_operationMutex );
        m_shutdown = aCancel;

        if( aCancel && m_activeCancel )
            m_activeCancel->store( true );
    }

    /// The SSH port the session's remote names for aHost, 22 when it names none, or -1 when the remote is on
    /// another host or its port is not valid.
    int SSHPortFor( const wxString& aHost ) const;

    std::unique_lock<std::mutex> TryLockAction()
    {
        return std::unique_lock<std::mutex>( m_gitActionMutex, std::try_to_lock );
    }

    /// Long enough for a status read to finish, short enough that a window does not look hung.
    static constexpr std::chrono::milliseconds ACTION_LOCK_WAIT{ 2000 };

    /// TryLockAction that keeps trying for aWait before giving up. For the UI thread, where a
    /// blocking LockAction could wait out a whole network timeout.
    std::unique_lock<std::mutex> TryLockActionFor( std::chrono::milliseconds aWait );

    std::unique_lock<std::mutex> LockAction() { return std::unique_lock<std::mutex>( m_gitActionMutex ); }

    // Views showing working-tree state register here, so a commit, push or pull in one of them
    // refreshes the others. UI thread only, so the list needs no lock.
    void AddRefreshListener( void* aOwner, std::function<void()> aCallback );
    void RemoveRefreshListener( void* aOwner );
    void NotifyRefreshListeners();

protected:
    git_repository* m_repo;

    wxString m_projectDir;  // Repository working directory preserving symlinks
    wxString m_projectPath; // Project directory captured when the session was created
    wxString m_attachError;
    wxString m_attachTrustCommand;

    GIT_CONN_TYPE m_connType;
    wxString m_remote;      // This is the full connection string
    wxString m_urlUsername; // The username m_remote was parsed with, which HTTPS strips from it
    wxString m_hostname;    // This is just the hostname without the protocol, username, or password
    int      m_port = 0;    // The port m_remote names, 0 when it names none and -1 when it is not valid
    wxString m_username;
    wxString m_password;

    std::atomic<unsigned> m_testedTypes;

    std::mutex m_gitActionMutex;

    /// Serializes the credential fields between the UI thread and the authentication
    /// callback's worker thread. wxString is copy on write, so a read racing a write corrupts
    /// it. Separate from m_gitActionMutex, which the worker holds while the callback runs.
    mutable std::mutex m_credentialsMutex;

    std::vector<std::pair<void*, std::function<void()>>> m_refreshListeners;

    // Make git handlers friends so they can access the mutex
    friend class GIT_PUSH_HANDLER;
    friend class GIT_PULL_HANDLER;
    friend class GIT_CLONE_HANDLER;
    friend class LIBGIT_BACKEND;

private:
    /// Count commits unique to aBranch and aBase. No value if the history cannot be read.
    std::optional<KIGIT_AHEAD_BEHIND> compareRefs( const git_reference* aBranch, const git_reference* aBase ) const;

    CANCEL_TOKEN ActiveCancelToken() const
    {
        std::lock_guard<std::mutex> lock( m_operationMutex );
        return m_activeCancel;
    }

    /// Remote to use when HEAD names no upstream, empty when several remotes leave it ambiguous.
    wxString remoteNameWithoutUpstream() const;

    void updatePublicKeys();

    /// Rebuild m_publicKeys as the project key, when offerable, then the key that last connected to
    /// m_remote, then the discovered keys, and restart from the first. The caller holds
    /// m_credentialsMutex.
    void rebuildPublicKeysUnlocked();
    void updateConnectionType();

    /// Body of updateConnectionType, for callers that already hold m_credentialsMutex.
    void updateConnectionTypeUnlocked();

    std::vector<wxString> m_discoveredKeys; // Gathered for m_publicKeysHost, without the project key
    std::vector<wxString> m_publicKeys;
    wxString m_publicKeysHost; // The host m_publicKeys was gathered for
    wxString m_projectSSHKey;  // Kept when its file is missing, so the setting is not lost
    wxString              m_acceptedSSHKey; // An encrypted key the server took for m_remote, still to be unlocked
    int m_nextPublicKey;
    bool m_secretFetched;

    /// Guarded together, so a shutdown cannot miss an operation registering itself at the
    /// same moment. The token is null when nothing is running.
    mutable std::mutex m_operationMutex;
    CANCEL_TOKEN       m_activeCancel;
    bool               m_shutdown = false;

    // Create a dummy flag to tell if we have tested ssh agent credentials separately
    // from the ssh key credentials. Our own bit, so it must clear every git_credential_t
    // value. The highest is GIT_CREDENTIAL_SSH_MEMORY at 1 << 6.
    static const unsigned KIGIT_CREDENTIAL_SSH_AGENT = 1u << ( sizeof( unsigned ) * 8 - 1 );
};

extern "C" APIEXPORT int  progress_cb( const char* str, int len, void* data );
extern "C" APIEXPORT void clone_progress_cb( const char* str, size_t len, size_t total, void* data );
extern "C" APIEXPORT int transfer_progress_cb( const git_transfer_progress* aStats, void* aPayload );
extern "C" APIEXPORT int update_cb( const char* aRefname, const git_oid* aFirst, const git_oid* aSecond,
                                    void* aPayload );
extern "C" APIEXPORT int push_transfer_progress_cb( unsigned int aCurrent, unsigned int aTotal,
                                                    size_t aBytes, void* aPayload );
extern "C" APIEXPORT int push_update_reference_cb( const char* aRefname, const char* aStatus,
                                                   void* aPayload );

extern "C" APIEXPORT int fetchhead_foreach_cb( const char*, const char*,
                                               const git_oid* aOID, unsigned int aIsMerge, void* aPayload );
extern "C" APIEXPORT int credentials_cb( git_cred** aOut, const char* aUrl, const char* aUsername,
                                         unsigned int aAllowedTypes, void* aPayload );

/**
 * Check an SSH host key against known_hosts, which decides alone. A listed key returns 0, and any other
 * state fails with GIT_ECERTIFICATE and is recorded with KIGIT_REPO_MIXIN::SetHostKeyFailure. A key KiCad
 * cannot read, or a host or port that does not match the synced remote, fails without a record. Other
 * certificates, such as HTTPS ones, are left to libgit2. Never returns a positive value, which libgit2 1.9
 * takes as accepting an unverified SSH host.
 */
extern "C" APIEXPORT int certificate_check_cb( git_cert* aCert, int aValid, const char* aHost, void* aPayload );


class wxWindow;


namespace KIGIT
{

/**
 * Report whether an SSH private key is protected by a passphrase.
 *
 * @p aKeyText is the whole contents of the private key file. Anything not
 * recognised as a private key is reported as unencrypted.
 */
APIEXPORT bool IsPrivateKeyEncrypted( const wxString& aKeyText );

/// IsPrivateKeyEncrypted for the key file @p aKey. A file that cannot be read is reported as unencrypted.
APIEXPORT bool IsPrivateKeyFileEncrypted( const wxString& aKey );

/// git's core.sshCommand for @p aRepo, or from the user's git config when @p aRepo is null. Empty when unset.
APIEXPORT wxString ConfiguredSSHCommand( git_repository* aRepo );


/**
 * The advanced git window, or null when none is open.
 *
 * Kept here because libcommon is linked into the project manager and into each editor
 * separately, so a static there would give every one of them a window of its own.
 */
APIEXPORT wxWindow*& ActiveAdvancedGitWindow();


/**
 * Resolve a string ref (branch name, short OID, full OID, tag) to its tree.
 *
 * Runs git_revparse_single followed by git_object_peel( GIT_OBJECT_TREE ), so a
 * commit, tag or tree ref all resolve to the underlying tree.  Returns nullptr
 * on error and logs via wxLogTrace( traceGit ).
 *
 * The caller owns the returned tree and must free it with git_tree_free (wrap
 * in KIGIT::GitTreePtr for RAII).  The return type is a raw pointer rather than
 * GitTreePtr because the lambda-deleter closure type backing GitTreePtr is
 * distinct per translation unit, so it cannot cross the kicommon ABI boundary.
 */
APIEXPORT git_tree* ResolveRefToTree( git_repository* aRepo, const wxString& aRef );


/**
 * Refs to offer when comparing branches, base first and head second. Only refs that resolve are
 * listed. The base is main or master when it is not the current branch, else the current
 * branch's upstream. The head is the current branch, or HEAD when detached.
 *
 * The caller must hold the git action lock.
 */
APIEXPORT std::vector<wxString> CompareBranchRefs( git_repository* aRepo );


/**
 * Walk every delta in a computed diff, invoking @p aCallback once per delta.
 *
 * Centralizes the git_diff_num_deltas / git_diff_get_delta loop.  Null deltas
 * are skipped.  A null diff is a no-op.
 *
 * The callback is a std::function rather than a header-inline template because
 * this helper lives behind the APIEXPORT/libkicommon ABI boundary; a template
 * would have to re-expose the git2 loop in the header to every caller.
 */
APIEXPORT void CollectDiffDeltas( git_diff*                                           aDiff,
                                  const std::function<void( const git_diff_delta& )>& aCallback );

/**
 * Join a summary line, an optional description and an optional sign-off into one commit
 * message. @p aSignOff is the "Name <email>" to put after "Signed-off-by:", or empty for none.
 * A message that already ends with that sign-off does not get it twice.
 */
APIEXPORT wxString ComposeCommitMessage( const wxString& aSummary, const wxString& aDescription,
                                         const wxString& aSignOff );

/// Split a commit message into its first line and the description after the first blank line.
APIEXPORT void SplitCommitMessage( const wxString& aMessage, wxString& aSummary, wxString& aDescription );

/// Empty when aName is a branch name git would accept, otherwise why it would not, in the
/// words to show the user.
APIEXPORT wxString BranchNameError( const wxString& aName );

/// Decode text that came out of a repository. Git stores it as bytes with no encoding, so a
/// repository written on Windows can carry ISO 8859-1, which FromUTF8 alone would blank.
APIEXPORT wxString DecodeRepoText( const char* aText );

/// What every caller says when it finds the session already busy. One wording, so the same
/// condition does not reach the user in three different sentences.
APIEXPORT wxString BusyMessage();

} // namespace KIGIT


#endif // _GIT_COMMON_H_
