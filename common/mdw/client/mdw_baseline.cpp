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

#include <mdw/client/mdw_baseline.h>
#include <mdw/client/mdw_source.h>
#include <mdw/mdw_proto_convert.h>

#include "mdw_call.h"
#include "mdw_pending_changes.h"

#include <api/mdw/backend_revisions.pb.h>
#include <kiid.h>

#include <utility>

namespace mdw = kiapi::mdw;
namespace ws = kiapi::workspace;


namespace
{

// The receiving backend's own items travel with an empty source
void toProto( ws::ItemRef* aOut, const MDW_ITEM_REF& aRef, const wxString& aOwnLocator )
{
    aOut->set_item_id( MdwToUtf8( aRef.item_id ) );
    aOut->set_path_hint( MdwToUtf8( aRef.path_hint ) );

    if( aRef.source != aOwnLocator )
        aOut->set_source( MdwToUtf8( aRef.source ) );
}


void toProto( mdw::PartFacts* aOut, const MDW_PART_FACTS& aFacts )
{
    for( mdw::PlacedPartStatus status : aFacts.statuses )
        aOut->add_statuses( status );

    aOut->set_matched_revision_id( MdwToUtf8( aFacts.matched_revision_id ) );
    aOut->set_matched_revision_label( MdwToUtf8( aFacts.matched_revision_label ) );
    aOut->set_latest_revision_label( MdwToUtf8( aFacts.latest_revision_label ) );
    aOut->set_message( MdwToUtf8( aFacts.message ) );
    MdwToProto( aOut->mutable_matched_state(), aFacts.matched_state );
}


MDW_PART_FACTS fromProto( const mdw::PartFacts& aProto )
{
    MDW_PART_FACTS facts;

    for( int status : aProto.statuses() )
    {
        const bool known = mdw::PlacedPartStatus_IsValid( status );
        facts.statuses.push_back( known ? static_cast<mdw::PlacedPartStatus>( status ) : mdw::PPS_UNKNOWN );
    }

    facts.matched_revision_id = MdwFromUtf8( aProto.matched_revision_id() );
    facts.matched_revision_label = MdwFromUtf8( aProto.matched_revision_label() );
    facts.latest_revision_label = MdwFromUtf8( aProto.latest_revision_label() );
    facts.message = MdwFromUtf8( aProto.message() );
    facts.matched_state = MdwToStruct( aProto.matched_state() );

    return facts;
}


void toProto( google::protobuf::RepeatedPtrField<mdw::PlacedPart>* aOut, const std::vector<MDW_BASELINE_PART>& aParts,
              const wxString& aOwnLocator )
{
    for( const MDW_BASELINE_PART& part : aParts )
    {
        mdw::PlacedPart* out = aOut->Add();

        toProto( out->mutable_item(), part.item, aOwnLocator );
        out->set_lib_id( MdwToUtf8( part.lib_id ) );
        out->set_content_digest( MdwToUtf8( part.content_digest ) );

        for( const wxString& reference : part.references )
            out->add_references( MdwToUtf8( reference ) );

        if( !part.part.item_id.IsEmpty() )
            toProto( out->mutable_part(), part.part, aOwnLocator );

        out->set_part_revision_id( MdwToUtf8( part.part_revision_id ) );

        if( part.owner_facts )
            toProto( out->mutable_owner_facts(), *part.owner_facts );
    }
}


/// A reply whose rows do not line up one to one with the request is a backend fault, not a verdict
template <typename T>
bool oneEach( const T& aRows, size_t aExpected )
{
    return static_cast<size_t>( aRows.size() ) == aExpected;
}

} // namespace


bool MdwReadBaselineVerdict( const kiapi::mdw::CheckBaselineReply& aReply, size_t aCount,
                             MDW_BASELINE_VERDICT& aVerdict )
{
    if( !oneEach( aReply.parts(), aCount ) )
        return false;

    aVerdict.checks.resize( aCount );
    std::vector<bool> seen( aCount, false );

    // Results address parts by index, so each index must name exactly one request part
    for( const mdw::PlacedPartCheck& check : aReply.parts() )
    {
        if( check.part_index() >= aCount || seen[check.part_index()] )
            return false;

        // A verdict left unset is no clearance
        if( check.severity() == mdw::SEV_UNKNOWN )
            return false;

        seen[check.part_index()] = true;

        MDW_BASELINE_CHECK& out = aVerdict.checks[check.part_index()];
        out.part_index = check.part_index();
        out.facts = fromProto( check.facts() );
        out.severity = MdwFromProto( check.severity() );
    }

    // A hold left unset or from a newer protocol is no clearance
    if( aReply.hold() == mdw::HOLD_UNKNOWN || !mdw::BaselineHold_IsValid( aReply.hold() ) )
        return false;

    aVerdict.hold = MdwFromProto( aReply.hold() );
    aVerdict.holder = wxString::FromUTF8( aReply.holder() );
    return true;
}


MDW_BASELINE::MDW_BASELINE( std::shared_ptr<MDW_SOURCE> aSource ) :
        m_source( std::move( aSource ) )
{
}


MDW_OP MDW_BASELINE::Facts( const std::vector<MDW_BASELINE_PART>& aParts, MDW_DONE<std::vector<MDW_PART_FACTS>> aDone )
{
    mdw::CheckPlacedPartsRequest request;
    toProto( request.mutable_parts(), aParts, m_source ? m_source->Info().locator : wxString() );

    return MdwCall<mdw::CheckPlacedPartsRequest, mdw::CheckPlacedPartsReply, std::vector<MDW_PART_FACTS>>(
            m_source, MDW_CAPABILITY_GROUP::REVISIONS, request,
            [count = aParts.size()]( const mdw::CheckPlacedPartsReply& aReply, std::vector<MDW_PART_FACTS>& aOut )
            {
                if( !oneEach( aReply.facts(), count ) )
                    return false;

                for( const mdw::PartFacts& facts : aReply.facts() )
                    aOut.push_back( fromProto( facts ) );

                return true;
            },
            std::move( aDone ) );
}


MDW_OP MDW_BASELINE::Check( const MDW_ITEM_REF& aProject, const wxString& aRevisionId,
                            const std::vector<MDW_BASELINE_PART>& aParts, MDW_DONE<MDW_BASELINE_VERDICT> aDone )
{
    if( m_source && !MdwOwnItem( *m_source, aProject ) )
        return MdwForeignItem( m_source, std::move( aDone ) );

    mdw::CheckBaselineRequest request;
    request.set_item_id( MdwToUtf8( aProject.item_id ) );
    request.set_revision_id( MdwToUtf8( aRevisionId ) );
    toProto( request.mutable_parts(), aParts, m_source ? m_source->Info().locator : wxString() );

    return MdwCall<mdw::CheckBaselineRequest, mdw::CheckBaselineReply, MDW_BASELINE_VERDICT>(
            m_source, MDW_CAPABILITY_GROUP::REVISIONS, request,
            [count = aParts.size()]( const mdw::CheckBaselineReply& aReply, MDW_BASELINE_VERDICT& aVerdict )
            {
                return MdwReadBaselineVerdict( aReply, count, aVerdict );
            },
            std::move( aDone ) );
}


MDW_OP MDW_BASELINE::Create( const MDW_BASELINE_REQUEST& aRequest, MDW_DONE<MDW_CHANGE> aDone )
{
    if( m_source && !MdwOwnItem( *m_source, aRequest.project ) )
        return MdwForeignItem( m_source, std::move( aDone ) );

    const wxString ownLocator = m_source ? m_source->Info().locator : wxString();

    mdw::CreateBaselineRequest request;
    request.set_item_id( MdwToUtf8( aRequest.project.item_id ) );
    request.set_revision_id( MdwToUtf8( aRequest.revision_id ) );
    toProto( request.mutable_parts(), aRequest.parts, ownLocator );

    for( const MDW_BASELINE_EXCEPTION& exception : aRequest.exceptions )
    {
        mdw::BaselineException* out = request.add_exceptions();
        out->set_part_index( exception.part_index );
        out->set_note( MdwToUtf8( exception.note ) );

        if( !exception.eco.eco_id.IsEmpty() )
        {
            out->mutable_eco()->set_eco_id( MdwToUtf8( exception.eco.eco_id ) );

            if( exception.eco.source != ownLocator )
                out->mutable_eco()->set_source( MdwToUtf8( exception.eco.source ) );
        }
    }

    request.set_comment( MdwToUtf8( aRequest.comment ) );
    request.set_requested_label( MdwToUtf8( aRequest.requested_label ) );
    request.set_idempotency_id( KIID().AsStdString() );

    return MdwCall<mdw::CreateBaselineRequest, mdw::CreateBaselineReply, MDW_CHANGE>(
            m_source, MDW_CAPABILITY_GROUP::REVISIONS, request,
            []( const mdw::CreateBaselineReply& aReply, MDW_CHANGE& aOut )
            {
                aOut.outcome = MdwFromProto( aReply.outcome() );
                aOut.revision = MdwToStruct( aReply.baseline() );
                aOut.pending = MdwToStruct( aReply.pending() );

                // A release with no baseline, or a pending change the user cannot follow up, is no outcome at all
                if( aOut.outcome == MDW_CHANGE_OUTCOME::APPLIED )
                    return !aOut.revision.revision_id.IsEmpty();

                return aOut.outcome == MDW_CHANGE_OUTCOME::PENDING && !aOut.pending.change_id.IsEmpty();
            },
            std::move( aDone ),
            // Kept even when the caller has gone, since its change id is the only way back to the release
            [source = m_source, project = MdwStamped( aRequest.project, ownLocator )]( const MDW_RESULT& aResult,
                                                                                       const MDW_CHANGE& aChange )
            {
                if( source && aResult.IsOk() && aChange.outcome == MDW_CHANGE_OUTCOME::PENDING )
                {
                    MDW_PENDING_CHANGES::Get().Remember(
                            { source->Info().locator, aChange.pending, project, MDW_PENDING_KIND::DESIGN_BASELINE } );
                }
            } );
}
