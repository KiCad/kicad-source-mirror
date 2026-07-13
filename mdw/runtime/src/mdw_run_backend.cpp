/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2026 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <mdw_runtime/mdw_backend_server.h>
#include <mdw_runtime/mdw_event_publisher.h>
#include <mdw_runtime/mdw_paths.h>

#include "mdw_runtime_internal.h"

#include <api/mdw/backend_base.pb.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <string>

#if defined( _WIN32 )
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace
{
std::atomic<bool> g_terminate{ false };


void onSignal( int )
{
    g_terminate.store( true );
}


// Whether the process that launched this backend is still alive. Linux also gets a death signal; elsewhere
// this polling is the only guard
class PARENT_WATCH
{
public:
    explicit PARENT_WATCH( long aParentPid ) :
            m_pid( aParentPid )
    {
#if defined( _WIN32 )
        // Held from the start, so a reused pid can never stand in for the parent
        if( m_pid > 0 )
            m_handle = OpenProcess( SYNCHRONIZE, FALSE, static_cast<DWORD>( m_pid ) );
#endif
    }

    ~PARENT_WATCH()
    {
#if defined( _WIN32 )
        if( m_handle )
            CloseHandle( m_handle );
#endif
    }

    bool Alive() const
    {
        if( m_pid <= 0 )
            return true;

#if defined( _WIN32 )
        return m_handle && WaitForSingleObject( m_handle, 0 ) == WAIT_TIMEOUT;
#else
        // An orphan is reparented, so its parent pid stops matching
        return getppid() == static_cast<pid_t>( m_pid );
#endif
    }

private:
    long m_pid;

#if defined( _WIN32 )
    HANDLE m_handle = nullptr;
#endif
};
} // namespace


void MDW_BACKEND_SERVER::registerBuiltins()
{
    namespace mdw = kiapi::mdw;

    RegisterHandler<mdw::Hello, mdw::HelloReply>(
            [this]( const mdw::Hello& aReq, mdw::HelloReply& aReply )
            {
                if( aReq.protocol_version().major() != MDW_PROTOCOL_VERSION_MAJOR )
                    return MdwFail( kiapi::workspace::WE_PROTOCOL, "Protocol major mismatch" );

                kiapi::workspace::WorkspaceError accepted = acceptHello( aReq );

                if( accepted.code() != kiapi::workspace::WE_OK )
                    return accepted;

                MDW_BACKEND_DESCRIPTOR descriptor = Descriptor();
                aReply.mutable_protocol_version()->set_major( MDW_PROTOCOL_VERSION_MAJOR );
                aReply.mutable_protocol_version()->set_minor( MDW_PROTOCOL_VERSION_MINOR );
                *aReply.mutable_backend_info() = descriptor.info;
                *aReply.mutable_capabilities() = descriptor.capabilities;

                if( descriptor.auth )
                {
                    if( std::optional<kiapi::workspace::AuthChallenge> auth = descriptor.auth( aReq ) )
                        *aReply.mutable_auth() = *auth;
                }

                return MdwOk();
            } );

    RegisterHandler<mdw::GetCapabilitiesRequest, mdw::GetCapabilitiesReply>(
            [this]( const mdw::GetCapabilitiesRequest&, mdw::GetCapabilitiesReply& aReply )
            {
                *aReply.mutable_capabilities() = Descriptor().capabilities;
                return MdwOk();
            } );

    RegisterHandler<mdw::PingRequest, mdw::PingReply>(
            []( const mdw::PingRequest&, mdw::PingReply& )
            {
                return MdwOk();
            } );

    RegisterHandler<mdw::ByeRequest, mdw::ByeReply>(
            [this]( const mdw::ByeRequest&, mdw::ByeReply& )
            {
                signalByeAfterReply();
                return MdwOk();
            } );

    RegisterHandler<mdw::CancelRequestRequest, mdw::CancelRequestReply>(
            [this]( const mdw::CancelRequestRequest& aReq, mdw::CancelRequestReply& )
            {
                cancelRequest( aReq.request_id() );
                return MdwOk();
            } );

    // The git backend never answers CO_PENDING, so it has no pending change to report
    RegisterHandler<mdw::GetPendingChangeRequest, mdw::GetPendingChangeReply>(
            []( const mdw::GetPendingChangeRequest& aReq, mdw::GetPendingChangeReply& )
            {
                return MdwFail( kiapi::workspace::WE_ITEM_NOT_FOUND, "No pending change " + aReq.change_id() );
            } );
}


std::filesystem::path MDW_BackendDataDir( const std::string& aBackendId )
{
    std::filesystem::path base;

#if defined( _WIN32 )
    const wchar_t* custom = _wgetenv( L"KICAD_MDW_DATA_DIR" );
    const wchar_t* local = _wgetenv( L"LOCALAPPDATA" );

    if( custom && custom[0] )
        return std::filesystem::path( custom ) / MdwFsPath( aBackendId );

    if( local && local[0] )
        base = std::filesystem::path( local ) / "kicad";
#else
    const char* custom = std::getenv( "KICAD_MDW_DATA_DIR" );
    const char* home = std::getenv( "HOME" );

    if( custom && custom[0] )
        return MdwFsPath( custom ) / MdwFsPath( aBackendId );

#if defined( __APPLE__ )
    if( home && home[0] )
        base = MdwFsPath( home ) / "Library" / "Application Support" / "kicad";
#else
    const char* state = std::getenv( "XDG_STATE_HOME" );

    if( state && state[0] )
        base = MdwFsPath( state ) / "kicad";
    else if( home && home[0] )
        base = MdwFsPath( home ) / ".local" / "state" / "kicad";
#endif
#endif

    if( base.empty() )
        return base;

    return base / "mdw" / MdwFsPath( aBackendId );
}


MDW_BACKEND_LAUNCH MDW_ParseLaunch( int argc, char** argv )
{
    MDW_BACKEND_LAUNCH launch;

    for( int i = 1; i < argc; ++i )
    {
        std::string arg = argv[i];

        if( arg == "--control" && i + 1 < argc )
            launch.control_url = argv[++i];
        else if( arg == "--events" && i + 1 < argc )
            launch.events_url = argv[++i];
        else if( arg == "--parent-pid" && i + 1 < argc )
            launch.parent_pid = std::strtol( argv[++i], nullptr, 10 );
    }

    if( const char* token = std::getenv( "KICAD_MDW_TOKEN" ) )
        launch.channel_token = token;

    launch.valid = !launch.control_url.empty();

    return launch;
}


int MDW_BACKEND_SERVER::Run( const MDW_BACKEND_LAUNCH& aLaunch, const MDW_BACKEND_DESCRIPTOR& aDescriptor,
                             MDW_EVENT_PUBLISHER* aPublisher )
{
    if( !aLaunch.valid )
        return 2;

    std::signal( SIGTERM, onSignal );
    std::signal( SIGINT, onSignal );

#ifndef _WIN32
    // A peer that closes mid-write must fail that write, not end the backend
    std::signal( SIGPIPE, SIG_IGN );
#endif

    // Not PR_SET_PDEATHSIG, which fires when the spawning thread exits rather than the parent process
    PARENT_WATCH parent( aLaunch.parent_pid );

    if( !parent.Alive() )
        return 0;

    m_channelToken = aLaunch.channel_token;
    m_publisher = aPublisher;
    SetDescriptor( aDescriptor );
    registerBuiltins();

    std::filesystem::path dataDir = MDW_BackendDataDir( aDescriptor.info.backend_id() );

    // Serving without the journal would apply a retried mutation twice
    if( dataDir.empty() )
        return 3;

    m_journalDir = dataDir / "idempotency";

    // Before serving, since an event raised earlier is lost, and never serving while every event would drop
    if( aPublisher && !aPublisher->Start( aLaunch.events_url ) )
        return 3;

    if( !Start( aLaunch.control_url, 4 ) )
        return 3;

    // A Bye ends the wait at once, while a signal or a lost parent is seen at the next poll
    while( Running() && !g_terminate.load() && parent.Alive() )
    {
        if( waitForBye( std::chrono::milliseconds( 100 ) ) )
            break;
    }

    if( aPublisher )
        aPublisher->Stop();

    Stop();

    // A fill left running past its cancelled request uses the backend, which the caller destroys next
    MdwJoinAbandonedFills();

    return 0;
}
