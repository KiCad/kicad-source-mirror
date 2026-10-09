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

#include <mdw/client/mdw_source.h>
#include <mdw/client/mdw_baseline.h>
#include <mdw/client/mdw_bom.h>
#include <mdw/client/mdw_eco.h>
#include <mdw/client/mdw_library.h>
#include <mdw/client/mdw_part.h>
#include <mdw/client/mdw_part_catalog.h>
#include <mdw/client/mdw_review.h>
#include <mdw/client/mdw_search.h>
#include "mdw_call.h"

#include <api/mdw/backend_locking.pb.h>
#include <api/mdw/backend_permissions.pb.h>
#include <api/mdw/backend_revisions.pb.h>

#include <utility>

namespace mdw = kiapi::mdw;


MDW_SOURCE::MDW_SOURCE( std::weak_ptr<MDW_SESSION> aSession ) :
        m_resolve(
                [session = std::move( aSession )]()
                {
                    return session.lock();
                } )
{
    if( std::shared_ptr<MDW_SESSION> session = Session() )
        m_locator = session->SourceLocator();
}


MDW_SOURCE::MDW_SOURCE( RESOLVER aResolve, const wxString& aLocator ) :
        m_resolve( std::move( aResolve ) ),
        m_locator( aLocator )
{
}


MDW_SOURCE::~MDW_SOURCE() = default;


MDW_SOURCE_INFO MDW_SOURCE::Info() const
{
    MDW_SOURCE_INFO              info;
    std::shared_ptr<MDW_SESSION> session = Session();

    info.locator = m_locator;

    if( !session )
        return info;

    info.locator = session->SourceLocator();
    info.workspace_id = session->Binding().workspace_id;
    info.connection = session->Status();

    if( std::shared_ptr<MDW_BACKEND> backend = session->Backend() )
    {
        info.backend = backend->Info();
        info.display_name = info.backend.display_name;
    }

    return info;
}


std::shared_ptr<MDW_BACKEND> MDW_SOURCE::Backend() const
{
    std::shared_ptr<MDW_SESSION> session = Session();

    return session ? session->Backend() : nullptr;
}


MDW_CAPABILITIES MDW_SOURCE::Capabilities() const
{
    std::shared_ptr<MDW_BACKEND> backend = Backend();

    return backend ? backend->Capabilities() : MDW_CAPABILITIES();
}


bool MDW_SOURCE::Has( MDW_CAPABILITY_GROUP aGroup ) const
{
    return Capabilities().Has( aGroup );
}


MDW_PART MDW_SOURCE::Part( const MDW_ITEM_REF& aItem )
{
    return MDW_PART( shared_from_this(), aItem );
}


MDW_PART_CATALOG MDW_SOURCE::Parts()
{
    return MDW_PART_CATALOG( shared_from_this() );
}


MDW_LIBRARY MDW_SOURCE::Libraries()
{
    return MDW_LIBRARY( shared_from_this() );
}


MDW_BASELINE MDW_SOURCE::Baselines()
{
    return MDW_BASELINE( shared_from_this() );
}


MDW_SEARCH MDW_SOURCE::Search()
{
    return MDW_SEARCH( shared_from_this() );
}


MDW_BOM MDW_SOURCE::Bom()
{
    return MDW_BOM( shared_from_this() );
}


MDW_ECO MDW_SOURCE::Ecos()
{
    return MDW_ECO( shared_from_this() );
}


MDW_REVIEW MDW_SOURCE::Reviews()
{
    return MDW_REVIEW( shared_from_this() );
}


namespace
{

/// The tracker update for a lock reply, as the workspace service's own lock operations make it
MDW_SETTLE<MDW_LOCK_INFO> settleLock( const MDW_TRACKED_OP& aOp, const wxString& aItemId, MDW_EVENT_TYPE aType )
{
    return [aOp, aItemId, aType]( const MDW_RESULT& aResult, const MDW_LOCK_INFO& aLock )
    {
        if( aResult.IsOk() )
            aOp.ApplyLock( aItemId, aLock, aType );
        else
            aOp.Abandon( aResult );
    };
}


template <typename ROWS, typename T>
MDW_PAGED<T> paged( const ROWS& aRows, const std::string& aNextPageToken )
{
    MDW_PAGED<T> page;

    for( const auto& row : aRows )
        page.rows.push_back( MdwToStruct( row ) );

    page.next_page_token = MdwFromUtf8( aNextPageToken );
    return page;
}

} // namespace


MDW_OP MDW_SOURCE::Item( const MDW_ITEM_REF& aItem, const wxString& aRevisionId, MDW_DONE<MDW_ITEM_INFO> aDone )
{
    std::optional<std::string> id = MdwWireId( *this, aItem );

    if( !id )
        return MdwForeignItem( shared_from_this(), std::move( aDone ) );

    mdw::GetItemInfoRequest request;
    request.set_item_id( *id );
    request.set_revision_id( MdwToUtf8( aRevisionId ) );

    return MdwCall<mdw::GetItemInfoRequest, mdw::GetItemInfoReply, MDW_ITEM_INFO>(
            shared_from_this(), std::nullopt, request,
            []( const mdw::GetItemInfoReply& aReply )
            {
                return MdwToStruct( aReply.item() );
            },
            std::move( aDone ) );
}


MDW_OP MDW_SOURCE::Items( const wxString& aFolderId, const wxString& aFilter, const MDW_PAGE& aPage,
                          MDW_DONE<MDW_PAGED<MDW_ITEM_INFO>> aDone )
{
    mdw::ListItemsRequest request;
    request.set_folder_id( MdwToUtf8( aFolderId ) );
    request.set_filter( MdwToUtf8( aFilter ) );
    MdwToProto( request.mutable_page(), aPage );

    return MdwCall<mdw::ListItemsRequest, mdw::ListItemsReply, MDW_PAGED<MDW_ITEM_INFO>>(
            shared_from_this(), std::nullopt, request,
            []( const mdw::ListItemsReply& aReply )
            {
                return paged<decltype( aReply.items() ), MDW_ITEM_INFO>( aReply.items(),
                                                                          aReply.next_page_token() );
            },
            std::move( aDone ) );
}


MDW_OP MDW_SOURCE::RootFolders( const MDW_PAGE& aPage, MDW_DONE<MDW_PAGED<MDW_FOLDER>> aDone )
{
    mdw::GetRootFoldersRequest request;
    MdwToProto( request.mutable_page(), aPage );

    return MdwCall<mdw::GetRootFoldersRequest, mdw::GetRootFoldersReply, MDW_PAGED<MDW_FOLDER>>(
            shared_from_this(), std::nullopt, request,
            []( const mdw::GetRootFoldersReply& aReply )
            {
                return paged<decltype( aReply.folders() ), MDW_FOLDER>( aReply.folders(), aReply.next_page_token() );
            },
            std::move( aDone ) );
}


MDW_OP MDW_SOURCE::ChildFolders( const wxString& aParentFolderId, const MDW_PAGE& aPage,
                                 MDW_DONE<MDW_PAGED<MDW_FOLDER>> aDone )
{
    mdw::GetChildFoldersRequest request;
    request.set_folder_id( MdwToUtf8( aParentFolderId ) );
    MdwToProto( request.mutable_page(), aPage );

    return MdwCall<mdw::GetChildFoldersRequest, mdw::GetChildFoldersReply, MDW_PAGED<MDW_FOLDER>>(
            shared_from_this(), std::nullopt, request,
            []( const mdw::GetChildFoldersReply& aReply )
            {
                return paged<decltype( aReply.folders() ), MDW_FOLDER>( aReply.folders(), aReply.next_page_token() );
            },
            std::move( aDone ) );
}


MDW_OP MDW_SOURCE::Checkout( const MDW_CHECKOUT_REQUEST& aRequest, MDW_DONE<MDW_CHECKOUT_RESULT> aDone )
{
    mdw::CheckoutRequest request;
    request.set_item_id( MdwToUtf8( aRequest.item_id ) );
    request.set_local_path( MdwToUtf8( aRequest.local_path ) );
    request.set_lock( aRequest.lock );

    MDW_TRACKED_OP op = MdwBeginOperation( *this, aRequest.idempotency_id );
    request.set_idempotency_id( MdwToUtf8( op.id ) );

    return MdwCall<mdw::CheckoutRequest, mdw::CheckoutReply, MDW_CHECKOUT_RESULT>(
            op, shared_from_this(), std::nullopt, request,
            [asked = aRequest.local_path]( const mdw::CheckoutReply& aReply )
            {
                // The backend may write elsewhere; a reply without a path wrote where it was asked
                wxString written = MdwFromUtf8( aReply.local_path() );
                return MDW_CHECKOUT_RESULT{ MdwToStruct( aReply.item() ), written.IsEmpty() ? asked : written };
            },
            std::move( aDone ),
            [op, lockRequested = aRequest.lock]( const MDW_RESULT& aResult, const MDW_CHECKOUT_RESULT& aOut )
            {
                if( !aResult.IsOk() )
                {
                    op.Abandon( aResult );
                }
                else if( aOut.item.lock.is_locked )
                {
                    // Ours when the reply says so or the checkout asked for it
                    MDW_LOCK_INFO lock = aOut.item.lock;
                    lock.locked_by_me = lock.locked_by_me || lockRequested;
                    op.ApplyLock( aOut.item.item_id, lock, MDW_EVENT_TYPE::ITEM_LOCKED );
                }
                else
                {
                    op.Emit( MDW_EVENT_TYPE::REVISION_CHECKED_OUT, aOut.item.item_id );
                }
            } );
}


MDW_OP MDW_SOURCE::AddItem( const MDW_ADD_ITEM_REQUEST& aRequest, MDW_DONE<MDW_CHECKOUT_RESULT> aDone )
{
    mdw::AddItemRequest request;
    request.set_relative_path( MdwToUtf8( aRequest.relative_path ) );
    request.set_local_path( MdwToUtf8( aRequest.local_path ) );
    request.set_comment( MdwToUtf8( aRequest.comment ) );

    MDW_TRACKED_OP op = MdwBeginOperation( *this, aRequest.idempotency_id );
    request.set_idempotency_id( MdwToUtf8( op.id ) );

    return MdwCall<mdw::AddItemRequest, mdw::AddItemReply, MDW_CHECKOUT_RESULT>(
            op, shared_from_this(), std::nullopt, request,
            [asked = aRequest.local_path]( const mdw::AddItemReply& aReply, MDW_CHECKOUT_RESULT& aOut )
            {
                // Without the item it made, the file cannot be bound to it
                if( aReply.item().item_id().empty() )
                    return false;

                aOut = MDW_CHECKOUT_RESULT{ MdwToStruct( aReply.item() ), asked };
                return true;
            },
            std::move( aDone ),
            [op]( const MDW_RESULT& aResult, const MDW_CHECKOUT_RESULT& aOut )
            {
                if( aResult.IsOk() )
                    op.Emit( MDW_EVENT_TYPE::REVISION_CHECKED_IN, aOut.item.item_id );
                else
                    op.Abandon( aResult );
            } );
}


MDW_OP MDW_SOURCE::Checkin( const MDW_CHECKIN_REQUEST& aRequest, MDW_DONE<MDW_REVISION> aDone )
{
    mdw::CheckinRequest request;
    request.set_item_id( MdwToUtf8( aRequest.item_id ) );
    request.set_local_path( MdwToUtf8( aRequest.local_path ) );
    request.set_comment( MdwToUtf8( aRequest.comment ) );
    request.set_base_revision_id( MdwToUtf8( aRequest.base_revision_id ) );

    MDW_TRACKED_OP op = MdwBeginOperation( *this, aRequest.idempotency_id );
    std::uint64_t  saved = op.DirtyGeneration( aRequest.item_id );
    request.set_idempotency_id( MdwToUtf8( op.id ) );

    return MdwCall<mdw::CheckinRequest, mdw::CheckinReply, MDW_REVISION>(
            op, shared_from_this(), std::nullopt, request,
            []( const mdw::CheckinReply& aReply, MDW_REVISION& aRevision )
            {
                // Without the revision it made, a check-in cannot count as done
                if( aReply.revision().revision_id().empty() )
                    return false;

                aRevision = MdwToStruct( aReply.revision() );
                return true;
            },
            std::move( aDone ),
            [op, itemId = aRequest.item_id, saved]( const MDW_RESULT& aResult, const MDW_REVISION& )
            {
                if( !aResult.IsOk() )
                {
                    op.Abandon( aResult );
                    return;
                }

                op.ClearDirty( itemId, saved );
                op.Emit( MDW_EVENT_TYPE::REVISION_CHECKED_IN, itemId );
            } );
}


MDW_OP MDW_SOURCE::CancelCheckout( const MDW_ITEM_REF& aItem, const wxString& aLocalPath, MDW_DONE<MDW_NONE> aDone )
{
    std::optional<std::string> id = MdwWireId( *this, aItem );

    if( !id )
        return MdwForeignItem( shared_from_this(), std::move( aDone ) );

    mdw::CancelCheckoutRequest request;
    request.set_item_id( *id );
    request.set_local_path( MdwToUtf8( aLocalPath ) );

    MDW_TRACKED_OP op = MdwBeginOperation( *this );
    std::uint64_t  saved = op.DirtyGeneration( wxString::FromUTF8( *id ) );
    request.set_idempotency_id( MdwToUtf8( op.id ) );

    return MdwCall<mdw::CancelCheckoutRequest, mdw::CancelCheckoutReply, MDW_NONE>(
            op, shared_from_this(), std::nullopt, request,
            []( const mdw::CancelCheckoutReply& )
            {
                return MDW_NONE();
            },
            std::move( aDone ),
            [op, itemId = wxString::FromUTF8( *id ), saved,
             restored = !aLocalPath.IsEmpty()]( const MDW_RESULT& aResult, const MDW_NONE& )
            {
                if( !aResult.IsOk() )
                {
                    op.Abandon( aResult );
                    return;
                }

                // Without a path only the lock goes, and the saved edits it leaves still need checking in
                if( restored )
                    op.ClearDirty( itemId, saved );

                op.ApplyLock( itemId, MDW_LOCK_INFO{}, MDW_EVENT_TYPE::ITEM_UNLOCKED );
            } );
}


MDW_OP MDW_SOURCE::Locate( const std::vector<wxString>& aRelativePaths,
                           MDW_DONE<std::vector<MDW_LOCATED_ITEM>> aDone )
{
    mdw::LocateItemsRequest request;

    for( const wxString& path : aRelativePaths )
        request.add_relative_paths( MdwToUtf8( path ) );

    return MdwCall<mdw::LocateItemsRequest, mdw::LocateItemsReply, std::vector<MDW_LOCATED_ITEM>>(
            shared_from_this(), std::nullopt, request,
            [locator = Info().locator, count = aRelativePaths.size()]( const mdw::LocateItemsReply& aReply,
                                                                      std::vector<MDW_LOCATED_ITEM>& aOut )
            {
                // Callers pair rows with their paths by position
                if( static_cast<std::size_t>( aReply.items_size() ) != count )
                    return false;

                for( const mdw::LocatedItem& row : aReply.items() )
                {
                    MDW_LOCATED_ITEM& located = aOut.emplace_back();

                    if( !row.item().item_id().empty() )
                        located.item = MdwToStruct( row.item(), locator );
                    else
                        located.item = MdwToStruct( row.item() );

                    located.base_revision_id = MdwFromUtf8( row.base_revision_id() );
                    located.base_revision_label = MdwFromUtf8( row.base_revision_label() );
                }

                return true;
            },
            std::move( aDone ) );
}


MDW_OP MDW_SOURCE::LocalChanges( const wxString& aLocalRoot, const MDW_PAGE& aPage,
                                 MDW_DONE<MDW_PAGED<MDW_LOCAL_CHANGE>> aDone )
{
    mdw::ListLocalChangesRequest request;
    request.set_local_root( MdwToUtf8( aLocalRoot ) );
    MdwToProto( request.mutable_page(), aPage );

    return MdwCall<mdw::ListLocalChangesRequest, mdw::ListLocalChangesReply, MDW_PAGED<MDW_LOCAL_CHANGE>>(
            shared_from_this(), std::nullopt, request,
            [locator = Info().locator]( const mdw::ListLocalChangesReply& aReply )
            {
                MDW_PAGED<MDW_LOCAL_CHANGE> page;

                for( const mdw::LocalChange& row : aReply.changes() )
                {
                    MDW_LOCAL_CHANGE& change = page.rows.emplace_back();
                    change.member = MdwToStruct( row.member(), locator );
                    change.library_id = MdwFromUtf8( row.library_id() );
                    change.name = MdwFromUtf8( row.name() );
                    change.change = MdwFromProto( row.change() );
                    change.kind = MdwFromProto( row.kind() );
                    change.relative_path = MdwFromUtf8( row.relative_path() );
                    change.provisional = row.provisional();
                    change.base_revision_id = MdwFromUtf8( row.base_revision_id() );
                    change.base_relative_path = MdwFromUtf8( row.base_relative_path() );
                    change.base_format = MdwFromUtf8( row.base_format() );
                    change.checkin = row.checkin();
                    MdwSanitize( change.checkin );
                }

                page.next_page_token = MdwFromUtf8( aReply.next_page_token() );
                return page;
            },
            std::move( aDone ) );
}


MDW_OP MDW_SOURCE::PendingChange( const wxString& aChangeId, MDW_DONE<MDW_PENDING_INFO> aDone )
{
    mdw::GetPendingChangeRequest request;
    request.set_change_id( MdwToUtf8( aChangeId ) );

    return MdwCall<mdw::GetPendingChangeRequest, mdw::GetPendingChangeReply, MDW_PENDING_INFO>(
            shared_from_this(), std::nullopt, request,
            [locator = Info().locator]( const mdw::GetPendingChangeReply& aReply )
            {
                MDW_PENDING_INFO info;
                info.status = MdwFromProto( aReply.status() );
                info.change = MdwToStruct( aReply.change() );
                info.item = MdwStamped( MdwToStruct( aReply.item() ), locator );
                info.revision = MdwToStruct( aReply.revision() );
                info.reason = MdwFromUtf8( aReply.reason() );
                return info;
            },
            std::move( aDone ) );
}


MDW_OP MDW_SOURCE::Lock( const MDW_ITEM_REF& aItem, MDW_DONE<MDW_LOCK_INFO> aDone )
{
    std::optional<std::string> id = MdwWireId( *this, aItem );

    if( !id )
        return MdwForeignItem( shared_from_this(), std::move( aDone ) );

    mdw::LockRequest request;
    request.set_item_id( *id );

    MDW_TRACKED_OP op = MdwBeginOperation( *this );
    request.set_idempotency_id( MdwToUtf8( op.id ) );

    return MdwCall<mdw::LockRequest, mdw::LockReply, MDW_LOCK_INFO>(
            op, shared_from_this(), MDW_CAPABILITY_GROUP::LOCKING, request,
            []( const mdw::LockReply& aReply )
            {
                return MdwToStruct( aReply.lock() );
            },
            std::move( aDone ), settleLock( op, wxString::FromUTF8( *id ), MDW_EVENT_TYPE::ITEM_LOCKED ) );
}


MDW_OP MDW_SOURCE::Unlock( const MDW_ITEM_REF& aItem, bool aForce, MDW_DONE<MDW_LOCK_INFO> aDone,
                           const wxString& aReason, const std::optional<MDW_LOCK_INFO>& aExpected )
{
    std::optional<std::string> id = MdwWireId( *this, aItem );

    if( !id )
        return MdwForeignItem( shared_from_this(), std::move( aDone ) );

    mdw::UnlockRequest request;
    request.set_item_id( *id );
    request.set_force( aForce );
    request.set_reason( MdwToUtf8( aReason ) );

    if( aForce && aExpected )
        MdwToProto( request.mutable_expected(), *aExpected );

    MDW_TRACKED_OP op = MdwBeginOperation( *this );
    request.set_idempotency_id( MdwToUtf8( op.id ) );

    return MdwCall<mdw::UnlockRequest, mdw::UnlockReply, MDW_LOCK_INFO>(
            op, shared_from_this(), MDW_CAPABILITY_GROUP::LOCKING, request,
            []( const mdw::UnlockReply& aReply )
            {
                return MdwToStruct( aReply.lock() );
            },
            std::move( aDone ), settleLock( op, wxString::FromUTF8( *id ), MDW_EVENT_TYPE::ITEM_UNLOCKED ) );
}


MDW_OP MDW_SOURCE::LockInfo( const MDW_ITEM_REF& aItem, MDW_DONE<MDW_LOCK_INFO> aDone )
{
    std::optional<std::string> id = MdwWireId( *this, aItem );

    if( !id )
        return MdwForeignItem( shared_from_this(), std::move( aDone ) );

    mdw::GetLockInfoRequest request;
    request.set_item_id( *id );

    std::shared_ptr<MDW_SESSION>           session = Session();
    std::shared_ptr<MDW_WORKSPACE_SERVICE> service = session ? session->Service() : nullptr;
    wxString                               itemId = wxString::FromUTF8( *id );
    std::uint64_t                          generation = service ? service->LockTracker().Generation( itemId ) : 0;

    // Asks the session whose tracker it refreshes, even if a restart replaces it meanwhile
    return MdwCallOn<mdw::GetLockInfoRequest, mdw::GetLockInfoReply, MDW_LOCK_INFO>(
            shared_from_this(), service ? service->Backend() : nullptr, MDW_CAPABILITY_GROUP::LOCKING, request,
            MdwAlwaysValid<mdw::GetLockInfoReply, MDW_LOCK_INFO>(
                    []( const mdw::GetLockInfoReply& aReply )
                    {
                        return MdwToStruct( aReply.lock() );
                    } ),
            std::move( aDone ),
            [service, itemId, generation]( const MDW_RESULT& aResult, const MDW_LOCK_INFO& aLock )
            {
                // An event that landed during the query is newer than its answer
                if( service && aResult.IsOk() )
                    service->LockTracker().SetIfUnchanged( itemId, aLock, generation );
            },
            MDW_CALL_TIMEOUT_MS );
}


MDW_LOCK_INFO MDW_SOURCE::Claim( const MDW_ITEM_REF& aItem ) const
{
    std::shared_ptr<MDW_SESSION> session = Session();
    std::optional<std::string>   wireId = MdwWireId( *this, aItem );

    if( !session || !wireId )
        return MDW_LOCK_INFO();

    return session->LockTracker().Get( wxString::FromUTF8( *wireId ) );
}


MDW_OP MDW_SOURCE::Locks( const wxString& aFolderId, const MDW_PAGE& aPage,
                          MDW_DONE<MDW_PAGED<MDW_ITEM_LOCK>> aDone )
{
    mdw::ListLocksRequest request;
    request.set_folder_id( MdwToUtf8( aFolderId ) );
    MdwToProto( request.mutable_page(), aPage );

    return MdwCall<mdw::ListLocksRequest, mdw::ListLocksReply, MDW_PAGED<MDW_ITEM_LOCK>>(
            shared_from_this(), MDW_CAPABILITY_GROUP::LOCKING, request,
            []( const mdw::ListLocksReply& aReply )
            {
                MDW_PAGED<MDW_ITEM_LOCK> page;

                for( const mdw::ItemLock& lock : aReply.locks() )
                    page.rows.push_back( { MdwFromUtf8( lock.item_id() ), MdwToStruct( lock.lock() ) } );

                page.next_page_token = MdwFromUtf8( aReply.next_page_token() );
                return page;
            },
            std::move( aDone ) );
}


MDW_OP MDW_SOURCE::History( const MDW_ITEM_REF& aItem, const MDW_PAGE& aPage,
                            MDW_DONE<MDW_PAGED<MDW_REVISION>> aDone )
{
    std::optional<std::string> id = MdwWireId( *this, aItem );

    if( !id )
        return MdwForeignItem( shared_from_this(), std::move( aDone ) );

    mdw::GetRevisionHistoryRequest request;
    request.set_item_id( *id );
    MdwToProto( request.mutable_page(), aPage );

    return MdwCall<mdw::GetRevisionHistoryRequest, mdw::GetRevisionHistoryReply, MDW_PAGED<MDW_REVISION>>(
            shared_from_this(), MDW_CAPABILITY_GROUP::REVISIONS, request,
            []( const mdw::GetRevisionHistoryReply& aReply )
            {
                return paged<decltype( aReply.revisions() ), MDW_REVISION>( aReply.revisions(),
                                                                             aReply.next_page_token() );
            },
            std::move( aDone ) );
}


MDW_OP MDW_SOURCE::Download( const MDW_ITEM_REF& aItem, const wxString& aRevisionId, const wxString& aLocalPath,
                             MDW_DONE<MDW_REVISION> aDone )
{
    std::optional<std::string> id = MdwWireId( *this, aItem );

    if( !id )
        return MdwForeignItem( shared_from_this(), std::move( aDone ) );

    mdw::DownloadRevisionRequest request;
    request.set_item_id( *id );
    request.set_revision_id( MdwToUtf8( aRevisionId ) );
    request.set_local_path( MdwToUtf8( aLocalPath ) );

    return MdwCall<mdw::DownloadRevisionRequest, mdw::DownloadRevisionReply, MDW_REVISION>(
            shared_from_this(), MDW_CAPABILITY_GROUP::REVISIONS, request,
            []( const mdw::DownloadRevisionReply& aReply )
            {
                return MdwToStruct( aReply.revision() );
            },
            std::move( aDone ) );
}


MDW_OP MDW_SOURCE::Permissions( const MDW_ITEM_REF& aItem, const wxString& aRevisionId,
                                MDW_DONE<std::vector<kiapi::workspace::Permission>> aDone )
{
    std::optional<std::string> id = MdwWireId( *this, aItem );

    if( !id )
        return MdwForeignItem( shared_from_this(), std::move( aDone ) );

    mdw::GetPermissionsRequest request;
    request.set_item_id( *id );
    request.set_revision_id( MdwToUtf8( aRevisionId ) );

    return MdwCall<mdw::GetPermissionsRequest, mdw::GetPermissionsReply, std::vector<kiapi::workspace::Permission>>(
            shared_from_this(), MDW_CAPABILITY_GROUP::PERMISSIONS, request,
            []( const mdw::GetPermissionsReply& aReply )
            {
                std::vector<kiapi::workspace::Permission> permissions( aReply.permissions().begin(),
                                                                       aReply.permissions().end() );

                for( kiapi::workspace::Permission& permission : permissions )
                    MdwSanitize( permission );

                return permissions;
            },
            std::move( aDone ) );
}
