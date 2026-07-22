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

#include <mdw/mdw_backend_supervisor.h>
#include <mdw/mdw_proto_convert.h>

#include <git/git_config_handler.h>
#include <git/kigit_platform.h>
#include <kiid.h>
#include <trace_helpers.h>

#include <wx/filename.h>
#include <wx/log.h>
#include <wx/process.h>
#include <wx/utils.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

#if defined( __unix__ ) || defined( __APPLE__ )
#include <cerrno>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>
#elif defined( _WIN32 )
#include <windows.h>
#endif


/// The restart policy counts attempts on a monotonic clock, so wall-clock jumps
/// cannot make a crash loop look like an idle backend
static std::int64_t monotonicNowMs()
{
    using namespace std::chrono;

    return duration_cast<milliseconds>( steady_clock::now().time_since_epoch() ).count();
}


MDW_BACKEND_SUPERVISOR::MDW_BACKEND_SUPERVISOR( const MDW_BACKEND_MANIFEST& aManifest,
                                                const MDW_BACKEND_CONTEXT& aContext ) :
        m_manifest( aManifest ),
        m_context( aContext )
{
}


MDW_BACKEND_SUPERVISOR::~MDW_BACKEND_SUPERVISOR()
{
    Shutdown();
}


/// True when an ipc:// url fits the platform's socket address, about 104 bytes on macOS and 108 on Linux
static bool ipcUrlFits( const wxString& aUrl )
{
#if defined( __unix__ ) || defined( __APPLE__ )
    const wxString prefix = wxS( "ipc://" );
    const size_t   pathBytes = aUrl.Mid( prefix.length() ).utf8_string().size();

    // sun_path also holds the terminating NUL
    return aUrl.StartsWith( prefix ) && pathBytes < sizeof( sockaddr_un::sun_path );
#else
    // Windows ipc:// is a named pipe, which has no short length limit
    return aUrl.StartsWith( wxS( "ipc://" ) );
#endif
}


/// The directory under which each spawn gets its private socket directory
static wxString runtimeRoot()
{
#if defined( __unix__ ) || defined( __APPLE__ )
    wxString dir;

    if( !wxGetEnv( wxS( "XDG_RUNTIME_DIR" ), &dir ) || dir.IsEmpty() )
    {
        // The temp dir on macOS is long enough on its own to overflow sun_path, so use a
        // per-user directory under /tmp that only this user can enter
        dir = wxString::Format( wxS( "/tmp/kicad-mdw-%lu" ), static_cast<unsigned long>( getuid() ) );

        if( mkdir( dir.utf8_string().c_str(), 0700 ) != 0 && errno != EEXIST )
            return wxEmptyString;
    }

    struct stat st;

    // Another user could have created it first, or pointed the environment at it, to hijack the sockets
    if( lstat( dir.utf8_string().c_str(), &st ) != 0 || !S_ISDIR( st.st_mode ) || st.st_uid != getuid()
        || ( st.st_mode & 077 ) != 0 )
    {
        return wxEmptyString;
    }

    return dir;
#else
    return wxFileName::GetTempDir();
#endif
}


/// Launch a child without wxExecute, which may only run on the main thread while every
/// launch and restart happens on a worker. Returns the pid, or 0 on failure
static long spawnDetached( const std::vector<wxString>& aArgv, const wxEnvVariableHashMap& aEnv )
{
#if defined( __unix__ ) || defined( __APPLE__ )
    std::vector<std::string> argStrings;
    std::vector<std::string> envStrings;

    for( const wxString& arg : aArgv )
        argStrings.push_back( arg.utf8_string() );

    for( const auto& [name, value] : aEnv )
        envStrings.push_back( ( name + wxS( "=" ) + value ).utf8_string() );

    // Everything the child touches is built before fork, which leaves it only
    // async-signal-safe calls in a multithreaded parent
    std::vector<char*> argvPtrs;
    std::vector<char*> envPtrs;

    for( std::string& arg : argStrings )
        argvPtrs.push_back( arg.data() );

    for( std::string& var : envStrings )
        envPtrs.push_back( var.data() );

    argvPtrs.push_back( nullptr );
    envPtrs.push_back( nullptr );

    struct rlimit limits;
    int           maxFd = 1024;

    if( getrlimit( RLIMIT_NOFILE, &limits ) == 0 && limits.rlim_cur != RLIM_INFINITY )
        maxFd = static_cast<int>( std::min<rlim_t>( limits.rlim_cur, 1 << 20 ) );

    pid_t pid = fork();

    if( pid == 0 )
    {
        // The child must not inherit the parent's sockets and files
        for( int fd = 3; fd < maxFd; ++fd )
            close( fd );

        execve( argvPtrs[0], argvPtrs.data(), envPtrs.data() );
        _exit( 127 );
    }

    return pid > 0 ? static_cast<long>( pid ) : 0;
#elif defined( _WIN32 )
    wxString commandLine;

    // None of the arguments contains a quote or ends in a backslash, so plain quoting is exact
    for( const wxString& arg : aArgv )
    {
        if( !commandLine.IsEmpty() )
            commandLine += wxS( " " );

        commandLine += wxS( "\"" ) + arg + wxS( "\"" );
    }

    std::vector<wxString> names;

    for( const auto& [name, value] : aEnv )
        names.push_back( name );

    // CreateProcessW requires the block sorted case-insensitively by name
    std::sort( names.begin(), names.end(),
               []( const wxString& aLhs, const wxString& aRhs )
               {
                   return aLhs.CmpNoCase( aRhs ) < 0;
               } );

    std::wstring envBlock;

    for( const wxString& name : names )
    {
        envBlock += ( name + wxS( "=" ) + aEnv.find( name )->second ).ToStdWstring();
        envBlock.push_back( L'\0' );
    }

    envBlock.push_back( L'\0' );

    std::wstring        cmd = commandLine.ToStdWstring();
    STARTUPINFOW        startup = {};
    PROCESS_INFORMATION info = {};

    startup.cb = sizeof( startup );

    if( !CreateProcessW( nullptr, cmd.data(), nullptr, nullptr, FALSE,
                         CREATE_UNICODE_ENVIRONMENT | CREATE_NO_WINDOW, envBlock.data(), nullptr, &startup,
                         &info ) )
    {
        return 0;
    }

    CloseHandle( info.hThread );
    CloseHandle( info.hProcess );
    return static_cast<long>( info.dwProcessId );
#else
    return 0;
#endif
}


bool MDW_BACKEND_SUPERVISOR::makeRuntimeDir()
{
    const wxString root = runtimeRoot();

    if( root.IsEmpty() )
    {
        wxLogTrace( traceMdw, wxS( "MDW supervisor has no private runtime directory" ) );
        return false;
    }

    // Eight hex digits keep the socket paths short, and Mkdir refuses a name already taken
    wxString token = KIID().AsString();
    wxFileName dir;

    for( int attempt = 0; attempt < 8 && dir.GetPath().IsEmpty(); ++attempt )
    {
        wxFileName candidate( root, wxEmptyString );
        candidate.AppendDir( KIID().AsString().Left( 8 ) );

        if( candidate.Mkdir( 0700 ) )
            dir = candidate;
    }

    if( dir.GetPath().IsEmpty() )
    {
        wxLogTrace( traceMdw, wxS( "MDW supervisor could not create a runtime dir under %s" ), root );
        return false;
    }

    {
        std::lock_guard<std::mutex> lock( m_backendMutex );
        m_runtimeDir = dir.GetPath();
    }

    m_channelToken = token;
    m_controlUrl = IpcUrl( wxS( "control.sock" ) );

    // control.sock is the longest name the session places in this directory
    if( !ipcUrlFits( m_controlUrl ) )
    {
        wxLogTrace( traceMdw, wxS( "MDW runtime dir %s is too long for a socket path" ), m_runtimeDir );
        terminateProcess();
        return false;
    }

    return true;
}


bool MDW_BACKEND_SUPERVISOR::spawnProcess()
{
    if( !makeRuntimeDir() )
        return false;

    wxString eventsUrl = IpcUrl( wxS( "events.sock" ) );

    std::vector<wxString> argv = { m_manifest.executable_path,
                                   wxS( "--kicad-mdw" ),
                                   wxS( "--control" ),
                                   m_controlUrl,
                                   wxS( "--events" ),
                                   eventsUrl,
                                   wxS( "--parent-pid" ),
                                   wxString::Format( wxS( "%ld" ), static_cast<long>( wxGetProcessId() ) ) };

    // The channel token travels in the environment, never on the command line
    wxEnvVariableHashMap env;
    wxGetEnvMap( &env );
    env[wxS( "KICAD_MDW_TOKEN" )] = m_channelToken;

    m_pid = spawnDetached( argv, env );

    if( m_pid <= 0 )
    {
        wxLogTrace( traceMdw, wxS( "MDW supervisor failed to launch %s" ), m_manifest.executable_path );
        return false;
    }

    return true;
}


std::shared_ptr<MDW_BACKEND> MDW_BACKEND_SUPERVISOR::Backend() const
{
    std::lock_guard<std::mutex> lock( m_backendMutex );
    return m_backend;
}


wxString MDW_BACKEND_SUPERVISOR::IpcUrl( const wxString& aSocketName ) const
{
    std::lock_guard<std::mutex> lock( m_backendMutex );

    if( m_runtimeDir.IsEmpty() )
        return wxEmptyString;

    return wxS( "ipc://" ) + m_runtimeDir + wxFileName::GetPathSeparator() + aSocketName;
}


std::shared_ptr<MDW_BACKEND> MDW_BACKEND_SUPERVISOR::exchangeBackend( std::shared_ptr<MDW_BACKEND> aBackend )
{
    std::lock_guard<std::mutex> lock( m_backendMutex );
    std::swap( m_backend, aBackend );
    return aBackend;
}


bool MDW_BACKEND_SUPERVISOR::Launch( int aHandshakeTimeoutMs )
{
    m_shutdownRequested = false;

    if( !spawnProcess() )
        return false;

    auto              backend = std::make_shared<MDW_BACKEND>( m_controlUrl.utf8_string(), m_channelToken );
    kiapi::mdw::Hello hello = makeHello();
    backend->SetOutcomeHook( outcomeHook() );

    if( !backend->Start() )
    {
        terminateProcess();
        return false;
    }

    if( !backend->Handshake( hello, aHandshakeTimeoutMs ) )
    {
        wxLogTrace( traceMdw, wxS( "MDW handshake with %s failed" ), m_manifest.backend_id );
        backend->Stop();
        terminateProcess();
        return false;
    }

    // Published only once usable, so no other thread sees a half-started backend
    exchangeBackend( std::move( backend ) );
    m_online.store( true );

    return true;
}


kiapi::mdw::Hello MDW_BACKEND_SUPERVISOR::makeHello()
{
    kiapi::mdw::Hello hello;
    hello.mutable_workspace()->set_workspace_id( MdwToUtf8( m_context.workspaceId ) );
    hello.mutable_workspace()->set_locator( MdwToUtf8( m_context.locator ) );

    if( !m_context.projectPath.IsEmpty() )
        hello.mutable_workspace()->set_local_root( MdwToUtf8( wxFileName( m_context.projectPath ).GetPath() ) );

    // The repository's own author is read by the backend, so Set Author applies without a restart
    COMMON_SETTINGS::GIT preference{};
    preference.useDefaultAuthor = true;

    if( auto& getPreference = KIGIT::PlatformServices().getAuthorPreference )
        preference = getPreference();

    GitUserConfig local;
    GitUserConfig global;
    KIGIT::ReadAuthorConfig( nullptr, local, global );

    GitUserConfig author = KIGIT::ResolveAuthor( preference, GitUserConfig(), global );

    if( author.hasName || author.hasEmail )
    {
        hello.mutable_author()->set_display_name( MdwToUtf8( author.authorName ) );
        hello.mutable_author()->set_email( MdwToUtf8( author.authorEmail ) );
    }

    return hello;
}


void MDW_BACKEND_SUPERVISOR::SetOutcomeListener( MDW_OUTCOME_HOOK aListener )
{
    std::lock_guard<std::mutex> lock( m_outcomeRelay->mutex );
    m_outcomeRelay->listener = std::move( aListener );
}


MDW_OUTCOME_HOOK MDW_BACKEND_SUPERVISOR::outcomeHook() const
{
    return [relay = m_outcomeRelay]( const MDW_RESULT& aResult, MDW_SEND_ORDER aSent )
    {
        // Held while the listener runs, so clearing it waits for this delivery to finish
        std::lock_guard<std::mutex> lock( relay->mutex );

        if( relay->listener )
            relay->listener( aResult, aSent );
    };
}


bool MDW_BACKEND_SUPERVISOR::Ping( int aTimeoutMs )
{
    std::shared_ptr<MDW_BACKEND> backend = Backend();

    if( !backend )
        return false;

    bool alive = backend->Ping( aTimeoutMs );
    m_online.store( alive );

    return alive;
}


void MDW_BACKEND_SUPERVISOR::awaitExit( const std::vector<MDW_BACKEND_SUPERVISOR*>& aSupervisors,
                                        std::chrono::milliseconds                   aTimeout )
{
    const auto deadline = std::chrono::steady_clock::now() + aTimeout;

    while( true )
    {
        bool running = false;

        // An exited child stays a zombie until reaped and still looks alive to wxProcess::Exists()
        for( MDW_BACKEND_SUPERVISOR* supervisor : aSupervisors )
            running |= supervisor->m_pid > 0 && !supervisor->reapIfExited();

        if( !running || std::chrono::steady_clock::now() >= deadline )
            return;

        std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
    }
}


void MDW_BACKEND_SUPERVISOR::terminateAll( const std::vector<MDW_BACKEND_SUPERVISOR*>& aSupervisors )
{
    for( MDW_BACKEND_SUPERVISOR* supervisor : aSupervisors )
    {
        if( supervisor->m_pid > 0 && !supervisor->reapIfExited() )
            wxKill( supervisor->m_pid, wxSIGTERM );
    }

    awaitExit( aSupervisors, std::chrono::seconds( 3 ) );

    for( MDW_BACKEND_SUPERVISOR* supervisor : aSupervisors )
    {
        if( supervisor->m_pid > 0 )
        {
            wxKill( supervisor->m_pid, wxSIGKILL );

#if defined( __unix__ ) || defined( __APPLE__ )
            int status = 0;

            while( waitpid( static_cast<pid_t>( supervisor->m_pid ), &status, 0 ) < 0 && errno == EINTR )
                ;
#endif

            supervisor->m_pid = 0;
        }

        // Each spawn mints its own directory, so the old one only holds dead sockets
        wxString dir;

        {
            std::lock_guard<std::mutex> lock( supervisor->m_backendMutex );
            std::swap( dir, supervisor->m_runtimeDir );
        }

        if( !dir.IsEmpty() )
            wxFileName::Rmdir( dir, wxPATH_RMDIR_RECURSIVE );
    }
}


void MDW_BACKEND_SUPERVISOR::ShutdownAll( const std::vector<MDW_BACKEND_SUPERVISOR*>& aSupervisors )
{
    const std::chrono::milliseconds           byeGrace( 2000 );
    std::vector<MDW_BACKEND_SUPERVISOR*>      asked;
    std::vector<std::shared_ptr<MDW_BACKEND>> backends;

    for( MDW_BACKEND_SUPERVISOR* supervisor : aSupervisors )
    {
        supervisor->m_shutdownRequested = true;
        supervisor->m_online.store( false );

        if( std::shared_ptr<MDW_BACKEND> backend = supervisor->exchangeBackend( nullptr ) )
        {
            backend->Bye( static_cast<int>( byeGrace.count() ) );
            backends.push_back( std::move( backend ) );
            asked.push_back( supervisor );
        }
    }

    // Every backend has its Bye before any is waited on, so they exit together
    awaitExit( asked, byeGrace );

    // Not sooner, since stopping the transport aborts a Bye still in flight
    for( std::shared_ptr<MDW_BACKEND>& backend : backends )
        backend->Stop();

    terminateAll( aSupervisors );
}


bool MDW_BACKEND_SUPERVISOR::reapIfExited()
{
    if( m_pid <= 0 )
        return false;

#if defined( __unix__ ) || defined( __APPLE__ )
    int   status = 0;
    pid_t reaped = waitpid( static_cast<pid_t>( m_pid ), &status, WNOHANG );

    // Zero means still running and EINTR says nothing, so only a reaped child or the ECHILD of one
    // reaped elsewhere counts as gone
    if( reaped == 0 || ( reaped < 0 && errno != ECHILD ) )
        return false;
#else
    if( wxProcess::Exists( m_pid ) )
        return false;
#endif

    m_pid = 0;

    return true;
}


bool MDW_BACKEND_SUPERVISOR::CrashDetected()
{
    if( m_shutdownRequested || !reapIfExited() )
        return false;

    m_online.store( false );

    wxLogTrace( traceMdw, wxS( "MDW backend %s exited without being asked to" ), m_manifest.backend_id );

    return true;
}


bool MDW_BACKEND_SUPERVISOR::Restart( int aHandshakeTimeoutMs )
{
    if( m_gaveUp || m_shutdownRequested )
        return false;

    int delay = m_restartPolicy.NextDelayMs( monotonicNowMs() );

    if( delay < 0 )
    {
        m_gaveUp = true;
        m_online.store( false );

        wxLogTrace( traceMdw, wxS( "MDW backend %s restarted too often; leaving it offline" ), m_manifest.backend_id );

        return false;
    }

    // No Bye, since the dead process cannot answer it
    m_online.store( false );

    if( std::shared_ptr<MDW_BACKEND> backend = exchangeBackend( nullptr ) )
        backend->Stop();

    if( delay > 0 )
        std::this_thread::sleep_for( std::chrono::milliseconds( delay ) );

    terminateProcess();

    return Launch( aHandshakeTimeoutMs );
}
