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

#include "git_lifecycle.h"
#include "git_backend.h"

#include <git/git_backend.h>
#include <git/kicad_git_common.h>
#include <git/kigit_lock_retry.h>
#include <git/kigit_transport.h>
#include <mdw_runtime/mdw_strings.h>

#include <ctime>
#include <stdexcept>

namespace mdw = kiapi::mdw;
namespace ws = kiapi::workspace;


namespace
{

const char DESIGN_KIND[] = "design-rel";


/// Accepts a design release manifest only when it names the tag's own number
bool designRelease( const nlohmann::json& aManifest, int aNumber )
{
    auto comment = aManifest.find( "comment" );

    // Typed here, since anyone who can push a tag writes these and a wrong type would throw on every read
    return aManifest.value( "design_revision", nlohmann::json() ) == aNumber
           && aManifest.value( "operation", nlohmann::json() ).is_string()
           && ( comment == aManifest.end() || comment->is_string() );
}


/// What ties a design release to the request that made it, so a retry finds it rather than releasing again
std::string operationOf( const mdw::CreateBaselineRequest& aRequest )
{
    return aRequest.idempotency_id() + "-" + MdwHashHex( aRequest.SerializeAsString() );
}


nlohmann::json partsOf( const mdw::CreateBaselineRequest& aRequest, const std::vector<mdw::PlacedPartCheck>& aChecks )
{
    nlohmann::json parts = nlohmann::json::array();

    for( const mdw::PlacedPartCheck& check : aChecks )
    {
        const mdw::PlacedPart& part = aRequest.parts( static_cast<int>( check.part_index() ) );
        nlohmann::json         statuses = nlohmann::json::array();

        for( int status : check.facts().statuses() )
            statuses.push_back( mdw::PlacedPartStatus_Name( static_cast<mdw::PlacedPartStatus>( status ) ) );

        parts.push_back(
                { { "references", std::vector<std::string>( part.references().begin(), part.references().end() ) },
                  { "item_id", part.item().item_id() },
                  { "source", part.item().source() },
                  { "content_digest", part.content_digest() },
                  { "revision", check.facts().matched_revision_label() },
                  { "statuses", statuses },
                  { "severity", mdw::Severity_Name( check.severity() ) } } );
    }

    return parts;
}


nlohmann::json exceptionsOf( const mdw::CreateBaselineRequest& aRequest )
{
    nlohmann::json exceptions = nlohmann::json::array();

    for( const mdw::BaselineException& exception : aRequest.exceptions() )
    {
        nlohmann::json entry = { { "part_index", exception.part_index() }, { "note", exception.note() } };

        if( !exception.eco().eco_id().empty() )
            entry["eco"] = exception.eco().eco_id();

        exceptions.push_back( std::move( entry ) );
    }

    return exceptions;
}


ws::Revision baselineRevision( const MDW_GIT_RELEASE& aRelease )
{
    ws::Revision revision;
    revision.set_revision_id( aRelease.tagOid );
    revision.set_label( MdwGitBaselineLabel( aRelease.number ) );
    revision.set_ordinal( aRelease.number );
    revision.set_kind( ws::RK_BASELINE );
    revision.add_parent_revision_ids( aRelease.commit );
    revision.set_author( MdwValidUtf8( aRelease.releasedBy ) );
    revision.set_timestamp_unix( aRelease.date );
    revision.set_comment( MdwValidUtf8( aRelease.manifest.value( "comment", std::string() ) ) );
    *revision.mutable_state() = MdwGitReleasedState();
    return revision;
}

} // namespace


MDW_GIT_LIFECYCLE::MDW_GIT_LIFECYCLE( MDW_GIT_BACKEND& aBackend ) :
        m_backend( aBackend )
{
}


std::vector<ws::CustomState> MDW_GIT_LIFECYCLE::States()
{
    return MdwGitStates();
}


bool MDW_GIT_LIFECYCLE::librarySource() const
{
    return !m_backend.m_server.HelloContext().workspace().locator().empty();
}


ws::WorkspaceError MDW_GIT_LIFECYCLE::Definition( const mdw::GetLifecycleDefinitionRequest&,
                                                  mdw::GetLifecycleDefinitionReply& aReply )
{
    ws::LifecycleDefinition* definition = aReply.mutable_definition();
    definition->set_definition_id( "git-tags" );
    definition->set_display_name( "Git releases" );

    for( const ws::CustomState& state : States() )
        *definition->add_states() = state;

    return MdwOk();
}


ws::WorkspaceError MDW_GIT_LIFECYCLE::Transitions( const mdw::GetAvailableTransitionsRequest& aRequest,
                                                   mdw::GetAvailableTransitionsReply&         aReply )
{
    if( !librarySource() )
        return MdwOk();

    if( ws::WorkspaceError error = m_backend.m_libraries.MemberTransitions( aRequest, aReply );
        error.code() != ws::WE_OK )
    {
        return error;
    }

    // A state tag is pushed like a release, so the server that refuses one refuses the other
    for( ws::LifecycleTransition& transition : *aReply.mutable_transitions() )
        *transition.mutable_permission() = m_backend.writePermission( "transition" );

    return MdwOk();
}


ws::WorkspaceError MDW_GIT_LIFECYCLE::Transition( const mdw::TransitionLifecycleRequest& aRequest,
                                                  mdw::TransitionLifecycleReply&         aReply )
{
    if( !librarySource() )
        return MdwFail( ws::WE_INVALID_STATE_TRANSITION, "A design release is a tag and never changes state" );

    return m_backend.m_libraries.TransitionMember( aRequest, aReply );
}


ws::WorkspaceError MDW_GIT_LIFECYCLE::Release( const mdw::ReleaseRevisionRequest& aRequest, const std::string& aToken,
                                               mdw::ReleaseRevisionReply& aReply )
{
    if( !librarySource() )
        return MdwFail( ws::WE_NOT_SUPPORTED, "Only a library source releases parts" );

    return releasePart( aRequest, aToken, aReply );
}


ws::WorkspaceError MDW_GIT_LIFECYCLE::ReleaseToken( const mdw::ReleaseRevisionRequest& aRequest, std::string& aToken )
{
    if( librarySource() && m_backend.ensureOpen() && m_backend.m_claims )
        aToken = m_backend.m_claims->LocalClaimId( aRequest.item_id() );

    return MdwOk();
}


ws::WorkspaceError MDW_GIT_LIFECYCLE::releasePart( const mdw::ReleaseRevisionRequest& aRequest,
                                                   const std::string& aClaim, mdw::ReleaseRevisionReply& aReply )
{
    using RESULT = MDW_GIT_CHECKOUTS::RESULT;

    ws::WorkspaceError error = m_backend.m_libraries.ReleaseMember( *this, aRequest, aReply );

    if( error.code() != ws::WE_OK || aClaim.empty() || !m_backend.ensureOpen() || !m_backend.m_claims )
        return error;

    // The claim that revised the part has done its job, and one someone took since is theirs to keep
    if( RESULT result = m_backend.m_claims->GiveClaim( aRequest.item_id(), aClaim );
        result != RESULT::OK && result != RESULT::NOT_HOLDER )
    {
        // The release is out, so recovery must replay it rather than a retry starting over
        throw std::runtime_error( "the part was released but its claim could not be given up" );
    }

    return error;
}


ws::WorkspaceError MDW_GIT_LIFECYCLE::Baseline( const mdw::CreateBaselineRequest& aRequest,
                                                mdw::CreateBaselineReply&         aReply )
{
    if( librarySource() )
        return MdwFail( ws::WE_NOT_SUPPORTED, "A library source holds no design to release" );

    if( !m_backend.ensureOpen() )
        return MdwFail( ws::WE_ITEM_NOT_FOUND, "The workspace has no git working tree yet" );

    MDW_GIT_REPO& repo = m_backend.m_repo;

    // The tagged commit is found from HEAD, so no branch switch or commit may land while the checks run
    std::unique_ptr<KIGIT::REPO_ACTION_LOCK> actions = KIGIT::RepoActionLock( repo.Handle() );

    if( ws::WorkspaceError error = MdwGitLockActions( actions.get() ); error.code() != ws::WE_OK )
        return error;

    if( !repo.HasRemote() )
        return MdwFail( ws::WE_NOT_SUPPORTED, "The project has no remote to share a release through" );

    const std::string remote = repo.RemoteName();
    const std::string operation = operationOf( aRequest );
    MDW_GIT_RELEASES  releases( repo.Handle(), m_backend.m_remote, remote, DESIGN_KIND );

    // Fetched before numbering, so the label the plan checks is the team's next
    if( ws::WorkspaceError error = releases.Fetch(); error.code() != ws::WE_OK )
        return error;

    std::optional<MDW_GIT_RELEASE> release;

    for( const MDW_GIT_RELEASE& known : releases.List( std::string(), designRelease ) )
    {
        if( !known.tampered && known.manifest.value( "operation", std::string() ) == operation )
            release = known;
    }

    MDW_BASELINE_PLAN plan;
    bool              planned = false;

    // Only a new tag needs the plan, which an earlier attempt's tag already passed
    auto prepare = [&]() -> ws::WorkspaceError
    {
        if( ws::WorkspaceError error = MdwPlanBaseline( *this, aRequest, plan ); error.code() != ws::WE_OK )
            return error;

        KIGIT_COMMON      common( repo.Handle() );
        const std::string branch = common.GetCurrentBranchName().utf8_string();
        const std::string upstreamRef = "refs/remotes/" + remote + "/" + branch;

        std::optional<std::string> upstream = GetGitBackend()->ReadReference( repo.Handle(), upstreamRef );
        const std::string&         tagged = plan.source.revision_id;

        planned = true;

        // The release must name a commit the team can reach from the branch
        if( upstream
            && ( *upstream == tagged
                 || GetGitBackend()->IsDescendant( repo.Handle(), *upstream, tagged ).value_or( false ) ) )
        {
            return MdwOk();
        }

        ws::WorkspaceError pushError;

        if( m_backend.m_remote.PushBranch( repo.Handle(), remote, branch, upstream, pushError )
            != RefPushResult::Success )
        {
            if( pushError.code() == ws::WE_VERSION_CONFLICT )
                return MdwFail( ws::WE_VERSION_CONFLICT, "The project has newer changes; get them first" );

            return pushError;
        }

        GetGitBackend()->SetReference( repo.Handle(), upstreamRef, repo.HeadOid(), std::nullopt, "release design" );
        return MdwOk();
    };

    const GIT_IDENTITY author = m_backend.identity( repo );
    const std::string  user = author.SignedName();
    const std::string  email = author.SignedEmail();

    ws::WorkspaceError published = releases.PublishNext(
            release,
            [&]( MDW_GIT_RELEASE& aOut ) -> ws::WorkspaceError
            {
                if( !planned )
                {
                    if( ws::WorkspaceError error = prepare(); error.code() != ws::WE_OK )
                        return error;
                }

                const int next = releases.Next( std::string() );

                if( std::optional<ws::WorkspaceError> refused =
                            MdwRequireNextLabel( *this, "baseline", aRequest.requested_label(), std::to_string( next ) ) )
                {
                    return *refused;
                }

                const nlohmann::json manifest = { { "design_revision", next },
                                                  { "item_id", aRequest.item_id() },
                                                  { "revision_id", plan.source.revision_id },
                                                  { "released_by", user },
                                                  { "date", static_cast<std::int64_t>( std::time( nullptr ) ) },
                                                  { "comment", aRequest.comment() },
                                                  { "operation", operation },
                                                  { "parts", partsOf( aRequest, plan.checks ) },
                                                  { "exceptions", exceptionsOf( aRequest ) } };

                const std::string file = MdwGitBaseName( aRequest.item_id() );
                const std::string subject =
                        "Baseline " + std::to_string( next ) + " of " + file.substr( 0, file.rfind( '.' ) );

                return releases.Create( std::string(), next, plan.source.revision_id, subject,
                                        MdwBaselineDescription( aRequest ), manifest, user, email, aOut );
            } );

    if( published.code() != ws::WE_OK )
        return published;

    aReply.set_outcome( ws::CO_APPLIED );
    *aReply.mutable_baseline() = baselineRevision( *release );
    return MdwOk();
}


MDW_RECOVERY MDW_GIT_LIFECYCLE::RecoverTransition( const mdw::TransitionLifecycleRequest& aRequest,
                                                   mdw::TransitionLifecycleReply& aReply, ws::WorkspaceError& aError )
{
    aError = Transition( aRequest, aReply );
    return aError.code() == ws::WE_OK ? MDW_RECOVERY::REPLAY : MDW_RECOVERY::FAIL;
}


MDW_RECOVERY MDW_GIT_LIFECYCLE::RecoverRelease( const mdw::ReleaseRevisionRequest& aRequest, const std::string& aToken,
                                                mdw::ReleaseRevisionReply& aReply, ws::WorkspaceError& aError )
{
    if( !librarySource() )
    {
        aError = MdwFail( ws::WE_NOT_SUPPORTED, "Only a library source releases parts" );
        return MDW_RECOVERY::FAIL;
    }

    // The claim the first attempt began with, whatever this repository records now
    aError = releasePart( aRequest, aToken, aReply );
    return aError.code() == ws::WE_OK ? MDW_RECOVERY::REPLAY : MDW_RECOVERY::FAIL;
}


MDW_RECOVERY MDW_GIT_LIFECYCLE::RecoverBaseline( const mdw::CreateBaselineRequest& aRequest,
                                                 mdw::CreateBaselineReply& aReply, ws::WorkspaceError& aError )
{
    aError = Baseline( aRequest, aReply );
    return aError.code() == ws::WE_OK ? MDW_RECOVERY::REPLAY : MDW_RECOVERY::FAIL;
}


bool MDW_GIT_LIFECYCLE::Owns( const ws::ItemRef& aRef ) const
{
    if( !librarySource() || !MDW_GIT_SHARED_LIBRARIES::IsMemberId( aRef.item_id() ) )
        return false;

    const std::string locator = m_backend.m_server.HelloContext().workspace().locator();

    return aRef.source().empty()
           || MDW_GIT_SHARED_LIBRARIES::NormalisedLocator( aRef.source() )
                      == MDW_GIT_SHARED_LIBRARIES::NormalisedLocator( locator );
}


ws::WorkspaceError MDW_GIT_LIFECYCLE::Releases( const std::set<std::string>&                            aItemIds,
                                                std::map<std::string, std::vector<MDW_RELEASE_RECORD>>& aOut )
{
    if( aItemIds.empty() )
        return MdwOk();

    std::map<std::string, std::vector<MDW_GIT_RELEASE>> found;

    if( ws::WorkspaceError error = m_backend.m_libraries.MemberReleases( aItemIds, found ); error.code() != ws::WE_OK )
        return error;

    for( const auto& [id, releases] : found )
    {
        std::vector<MDW_RELEASE_RECORD>& records = aOut[id];

        for( const MDW_GIT_RELEASE& release : releases )
        {
            ws::Revision revision = MdwGitPartRevision( release );

            // A tag the remote holds differently is no release anyone can rely on, so nothing matches it
            records.push_back( { revision.revision_id(), revision.label(),
                                 release.tampered ? std::string() : revision.content_digest(), revision.state(),
                                 release.published } );
        }
    }

    return MdwOk();
}


ws::WorkspaceError MDW_GIT_LIFECYCLE::BaselineSource( const std::string& aItemId, const std::string& aRevisionId,
                                                      bool aCreating, MDW_BASELINE_SOURCE& aOut )
{
    if( librarySource() )
        return MdwFail( ws::WE_NOT_SUPPORTED, "A library source holds no design to release" );

    if( !m_backend.ensureOpen() )
        return MdwFail( ws::WE_ITEM_NOT_FOUND, "The workspace has no git working tree yet" );

    MDW_GIT_REPO& repo = m_backend.m_repo;

    // A check may name no item, for a design not added to git yet
    if( !repo.IsTracked( aItemId ) && ( aCreating || !aItemId.empty() ) )
        return MdwFail( ws::WE_ITEM_NOT_FOUND, "No such tracked file" );

    std::optional<MDW_GIT_VERSION> newest = repo.ScopeRevision( m_backend.m_prefix, MdwCurrentRequestCancelled );

    if( MdwCurrentRequestCancelled() )
        return MdwCancelled();

    if( aCreating && !newest )
        return MdwFail( ws::WE_REVISION_NOT_FOUND, aItemId + " has no commit yet" );

    // A design no commit holds waits only for its first, which an unborn head still puts on a branch
    if( !newest || aItemId.empty() )
    {
        aOut.hold = KIGIT_COMMON( repo.Handle() ).IsHeadDetached() ? mdw::HOLD_DETACHED : mdw::HOLD_UNCOMMITTED;
        return MdwOk();
    }

    if( !aRevisionId.empty() && aRevisionId != newest->oid )
    {
        return MdwFail( ws::WE_VERSION_CONFLICT,
                        "A newer revision of " + aItemId + " exists; baseline that one instead" );
    }

    aOut.revision_id = newest->oid;
    aOut.next_label = "1";

    if( repo.HasRemote() )
    {
        MDW_GIT_RELEASES releases( repo.Handle(), m_backend.m_remote, repo.RemoteName(), DESIGN_KIND );
        aOut.next_label = std::to_string( releases.Next( std::string() ) );
    }

    KIGIT_COMMON common( repo.Handle() );

    // Checked in the order the user must clear them, since a commit on no branch could not be shared
    if( common.IsHeadDetached() || repo.HeadOid().empty() )
    {
        aOut.hold = mdw::HOLD_DETACHED;
    }
    else
    {
        // Anything not committed would be missing from the release
        std::optional<std::vector<MDW_GIT_CHANGE>> changes = repo.LocalChanges( m_backend.m_prefix );

        if( !changes )
            return MdwFail( ws::WE_INTERNAL, "Cannot read the git status: " + MdwGitError() );

        if( !changes->empty() )
            aOut.hold = mdw::HOLD_UNCOMMITTED;
        else if( !repo.HasRemote() )
            aOut.hold = mdw::HOLD_NO_REMOTE;
    }

    if( !aCreating )
        return MdwOk();

    switch( aOut.hold )
    {
    case mdw::HOLD_DETACHED:
        aOut.held = MdwFail( ws::WE_INVALID_STATE_TRANSITION, "The project is not on a branch" );
        break;

    case mdw::HOLD_UNCOMMITTED:
        aOut.held = MdwFail( ws::WE_VERSION_CONFLICT, "The project has changes to commit before releasing it" );
        break;

    case mdw::HOLD_NO_REMOTE:
        aOut.held = MdwFail( ws::WE_NOT_SUPPORTED, "The project has no remote to share a release through" );
        break;

    default: break;
    }

    return MdwOk();
}


std::map<std::string, int> MDW_GIT_LIFECYCLE::Baselines( const std::string& aItemId ) const
{
    MDW_GIT_REPO&              repo = m_backend.m_repo;
    MDW_GIT_RELEASES           releases( repo.Handle(), m_backend.m_remote, repo.RemoteName(), DESIGN_KIND );
    std::map<std::string, int> baselines;

    // Oldest first, so a commit released twice keeps its newest number
    for( const MDW_GIT_RELEASE& release : releases.List( std::string(), designRelease ) )
    {
        if( release.published && release.manifest.value( "item_id", nlohmann::json() ) == aItemId )
            baselines[release.commit] = release.number;
    }

    return baselines;
}


std::string MDW_GIT_LIFECYCLE::MirrorRefspec() const
{
    MDW_GIT_REPO& repo = m_backend.m_repo;

    return MDW_GIT_RELEASES( repo.Handle(), m_backend.m_remote, repo.RemoteName(), DESIGN_KIND ).MirrorRefspec();
}
