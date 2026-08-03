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

#include <mdw/client/mdw_client.h>

#include <git/kigit_ssh_keys.h>
#include <mdw/client/mdw_library_sources.h>
#include <mdw/mdw_backend_registry.h>
#include <mdw/mdw_event_bus.h>
#include <mdw/mdw_framework.h>
#include <mdw/mdw_serial_worker.h>
#include <mdw/mdw_session.h>
#include <pgm_base.h>
#include <project.h>
#include <project/project_file.h>
#include <project/project_local_settings.h>
#include <settings/settings_manager.h>

#include "mdw_project_follower.h"

#include <map>
#include <mutex>

#include <wx/app.h>
#include <wx/intl.h>


struct MDW_CLIENT::IMPL
{
    MDW_EVENT_BUS bus;

    MDW_PROJECT_FOLLOWER project{ bus };

    /// Project paths whose connection changed, in each event's payload
    MDW_EVENT_BUS connections;

    std::mutex mutex;

    struct LIBRARY_SOURCE
    {
        wxString                    key;        ///< The framework's session key, the copy folder
        MDW_PROJECT_BINDING         binding;
        wxString                    account;
        std::shared_ptr<MDW_SOURCE> source;
        std::weak_ptr<MDW_SESSION>  session;
        MDW_SUBSCRIPTION            events;
    };

    MDW_FRAMEWORK*                        libraryListeningTo = nullptr;
    std::map<wxString, LIBRARY_SOURCE>    librarySources;     ///< By source id
};


namespace
{

constexpr int CONNECT_TIMEOUT_MS = 8000;


// Leased, since a worker may still ask after shutdown has begun
MDW_FRAMEWORK_LEASE framework( bool aGated = true )
{
    PGM_BASE* pgm = PgmOrNull();

    if( !pgm )
        return {};

    return pgm->LeaseMdwFramework( aGated );
}


std::shared_ptr<MDW_SESSION> openProjectSession( MDW_FRAMEWORK* aFramework )
{
    PGM_BASE* pgm = PgmOrNull();

    if( !aFramework || !pgm || !pgm->GetSettingsManager().IsProjectOpen() )
        return nullptr;

    return aFramework->Session( pgm->GetSettingsManager().Prj().GetProjectFullName() );
}


/// Deliver aBus's events to aHandler on the main thread while aOwner lives
MDW_SUBSCRIPTION subscribeOnUi( MDW_EVENT_BUS& aBus, wxEvtHandler* aOwner, MDW_CLIENT::EVENT_HANDLER aHandler )
{
    struct SUBSCRIBER
    {
        MDW_UI_COMPLETION::TICKET ticket;
        MDW_CLIENT::EVENT_HANDLER handler;
    };

    // Only the bus entry owns this, so a reset subscription strands every delivery already queued
    auto subscriber =
            std::make_shared<SUBSCRIBER>( SUBSCRIBER{ MDW_UI_COMPLETION::Register( aOwner ), std::move( aHandler ) } );

    return aBus.Subscribe(
            [subscriber]( const MDW_EVENT& aEvent )
            {
                MDW_UI_COMPLETION::Post( subscriber->ticket,
                                         [weak = std::weak_ptr<SUBSCRIBER>( subscriber ), aEvent]()
                                         {
                                             if( std::shared_ptr<SUBSCRIBER> live = weak.lock() )
                                                 live->handler( aEvent );
                                         } );
            } );
}


/// The source locator the open project's file binds, live session or not
wxString boundProjectLocator( MDW_FRAMEWORK& aFramework )
{
    PGM_BASE* pgm = PgmOrNull();

    if( !pgm || !pgm->GetSettingsManager().IsProjectOpen() )
        return wxEmptyString;

    const MDW_PROJECT_BINDING   binding = MDW_CLIENT::ProjectBinding( pgm->GetSettingsManager().Prj() );
    const MDW_BACKEND_MANIFEST* manifest = aFramework.Registry().Find( binding.backend_id );

    return manifest ? MDW_SESSION::SourceLocatorOf( binding.binding, *manifest ) : binding.binding;
}

} // namespace


MDW_PROJECT_BINDING MDW_CLIENT::ProjectBinding( const PROJECT& aProject )
{
    // The file keeps the team's binding while this computer sits out
    if( aProject.GetLocalSettings().m_WorkspaceDisabledHere )
        return MDW_PROJECT_BINDING();

    return aProject.GetProjectFile().WorkspaceBinding();
}


bool MDW_CLIENT::BindsGitRemote( const MDW_PROJECT_BINDING& aBinding )
{
    MDW_FRAMEWORK_LEASE         fw = framework();
    const MDW_BACKEND_MANIFEST* manifest = fw ? fw->Registry().Find( aBinding.backend_id ) : nullptr;

    return manifest && manifest->binding_source == MDW_BINDING_SOURCE::GIT_REMOTE;
}


MDW_CLIENT& MDW_CLIENT::Get()
{
    static MDW_CLIENT client;
    return client;
}


MDW_CLIENT::MDW_CLIENT() :
        m_impl( std::make_unique<IMPL>() )
{
}


// The framework is gone by static destruction, so its listener registration is left behind unread
MDW_CLIENT::~MDW_CLIENT() = default;


void MDW_CLIENT::Listen( MDW_FRAMEWORK& aFramework )
{
    // A replaced, restarted or closed session must reach the bus even when nobody asks for the source
    aFramework.AddSessionListener( m_impl.get(),
                                   []( const wxString& )
                                   {
                                       MDW_CLIENT::Get().ProjectSource();
                                   } );

    // The bus carries MDW_EVENT, so the path rides in payload; this bus never reaches a source subscriber
    aFramework.AddSessionListener( &m_impl->connections,
                                   [this]( const wxString& aProjectPath )
                                   {
                                       MDW_EVENT event;
                                       event.payload = aProjectPath;
                                       m_impl->connections.Publish( event );
                                   } );

    // A new framework can reuse a stopped one's address, which would pass for already listened to
    std::lock_guard<std::mutex> lock( m_impl->mutex );
    m_impl->libraryListeningTo = nullptr;
}


std::shared_ptr<MDW_SOURCE> MDW_CLIENT::ProjectSource() const
{
    MDW_FRAMEWORK_LEASE fw = framework();

    return m_impl->project.Follow( openProjectSession( fw.Get() ) );
}


std::vector<std::shared_ptr<MDW_SOURCE>> MDW_CLIENT::LibrarySources() const
{
    MDW_FRAMEWORK_LEASE                    fw = framework();
    std::vector<MDW_LIBRARY_SOURCE_CONFIG> configs;

    if( fw && MDW_LIBRARY_SOURCES::TeamLibrariesEnabled() )
        configs = MDW_LIBRARY_SOURCES::Get().Sources();

    struct WANTED
    {
        MDW_LIBRARY_SOURCE_CONFIG    config;
        wxString                     key;
        std::shared_ptr<MDW_SESSION> session;
        bool                         idle = false;
        bool                         hasCopy = false; ///< Only asked when there is no session
    };

    // Asked before the lock, since the framework calls its listeners, and so this, under its own
    std::vector<WANTED>         wanted;
    std::shared_ptr<MDW_SOURCE> project = configs.empty() ? nullptr : ProjectSource();
    wxString                    projectLocator = configs.empty() ? wxString() : boundProjectLocator( *fw.Get() );

    for( const MDW_LIBRARY_SOURCE_CONFIG& config : configs )
    {
        // A locator names one source, and the project's answers for it, even while it reconnects
        if( ( project && project->Info().locator == config.locator )
            || ( !projectLocator.IsEmpty() && projectLocator == config.locator ) )
        {
            continue;
        }

        wxString key = MDW_LIBRARY_SOURCES::Get().SessionKey( config );

        std::shared_ptr<MDW_SESSION> session = fw->Session( key );

        if( session && ( session->Binding().backend_id != config.backend_id
                         || session->Binding().binding != config.locator ) )
        {
            session.reset();
        }

        wanted.push_back( { config, key, session, fw->ConnectState( key ) == MDW_FRAMEWORK::CONNECT_STATE::IDLE,
                            !session && MDW_LIBRARY_SOURCES::Get().HasCopy( config ) } );
    }

    // Released after the lock, since releasing waits for a handler that may call back in here
    std::vector<MDW_SUBSCRIPTION>            stale;
    std::vector<wxString>                    closed;
    std::vector<IMPL::LIBRARY_SOURCE>        connect;
    std::vector<std::shared_ptr<MDW_SOURCE>> sources;

    {
        std::lock_guard<std::mutex> lock( m_impl->mutex );
        std::map<wxString, IMPL::LIBRARY_SOURCE> previous = std::move( m_impl->librarySources );

        m_impl->librarySources.clear();

        if( fw && fw.Get() != m_impl->libraryListeningTo && wxTheApp )
        {
            m_impl->libraryListeningTo = fw.Get();

            // A session opening later must start feeding the bus without anyone asking again
            fw->AddSessionListener( &m_impl->librarySources,
                                    []( const wxString& )
                                    {
                                        MDW_CLIENT::Get().LibrarySources();
                                    } );
        }

        for( const auto& [config, key, session, idle, hasCopy] : wanted )
        {
            MDW_PROJECT_BINDING binding{ config.backend_id, config.name, config.locator };
            auto                old = previous.find( config.id );

            IMPL::LIBRARY_SOURCE entry;

            if( old != previous.end() && old->second.key == key && old->second.binding.backend_id == binding.backend_id
                && old->second.binding.binding == binding.binding )
            {
                entry = std::move( old->second );
                previous.erase( old );
            }
            else
            {
                entry.key = key;
                entry.binding = binding;
                entry.source = std::make_shared<MDW_SOURCE>(
                        [key, binding]() -> std::shared_ptr<MDW_SESSION>
                        {
                            MDW_FRAMEWORK_LEASE          current = framework();
                            std::shared_ptr<MDW_SESSION> live = current ? current->Session( key ) : nullptr;

                            // A source set up later in the same folder is not this one
                            if( !live || live->Binding().backend_id != binding.backend_id
                                || live->Binding().binding != binding.binding )
                            {
                                return nullptr;
                            }

                            return live;
                        },
                        config.locator );
            }

            if( session != entry.session.lock() || ( session && !entry.events.IsActive() ) )
            {
                stale.push_back( std::move( entry.events ) );
                entry.session = session;

                if( session )
                {
                    entry.events = session->EventBus().Subscribe(
                            [this, locator = config.locator]( const MDW_EVENT& aEvent )
                            {
                                MDW_EVENT event = aEvent;
                                event.source = locator;
                                m_impl->bus.Publish( event );
                            } );
                }
            }

            // A failed attempt waits for the user to retry rather than for the next lookup. A user source with no
            // copy has nothing to serve and opens its own session at set-up, but a site source serves its catalog
            if( !session && idle && ( hasCopy || config.site ) )
                connect.push_back( { key, binding, config.account, nullptr, {}, {} } );

            sources.push_back( entry.source );
            m_impl->librarySources.emplace( config.id, std::move( entry ) );
        }

        for( auto& [id, entry] : previous )
        {
            stale.push_back( std::move( entry.events ) );
            closed.push_back( entry.key );
        }
    }

    stale.clear();

    if( !fw )
        return sources;

    for( const wxString& key : closed )
        fw->CloseSession( key );

    // A backend spawned during shutdown would only be told Bye
    if( PgmOrNull() && Pgm().m_Quitting )
        return sources;

    for( const IMPL::LIBRARY_SOURCE& entry : connect )
        fw->EnsureSessionAsync( entry.key, entry.binding, CONNECT_TIMEOUT_MS, entry.account );

    return sources;
}


void MDW_CLIENT::CloseProject( const wxString& aProjectPath )
{
    if( MDW_FRAMEWORK_LEASE fw = framework( false ) )
        fw->CloseSession( aProjectPath );
}


wxString MdwConnectionName( MDW_CONNECTION aPhase )
{
    switch( aPhase )
    {
    case MDW_CONNECTION::CONNECTING: return _( "Connecting" );
    case MDW_CONNECTION::ONLINE: return _( "Connected" );
    case MDW_CONNECTION::OFFLINE: return _( "Offline" );
    case MDW_CONNECTION::SIGN_IN_NEEDED: return _( "Sign in needed" );
    case MDW_CONNECTION::TRUST_NEEDED: return _( "Verify server" );
    case MDW_CONNECTION::BACKEND_RESTARTING: return _( "Backend restarting" );
    case MDW_CONNECTION::BACKEND_STOPPED: return _( "Backend stopped" );
    default: return _( "Not connected" );
    }
}


MDW_PROJECT_CONNECTION MDW_CLIENT::ProjectConnection( const PROJECT& aProject ) const
{
    MDW_FRAMEWORK_LEASE    fw = framework();
    MDW_PROJECT_CONNECTION connection;
    const wxString         path = aProject.GetProjectFullName();

    if( !fw || path.IsEmpty() )
        return connection;

    const MDW_PROJECT_BINDING binding = ProjectBinding( aProject );

    if( !binding.IsBound() )
    {
        connection.status.phase = MDW_CONNECTION::UNBOUND;
        return connection;
    }

    const MDW_BACKEND_MANIFEST* manifest = fw->Registry().Find( binding.backend_id );

    connection.source_name =
            manifest && !manifest->display_name.IsEmpty() ? manifest->display_name : binding.backend_id;
    connection.workspace_id = binding.workspace_id;

    switch( fw->ConnectState( path ) )
    {
    case MDW_FRAMEWORK::CONNECT_STATE::OPENING: connection.status.phase = MDW_CONNECTION::CONNECTING; break;

    case MDW_FRAMEWORK::CONNECT_STATE::BACKEND_STOPPED:
        connection.status.phase = MDW_CONNECTION::BACKEND_STOPPED;
        connection.status.reason =
                MDW_RESULT::Fail( MDW_ERROR_CODE::BACKEND_LOST,
                                  wxString::Format( _( "%s could not be started." ), connection.source_name ) );
        break;

    case MDW_FRAMEWORK::CONNECT_STATE::IDLE:
        if( std::shared_ptr<MDW_SESSION> session = fw->Session( path ) )
        {
            connection.status = session->Status();
            connection.status.storedLoginRefused = session->StoredLoginRefused();
        }
        else
            connection.status.phase = MDW_CONNECTION::IDLE;

        break;
    }

    return connection;
}


void MDW_CLIENT::ReconnectProject( const PROJECT& aProject, const MDW_CONNECTION_ANSWER& aAnswer )
{
    MDW_FRAMEWORK_LEASE fw = framework();
    const wxString      path = aProject.GetProjectFullName();

    if( !fw || path.IsEmpty() )
        return;

    std::shared_ptr<MDW_SESSION> session = fw->Session( path );

    // A backend that never started, kept stopping or is too old needs a new session, which listeners open on
    // the close
    if( !session || session->Status().phase == MDW_CONNECTION::BACKEND_STOPPED )
        fw->CloseSession( path );
    else
        fw->ReconnectAsync( path, aAnswer, CONNECT_TIMEOUT_MS );
}


void MDW_CLIENT::ConnectProject( PROJECT& aProject )
{
    MDW_FRAMEWORK_LEASE fw = framework();
    const wxString      path = aProject.GetProjectFullName();

    if( !fw || path.IsEmpty() )
        return;

    const MDW_PROJECT_BINDING binding = ProjectBinding( aProject );

    // The framework skips a binding that is already open or opening, so every frame may ask
    // A backend on the project's git remote signs in as the project's git settings do
    if( binding.IsBound() && BindsGitRemote( binding ) )
    {
        const PROJECT_LOCAL_SETTINGS& local = aProject.GetLocalSettings();

        fw->EnsureSessionAsync( path, binding, CONNECT_TIMEOUT_MS, local.m_GitRepoUsername,
                                KIGIT::PrivateKeyPath( local.m_GitSSHKey ) );
    }
    else if( binding.IsBound() )
    {
        fw->EnsureSessionAsync( path, binding, CONNECT_TIMEOUT_MS );
    }
}


MDW_SUBSCRIPTION MDW_CLIENT::SubscribeConnections( wxEvtHandler*                          aOwner,
                                                   std::function<void( const wxString& )> aChanged )
{
    return subscribeOnUi( m_impl->connections, aOwner,
                          [changed = std::move( aChanged )]( const MDW_EVENT& aEvent )
                          {
                              changed( aEvent.payload );
                          } );
}


std::shared_ptr<MDW_SOURCE> MDW_CLIENT::FindSource( const wxString& aLocator ) const
{
    if( aLocator.IsEmpty() )
        return nullptr;

    std::shared_ptr<MDW_SOURCE> project = ProjectSource();

    if( project && project->Info().locator == aLocator )
        return project;

    for( const std::shared_ptr<MDW_SOURCE>& source : LibrarySources() )
    {
        if( source->Info().locator == aLocator )
            return source;
    }

    return nullptr;
}


MDW_SUBSCRIPTION MDW_CLIENT::Subscribe( wxEvtHandler* aOwner, EVENT_HANDLER aHandler )
{
    ProjectSource();

    return subscribeOnUi( m_impl->bus, aOwner, std::move( aHandler ) );
}


MDW_SUBSCRIPTION MDW_CLIENT::Subscribe( EVENT_HANDLER aHandler )
{
    ProjectSource();

    return m_impl->bus.Subscribe( std::move( aHandler ) );
}


void MDW_CLIENT::Publish( const MDW_EVENT& aEvent )
{
    m_impl->bus.Publish( aEvent );
}
