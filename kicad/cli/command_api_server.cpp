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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <csignal>
#include <atomic>
#include <algorithm>
#include <vector>

#include <api/api_handler_common.h>
#include <api/api_utils.h>
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

    // Eventually we might support opening multiple projects at once, but for now
    // we support one project at a time, but multiple documents within that project
    // (e.g. up to one schematic, up to one board, and arbitrarily many library files
    // which are not associated with the project)
    std::optional<wxFileName> openProjectPath;

    struct OPEN_DOCUMENT
    {
        types::DocumentType type;
        wxString            fileName;
    };

    std::vector<OPEN_DOCUMENT> openDocuments;

    auto faceForDocument = []( types::DocumentType aType ) -> KIWAY::FACE_T
    {
        switch( aType )
        {
        case types::DOCTYPE_SCHEMATIC:  return KIWAY::FACE_SCH;
        case types::DOCTYPE_PCB:        return KIWAY::FACE_PCB;
        case types::DOCTYPE_FOOTPRINT:  return KIWAY::FACE_PCB;
        default:                        return KIWAY::KIWAY_FACE_COUNT;
        }
    };

    auto closeAllDocuments =
            [&]( const commands::CloseAllDocuments& aRequest ) -> HANDLER_RESULT<google::protobuf::Empty>
    {
        for( const OPEN_DOCUMENT& doc : openDocuments )
        {
            wxString error;
            aKiway.ProcessApiCloseDocument( faceForDocument( doc.type ), doc.fileName, server.get(), &error );
        }

        openDocuments.clear();
        openProjectPath.reset();

        return google::protobuf::Empty();
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

        wxFileName projectPath( inputPath );
        projectPath.SetExt( FILEEXT::ProjectFileExtension );
        projectPath.MakeAbsolute();

        if( openProjectPath && projectPath.GetFullPath() != openProjectPath->GetFullPath() )
        {
            ApiResponseStatus e;
            e.set_status( ApiStatusCode::AS_BAD_REQUEST );
            e.set_error_message( wxString::Format( "cannot open a document from project '%s' because project "
                                                   "'%s' is already open.",
                                                   projectPath.GetFullName(), openProjectPath->GetFullName() )
                                         .ToStdString() );
            return tl::unexpected( e );
        }

        if( requestType == types::DOCTYPE_PROJECT )
        {
            if( !openProjectPath )
            {
                if( !Pgm().GetSettingsManager().LoadProject( projectPath.GetFullPath(), true ) )
                {
                    wxLogTrace( traceApi, "Warning: no project file found for %s", inputPath );
                }

                if( !Pgm().GetSettingsManager().GetProject( projectPath.GetFullPath() ) )
                {
                    ApiResponseStatus e;
                    e.set_status( ApiStatusCode::AS_BAD_REQUEST );
                    e.set_error_message( wxString::Format( "failed to load project '%s'", projectPath.GetFullPath() )
                                                 .ToStdString() );
                    return tl::unexpected( e );
                }

                openProjectPath = projectPath;
            }

            commands::OpenDocumentResponse response;
            types::DocumentSpecifier*      doc = response.mutable_document();
            PROJECT&                       project = Pgm().GetSettingsManager().Prj();

            doc->set_type( types::DOCTYPE_PROJECT );
            doc->mutable_project()->set_name( project.GetProjectName().ToUTF8() );
            doc->mutable_project()->set_path( project.GetProjectPath().ToUTF8() );

            return response;
        }

        KIWAY::FACE_T face = faceForDocument( requestType );

        if( face == KIWAY::KIWAY_FACE_COUNT )
        {
            ApiResponseStatus e;
            e.set_status( ApiStatusCode::AS_BAD_REQUEST );
            e.set_error_message( "unsupported document type" );
            return tl::unexpected( e );
        }

        if( requestType == types::DOCTYPE_PCB || requestType == types::DOCTYPE_SCHEMATIC )
        {
            auto existing = std::ranges::find_if( openDocuments,
                                                  [&]( const OPEN_DOCUMENT& d )
                                                  {
                                                      return d.type == requestType;
                                                  } );

            if( existing != openDocuments.end() )
            {
                ApiResponseStatus e;
                e.set_status( ApiStatusCode::AS_BAD_REQUEST );
                e.set_error_message( "a document of this type is already open" );
                return tl::unexpected( e );
            }
        }

        wxString error;

        if( !aKiway.ProcessApiOpenDocument( face, projectPath.GetFullPath(), server.get(), &error ) )
        {
            ApiResponseStatus e;
            e.set_status( ApiStatusCode::AS_BAD_REQUEST );
            e.set_error_message( error.ToStdString() );
            return tl::unexpected( e );
        }

        wxFileName docFile( inputPath );
        docFile.MakeAbsolute();

        OPEN_DOCUMENT doc;
        doc.type = requestType;
        doc.fileName = docFile.GetFullName();
        openDocuments.push_back( doc );

        openProjectPath = projectPath;

        commands::OpenDocumentResponse response;
        types::DocumentSpecifier*      docSpec = response.mutable_document();
        PROJECT&                       project = Pgm().GetSettingsManager().Prj();

        docSpec->set_type( requestType );

        if( requestType == types::DOCTYPE_PCB )
            docSpec->set_board_filename( doc.fileName.ToStdString() );

        docSpec->mutable_project()->set_name( project.GetProjectName().ToUTF8() );
        docSpec->mutable_project()->set_path( project.GetProjectPath().ToUTF8() );

        return response;
    };

    auto closeDocument =
            [&]( const commands::CloseDocument& aRequest ) -> HANDLER_RESULT<google::protobuf::Empty>
    {
        if( openDocuments.empty() )
        {
            ApiResponseStatus e;
            e.set_status( ApiStatusCode::AS_BAD_REQUEST );
            e.set_error_message( "No document is currently open" );
            return tl::unexpected( e );
        }

        auto it = openDocuments.end();

        if( aRequest.has_document() )
        {
            types::DocumentType typeToClose = aRequest.document().type();

            it = std::ranges::find_if( openDocuments,
                                       [&]( const OPEN_DOCUMENT& d )
                                       {
                                           return d.type == typeToClose;
                                       } );

            if( it == openDocuments.end() )
            {
                ApiResponseStatus e;
                e.set_status( ApiStatusCode::AS_BAD_REQUEST );
                e.set_error_message( "Requested document type does not match any open document" );
                return tl::unexpected( e );
            }

            if( typeToClose == types::DOCTYPE_PCB
                && !aRequest.document().board_filename().empty() )
            {
                wxString requestedName = wxString::FromUTF8( aRequest.document().board_filename() );

                if( it->fileName != requestedName )
                {
                    ApiResponseStatus e;
                    e.set_status( ApiStatusCode::AS_BAD_REQUEST );
                    e.set_error_message( "Requested document does not match the open document" );
                    return tl::unexpected( e );
                }
            }
        }
        else
        {
            // No document specifier: close the first open document.
            it = openDocuments.begin();
        }

        wxString error;

        if( !aKiway.ProcessApiCloseDocument( faceForDocument( it->type ), it->fileName, server.get(), &error ) )
        {
            ApiResponseStatus e;
            e.set_status( ApiStatusCode::AS_BAD_REQUEST );
            e.set_error_message( error.ToStdString() );
            return tl::unexpected( e );
        }

        openDocuments.erase( it );

        if( openDocuments.empty() )
        {
            PROJECT& project = Pgm().GetSettingsManager().Prj();
            Pgm().GetSettingsManager().UnloadProject( &project, false );
            openProjectPath.reset();
        }

        return google::protobuf::Empty();
    };

    commonHandler.SetOpenDocumentHandler( openDocument );
    commonHandler.SetCloseDocumentHandler( closeDocument );
    commonHandler.SetCloseAllDocumentsHandler( closeAllDocuments );

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

    commands::CloseAllDocuments closeAllReq;
    closeAllDocuments( closeAllReq );
    server->DeregisterHandler( &commonHandler );

    return EXIT_CODES::OK;
}
