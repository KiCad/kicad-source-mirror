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

/**
 * @file mdw_git_remote.h
 * @brief What kicad-mdw-git knows about reaching its repository's remote.
 *
 * Credentials arrive in Authenticate and live here, in memory, for the life of the process. A
 * transfer that fails is turned into the error the client acts on, a credential prompt or a host
 * key the user may trust, which this remembers until TrustHostKey answers it.
 */

#ifndef MDW_GIT_REMOTE_H
#define MDW_GIT_REMOTE_H

#include <git/kigit_failure.h>
#include <git/kigit_transport.h>

#include <api/workspace/workspace_types.pb.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

struct git_repository;
class MDW_GIT_SESSION;

namespace KIGIT
{
class REPO_ACTION_LOCK;
}

/// The last libgit2 error with any credentials in a quoted URL removed
std::string MdwGitError();

/// Take aLock, waiting out other holders for a while. WE_BUSY when one outlasts the wait, and a null aLock is refused
kiapi::workspace::WorkspaceError MdwGitLockActions( KIGIT::REPO_ACTION_LOCK* aLock );

/// A random name no other writer picks, which also keeps each claim record unique
std::string MdwRandomToken();

/// aText, or empty when it is not UTF-8, which protobuf string fields and JSON events refuse
std::string MdwValidUtf8( const std::string& aText );

/// The credential form a git remote takes, with no realm, naming the password a passphrase over SSH
kiapi::workspace::AuthChallenge MdwGitAuthChallenge( const std::string& aUsernameHint, bool aSsh );


/// What a transfer may authenticate with, held in memory only
struct MDW_GIT_CREDENTIALS
{
    std::string username;
    std::string password;   ///< An HTTPS password or token
    std::string sshKeyPath; ///< Offered before the default keys when set
    std::string sshPassphrase;
};


/// What a failed transfer's session learned beyond libgit2's own error
struct MDW_GIT_FAILURE_FACTS
{
    KIGIT_AUTH_ATTEMPT                     attempt;
    bool                                   authFailed = false; ///< The credential callback had nothing left to offer
    std::optional<KIGIT::HOST_KEY_FAILURE> hostKey;            ///< The host key the transfer refused, if any
    wxString                               hostKeyRefusal;     ///< Why a key that cannot be trusted was refused
    bool                                   ssh = false;
    bool                                   urlLogin = false; ///< The HTTP(S) address carried a login of its own
};


class MDW_GIT_REMOTE
{
public:
    using CALL = std::function<int( KIGIT::REMOTE_TRANSPORT& aTransport )>;

    /// Whether a failed push of this kind shows the user cannot write the library, as opposed to being refused once
    static bool RecordsDenial( KIGIT_FAILURE aKind )
    {
        // Any SSH stderr reads as a server refusal, so a transient one must not lock the library
        return aKind == KIGIT_FAILURE::PUSH_FORBIDDEN;
    }

    /**
     * The headline for a failed sign-in. A realm means HTTP(S), where the host words it from the server and the
     * user, so none is given. SSH has no realm, so the classifier's advice about keys and passphrases is kept.
     */
    static wxString SignInMessage( const KIGIT_FAILURE_REPORT& aReport, const wxString& aRealm )
    {
        return ( aRealm.IsEmpty() && KIGIT::IsCredentialFailure( aReport.kind ) ) ? aReport.summary : wxString();
    }

    /// The error a failed transfer to aUrl answers with. aForbidden is set only for a push, as only Push passes it
    kiapi::workspace::WorkspaceError Describe( const KIGIT_ERROR_INFO& aError, const MDW_GIT_FAILURE_FACTS& aFacts,
                                               const char* aUrl, bool* aForbidden = nullptr );

    /// Waits for the checks given up on, which must end before libgit2 shuts down
    ~MDW_GIT_REMOTE();

    /// Keep the fields of an Authenticate request, replacing any kept before. Credentials the server refused are
    /// offered to no transfer until this is called again, even with the same fields
    void SetCredentials( const std::map<std::string, std::string>& aFields );

    /// Counts the credential changes seen, so what was learned under older credentials can be told from the rest
    std::uint64_t CredentialGeneration() const;

    /// The user name kept now, empty when none was given
    std::string Username() const;

    /**
     * Run aCall with the kept credentials on a transport to aRemoteName of aRepo, or to no remote when aRepo
     * is null, as a clone wants, and answer the error to report. An unknown host key is remembered for
     * TrustHostKey; a changed or revoked one is refused outright.
     */
    kiapi::workspace::WorkspaceError Transfer( git_repository* aRepo, const std::string& aRemoteName,
                                               const CALL& aCall );

    /// Connect to aUrl, which no remote need name, as Transfer would
    kiapi::workspace::WorkspaceError Check( const std::string& aUrl );

    /**
     * Check aUrl, giving up with WE_TIMEOUT once aLimit passes. libgit2 cannot stop a silent read, so a check
     * given up on finishes alone on its own thread.
     */
    kiapi::workspace::WorkspaceError Check( const std::string& aUrl, std::chrono::milliseconds aLimit );

    /// Push aUpdates as REMOTE_TRANSPORT::PushRefs does. aError says why anything but Success came back
    RefPushResult Push( git_repository* aRepo, const std::string& aRemoteName,
                        const std::vector<GIT_REF_UPDATE>& aUpdates, std::vector<GIT_REF_RESULT>& aResults,
                        kiapi::workspace::WorkspaceError& aError );

    /// Push branch aBranch only while the remote still holds aUpstream there, or no such branch when it is absent
    RefPushResult PushBranch( git_repository* aRepo, const std::string& aRemoteName, const std::string& aBranch,
                              const std::optional<std::string>& aUpstream, kiapi::workspace::WorkspaceError& aError );

    /// Told after each push whether the remote took it or its hook or ACL refused it, with the refs it carried
    using PUSH_OBSERVER = std::function<void( const std::vector<GIT_REF_UPDATE>&, kiapi::workspace::PermissionState )>;

    void SetPushObserver( PUSH_OBSERVER aObserver ) { m_pushObserver = std::move( aObserver ); }

    /// Add aKey to known_hosts when it is the key the last refusal named. False when it is not
    bool Trust( const kiapi::workspace::HostKey& aKey );

private:
    using CANCEL = std::shared_ptr<const std::atomic<bool>>;

    /// A call that also reads what the session kept, before a later call on the same transport resets it
    using SESSION_CALL = std::function<int( KIGIT::REMOTE_TRANSPORT& aTransport, MDW_GIT_SESSION& aSession )>;

    struct CHECK_OUTCOME;

    kiapi::workspace::WorkspaceError check( const std::string& aUrl, const CANCEL& aCancel );

    /// Transfer, setting aForbidden when a push failed because the server forbids writing
    kiapi::workspace::WorkspaceError transfer( git_repository* aRepo, const std::string& aRemoteName,
                                               const SESSION_CALL& aCall, bool* aForbidden );

    kiapi::workspace::WorkspaceError run( git_repository* aRepo, git_remote* aRemote, const SESSION_CALL& aCall,
                                          const CANCEL& aCancel, bool* aForbidden = nullptr );

    kiapi::workspace::WorkspaceError fail( MDW_GIT_SESSION& aSession, const KIGIT_ERROR_INFO& aError, const char* aUrl,
                                           bool* aForbidden );

    mutable std::mutex                     m_mutex;
    MDW_GIT_CREDENTIALS                    m_credentials;
    std::uint64_t                          m_generation = 0;
    std::uint64_t                          m_signIns = 0; ///< Counts every sign-in, changed or not
    std::optional<std::uint64_t>           m_refused;     ///< The sign-in the server refused
    std::optional<KIGIT::HOST_KEY_FAILURE> m_pendingHostKey;
    PUSH_OBSERVER                          m_pushObserver;

    std::mutex                                                          m_abandonedMutex;
    std::vector<std::pair<std::thread, std::shared_ptr<CHECK_OUTCOME>>> m_abandoned;
};

#endif // MDW_GIT_REMOTE_H
