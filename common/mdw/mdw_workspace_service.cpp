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

#include <mdw/mdw_workspace_service.h>
#include <mdw/mdw_proto_convert.h>

#include <api/mdw/backend_base.pb.h>
#include <api/mdw/backend_locking.pb.h>

#include <kiid.h>

#include <functional>
#include <set>
#include <string>

namespace
{

constexpr int OPERATION_TIMEOUT_MS = 60000;


/// Send aRequest for every page, handing each reply to aEach
template <typename REQUEST, typename REPLY>
MDW_RESULT allPages( MDW_BACKEND& aBackend, REQUEST& aRequest, const std::function<void( const REPLY& )>& aEach )
{
    std::set<std::string> tokens;

    do
    {
        REPLY      reply;
        MDW_RESULT result = aBackend.CallSync( aRequest, reply, OPERATION_TIMEOUT_MS );

        if( !result.IsOk() )
            return result;

        aEach( reply );

        // A backend that hands back a token it gave before would keep us here forever
        if( !reply.next_page_token().empty() && !tokens.insert( reply.next_page_token() ).second )
            return MDW_RESULT::Fail( MDW_ERROR_CODE::PROTOCOL );

        aRequest.mutable_page()->set_page_token( reply.next_page_token() );
    } while( !aRequest.page().page_token().empty() );

    return MDW_RESULT::Ok();
}

} // namespace


MDW_WORKSPACE_SERVICE::MDW_WORKSPACE_SERVICE( std::shared_ptr<MDW_BACKEND>       aBackend,
                                              std::shared_ptr<MDW_EVENT_BUS>     aBus,
                                              std::shared_ptr<MDW_LOCK_TRACKER>  aLocks,
                                              std::shared_ptr<MDW_DIRTY_TRACKER> aDirty,
                                              const wxString&                    aWorkspaceId,
                                              const wxString&                    aClientIdentity ) :
        m_backend( std::move( aBackend ) ),
        m_bus( std::move( aBus ) ),
        m_locks( std::move( aLocks ) ),
        m_dirty( std::move( aDirty ) ),
        m_workspaceId( aWorkspaceId ),
        m_clientIdentity( aClientIdentity )
{
}


bool MDW_WORKSPACE_SERVICE::Supports( MDW_CAPABILITY_GROUP aGroup ) const
{
    return m_backend && m_backend->Has( aGroup );
}


wxString MDW_WORKSPACE_SERVICE::BeginOperation( const wxString& aIdempotencyId )
{
    wxString operationId = aIdempotencyId.IsEmpty() ? KIID().AsString() : aIdempotencyId;

    m_bus->ExpectEcho( operationId );

    return operationId;
}


void MDW_WORKSPACE_SERVICE::AbandonOperation( const wxString& aOperationId, MDW_ERROR_CODE aCode )
{
    switch( aCode )
    {
    // The backend may have done it anyway, so its echo must still read as this client's own
    case MDW_ERROR_CODE::CANCELLED:
    case MDW_ERROR_CODE::TIMEOUT:
    case MDW_ERROR_CODE::NETWORK:
    case MDW_ERROR_CODE::BACKEND_LOST:
    case MDW_ERROR_CODE::PROTOCOL:
        break;

    default:
        m_bus->CancelEcho( aOperationId );
    }
}


void MDW_WORKSPACE_SERVICE::Emit( MDW_EVENT_TYPE aType, const wxString& aItemId, const wxString& aOperationId,
                                  const std::function<void()>& aApply )
{
    MDW_EVENT event;
    event.type = aType;
    event.workspace_id = m_workspaceId;
    event.item_id = aItemId;
    event.client_identity = m_clientIdentity;
    event.operation_id = aOperationId;

    m_bus->PublishLocal( event, aApply );
}


void MDW_WORKSPACE_SERVICE::ApplyLock( const wxString& aItemId, const MDW_LOCK_INFO& aLock, MDW_EVENT_TYPE aType,
                                       const wxString& aOperationId )
{
    Emit( aType, aItemId, aOperationId,
          [&]()
          {
              m_locks->Set( aItemId, aLock );
          } );
}


MDW_RESULT MDW_WORKSPACE_SERVICE::ListLocks()
{
    const std::uint64_t               since = m_locks->Clock();
    kiapi::mdw::ListLocksRequest      request;
    std::map<wxString, MDW_LOCK_INFO> locks;
    MDW_RESULT                        result = MDW_RESULT::Fail( MDW_ERROR_CODE::NOT_SUPPORTED );

    // Every lock missing from the list is dropped, so the list must be whole
    if( Supports( MDW_CAPABILITY_GROUP::LOCKING ) )
    {
        result = allPages<kiapi::mdw::ListLocksRequest, kiapi::mdw::ListLocksReply>(
                *m_backend, request,
                [&]( const kiapi::mdw::ListLocksReply& aReply )
                {
                    for( const kiapi::mdw::ItemLock& entry : aReply.locks() )
                        locks[MdwFromUtf8( entry.item_id() )] = MdwToStruct( entry.lock() );
                } );
    }

    if( result.IsOk() )
        m_locks->ReplaceAll( locks, since );
    else if( result.error == MDW_ERROR_CODE::NOT_SUPPORTED )
        m_locks->TrustEvents(); // Locks a backend cannot list reach the tracker only as events
    else
        return result;

    // Every lock may have changed, and no item event says so
    MDW_EVENT event;
    event.type = MDW_EVENT_TYPE::LOCKS_CHANGED;
    event.workspace_id = m_workspaceId;
    m_bus->Publish( event );

    return result;
}
