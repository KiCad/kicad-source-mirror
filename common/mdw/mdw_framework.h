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
 * @file mdw_framework.h
 * @brief The application-level owner of the Managed Design Workspace runtime.
 *
 * The framework is constructed once in the project-manager process, beside the
 * API server, so backend processes survive editor frame recreation. It owns the
 * backend registry, the credentials, and one session per bound project.
 */

#ifndef MDW_FRAMEWORK_H
#define MDW_FRAMEWORK_H

#include <kicommon.h>
#include "mdw_backend_registry.h"
#include "mdw_credentials.h"
#include "mdw_session.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <thread>
#include <vector>

#include <wx/string.h>

class KICOMMON_API MDW_FRAMEWORK
{
public:
    MDW_FRAMEWORK();

    ~MDW_FRAMEWORK();

    MDW_BACKEND_REGISTRY& Registry() { return m_registry; }

    MDW_CREDENTIALS& Credentials() { return m_credentials; }

    using SESSION_PTR = std::shared_ptr<MDW_SESSION>;

    /**
     * Spawn a backend for a bound project and open its session. Blocking; must
     * run off the UI thread. Returns the session, or nullptr if the backend id
     * is unknown or the launch or handshake fails. A session already open for the
     * project is replaced, and listeners are told. aUsername is who the session
     * signs in as, empty for the server's default account.
     */
    SESSION_PTR OpenSession( const wxString& aProjectPath, const MDW_PROJECT_BINDING& aBinding, int aTimeoutMs,
                             const wxString& aUsername = wxEmptyString );

    /// How far opening a session has got, until there is one to ask
    enum class CONNECT_STATE
    {
        IDLE, ///< No attempt since the last close
        OPENING,
        BACKEND_STOPPED ///< The backend could not be started; EnsureSessionAsync() retries
    };

    /**
     * Open a session for the project on the worker thread unless one with the same binding
     * is open or opening, so every frame of a project can ask without starting duplicates.
     * Listeners are told when the attempt starts and when it ends. aUsername is who the session
     * signs in as, empty for the server's default account, and aKeyFile the private key it signs in
     * with, empty for the backend's default.
     */
    void EnsureSessionAsync( const wxString& aProjectPath, const MDW_PROJECT_BINDING& aBinding, int aTimeoutMs,
                             const wxString& aUsername = wxEmptyString, const wxString& aKeyFile = wxEmptyString );

    CONNECT_STATE ConnectState( const wxString& aProjectPath ) const;

    /**
     * The open session for a project. Holding the result keeps the session usable
     * after it is closed or replaced, so hold it only for one operation; observers
     * keep a weak_ptr and re-resolve when a listener fires.
     */
    SESSION_PTR Session( const wxString& aProjectPath ) const;

    /// The credential saved for the realm aSession's backend names, read only when it asks for a form
    MDW_TYPED_CREDENTIALS StoredCredentials( const MDW_SESSION& aSession ) const;

    /// Whether a stored login for aSession can be read now and its server has not refused it
    bool HasUnrefusedStoredLogin( const MDW_SESSION& aSession ) const;

    /**
     * Sign aProjectPath's session in again in place, first trusting the host key in aAnswer, with the
     * credentials in aAnswer or else the stored ones. Typed credentials in aAnswer are stored once the server
     * accepts them. Blocking; must not run on the UI thread.
     */
    MDW_RESULT Reconnect( const wxString& aProjectPath, const MDW_CONNECTION_ANSWER& aAnswer, int aTimeoutMs );

    /// Reconnect() on the worker thread
    void ReconnectAsync( const wxString& aProjectPath, const MDW_CONNECTION_ANSWER& aAnswer, int aTimeoutMs );

    /**
     * Close the session for aProjectPath and invalidate any open still pending for it. Without aWaitForExit
     * its backend shuts down in the background, since it may first finish work a cancelled request left
     * running. Shutdown() still waits for it.
     */
    void CloseSession( const wxString& aProjectPath, bool aWaitForExit = true );

    /// Every project path and library session key with a session open now
    std::vector<wxString> SessionPaths() const;

    /**
     * Register a callback run on the main thread whenever a project's session is
     * opened, replaced, restarted or closed. aOwner identifies the registration.
     */
    void AddSessionListener( const void* aOwner, std::function<void( const wxString& )> aListener );

    void RemoveSessionListener( const void* aOwner );

    /**
     * Look for out-of-process backends that exited on their own every aIntervalMs and restart them
     * on the worker under the supervisor's policy. Each outcome reaches the session listeners as a
     * status change. Zero stops watching.
     */
    void WatchBackends( int aIntervalMs, int aRestartTimeoutMs );

    void Shutdown();

private:
    /// Tell listeners about aProjectPath on the main thread.
    void notify( const wxString& aProjectPath );

    /**
     * A credential for aUsername was saved at aRealm, or with no username one was saved elsewhere. Every open
     * git pane and signed-out session on that server signing in as that user reads the store again.
     */
    void credentialSaved( const wxString& aRealm, const wxString& aUsername );

    /**
     * Bring a crashed session's backend back under the supervisor's restart policy. On success the
     * session has a fresh backend, is re-authenticated and receives events again. Worker only.
     */
    bool restartSession( const wxString& aProjectPath, const SESSION_PTR& aSession, int aTimeoutMs );

    void watchLoop();

    /// How long until the soonest reachability probe of any session, or -1 while none can be probed
    std::chrono::milliseconds nextProbeDue() const;

    /// On the worker, so every supervisor inspection is serialized with the restarts
    void pollBackends();

    void restartCrashed( const wxString& aProjectPath, const SESSION_PTR& aSession );

    /// Call with m_watchControl held
    void stopWatching();

    bool watching();

    /// The project's open epoch. Closing the project or changing its binding moves it on.
    std::uint64_t openEpoch( const wxString& aProjectPath ) const;

    /// m_lifetime, which Shutdown() resets while leaseholders may still read it
    std::weak_ptr<int> lifetimeToken();

    /// aCall on aSession's backend, skipped once Shutdown() has begun, which stops the backend under one in flight
    void callBackend( const SESSION_PTR& aSession, const std::function<void()>& aCall );

    /// OpenSession() that publishes only while aProjectPath is still at aEpoch.
    SESSION_PTR openSession( const wxString& aProjectPath, const MDW_PROJECT_BINDING& aBinding, int aTimeoutMs,
                             std::uint64_t aEpoch, const wxString& aUsername,
                             const wxString& aKeyFile = wxEmptyString );

    MDW_BACKEND_REGISTRY                                        m_registry;
    MDW_CREDENTIALS                                             m_credentials;
    std::map<wxString, SESSION_PTR>                             m_sessions;
    std::map<wxString, CONNECT_STATE>                           m_connectStates;
    std::map<wxString, MDW_PROJECT_BINDING>                     m_openingBindings;
    std::map<wxString, std::uint64_t>                           m_openEpochs;
    std::vector<std::future<void>>                              m_closing; ///< Closes in progress, guarded by m_mutex
    std::multiset<SESSION_PTR>                                  m_calling; ///< Guarded by m_mutex
    mutable std::mutex                                          m_mutex;

    std::mutex                                                  m_listenerMutex;
    std::map<const void*, std::function<void( const wxString& )>> m_listeners;

    std::mutex                                                  m_watchControl;   ///< Serializes start, stop and shutdown
    bool                                                        m_watchClosed = false; ///< Guarded by m_watchControl
    std::mutex                                                  m_watchMutex;
    std::condition_variable                                     m_watchWake;
    std::thread                                                 m_watchThread;
    int                                                         m_watchIntervalMs = 0;  ///< Guarded by m_watchMutex
    int                                                         m_watchTimeoutMs = 0;   ///< Guarded by m_watchMutex
    bool                                                        m_pollQueued = false;   ///< Guarded by m_watchMutex
    bool                                                        m_probeWake = false;    ///< Guarded by m_watchMutex

    /// Sessions whose last restart failed short of giving up. A failed respawn leaves no process to
    /// detect as crashed, so these are retried without waiting for one. Worker only.
    std::map<wxString, std::weak_ptr<MDW_SESSION>>              m_retryPending;

    // Expires with the framework, so a queued listener notification is skipped after it. Guarded by
    // m_listenerMutex
    std::shared_ptr<int>                                        m_lifetime = std::make_shared<int>();

    // Last member, so it is joined before the sessions and registries it touches go. Null once Shutdown() has
    // run, and read under m_mutex by all but the watch, which Shutdown() stops first
    std::unique_ptr<MDW_SERIAL_WORKER>                          m_worker;
};

#endif // MDW_FRAMEWORK_H
