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

#include <mdw/client/mdw_part.h>
#include <mdw/client/mdw_source.h>

#include "mdw_call.h"
#include "mdw_pending_changes.h"

#include <api/mdw/backend_lifecycle.pb.h>

#include <utility>

namespace mdw = kiapi::mdw;

namespace
{

/// A release pushes content to a server and may wait on its hooks
constexpr int RELEASE_TIMEOUT_MS = 5 * MDW_CALL_TIMEOUT_MS;

} // namespace


MDW_PART::MDW_PART( std::shared_ptr<MDW_SOURCE> aSource, const MDW_ITEM_REF& aItem ) :
        m_source( std::move( aSource ) ),
        m_item( aItem )
{
}


MDW_OP MDW_PART::Definition( MDW_DONE<kiapi::workspace::LifecycleDefinition> aDone )
{
    if( m_source && !MdwOwnItem( *m_source, m_item ) )
        return MdwForeignItem( m_source, std::move( aDone ) );

    mdw::GetLifecycleDefinitionRequest request;
    request.set_item_id( MdwToUtf8( m_item.item_id ) );

    return MdwCall<mdw::GetLifecycleDefinitionRequest, mdw::GetLifecycleDefinitionReply,
                   kiapi::workspace::LifecycleDefinition>(
            m_source, MDW_CAPABILITY_GROUP::LIFECYCLE, request,
            []( const mdw::GetLifecycleDefinitionReply& aReply )
            {
                kiapi::workspace::LifecycleDefinition definition = aReply.definition();

                for( kiapi::workspace::CustomState& state : *definition.mutable_states() )
                    MdwSanitize( state );

                return definition;
            },
            std::move( aDone ) );
}


MDW_OP MDW_PART::Transitions( const wxString&                                              aRevisionId,
                              MDW_DONE<std::vector<kiapi::workspace::LifecycleTransition>> aDone )
{
    if( m_source && !MdwOwnItem( *m_source, m_item ) )
        return MdwForeignItem( m_source, std::move( aDone ) );

    mdw::GetAvailableTransitionsRequest request;
    request.set_item_id( MdwToUtf8( m_item.item_id ) );
    request.set_revision_id( MdwToUtf8( aRevisionId ) );

    return MdwCall<mdw::GetAvailableTransitionsRequest, mdw::GetAvailableTransitionsReply,
                   std::vector<kiapi::workspace::LifecycleTransition>>(
            m_source, MDW_CAPABILITY_GROUP::LIFECYCLE, request,
            []( const mdw::GetAvailableTransitionsReply& aReply )
            {
                std::vector<kiapi::workspace::LifecycleTransition> transitions( aReply.transitions().begin(),
                                                                                aReply.transitions().end() );

                for( kiapi::workspace::LifecycleTransition& transition : transitions )
                    MdwSanitize( transition );

                return transitions;
            },
            std::move( aDone ) );
}


MDW_OP MDW_PART::Transition( const wxString& aRevisionId, const wxString& aTargetStateId, const wxString& aComment,
                             MDW_DONE<MDW_TRANSITION> aDone )
{
    if( m_source && !MdwOwnItem( *m_source, m_item ) )
        return MdwForeignItem( m_source, std::move( aDone ) );

    mdw::TransitionLifecycleRequest request;
    request.set_item_id( MdwToUtf8( m_item.item_id ) );
    request.set_revision_id( MdwToUtf8( aRevisionId ) );
    request.set_target_state_id( MdwToUtf8( aTargetStateId ) );
    request.set_comment( MdwToUtf8( aComment ) );

    MDW_TRACKED_OP op = m_source ? MdwBeginOperation( *m_source ) : MDW_TRACKED_OP();
    request.set_idempotency_id( MdwToUtf8( op.id.IsEmpty() ? KIID().AsString() : op.id ) );

    return MdwCall<mdw::TransitionLifecycleRequest, mdw::TransitionLifecycleReply, MDW_TRANSITION>(
            op, m_source, MDW_CAPABILITY_GROUP::LIFECYCLE, request,
            []( const mdw::TransitionLifecycleReply& aReply )
            {
                MDW_TRANSITION transition;
                transition.outcome = MdwFromProto( aReply.outcome() );
                transition.state = MdwToStruct( aReply.state() );
                transition.pending = MdwToStruct( aReply.pending() );
                return transition;
            },
            std::move( aDone ),
            [op, itemId = m_item.item_id]( const MDW_RESULT& aResult, const MDW_TRANSITION& aTransition )
            {
                // A pending transition changes nothing until it is decided
                if( aResult.IsOk() && aTransition.outcome == MDW_CHANGE_OUTCOME::APPLIED )
                    op.Emit( MDW_EVENT_TYPE::LIFECYCLE_CATEGORY_CHANGED, itemId );
                else
                    op.Abandon( aResult );
            } );
}


MDW_OP MDW_PART::Release( const MDW_RELEASE_REQUEST& aRequest, MDW_DONE<MDW_CHANGE> aDone )
{
    if( m_source && !MdwOwnItem( *m_source, m_item ) )
        return MdwForeignItem( m_source, std::move( aDone ) );

    mdw::ReleaseRevisionRequest request;
    request.set_item_id( MdwToUtf8( m_item.item_id ) );
    request.set_base_revision_id( MdwToUtf8( aRequest.base_revision_id ) );
    request.set_local_path( MdwToUtf8( aRequest.local_path ) );
    request.set_content_digest( MdwToUtf8( aRequest.content_digest ) );
    request.set_member_digest( MdwToUtf8( aRequest.member_digest ) );
    request.set_comment( MdwToUtf8( aRequest.comment ) );
    request.set_requested_label( MdwToUtf8( aRequest.requested_label ) );

    MDW_TRACKED_OP op = m_source ? MdwBeginOperation( *m_source ) : MDW_TRACKED_OP();
    std::uint64_t  saved = op.DirtyGeneration( m_item.item_id );
    request.set_idempotency_id( MdwToUtf8( op.id.IsEmpty() ? KIID().AsString() : op.id ) );

    return MdwCall<mdw::ReleaseRevisionRequest, mdw::ReleaseRevisionReply, MDW_CHANGE>(
            op, m_source, MDW_CAPABILITY_GROUP::LIFECYCLE, request,
            []( const mdw::ReleaseRevisionReply& aReply )
            {
                MDW_CHANGE change;
                change.outcome = MdwFromProto( aReply.outcome() );
                change.revision = MdwToStruct( aReply.revision() );
                change.pending = MdwToStruct( aReply.pending() );
                change.snapshot.token = MdwFromUtf8( aReply.snapshot().token() );
                change.snapshot.label = MdwFromUtf8( aReply.snapshot().label() );
                change.base_snapshot = MdwFromUtf8( aReply.base_snapshot() );
                return change;
            },
            std::move( aDone ),
            [op, source = m_source, item = m_item, saved]( const MDW_RESULT& aResult, const MDW_CHANGE& aChange )
            {
                // Kept even when the caller has gone, since its change id is the only way back to the release
                if( source && aResult.IsOk() && aChange.outcome == MDW_CHANGE_OUTCOME::PENDING )
                {
                    MDW_PENDING_CHANGES::Get().Remember(
                            { source->Info().locator, aChange.pending, item, MDW_PENDING_KIND::PART_REVISION } );
                }

                if( !aResult.IsOk() || aChange.outcome != MDW_CHANGE_OUTCOME::APPLIED )
                {
                    op.Abandon( aResult );
                    return;
                }

                // The release published what was saved and gave up the claim that revised the part
                op.ClearDirty( item.item_id, saved );
                op.Emit( MDW_EVENT_TYPE::REVISION_RELEASED, item.item_id, MDW_LOCK_INFO() );
            },
            RELEASE_TIMEOUT_MS );
}


MDW_OP MDW_PART::Revise( const wxString& aLocalPath, MDW_DONE<MDW_CHECKOUT_RESULT> aDone )
{
    if( !m_source )
        return MdwFailNow<MDW_CHECKOUT_RESULT>( MDW_RESULT::Fail( MDW_ERROR_CODE::NOT_SUPPORTED ), std::move( aDone ) );

    if( !MdwOwnItem( *m_source, m_item ) )
        return MdwForeignItem( m_source, std::move( aDone ) );

    // Revising opens the released item for edits, so it needs the lifecycle that released it
    if( !m_source->Has( MDW_CAPABILITY_GROUP::LIFECYCLE ) )
        return MdwFailNow<MDW_CHECKOUT_RESULT>( MDW_RESULT::Fail( MDW_ERROR_CODE::NOT_SUPPORTED ), std::move( aDone ) );

    MDW_CHECKOUT_REQUEST request;
    request.item_id = m_item.item_id;
    request.local_path = aLocalPath;
    request.lock = true;

    return m_source->Checkout( request, std::move( aDone ) );
}
