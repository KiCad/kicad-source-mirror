/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include <mdw_runtime/mdw_lifecycle.h>
#include <mdw_runtime/mdw_member_facts.h>

#include <algorithm>
#include <set>

namespace mdw = kiapi::mdw;
namespace ws = kiapi::workspace;


namespace
{

/// What an item's releases say about a member placed with aDigest
mdw::PartFacts memberFacts( const MDW_LIFECYCLE_STORE& aStore, const std::vector<MDW_RELEASE_RECORD>* aReleases,
                            const std::string& aDigest )
{
    mdw::PartFacts facts;

    if( !aReleases )
    {
        facts.add_statuses( mdw::PPS_UNKNOWN );
        facts.set_message( aStore.Name() + " holds no such item" );
        return facts;
    }

    std::vector<MDW_RELEASE_VIEW> views;

    for( const MDW_RELEASE_RECORD& release : *aReleases )
    {
        views.push_back( { release.digest, release.state.category() == ws::LC_RELEASED, release.published,
                           !release.state.not_for_new_designs() } );
    }

    const MDW_MEMBER_VERDICT verdict = MdwWeighMember( views, aDigest );

    if( !aReleases->empty() )
        facts.set_latest_revision_label( aReleases->back().label );

    if( verdict.statuses.front() == mdw::PPS_UNKNOWN )
        facts.set_message( "The placed part carries no content digest" );

    if( verdict.matched >= 0 )
    {
        const MDW_RELEASE_RECORD& matched = ( *aReleases )[verdict.matched];

        facts.set_matched_revision_id( matched.revision_id );
        facts.set_matched_revision_label( matched.label );
        *facts.mutable_matched_state() = matched.state;
    }

    for( mdw::PlacedPartStatus status : verdict.statuses )
        facts.add_statuses( status );

    return facts;
}


/// Facts for every part, in order. A part another source owns brings its own
ws::WorkspaceError placedFacts( MDW_LIFECYCLE_STORE&                                       aStore,
                                const google::protobuf::RepeatedPtrField<mdw::PlacedPart>& aParts,
                                std::vector<mdw::PartFacts>&                               aOut )
{
    // Another source's member has nothing here, so its facts come with it or stay unknown
    auto local = [&]( const mdw::PlacedPart& aPart )
    {
        return !aPart.has_owner_facts() && aStore.Owns( aPart.item() );
    };

    std::set<std::string> items;

    for( const mdw::PlacedPart& part : aParts )
    {
        if( local( part ) )
            items.insert( part.item().item_id() );
    }

    std::map<std::string, std::vector<MDW_RELEASE_RECORD>> releases;

    if( ws::WorkspaceError error = aStore.Releases( items, releases ); error.code() != ws::WE_OK )
        return error;

    for( const mdw::PlacedPart& part : aParts )
    {
        if( part.has_owner_facts() )
        {
            aOut.push_back( part.owner_facts() );
            continue;
        }

        const std::vector<MDW_RELEASE_RECORD>* found = nullptr;

        if( local( part ) )
        {
            auto it = releases.find( part.item().item_id() );

            if( it != releases.end() )
                found = &it->second;
        }

        aOut.push_back( memberFacts( aStore, found, part.content_digest() ) );
    }

    return MdwOk();
}


/// A part unreleased, differing, unusable, unpublished or unchecked blocks a baseline; one merely outdated, not for
/// new designs or untracked only warns
mdw::Severity severity( const mdw::PartFacts& aFacts )
{
    mdw::Severity worst = aFacts.statuses().empty() ? mdw::SEV_WARNING : mdw::SEV_NONE;

    for( int status : aFacts.statuses() )
    {
        mdw::Severity each = mdw::SEV_WARNING;

        if( status == mdw::PPS_RELEASED )
            each = mdw::SEV_NONE;
        else if( status == mdw::PPS_NEVER_RELEASED || status == mdw::PPS_DIFFERS || status == mdw::PPS_NOT_USABLE
                 || status == mdw::PPS_UNPUBLISHED || status == mdw::PPS_UNVERIFIED )
            each = mdw::SEV_ERROR;

        worst = std::max( worst, each );
    }

    return worst;
}


ws::WorkspaceError checkParts( MDW_LIFECYCLE_STORE&                                       aStore,
                               const google::protobuf::RepeatedPtrField<mdw::PlacedPart>& aParts,
                               std::vector<mdw::PlacedPartCheck>&                         aOut )
{
    std::vector<mdw::PartFacts> facts;
    ws::WorkspaceError          error = placedFacts( aStore, aParts, facts );

    if( error.code() != ws::WE_OK )
        return error;

    for( std::size_t ii = 0; ii < facts.size(); ++ii )
    {
        mdw::PlacedPartCheck check;
        check.set_part_index( static_cast<uint32_t>( ii ) );
        check.set_severity( severity( facts[ii] ) );
        *check.mutable_facts() = std::move( facts[ii] );
        aOut.push_back( std::move( check ) );
    }

    return MdwOk();
}


/// A symbol and its footprint share designators, so the library id tells their lines apart
std::string describe( const mdw::PlacedPart& aPart )
{
    std::string references;

    for( const std::string& reference : aPart.references() )
        references += ( references.empty() ? "" : ", " ) + reference;

    const std::string& name = aPart.lib_id().empty() ? aPart.item().item_id() : aPart.lib_id();

    if( references.empty() )
        return name;

    return name.empty() ? references : name + " (" + references + ")";
}


template <typename Req, typename Resp>
void serve( MDW_BACKEND_SERVER& aServer, const MDW_LIFECYCLE_HOOKS& aHooks,
            ws::WorkspaceError ( MDW_LIFECYCLE_STORE::*aCall )( const Req&, Resp& ) )
{
    aServer.RegisterHandler<Req, Resp>(
            [use = aHooks.use, aCall]( const Req& aRequest, Resp& aReply )
            {
                return use(
                        [&]( MDW_LIFECYCLE_STORE& aStore )
                        {
                            return ( aStore.*aCall )( aRequest, aReply );
                        } );
            } );
}


/// aRecover run with the store held; a refusal fails the recovery
MDW_RECOVERY recoverWith( const MDW_LIFECYCLE_HOOKS&                                 aHooks,
                          const std::function<MDW_RECOVERY( MDW_LIFECYCLE_STORE& )>& aRecover,
                          ws::WorkspaceError&                                        aError )
{
    MDW_RECOVERY       result = MDW_RECOVERY::FAIL;
    ws::WorkspaceError refused = aHooks.use(
            [&]( MDW_LIFECYCLE_STORE& aStore )
            {
                result = aRecover( aStore );
                return MdwOk();
            } );

    if( refused.code() != ws::WE_OK )
    {
        aError = refused;
        return MDW_RECOVERY::FAIL;
    }

    return result;
}


template <typename Req, typename Resp>
void recover( MDW_BACKEND_SERVER& aServer, const MDW_LIFECYCLE_HOOKS& aHooks,
              MDW_RECOVERY ( MDW_LIFECYCLE_STORE::*aCall )( const Req&, Resp&, ws::WorkspaceError& ) )
{
    aServer.RegisterRecovery<Req, Resp>(
            []( const Req&, std::string& )
            {
                return MdwOk();
            },
            [aHooks, aCall]( const Req& aRequest, const std::string&, Resp& aReply, ws::WorkspaceError& aError )
            {
                return recoverWith(
                        aHooks,
                        [&]( MDW_LIFECYCLE_STORE& aStore )
                        {
                            return ( aStore.*aCall )( aRequest, aReply, aError );
                        },
                        aError );
            } );
}


void serveRevisions( MDW_BACKEND_SERVER& aServer, const MDW_LIFECYCLE_HOOKS& aHooks )
{
    aServer.RegisterHandler<mdw::CheckPlacedPartsRequest, mdw::CheckPlacedPartsReply>(
            [use = aHooks.use]( const mdw::CheckPlacedPartsRequest& aRequest, mdw::CheckPlacedPartsReply& aReply )
            {
                return use(
                        [&]( MDW_LIFECYCLE_STORE& aStore )
                        {
                            std::vector<mdw::PartFacts> facts;
                            ws::WorkspaceError          error = placedFacts( aStore, aRequest.parts(), facts );

                            if( error.code() != ws::WE_OK )
                                return error;

                            for( mdw::PartFacts& each : facts )
                                *aReply.add_facts() = std::move( each );

                            return MdwOk();
                        } );
            } );

    aServer.RegisterHandler<mdw::CheckBaselineRequest, mdw::CheckBaselineReply>(
            [use = aHooks.use]( const mdw::CheckBaselineRequest& aRequest, mdw::CheckBaselineReply& aReply )
            {
                return use(
                        [&]( MDW_LIFECYCLE_STORE& aStore )
                        {
                            MDW_BASELINE_SOURCE source;
                            ws::WorkspaceError  error =
                                    aStore.BaselineSource( aRequest.item_id(), aRequest.revision_id(), false, source );

                            std::vector<mdw::PlacedPartCheck> checks;

                            if( error.code() == ws::WE_OK )
                                error = checkParts( aStore, aRequest.parts(), checks );

                            if( error.code() != ws::WE_OK )
                                return error;

                            for( mdw::PlacedPartCheck& check : checks )
                                *aReply.add_parts() = std::move( check );

                            aReply.set_hold( source.hold );
                            aReply.set_holder( source.holder );
                            return MdwOk();
                        } );
            } );
}

} // namespace


void MdwServeLifecycle( MDW_BACKEND_SERVER& aServer, const MDW_LIFECYCLE_HOOKS& aHooks )
{
    serveRevisions( aServer, aHooks );
    serve( aServer, aHooks, &MDW_LIFECYCLE_STORE::Definition );
    serve( aServer, aHooks, &MDW_LIFECYCLE_STORE::Transitions );
    serve( aServer, aHooks, &MDW_LIFECYCLE_STORE::Transition );
    serve( aServer, aHooks, &MDW_LIFECYCLE_STORE::Baseline );
    recover( aServer, aHooks, &MDW_LIFECYCLE_STORE::RecoverTransition );
    recover( aServer, aHooks, &MDW_LIFECYCLE_STORE::RecoverBaseline );

    // The attempt and its recovery act on the token, which pins what the attempt acts on
    aServer.registerTokenHandler<mdw::ReleaseRevisionRequest, mdw::ReleaseRevisionReply>(
            [use = aHooks.use]( const mdw::ReleaseRevisionRequest& aRequest, const std::string& aToken,
                                mdw::ReleaseRevisionReply& aReply )
            {
                return use(
                        [&]( MDW_LIFECYCLE_STORE& aStore )
                        {
                            return aStore.Release( aRequest, aToken, aReply );
                        } );
            } );

    aServer.RegisterRecovery<mdw::ReleaseRevisionRequest, mdw::ReleaseRevisionReply>(
            [use = aHooks.use]( const mdw::ReleaseRevisionRequest& aRequest, std::string& aOut )
            {
                return use(
                        [&]( MDW_LIFECYCLE_STORE& aStore )
                        {
                            return aStore.ReleaseToken( aRequest, aOut );
                        } );
            },
            [aHooks]( const mdw::ReleaseRevisionRequest& aRequest, const std::string& aSaved,
                      mdw::ReleaseRevisionReply& aReply, ws::WorkspaceError& aError )
            {
                return recoverWith(
                        aHooks,
                        [&]( MDW_LIFECYCLE_STORE& aStore )
                        {
                            return aStore.RecoverRelease( aRequest, aSaved, aReply, aError );
                        },
                        aError );
            } );
}


ws::WorkspaceError MdwPlanBaseline( MDW_LIFECYCLE_STORE& aStore, const mdw::CreateBaselineRequest& aRequest,
                                    MDW_BASELINE_PLAN& aPlan )
{
    MDW_BASELINE_SOURCE source;
    ws::WorkspaceError  error = aStore.BaselineSource( aRequest.item_id(), aRequest.revision_id(), true, source );

    if( error.code() != ws::WE_OK )
        return error;

    if( std::optional<ws::WorkspaceError> refused =
                MdwRequireNextLabel( aStore, "baseline", aRequest.requested_label(), source.next_label ) )
    {
        return *refused;
    }

    if( source.held )
        return *source.held;

    std::set<uint32_t> excepted;

    for( const mdw::BaselineException& exception : aRequest.exceptions() )
    {
        if( exception.part_index() >= static_cast<uint32_t>( aRequest.parts_size() ) )
            return MdwFail( ws::WE_INVALID_PARAMETER, "An exception names a part the request lacks" );

        excepted.insert( exception.part_index() );
    }

    std::vector<mdw::PlacedPartCheck> checks;
    error = checkParts( aStore, aRequest.parts(), checks );

    if( error.code() != ws::WE_OK )
        return error;

    std::string blocked;

    for( const mdw::PlacedPartCheck& check : checks )
    {
        if( check.severity() == mdw::SEV_ERROR && !excepted.count( check.part_index() ) )
        {
            const mdw::PlacedPart& part = aRequest.parts( static_cast<int>( check.part_index() ) );
            blocked += ( blocked.empty() ? "" : "; " ) + describe( part );
        }
    }

    if( !blocked.empty() )
        return MdwFail( ws::WE_INVALID_STATE_TRANSITION, "These parts are not released as placed: " + blocked );

    aPlan.source = std::move( source );
    aPlan.checks = std::move( checks );
    return MdwOk();
}


std::string MdwBaselineDescription( const mdw::CreateBaselineRequest& aRequest )
{
    std::string description = aRequest.comment();

    for( const mdw::BaselineException& exception : aRequest.exceptions() )
    {
        if( exception.part_index() >= static_cast<uint32_t>( aRequest.parts_size() ) )
            continue;

        const std::string& eco = exception.eco().eco_id();

        description += ( description.empty() ? "" : "\n" ) + std::string( "Exception for " )
                       + describe( aRequest.parts( static_cast<int>( exception.part_index() ) ) ) + ": "
                       + exception.note() + ( eco.empty() ? "" : " (ECO " + eco + ")" );
    }

    return description;
}


std::optional<ws::WorkspaceError> MdwRequireNextLabel( const MDW_LIFECYCLE_STORE& aStore, const std::string& aWhat,
                                                       const std::string& aRequested, const std::string& aNext )
{
    if( aRequested.empty() || aRequested == aNext )
        return std::nullopt;

    return MdwFail( ws::WE_INVALID_PARAMETER,
                    aStore.Name() + " numbers revisions, so this " + aWhat + " can only be " + aNext );
}
