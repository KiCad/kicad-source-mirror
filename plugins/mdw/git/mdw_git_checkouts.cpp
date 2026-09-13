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


#include "mdw_git_checkouts.h"
#include "mdw_git_json.h"
#include "mdw_git_repo.h"

#include <git/git_backend.h>
#include <mdw_runtime/mdw_backend_server.h>

#include <git2.h>
#include <wx/log.h>
#include <wx/utils.h>

#include <cctype>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <map>
#include <set>


namespace
{
const char CLAIMS[] = "refs/kicad/checkouts/";
const char RELEASED[] = "refs/kicad/released/";
const std::string TRACKING = "refs/kicad-remote/";

// Under the 255 byte file name limit, with room for the ".lock" git writes beside a ref
const size_t MAX_COMPONENT = 200;


std::string decodeComponent( const std::string& aComponent )
{
    std::string out;

    for( size_t ii = 0; ii < aComponent.size(); ++ii )
    {
        if( aComponent[ii] == '%' && ii + 2 < aComponent.size() && std::isxdigit( (unsigned char) aComponent[ii + 1] )
            && std::isxdigit( (unsigned char) aComponent[ii + 2] ) )
        {
            out += static_cast<char>( std::strtol( aComponent.substr( ii + 1, 2 ).c_str(), nullptr, 16 ) );
            ii += 2;
        }
        else
        {
            out += aComponent[ii];
        }
    }

    return out;
}
}


MDW_GIT_CHECKOUTS::MDW_GIT_CHECKOUTS( git_repository* aRepo, MDW_GIT_REMOTE& aRemote, const std::string& aRemoteName ) :
        m_repo( aRepo ),
        m_remote( aRemote ),
        m_remoteName( aRemoteName )
{
}


std::string MDW_GIT_CHECKOUTS::RefComponent( const std::string& aItemId )
{
    static const char HEX[] = "0123456789ABCDEF";

    std::string out;

    // Percent-encoding everything outside a safe set satisfies git check-ref-format for any path,
    // including components that start with '.' or end in ".lock"
    for( unsigned char c : aItemId )
    {
        bool safe = ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) || ( c >= '0' && c <= '9' ) || c == '-'
                    || c == '_';

        if( safe )
        {
            out += static_cast<char>( c );
        }
        else
        {
            out += '%';
            out += HEX[c >> 4];
            out += HEX[c & 0xf];
        }
    }

    // One ref component is one file name, so a deep path is keyed by its hash. The encoding above
    // never puts 'H' after '%', so a hashed key cannot equal an encoded one
    if( out.size() > MAX_COMPONENT )
        out = "%H" + GetGitBackend()->BlobId( aItemId );

    return out;
}


std::string MDW_GIT_CHECKOUTS::localRef( const std::string& aItemId ) const
{
    return CLAIMS + RefComponent( aItemId );
}


std::string MDW_GIT_CHECKOUTS::trackingRef( const std::string& aItemId ) const
{
    return "refs/kicad-remote/" + m_remoteName + "/checkouts/" + RefComponent( aItemId );
}


std::string MDW_GIT_CHECKOUTS::releasedRef( const std::string& aItemId ) const
{
    return "refs/kicad-remote/" + m_remoteName + "/released/" + RefComponent( aItemId );
}


std::optional<MDW_GIT_CLAIM> MDW_GIT_CHECKOUTS::read( const std::string& aRef )
{
    std::optional<std::string> oid = GetGitBackend()->ReadReference( m_repo, aRef );

    if( !oid )
        return std::nullopt;

    MDW_GIT_CLAIM claim;
    claim.oid = *oid;

    std::optional<KIGIT_COMMIT_RECORD> commit = GetGitBackend()->ReadCommit( m_repo, *oid );

    // A ref someone pushed by hand still counts as a claim, just without a readable holder
    if( !commit )
        return claim;

    nlohmann::json record = MdwGitRecordJson( commit->message );

    // Anyone who can push writes these, so a field of the wrong type or bad UTF-8 reads as absent
    auto text = [&record]( const char* aKey )
    {
        auto it = record.find( aKey );
        return it != record.end() && it->is_string() ? MdwValidUtf8( it->get<std::string>() ) : std::string();
    };

    if( record.is_object() )
    {
        claim.owner = text( "owner" );
        claim.user = text( "user" );
        claim.email = text( "email" );
        claim.item = text( "item_id" );

        if( auto since = record.find( "since" ); since != record.end() && since->is_number_integer() )
            claim.since = since->get<std::int64_t>();
    }

    if( claim.user.empty() )
        claim.user = MdwValidUtf8( commit->authorName );

    return claim;
}


bool MDW_GIT_CHECKOUTS::setRef( const std::string& aRef, const std::string& aOid )
{
    ++m_moves;

    if( GetGitBackend()->SetReference( m_repo, aRef, aOid, std::nullopt, "kicad: claim" ) != 0 )
    {
        m_error = MdwGitError();
        return false;
    }

    return true;
}


void MDW_GIT_CHECKOUTS::dropRef( const std::string& aRef )
{
    ++m_moves;
    GetGitBackend()->DeleteReference( m_repo, aRef, std::nullopt );
}


bool MDW_GIT_CHECKOUTS::dropRefIf( const std::string& aRef, const std::string& aOid )
{
    ++m_moves;
    return GetGitBackend()->DeleteReference( m_repo, aRef, aOid ) == 0;
}


std::optional<kiapi::workspace::WorkspaceError> MDW_GIT_CHECKOUTS::CredentialFailure() const
{
    switch( m_last.code() )
    {
    case kiapi::workspace::WE_NOT_AUTHENTICATED:
    case kiapi::workspace::WE_AUTH_FAILED:
    case kiapi::workspace::WE_UNTRUSTED_HOST:
    case kiapi::workspace::WE_PERMISSION_DENIED: return m_last;
    default: return std::nullopt;
    }
}


RefPushResult MDW_GIT_CHECKOUTS::push( const std::vector<GIT_REF_UPDATE>& aUpdates )
{
    std::vector<GIT_REF_RESULT> results;

    return m_remote.Push( m_repo, m_remoteName, aUpdates, results, m_last );
}


std::vector<std::string> MDW_GIT_CHECKOUTS::refspecs( const std::vector<std::string>& aMirrors ) const
{
    const std::string        tracking = TRACKING + m_remoteName;
    std::vector<std::string> specs = { "+" + std::string( CLAIMS ) + "*:" + tracking + "/checkouts/*",
                                       "+" + std::string( RELEASED ) + "*:" + tracking + "/released/*" };

    specs.insert( specs.end(), aMirrors.begin(), aMirrors.end() );
    return specs;
}


kiapi::workspace::WorkspaceError MDW_GIT_CHECKOUTS::transfer( git_repository*                 aRepo,
                                                              const std::vector<std::string>& aRefspecs )
{
    std::vector<wxString> refspecs;
    bool                  locked = false;

    for( const std::string& refspec : aRefspecs )
        refspecs.push_back( wxString::FromUTF8( refspec ) );

    // Nothing reads a claims fetch's FETCH_HEAD, and a poll would rewrite it every pass
    kiapi::workspace::WorkspaceError fetched =
            m_remote.Transfer( aRepo, m_remoteName,
                               [&]( KIGIT::REMOTE_TRANSPORT& aTransport )
                               {
                                   int rc = aTransport.FetchRefspecs( refspecs, true, false );
                                   locked = rc == GIT_ELOCKED;
                                   return rc;
                               } );

    // Another git operation writing the same refs says nothing of the remote
    if( locked )
        return MdwFail( kiapi::workspace::WE_BUSY, "Another git operation holds the claim refs" );

    return fetched;
}


MDW_GIT_CHECKOUTS::RESULT MDW_GIT_CHECKOUTS::Refresh( const std::vector<std::string>& aMirrors )
{
    std::map<std::string, MDW_GIT_CLAIM> before = snapshot();
    kiapi::workspace::WorkspaceError     fetched;

    ++m_moves;

    // The poll's own fetch may hold packed-refs for a moment, so a busy repository is waited out
    KIGIT::WithLockRetry( KIGIT::BACKEND_LOCK_BUDGET,
                          [&]()
                          {
                              fetched = transfer( m_repo, refspecs( aMirrors ) );
                              return fetched.code() == kiapi::workspace::WE_BUSY ? GIT_ELOCKED : GIT_OK;
                          } );

    return adopt( fetched, before );
}


std::map<std::string, MDW_GIT_CLAIM> MDW_GIT_CHECKOUTS::snapshot()
{
    std::map<std::string, MDW_GIT_CLAIM> claims;

    if( m_changeObserver )
    {
        for( MDW_GIT_CLAIM& claim : All() )
            claims.emplace( claim.item, std::move( claim ) );
    }

    return claims;
}


MDW_GIT_CHECKOUTS::STAGE MDW_GIT_CHECKOUTS::Stage( const std::vector<std::string>& aMirrors )
{
    STAGE stage;

    stage.moves = m_moves;

    for( const std::string& refspec : refspecs( aMirrors ) )
    {
        const std::size_t start = refspec.starts_with( '+' ) ? 1 : 0;
        const std::size_t colon = refspec.find( ':' );

        if( colon == std::string::npos || colon <= start || refspec[colon - 1] != '*' || !refspec.ends_with( '*' ) )
            continue;

        const std::string source = refspec.substr( start, colon - start - 1 );
        const std::string target = refspec.substr( colon + 1, refspec.size() - colon - 2 );

        if( !target.starts_with( TRACKING ) )
            continue;

        stage.prefixes.emplace_back( source, target );

        for( const KIGIT_REFERENCE& ref : GetGitBackend()->ListReferences( m_repo, target + "*" ) )
            stage.tracked[ref.name] = ref.oid;
    }

    return stage;
}


kiapi::workspace::WorkspaceError MDW_GIT_CHECKOUTS::Fetch( git_repository* aRepo, STAGE& aStage )
{
    std::vector<wxString>        refspecs;
    std::map<wxString, wxString> advertised;

    for( const auto& [source, target] : aStage.prefixes )
        refspecs.push_back( wxString::FromUTF8( "+" + source + "*:" + target + "*" ) );

    kiapi::workspace::WorkspaceError fetched =
            m_remote.Transfer( aRepo, m_remoteName,
                               [&]( KIGIT::REMOTE_TRANSPORT& aTransport )
                               {
                                   return aTransport.DownloadRefspecs( refspecs, advertised );
                               } );

    for( const auto& [name, oid] : advertised )
    {
        const std::string ref = name.utf8_string();

        for( const auto& [source, target] : aStage.prefixes )
        {
            if( ref.starts_with( source ) )
                aStage.remote[target + ref.substr( source.size() )] = oid.utf8_string();
        }
    }

    return fetched;
}


MDW_GIT_CHECKOUTS::RESULT MDW_GIT_CHECKOUTS::Apply( const kiapi::workspace::WorkspaceError& aFetched,
                                                    const STAGE&                            aStage )
{
    std::map<std::string, MDW_GIT_CLAIM> before = snapshot();

    // A request fetched since Stage(), and a claim released in between must not come back
    if( aFetched.code() == kiapi::workspace::WE_OK && aStage.moves == m_moves )
    {
        // Compared with what Stage() read, so a quiet pass writes nothing and a ref moved meanwhile is never undone
        for( const auto& [target, oid] : aStage.remote )
        {
            auto was = aStage.tracked.find( target );

            if( was == aStage.tracked.end() || was->second != oid )
            {
                GetGitBackend()->SetReference( m_repo, target, oid,
                                               was == aStage.tracked.end() ? std::string() : was->second,
                                               "kicad: claims" );
            }
        }

        for( const auto& [target, oid] : aStage.tracked )
        {
            if( !aStage.remote.contains( target ) )
                GetGitBackend()->DeleteReference( m_repo, target, oid );
        }
    }

    return adopt( aFetched, before );
}


MDW_GIT_CHECKOUTS::RESULT MDW_GIT_CHECKOUTS::adopt( const kiapi::workspace::WorkspaceError&     aFetched,
                                                    const std::map<std::string, MDW_GIT_CLAIM>& aBefore )
{
    m_last = aFetched;

    if( m_last.code() != kiapi::workspace::WE_OK )
    {
        m_error = m_last.message();

        if( m_last.code() == kiapi::workspace::WE_BUSY )
            return RESULT::BUSY;

        return m_last.code() == kiapi::workspace::WE_CANCELLED ? RESULT::CANCELLED : RESULT::NETWORK;
    }

    if( !m_changeObserver )
        return RESULT::OK;

    std::map<std::string, MDW_GIT_CLAIM> after = snapshot();
    std::set<std::string>                candidates;
    std::vector<CHANGE>                  changes;

    for( const auto& [item, claim] : after )
    {
        auto was = aBefore.find( item );

        if( was == aBefore.end() || was->second.oid != claim.oid )
            candidates.insert( item );
    }

    for( const auto& [item, claim] : aBefore )
    {
        if( !after.contains( item ) )
            candidates.insert( item );
    }

    for( const std::string& item : candidates )
    {
        // All() cannot read every hashed claim, so Find() decides
        std::optional<MDW_GIT_CLAIM> current = Find( item );
        auto                         was = aBefore.find( item );

        if( !current )
        {
            if( was != aBefore.end() )
                changes.push_back( { was->second, false, releaseOf( was->second ) } );

            continue;
        }

        if( was != aBefore.end() && was->second.oid == current->oid )
            continue;

        // A claim broken and retaken between fetches still ends the former holder's claim
        if( was != aBefore.end() )
            changes.push_back( { was->second, false, releaseOf( was->second ) } );

        if( current->mine )
            continue;

        current->item = item;
        changes.push_back( { *current, true, std::nullopt } );
    }

    if( !changes.empty() )
        m_changeObserver( changes );

    return RESULT::OK;
}


std::optional<MDW_GIT_CHECKOUTS::RELEASE> MDW_GIT_CHECKOUTS::releaseOf( const MDW_GIT_CLAIM& aClaim )
{
    std::optional<std::string>         oid = GetGitBackend()->ReadReference( m_repo, releasedRef( aClaim.item ) );
    std::optional<KIGIT_COMMIT_RECORD> commit = oid ? GetGitBackend()->ReadCommit( m_repo, *oid ) : std::nullopt;
    nlohmann::json                     record = commit ? MdwGitRecordJson( commit->message ) : nlohmann::json();

    auto text = [&record]( const char* aKey )
    {
        auto it = record.find( aKey );
        return it != record.end() && it->is_string() ? MdwValidUtf8( it->get<std::string>() ) : std::string();
    };

    // The record of an older release of the item says nothing of this claim
    if( !record.is_object() || text( "claim" ) != aClaim.oid )
        return std::nullopt;

    return RELEASE{ text( "user" ), text( "email" ), text( "reason" ), text( "owner" ), std::nullopt };
}


std::optional<MDW_GIT_CLAIM> MDW_GIT_CHECKOUTS::Find( const std::string& aItemId )
{
    std::optional<MDW_GIT_CLAIM> held = read( trackingRef( aItemId ) );
    std::optional<MDW_GIT_CLAIM> ours = read( localRef( aItemId ) );

    if( held && ours && held->oid == ours->oid )
        held->mine = true;

    return held;
}


std::vector<MDW_GIT_CLAIM> MDW_GIT_CHECKOUTS::All()
{
    std::vector<MDW_GIT_CLAIM> claims;
    const std::string          prefix = "refs/kicad-remote/" + m_remoteName + "/checkouts/";

    for( const KIGIT_REFERENCE& ref : GetGitBackend()->ListReferences( m_repo, prefix + "*" ) )
    {
        std::string                  component = ref.name.substr( prefix.size() );
        std::optional<MDW_GIT_CLAIM> held = read( ref.name );
        std::optional<MDW_GIT_CLAIM> ours = read( CLAIMS + component );

        if( !held )
            continue;

        held->mine = ours && ours->oid == held->oid;

        // The ref names the item, so a record claiming another item is not believed
        if( held->item.empty() || RefComponent( held->item ) != component )
            held->item = component.starts_with( "%H" ) ? std::string() : MdwValidUtf8( decodeComponent( component ) );

        if( !held->item.empty() )
            claims.push_back( *held );
    }

    return claims;
}


std::unique_ptr<KIGIT::REPO_ACTION_LOCK> MDW_GIT_CHECKOUTS::lockClaims()
{
    std::unique_ptr<KIGIT::REPO_ACTION_LOCK> lock = KIGIT::RepoActionLock( m_repo, "kicad-claims" );

    if( !lock )
    {
        m_error = "The repository has no git directory";
        return nullptr;
    }

    if( lock->Acquire() == 0 )
        return lock;

    m_error = lock->Error().empty() ? std::string( "Another KiCad is changing editing claims" ) : lock->Error();
    return nullptr;
}


MDW_GIT_HOLDER MdwGitHolder( const GIT_IDENTITY& aAuthor, const std::string& aUsername, const std::string& aRemoteUrl )
{
    const std::string email = MdwValidUtf8( aAuthor.SignedEmail() );

    return { MdwValidUtf8( MdwGitOwnerId( aUsername, aRemoteUrl, email ) ), MdwValidUtf8( aAuthor.SignedName() ),
             email };
}


MDW_GIT_CHECKOUTS::RESULT MDW_GIT_CHECKOUTS::Take( const std::string& aItemId, const MDW_GIT_HOLDER& aHolder,
                                                   std::string* aCreated )
{
    m_holder.reset();

    std::unique_ptr<KIGIT::REPO_ACTION_LOCK> lock = lockClaims();

    if( !lock )
        return RESULT::BUSY;

    return take( aItemId, aHolder, aCreated );
}


MDW_GIT_CHECKOUTS::RESULT MDW_GIT_CHECKOUTS::take( const std::string& aItemId, const MDW_GIT_HOLDER& aHolder,
                                                   std::string* aCreated )
{
    if( RESULT fetched = Refresh(); fetched != RESULT::OK )
        return fetched;

    std::optional<MDW_GIT_CLAIM> held = Find( aItemId );

    if( held && held->mine )
        return RESULT::OK;

    if( held )
    {
        // Our record is stale once someone else's claim stands on the remote
        dropRef( localRef( aItemId ) );
        m_holder = held;
        return RESULT::HELD_BY_OTHER;
    }

    nlohmann::json record = { { "owner", aHolder.owner },
                              { "user", aHolder.user },
                              { "email", aHolder.email },
                              { "host", wxGetHostName().utf8_string() },
                              { "since", static_cast<std::int64_t>( std::time( nullptr ) ) },
                              { "item_id", aItemId },
                              { "token", MdwRandomToken() } };

    const std::string holder = aHolder.user.empty() ? "KiCad" : aHolder.user;
    const std::string subject = holder + " is editing " + MdwGitBaseName( aItemId );

    std::optional<std::string> created = GetGitBackend()->CreateRecordCommit(
            m_repo, MdwGitRecordMessage( subject, std::string(), record ), holder,
            aHolder.email.empty() ? "local" : aHolder.email );

    if( !created )
    {
        m_error = MdwGitError();
        return RESULT::NETWORK;
    }

    const std::string& oid = *created;

    // The local ref is the evidence a retry uses to recognise a claim that reached the remote
    if( !setRef( localRef( aItemId ), oid ) )
        return RESULT::NETWORK;

    GIT_REF_UPDATE create;
    create.localRef = wxString::FromUTF8( oid );
    create.remoteRef = wxString::FromUTF8( localRef( aItemId ) );
    create.leased = true;

    if( MdwCurrentRequestCancelled() )
    {
        dropRef( localRef( aItemId ) );
        return RESULT::CANCELLED;
    }

    RefPushResult     pushed = push( { create } );
    const std::string pushError = m_last.message();

    if( pushed == RefPushResult::Success )
    {
        setRef( trackingRef( aItemId ), oid );

        if( aCreated )
            *aCreated = oid;

        return RESULT::OK;
    }

    if( pushed == RefPushResult::Error )
    {
        m_error = pushError;
        return RESULT::UNKNOWN;
    }

    dropRef( localRef( aItemId ) );

    if( pushed == RefPushResult::LeaseBroken || pushed == RefPushResult::Rejected )
    {
        // Someone claimed it between our fetch and push
        if( Refresh() == RESULT::OK )
            m_holder = Find( aItemId );

        if( m_holder )
            return RESULT::HELD_BY_OTHER;
    }

    m_error = pushError;
    return RESULT::NETWORK;
}


std::string MDW_GIT_CHECKOUTS::LocalClaimId( const std::string& aItemId )
{
    std::optional<MDW_GIT_CLAIM> ours = read( localRef( aItemId ) );

    return ours ? ours->oid : std::string();
}


MDW_GIT_CHECKOUTS::RESULT MDW_GIT_CHECKOUTS::Give( const std::string& aItemId, const std::optional<RELEASE>& aForced )
{
    return give( aItemId, aForced, nullptr );
}


MDW_GIT_CHECKOUTS::RESULT MDW_GIT_CHECKOUTS::GiveClaim( const std::string& aItemId, const std::string& aClaimId )
{
    return give( aItemId, std::nullopt, &aClaimId );
}


MDW_GIT_CHECKOUTS::RESULT MDW_GIT_CHECKOUTS::give( const std::string& aItemId, const std::optional<RELEASE>& aForced,
                                                   const std::string* aClaimId )
{
    m_holder.reset();

    std::unique_ptr<KIGIT::REPO_ACTION_LOCK> lock = lockClaims();

    if( !lock )
        return RESULT::BUSY;

    const std::string ours = aClaimId ? *aClaimId : LocalClaimId( aItemId );

    std::string                  expected = ours;
    std::optional<MDW_GIT_CLAIM> held;

    if( aForced || ours.empty() )
    {
        if( RESULT fetched = Refresh(); fetched != RESULT::OK )
            return fetched;

        held = Find( aItemId );

        // Nothing to give up, which is also how a retried release finds its own success
        if( !held )
        {
            dropRefIf( localRef( aItemId ), ours );
            return RESULT::OK;
        }

        if( !aForced )
        {
            m_holder = held;
            return RESULT::NOT_HOLDER;
        }

        // The user confirmed one holder's claim, and another may have replaced it while they gave the reason
        if( aForced->confirmed
            && ( held->OwnerId() != aForced->confirmed->first || held->since != aForced->confirmed->second ) )
        {
            m_holder = held;
            return RESULT::REPLACED;
        }

        expected = held->oid;
    }

    std::vector<GIT_REF_UPDATE> updates( 1 );
    updates[0].remoteRef = wxString::FromUTF8( localRef( aItemId ) );
    updates[0].leased = true;
    updates[0].expectedOld = wxString::FromUTF8( expected );

    std::vector<GIT_REF_UPDATE> releaseRecord;

    if( aForced )
    {
        const std::string by = aForced->user.empty() ? "KiCad" : aForced->user;
        nlohmann::json    record = { { "owner", aForced->owner },
                                     { "user", aForced->user },
                                     { "email", aForced->email },
                                     { "since", static_cast<std::int64_t>( std::time( nullptr ) ) },
                                     { "item_id", aItemId },
                                     { "claim", expected },
                                     { "reason", aForced->reason } };

        std::optional<std::string> created = GetGitBackend()->CreateRecordCommit(
                m_repo,
                MdwGitRecordMessage( by + " released " + held->user + "'s claim on " + MdwGitBaseName( aItemId ),
                                     aForced->reason, record ),
                by, aForced->email.empty() ? "local" : aForced->email );

        if( !created )
        {
            m_error = MdwGitError();
            return RESULT::NETWORK;
        }

        GIT_REF_UPDATE& released = releaseRecord.emplace_back();
        released.localRef = wxString::FromUTF8( *created );
        released.remoteRef = wxString::FromUTF8( RELEASED + RefComponent( aItemId ) );
        released.force = true;
    }

    if( MdwCurrentRequestCancelled() )
        return RESULT::CANCELLED;

    RefPushResult pushed = push( updates );

    // Another process sharing this repository may have recorded a newer claim during the push
    if( pushed == RefPushResult::Success )
    {
        dropRefIf( localRef( aItemId ), ours );
        dropRef( trackingRef( aItemId ) );

        // The record follows the landed removal, since a server may take one ref and refuse the other. Losing
        // it only leaves the holder an unattributed notice, so its outcome stays out of m_last
        std::vector<GIT_REF_RESULT>      recorded;
        kiapi::workspace::WorkspaceError recordError;

        if( !releaseRecord.empty()
            && m_remote.Push( m_repo, m_remoteName, releaseRecord, recorded, recordError ) != RefPushResult::Success )
        {
            wxLogTrace( wxS( "KICAD_MDW" ), wxS( "Release record for %s was not pushed: %s" ),
                        wxString::FromUTF8( aItemId ), wxString::FromUTF8( recordError.message() ) );
        }

        return RESULT::OK;
    }

    if( pushed == RefPushResult::LeaseBroken )
    {
        // The claim moved under us, so it was taken over and is no longer ours to delete
        dropRefIf( localRef( aItemId ), ours );

        if( RESULT fetched = Refresh(); fetched != RESULT::OK )
            return fetched;

        m_holder = Find( aItemId );

        // Gone altogether is what a retry sees when an earlier release reached the remote
        return m_holder ? RESULT::NOT_HOLDER : RESULT::OK;
    }

    m_error = m_last.message();
    return pushed == RefPushResult::Error ? RESULT::UNKNOWN : RESULT::NETWORK;
}
