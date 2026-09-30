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

#include <git/kicad_git_errors.h>
#include <import_export.h>

#include <git2.h>
#include <atomic>
#include <functional>
#include <mutex>
#include <set>
#include <utility>
#include <vector>

#include <wx/string.h>

class LIBGIT_BACKEND;

class APIEXPORT KIGIT_COMMON
{

public:
    KIGIT_COMMON( git_repository* aRepo );
    KIGIT_COMMON( const KIGIT_COMMON& aOther );
    ~KIGIT_COMMON();

    git_repository* GetRepo() const;

    void SetRepo( git_repository* aRepo )
    {
        m_repo = aRepo;
    }

    wxString GetCurrentBranchName() const;

    std::vector<wxString> GetBranchNames() const;

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

    wxString GetUsername() const
    {
        std::lock_guard<std::mutex> lock( m_credentialsMutex );
        return m_username;
    }

    wxString GetPassword();
    GIT_CONN_TYPE GetConnType() const;

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

    // Holds a temporary variable that can be used by the authentication callback
    // to remember which types of authentication have been tested so that we
    // don't loop forever.
    std::atomic<unsigned>& TestedTypes() { return m_testedTypes; }

    // Returns true if the repository has local commits that have not been pushed
    bool HasLocalCommits() const;

    // Returns true if the repository has a remote that can be pushed to pulled from
    bool HasPushAndPullRemote() const;

    // Updates the password and remote information for the repository given the current branch
    void UpdateCurrentBranchInfo();

    wxString GetGitRootDirectory() const;

    wxString GetRemotename() const;

    /// Returns GetRemotename() when non-empty, otherwise "origin".  Centralizes the
    /// fallback policy used by push/pull/fetch paths.
    wxString GetRemoteNameOrDefault() const;

    /// Returns the upstream shorthand for the current branch (e.g. "origin/master"),
    /// or an empty string when there is no current branch.
    wxString GetUpstreamShorthand() const;

    /**
     * Set the project directory path, preserving any symlinks in the path.
     * This is used to ensure git status paths match the paths used in the project tree.
     */
    void SetProjectDir( const wxString& aProjectDir ) { m_projectDir = aProjectDir; }

    /**
     * Get the project directory path. If a symlink-preserving path was set via
     * SetProjectDir(), returns that path. Otherwise falls back to git_repository_workdir().
     */
    wxString GetProjectDir() const;

    /// Why binding a repository last failed, for the panels to show. Empty on success.
    void            SetAttachError( const wxString& aError ) { m_attachError = aError; }
    const wxString& GetAttachError() const { return m_attachError; }

    void ResetNextKey()
    {
        std::lock_guard<std::mutex> lock( m_credentialsMutex );
        m_nextPublicKey = 0;
    }

    /// By value, so the caller never holds a pointer into the vector.
    wxString GetNextPublicKey()
    {
        std::lock_guard<std::mutex> lock( m_credentialsMutex );

        if( m_nextPublicKey >= static_cast<int>( m_publicKeys.size() ) )
            return wxEmptyString;

        return m_publicKeys[m_nextPublicKey++];
    }

    void SetRemote( const wxString& aRemote )
    {
        std::lock_guard<std::mutex> lock( m_credentialsMutex );
        m_remote = aRemote;
        m_password.clear();
        m_secretFetched = false;
        updateConnectionTypeUnlocked();
    }

    wxString GetRemote() const
    {
        std::lock_guard<std::mutex> lock( m_credentialsMutex );
        return m_remote;
    }

    int HandleSSHKeyAuthentication( git_cred** aOut, const wxString& aUsername );

    int HandlePlaintextAuthentication( git_cred** aOut, const wxString& aUsername );

    int HandleSSHAgentAuthentication( git_cred** aOut, const wxString& aUsername );

    static wxString GetLastGitError()
    {
        const git_error* error = git_error_last();

        if( error == nullptr )
            return wxString( "No error" );

        return wxString( error->message );
    }

    /// One token per operation. Born uncancelled, so nothing has to be reset before an
    /// operation starts and one operation's cancel cannot reach another.
    using CANCEL_TOKEN = std::shared_ptr<std::atomic<bool>>;

    static CANCEL_TOKEN NewCancelToken() { return std::make_shared<std::atomic<bool>>( false ); }

    /// Cancel whichever operation is running now. Nothing happens when none is.
    void CancelActiveOperation()
    {
        std::lock_guard<std::mutex> lock( m_operationMutex );

        if( m_activeCancel )
            m_activeCancel->store( true );
    }

    CANCEL_TOKEN ActiveCancelToken() const
    {
        std::lock_guard<std::mutex> lock( m_operationMutex );
        return m_activeCancel;
    }

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

    bool WasAuthFailure() const { return m_authFailed; }
    void ClearAuthFailure() { m_authFailed = false; }
    void SetAuthFailure() { m_authFailed = true; }

    std::unique_lock<std::mutex> TryLockAction()
    {
        return std::unique_lock<std::mutex>( m_gitActionMutex, std::try_to_lock );
    }

    std::unique_lock<std::mutex> LockAction() { return std::unique_lock<std::mutex>( m_gitActionMutex ); }

    // Views showing working-tree state register here, so a commit, push or pull in one of them
    // refreshes the others. UI thread only, so the list needs no lock.
    void AddRefreshListener( void* aOwner, std::function<void()> aCallback );
    void RemoveRefreshListener( void* aOwner );
    void NotifyRefreshListeners();

protected:
    git_repository* m_repo;

    wxString m_projectDir;  // Project directory path preserving symlinks
    wxString m_attachError;

    GIT_CONN_TYPE m_connType;
    wxString m_remote;      // This is the full connection string
    wxString m_hostname;    // This is just the hostname without the protocol, username, or password
    wxString m_username;
    wxString m_password;

    std::atomic<unsigned> m_testedTypes;

    std::atomic<bool> m_authFailed{ false };

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
    friend class PROJECT_TREE_PANE;

private:
    void updatePublicKeys();

    /// Body of SetSSHKey, for callers that already hold m_credentialsMutex.
    void setSSHKeyUnlocked( const wxString& aKey );
    void updateConnectionType();

    /// Body of updateConnectionType, for callers that already hold m_credentialsMutex.
    void updateConnectionTypeUnlocked();

    std::vector<wxString> m_publicKeys;
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


namespace KIGIT
{

/**
 * Report whether an SSH private key is protected by a passphrase.
 *
 * @p aKeyText is the whole contents of the private key file. Anything not
 * recognised as a private key is reported as unencrypted.
 */
APIEXPORT bool IsPrivateKeyEncrypted( const wxString& aKeyText );


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

} // namespace KIGIT


#endif // _GIT_COMMON_H_
