/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 Jon Evans <jon@craftyjon.com>
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <api/api_handler_editor.h>
#include <api/api_utils.h>
#include <eda_base_frame.h>
#include <eda_item.h>
#include <wx/wx.h>

using namespace kiapi::common::commands;


API_HANDLER_EDITOR::API_HANDLER_EDITOR( EDA_BASE_FRAME* aFrame ) :
        API_HANDLER(),
        m_frame( aFrame )
{
    registerHandler<BeginCommit, BeginCommitResponse>( &API_HANDLER_EDITOR::handleBeginCommit );
    registerHandler<EndCommit, EndCommitResponse>( &API_HANDLER_EDITOR::handleEndCommit );
    registerHandler<CreateItems, CreateItemsResponse>( &API_HANDLER_EDITOR::handleCreateItems );
    registerHandler<UpdateItems, UpdateItemsResponse>( &API_HANDLER_EDITOR::handleUpdateItems );
    registerHandler<DeleteItems, DeleteItemsResponse>( &API_HANDLER_EDITOR::handleDeleteItems );
    registerHandler<HitTest, HitTestResponse>( &API_HANDLER_EDITOR::handleHitTest );
}


HANDLER_RESULT<BeginCommitResponse> API_HANDLER_EDITOR::handleBeginCommit(
        const HANDLER_CONTEXT<BeginCommit>& aCtx )
{
    if( std::optional<ApiResponseStatus> busy = checkForBusy() )
        return tl::unexpected( *busy );

    if( m_commits.count( aCtx.ClientName ) )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( fmt::format( "the client {} already has a commit in progress",
                                          aCtx.ClientName ) );
        return tl::unexpected( e );
    }

    wxASSERT( !m_activeClients.count( aCtx.ClientName ) );

    BeginCommitResponse response;

    KIID id;
    m_commits[aCtx.ClientName] = std::make_pair( id, createCommit() );
    response.mutable_id()->set_value( id.AsStdString() );

    m_activeClients.insert( aCtx.ClientName );

    return response;
}


HANDLER_RESULT<EndCommitResponse> API_HANDLER_EDITOR::handleEndCommit(
        const HANDLER_CONTEXT<EndCommit>& aCtx )
{
    if( std::optional<ApiResponseStatus> busy = checkForBusy() )
        return tl::unexpected( *busy );

    if( !m_commits.count( aCtx.ClientName ) )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( fmt::format( "the client {} does not has a commit in progress",
                                          aCtx.ClientName ) );
        return tl::unexpected( e );
    }

    wxASSERT( m_activeClients.count( aCtx.ClientName ) );

    const std::pair<KIID, std::unique_ptr<COMMIT>>& pair = m_commits.at( aCtx.ClientName );
    const KIID& id = pair.first;
    const std::unique_ptr<COMMIT>& commit = pair.second;

    EndCommitResponse response;

    // Do not check IDs with drop; it is a safety net in case the id was lost on the client side
    switch( aCtx.Request.action() )
    {
    case kiapi::common::commands::CMA_DROP:
    {
        commit->Revert();
        m_commits.erase( aCtx.ClientName );
        m_activeClients.erase( aCtx.ClientName );
        break;
    }

    case kiapi::common::commands::CMA_COMMIT:
    {
        if( aCtx.Request.id().value().compare( id.AsStdString() ) != 0 )
        {
            ApiResponseStatus e;
            e.set_status( ApiStatusCode::AS_BAD_REQUEST );
            e.set_error_message( fmt::format( "the id {} does not match the commit in progress",
                                              aCtx.Request.id().value() ) );
            return tl::unexpected( e );
        }

        pushCurrentCommit( aCtx.ClientName, wxString( aCtx.Request.message().c_str(), wxConvUTF8 ) );
        break;
    }

    default:
        break;
    }

    return response;
}


COMMIT* API_HANDLER_EDITOR::getCurrentCommit( const std::string& aClientName )
{
    if( !m_commits.count( aClientName ) )
    {
        KIID id;
        m_commits[aClientName] = std::make_pair( id, createCommit() );
    }

    return m_commits.at( aClientName ).second.get();
}


void API_HANDLER_EDITOR::pushCurrentCommit( const std::string& aClientName,
                                            const wxString& aMessage )
{
    auto it = m_commits.find( aClientName );

    if( it == m_commits.end() )
        return;

    it->second.second->Push( aMessage.IsEmpty() ? m_defaultCommitMessage : aMessage );
    m_commits.erase( it );
    m_activeClients.erase( aClientName );
}


HANDLER_RESULT<bool> API_HANDLER_EDITOR::validateDocument( const DocumentSpecifier& aDocument )
{
    if( !validateDocumentInternal( aDocument ) )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( fmt::format( "the requested document {} is not open",
                                          aDocument.board_filename() ) );
        return tl::unexpected( e );
    }

    return true;
}


HANDLER_RESULT<std::optional<KIID>> API_HANDLER_EDITOR::validateItemHeaderDocument(
        const types::ItemHeader& aHeader )
{
    if( !aHeader.has_document() || aHeader.document().type() != thisDocumentType() )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_UNHANDLED );
        // No error message, this is a flag that the server should try a different handler
        return tl::unexpected( e );
    }

    HANDLER_RESULT<bool> documentValidation = validateDocument( aHeader.document() );

    if( !documentValidation )
        return tl::unexpected( documentValidation.error() );

    if( !validateDocumentInternal( aHeader.document() ) )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( fmt::format( "the requested document {} is not open",
                                          aHeader.document().board_filename() ) );
        return tl::unexpected( e );
    }

    if( aHeader.has_container() )
    {
        return KIID( aHeader.container().value() );
    }

    // Valid header, but no container provided
    return std::nullopt;
}


std::optional<ApiResponseStatus> API_HANDLER_EDITOR::checkForBusy()
{
    if( !m_frame->CanAcceptApiCommands() )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BUSY );
        e.set_error_message( "KiCad is busy and cannot respond to API requests right now" );
        return e;
    }

    return std::nullopt;
}


HANDLER_RESULT<CreateItemsResponse> API_HANDLER_EDITOR::handleCreateItems(
        const HANDLER_CONTEXT<CreateItems>& aCtx )
{
    if( std::optional<ApiResponseStatus> busy = checkForBusy() )
        return tl::unexpected( *busy );

    CreateItemsResponse response;

    HANDLER_RESULT<ItemRequestStatus> result = handleCreateUpdateItemsInternal( true,
            aCtx.ClientName,
            aCtx.Request.header(), aCtx.Request.items(),
            [&]( const ItemStatus& aStatus, const google::protobuf::Any& aItem )
            {
                ItemCreationResult itemResult;
                itemResult.mutable_status()->CopyFrom( aStatus );
                itemResult.mutable_item()->CopyFrom( aItem );
                response.mutable_created_items()->Add( std::move( itemResult ) );
            } );

    if( !result.has_value() )
        return tl::unexpected( result.error() );

    response.set_status( *result );
    return response;
}


HANDLER_RESULT<UpdateItemsResponse> API_HANDLER_EDITOR::handleUpdateItems(
        const HANDLER_CONTEXT<UpdateItems>& aCtx )
{
    if( std::optional<ApiResponseStatus> busy = checkForBusy() )
        return tl::unexpected( *busy );

    UpdateItemsResponse response;

    HANDLER_RESULT<ItemRequestStatus> result = handleCreateUpdateItemsInternal( false,
            aCtx.ClientName,
            aCtx.Request.header(), aCtx.Request.items(),
            [&]( const ItemStatus& aStatus, const google::protobuf::Any& aItem )
            {
                ItemUpdateResult itemResult;
                itemResult.mutable_status()->CopyFrom( aStatus );
                itemResult.mutable_item()->CopyFrom( aItem );
                response.mutable_updated_items()->Add( std::move( itemResult ) );
            } );

    if( !result.has_value() )
        return tl::unexpected( result.error() );

    response.set_status( *result );
    return response;
}


HANDLER_RESULT<DeleteItemsResponse> API_HANDLER_EDITOR::handleDeleteItems(
        const HANDLER_CONTEXT<DeleteItems>& aCtx )
{
    if( std::optional<ApiResponseStatus> busy = checkForBusy() )
        return tl::unexpected( *busy );

    if( !validateItemHeaderDocument( aCtx.Request.header() ) )
    {
        ApiResponseStatus e;
        // No message needed for AS_UNHANDLED; this is an internal flag for the API server
        e.set_status( ApiStatusCode::AS_UNHANDLED );
        return tl::unexpected( e );
    }

    std::map<KIID, ItemDeletionStatus> itemsToDelete;

    for( const kiapi::common::types::KIID& kiidBuf : aCtx.Request.item_ids() )
    {
        if( !kiidBuf.value().empty() )
        {
            KIID kiid( kiidBuf.value() );
            itemsToDelete[kiid] = ItemDeletionStatus::IDS_NONEXISTENT;
        }
    }

    if( itemsToDelete.empty() )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( "no valid items to delete were given" );
        return tl::unexpected( e );
    }

    deleteItemsInternal( itemsToDelete, aCtx.ClientName );

    DeleteItemsResponse response;

    for( const auto& [id, status] : itemsToDelete )
    {
        ItemDeletionResult result;
        result.mutable_id()->set_value( id.AsStdString() );
        result.set_status( status );
    }

    response.set_status( kiapi::common::types::ItemRequestStatus::IRS_OK );
    return response;
}


HANDLER_RESULT<HitTestResponse> API_HANDLER_EDITOR::handleHitTest(
        const HANDLER_CONTEXT<HitTest>& aCtx )
{
    if( std::optional<ApiResponseStatus> busy = checkForBusy() )
        return tl::unexpected( *busy );

    if( !validateItemHeaderDocument( aCtx.Request.header() ) )
    {
        ApiResponseStatus e;
        // No message needed for AS_UNHANDLED; this is an internal flag for the API server
        e.set_status( ApiStatusCode::AS_UNHANDLED );
        return tl::unexpected( e );
    }

    HitTestResponse response;

    std::optional<EDA_ITEM*> item = getItemFromDocument( aCtx.Request.header().document(),
                                                         KIID( aCtx.Request.id().value() ) );

    if( !item )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( "the requested item ID is not present in the given document" );
        return tl::unexpected( e );
    }

    if( ( *item )->HitTest( UnpackVector2( aCtx.Request.position() ), aCtx.Request.tolerance() ) )
        response.set_result( HitTestResult::HTR_HIT );
    else
        response.set_result( HitTestResult::HTR_NO_HIT );

    return response;
}
