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

#include <mdw/mdw_session.h>
#include <mdw/mdw_credentials.h>
#include <mdw/client/mdw_library_sources.h>
#include <mdw/mdw_event_ingress.h>
#include <mdw/mdw_proto_convert.h>

#include <kinng.h>

#include <api/mdw/backend_base.pb.h>

#include <wx/filename.h>
#include <wx/intl.h>
#include <wx/datetime.h>

#include <algorithm>
#include <functional>
#include <string>


namespace
{

constexpr int FIRST_PROBE_MS = 2000;
constexpr int LAST_PROBE_MS = 60000;

/// How long a reachable server may stay quiet before a probe asks it again
constexpr std::chrono::milliseconds HEARTBEAT( 60000 );

/// How long a re-read of the capabilities a backend announced may take
constexpr int REFRESH_TIMEOUT_MS = 10000;

/// The session whose server check runs on this thread, which applies its own answer
thread_local const MDW_SESSION* t_checking = nullptr;

struct CHECKING
{
    explicit CHECKING( const MDW_SESSION* aSession ) :
            m_previous( t_checking )
    {
        t_checking = aSession;
    }

    ~CHECKING() { t_checking = m_previous; }

    const MDW_SESSION* m_previous;
};


/// The phase aResult shows. Only a check or a login proves the server reachable, since a backend answers some
/// ordinary calls from its own cache
std::optional<MDW_CONNECTION> phaseOf( const MDW_RESULT& aResult, bool aChecked )
{
    switch( aResult.IsOk() ? MDW_ERROR_CODE::SUCCESS : aResult.error )
    {
    case MDW_ERROR_CODE::NETWORK:
    case MDW_ERROR_CODE::TIMEOUT: return MDW_CONNECTION::OFFLINE;
    case MDW_ERROR_CODE::NOT_AUTHENTICATED:
    case MDW_ERROR_CODE::AUTH_FAILED: return MDW_CONNECTION::SIGN_IN_NEEDED;
    case MDW_ERROR_CODE::UNTRUSTED_HOST: return MDW_CONNECTION::TRUST_NEEDED;
    case MDW_ERROR_CODE::BACKEND_LOST:
        return aChecked ? std::optional( MDW_CONNECTION::BACKEND_RESTARTING ) : std::nullopt;
    case MDW_ERROR_CODE::CANCELLED: return std::nullopt;

    // A backend too old to check its server never recovers by asking again
    case MDW_ERROR_CODE::PROTOCOL: return aChecked ? std::optional( MDW_CONNECTION::BACKEND_STOPPED ) : std::nullopt;
    case MDW_ERROR_CODE::SUCCESS: return aChecked ? std::optional( MDW_CONNECTION::ONLINE ) : std::nullopt;

    // A check that failed otherwise, such as one a stale backend cannot make, never reached the server
    default: return aChecked ? std::optional( MDW_CONNECTION::OFFLINE ) : std::nullopt;
    }
}

} // namespace


MDW_SESSION::MDW_SESSION( const MDW_PROJECT_BINDING& aBinding, std::unique_ptr<MDW_BACKEND_SUPERVISOR> aSupervisor ) :
        m_binding( aBinding ),
        m_supervisor( std::move( aSupervisor ) ),
        m_sourceLocator( m_supervisor ? SourceLocatorOf( aBinding.binding, m_supervisor->Manifest() )
                                      : aBinding.binding ),
        m_eventBus( std::make_shared<MDW_EVENT_BUS>() ),
        m_lockTracker( std::make_shared<MDW_LOCK_TRACKER>() ),
        m_dirtyTracker( std::make_shared<MDW_DIRTY_TRACKER>() ),
        m_worker( std::make_unique<MDW_SERIAL_WORKER>() )
{
    std::weak_ptr<MDW_LOCK_TRACKER> tracker = m_lockTracker;

    m_eventBus->SetAcceptHook(
            [tracker]( const MDW_EVENT& aEvent )
            {
                if( std::shared_ptr<MDW_LOCK_TRACKER> locks = tracker.lock() )
                    locks->ApplyEvent( aEvent );
            } );

    m_status.phase = MDW_CONNECTION::CONNECTING;
    m_nextProbe = std::chrono::steady_clock::now() + HEARTBEAT;

    if( m_supervisor )
    {
        m_supervisor->SetOutcomeListener(
                [this]( const MDW_RESULT& aResult, MDW_SEND_ORDER aSent )
                {
                    onOutcome( aResult, aSent );
                } );
    }
}


void MDW_SESSION::SetStatusListener( std::function<void()> aListener )
{
    m_onStatus = std::move( aListener );
}


void MDW_SESSION::SetCredentialSource( std::function<MDW_TYPED_CREDENTIALS()> aSource )
{
    m_credentialSource = std::move( aSource );
}


void MDW_SESSION::SetRefusalRecord( MDW_CREDENTIALS* aRecord )
{
    m_refusals = aRecord;
}


void MDW_SESSION::SetUsername( const wxString& aUsername )
{
    std::lock_guard<std::mutex> lock( m_statusMutex );
    m_status.username = aUsername;
}


void MDW_SESSION::SetKeyFile( const wxString& aKeyFile )
{
    std::lock_guard<std::mutex> lock( m_statusMutex );
    m_keyFile = aKeyFile;
}


MDW_CONNECTION_STATUS MDW_SESSION::Status() const
{
    MDW_CONNECTION_STATUS status;

    {
        std::lock_guard<std::mutex> lock( m_statusMutex );
        status = m_status;
    }

    if( std::shared_ptr<MDW_BACKEND> backend = Backend(); backend && backend->AuthChallenge() )
        status.realm = backend->AuthChallenge()->realm;

    return status;
}


wxString MDW_SESSION::challengeRealm() const
{
    std::shared_ptr<MDW_BACKEND> backend = Backend();

    return backend && backend->AuthChallenge() ? backend->AuthChallenge()->realm : wxString();
}


void MDW_SESSION::apply( MDW_CONNECTION aPhase, const MDW_RESULT& aReason, MDW_SEND_ORDER aAsked,
                         bool aChecked, const std::optional<MDW_ACCOUNT>& aAccount )
{
    bool tell = false;

    {
        std::lock_guard<std::mutex> lock( m_statusMutex );
        const auto                  now = std::chrono::steady_clock::now();
        const MDW_CONNECTION        was = m_status.phase;

        // An answer to an older question is older news, however late it lands and whichever way it went
        if( aAsked < m_shownAsked )
        {
            // The server answered meanwhile, so the state shown instead is worth checking soon
            if( aPhase == MDW_CONNECTION::ONLINE && was != MDW_CONNECTION::ONLINE )
                m_nextProbe = now + std::chrono::milliseconds( FIRST_PROBE_MS );

            return;
        }

        m_shownAsked = aAsked;

        if( aPhase == MDW_CONNECTION::OFFLINE )
        {
            if( was != MDW_CONNECTION::OFFLINE )
                m_probeDelayMs = FIRST_PROBE_MS;
            else if( aChecked )
                m_probeDelayMs = std::min( m_probeDelayMs * 2, LAST_PROBE_MS );

            // A failed call keeps the backoff a check set, and a call answered meanwhile asks again at once
            if( was != MDW_CONNECTION::OFFLINE || aChecked )
                m_nextProbe = now + std::chrono::milliseconds( m_probeEarly ? 0 : m_probeDelayMs );
        }
        else if( aPhase == MDW_CONNECTION::ONLINE || aPhase == MDW_CONNECTION::SIGN_IN_NEEDED
                 || aPhase == MDW_CONNECTION::TRUST_NEEDED )
        {
            m_nextProbe = now + HEARTBEAT;
        }
        else
        {
            m_nextProbe = now + std::chrono::milliseconds( FIRST_PROBE_MS );
        }

        // A backend that dropped a working login gets it again from the next probe, and never one it refused
        if( !aChecked && aReason.error == MDW_ERROR_CODE::NOT_AUTHENTICATED && was == MDW_CONNECTION::ONLINE
            && m_credentialSource )
        {
            m_loginPending = true;
            m_nextProbe = now;
        }

        if( aPhase == MDW_CONNECTION::ONLINE )
        {
            m_status.lastOnline = wxDateTime::Now();

            if( aAccount )
                m_status.account = *aAccount;
        }

        m_status.phase = aPhase;
        m_status.reason = aPhase == MDW_CONNECTION::ONLINE ? MDW_RESULT::Ok() : aReason;

        // A due probe waits for the next poll, so only a new phase wakes the listener and the UI it refreshes
        tell = aPhase != was;
    }

    if( tell && m_onStatus )
        m_onStatus();

    // Events missed while away leave the tracker stale
    if( tell && aPhase == MDW_CONNECTION::ONLINE && m_ingressRunning )
        seedLocks();
}


void MDW_SESSION::seedLocks()
{
    post(
            [this]()
            {
                ListLocks();
            } );
}


void MDW_SESSION::RefreshLocks()
{
    // Starting ingress reads them itself, and without events a list read now would go stale unseen
    if( m_ingressRunning )
        seedLocks();
}


MDW_RESULT MDW_SESSION::ListLocks()
{
    std::shared_ptr<MDW_WORKSPACE_SERVICE> service = Service();

    return service ? service->ListLocks() : MDW_RESULT::Fail( MDW_ERROR_CODE::BACKEND_LOST );
}


void MDW_SESSION::onOutcome( const MDW_RESULT& aResult, MDW_SEND_ORDER aSent )
{
    // A server check applies its own answer
    if( t_checking == this || aResult.error == MDW_ERROR_CODE::CANCELLED )
        return;

    std::optional<MDW_CONNECTION> phase = phaseOf( aResult, false );
    bool                          applies = false;

    {
        std::lock_guard<std::mutex> lock( m_statusMutex );
        const MDW_CONNECTION        current = m_status.phase;
        const auto                  now = std::chrono::steady_clock::now();

        // A stopped backend leaves only through a login or a reopen, so a call cannot hide why it stopped
        if( current == MDW_CONNECTION::BACKEND_STOPPED )
            return;

        // Only a lost connection is news to an offline session, since some answers come from the backend's cache
        if( phase && ( *phase == MDW_CONNECTION::OFFLINE || current != MDW_CONNECTION::OFFLINE ) )
        {
            applies = true;
        }
        else if( current == MDW_CONNECTION::ONLINE )
        {
            m_nextProbe = now + HEARTBEAT;
        }
        else if( current == MDW_CONNECTION::OFFLINE && !m_probeEarly )
        {
            m_probeEarly = true;
            m_nextProbe = now;
        }
    }

    if( applies )
        apply( *phase, aResult, aSent );
}


std::chrono::milliseconds MDW_SESSION::ProbeDueIn() const
{
    // A stopped or respawning backend has nothing to ask, and the crash path owns it meanwhile
    if( !m_supervisor || !m_supervisor->Online() )
        return std::chrono::milliseconds( -1 );

    std::lock_guard<std::mutex> lock( m_statusMutex );

    if( m_status.phase == MDW_CONNECTION::BACKEND_STOPPED )
        return std::chrono::milliseconds( -1 );

    auto due = std::chrono::duration_cast<std::chrono::milliseconds>( m_nextProbe - std::chrono::steady_clock::now() );

    return std::max( due, std::chrono::milliseconds( 0 ) );
}


void MDW_SESSION::Probe( int aTimeoutMs )
{
    // Stands for a check that could not be sent
    MDW_SEND_ORDER asked = MDW_BACKEND::NextSendOrder();

    {
        std::lock_guard<std::mutex> lock( m_statusMutex );
        m_probeEarly = false;
    }

    CHECKING                     checking( this );
    std::shared_ptr<MDW_BACKEND> backend = Backend();
    MDW_ACCOUNT                  account;
    MDW_RESULT                   result = backend ? backend->CheckConnection( aTimeoutMs, &account, &asked )
                                                  : MDW_RESULT::Fail( MDW_ERROR_CODE::BACKEND_LOST );
    bool                         relogin = false;

    {
        std::lock_guard<std::mutex> lock( m_statusMutex );

        // The server is back, and a login it could not verify, or one stored since it refused, may be what it wants
        relogin = m_loginPending && m_credentialSource
                  && ( result.IsOk() || result.error == MDW_ERROR_CODE::NOT_AUTHENTICATED
                       || result.error == MDW_ERROR_CODE::AUTH_FAILED );
        m_loginPending &= !relogin;
    }

    MDW_TYPED_CREDENTIALS stored = relogin ? storedCredentials() : MDW_TYPED_CREDENTIALS();

    // Without a stored credential the check's own answer stands, so a user who saved none signs in again
    if( !stored.fields.empty() )
        result = authenticate( stored, aTimeoutMs, account, asked );

    std::optional<MDW_CONNECTION> phase = phaseOf( result, true );

    {
        std::lock_guard<std::mutex> lock( m_statusMutex );

        // A server gone again before the login reached it still owes that login a verdict
        m_loginPending |= !stored.fields.empty() && phase == MDW_CONNECTION::OFFLINE;

        if( relogin )
            judgeStored( stored, phase );

        if( !phase )
        {
            m_nextProbe = std::chrono::steady_clock::now() + std::chrono::milliseconds( FIRST_PROBE_MS );
            return;
        }
    }

    apply( *phase, result, asked, true, account );
}


MDW_RESULT MDW_SESSION::Authenticate( const MDW_TYPED_CREDENTIALS& aCredentials, int aTimeoutMs )
{
    return login( aCredentials, false, aTimeoutMs );
}


MDW_RESULT MDW_SESSION::Reconnect( int aTimeoutMs, std::optional<MDW_TYPED_CREDENTIALS> aStored )
{
    return login( storedCredentials( std::move( aStored ) ), true, aTimeoutMs );
}


bool MDW_SESSION::StoredLoginRefused( const MDW_TYPED_CREDENTIALS& aStored ) const
{
    // The shared record alone, so a login another session got working is free here too
    if( m_refusals )
        return m_refusals->Refused( challengeRealm(), aStored );

    std::lock_guard<std::mutex> lock( m_statusMutex );
    return m_refusedLogin == MDW_CREDENTIALS::Fingerprint( aStored );
}


bool MDW_SESSION::StoredLoginRefused() const
{
    wxString username;

    {
        std::lock_guard<std::mutex> lock( m_statusMutex );

        if( !m_refusals )
            return m_refusedLogin.has_value();

        username = m_status.username;
    }

    return m_refusals->LoginRefused( challengeRealm(), username );
}


bool MDW_SESSION::StoredLoginChanged()
{
    std::lock_guard<std::mutex> lock( m_statusMutex );

    if( m_status.phase != MDW_CONNECTION::SIGN_IN_NEEDED || !m_credentialSource )
        return false;

    m_loginPending = true;
    m_nextProbe = std::chrono::steady_clock::now();
    return true;
}


MDW_TYPED_CREDENTIALS MDW_SESSION::storedCredentials( std::optional<MDW_TYPED_CREDENTIALS> aRead ) const
{
    if( !aRead && m_credentialSource )
        aRead = m_credentialSource();

    MDW_TYPED_CREDENTIALS stored = aRead.value_or( MDW_TYPED_CREDENTIALS() );

    // Sending a refused credential again could lock the account, so it waits for another one to be stored
    if( StoredLoginRefused( stored ) )
        stored.fields.clear();

    return stored;
}


void MDW_SESSION::judgeStored( const MDW_TYPED_CREDENTIALS& aStored, std::optional<MDW_CONNECTION> aPhase )
{
    // Nothing sent, as from a locked keychain, cannot have been refused
    if( aStored.fields.empty() )
        return;

    if( aPhase == MDW_CONNECTION::SIGN_IN_NEEDED )
        m_refusedLogin = MDW_CREDENTIALS::Fingerprint( aStored );
    else if( aPhase == MDW_CONNECTION::ONLINE )
        m_refusedLogin.reset();

    // Shared, so a session opened later never sends what this one was refused
    if( m_refusals && ( aPhase == MDW_CONNECTION::SIGN_IN_NEEDED || aPhase == MDW_CONNECTION::ONLINE ) )
        m_refusals->JudgeStored( challengeRealm(), aStored, aPhase == MDW_CONNECTION::SIGN_IN_NEEDED );
}


MDW_RESULT MDW_SESSION::login( const MDW_TYPED_CREDENTIALS& aCredentials, bool aStored, int aTimeoutMs )
{
    {
        std::lock_guard<std::mutex> lock( m_statusMutex );
        m_loginPending = false;
    }

    apply( MDW_CONNECTION::CONNECTING, MDW_RESULT::Ok(), MDW_BACKEND::NextSendOrder() );

    CHECKING       checking( this );
    MDW_ACCOUNT    account;
    MDW_SEND_ORDER asked = MDW_BACKEND::NextSendOrder();
    MDW_RESULT     result = authenticate( aCredentials, aTimeoutMs, account, asked );

    if( ( result.error == MDW_ERROR_CODE::AUTH_FAILED || result.error == MDW_ERROR_CODE::NOT_AUTHENTICATED )
        && result.message.IsEmpty() )
    {
        result.message = MdwErrorMessage( result );
    }

    // A shutdown that cancelled the login leaves nothing known about the server
    MDW_CONNECTION phase = phaseOf( result, true ).value_or( MDW_CONNECTION::OFFLINE );

    {
        std::lock_guard<std::mutex> lock( m_statusMutex );

        // Only a stored credential can be read again, so a typed one the server could not verify is gone
        m_loginPending = phase == MDW_CONNECTION::OFFLINE && aStored && !aCredentials.fields.empty();

        // A typed login says nothing about the stored credential, unless it is the refused one now working
        const bool retyped = phase == MDW_CONNECTION::ONLINE
                             && ( m_refusals ? m_refusals->Refused( challengeRealm(), aCredentials )
                                             : m_refusedLogin == MDW_CREDENTIALS::Fingerprint( aCredentials ) );

        if( aStored || retyped )
            judgeStored( aCredentials, phase );
    }

    apply( phase, result, asked, true, account );
    return result;
}


MDW_RESULT MDW_SESSION::authenticate( const MDW_TYPED_CREDENTIALS& aCredentials, int aTimeoutMs, MDW_ACCOUNT& aAccount,
                                      MDW_SEND_ORDER& aSent )
{
    std::shared_ptr<MDW_BACKEND> backend = Backend();

    if( !backend )
        return MDW_RESULT::Fail( MDW_ERROR_CODE::BACKEND_LOST );

    MDW_TYPED_CREDENTIALS sent = aCredentials;
    const wxString        keyField = wxS( "ssh_key_path" );

    {
        std::lock_guard<std::mutex> lock( m_statusMutex );

        // Rides along with every login rather than living in the store, so a refusal never marks it
        if( !m_keyFile.IsEmpty() && backend->AuthChallenge() && !sent.fields.contains( keyField )
            && std::ranges::any_of( backend->AuthChallenge()->fields,
                                    [&]( const MDW_AUTH_FIELD& aField )
                                    {
                                        return aField.field_id == keyField;
                                    } ) )
        {
            sent.fields[keyField] = m_keyFile;
        }
    }

    // No form, a backend too old to name one or nothing to send leaves only the server check to vouch for it
    if( !backend->AuthChallenge() || sent.fields.empty() )
        return backend->CheckConnection( aTimeoutMs, &aAccount, &aSent );

    kiapi::mdw::AuthenticateRequest request;

    for( const auto& [field, value] : sent.fields )
    {
        if( !value.IsEmpty() )
            ( *request.mutable_credentials()->mutable_fields() )[field.utf8_string()] = value.utf8_string();
    }

    kiapi::mdw::AuthenticateReply reply;
    MDW_RESULT                    called = backend->CallSync( request, reply, aTimeoutMs, &aSent );

    if( !called.IsOk() )
        return called;

    if( !reply.authenticated() )
        return MDW_RESULT::Fail( MDW_ERROR_CODE::AUTH_FAILED );

    aAccount = MdwToStruct( reply.account() );
    refreshCapabilities( aTimeoutMs );
    return MDW_RESULT::Ok();
}


void MDW_SESSION::refreshCapabilities( int aTimeoutMs )
{
    std::shared_ptr<MDW_BACKEND> backend = Backend();

    if( !backend || !backend->RefreshCapabilities( aTimeoutMs ) )
        return;

    MDW_EVENT event;
    event.type = MDW_EVENT_TYPE::CAPABILITIES_CHANGED;
    event.workspace_id = m_binding.workspace_id;
    event.timestamp = wxDateTime::UNow();
    m_eventBus->Publish( event );
}


MDW_SESSION::~MDW_SESSION()
{
    DetachBackend();

    if( m_supervisor )
        m_supervisor->Shutdown();

    // A queued task holds its own service reference, so joining here only waits
    // for a call that the shutdown above has already failed
    m_worker.reset();
    m_service.reset();
}


wxString MDW_SESSION::SourceLocatorOf( const wxString& aBinding, const MDW_BACKEND_MANIFEST& aManifest )
{
    return MDW_SETUP::LocatorWithoutItem( aBinding, aManifest.binding_target_param );
}


std::shared_ptr<MDW_BACKEND> MDW_SESSION::Backend() const
{
    return m_supervisor ? m_supervisor->Backend() : nullptr;
}


std::shared_ptr<MDW_WORKSPACE_SERVICE> MDW_SESSION::Service()
{
    std::lock_guard<std::mutex> lock( m_serviceMutex );
    std::shared_ptr<MDW_BACKEND> backend = Backend();

    // A service cached against a backend that has since been replaced or stopped is stale. An unreachable
    // server still gets calls, since one of them may be the first to find it back
    if( !backend || !m_supervisor->Online() )
    {
        m_service.reset();
        return nullptr;
    }

    if( !m_service || m_service->BackendPtr() != backend.get() )
    {
        wxString name = Status().account.Name();

        if( name.IsEmpty() )
            name = wxS( "kicad" );

        m_service = std::make_shared<MDW_WORKSPACE_SERVICE>( backend, m_eventBus, m_lockTracker,
                                                             m_dirtyTracker, m_binding.workspace_id, name );
    }

    return m_service;
}


void MDW_SESSION::post( std::function<void()> aTask )
{
    m_worker->Post( std::move( aTask ) );
}


std::shared_ptr<void> MDW_SESSION::WatchProgress( const wxString& aOperationId, MDW_PROGRESS_SCOPE::SINK aSink )
{
    std::shared_ptr<PROGRESS_WATCHES> watches = m_progress;
    auto                              watch = std::make_shared<PROGRESS_WATCH>();

    watch->sink = std::move( aSink );

    {
        std::lock_guard<std::mutex> lock( watches->mutex );
        watches->sinks[aOperationId] = watch;
    }

    return std::shared_ptr<void>( nullptr,
                                  [watches, watch, aOperationId]( std::nullptr_t )
                                  {
                                      {
                                          std::lock_guard<std::mutex> lock( watches->mutex );
                                          auto it = watches->sinks.find( aOperationId );

                                          if( it != watches->sinks.end() && it->second == watch )
                                              watches->sinks.erase( it );
                                      }

                                      // A call already under way may reach state its owner frees once this returns
                                      std::lock_guard<std::recursive_mutex> calling( watch->calling );
                                      watch->live = false;
                                  } );
}


void MDW_SESSION::onBackendEvent( const MDW_EVENT& aEvent )
{
    // Progress is advisory and unnumbered, so it never reaches the bus's subscribers or their gap check
    if( aEvent.type == MDW_EVENT_TYPE::PROGRESS )
    {
        std::shared_ptr<PROGRESS_WATCH> watch;

        {
            std::lock_guard<std::mutex> lock( m_progress->mutex );

            if( auto it = m_progress->sinks.find( aEvent.operation_id ); it != m_progress->sinks.end() )
                watch = it->second;
        }

        if( watch )
        {
            std::lock_guard<std::recursive_mutex> calling( watch->calling );

            if( watch->live && watch->sink )
                watch->sink( { aEvent.progress_done, aEvent.progress_total, aEvent.progress_phase } );
        }

        return;
    }

    // Runs on the ingress thread; the bus applies lock state through its accept hook
    bool accepted = m_eventBus->PublishFromBackend( aEvent );

    // Subscribers saw the event before the re-read, so the worker announces it again once it is done
    if( accepted && aEvent.type == MDW_EVENT_TYPE::CAPABILITIES_CHANGED )
        post( [this]() { refreshCapabilities( REFRESH_TIMEOUT_MS ); } );
}


bool MDW_SESSION::StartEventIngress()
{
    if( m_ingress )
        return true;

    // The same url the supervisor passes the child with --events
    const wxString url = m_supervisor ? m_supervisor->IpcUrl( wxS( "events.sock" ) ) : wxString();

    if( url.IsEmpty() )
        return false;

    auto ingress = std::make_unique<MDW_EVENT_INGRESS>(
            [this]( const MDW_EVENT& aEvent ) { onBackendEvent( aEvent ); }, m_supervisor->ChannelToken() );

    if( !ingress->Start( url ) )
        return false;

    m_ingress = std::move( ingress );
    m_ingressRunning = true;

    // Events keep the lock tracker current only from here on, so it starts from the backend's whole list
    if( Status().phase == MDW_CONNECTION::ONLINE )
        seedLocks();

    return true;
}


void MDW_SESSION::DetachBackend()
{
    // Waits out an outcome in flight, so none reaches this session once it starts going
    if( m_supervisor )
        m_supervisor->SetOutcomeListener( nullptr );

    // Ingress goes first so no backend event reaches the session while the
    // supervisor stops the backend and joins its transport threads
    m_ingressRunning = false;
    m_ingress.reset();
    m_lockTracker->Invalidate();
}


void MDW_SESSION::DiscardBackendState()
{
    m_ingressRunning = false;
    m_ingress.reset();
    m_lockTracker->Invalidate();

    {
        std::lock_guard<std::mutex> lock( m_serviceMutex );
        m_service.reset();
    }

    m_eventBus->ResetBackendState();
    apply( MDW_CONNECTION::BACKEND_RESTARTING, MDW_RESULT::Fail( MDW_ERROR_CODE::BACKEND_LOST ),
           MDW_BACKEND::NextSendOrder() );
}


void MDW_SESSION::BackendReplaced()
{
    {
        std::lock_guard<std::mutex> lock( m_respawnMutex );
        ++m_backendGeneration;

        if( m_supervisor && m_supervisor->GaveUp() )
            m_respawnable = false;
    }

    m_respawned.notify_all();

    if( m_supervisor && m_supervisor->GaveUp() )
    {
        apply( MDW_CONNECTION::BACKEND_STOPPED,
               MDW_RESULT::Fail( MDW_ERROR_CODE::BACKEND_LOST, _( "The workspace backend keeps stopping." ) ),
               MDW_BACKEND::NextSendOrder() );
    }
}


void MDW_SESSION::SetRespawnable( bool aRespawnable )
{
    {
        std::lock_guard<std::mutex> lock( m_respawnMutex );
        m_respawnable = aRespawnable;
    }

    m_respawned.notify_all();
}


bool MDW_SESSION::AwaitRespawn( std::uint64_t aGeneration, std::chrono::milliseconds aTimeout )
{
    const auto                   until = std::chrono::steady_clock::now() + aTimeout;
    std::unique_lock<std::mutex> lock( m_respawnMutex );

    // A failed restart also moves the generation, so only a backend back online ends the wait
    auto respawned = [&]()
    {
        return m_backendGeneration != aGeneration && m_supervisor && m_supervisor->Online();
    };

    // Woken in slices, since a cancel only sets a flag
    while( m_respawnable && !respawned() && !MDW_CANCEL_SCOPE::Requested() && std::chrono::steady_clock::now() < until )
    {
        m_respawned.wait_for( lock, std::chrono::milliseconds( 50 ) );
    }

    return respawned();
}
