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
 * @file mdw_backend_supervisor.h
 * @brief Owns the lifecycle of one spawned backend process.
 *
 * The supervisor mints the per-spawn socket urls and channel token, launches the process, drives the handshake,
 * notices a process that died on its own and tears it down on shutdown. The restart budget belongs to the
 * supervisor, so a user's Reconnect, which opens a new session, is the way back after a give-up.
 *
 * A supervisor is driven from a single control thread; its methods are not synchronized against one another.
 */

#ifndef MDW_BACKEND_SUPERVISOR_H
#define MDW_BACKEND_SUPERVISOR_H

#include <kicommon.h>
#include "mdw_backend.h"
#include "mdw_backend_registry.h"
#include "mdw_restart_policy.h"

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

#include <wx/string.h>


/// What a backend is told about its project when it launches
struct MDW_BACKEND_CONTEXT
{
    wxString projectPath;
    wxString workspaceId;     ///< What Hello.workspace tells the backend
    wxString locator;
};

class KICOMMON_API MDW_BACKEND_SUPERVISOR
{
public:
    MDW_BACKEND_SUPERVISOR( const MDW_BACKEND_MANIFEST& aManifest,
                            const MDW_BACKEND_CONTEXT& aContext = MDW_BACKEND_CONTEXT() );

    ~MDW_BACKEND_SUPERVISOR();

    /**
     * Spawn the backend and complete the handshake. Blocking; must not run on the
     * UI thread. Returns true and makes Backend() available on success.
     */
    bool Launch( int aHandshakeTimeoutMs );

    /// Send Bye then terminate the process, escalating SIGTERM to SIGKILL.
    void Shutdown() { ShutdownAll( { this } ); }

    /// Shutdown() for several supervisors at once, so the wait does not grow with their number
    static void ShutdownAll( const std::vector<MDW_BACKEND_SUPERVISOR*>& aSupervisors );

    /**
     * Reap the spawned process if it has exited and report whether that exit was
     * unexpected. Shutdown() is the only exit this supervisor asks for, so a
     * process gone without one has crashed and the owner should either Restart()
     * or treat the session as offline. Non-blocking, and cheap enough to poll.
     */
    bool CrashDetected();

    /**
     * Respawn after CrashDetected(), waiting out the backoff MDW_RESTART_POLICY
     * schedules for this attempt. Returns false when the policy has given up on
     * the backend or the fresh handshake fails, either of which leaves the
     * supervisor offline.
     *
     * A relaunch replaces MDW_BACKEND and moves the runtime directory, so on
     * success every object the owner derived from Backend(), IpcUrl() or
     * ChannelToken() is stale and must be rebuilt. Blocking; must not run on the
     * UI thread.
     */
    bool Restart( int aHandshakeTimeoutMs );

    /// True once the restart policy refused a further attempt, which is
    /// permanent for this supervisor.
    bool GaveUp() const { return m_gaveUp; }

    bool Ping( int aTimeoutMs );

    /// The running backend, or null while none is published. Safe from any thread.
    std::shared_ptr<MDW_BACKEND> Backend() const;

    bool Online() const { return m_online.load(); }

    /**
     * Receives the result of every typed call a backend of this supervisor answered, this one and any
     * respawn, on the completing thread. Clearing it waits out a delivery in flight.
     */
    void SetOutcomeListener( MDW_OUTCOME_HOOK aListener );

    const wxString& ChannelToken() const { return m_channelToken; }

    const MDW_BACKEND_MANIFEST& Manifest() const { return m_manifest; }

    /// The ipc:// url of aSocketName in the 0700 per-spawn runtime directory, empty while no backend process exists
    wxString IpcUrl( const wxString& aSocketName ) const;

private:
    /// Create the private per-launch directory and mint the channel token.
    bool makeRuntimeDir();

    bool spawnProcess();

    /// Stop the child and remove its runtime directory.
    void terminateProcess() { terminateAll( { this } ); }

    /// Stop each child, escalating SIGTERM to SIGKILL, and remove its runtime directory
    static void terminateAll( const std::vector<MDW_BACKEND_SUPERVISOR*>& aSupervisors );

    /// Reap every child that exits within aTimeout
    static void awaitExit( const std::vector<MDW_BACKEND_SUPERVISOR*>& aSupervisors,
                           std::chrono::milliseconds                   aTimeout );

    /// Collect the child if it has already exited, clearing m_pid so nothing
    /// waits on it twice. False while it is still running.
    bool reapIfExited();

    std::shared_ptr<MDW_BACKEND> exchangeBackend( std::shared_ptr<MDW_BACKEND> aBackend );

    kiapi::mdw::Hello makeHello();

    MDW_BACKEND_MANIFEST          m_manifest;
    MDW_BACKEND_CONTEXT           m_context;
    wxString                      m_controlUrl;
    wxString                      m_channelToken;
    wxString                      m_runtimeDir;   ///< Guarded by m_backendMutex
    long                          m_pid = 0;
    std::atomic<bool>             m_online{ false };
    mutable std::mutex            m_backendMutex;
    std::shared_ptr<MDW_BACKEND>  m_backend;      ///< Guarded by m_backendMutex
    MDW_RESTART_POLICY            m_restartPolicy;
    bool                          m_shutdownRequested = false;
    bool                          m_gaveUp = false;

    /// Shared with every backend's hook, so the listener can change after the backend started
    struct OUTCOME_RELAY
    {
        std::mutex       mutex;
        MDW_OUTCOME_HOOK listener;
    };

    std::shared_ptr<OUTCOME_RELAY> m_outcomeRelay = std::make_shared<OUTCOME_RELAY>();

    MDW_OUTCOME_HOOK outcomeHook() const;
};

#endif // MDW_BACKEND_SUPERVISOR_H
