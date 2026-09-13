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

#include "mdw_git_releases.h"
#include "mdw_git_remote.h"
#include "mdw_git_repo.h"

#include <git/git_backend.h>
#include <git/kigit_transport.h>
#include <mdw_runtime/mdw_backend_server.h>

#include <git2.h>

#include <algorithm>
#include <cstdio>
#include <optional>

namespace ws = kiapi::workspace;


namespace
{

/// The release number aName ends in below aPrefix, or 0 when the rest is not a plain number
int numberOf( const std::string& aName, const std::string& aPrefix )
{
    if( !aName.starts_with( aPrefix ) )
        return 0;

    const std::string rest = aName.substr( aPrefix.size() );

    if( rest.empty() || rest.size() > 9 || rest[0] == '0'
        || !std::all_of( rest.begin(), rest.end(),
                         []( char c )
                         {
                             return c >= '0' && c <= '9';
                         } ) )
    {
        return 0;
    }

    return std::stoi( rest );
}

} // namespace


ws::LifecycleState MdwGitReleasedState()
{
    return *MdwGitState( "released" );
}


std::vector<ws::CustomState> MdwGitStates()
{
    struct STATE
    {
        const char*           id;
        const char*           name;
        const char*           description;
        ws::LifecycleCategory category;
        bool                  notForNewDesigns = false;
    };

    // Not for New Designs stays placeable, where Obsolete is withdrawn
    static const STATE states[] = {
        { "released", "Released", "Tagged as a numbered release the team shares", ws::LC_RELEASED },
        { "not_for_new_designs", "Not for New Designs", "Released, but not for use in new designs", ws::LC_RELEASED,
          true },
        { "obsolete", "Obsolete", "Withdrawn from use", ws::LC_RESTRICTED }
    };

    std::vector<ws::CustomState> custom;

    for( const STATE& entry : states )
    {
        ws::CustomState& state = custom.emplace_back();
        state.set_state_id( entry.id );
        state.set_display_name( entry.name );
        state.set_description( entry.description );
        state.set_category( entry.category );
        state.set_not_for_new_designs( entry.notForNewDesigns );
    }

    return custom;
}


std::optional<ws::LifecycleState> MdwGitState( const std::string& aStateId )
{
    for( const ws::CustomState& custom : MdwGitStates() )
    {
        if( custom.state_id() != aStateId )
            continue;

        ws::LifecycleState state;
        state.set_state_id( aStateId );
        state.set_category( custom.category() );
        state.set_not_for_new_designs( custom.not_for_new_designs() );
        return state;
    }

    return std::nullopt;
}


std::string MdwGitBaselineLabel( int aNumber )
{
    return "Baseline " + std::to_string( aNumber );
}


ws::Revision MdwGitPartRevision( const MDW_GIT_RELEASE& aRelease )
{
    auto text = [&]( const char* aKey )
    {
        auto it = aRelease.manifest.find( aKey );
        return it != aRelease.manifest.end() && it->is_string() ? it->get<std::string>() : std::string();
    };

    ws::Revision revision;
    revision.set_revision_id( aRelease.commit );
    revision.set_label( std::to_string( aRelease.number ) );
    revision.set_ordinal( aRelease.number );
    revision.set_kind( ws::RK_RELEASE );
    revision.set_author( MdwValidUtf8( aRelease.releasedBy ) );
    revision.set_timestamp_unix( aRelease.date );
    revision.set_comment( MdwValidUtf8( text( "comment" ) ) );
    revision.set_content_digest( text( "content_digest" ) );
    revision.set_member_digest( text( "member_digest" ) );
    *revision.mutable_state() = MdwGitState( aRelease.stateId ).value_or( MdwGitReleasedState() );
    return revision;
}


MDW_GIT_RELEASES::MDW_GIT_RELEASES( git_repository* aRepo, MDW_GIT_REMOTE& aRemote, const std::string& aRemoteName,
                                    const std::string& aKind ) :
        m_repo( aRepo ),
        m_remote( aRemote ),
        m_remoteName( aRemoteName ),
        m_kind( aKind )
{
}


std::vector<MDW_GIT_RELEASE> MDW_GIT_RELEASES::List( const std::string& aScope, const VALID& aValid ) const
{
    std::vector<MDW_GIT_RELEASE> releases;
    const std::string            prefix = tagRoot() + aScope;

    for( const KIGIT_REFERENCE& ref : GetGitBackend()->ListReferences( m_repo, prefix + "*" ) )
    {
        const int number = numberOf( ref.name, prefix );

        if( number == 0 )
            continue;

        std::optional<KIGIT_TAG_RECORD> tag = GetGitBackend()->ReadAnnotatedTag( m_repo, ref.name );

        if( !tag )
            continue;

        nlohmann::json manifest = MdwGitRecordJson( tag->message );

        // Anyone who can push a tag writes these, so a manifest naming another commit is no release at all
        if( !manifest.is_object() || !manifest.contains( "revision_id" ) || manifest["revision_id"] != tag->target
            || !aValid( manifest, number ) )
        {
            std::fprintf( stderr, "kicad-mdw-git: %s is not a release, skipped\n", ref.name.c_str() );
            continue;
        }

        MDW_GIT_RELEASE& release = releases.emplace_back();
        release.ref = ref.name;
        release.number = number;
        release.tagOid = tag->oid;
        release.commit = tag->target;
        release.manifest = std::move( manifest );
        release.releasedBy = tag->taggerName;
        release.date = tag->time;

        std::optional<std::string> mirrored =
                GetGitBackend()->ReadReference( m_repo, trackingRoot() + ref.name.substr( tagRoot().size() ) );

        release.published = mirrored && *mirrored == tag->oid;
        release.tampered = mirrored && *mirrored != tag->oid;
    }

    std::sort( releases.begin(), releases.end(),
               []( const MDW_GIT_RELEASE& aLeft, const MDW_GIT_RELEASE& aRight )
               {
                   return aLeft.number < aRight.number;
               } );

    return releases;
}


int MDW_GIT_RELEASES::Next( const std::string& aScope ) const
{
    const std::string prefix = tagRoot() + aScope;
    int               highest = 0;

    for( const KIGIT_REFERENCE& ref : GetGitBackend()->ListReferences( m_repo, prefix + "*" ) )
        highest = std::max( highest, numberOf( ref.name, prefix ) );

    return highest + 1;
}


std::string MDW_GIT_RELEASES::MirrorRefspec() const
{
    return "+" + tagRoot() + "*:" + trackingRoot() + "*";
}


ws::WorkspaceError MDW_GIT_RELEASES::Fetch()
{
    return Fetch( { this } );
}


ws::WorkspaceError MDW_GIT_RELEASES::Fetch( const std::vector<MDW_GIT_RELEASES*>& aKinds )
{
    if( aKinds.empty() )
        return MdwOk();

    std::vector<wxString> refspecs;

    for( const MDW_GIT_RELEASES* kind : aKinds )
        refspecs.push_back( wxString::FromUTF8( kind->MirrorRefspec() ) );

    const MDW_GIT_RELEASES& first = *aKinds.front();

    ws::WorkspaceError error = first.m_remote.Transfer( first.m_repo, first.m_remoteName,
                                                        [&]( KIGIT::REMOTE_TRANSPORT& aTransport )
                                                        {
                                                            return aTransport.FetchRefspecs( refspecs, true );
                                                        } );

    if( error.code() == ws::WE_OK )
    {
        for( MDW_GIT_RELEASES* kind : aKinds )
            kind->Adopt();
    }

    return error;
}


void MDW_GIT_RELEASES::Adopt()
{
    for( const KIGIT_REFERENCE& mirrored : GetGitBackend()->ListReferences( m_repo, trackingRoot() + "*" ) )
    {
        // Created only where no tag exists, so an existing local tag stays the pin whatever the remote says
        GetGitBackend()->SetReference( m_repo, tagRoot() + mirrored.name.substr( trackingRoot().size() ), mirrored.oid,
                                       std::string(), "adopt release" );
    }
}


ws::WorkspaceError MDW_GIT_RELEASES::Create( const std::string& aScope, int aNumber, const std::string& aCommit,
                                             const std::string& aSubject, const std::string& aBody,
                                             const nlohmann::json& aManifest, const std::string& aTaggerName,
                                             const std::string& aTaggerEmail, MDW_GIT_RELEASE& aOut )
{
    const std::string name = "kicad/" + m_kind + "/" + aScope + std::to_string( aNumber );
    const std::string message = MdwGitRecordMessage( aSubject, aBody, aManifest );
    std::string       tagOid;
    const int         rc =
            GetGitBackend()->CreateAnnotatedTag( m_repo, name, aCommit, message, aTaggerName, aTaggerEmail, tagOid );

    if( rc == GIT_EEXISTS )
        return MdwFail( ws::WE_VERSION_CONFLICT, "Release " + std::to_string( aNumber ) + " is already taken" );

    if( rc != 0 )
        return MdwFail( ws::WE_INTERNAL, "Cannot tag the release: " + MdwGitError() );

    std::optional<KIGIT_TAG_RECORD> tag = GetGitBackend()->ReadAnnotatedTag( m_repo, "refs/tags/" + name );

    aOut = MDW_GIT_RELEASE();
    aOut.ref = "refs/tags/" + name;
    aOut.number = aNumber;
    aOut.tagOid = tagOid;
    aOut.commit = aCommit;
    aOut.manifest = aManifest;
    aOut.releasedBy = aTaggerName;
    aOut.date = tag ? tag->time : 0;
    return MdwOk();
}


ws::WorkspaceError MDW_GIT_RELEASES::Publish( MDW_GIT_RELEASE& aRelease, bool& aTaken )
{
    GIT_REF_UPDATE update;

    update.localRef = wxString::FromUTF8( aRelease.ref );
    update.remoteRef = update.localRef;
    update.leased = true;

    std::vector<GIT_REF_RESULT> results;
    ws::WorkspaceError          error;
    const RefPushResult         pushed = m_remote.Push( m_repo, m_remoteName, { update }, results, error );

    aTaken = pushed == RefPushResult::Rejected || pushed == RefPushResult::LeaseBroken;

    if( pushed != RefPushResult::Success )
        return error;

    // What the remote confirmed, so List reads the release as published without another fetch
    GetGitBackend()->SetReference( m_repo, trackingRoot() + aRelease.ref.substr( tagRoot().size() ), aRelease.tagOid,
                                   std::nullopt, "release published" );

    aRelease.published = true;
    return MdwOk();
}


ws::WorkspaceError MDW_GIT_RELEASES::PublishNext( std::optional<MDW_GIT_RELEASE>& aRelease, const CREATE& aCreate )
{
    for( int attempt = 0; !aRelease || !aRelease->published; ++attempt )
    {
        if( !aRelease )
        {
            aRelease.emplace();

            if( ws::WorkspaceError error = aCreate( *aRelease ); error.code() != ws::WE_OK )
                return error;
        }

        bool               taken = false;
        ws::WorkspaceError error = Publish( *aRelease, taken );

        if( error.code() != ws::WE_OK && !taken )
            return error;

        if( error.code() != ws::WE_OK )
        {
            Drop( *aRelease );
            aRelease.reset();

            if( attempt > 0 )
                return error;

            if( error = Fetch(); error.code() != ws::WE_OK )
                return error;
        }
    }

    return MdwOk();
}


void MDW_GIT_RELEASES::Drop( const MDW_GIT_RELEASE& aRelease )
{
    GetGitBackend()->DeleteReference( m_repo, aRelease.ref, aRelease.tagOid );
}
