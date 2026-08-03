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

#include <mdw/mdw_framework.h>
#include <mdw/mdw_backend_supervisor.h>
#include <mdw/mdw_proto_convert.h>
#include <git/kicad_git_common.h>
#include <git/kigit_platform.h>

#include <api/mdw/backend_base.pb.h>

#include <chrono>
#include <future>
#include <optional>
#include <system_error>
#include <vector>

#include <wx/app.h>


MDW_FRAMEWORK::MDW_FRAMEWORK() :
        m_worker( std::make_unique<MDW_SERIAL_WORKER>() )
{
    m_registry.Discover();

    m_credentials.SetSaveListener(
            [this]( const wxString& aRealm, const wxString& aUsername )
            {
                credentialSaved( aRealm, aUsername );
            } );
}


MDW_FRAMEWORK::~MDW_FRAMEWORK()
{
    Shutdown();
}


std::uint64_t MDW_FRAMEWORK::openEpoch( const wxString& aProjectPath ) const
{
    std::lock_guard<std::mutex> lock( m_mutex );
    auto                        it = m_openEpochs.find( aProjectPath );

    return it == m_openEpochs.end() ? 0 : it->second;
}


MDW_FRAMEWORK::SESSION_PTR MDW_FRAMEWORK::OpenSession( const wxString&            aProjectPath,
                                                       const MDW_PROJECT_BINDING& aBinding, int aTimeoutMs,
                                                       const wxString& aUsername )
{
    return openSession( aProjectPath, aBinding, aTimeoutMs, openEpoch( aProjectPath ), aUsername );
}


MDW_FRAMEWORK::SESSION_PTR MDW_FRAMEWORK::openSession( const wxString&            aProjectPath,
                                                       const MDW_PROJECT_BINDING& aBinding, int aTimeoutMs,
                                                       std::uint64_t aEpoch, const wxString& aUsername,
                                                       const wxString& aKeyFile )
{
    if( !aBinding.IsBound() )
        return nullptr;

    {
        std::lock_guard<std::mutex> lock( m_mutex );

        if( !m_worker )
            return nullptr;
    }

    const MDW_BACKEND_MANIFEST* manifest = m_registry.Find( aBinding.backend_id );

    if( !manifest )
        return nullptr;

    MDW_BACKEND_CONTEXT context;
    context.projectPath = aProjectPath;
    context.workspaceId = aBinding.workspace_id;
    context.locator = aBinding.binding;

    auto supervisor = std::make_unique<MDW_BACKEND_SUPERVISOR>( *manifest, context );

    if( !supervisor->Launch( aTimeoutMs ) )
        return nullptr;

    auto session = std::make_shared<MDW_SESSION>( aBinding, std::move( supervisor ) );
    session->SetUsername( aUsername );
    session->SetKeyFile( aKeyFile );

    // Runs on a transport thread, so the work goes to the main thread, where the framework is destroyed
    session->SetStatusListener(
            [this, lifetime = lifetimeToken(), aProjectPath]()
            {
                if( !wxTheApp || lifetime.expired() )
                    return;

                wxTheApp->CallAfter(
                        [this, lifetime, aProjectPath]()
                        {
                            if( lifetime.expired() )
                                return;

                            {
                                std::lock_guard<std::mutex> lock( m_watchMutex );
                                m_probeWake = true;
                            }

                            m_watchWake.notify_all();
                            notify( aProjectPath );
                        } );
            } );

    // Authenticate before publishing so nothing borrows an anonymous session; a failed login stays
    // open so the UI can report it and offer Configure
    session->SetCredentialSource(
            [this, weak = std::weak_ptr<MDW_SESSION>( session )]()
            {
                std::shared_ptr<MDW_SESSION> held = weak.lock();
                return held ? StoredCredentials( *held ) : MDW_TYPED_CREDENTIALS();
            } );
    session->SetRefusalRecord( &m_credentials );

    callBackend( session,
                 [&]()
                 {
                     session->Reconnect( aTimeoutMs );
                 } );

    session->StartEventIngress();

    SESSION_PTR previous;

    {
        std::lock_guard<std::mutex> lock( m_mutex );
        auto                        epoch = m_openEpochs.find( aProjectPath );

        // The project closed or was rebound while this launched, or the framework shut down, so the session is
        // already obsolete
        if( !m_worker || ( epoch == m_openEpochs.end() ? 0 : epoch->second ) != aEpoch )
            previous = std::move( session );
        else if( auto it = m_sessions.find( aProjectPath ); it != m_sessions.end() )
            previous = std::move( it->second );

        if( session )
        {
            m_sessions[aProjectPath] = session;
            session->SetRespawnable( true );
        }
    }

    if( previous )
        previous->SetRespawnable( false );

    if( !session )
        return nullptr;

    // Released outside the lock, since backend teardown can take seconds
    previous.reset();
    notify( aProjectPath );

    return session;
}


void MDW_FRAMEWORK::EnsureSessionAsync( const wxString& aProjectPath, const MDW_PROJECT_BINDING& aBinding,
                                        int aTimeoutMs, const wxString& aUsername, const wxString& aKeyFile )
{
    if( !aBinding.IsBound() )
        return;

    std::uint64_t epoch;

    {
        std::lock_guard<std::mutex> lock( m_mutex );
        auto                        it = m_sessions.find( aProjectPath );

        if( !m_worker || ( it != m_sessions.end() && it->second->Binding() == aBinding ) )
            return;

        CONNECT_STATE& state = m_connectStates[aProjectPath];

        if( state == CONNECT_STATE::OPENING )
        {
            if( m_openingBindings[aProjectPath] == aBinding )
                return;

            // The binding changed under a running open, so that open must not publish
            ++m_openEpochs[aProjectPath];
        }

        state = CONNECT_STATE::OPENING;
        m_openingBindings[aProjectPath] = aBinding;
        epoch = m_openEpochs[aProjectPath];
    }

    notify( aProjectPath );

    std::lock_guard<std::mutex> lock( m_mutex );

    if( !m_worker )
        return;

    m_worker->Post(
            [this, aProjectPath, aBinding, aTimeoutMs, epoch, aUsername, aKeyFile]()
            {
                SESSION_PTR existing = Session( aProjectPath );

                // A synchronous open may have won the race since this was queued
                SESSION_PTR session =
                        existing && existing->Binding() == aBinding
                                ? existing
                                : openSession( aProjectPath, aBinding, aTimeoutMs, epoch, aUsername, aKeyFile );

                {
                    std::lock_guard<std::mutex> guard( m_mutex );
                    auto                        current = m_openEpochs.find( aProjectPath );

                    if( ( current == m_openEpochs.end() ? 0 : current->second ) != epoch )
                        return;

                    m_connectStates[aProjectPath] = session ? CONNECT_STATE::IDLE : CONNECT_STATE::BACKEND_STOPPED;
                    m_openingBindings.erase( aProjectPath );
                }

                notify( aProjectPath );
            } );
}


MDW_FRAMEWORK::CONNECT_STATE MDW_FRAMEWORK::ConnectState( const wxString& aProjectPath ) const
{
    std::lock_guard<std::mutex> lock( m_mutex );
    auto                        it = m_connectStates.find( aProjectPath );

    return it == m_connectStates.end() ? CONNECT_STATE::IDLE : it->second;
}


MDW_FRAMEWORK::SESSION_PTR MDW_FRAMEWORK::Session( const wxString& aProjectPath ) const
{
    std::lock_guard<std::mutex> lock( m_mutex );
    auto                        it = m_sessions.find( aProjectPath );

    return it == m_sessions.end() ? nullptr : it->second;
}


MDW_TYPED_CREDENTIALS MDW_FRAMEWORK::StoredCredentials( const MDW_SESSION& aSession ) const
{
    // A workspace whose Hello names no form has no realm, so the keychain stays quiet
    const MDW_CONNECTION_STATUS status = aSession.Status();

    if( status.realm.IsEmpty() )
        return MDW_TYPED_CREDENTIALS();

    return m_credentials.Find( status.realm, status.username ).value_or( MDW_TYPED_CREDENTIALS() );
}


bool MDW_FRAMEWORK::HasUnrefusedStoredLogin( const MDW_SESSION& aSession ) const
{
    const MDW_CONNECTION_STATUS          status = aSession.Status();
    std::optional<MDW_TYPED_CREDENTIALS> stored = m_credentials.Find( status.realm, status.username );

    return stored && !aSession.StoredLoginRefused( *stored );
}


void MDW_FRAMEWORK::credentialSaved( const wxString& aRealm, const wxString& aUsername )
{
    KIGIT_COMMON::CredentialSaved( aRealm, aUsername );

    std::vector<SESSION_PTR> sessions;

    {
        std::lock_guard<std::mutex> lock( m_mutex );

        for( const auto& [path, session] : m_sessions )
            sessions.push_back( session );
    }

    const KIGIT::PLATFORM_SERVICES& services = KIGIT::PlatformServices();
    bool                            woken = false;

    for( const SESSION_PTR& session : sessions )
    {
        const MDW_CONNECTION_STATUS status = session->Status();
        wxString                    account = status.username;

        if( status.realm != aRealm )
            continue;

        if( account.IsEmpty() && services.getAccount )
            account = services.getAccount( aRealm );

        // A save for another user leaves this one's refused login alone
        if( aUsername.IsEmpty() || aUsername == account )
            woken |= session->StoredLoginChanged();
    }

    if( !woken )
        return;

    {
        std::lock_guard<std::mutex> lock( m_watchMutex );
        m_probeWake = true;
    }

    m_watchWake.notify_all();
}


MDW_RESULT MDW_FRAMEWORK::Reconnect( const wxString& aProjectPath, const MDW_CONNECTION_ANSWER& aAnswer,
                                     int aTimeoutMs )
{
    SESSION_PTR                  session = Session( aProjectPath );
    std::shared_ptr<MDW_BACKEND> backend = session ? session->Backend() : nullptr;

    if( !backend )
        return MDW_RESULT::Fail( MDW_ERROR_CODE::BACKEND_LOST );

    // Only the backend whose connection met the key may trust it
    if( aAnswer.trust )
    {
        kiapi::mdw::TrustHostKeyRequest request;
        kiapi::mdw::TrustHostKeyReply   reply;
        MdwToProto( request.mutable_host_key(), *aAnswer.trust );

        if( MDW_RESULT trusted = backend->CallSync( request, reply, aTimeoutMs ); !trusted.IsOk() )
            return trusted;
    }

    if( aAnswer.credentials )
        return m_credentials.SignIn( *session, *aAnswer.credentials, aTimeoutMs );

    return session->Reconnect( aTimeoutMs, aAnswer.stored );
}


void MDW_FRAMEWORK::ReconnectAsync( const wxString& aProjectPath, const MDW_CONNECTION_ANSWER& aAnswer, int aTimeoutMs )
{
    std::lock_guard<std::mutex> lock( m_mutex );

    if( !m_worker )
        return;

    m_worker->Post(
            [this, aProjectPath, aAnswer, aTimeoutMs]()
            {
                if( SESSION_PTR session = Session( aProjectPath ) )
                {
                    callBackend( session,
                                 [&]()
                                 {
                                     Reconnect( aProjectPath, aAnswer, aTimeoutMs );
                                 } );
                }
            } );
}


void MDW_FRAMEWORK::CloseSession( const wxString& aProjectPath, bool aWaitForExit )
{
    SESSION_PTR session;

    {
        std::lock_guard<std::mutex> lock( m_mutex );
        auto                        it = m_sessions.find( aProjectPath );

        if( it != m_sessions.end() )
        {
            session = std::move( it->second );
            m_sessions.erase( it );
            session->SetRespawnable( false );
        }

        m_connectStates.erase( aProjectPath );
        m_openingBindings.erase( aProjectPath );
        ++m_openEpochs[aProjectPath];

        std::erase_if( m_closing,
                       []( const std::future<void>& aClosing )
                       {
                           return aClosing.wait_for( std::chrono::seconds( 0 ) ) == std::future_status::ready;
                       } );
    }

    if( session && !aWaitForExit )
    {
        // A task that cannot start drops the session here instead, as a wait would
        try
        {
            std::future<void> closing = std::async( std::launch::async,
                                                    [moved = std::move( session )]() mutable
                                                    {
                                                        moved.reset();
                                                    } );

            std::lock_guard<std::mutex> lock( m_mutex );
            m_closing.push_back( std::move( closing ) );
        }
        catch( const std::system_error& )
        {
        }
    }

    // Released outside the lock, so a slow backend shutdown does not block other lookups
    session.reset();
    notify( aProjectPath );
}


std::vector<wxString> MDW_FRAMEWORK::SessionPaths() const
{
    std::lock_guard<std::mutex> lock( m_mutex );
    std::vector<wxString>       paths;

    for( const auto& [path, session] : m_sessions )
        paths.push_back( path );

    return paths;
}


void MDW_FRAMEWORK::callBackend( const SESSION_PTR& aSession, const std::function<void()>& aCall )
{
    {
        std::lock_guard<std::mutex> lock( m_mutex );

        if( !m_worker )
            return;

        m_calling.insert( aSession );
    }

    aCall();

    std::lock_guard<std::mutex> lock( m_mutex );
    m_calling.erase( m_calling.find( aSession ) );
}


bool MDW_FRAMEWORK::restartSession( const wxString& aProjectPath, const SESSION_PTR& aSession, int aTimeoutMs )
{
    if( !aSession || !aSession->Supervisor() )
        return false;

    aSession->DiscardBackendState();

    bool restarted = aSession->Supervisor()->Restart( aTimeoutMs );

    if( restarted )
    {
        callBackend( aSession,
                     [&]()
                     {
                         aSession->Reconnect( aTimeoutMs );
                     } );
        aSession->StartEventIngress();
    }

    // A listener that sees the new generation must find the backend ready to use
    aSession->BackendReplaced();
    notify( aProjectPath );

    return restarted;
}


void MDW_FRAMEWORK::AddSessionListener( const void* aOwner,
                                        std::function<void( const wxString& )> aListener )
{
    std::lock_guard<std::mutex> lock( m_listenerMutex );
    m_listeners[aOwner] = std::move( aListener );
}


void MDW_FRAMEWORK::RemoveSessionListener( const void* aOwner )
{
    std::lock_guard<std::mutex> lock( m_listenerMutex );
    m_listeners.erase( aOwner );
}


std::weak_ptr<int> MDW_FRAMEWORK::lifetimeToken()
{
    std::lock_guard<std::mutex> lock( m_listenerMutex );
    return m_lifetime;
}


void MDW_FRAMEWORK::notify( const wxString& aProjectPath )
{
    if( !wxTheApp )
        return;

    // Listeners are looked up when the call runs, so one removed in the meantime is skipped
    std::weak_ptr<int> lifetime = lifetimeToken();

    wxTheApp->CallAfter(
            [this, lifetime, aProjectPath]()
            {
                if( lifetime.expired() )
                    return;

                std::vector<std::function<void( const wxString& )>> listeners;

                {
                    std::lock_guard<std::mutex> lock( m_listenerMutex );

                    for( const auto& [owner, listener] : m_listeners )
                        listeners.push_back( listener );
                }

                for( const auto& listener : listeners )
                    listener( aProjectPath );
            } );
}


void MDW_FRAMEWORK::WatchBackends( int aIntervalMs, int aRestartTimeoutMs )
{
    std::lock_guard<std::mutex> control( m_watchControl );

    stopWatching();

    // A listener running while the framework shuts down must not start a thread nobody joins
    if( aIntervalMs <= 0 || m_watchClosed )
        return;

    {
        std::lock_guard<std::mutex> lock( m_watchMutex );
        m_watchIntervalMs = aIntervalMs;
        m_watchTimeoutMs = aRestartTimeoutMs;
    }

    m_watchThread = std::thread(
            [this]()
            {
                watchLoop();
            } );
}


void MDW_FRAMEWORK::stopWatching()
{
    {
        std::lock_guard<std::mutex> lock( m_watchMutex );
        m_watchIntervalMs = 0;
    }

    m_watchWake.notify_all();

    if( m_watchThread.joinable() )
        m_watchThread.join();
}


bool MDW_FRAMEWORK::watching()
{
    std::lock_guard<std::mutex> lock( m_watchMutex );
    return m_watchIntervalMs > 0;
}


std::chrono::milliseconds MDW_FRAMEWORK::nextProbeDue() const
{
    std::chrono::milliseconds due( -1 );
    std::lock_guard<std::mutex> lock( m_mutex );

    for( const auto& [path, session] : m_sessions )
    {
        std::chrono::milliseconds in = session ? session->ProbeDueIn() : std::chrono::milliseconds( -1 );

        if( in.count() >= 0 && ( due.count() < 0 || in < due ) )
            due = in;
    }

    return due;
}


void MDW_FRAMEWORK::watchLoop()
{
    std::unique_lock<std::mutex> lock( m_watchMutex );

    while( m_watchIntervalMs > 0 )
    {
        // A worker held up by a backoff or a slow probe must not collect a queue of polls behind it
        m_watchWake.wait( lock,
                          [this]()
                          {
                              return m_watchIntervalMs <= 0 || !m_pollQueued;
                          } );

        if( m_watchIntervalMs <= 0 )
            break;

        std::chrono::milliseconds wait( m_watchIntervalMs );
        m_probeWake = false;

        // Released while the sessions are read, so the two locks are never nested
        lock.unlock();
        std::chrono::milliseconds probe = nextProbeDue();
        lock.lock();

        // A stop that came while unlocked has already notified
        if( m_watchIntervalMs <= 0 )
            break;

        if( probe.count() >= 0 && probe < wait )
            wait = probe;

        // A flag, since a deadline moved while unlocked has already notified
        bool woken = m_watchWake.wait_for( lock, wait,
                                           [this]()
                                           {
                                               return m_watchIntervalMs <= 0 || m_probeWake;
                                           } );

        if( m_watchIntervalMs <= 0 || !m_worker )
            break;

        if( woken )
            continue;

        m_pollQueued = true;

        m_worker->Post(
                [this]()
                {
                    pollBackends();

                    {
                        std::lock_guard<std::mutex> done( m_watchMutex );
                        m_pollQueued = false;
                    }

                    m_watchWake.notify_all();
                } );
    }
}


void MDW_FRAMEWORK::pollBackends()
{
    if( !watching() )
        return;

    std::map<wxString, SESSION_PTR> sessions;
    int                             probeTimeoutMs = 0;

    {
        std::lock_guard<std::mutex> lock( m_mutex );
        sessions = m_sessions;
    }

    {
        std::lock_guard<std::mutex> lock( m_watchMutex );
        probeTimeoutMs = m_watchTimeoutMs;
    }

    for( const auto& [path, session] : sessions )
    {
        // Shutdown may have stopped the watch while an earlier session restarted
        if( !watching() )
            return;

        // A reachable workspace is asked only after its server has been quiet for the heartbeat
        if( session && session->ProbeDueIn().count() == 0 )
            callBackend( session,
                         [&]()
                         {
                             session->Probe( probeTimeoutMs );
                         } );

        MDW_BACKEND_SUPERVISOR* supervisor = session ? session->Supervisor() : nullptr;

        // A supervisor that gave up stays down until the project is reopened with a new one
        if( !supervisor || supervisor->GaveUp() )
            continue;

        // A restart from elsewhere may have recovered it since, and an online backend is never retried
        auto pending = m_retryPending.find( path );
        bool retry = pending != m_retryPending.end() && pending->second.lock() == session && !supervisor->Online();

        if( !retry && !supervisor->CrashDetected() )
            continue;

        restartCrashed( path, session );
    }
}


void MDW_FRAMEWORK::restartCrashed( const wxString& aProjectPath, const SESSION_PTR& aSession )
{
    int timeoutMs = 0;

    {
        std::lock_guard<std::mutex> lock( m_watchMutex );
        timeoutMs = m_watchTimeoutMs;
    }

    // A session closed or replaced since the poll belongs to nobody, so it is left alone
    if( Session( aProjectPath ) != aSession )
    {
        m_retryPending.erase( aProjectPath );
        return;
    }

    MDW_BACKEND_SUPERVISOR* supervisor = aSession->Supervisor();
    bool                    restarted = restartSession( aProjectPath, aSession, timeoutMs );

    if( restarted || supervisor->GaveUp() )
        m_retryPending.erase( aProjectPath );
    else
        m_retryPending[aProjectPath] = aSession;
}


void MDW_FRAMEWORK::Shutdown()
{
    // The watch posts to the worker, so it stops first and stays stopped
    {
        std::lock_guard<std::mutex> control( m_watchControl );
        m_watchClosed = true;
        stopWatching();
    }

    std::unique_ptr<MDW_SERIAL_WORKER> worker;
    std::vector<SESSION_PTR>           calling;

    // Leaseholders may still call in, and a missing worker tells them the framework is shut down
    {
        std::lock_guard<std::mutex> lock( m_mutex );
        worker = std::move( m_worker );
        calling.assign( m_calling.begin(), m_calling.end() );
    }

    // A login or probe can wait out an unreachable server, so its transport is stopped under it
    for( const SESSION_PTR& session : calling )
    {
        if( std::shared_ptr<MDW_BACKEND> backend = session->Backend() )
            backend->Stop();
    }

    // Join any open or restart in progress before tearing down what it touches
    worker.reset();

    {
        std::lock_guard<std::mutex> lock( m_listenerMutex );
        m_lifetime.reset();
    }

    std::map<wxString, SESSION_PTR> sessions;

    {
        std::lock_guard<std::mutex> lock( m_mutex );
        sessions.swap( m_sessions );
    }

    std::vector<MDW_BACKEND_SUPERVISOR*> supervisors;

    for( const auto& [path, session] : sessions )
    {
        if( session && session->Supervisor() )
        {
            session->SetRespawnable( false );
            session->DetachBackend();
            supervisors.push_back( session->Supervisor() );
        }
    }

    // Together, since one at a time each backend's exit would add to the quit
    MDW_BACKEND_SUPERVISOR::ShutdownAll( supervisors );

    sessions.clear();

    std::vector<std::future<void>> closing;

    {
        std::lock_guard<std::mutex> lock( m_mutex );
        closing.swap( m_closing );
    }

    for( std::future<void>& pending : closing )
        pending.wait();
}
