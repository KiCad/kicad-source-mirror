/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Jon Evans <jon@craftyjon.com>
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <csignal>
#include <atomic>

#include <api/api_handler_common.h>
#include <api/api_server.h>
#include <cli/exit_codes.h>
#include <lib_id.h>
#include <settings/settings_manager.h>
#include <wildcards_and_files_ext.h>
#include <wx/app.h>
#include <wx/crt.h>
#include <wx/filename.h>

#include "command_api_server.h"

#define ARG_PATH "path"
#define ARG_SOCKET "--socket"


std::atomic_bool g_apiServerExitRequested{ false };

void apiServerSignalHandler( int )
{
    g_apiServerExitRequested.store( true );
}


CLI::API_SERVER_COMMAND::API_SERVER_COMMAND() :
        COMMAND( "api-server" )
{
    m_argParser.add_description( UTF8STDSTR( _( "Run the KiCad IPC API server in headless mode" ) ) );

    m_argParser.add_argument( ARG_PATH )
            .default_value( std::string() )
            .nargs( argparse::nargs_pattern::optional )
            .help( UTF8STDSTR( _( "Optional path to a .kicad_pro, .kicad_pcb, or .kicad_sch file to pre-load" ) ) )
            .metavar( "PROJECT_OR_FILE" );

    m_argParser.add_argument( ARG_SOCKET )
            .default_value( std::string() )
            .help( UTF8STDSTR( _( "Override API socket path" ) ) )
            .metavar( "SOCKET_PATH" );
}


int CLI::API_SERVER_COMMAND::doPerform( KIWAY& aKiway )
{
    using namespace kiapi::common;

    std::unique_ptr<KICAD_API_SERVER> server = std::make_unique<KICAD_API_SERVER>( false );
    API_HANDLER_COMMON                commonHandler;

    wxString socketPath = wxString::FromUTF8( m_argParser.get<std::string>( ARG_SOCKET ) );

    if( !socketPath.IsEmpty() )
        server->SetSocketPath( socketPath );

    types::DocumentType openDocumentType = types::DOCTYPE_UNKNOWN;
    KIWAY::FACE_T       openFace = KIWAY::FACE_PCB;
    wxFileName          openProjectPath;

    auto faceForDocument = []( types::DocumentType aType ) -> KIWAY::FACE_T
    {
        switch( aType )
        {
        case types::DOCTYPE_SCHEMATIC:  return KIWAY::FACE_SCH;
        case types::DOCTYPE_PCB:        return KIWAY::FACE_PCB;
        case types::DOCTYPE_FOOTPRINT: return KIWAY::FACE_PCB;
        default:                        return KIWAY::KIWAY_FACE_COUNT;
        }
    };

    auto docFileExtension = []( types::DocumentType aType ) -> std::string
    {
        switch( aType )
        {
        case types::DOCTYPE_SCHEMATIC:  return FILEEXT::KiCadSchematicFileExtension;
        case types::DOCTYPE_PCB:        return FILEEXT::KiCadPcbFileExtension;
        default:                        return "";
        }
    };

    auto closeCurrentDocument = [&]()
    {
        if( openDocumentType != types::DOCTYPE_UNKNOWN )
        {
            wxString docFileName;

            if( openDocumentType == types::DOCTYPE_PCB || openDocumentType == types::DOCTYPE_SCHEMATIC )
            {
                wxFileName docPath( openProjectPath );
                docPath.SetExt( docFileExtension( openDocumentType ) );
                docFileName = docPath.GetFullName();
            }

            wxString error;
            aKiway.ProcessApiCloseDocument( openFace, docFileName, server.get(), &error );
        }

        openProjectPath.Clear();
        openDocumentType = types::DOCTYPE_UNKNOWN;
    };

    auto openDocument = [&]( const commands::OpenDocument& aRequest )
            -> HANDLER_RESULT<commands::OpenDocumentResponse>
    {
        types::DocumentType requestType = aRequest.type();

        if( requestType != types::DOCTYPE_PCB && requestType != types::DOCTYPE_SCHEMATIC
            && requestType != types::DOCTYPE_PROJECT && requestType != types::DOCTYPE_FOOTPRINT )
        {
            ApiResponseStatus e;
            e.set_status( ApiStatusCode::AS_UNIMPLEMENTED );
            e.set_error_message( "Only PCB, schematic, footprint, and project document types are supported" );
            return tl::unexpected( e );
        }

        wxString inputPath = wxString::FromUTF8( aRequest.path() );

        if( inputPath.IsEmpty() )
        {
            ApiResponseStatus e;
            e.set_status( ApiStatusCode::AS_BAD_REQUEST );
            e.set_error_message( "OpenDocument requires a non-empty path" );
            return tl::unexpected( e );
        }

        closeCurrentDocument();

        KIWAY::FACE_T face = faceForDocument( requestType );
        wxString      error;

        if( face == KIWAY::KIWAY_FACE_COUNT )
        {
            ApiResponseStatus e;
            e.set_status( ApiStatusCode::AS_BAD_REQUEST );
            e.set_error_message( "unsupported document type" );
            return tl::unexpected( e );
        }

        wxFileName projectPath;
        wxString   openPath;

        projectPath = wxFileName( inputPath );

        // TODO(JE) if the API client just gives a project path rather than sch/board,
        // we won't dispatch correctly.  We could instead try both handlers until one
        // succeeds, like we do with other API calls.

        projectPath.SetExt( FILEEXT::ProjectFileExtension );
        projectPath.MakeAbsolute();
        openPath = projectPath.GetFullPath();

        if( !aKiway.ProcessApiOpenDocument( face, openPath, server.get(), &error ) )
        {
            ApiResponseStatus e;
            e.set_status( ApiStatusCode::AS_BAD_REQUEST );
            e.set_error_message( error.ToStdString() );
            return tl::unexpected( e );
        }

        openProjectPath = projectPath;
        openDocumentType = requestType;
        openFace = face;

        commands::OpenDocumentResponse response;
        types::DocumentSpecifier*      doc = response.mutable_document();
        PROJECT&                       project = Pgm().GetSettingsManager().Prj();

        doc->set_type( openDocumentType );

        if( openDocumentType == types::DOCTYPE_PCB )
        {
            wxFileName boardPath( openProjectPath );
            boardPath.SetExt( FILEEXT::KiCadPcbFileExtension );
            doc->set_board_filename( boardPath.GetFullName().ToStdString() );
        }
        else if( openDocumentType == types::DOCTYPE_SCHEMATIC )
        {
            // TODO(JE) stateful sheet path handling?
        }

        doc->mutable_project()->set_name( project.GetProjectName().ToUTF8() );
        doc->mutable_project()->set_path( project.GetProjectPath().ToUTF8() );

        return response;
    };

    auto closeDocument =
            [&]( const commands::CloseDocument& aRequest ) -> HANDLER_RESULT<google::protobuf::Empty>
    {
        if( openDocumentType == types::DOCTYPE_UNKNOWN )
        {
            ApiResponseStatus e;
            e.set_status( ApiStatusCode::AS_BAD_REQUEST );
            e.set_error_message( "No document is currently open" );
            return tl::unexpected( e );
        }

        if( aRequest.has_document() )
        {
            if( aRequest.document().type() != openDocumentType )
            {
                ApiResponseStatus e;
                e.set_status( ApiStatusCode::AS_BAD_REQUEST );
                e.set_error_message( "Requested document type does not match the open document" );
                return tl::unexpected( e );
            }

            wxFileName expectedPath( openProjectPath );
            expectedPath.SetExt( docFileExtension( openDocumentType ) );

            wxString requestedName;

            if( openDocumentType == types::DOCTYPE_PCB
                && !aRequest.document().board_filename().empty() )
            {
                requestedName = wxString::FromUTF8( aRequest.document().board_filename() );
            }
            else if( openDocumentType == types::DOCTYPE_SCHEMATIC
                     && !aRequest.document().project().path().empty() )
            {
                requestedName = wxString::FromUTF8( aRequest.document().project().name() )
                                + FILEEXT::KiCadSchematicFileExtension;
            }

            if( !requestedName.IsEmpty() && expectedPath.GetFullName() != requestedName )
            {
                ApiResponseStatus e;
                e.set_status( ApiStatusCode::AS_BAD_REQUEST );
                e.set_error_message( "Requested document does not match the open document" );
                return tl::unexpected( e );
            }
        }

        wxString docFileName;

        if( openDocumentType == types::DOCTYPE_PCB || openDocumentType == types::DOCTYPE_SCHEMATIC )
        {
            wxFileName expectedPath( openProjectPath );
            expectedPath.SetExt( docFileExtension( openDocumentType ) );
            docFileName = expectedPath.GetFullName();
        }

        wxString error;

        if( !aKiway.ProcessApiCloseDocument( openFace, docFileName, server.get(), &error ) )
        {
            ApiResponseStatus e;
            e.set_status( ApiStatusCode::AS_BAD_REQUEST );
            e.set_error_message( error.ToStdString() );
            return tl::unexpected( e );
        }

        openProjectPath.Clear();
        openDocumentType = types::DOCTYPE_UNKNOWN;

        return google::protobuf::Empty();
    };

    commonHandler.SetOpenDocumentHandler( openDocument );
    commonHandler.SetCloseDocumentHandler( closeDocument );

    server->RegisterHandler( &commonHandler );
    server->Start();

    if( !server->Running() )
    {
        wxFprintf( stderr, _( "Failed to start API server\n" ) );
        return EXIT_CODES::ERR_UNKNOWN;
    }

    wxString preloadPath = wxString::FromUTF8( m_argParser.get<std::string>( ARG_PATH ) );

    if( !preloadPath.IsEmpty() )
    {
        using namespace kiapi::common;

        wxFileName preloadFile( preloadPath );
        types::DocumentType preloadType = types::DOCTYPE_PROJECT;

        if( preloadFile.GetExt() == FILEEXT::KiCadSchematicFileExtension )
            preloadType = types::DOCTYPE_SCHEMATIC;
        else if( preloadFile.GetExt() == FILEEXT::KiCadPcbFileExtension )
            preloadType = types::DOCTYPE_PCB;

        commands::OpenDocument request;
        request.set_type( preloadType );
        request.set_path( preloadPath.ToStdString() );

        auto preloadResult = openDocument( request );

        if( !preloadResult )
        {
            wxFprintf( stderr, "%s\n", preloadResult.error().error_message() );
            server->DeregisterHandler( &commonHandler );
            return EXIT_CODES::ERR_ARGS;
        }
    }

    server->SetReadyToReply( true );

    wxString listenPath = wxString::FromUTF8( server->SocketPath() );
    wxFprintf( stdout, "KiCad API server listening at %s\n", listenPath );

    auto oldSigInt = std::signal( SIGINT, apiServerSignalHandler );
#ifdef SIGTERM
    auto oldSigTerm = std::signal( SIGTERM, apiServerSignalHandler );
#endif

    g_apiServerExitRequested.store( false );

    while( !g_apiServerExitRequested.load() )
    {
        wxTheApp->ProcessPendingEvents();
        wxMilliSleep( 10 );
    }

    std::signal( SIGINT, oldSigInt );
#ifdef SIGTERM
    std::signal( SIGTERM, oldSigTerm );
#endif

    wxFprintf( stdout, "Shutting down\n" );

    closeCurrentDocument();
    server->DeregisterHandler( &commonHandler );

    return EXIT_CODES::OK;
}
