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

/**
 * @file mdw_call.h
 * One backend request behind a client op. Client implementations only.
 */

#ifndef MDW_CALL_H
#define MDW_CALL_H

#include <kiid.h>
#include <mdw/client/mdw_source.h>
#include <mdw/mdw_backend.h>
#include <mdw/mdw_session.h>
#include <mdw/mdw_workspace_service.h>

#include "mdw_op_state.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include <wx/translation.h>

/// Long enough for a backend that talks to a remote server, short enough that a hung one is noticed
constexpr int MDW_CALL_TIMEOUT_MS = 60000;

/**
 * For a call that downloads a whole library copy, which takes minutes on a slow link. The backend reports no
 * progress, so this only bounds a hung one for callers that cannot cancel; an interactive caller cancels sooner.
 */
constexpr int MDW_TRANSFER_TIMEOUT_MS = 60 * 60 * 1000;


/// An item names this source when its source is empty or this source's locator
inline bool MdwOwnItem( const MDW_SOURCE& aSource, const MDW_ITEM_REF& aItem )
{
    return aItem.source.IsEmpty() || aItem.source == aSource.Info().locator;
}


/// The id a request names aItem by, or nothing when it belongs to another source
inline std::optional<std::string> MdwWireId( const MDW_SOURCE& aSource, const MDW_ITEM_REF& aItem )
{
    if( !MdwOwnItem( aSource, aItem ) )
        return std::nullopt;

    return MdwToUtf8( aItem.item_id );
}


/// aDone, holding aSource alive until it has run
template <typename T>
MDW_DONE<T> MdwKeeping( const std::shared_ptr<MDW_SOURCE>& aSource, MDW_DONE<T> aDone )
{
    return [source = aSource, done = std::move( aDone )]( const MDW_RESULT& aResult, const T& aValue )
    {
        if( done )
            done( aResult, aValue );
    };
}


/// Refuse, unsent, a request naming another source's item
template <typename T>
MDW_OP MdwForeignItem( const std::shared_ptr<MDW_SOURCE>& aSource, MDW_DONE<T> aDone )
{
    return MdwFailNow<T>( MDW_RESULT::Fail( MDW_ERROR_CODE::INVALID_PARAMETER,
                                            wxS( "The item belongs to another source" ) ),
                          MdwKeeping( aSource, std::move( aDone ) ) );
}


/// A reference we keep always names its source
inline MDW_ITEM_REF MdwStamped( MDW_ITEM_REF aItem, const wxString& aLocator )
{
    if( aItem.source.IsEmpty() )
        aItem.source = aLocator;

    return aItem;
}


inline void MdwStamp( kiapi::workspace::ItemRef& aItem, const wxString& aLocator )
{
    if( aItem.source().empty() )
        aItem.set_source( MdwToUtf8( aLocator ) );
}


inline void MdwToProto( kiapi::mdw::PageRequest* aOut, const MDW_PAGE& aPage )
{
    aOut->set_page_size( aPage.page_size );
    aOut->set_page_token( MdwToUtf8( aPage.page_token ) );
}


/// A new idempotency id for one mutating op
inline std::string MdwNewOperationId()
{
    return KIID().AsStdString();
}


/**
 * The session bookkeeping around one mutating op, so the lock and dirty trackers and the local events
 * stay as the workspace service's own operations leave them. The op's request goes to the backend of
 * the session the bookkeeping began on. Inert while the source has no session.
 */
struct MDW_TRACKED_OP
{
    /// Weak, so a session closed mid-op closes then rather than on the transport thread that completes the op
    std::weak_ptr<MDW_SESSION>             session;
    std::shared_ptr<MDW_WORKSPACE_SERVICE> service;
    std::shared_ptr<MDW_BACKEND>           backend;
    wxString                               id;      ///< Also the request's idempotency id

    void ApplyLock( const wxString& aItemId, const MDW_LOCK_INFO& aLock, MDW_EVENT_TYPE aType ) const
    {
        if( service )
            service->ApplyLock( aItemId, aLock, aType, id );
    }

    /// Publish aType for aItemId, also leaving its lock as aLock when one is given
    void Emit( MDW_EVENT_TYPE aType, const wxString& aItemId,
               const std::optional<MDW_LOCK_INFO>& aLock = std::nullopt ) const
    {
        if( !service )
            return;

        MDW_WORKSPACE_SERVICE* tracked = service.get();

        service->Emit( aType, aItemId, id,
                       [tracked, aItemId, aLock]()
                       {
                           if( aLock )
                               tracked->LockTracker().Set( aItemId, *aLock );
                       } );
    }

    /// Taken before the request goes out, so ClearDirty spares a save made while it was in flight
    std::uint64_t DirtyGeneration( const wxString& aItemId ) const
    {
        return service ? service->DirtyTracker().Generation( aItemId ) : 0;
    }

    void ClearDirty( const wxString& aItemId, std::uint64_t aGeneration ) const
    {
        if( service )
            service->DirtyTracker().ClearIfUnchanged( aItemId, aGeneration );
    }

    /// Pass the backend's progress for this op to the sink open on the calling thread, until the handle is released
    std::shared_ptr<void> WatchProgress() const
    {
        MDW_PROGRESS_SCOPE::SINK     sink = MDW_PROGRESS_SCOPE::Current();
        std::shared_ptr<MDW_SESSION> live = session.lock();

        return live && sink ? live->WatchProgress( id, std::move( sink ) ) : nullptr;
    }

    /// For an op that failed, or that the backend left pending
    void Abandon( const MDW_RESULT& aResult ) const
    {
        if( service )
            service->AbandonOperation( id, aResult.IsOk() ? MDW_ERROR_CODE::SUCCESS : aResult.error );
    }
};


inline MDW_TRACKED_OP MdwBeginOperation( const MDW_SOURCE& aSource, const wxString& aIdempotencyId = wxEmptyString )
{
    MDW_TRACKED_OP               op;
    std::shared_ptr<MDW_SESSION> session = aSource.Session();

    op.session = session;
    op.service = session ? session->Service() : nullptr;
    op.backend = op.service ? op.service->Backend() : nullptr;

    if( op.service )
        op.id = op.service->BeginOperation( aIdempotencyId );
    else
        op.id = aIdempotencyId.IsEmpty() ? KIID().AsString() : aIdempotencyId;

    return op;
}


/// Runs on the settle thread once an op has its answer, even when nobody waits for it any more
template <typename T>
using MDW_SETTLE = std::function<void( const MDW_RESULT&, const T& )>;


/// A conversion for a reply every value of which is a valid answer
template <typename REPLY, typename T>
std::function<bool( const REPLY&, T& )> MdwAlwaysValid( std::function<T( const REPLY& )> aConvert )
{
    return [convert = std::move( aConvert )]( const REPLY& aReply, T& aValue )
    {
        aValue = convert( aReply );
        return true;
    };
}


/**
 * Send aRequest to aBackend on behalf of aSource and complete with what aConvert fills from the reply.
 * Without aGroup in aBackend's capabilities the op completes NOT_SUPPORTED with no round trip;
 * std::nullopt marks a mandatory request. aConvert runs only on success, on the transport's thread, and
 * returns false for a reply that breaks the contract, which completes PROTOCOL with a default value.
 * aSettle keeps local bookkeeping in step with what the backend did, whether or not the op is still
 * wanted. The op keeps aSource alive until its callback has run.
 */
template <typename REQ, typename REPLY, typename T>
MDW_OP MdwCallOn( const std::shared_ptr<MDW_SOURCE>& aSource, const std::shared_ptr<MDW_BACKEND>& aBackend,
                  std::optional<MDW_CAPABILITY_GROUP> aGroup, const REQ& aRequest,
                  std::function<bool( const REPLY&, T& )> aConvert, MDW_DONE<T> aDone, MDW_SETTLE<T> aSettle,
                  int aTimeoutMs, std::shared_ptr<void> aProgress = nullptr )
{
    MDW_DONE<T> kept = MdwKeeping( aSource, std::move( aDone ) );

    auto failNow = [&]( const MDW_RESULT& aResult )
    {
        if( aSettle )
            aSettle( aResult, T() );

        return MdwFailNow<T>( aResult, std::move( kept ) );
    };

    if( !aSource )
        return failNow( MDW_RESULT::Fail( MDW_ERROR_CODE::NOT_SUPPORTED ) );

    // A source can be handed out before its session opens, or while it restarts
    if( !aBackend )
    {
        return failNow( MDW_RESULT::Fail( MDW_ERROR_CODE::BACKEND_LOST,
                                          wxString::Format( _( "%s is not connected." ), aSource->Info().locator ) ) );
    }

    if( aGroup && !aBackend->Has( *aGroup ) )
        return failNow( MDW_RESULT::Fail( MDW_ERROR_CODE::NOT_SUPPORTED ) );

    auto state = std::make_shared<MDW_OP_STATE>();

    // The transport thread copies and drops its callback, so it holds only the state and never the caller's
    state->Hold<T>( std::move( kept ) );

    MDW_BACKEND::CALL call = aBackend->Call<REQ, REPLY>(
            aRequest, aTimeoutMs,
            [state, convert = std::move( aConvert ), settle = std::move( aSettle ),
             progress = std::move( aProgress )]( const MDW_RESULT& aResult, const REPLY& aReply )
            {
                MDW_RESULT result = aResult;
                T          value{};

                if( result.IsOk() && !convert( aReply, value ) )
                {
                    result = MDW_RESULT::Fail( MDW_ERROR_CODE::PROTOCOL );
                    value = T();
                }

                auto complete = [state, result, value]()
                {
                    state->Answer<T>( result, value );
                };

                if( !settle )
                {
                    complete();
                    return;
                }

                // Off the transport's thread, where a subscriber waiting on another request would hang it
                MDW_OP_STATE::Settle(
                        [settle, result, value = std::move( value ), complete = std::move( complete )]()
                        {
                            settle( result, value );
                            complete();
                        } );
            } );

    if( call )
    {
        state->SetCanceller(
                [weak = std::weak_ptr<MDW_BACKEND>( aBackend ), call]()
                {
                    if( std::shared_ptr<MDW_BACKEND> running = weak.lock() )
                        running->Cancel( call );
                } );

        state->SetStopper(
                [weak = std::weak_ptr<MDW_BACKEND>( aBackend ), call]()
                {
                    if( std::shared_ptr<MDW_BACKEND> running = weak.lock() )
                        running->StopServing( call );
                } );
    }

    return MDW_OP( state );
}


/// MdwCallOn the backend aSource has now
template <typename REQ, typename REPLY, typename T>
MDW_OP MdwCall( const std::shared_ptr<MDW_SOURCE>& aSource, std::optional<MDW_CAPABILITY_GROUP> aGroup,
                const REQ& aRequest, std::function<bool( const REPLY&, T& )> aConvert, MDW_DONE<T> aDone,
                MDW_SETTLE<T> aSettle = nullptr, int aTimeoutMs = MDW_CALL_TIMEOUT_MS )
{
    std::shared_ptr<MDW_BACKEND> backend = aSource ? aSource->Backend() : nullptr;

    return MdwCallOn<REQ, REPLY, T>( aSource, backend, aGroup, aRequest, std::move( aConvert ), std::move( aDone ),
                                     std::move( aSettle ), aTimeoutMs );
}


template <typename REQ, typename REPLY, typename T>
MDW_OP MdwCall( const std::shared_ptr<MDW_SOURCE>& aSource, std::optional<MDW_CAPABILITY_GROUP> aGroup,
                const REQ& aRequest, std::function<T( const REPLY& )> aConvert, MDW_DONE<T> aDone,
                MDW_SETTLE<T> aSettle = nullptr, int aTimeoutMs = MDW_CALL_TIMEOUT_MS )
{
    return MdwCall<REQ, REPLY, T>( aSource, aGroup, aRequest, MdwAlwaysValid<REPLY, T>( std::move( aConvert ) ),
                                   std::move( aDone ), std::move( aSettle ), aTimeoutMs );
}


/// MdwCallOn the backend whose session aOp's bookkeeping began on
template <typename REQ, typename REPLY, typename T>
MDW_OP MdwCall( const MDW_TRACKED_OP& aOp, const std::shared_ptr<MDW_SOURCE>& aSource,
                std::optional<MDW_CAPABILITY_GROUP> aGroup, const REQ& aRequest,
                std::function<bool( const REPLY&, T& )> aConvert, MDW_DONE<T> aDone, MDW_SETTLE<T> aSettle,
                int aTimeoutMs = MDW_CALL_TIMEOUT_MS )
{
    return MdwCallOn<REQ, REPLY, T>( aSource, aOp.backend, aGroup, aRequest, std::move( aConvert ), std::move( aDone ),
                                     std::move( aSettle ), aTimeoutMs, aOp.WatchProgress() );
}


template <typename REQ, typename REPLY, typename T>
MDW_OP MdwCall( const MDW_TRACKED_OP& aOp, const std::shared_ptr<MDW_SOURCE>& aSource,
                std::optional<MDW_CAPABILITY_GROUP> aGroup, const REQ& aRequest,
                std::function<T( const REPLY& )> aConvert, MDW_DONE<T> aDone, MDW_SETTLE<T> aSettle,
                int aTimeoutMs = MDW_CALL_TIMEOUT_MS )
{
    return MdwCall<REQ, REPLY, T>( aOp, aSource, aGroup, aRequest, MdwAlwaysValid<REPLY, T>( std::move( aConvert ) ),
                                   std::move( aDone ), std::move( aSettle ), aTimeoutMs );
}

#endif // MDW_CALL_H
