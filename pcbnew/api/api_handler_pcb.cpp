/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <magic_enum.hpp>

#include <api/api_handler_pcb.h>
#include <board_commit.h>
#include <pcb_edit_frame.h>
#include <pcb_track.h>
#include <tool/tool_manager.h>

#include <api/common/types/base_types.pb.h>

using namespace kiapi::common::commands;
using kiapi::common::types::CommandStatus;
using kiapi::common::types::DocumentType;
using kiapi::common::types::ItemRequestStatus;

static const wxString s_defaultCommitMessage = wxS( "Modification from API" );


API_HANDLER_PCB::API_HANDLER_PCB( PCB_EDIT_FRAME* aFrame ) :
        API_HANDLER(),
        m_frame( aFrame )
{
    registerHandler<RunAction, RunActionResponse>( &API_HANDLER_PCB::handleRunAction );
    registerHandler<GetOpenDocuments, GetOpenDocumentsResponse>(
            &API_HANDLER_PCB::handleGetOpenDocuments );

    registerHandler<BeginCommit, BeginCommitResponse>( &API_HANDLER_PCB::handleBeginCommit );
    registerHandler<EndCommit, EndCommitResponse>( &API_HANDLER_PCB::handleEndCommit );

    registerHandler<CreateItems, CreateItemsResponse>( &API_HANDLER_PCB::handleCreateItems );
    registerHandler<GetItems, GetItemsResponse>( &API_HANDLER_PCB::handleGetItems );
    registerHandler<UpdateItems, UpdateItemsResponse>( &API_HANDLER_PCB::handleUpdateItems );
    registerHandler<DeleteItems, DeleteItemsResponse>( &API_HANDLER_PCB::handleDeleteItems );

}


HANDLER_RESULT<RunActionResponse> API_HANDLER_PCB::handleRunAction( RunAction& aRequest )
{
    RunActionResponse response;

    if( m_frame->GetToolManager()->RunAction( aRequest.action(), true ) )
        response.set_status( RunActionStatus::RAS_OK );
    else
        response.set_status( RunActionStatus::RAS_INVALID );

    return response;
}


HANDLER_RESULT<GetOpenDocumentsResponse> API_HANDLER_PCB::handleGetOpenDocuments(
        GetOpenDocuments& aMsg )
{
    if( aMsg.type() != DocumentType::DOCTYPE_PCB )
    {
        ApiResponseStatus e;
        // No message needed for AS_UNHANDLED; this is an internal flag for the API server
        e.set_status( ApiStatusCode::AS_UNHANDLED );
        return tl::unexpected( e );
    }

    GetOpenDocumentsResponse response;
    common::types::DocumentSpecifier doc;

    wxFileName fn( m_frame->GetCurrentFileName() );

    doc.set_type( DocumentType::DOCTYPE_PCB );
    doc.set_board_filename( fn.GetFullName() );

    response.mutable_documents()->Add( std::move( doc ) );
    return response;
}


HANDLER_RESULT<BeginCommitResponse> API_HANDLER_PCB::handleBeginCommit( BeginCommit& aMsg )
{
    BeginCommitResponse response;

    if( m_commit )
    {
        // TODO: right now there is no way for m_transactionInProgress to be true here, but
        // we should still check it as a safety measure and return a specific error
        //if( !m_transactionInProgress )

        m_commit->Revert();
    }

    m_commit.reset( new BOARD_COMMIT( m_frame ) );

    // TODO: return an opaque ID for this new commit to make this more robust
    m_transactionInProgress = true;

    return response;
}


HANDLER_RESULT<EndCommitResponse> API_HANDLER_PCB::handleEndCommit( EndCommit& aMsg )
{
    EndCommitResponse response;

    // TODO: return more specific error if m_transactionInProgress is false
    if( !m_transactionInProgress )
    {
        // Make sure we don't get stuck with a commit we can never push
        m_commit.reset();
        response.set_result( CommitResult::CR_NO_COMMIT );
        return response;
    }

    if( !m_commit )
    {
        response.set_result( CommitResult::CR_NO_COMMIT );
        return response;
    }

    pushCurrentCommit( aMsg.message() );
    m_transactionInProgress = false;

    response.set_result( CommitResult::CR_OK );
    return response;
}


BOARD_COMMIT* API_HANDLER_PCB::getCurrentCommit()
{
    if( !m_commit )
        m_commit.reset( new BOARD_COMMIT( m_frame ) );

    return m_commit.get();
}


void API_HANDLER_PCB::pushCurrentCommit( const std::string& aMessage )
{
    wxCHECK( m_commit, /* void */ );

    wxString msg( aMessage.c_str(), wxConvUTF8 );

    if( msg.IsEmpty() )
        msg = s_defaultCommitMessage;

    m_commit->Push( msg );
    m_commit.reset();

    m_frame->Refresh();
}


bool API_HANDLER_PCB::validateItemHeaderDocument( const common::types::ItemHeader& aHeader )
{
    // TODO: this should return a more complex error type.
    // We should provide detailed feedback when a header fails validation, and distinguish between
    // "skip this handler" and "this is the right handler, but the request is invalid"
    if( !aHeader.has_document() || aHeader.document().type() != DocumentType::DOCTYPE_PCB )
        return false;

    wxFileName fn( m_frame->GetCurrentFileName() );

    return aHeader.document().board_filename().compare( fn.GetFullName() ) == 0;
}


std::unique_ptr<BOARD_ITEM> API_HANDLER_PCB::createItemForType( KICAD_T aType,
                                                                BOARD_ITEM_CONTAINER* aContainer )
{
    switch( aType )
    {
    case PCB_TRACE_T: return std::make_unique<PCB_TRACK>( aContainer );
    case PCB_ARC_T:   return std::make_unique<PCB_ARC>( aContainer );
    case PCB_VIA_T:   return std::make_unique<PCB_VIA>( aContainer );
    default:          return nullptr;
    }
}


HANDLER_RESULT<CreateItemsResponse> API_HANDLER_PCB::handleCreateItems( CreateItems& aMsg )
{
    ApiResponseStatus e;

    if( !validateItemHeaderDocument( aMsg.header() ) )
    {
        // No message needed for AS_UNHANDLED; this is an internal flag for the API server
        e.set_status( ApiStatusCode::AS_UNHANDLED );
        return tl::unexpected( e );
    }

    BOARD* board = m_frame->GetBoard();
    BOARD_ITEM_SET boardItems = board->GetItemSet();

    std::map<KIID, BOARD_ITEM*> itemUuidMap;

    std::for_each( boardItems.begin(), boardItems.end(),
                   [&]( BOARD_ITEM* aItem )
                   {
                       itemUuidMap[aItem->m_Uuid] = aItem;
                   } );

    BOARD_COMMIT* commit = getCurrentCommit();

    CreateItemsResponse response;

    for( const google::protobuf::Any& anyItem : aMsg.items() )
    {
        ItemCreationResult itemResult;
        std::optional<KICAD_T> type = TypeNameFromAny( anyItem );

        if( !type )
        {
            itemResult.set_status( ItemCreationStatus::ICS_INVALID_TYPE );
            response.mutable_created_items()->Add( std::move( itemResult ) );
            continue;
        }

        std::unique_ptr<BOARD_ITEM> item = createItemForType( *type, board );

        if( !item )
        {
            itemResult.set_status( ItemCreationStatus::ICS_INVALID_TYPE );
            e.set_error_message( fmt::format( "item type {} not supported for board",
                                              magic_enum::enum_name( *type ) ) );
            response.mutable_created_items()->Add( std::move( itemResult ) );
            continue;
        }

        if( !item->Deserialize( anyItem ) )
        {
            e.set_status( ApiStatusCode::AS_BAD_REQUEST );
            e.set_error_message( fmt::format( "could not unpack {} from request",
                                              item->GetClass().ToStdString() ) );
            return tl::unexpected( e );
        }

        if( itemUuidMap.count( item->m_Uuid ) )
        {
            itemResult.set_status( ItemCreationStatus::ICS_EXISTING );
            response.mutable_created_items()->Add( std::move( itemResult ) );
            continue;
        }

        itemResult.set_status( ItemCreationStatus::ICS_OK );
        item->Serialize( *itemResult.mutable_item() );
        commit->Add( item.release() );

        response.mutable_created_items()->Add( std::move( itemResult ) );
    }

    pushCurrentCommit( "Added items via API" );
    response.set_status( ItemRequestStatus::IRS_OK );
    return response;
}


HANDLER_RESULT<GetItemsResponse> API_HANDLER_PCB::handleGetItems( GetItems& aMsg )
{
    if( !validateItemHeaderDocument( aMsg.header() ) )
    {
        ApiResponseStatus e;
        // No message needed for AS_UNHANDLED; this is an internal flag for the API server
        e.set_status( ApiStatusCode::AS_UNHANDLED );
        return tl::unexpected( e );
    }

    GetItemsResponse response;

    BOARD* board = m_frame->GetBoard();
    std::vector<BOARD_ITEM*> items;
    std::set<KICAD_T> typesRequested, typesInserted;
    bool handledAnything = false;

    for( const common::types::ItemType& typeMessage : aMsg.types() )
    {
        KICAD_T type;

        if( std::optional<KICAD_T> opt_type = magic_enum::enum_cast<KICAD_T>( typeMessage.type() ) )
            type = *opt_type;
        else
            continue;

        typesRequested.emplace( type );

        if( typesInserted.count( type ) )
            continue;

        switch( type )
        {
        case PCB_TRACE_T:
        case PCB_ARC_T:
        case PCB_VIA_T:
            handledAnything = true;
            std::copy( board->Tracks().begin(), board->Tracks().end(),
                       std::back_inserter( items ) );
            typesInserted.insert( { PCB_TRACE_T, PCB_ARC_T, PCB_VIA_T } );
            break;

        default:
            break;
        }
    }

    if( !handledAnything )
    {
        ApiResponseStatus e;
        e.set_status( ApiStatusCode::AS_BAD_REQUEST );
        e.set_error_message( "none of the requested types are valid for a Board object" );
        return tl::unexpected( e );
    }

    for( const BOARD_ITEM* item : items )
    {
        if( !typesRequested.count( item->Type() ) )
            continue;

        google::protobuf::Any itemBuf;
        item->Serialize( itemBuf );
        response.mutable_items()->Add( std::move( itemBuf ) );
    }

    response.set_status( ItemRequestStatus::IRS_OK );
    return response;
}


HANDLER_RESULT<UpdateItemsResponse> API_HANDLER_PCB::handleUpdateItems( UpdateItems& aMsg )
{
    ApiResponseStatus e;

    if( !validateItemHeaderDocument( aMsg.header() ) )
    {
        // No message needed for AS_UNHANDLED; this is an internal flag for the API server
        e.set_status( ApiStatusCode::AS_UNHANDLED );
        return tl::unexpected( e );
    }

    BOARD* board = m_frame->GetBoard();
    BOARD_ITEM_SET boardItems = board->GetItemSet();

    std::map<KIID, BOARD_ITEM*> itemUuidMap;

    std::for_each( boardItems.begin(), boardItems.end(),
                   [&]( BOARD_ITEM* aItem )
                   {
                       itemUuidMap[aItem->m_Uuid] = aItem;
                   } );

    BOARD_COMMIT* commit = getCurrentCommit();

    UpdateItemsResponse response;

    for( const google::protobuf::Any& anyItem : aMsg.items() )
    {
        ItemUpdateResult itemResult;
        std::optional<KICAD_T> type = TypeNameFromAny( anyItem );

        if( !type )
        {
            itemResult.set_status( ItemUpdateStatus::IUS_INVALID_TYPE );
            response.mutable_updated_items()->Add( std::move( itemResult ) );
            continue;
        }

        std::unique_ptr<BOARD_ITEM> temporaryItem = createItemForType( *type, board );

        if( !temporaryItem )
        {
            itemResult.set_status( ItemUpdateStatus::IUS_INVALID_TYPE );
            response.mutable_updated_items()->Add( std::move( itemResult ) );
            continue;
        }

        if( !temporaryItem->Deserialize( anyItem ) )
        {
            e.set_status( ApiStatusCode::AS_BAD_REQUEST );
            e.set_error_message( fmt::format( "could not unpack {} from request",
                                              magic_enum::enum_name( *type ) ) );
            return tl::unexpected( e );
        }

        if( !itemUuidMap.count( temporaryItem->m_Uuid ) )
        {
            itemResult.set_status( ItemUpdateStatus::IUS_NONEXISTENT );
            response.mutable_updated_items()->Add( std::move( itemResult ) );
            continue;
        }

        BOARD_ITEM* boardItem = itemUuidMap[temporaryItem->m_Uuid];

        boardItem->SwapItemData( temporaryItem.get() );

        itemResult.set_status( ItemUpdateStatus::IUS_OK );
        boardItem->Serialize( *itemResult.mutable_item() );
        commit->Modify( boardItem );

        itemResult.set_status( ItemUpdateStatus::IUS_OK );
        response.mutable_updated_items()->Add( std::move( itemResult ) );
    }

    response.set_status( ItemRequestStatus::IRS_OK );
    pushCurrentCommit( "Updated items via API" );
    return response;
}


HANDLER_RESULT<DeleteItemsResponse> API_HANDLER_PCB::handleDeleteItems( DeleteItems& aMsg )
{
    if( !validateItemHeaderDocument( aMsg.header() ) )
    {
        ApiResponseStatus e;
        // No message needed for AS_UNHANDLED; this is an internal flag for the API server
        e.set_status( ApiStatusCode::AS_UNHANDLED );
        return tl::unexpected( e );
    }

    std::map<KIID, ItemDeletionStatus> itemsToDelete;

    for( const common::types::KIID& kiidBuf : aMsg.item_ids() )
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

    BOARD* board = m_frame->GetBoard();

    // This is somewhat inefficient on paper, but the total number of items on a board is
    // not computationally-speaking very high even on what we'd consider a large design.
    // If this ends up not being the case, we should consider doing something like refactoring
    // BOARD to contain all items in a contiguous memory arena and constructing views over it
    // when we want to filter to just tracks, etc.
    BOARD_ITEM_SET items = board->GetItemSet();
    std::vector<BOARD_ITEM*> validatedItems;

    for( BOARD_ITEM* item : items )
    {
        if( itemsToDelete.count( item->m_Uuid ) )
        {
            validatedItems.push_back( item );
            itemsToDelete[item->m_Uuid] = ItemDeletionStatus::IDS_OK;
        }

        // Note: we don't currently support locking items from API modification, but here is where
        // to add it in the future (and return IDS_IMMUTABLE)
    }

    BOARD_COMMIT* commit = getCurrentCommit();

    for( BOARD_ITEM* item : validatedItems )
        commit->Remove( item );

    if( !m_transactionInProgress )
        pushCurrentCommit( "Deleted items via API" );

    DeleteItemsResponse response;

    for( const auto& [id, status] : itemsToDelete )
    {
        ItemDeletionResult result;
        result.mutable_id()->set_value( id.AsStdString() );
        result.set_status( status );
    }

    response.set_status( ItemRequestStatus::IRS_OK );
    return response;
}
