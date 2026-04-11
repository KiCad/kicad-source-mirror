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

#ifndef QA_API_E2E_UTILS_H
#define QA_API_E2E_UTILS_H

#include <chrono>
#include <functional>
#include <string>
#include <vector>

#include <google/protobuf/any.pb.h>
#include <magic_enum.hpp>
#include <wx/filename.h>
#include <wx/process.h>
#include <wx/stream.h>
#include <wx/utils.h>

#ifdef __UNIX__
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#include <nng/nng.h>
#include <nng/protocol/reqrep0/req.h>

#include <import_export.h>
#include <api/api_utils.h>
#include <api/common/envelope.pb.h>
#include <api/common/commands/base_commands.pb.h>
#include <api/common/commands/editor_commands.pb.h>
#include <api/common/commands/project_commands.pb.h>
#include <api/common/types/base_types.pb.h>
#include <api/common/types/enums.pb.h>
#include <api/common/types/jobs.pb.h>
#include <footprint.h>
#include <pcbnew_utils/board_file_utils.h>
#include <qa_utils/wx_utils/unit_test_utils.h>
#include <wx/log.h>


#ifndef QA_KICAD_CLI_PATH
#define QA_KICAD_CLI_PATH "kicad-cli"
#endif


class API_SERVER_PROCESS : public wxProcess
{
public:
    API_SERVER_PROCESS( std::function<void( int, const wxString&, const wxString& )> aCallback ) :
            wxProcess(),
            m_callback( std::move( aCallback ) )
    {
    }

    void OnTerminate( int aPid, int aStatus ) override
    {
        wxString output;
        wxString error;

        drainStream( GetInputStream(), output );
        drainStream( GetErrorStream(), error );

        if( m_callback )
            m_callback( aStatus, output, error );
    }

    static void drainStream( wxInputStream* aStream, wxString& aDest )
    {
        if( !aStream || !aStream->CanRead() )
            return;

        constexpr size_t chunkSize = 1024;
        char             buffer[chunkSize + 1] = { 0 };

        while( aStream->CanRead() )
        {
            aStream->Read( buffer, chunkSize );
            size_t bytesRead = aStream->LastRead();

            if( bytesRead == 0 )
                break;

            buffer[bytesRead] = '\0';
            aDest += wxString::FromUTF8( buffer );
        }
    }

private:
    std::function<void( int, const wxString&, const wxString& )> m_callback;
};


class API_TEST_CLIENT
{
public:
    API_TEST_CLIENT()
    {
        int ret = nng_req0_open( &m_socket );

        if( ret == 0 )
        {
            m_isOpen = true;
            nng_socket_set_ms( m_socket, NNG_OPT_RECVTIMEO, 10000 );
            nng_socket_set_ms( m_socket, NNG_OPT_SENDTIMEO, 10000 );
        }
        else
        {
            m_lastError =
                    wxString::Format( wxS( "nng_req0_open failed: %s" ), wxString::FromUTF8( nng_strerror( ret ) ) );
        }
    }

    ~API_TEST_CLIENT()
    {
        if( m_isOpen )
            nng_close( m_socket );
    }

    bool Connect( const wxString& aSocketUrl )
    {
        if( !m_isOpen )
            return false;

        if( m_isConnected )
            return true;

        int ret = nng_dial( m_socket, aSocketUrl.ToStdString().c_str(), nullptr, 0 );

        if( ret != 0 )
        {
            m_lastError = wxString::Format( wxS( "nng_dial failed: %s" ), wxString::FromUTF8( nng_strerror( ret ) ) );
            return false;
        }

        m_isConnected = true;
        return true;
    }

    void Disconnect()
    {
        if( m_isOpen )
        {
            nng_close( m_socket );
            m_isOpen = false;
            m_isConnected = false;

            int ret = nng_req0_open( &m_socket );

            if( ret == 0 )
            {
                m_isOpen = true;
                nng_socket_set_ms( m_socket, NNG_OPT_RECVTIMEO, 10000 );
                nng_socket_set_ms( m_socket, NNG_OPT_SENDTIMEO, 10000 );
            }
        }
    }

    const wxString& LastError() const { return m_lastError; }

    bool Ping( kiapi::common::ApiStatusCode* aStatusOut = nullptr )
    {
        kiapi::common::commands::Ping ping;
        kiapi::common::ApiResponse    response;

        if( !sendCommand( ping, &response ) )
            return false;

        if( aStatusOut )
            *aStatusOut = response.status().status();

        return true;
    }

    bool GetVersion()
    {
        kiapi::common::commands::GetVersion request;
        kiapi::common::ApiResponse          response;

        if( !sendCommand( request, &response ) )
            return false;

        if( response.status().status() != kiapi::common::AS_OK )
        {
            m_lastError = response.status().error_message();
            return false;
        }

        kiapi::common::commands::GetVersionResponse version;

        if( !response.message().UnpackTo( &version ) )
        {
            m_lastError = wxS( "Failed to unpack GetVersionResponse" );
            return false;
        }

        return true;
    }

    bool OpenDocument( const wxString& aPath, kiapi::common::types::DocumentType aType,
                       kiapi::common::types::DocumentSpecifier* aDocument = nullptr )
    {
        kiapi::common::commands::OpenDocument request;
        request.set_type( aType );
        request.set_path( aPath.ToStdString() );

        kiapi::common::ApiResponse response;

        if( !sendCommand( request, &response ) )
            return false;

        if( response.status().status() != kiapi::common::AS_OK )
        {
            m_lastError = response.status().error_message();
            return false;
        }

        kiapi::common::commands::OpenDocumentResponse openResponse;

        if( !response.message().UnpackTo( &openResponse ) )
        {
            m_lastError = wxS( "Failed to unpack OpenDocumentResponse" );
            return false;
        }

        if( aDocument )
            *aDocument = openResponse.document();

        return true;
    }

    // Convenience overload for PCB documents (backward compatible)
    bool OpenDocument( const wxString& aPath, kiapi::common::types::DocumentSpecifier* aDocument = nullptr )
    {
        return OpenDocument( aPath, kiapi::common::types::DOCTYPE_PCB, aDocument );
    }

    bool GetItemsCount( const kiapi::common::types::DocumentSpecifier& aDocument,
                        kiapi::common::types::KiCadObjectType aType, int* aCount )
    {
        kiapi::common::commands::GetItems request;
        *request.mutable_header()->mutable_document() = aDocument;
        request.add_types( aType );

        kiapi::common::ApiResponse response;

        if( !sendCommand( request, &response ) )
            return false;

        if( response.status().status() != kiapi::common::AS_OK )
        {
            m_lastError = response.status().error_message();
            return false;
        }

        kiapi::common::commands::GetItemsResponse itemsResponse;

        if( !response.message().UnpackTo( &itemsResponse ) )
        {
            m_lastError = wxS( "Failed to unpack GetItemsResponse" );
            return false;
        }

        if( itemsResponse.status() != kiapi::common::types::IRS_OK )
        {
            m_lastError = wxString::Format( wxS( "GetItems returned non-OK status: %d" ),
                                            static_cast<int>( itemsResponse.status() ) );
            return false;
        }

        *aCount = itemsResponse.items_size();
        return true;
    }

    bool GetFirstFootprint( const kiapi::common::types::DocumentSpecifier& aDocument, FOOTPRINT* aFootprint )
    {
        wxCHECK( aFootprint, false );

        kiapi::common::commands::GetItems request;
        *request.mutable_header()->mutable_document() = aDocument;
        request.add_types( kiapi::common::types::KOT_PCB_FOOTPRINT );

        kiapi::common::ApiResponse response;

        if( !sendCommand( request, &response ) )
        {
            m_lastError = wxS( "Failed to send command" );
            return false;
        }

        if( response.status().status() != kiapi::common::AS_OK )
        {
            m_lastError = response.status().error_message();
            return false;
        }

        kiapi::common::commands::GetItemsResponse itemsResponse;

        if( !response.message().UnpackTo( &itemsResponse ) )
        {
            m_lastError = wxS( "Failed to unpack GetItemsResponse" );
            return false;
        }

        if( itemsResponse.status() != kiapi::common::types::IRS_OK )
        {
            m_lastError = wxString::Format( wxS( "GetItems returned non-OK status: %s" ),
                                            magic_enum::enum_name( itemsResponse.status() ) );
            return false;
        }

        if( itemsResponse.items_size() == 0 )
        {
            m_lastError = wxS( "GetItems returned no footprints" );
            return false;
        }

        if( !aFootprint->Deserialize( itemsResponse.items( 0 ) ) )
        {
            m_lastError = wxS( "Failed to deserialize first footprint from GetItems response" );
            return false;
        }

        return true;
    }

    bool CloseDocument( const kiapi::common::types::DocumentSpecifier* aDocument,
                        kiapi::common::ApiStatusCode*                  aStatus = nullptr )
    {
        kiapi::common::commands::CloseDocument request;

        if( aDocument )
            *request.mutable_document() = *aDocument;

        kiapi::common::ApiResponse response;

        if( !sendCommand( request, &response ) )
        {
            m_lastError = wxS( "Failed to send command" );
            return false;
        }

        if( aStatus )
            *aStatus = response.status().status();

        if( response.status().status() != kiapi::common::AS_OK )
            m_lastError = response.status().error_message();

        return true;
    }

    /**
     * Send an arbitrary job request and receive a RunJobResponse.
     */
    template <typename T>
    bool RunJob( const T& aJobRequest, kiapi::common::types::RunJobResponse* aResponse )
    {
        kiapi::common::ApiResponse apiResponse;

        if( !sendCommand( aJobRequest, &apiResponse ) )
            return false;

        if( apiResponse.status().status() != kiapi::common::AS_OK )
        {
            m_lastError = apiResponse.status().error_message();
            return false;
        }

        if( !apiResponse.message().UnpackTo( aResponse ) )
        {
            m_lastError = wxS( "Failed to unpack RunJobResponse" );
            return false;
        }

        return true;
    }

    template <typename T>
    bool SendCommand( const T& aMessage, kiapi::common::ApiResponse* aResponse )
    {
        return sendCommand( aMessage, aResponse );
    }

private:
    template <typename T>
    bool sendCommand( const T& aMessage, kiapi::common::ApiResponse* aResponse )
    {
        if( !m_isConnected )
        {
            m_lastError = wxS( "API client is not connected" );
            return false;
        }

        kiapi::common::ApiRequest request;
        request.mutable_header()->set_client_name( "kicad.qa" );

        if( !request.mutable_message()->PackFrom( aMessage ) )
        {
            m_lastError = wxS( "Failed to pack command into ApiRequest" );
            return false;
        }

        std::string requestStr = request.SerializeAsString();
        void*       sendBuf = nng_alloc( requestStr.size() );

        if( !sendBuf )
        {
            m_lastError = wxS( "nng_alloc failed" );
            return false;
        }

        memcpy( sendBuf, requestStr.data(), requestStr.size() );

        int ret = nng_send( m_socket, sendBuf, requestStr.size(), NNG_FLAG_ALLOC );

        if( ret != 0 )
        {
            m_lastError = wxString::Format( wxS( "nng_send failed: %s" ), wxString::FromUTF8( nng_strerror( ret ) ) );
            return false;
        }

        char*  reply = nullptr;
        size_t replySize = 0;

        ret = nng_recv( m_socket, &reply, &replySize, NNG_FLAG_ALLOC );

        if( ret != 0 )
        {
            m_lastError = wxString::Format( wxS( "nng_recv failed: %s" ), wxString::FromUTF8( nng_strerror( ret ) ) );
            return false;
        }

        std::string responseStr( reply, replySize );
        nng_free( reply, replySize );

        if( !aResponse->ParseFromString( responseStr ) )
        {
            m_lastError = wxS( "Failed to parse reply from KiCad" );
            return false;
        }

        return true;
    }

    nng_socket m_socket;
    bool       m_isOpen = false;
    bool       m_isConnected = false;
    wxString   m_lastError;
};


/**
 * Manages a single shared kicad-cli api-server process and NNG client across all E2E tests.
 *
 * Starting the api-server takes several seconds due to process startup and IPC handshake.
 * Rather than paying that cost per test case, this singleton starts the server and connects
 * a single persistent client on first use. Both live until the test module exits.
 *
 * A single NNG client is used because the NNG req/rep protocol does not reliably handle
 * rapid connect/disconnect cycles on IPC sockets. Keeping one persistent connection avoids
 * stale pipe state in the server's rep socket.
 */
class API_SERVER_MANAGER
{
public:
    static API_SERVER_MANAGER& Instance()
    {
        static API_SERVER_MANAGER s_instance;
        return s_instance;
    }

    /**
     * Ensure the api-server is running and the shared client is connected and responsive.
     */
    bool EnsureReady( const wxString& aCliPath, wxString* aError )
    {
        if( m_ready )
        {
            if( isProcessAlive() )
                return true;

            m_ready = false;
        }

        if( !startServerProcess( aCliPath, aError ) )
            return false;

        if( !connectAndWaitReady( aError ) )
            return false;

        m_ready = true;
        return true;
    }

    API_TEST_CLIENT& Client() { return m_client; }

    /**
     * Kill the current server and reset state so EnsureReady() will start fresh.
     */
    void Reset()
    {
        m_ready = false;
        m_client.Disconnect();

        if( m_pid != 0 && isProcessAlive() )
        {
#ifdef __WXMAC__
            wxProcess::Kill( m_pid, wxSIGKILL );
#else
            wxProcess::Kill( m_pid, wxSIGKILL, wxKILL_CHILDREN );
#endif
        }

        m_pid = 0;
        m_process = nullptr;

        if( wxFileName::Exists( m_socketPath ) )
            wxRemoveFile( m_socketPath );
    }

private:
    API_SERVER_MANAGER()
    {
#ifdef __UNIX__
        m_socketPath = wxString::Format( wxS( "/tmp/kicad-api-e2e-%ld.sock" ), wxGetProcessId() );
#else
        wxString tempPath = wxFileName::CreateTempFileName( wxS( "kicad-api-e2e" ) );

        if( wxFileExists( tempPath ) )
            wxRemoveFile( tempPath );

        m_socketPath = tempPath + wxS( ".sock" );
#endif

        m_socketUrl = wxS( "ipc://" ) + m_socketPath;

        if( wxFileName::Exists( m_socketPath ) )
            wxRemoveFile( m_socketPath );
    }

    ~API_SERVER_MANAGER()
    {
        // Use POSIX primitives here because this destructor runs during static destruction
        // when wxWidgets may already be partially torn down.
#ifdef __UNIX__
        if( m_pid != 0 )
        {
            kill( static_cast<pid_t>( m_pid ), SIGTERM );
            usleep( 200000 );
            kill( static_cast<pid_t>( m_pid ), SIGKILL );
        }

        if( !m_socketPath.IsEmpty() )
            unlink( m_socketPath.ToUTF8().data() );
#else
        if( m_pid != 0 )
            wxProcess::Kill( m_pid, wxSIGKILL, wxKILL_CHILDREN );

        if( !m_socketPath.IsEmpty() && wxFileName::Exists( m_socketPath ) )
            wxRemoveFile( m_socketPath );
#endif
    }

    API_SERVER_MANAGER( const API_SERVER_MANAGER& ) = delete;
    API_SERVER_MANAGER& operator=( const API_SERVER_MANAGER& ) = delete;

    bool startServerProcess( const wxString& aCliPath, wxString* aError )
    {
        if( m_pid != 0 )
            return true;

        if( !wxFileExists( aCliPath ) )
        {
            if( aError )
                *aError = wxS( "kicad-cli path does not exist: " ) + aCliPath;

            return false;
        }

        m_stdout.clear();
        m_stderr.clear();
        m_exitCode = 0;

        m_process = new API_SERVER_PROCESS(
                [this]( int aStatus, const wxString& aOutput, const wxString& aErrOutput )
                {
                    m_exitCode = aStatus;
                    m_stdout += aOutput;
                    m_stderr += aErrOutput;
                    m_pid = 0;
                    m_process = nullptr;
                    m_ready = false;
                } );

        m_process->Redirect();

        std::vector<const wchar_t*> args = { aCliPath.wc_str(), wxS( "api-server" ), wxS( "--socket" ),
                                             m_socketPath.wc_str(), nullptr };

        m_pid = wxExecute( args.data(), wxEXEC_ASYNC, m_process );

        if( m_pid == 0 )
        {
            delete m_process;
            m_process = nullptr;

            if( aError )
                *aError = wxS( "Failed to launch kicad-cli api-server" );

            return false;
        }

        return true;
    }

    bool connectAndWaitReady( wxString* aError )
    {
        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds( 15 );

        while( std::chrono::steady_clock::now() < deadline )
        {
            wxMilliSleep( 100 );
            collectServerOutput();

            if( !isProcessAlive() )
            {
                if( aError )
                    *aError = wxS( "kicad-cli process exited before ready" );

                return false;
            }

            if( !m_client.Connect( m_socketUrl ) )
            {
                wxLogTrace( traceApi, "kicad-cli connect attempt failed, will retry" );
                continue;
            }

            kiapi::common::ApiStatusCode pingStatus = kiapi::common::AS_UNKNOWN;

            if( m_client.Ping( &pingStatus ) )
            {
                if( pingStatus == kiapi::common::AS_OK )
                    return true;

                if( pingStatus != kiapi::common::AS_NOT_READY )
                {
                    if( aError )
                    {
                        *aError = wxString::Format( wxS( "Ping returned unexpected status: %s" ),
                                                    magic_enum::enum_name( pingStatus ) );
                    }

                    return false;
                }

                wxLogTrace( traceApi, "kicad-cli connected but returned AS_NOT_READY, will retry" );
            }
            else
            {
                wxLogTrace( traceApi, "kicad-cli ping attempt failed, will retry" );
            }
        }

        if( aError )
            *aError = wxS( "Timed out waiting for kicad-cli to start up" );

        return false;
    }

    void collectServerOutput()
    {
        if( !m_process )
            return;

        API_SERVER_PROCESS::drainStream( m_process->GetInputStream(), m_stdout );
        API_SERVER_PROCESS::drainStream( m_process->GetErrorStream(), m_stderr );
    }

    bool isProcessAlive() const
    {
        if( m_pid == 0 )
            return false;

#ifdef __UNIX__
        int   status = 0;
        pid_t result = waitpid( static_cast<pid_t>( m_pid ), &status, WNOHANG );

        if( result == 0 )
            return true;

        if( result > 0 )
        {
            if( WIFSIGNALED( status ) )
            {
                BOOST_TEST_MESSAGE( wxString::Format( "api-server killed by signal %d (%s)",
                                                      WTERMSIG( status ),
                                                      strsignal( WTERMSIG( status ) ) ) );
            }
            else if( WIFEXITED( status ) )
            {
                BOOST_TEST_MESSAGE( wxString::Format( "api-server exited with code %d",
                                                      WEXITSTATUS( status ) ) );
            }

            return false;
        }
#endif

        return wxProcess::Exists( m_pid );
    }

    bool                m_ready = false;
    API_TEST_CLIENT     m_client;
    API_SERVER_PROCESS* m_process = nullptr;
    long                m_pid = 0;
    wxString            m_socketPath;
    wxString            m_socketUrl;
    wxString            m_stdout;
    wxString            m_stderr;
    mutable int         m_exitCode = 0;
};


/**
 * Per-test fixture for API E2E tests.
 *
 * All tests share a single api-server process and NNG client managed by API_SERVER_MANAGER.
 * The server and client are created on the first test's Start() call and remain alive for
 * all subsequent tests in the module.
 *
 * If the server crashed during a previous test, Start() automatically restarts it.
 */
class API_SERVER_E2E_FIXTURE
{
public:
    API_SERVER_E2E_FIXTURE()
    {
        m_cliPath = wxString::FromUTF8( QA_KICAD_CLI_PATH );
    }

    ~API_SERVER_E2E_FIXTURE() = default;

    bool Start( const wxString& aCliPathOverride = wxString() )
    {
        wxString cliPath = aCliPathOverride.IsEmpty() ? m_cliPath : aCliPathOverride;

        API_SERVER_MANAGER& manager = API_SERVER_MANAGER::Instance();

        if( manager.EnsureReady( cliPath, &m_lastError ) )
            return true;

        // First attempt failed. The server may have crashed during a previous test.
        manager.Reset();

        return manager.EnsureReady( cliPath, &m_lastError );
    }

    API_TEST_CLIENT& Client() { return API_SERVER_MANAGER::Instance().Client(); }

    wxString CliPath() const { return m_cliPath; }

    const wxString& LastError() const { return m_lastError; }

private:
    wxString m_cliPath;
    wxString m_lastError;
};

#endif // QA_API_E2E_UTILS_H
