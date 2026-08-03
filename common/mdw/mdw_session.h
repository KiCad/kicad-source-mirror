/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file mdw_session.h
 * @brief One bound project's workspace session.
 *
 * A session ties a project path to its binding and the supervisor that owns the
 * spawned backend process. One process runs per (backend_id, project_path); a
 * second editor opening the same project spawns its own, indistinguishable from
 * a second user to the backend.
 */

#ifndef MDW_SESSION_H
#define MDW_SESSION_H

#include <kicommon.h>
#include <mdw/client/mdw_client_types.h>
#include <mdw/client/mdw_op.h>
#include "mdw_backend_supervisor.h"
#include "mdw_workspace_service.h"
#include "mdw_event_bus.h"
#include "mdw_lock_tracker.h"
#include "mdw_dirty_tracker.h"
#include "mdw_serial_worker.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>

#include <wx/string.h>

class MDW_EVENT_INGRESS;
class MDW_CREDENTIALS;

class KICOMMON_API MDW_SESSION
{
public:
    MDW_SESSION( const MDW_PROJECT_BINDING& aBinding, std::unique_ptr<MDW_BACKEND_SUPERVISOR> aSupervisor );

    ~MDW_SESSION();

    const MDW_PROJECT_BINDING& Binding() const { return m_binding; }

    /**
     * The source's identity, the binding without the backend's binding target. Every source compares by it, while
     * the backend's Hello keeps the whole binding.
     */
    const wxString& SourceLocator() const { return m_sourceLocator; }

    /// The SourceLocator() of a session aManifest's backend opens on aBinding
    static wxString SourceLocatorOf( const wxString& aBinding, const MDW_BACKEND_MANIFEST& aManifest );

    MDW_BACKEND_SUPERVISOR* Supervisor() const { return m_supervisor.get(); }

    std::shared_ptr<MDW_BACKEND> Backend() const;

    /// Where the connection stands. Every change of phase reaches the status listener once
    MDW_CONNECTION_STATUS Status() const;

    /**
     * Called from any thread when the phase changes. Set it before the session is published; it must not
     * call back into the session.
     */
    void SetStatusListener( std::function<void()> aListener );

    /// How long until the next server check is due, or -1 while the backend is stopped or respawning
    std::chrono::milliseconds ProbeDueIn() const;

    /**
     * Ask the backend once with CheckConnection whether its server answers, backing off from 2 s to
     * 60 s while it does not. A reachable session is asked once its server has been quiet for the
     * heartbeat, so an idle session still notices the server going away. A server found again while
     * a login is still unverified is sent that login once more. Blocking; the framework runs it on its
     * worker when ProbeDueIn() is zero.
     */
    void Probe( int aTimeoutMs );

    /**
     * Where the session reads the stored credentials whenever it signs in again. Set it before the
     * session is published. It runs on the thread signing in, never the UI thread.
     */
    void SetCredentialSource( std::function<MDW_TYPED_CREDENTIALS()> aSource );

    /**
     * Where refusals of the stored login are kept for every session on the server, so a session opened later
     * never sends one again. It must outlive the session. Set it before the session is published.
     */
    void SetRefusalRecord( MDW_CREDENTIALS* aRecord );

    /// Who the stored credentials sign in as, which Status() reports; empty for the server's default account
    void SetUsername( const wxString& aUsername );

    /// The private key file every login sends when the backend's form has a field for one, empty for its default
    void SetKeyFile( const wxString& aKeyFile );

    /**
     * Sign the backend in with credentials the user typed, passing through CONNECTING. A backend whose
     * Hello named no form is sent none. The session keeps no copy, so an unverified login is sent again
     * only from the credential source. Success re-reads the capabilities and announces
     * CAPABILITIES_CHANGED. Blocking; must not run on the UI thread.
     */
    MDW_RESULT Authenticate( const MDW_TYPED_CREDENTIALS& aCredentials, int aTimeoutMs );

    /**
     * Authenticate() with the credentials the source reads now, or with aStored when an unlock prompt read
     * them where the source cannot. A login the server could not verify is sent again once the server
     * answers, as is one the backend drops later. A stored login the server refused is never sent again
     * until the source reads another one.
     */
    MDW_RESULT Reconnect( int aTimeoutMs, std::optional<MDW_TYPED_CREDENTIALS> aStored = std::nullopt );

    /// Whether the server refused aStored when it was last sent as the stored login, so sending it again cannot help
    bool StoredLoginRefused( const MDW_TYPED_CREDENTIALS& aStored ) const;

    /// Whether the server refused the last stored login sent, so the store holds nothing worth sending yet
    bool StoredLoginRefused() const;

    /**
     * Another login was stored for this session's server. A session waiting for a sign-in sends what the
     * source reads now at its next probe, unless the server already refused that login. True when it will.
     */
    bool StoredLoginChanged();

    /**
     * The shared dispatch path for the current backend process, or nullptr when none runs. Rebuilt after
     * a restart, so take it per operation; one held across a restart fails its calls.
     */
    std::shared_ptr<MDW_WORKSPACE_SERVICE> Service();

    /**
     * Pass the progress the backend reports for the operation aOperationId to aSink, on the ingress thread,
     * until the returned handle is released. A backend that reports none leaves aSink unused. Releasing the
     * handle waits for a call to aSink already running, so aSink must never wait on the releasing thread.
     */
    [[nodiscard]] std::shared_ptr<void> WatchProgress( const wxString& aOperationId, MDW_PROGRESS_SCOPE::SINK aSink );

    /**
     * Run aWork on this session's worker thread and hand its result to aDone on the main thread, unless
     * aOwner is gone by then. Work runs in submission order off the shared thread pool, so a backend timeout
     * never occupies it, and may block on MdwSync.
     */
    template <typename RESULT>
    void RunAsync( wxEvtHandler* aOwner, std::function<RESULT()> aWork, std::function<void( RESULT )> aDone );

    /**
     * Read every lock the backend knows into the lock tracker and announce LOCKS_CHANGED. Blocking; must not
     * run on the UI thread. BACKEND_LOST when no backend runs.
     */
    MDW_RESULT ListLocks();

    /**
     * Read every lock into the lock tracker again on the worker, once events flow. A library source's backend
     * can list them only after its copy is made, which may be after this session started.
     */
    void RefreshLocks();

    MDW_EVENT_BUS& EventBus() { return *m_eventBus; }

    MDW_LOCK_TRACKER& LockTracker() { return *m_lockTracker; }

    MDW_DIRTY_TRACKER& DirtyTracker() { return *m_dirtyTracker; }

    /**
     * Begin accepting the events the backend pushes at the URL the supervisor gave it. The backend
     * retries an unacknowledged event, so starting after it spawned delays delivery rather than losing it.
     * The lock tracker is read whole now and whenever the session comes online again, since events only
     * keep it current from there. Returns false when the address cannot be bound.
     */
    bool StartEventIngress();

    /// Take no more outcomes or events from the backend, so the supervisor can shut it down
    void DetachBackend();

    /**
     * Drop everything built around the dead backend so a replacement can be launched. Its sockets live
     * in a runtime directory the replacement will not share and its bus sequence numbers belong to the old
     * process. Bus subscribers are kept.
     */
    void DiscardBackendState();

    /// Mark the respawn finished or failed, and stop the session for good when the supervisor gave up on the
    /// backend
    void BackendReplaced();

    /// Moves on with every respawn, so a listener can tell a restarted backend from the one it saw
    std::uint64_t BackendGeneration() const { return m_backendGeneration.load(); }

    /// Whether the framework holds this session and so respawns a backend it loses. False ends every wait
    void SetRespawnable( bool aRespawnable );

    /**
     * Wait up to aTimeout for a respawn to replace the backend of generation aGeneration, covering the crash
     * detection before it, or until the calling thread's MDW_CANCEL_SCOPE is cancelled. True once another backend
     * runs, so a call the lost backend failed can be sent again. False at once when nothing will respawn this
     * session, as when the framework no longer holds it or its supervisor gave up.
     */
    bool AwaitRespawn( std::uint64_t aGeneration, std::chrono::milliseconds aTimeout );

private:
    void post( std::function<void()> aTask );

    void onBackendEvent( const MDW_EVENT& aEvent );

    /// Read every lock into the tracker on the worker
    void seedLocks();

    /// Re-read the backend's capabilities and announce CAPABILITIES_CHANGED once they are current
    void refreshCapabilities( int aTimeoutMs );

    /// aSent is the order the call was sent in
    void onOutcome( const MDW_RESULT& aResult, MDW_SEND_ORDER aSent );

    /// Send aCredentials, or run the server check when the backend asks for none. aAccount receives who the
    /// server accepted, and aSent the send order of the request that answered
    MDW_RESULT authenticate( const MDW_TYPED_CREDENTIALS& aCredentials, int aTimeoutMs, MDW_ACCOUNT& aAccount,
                             MDW_SEND_ORDER& aSent );

    /// Authenticate() and Reconnect(); aStored marks credentials the source can read again
    MDW_RESULT login( const MDW_TYPED_CREDENTIALS& aCredentials, bool aStored, int aTimeoutMs );

    /// aRead, or else what the credential source reads now, with no fields when the server already refused it
    MDW_TYPED_CREDENTIALS storedCredentials( std::optional<MDW_TYPED_CREDENTIALS> aRead = std::nullopt ) const;

    /// Remember a stored login aPhase refused, or forget the refusal once a login sent works. Hold m_statusMutex
    void judgeStored( const MDW_TYPED_CREDENTIALS& aStored, std::optional<MDW_CONNECTION> aPhase );

    /// The realm the backend's challenge names, read without m_statusMutex
    wxString challengeRealm() const;

    /**
     * The only writer of m_status. aChecked marks a server check or a login, which alone can prove the
     * server reachable. aAsked is the send order of the request that gave the answer, so an answer to an
     * older question never overrides one to a newer.
     */
    void apply( MDW_CONNECTION aPhase, const MDW_RESULT& aReason, MDW_SEND_ORDER aAsked,
                bool aChecked = false, const std::optional<MDW_ACCOUNT>& aAccount = std::nullopt );

    MDW_PROJECT_BINDING                     m_binding;
    std::unique_ptr<MDW_BACKEND_SUPERVISOR> m_supervisor;
    wxString                                m_sourceLocator;

    mutable std::mutex                     m_statusMutex;
    MDW_CONNECTION_STATUS                  m_status;               ///< Guarded by m_statusMutex
    bool                                   m_loginPending = false; ///< Guarded by m_statusMutex

    /// The send order behind the state now shown, so only a newer question can change it
    MDW_SEND_ORDER                         m_shownAsked = 0;       ///< Guarded by m_statusMutex

    bool                                   m_probeEarly = false;   ///< Guarded by m_statusMutex
    std::optional<std::size_t>             m_refusedLogin;         ///< Guarded by m_statusMutex
    wxString                               m_keyFile;              ///< Guarded by m_statusMutex
    MDW_CREDENTIALS*                       m_refusals = nullptr;
    std::chrono::steady_clock::time_point  m_nextProbe;            ///< Guarded by m_statusMutex
    int                                    m_probeDelayMs = 0;     ///< Guarded by m_statusMutex
    std::function<void()>                  m_onStatus;
    std::function<MDW_TYPED_CREDENTIALS()> m_credentialSource;

    std::shared_ptr<MDW_EVENT_BUS>          m_eventBus;
    std::shared_ptr<MDW_LOCK_TRACKER>       m_lockTracker;
    std::shared_ptr<MDW_DIRTY_TRACKER>      m_dirtyTracker;

    // Delivers backend events onto the bus from its own thread, so it is stopped
    // in the destructor before the bus goes.
    std::unique_ptr<MDW_EVENT_INGRESS>      m_ingress;
    std::atomic<bool>                       m_ingressRunning = false; ///< m_ingress, readable from any thread

    std::mutex                              m_serviceMutex;
    std::atomic<std::uint64_t>              m_backendGeneration = 0;
    std::shared_ptr<MDW_WORKSPACE_SERVICE>  m_service;

    std::mutex              m_respawnMutex;
    std::condition_variable m_respawned;
    bool                    m_respawnable = false; ///< Guarded by m_respawnMutex

    std::unique_ptr<MDW_SERIAL_WORKER>      m_worker;

    struct PROGRESS_WATCH
    {
        std::recursive_mutex     calling;
        MDW_PROGRESS_SCOPE::SINK sink;
        bool                     live = true;
    };

    struct PROGRESS_WATCHES
    {
        std::mutex                                         mutex;
        std::map<wxString, std::shared_ptr<PROGRESS_WATCH>> sinks;
    };

    std::shared_ptr<PROGRESS_WATCHES> m_progress = std::make_shared<PROGRESS_WATCHES>();
};


template <typename RESULT>
void MDW_SESSION::RunAsync( wxEvtHandler* aOwner, std::function<RESULT()> aWork, std::function<void( RESULT )> aDone )
{
    MDW_UI_COMPLETION::TICKET ticket = MDW_UI_COMPLETION::Register( aOwner );

    post( [ticket, work = std::move( aWork ), done = std::move( aDone )]()
          {
              RESULT result = work();

              MDW_UI_COMPLETION::Post( ticket,
                                       [done, result = std::move( result )]()
                                       {
                                           done( result );
                                       } );
          } );
}

#endif // MDW_SESSION_H
