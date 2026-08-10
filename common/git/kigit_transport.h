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
 * @file kigit_transport.h
 * @brief The one place a fetch, push, clone or connection check reaches a remote.
 *
 * Every call offers the session's credentials and checks the server's host key the same way,
 * reports progress through its handler and stops when the handler's operation is cancelled.
 */

#ifndef KIGIT_TRANSPORT_H
#define KIGIT_TRANSPORT_H

#include <git/kigit_known_hosts.h>
#include <git/kigit_remote_url.h>
#include <import_export.h>
#include <optional>

#include <git2.h>
#include <wx/arrstr.h>

#include <functional>
#include <map>
#include <memory>
#include <vector>


/// What libgit2 reported for a failed call, read before anything else can replace it.
struct KIGIT_ERROR_INFO
{
    int      klass = 0; ///< git_error_t
    int      code = 0;  ///< The failed call's return value
    wxString message;
};


/// What the credential callback handed out during one operation, kept to explain a rejection.
struct KIGIT_AUTH_ATTEMPT
{
    KIGIT::GIT_CONN_TYPE  connType = KIGIT::GIT_CONN_TYPE::GIT_CONN_LOCAL;
    std::vector<wxString> keysTried;               ///< Key files offered, in order.
    bool                  agentTried = false;      ///< The SSH agent was offered before any key file.
    bool                  passphraseGiven = false; ///< A passphrase went with the last key file offered.
    wxString              acceptedKey;             ///< An encrypted key whose public half the server accepted.
    bool                  lastKeyRefused = false;  ///< The server refused the last key's public half.
    wxString              probing;                 ///< The encrypted key whose public half is out to the server now.
    bool                  passwordGiven = false;   ///< A password went with the last username offered.
};


/// One ref a remote should create, move or delete in one push.
struct GIT_REF_UPDATE
{
    wxString localRef; ///< The ref sent, or empty to delete remoteRef
    wxString remoteRef;
    bool     force = false;
    bool     leased = false; ///< Refuse the whole push unless the remote holds expectedOld
    wxString expectedOld;    ///< Hex id the remote must hold, or empty when the ref must not exist
};


enum class RefPushResult
{
    Success,
    Rejected,    ///< The remote refused at least one ref; the results say which
    LeaseBroken, ///< A leased ref had moved, so nothing was sent
    Error
};


/// How the remote answered one GIT_REF_UPDATE.
struct GIT_REF_RESULT
{
    wxString remoteRef;
    bool     accepted = false;
    wxString message;
};


namespace KIGIT
{

/// How long a connect and a stalled transfer may wait, which libgit2 otherwise leaves to the OS
constexpr int SERVER_CONNECT_TIMEOUT_MS = 10000;

/// The longest silence on one read, not the whole transfer. A server packing a large repository can stay
/// silent a long time before its first byte, and a cancel during a silence waits this long too.
constexpr int SERVER_TRANSFER_TIMEOUT_MS = 120000;

/**
 * Apply SERVER_CONNECT_TIMEOUT_MS and SERVER_TRANSFER_TIMEOUT_MS to libgit2 after git_libgit2_init. False
 * when libgit2 refuses them. Without them a host that drops packets holds a call for minutes.
 */
APIEXPORT bool SetServerTimeouts();

/// The error libgit2 holds for a call that returned aCode. Read it straight after the call, because the
/// callbacks and cleanup that follow can replace it.
APIEXPORT KIGIT_ERROR_INFO GetLastGitErrorInfo( int aCode );


/// The part of a fetch or clone that UpdateStep counts
enum class TRANSFER_STEP
{
    RECEIVE, ///< Objects arriving from the remote
    CHECKOUT ///< Files written to the working tree
};


/// The credentials and outcome records one transfer runs on
class APIEXPORT TRANSPORT_SESSION
{
public:
    virtual ~TRANSPORT_SESSION() = default;

    /// Aim the session at the URL a call is about to dial
    virtual void SyncRemoteUrl( const wxString& aUrl ) = 0;

    /// As SyncRemoteUrl, for a URL url.insteadOf rewrote, keeping the credentials entered for the original
    virtual void FollowRewrittenUrl( const wxString& aUrl ) = 0;

    virtual GIT_CONN_TYPE GetConnType() const = 0;
    virtual wxString      GetUsername() const = 0;

    /// The HTTPS password, or the passphrase for the ssh key files
    virtual wxString GetPassword() = 0;

    /// The next ssh key file to offer, or empty when none is left
    virtual wxString GetNextPublicKey() = 0;

    /// The port the remote names for aHost, or -1 when aHost is not the remote's
    virtual int SSHPortFor( const wxString& aHost ) const = 0;

    virtual bool IsThisOperationCancelled() const = 0;

    /// False to offer the server no credential at all, not even a login in the URL, so a login it refused is never
    /// sent again
    virtual bool MayAuthenticate() const { return true; }

    /// True when the session answers with a login of its own, so one written into the URL, which may be stale,
    /// is never sent ahead of it
    virtual bool HasOwnCredentials() const { return false; }

    /// Forget everything an earlier call recorded
    virtual void                ResetTransferState() = 0;
    virtual KIGIT_AUTH_ATTEMPT& AuthAttempt() = 0;
    virtual void                SetAuthFailure() = 0;
    virtual void                SetHostKeyFailure( const HOST_KEY_FAILURE& aFailure ) = 0;
    virtual void                SetHostKeyRefusal( const wxString& aReason ) = 0;

    virtual void UpdateProgress( int aCurrent, int aTotal, const wxString& aMessage ) {}

    /// Transfer progress as counts, for a caller without a progress bar. aTotal is 0 while unknown
    virtual void UpdateStep( TRANSFER_STEP aStep, size_t aDone, size_t aTotal ) {}
    virtual void ReceiveSideband( const char* aData, size_t aLength ) {}
    virtual void SetPushRefusal( const wxString& aStatus ) {}

    /// Called once a call has connected, so the key that worked is offered first next time and a waiting save
    /// keeps the password that worked
    virtual void RememberWorkingCredential() {}
};


class APIEXPORT REMOTE_TRANSPORT
{
public:
    /**
     * aSession supplies the credentials and records progress, the credentials offered and why a call
     * failed. aRemote is the caller's and must outlive the transport. A transport for a clone takes none.
     * Without aFollowRedirects a push the server redirects fails, so it reaches only the address the
     * caller checked. Older libgit2 versions cannot enforce this and use their default redirect behavior.
     */
    explicit REMOTE_TRANSPORT( TRANSPORT_SESSION* aSession, git_remote* aRemote = nullptr,
                               bool aFollowRedirects = true );
    ~REMOTE_TRANSPORT();

    REMOTE_TRANSPORT( const REMOTE_TRANSPORT& ) = delete;
    REMOTE_TRANSPORT& operator=( const REMOTE_TRANSPORT& ) = delete;

    /// Fetch as `git fetch` would. aBranches, when given, receives the branches the remote advertised
    int Fetch( std::vector<wxString>* aBranches = nullptr );

    /**
     * Connect, then push the refspec aConnected answers, so the caller can check what it pushes after a
     * connection that may have taken long. An empty answer disconnects and returns GIT_EUSER.
     */
    int Push( const std::function<wxString()>& aConnected );

    /// Fetch exactly aRefspecs. Tags arrive only when a refspec names them. Without aWriteFetchHead, FETCH_HEAD
    /// keeps the last fetch that wrote it
    int FetchRefspecs( const std::vector<wxString>& aRefspecs, bool aPrune, bool aWriteFetchHead = true );

    /**
     * Download the objects aRefspecs would fetch without writing any ref. aAdvertised gets every ref the remote
     * advertised on that connection, by name, as a hex id, so the objects each matching ref names are present.
     */
    int DownloadRefspecs( const std::vector<wxString>& aRefspecs, std::map<wxString, wxString>& aAdvertised );

    /**
     * Push aUpdates in one connection. Leased updates are checked against the remote's own values before
     * any object is sent, so a ref someone else moved is never overwritten or deleted. Error() explains
     * an Error answer.
     */
    RefPushResult PushRefs( const std::vector<GIT_REF_UPDATE>& aUpdates, std::vector<GIT_REF_RESULT>& aResults );

    /**
     * Connect and read the commit the remote holds at aRef, empty when it holds none. Nothing when the remote
     * could not be read.
     */
    std::optional<wxString> ReadRemoteRef( const wxString& aRef );

    /// Connect, which proves the credentials and the host key without transferring anything
    int Check();

    /**
     * Clone aUrl into aPath, with no working tree when aBare. The new repository is returned in aRepo for the
     * caller to free
     */
    int Clone( const wxString& aUrl, const wxString& aPath, git_repository** aRepo, bool aBare = false );

    /// libgit2's error for the last call that failed, read before anything else could replace it
    const KIGIT_ERROR_INFO& Error() const;

    struct STATE;

private:
    int connect( git_direction aDirection, const git_remote_callbacks& aCallbacks );

    std::unique_ptr<STATE> m_state;
    git_remote*            m_remote;
    bool                   m_followRedirects;
};

} // namespace KIGIT

#endif // KIGIT_TRANSPORT_H
