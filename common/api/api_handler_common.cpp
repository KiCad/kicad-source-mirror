/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Jon Evans <jon@craftyjon.com>
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

#include <optional>
#include <ranges>
#include <set>
#include <tuple>

#include <api/api_handler_common.h>
#include <build_version.h>
#include <eda_shape.h>
#include <eda_text.h>
#include <gestfich.h>
#include <geometry/shape_compound.h>
#include <google/protobuf/empty.pb.h>
#include <paths.h>
#include <pgm_base.h>
#include <api/api_plugin.h>
#include <api/api_utils.h>
#include <project/net_settings.h>
#include <project/project_file.h>
#include <settings/settings_manager.h>
#include <wx/string.h>

using namespace kiapi::common::commands;
using namespace kiapi::common::types;
using google::protobuf::Empty;


API_HANDLER_COMMON::API_HANDLER_COMMON() :
        API_HANDLER()
{
    registerHandler<commands::GetVersion, GetVersionResponse>( &API_HANDLER_COMMON::handleGetVersion );
    registerHandler<GetKiCadBinaryPath, PathResponse>(
            &API_HANDLER_COMMON::handleGetKiCadBinaryPath );
    registerHandler<GetPaths, GetPathsResponse>( &API_HANDLER_COMMON::handleGetPaths );
    registerHandler<GetNetClasses, NetClassesResponse>( &API_HANDLER_COMMON::handleGetNetClasses );
    registerHandler<SetNetClasses, Empty>( &API_HANDLER_COMMON::handleSetNetClasses );
    registerHandler<GetNetClassAssignments, NetClassAssignmentsResponse>(
            &API_HANDLER_COMMON::handleGetNetClassAssignments );
    registerHandler<SetNetClassAssignments, Empty>( &API_HANDLER_COMMON::handleSetNetClassAssignments );
    registerHandler<Ping, Empty>( &API_HANDLER_COMMON::handlePing );
    registerHandler<GetTextExtents, types::Box2>( &API_HANDLER_COMMON::handleGetTextExtents );
    registerHandler<GetTextAsShapes, GetTextAsShapesResponse>(
            &API_HANDLER_COMMON::handleGetTextAsShapes );
    registerHandler<ExpandTextVariables, ExpandTextVariablesResponse>(
            &API_HANDLER_COMMON::handleExpandTextVariables );
    registerHandler<GetPluginSettingsPath, StringResponse>(
            &API_HANDLER_COMMON::handleGetPluginSettingsPath );
    registerHandler<GetTextVariables, project::TextVariables>(
            &API_HANDLER_COMMON::handleGetTextVariables );
    registerHandler<SetTextVariables, Empty>(
            &API_HANDLER_COMMON::handleSetTextVariables );
    registerHandler<OpenDocument, OpenDocumentResponse>(
            &API_HANDLER_COMMON::handleOpenDocument );
    registerHandler<CloseDocument, Empty>(
            &API_HANDLER_COMMON::handleCloseDocument );
    registerHandler<CloseAllDocuments, Empty>(
            &API_HANDLER_COMMON::handleCloseAllDocuments );
    registerHandler<CreateDocument, OpenDocumentResponse>(
            &API_HANDLER_COMMON::handleCreateDocument );
}


HANDLER_RESULT<GetVersionResponse> API_HANDLER_COMMON::handleGetVersion(
        const HANDLER_CONTEXT<commands::GetVersion>& )
{
    GetVersionResponse reply;

    reply.mutable_version()->set_full_version( GetBuildVersion().ToStdString() );

    std::tuple<int, int, int> version = GetMajorMinorPatchTuple();
    reply.mutable_version()->set_major( std::get<0>( version ) );
    reply.mutable_version()->set_minor( std::get<1>( version ) );
    reply.mutable_version()->set_patch( std::get<2>( version ) );

    return reply;
}


HANDLER_RESULT<PathResponse> API_HANDLER_COMMON::handleGetKiCadBinaryPath(
        const HANDLER_CONTEXT<GetKiCadBinaryPath>& aCtx )
{
    wxFileName fn( wxEmptyString, wxString::FromUTF8( aCtx.Request.binary_name() ) );
#ifdef _WIN32
    fn.SetExt( wxT( "exe" ) );
#endif

    wxString path = FindKicadFile( fn.GetFullName() );
    PathResponse reply;
    reply.set_path( path.ToUTF8() );
    return reply;
}


tl::expected<bool, ApiResponseStatus> API_HANDLER_COMMON::validateProject( const ProjectSpecifier& aProject,
                                                                           bool aAllowEmpty )
{
    if( !aAllowEmpty && ( aProject.name().empty() || aProject.path().empty() ) )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( "a project name and path must be specified" );
        return tl::unexpected( e );
    }

    const PROJECT& prj = Pgm().GetSettingsManager().Prj();

    if( aProject.name().compare( prj.GetProjectName().ToUTF8() ) != 0 )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( fmt::format( "the requested project {} is not open", aProject.name() ) );
        return tl::unexpected( e );
    }

    if( aProject.path().compare( prj.GetProjectPath().ToUTF8() ) != 0 )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( fmt::format( "the requested project {} is not open at path {}", aProject.name(),
                                          aProject.path() ) );
        return tl::unexpected( e );
    }

    return true;
}


HANDLER_RESULT<NetClassesResponse> API_HANDLER_COMMON::handleGetNetClasses( const HANDLER_CONTEXT<GetNetClasses>& aCtx )
{
    if( tl::expected<bool, ApiResponseStatus> result = validateProject( aCtx.Request.project(), true ); !result )
        return tl::unexpected( result.error() );

    NetClassesResponse reply;

    std::shared_ptr<NET_SETTINGS>& netSettings =
            Pgm().GetSettingsManager().Prj().GetProjectFile().m_NetSettings;

    google::protobuf::Any any;

    netSettings->GetDefaultNetclass()->Serialize( any );
    any.UnpackTo( reply.add_net_classes() );

    for( const auto& netClass : netSettings->GetNetclasses() | std::views::values )
    {
        netClass->Serialize( any );
        any.UnpackTo( reply.add_net_classes() );
    }

    return reply;
}


HANDLER_RESULT<Empty> API_HANDLER_COMMON::handleSetNetClasses( const HANDLER_CONTEXT<SetNetClasses>& aCtx )
{
    if( tl::expected<bool, ApiResponseStatus> result = validateProject( aCtx.Request.project(), true ); !result )
        return tl::unexpected( result.error() );

    std::shared_ptr<NET_SETTINGS>& netSettings =
            Pgm().GetSettingsManager().Prj().GetProjectFile().m_NetSettings;

    if( aCtx.Request.merge_mode() == MapMergeMode::MMM_REPLACE )
        netSettings->ClearNetclasses();

    auto netClasses = netSettings->GetNetclasses();
    google::protobuf::Any any;

    for( const auto& ncProto : aCtx.Request.net_classes() )
    {
        any.PackFrom( ncProto );
        wxString name = wxString::FromUTF8( ncProto.name() );

        bool deserialized = false;

        if( name == wxT( "Default" ) )
        {
            deserialized = netSettings->GetDefaultNetclass()->Deserialize( any );
        }
        else
        {
            if( !netClasses.contains( name ) )
                netClasses.insert( { name, std::make_shared<NETCLASS>( name, false ) } );

            deserialized = netClasses[name]->Deserialize( any );
        }

        if( !deserialized )
        {
            ApiResponseStatus e;
            e.set_status( ApiStatusCode::AS_BAD_REQUEST );
            e.set_error_message( fmt::format( "could not unpack netclass '{}'", name.ToUTF8().data() ) );
            return tl::unexpected( e );
        }
    }

    netSettings->SetNetclasses( netClasses );
    requestNetSettingsNotification();

    return Empty();
}


HANDLER_RESULT<NetClassAssignmentsResponse>
API_HANDLER_COMMON::handleGetNetClassAssignments( const HANDLER_CONTEXT<GetNetClassAssignments>& aCtx )
{
    if( tl::expected<bool, ApiResponseStatus> result = validateProject( aCtx.Request.project() ); !result )
        return tl::unexpected( result.error() );

    std::shared_ptr<NET_SETTINGS>& netSettings = Pgm().GetSettingsManager().Prj().GetProjectFile().m_NetSettings;

    NetClassAssignmentsResponse reply;

    for( const auto& [netName, netclassNames] : netSettings->GetNetclassLabelAssignments() )
    {
        project::NetClassAssignment* assignment = reply.add_assignments();
        assignment->set_net( netName.ToUTF8() );

        for( const wxString& netclassName : netclassNames )
            assignment->add_netclasses( netclassName.ToUTF8() );
    }

    for( const auto& [matcher, netclassName] : netSettings->GetNetclassPatternAssignments() )
    {
        project::NetClassPatternAssignment* pattern = reply.add_pattern_assignments();
        pattern->set_pattern( matcher->GetPattern().ToUTF8() );
        pattern->set_netclass( netclassName.ToUTF8() );
    }

    return reply;
}


HANDLER_RESULT<Empty>
API_HANDLER_COMMON::handleSetNetClassAssignments( const HANDLER_CONTEXT<SetNetClassAssignments>& aCtx )
{
    if( tl::expected<bool, ApiResponseStatus> result = validateProject( aCtx.Request.project() ); !result )
        return tl::unexpected( result.error() );

    std::shared_ptr<NET_SETTINGS>& netSettings = Pgm().GetSettingsManager().Prj().GetProjectFile().m_NetSettings;

    std::set<wxString, std::less<>> knownNetclasses;
    knownNetclasses.insert( NETCLASS::Default );

    for( const wxString& name : netSettings->GetNetclasses() | std::views::keys )
        knownNetclasses.insert( name );

    auto checkNetclassName = [&]( const std::string& aName, const char* aKind ) -> std::optional<ApiResponseStatus>
    {
        if( knownNetclasses.contains( wxString::FromUTF8( aName ) ) )
            return std::nullopt;

        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( fmt::format( "unknown netclass '{}' in {} assignment", aName, aKind ) );
        return e;
    };

    if( aCtx.Request.merge_mode() == MapMergeMode::MMM_REPLACE )
    {
        netSettings->ClearNetclassLabelAssignments();
        netSettings->ClearNetclassPatternAssignments();
    }

    for( const project::NetClassAssignment& assignment : aCtx.Request.assignments() )
    {
        if( assignment.net().empty() )
        {
            ApiResponseStatus e;
            e.set_status( ApiStatusCode::AS_BAD_REQUEST );
            e.set_error_message( "net name cannot be empty in a netclass assignment" );
            return tl::unexpected( e );
        }

        for( const std::string& netclassName : assignment.netclasses() )
        {
            if( std::optional<ApiResponseStatus> err = checkNetclassName( netclassName, "net" ) )
                return tl::unexpected( *err );
        }

        std::set<wxString> netclasses;

        for( const std::string& netclassName : assignment.netclasses() )
            netclasses.insert( wxString::FromUTF8( netclassName ) );

        if( netclasses.empty() )
            netSettings->ClearNetclassLabelAssignment( wxString::FromUTF8( assignment.net() ) );
        else
            netSettings->SetNetclassLabelAssignment( wxString::FromUTF8( assignment.net() ), netclasses );
    }

    for( const project::NetClassPatternAssignment& pattern : aCtx.Request.pattern_assignments() )
    {
        if( pattern.pattern().empty() )
        {
            ApiResponseStatus e;
            e.set_status( ApiStatusCode::AS_BAD_REQUEST );
            e.set_error_message( "pattern cannot be empty in a netclass pattern assignment" );
            return tl::unexpected( e );
        }

        if( !pattern.netclass().empty() )
        {
            if( std::optional<ApiResponseStatus> err = checkNetclassName( pattern.netclass(), "pattern" ) )
                return tl::unexpected( *err );
        }

        std::vector<std::pair<std::unique_ptr<EDA_COMBINED_MATCHER>, wxString>> kept;

        for( auto& existing : netSettings->GetNetclassPatternAssignments() )
        {
            if( existing.first->GetPattern() != wxString::FromUTF8( pattern.pattern() ) )
                kept.emplace_back( std::move( existing ) );
        }

        netSettings->SetNetclassPatternAssignments( std::move( kept ) );

        if( !pattern.netclass().empty() )
        {
            netSettings->SetNetclassPatternAssignment( wxString::FromUTF8( pattern.pattern() ),
                                                       wxString::FromUTF8( pattern.netclass() ) );
        }
    }

    netSettings->ClearAllCaches();
    requestNetSettingsNotification();

    return Empty();
}


HANDLER_RESULT<Empty> API_HANDLER_COMMON::handlePing( const HANDLER_CONTEXT<Ping>& aCtx )
{
    return Empty();
}


HANDLER_RESULT<types::Box2> API_HANDLER_COMMON::handleGetTextExtents(
        const HANDLER_CONTEXT<GetTextExtents>& aCtx )
{
    EDA_TEXT text( pcbIUScale );
    google::protobuf::Any any;
    any.PackFrom( aCtx.Request.text() );

    if( !text.Deserialize( any ) )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( "Could not decode text in GetTextExtents message" );
        return tl::unexpected( e );
    }

    types::Box2 response;

    BOX2I bbox = text.GetTextBox( nullptr );
    EDA_ANGLE angle = text.GetTextAngle();

    if( !angle.IsZero() )
        bbox = bbox.GetBoundingBoxRotated( text.GetTextPos(), text.GetTextAngle() );

    response.mutable_position()->set_x_nm( bbox.GetPosition().x );
    response.mutable_position()->set_y_nm( bbox.GetPosition().y );
    response.mutable_size()->set_x_nm( bbox.GetSize().x );
    response.mutable_size()->set_y_nm( bbox.GetSize().y );

    return response;
}


HANDLER_RESULT<GetTextAsShapesResponse> API_HANDLER_COMMON::handleGetTextAsShapes(
        const HANDLER_CONTEXT<GetTextAsShapes>& aCtx )
{
    GetTextAsShapesResponse reply;

    for( const TextOrTextBox& textMsg : aCtx.Request.text() )
    {
        Text dummyText;
        const Text* textPtr = &textMsg.text();

        if( textMsg.has_textbox() )
        {
            dummyText.set_text( textMsg.textbox().text() );
            dummyText.mutable_attributes()->CopyFrom( textMsg.textbox().attributes() );
            textPtr = &dummyText;
        }

        EDA_TEXT text( pcbIUScale );
        google::protobuf::Any any;
        any.PackFrom( *textPtr );

        if( !text.Deserialize( any ) )
        {
            ApiResponseStatus e;
            e.set_status( ApiStatusCode::AS_BAD_REQUEST );
            e.set_error_message( "Could not decode text in GetTextAsShapes message" );
            return tl::unexpected( e );
        }

        std::shared_ptr<SHAPE_COMPOUND> shapes = text.GetEffectiveTextShape( false );

        TextWithShapes* entry = reply.add_text_with_shapes();
        entry->mutable_text()->CopyFrom( textMsg );

        for( SHAPE* subshape : shapes->Shapes() )
        {
            EDA_SHAPE proxy( *subshape );
            proxy.Serialize( any );
            GraphicShape* shapeMsg = entry->mutable_shapes()->add_shapes();
            any.UnpackTo( shapeMsg );
        }

        if( textMsg.has_textbox() && textMsg.textbox().border_enabled() )
        {
            GraphicShape* border = entry->mutable_shapes()->add_shapes();
            int width = textMsg.textbox().attributes().stroke_width().value_nm();
            border->mutable_attributes()->mutable_stroke()->mutable_width()->set_value_nm( width );
            VECTOR2I tl = UnpackVector2( textMsg.textbox().top_left() );
            VECTOR2I br = UnpackVector2( textMsg.textbox().bottom_right() );

            // top
            PackVector2( *border->mutable_segment()->mutable_start(), tl );
            PackVector2( *border->mutable_segment()->mutable_end(), VECTOR2I( br.x, tl.y ) );

            // right
            border = entry->mutable_shapes()->add_shapes();
            border->mutable_attributes()->mutable_stroke()->mutable_width()->set_value_nm( width );
            PackVector2( *border->mutable_segment()->mutable_start(), VECTOR2I( br.x, tl.y ) );
            PackVector2( *border->mutable_segment()->mutable_end(), br );

            // bottom
            border = entry->mutable_shapes()->add_shapes();
            border->mutable_attributes()->mutable_stroke()->mutable_width()->set_value_nm( width );
            PackVector2( *border->mutable_segment()->mutable_start(), br );
            PackVector2( *border->mutable_segment()->mutable_end(), VECTOR2I( tl.x, br.y ) );

            // left
            border = entry->mutable_shapes()->add_shapes();
            border->mutable_attributes()->mutable_stroke()->mutable_width()->set_value_nm( width );
            PackVector2( *border->mutable_segment()->mutable_start(), VECTOR2I( tl.x, br.y ) );
            PackVector2( *border->mutable_segment()->mutable_end(), tl );
        }
    }

    return reply;
}


HANDLER_RESULT<ExpandTextVariablesResponse> API_HANDLER_COMMON::handleExpandTextVariables(
        const HANDLER_CONTEXT<ExpandTextVariables>& aCtx )
{
    if( !aCtx.Request.has_document() || aCtx.Request.document().type() != DOCTYPE_PROJECT )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_UNHANDLED );
        // No error message, this is a flag that the server should try a different handler
        return tl::unexpected( e );
    }

    ExpandTextVariablesResponse reply;
    PROJECT& project = Pgm().GetSettingsManager().Prj();

    for( const std::string& textMsg : aCtx.Request.text() )
    {
        wxString result = ExpandTextVars( wxString::FromUTF8( textMsg ), &project, INTERNAL );

        if( aCtx.Request.expand_env_vars() )
            result = ExpandEnvVarSubstitutions( result, &project );

        reply.add_text( result.ToUTF8() );
    }

    return reply;
}


HANDLER_RESULT<StringResponse> API_HANDLER_COMMON::handleGetPluginSettingsPath(
        const HANDLER_CONTEXT<GetPluginSettingsPath>& aCtx )
{
    wxString identifier = wxString::FromUTF8( aCtx.Request.identifier() );

    if( identifier.IsEmpty() )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( "plugin identifier is missing" );
        return tl::unexpected( e );
    }

    if( !API_PLUGIN::IsValidIdentifier( identifier ) )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( "plugin identifier is invalid" );
        return tl::unexpected( e );
    }

    wxFileName path( PATHS::GetUserSettingsPath(), wxEmptyString );
    path.AppendDir( "plugins" );

    // Create the base plugins path if needed, but leave the specific plugin to create its own path
    PATHS::EnsurePathExists( path.GetPath() );

    path.AppendDir( identifier );

    StringResponse reply;
    reply.set_response( path.GetPath() );
    return reply;
}


HANDLER_RESULT<project::TextVariables> API_HANDLER_COMMON::handleGetTextVariables(
        const HANDLER_CONTEXT<GetTextVariables>& aCtx )
{
    if( !aCtx.Request.has_document() || aCtx.Request.document().type() != DOCTYPE_PROJECT )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_UNHANDLED );
        // No error message, this is a flag that the server should try a different handler
        return tl::unexpected( e );
    }

    if( tl::expected<bool, ApiResponseStatus> result = validateProject( aCtx.Request.document().project() );
        !result )
    {
        return tl::unexpected( result.error() );
    }

    const PROJECT& project = Pgm().GetSettingsManager().Prj();

    if( project.IsNullProject() )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_NOT_READY );
        e.set_error_message( "no valid project is loaded, cannot get text variables" );
        return tl::unexpected( e );
    }

    const std::map<wxString, wxString>& vars = project.GetTextVars();

    project::TextVariables reply;
    auto map = reply.mutable_variables();

    for( const auto& [key, value] : vars )
        ( *map )[ std::string( key.ToUTF8() ) ] = value.ToUTF8();

    return reply;
}


HANDLER_RESULT<Empty> API_HANDLER_COMMON::handleSetTextVariables(
    const HANDLER_CONTEXT<SetTextVariables>& aCtx )
{
    if( !aCtx.Request.has_document() || aCtx.Request.document().type() != DOCTYPE_PROJECT )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_UNHANDLED );
        // No error message, this is a flag that the server should try a different handler
        return tl::unexpected( e );
    }

    if( tl::expected<bool, ApiResponseStatus> result = validateProject( aCtx.Request.document().project() );
        !result )
    {
        return tl::unexpected( result.error() );
    }

    PROJECT& project = Pgm().GetSettingsManager().Prj();

    if( project.IsNullProject() )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_NOT_READY );
        e.set_error_message( "no valid project is loaded, cannot set text variables" );
        return tl::unexpected( e );
    }

    const project::TextVariables& newVars = aCtx.Request.variables();
    std::map<wxString, wxString>& vars = project.GetTextVars();

    if( aCtx.Request.merge_mode() == MapMergeMode::MMM_REPLACE )
        vars.clear();

    for( const auto& [key, value] : newVars.variables() )
        vars[wxString::FromUTF8( key )] = wxString::FromUTF8( value );

    if( !Pgm().GetSettingsManager().SaveProject() )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_INTERNAL_ERROR );
        e.set_error_message( "failed to save project text variables" );
        return tl::unexpected( e );
    }

    return Empty();
}


HANDLER_RESULT<OpenDocumentResponse> API_HANDLER_COMMON::handleOpenDocument(
        const HANDLER_CONTEXT<OpenDocument>& aCtx )
{
    if( !m_openDocumentHandler )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_UNIMPLEMENTED );
        e.set_error_message( "OpenDocument is not available in this KiCad mode" );
        return tl::unexpected( e );
    }

    return m_openDocumentHandler( aCtx.Request );
}


HANDLER_RESULT<Empty> API_HANDLER_COMMON::handleCloseDocument(
        const HANDLER_CONTEXT<CloseDocument>& aCtx )
{
    if( !m_closeDocumentHandler )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_UNIMPLEMENTED );
        e.set_error_message( "CloseDocument is not available in this KiCad mode" );
        return tl::unexpected( e );
    }

    return m_closeDocumentHandler( aCtx.Request );
}


HANDLER_RESULT<OpenDocumentResponse>
API_HANDLER_COMMON::handleCreateDocument( const HANDLER_CONTEXT<CreateDocument>& aCtx )
{
    if( !m_createDocumentHandler )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_UNIMPLEMENTED );
        e.set_error_message( "CreateDocument is not available in this KiCad mode" );
        return tl::unexpected( e );
    }

    return m_createDocumentHandler( aCtx.Request );
}


HANDLER_RESULT<Empty> API_HANDLER_COMMON::handleCloseAllDocuments( const HANDLER_CONTEXT<CloseAllDocuments>& aCtx )
{
    if( !m_closeAllDocumentsHandler )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_UNIMPLEMENTED );
        e.set_error_message( "CloseAllDocuments is not available in this KiCad mode" );
        return tl::unexpected( e );
    }

    return m_closeAllDocumentsHandler( aCtx.Request );
}


HANDLER_RESULT<GetPathsResponse> API_HANDLER_COMMON::handleGetPaths( const HANDLER_CONTEXT<GetPaths>& )
{
    GetPathsResponse reply;

    auto addPath = [&]( types::PathType aType, const wxString& aPath )
    {
        PathEntry* entry = reply.add_paths();
        entry->set_type( aType );
        entry->set_path( aPath.ToUTF8() );
    };

    addPath( types::PATH_USER_PLUGINS, PATHS::GetUserPluginsPath() );
    addPath( types::PATH_USER_TEMPLATES, PATHS::GetUserTemplatesPath() );
    addPath( types::PATH_USER_SETTINGS, PATHS::GetUserSettingsPath() );
    addPath( types::PATH_STOCK_SYMBOLS, PATHS::GetStockSymbolsPath() );
    addPath( types::PATH_STOCK_FOOTPRINTS, PATHS::GetStockFootprintsPath() );
    addPath( types::PATH_STOCK_DESIGN_BLOCKS, PATHS::GetStockDesignBlocksPath() );
    addPath( types::PATH_STOCK_3DMODELS, PATHS::GetStock3dmodelsPath() );
    addPath( types::PATH_STOCK_TEMPLATES, PATHS::GetStockTemplatesPath() );

    return reply;
}
