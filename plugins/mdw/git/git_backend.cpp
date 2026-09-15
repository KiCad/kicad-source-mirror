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

#include "git_backend.h"

#include <git/git_backend.h>
#include <git/kicad_git_common.h>
#include <git/kicad_git_memory.h>
#include <git/kigit_failure.h>
#include <git/kigit_lock_retry.h>
#include <git/kigit_remote_url.h>
#include <git/kigit_transport.h>

#include <mdw_runtime/mdw_event_publisher.h>
#include <mdw_runtime/mdw_file_io.h>
#include <mdw_runtime/mdw_paging.h>
#include <mdw_runtime/mdw_paths.h>
#include <mdw_runtime/mdw_strings.h>

#include <api/mdw/backend_base.pb.h>
#include <api/mdw/backend_locking.pb.h>
#include <api/mdw/backend_permissions.pb.h>
#include <api/mdw/backend_revisions.pb.h>

#include <git2.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <fstream>
#include <iterator>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <utility>

namespace fs = std::filesystem;
namespace mdw = kiapi::mdw;
namespace ws = kiapi::workspace;

namespace
{

const char OPERATION_TRAILER[] = "KiCad-MDW-Operation";

// A teammate's claim or library change arrives within this, and a quiet remote costs one fetch per interval
constexpr std::chrono::seconds POLL_INTERVAL{ 20 };


bool remoteAsksCredentials( const std::string& aUrl )
{
    return !aUrl.empty()
           && KIGIT::ParseRemoteURL( wxString::FromUTF8( aUrl ) ).connType != KIGIT::GIT_CONN_TYPE::GIT_CONN_LOCAL;
}


std::string parentDir( const std::string& aPath )
{
    std::size_t slash = aPath.find_last_of( '/' );

    return slash == std::string::npos ? std::string() : aPath.substr( 0, slash );
}


/// True for a project file at the bound root aPrefix, the item that stands for the whole project
bool isProjectItem( const std::string& aPrefix, const std::string& aPath )
{
    return parentDir( aPath ) == aPrefix && aPath.ends_with( ".kicad_pro" );
}


/// The repository path aPath relative to the bound root aPrefix, empty for a file outside it or one no local path names
std::string projectRelative( const std::string& aPrefix, const std::string& aPath )
{
    // A POSIX name may hold a backslash, which a local path reads as a separator naming another file
    if( aPath.find( '\\' ) != std::string::npos )
        return std::string();

    if( aPrefix.empty() )
        return aPath;

    const std::string folder = aPrefix + "/";

    return aPath.starts_with( folder ) ? aPath.substr( folder.size() ) : std::string();
}


/// Append the immediate child folders of aParentFolderId among aFolderPaths to aReply, marking has_children only
/// where expanding would show something
void addChildFolders( const std::set<std::string>& aFolderPaths, const std::string& aParentFolderId,
                      mdw::GetChildFoldersReply& aReply )
{
    const std::string     prefix = aParentFolderId.empty() ? std::string() : aParentFolderId + "/";
    std::set<std::string> emitted;

    for( const std::string& path : aFolderPaths )
    {
        if( path.compare( 0, prefix.size(), prefix ) != 0 )
            continue;

        std::string rest = path.substr( prefix.size() );

        if( rest.empty() )
            continue;

        std::size_t slash = rest.find( '/' );
        std::string child = prefix + ( slash == std::string::npos ? rest : rest.substr( 0, slash ) );

        if( child == aParentFolderId || !emitted.insert( child ).second )
            continue;

        // The set is ordered, so any descendant sorts right after child under "child/" and one lookup suffices
        const std::string deeper = child + "/";
        auto              below = aFolderPaths.lower_bound( deeper );

        ws::Folder* folder = aReply.add_folders();
        folder->set_folder_id( child );
        folder->set_display_name( MdwGitBaseName( child ) );
        folder->set_parent_folder_id( aParentFolderId );
        folder->set_has_children( below != aFolderPaths.end() && below->compare( 0, deeper.size(), deeper ) == 0 );
    }
}


ws::WorkspaceError filterDriverError( const std::string& aPath, const std::string& aDriver )
{
    return MdwFail( ws::WE_NOT_SUPPORTED, aPath + " uses the git filter \"" + aDriver
                                                  + "\", which KiCad cannot run. Check it in with git directly." );
}


/// The nearest existing directory that will hold aTarget's own entry once it is written
fs::path destinationDirectory( const fs::path& aTarget )
{
    std::error_code ec;

    // Writes replace aTarget's own entry, so a symlink there pointing elsewhere does not move it
    fs::path dir = fs::weakly_canonical( aTarget.parent_path(), ec );

    if( ec )
        dir = aTarget.parent_path();

    while( !fs::is_directory( dir, ec ) && dir.has_relative_path() )
        dir = dir.parent_path();

    return dir;
}


/// Puts a replaced working file back unless the check-in commits, so a failed check-in leaves no effect
class WORKING_FILE_GUARD
{
public:
    ~WORKING_FILE_GUARD() { Restore(); }

    /// Copy aFile aside into aStoreDir, outside the working tree; false when no copy could be made
    /// aInstalled is the content the check-in is about to put there
    bool Arm( const fs::path& aFile, const fs::path& aStoreDir, const std::string& aInstalled )
    {
        std::error_code ec;
        fs::path        saved = aStoreDir / ( "kicad-mdw-checkin-" + MdwRandomToken() );

        if( !fs::copy_file( aFile, saved, fs::copy_options::none, ec ) || ec )
            return false;

        m_file = aFile;
        m_saved = saved;
        m_installed = aInstalled;
        return true;
    }

    /// The check-in committed, so the replaced file stays
    void Keep()
    {
        std::error_code ignored;

        if( !m_saved.empty() )
            fs::remove( m_saved, ignored );

        m_saved.clear();
    }

    /// False when the saved copy could not be put back, which leaves it in the git directory. A file
    /// an editor saved since the check-in installed its copy is never overwritten
    bool Restore()
    {
        if( m_saved.empty() )
            return true;

        // A save landing between this check and the rename is still lost; editors take no lock
        std::optional<std::string> current = MDW_ReadFile( m_file );
        std::optional<std::string> saved = MDW_ReadFile( m_saved );

        if( !current || *current != m_installed || !saved || MDW_ReplaceFile( m_file, *saved ) )
            return false;

        Keep();
        return true;
    }

private:
    fs::path    m_file;
    fs::path    m_saved;
    std::string m_installed; ///< Copied once, so editing the source meanwhile cannot fool Restore
};


/// The trailer value for a check-in, unique to its idempotency id and request content
std::string operationValue( const mdw::CheckinRequest& aRequest )
{
    static const char HEX[] = "0123456789abcdef";

    // Hex keeps a client-supplied id from breaking out of the trailer line
    std::string value;

    for( unsigned char c : aRequest.idempotency_id() )
    {
        value += HEX[c >> 4];
        value += HEX[c & 0xf];
    }

    return value + "-" + MdwHashHex( aRequest.SerializeAsString() );
}


ws::Revision revisionOf( const MDW_GIT_VERSION& aVersion )
{
    ws::Revision revision;
    revision.set_revision_id( aVersion.oid );
    revision.set_label( aVersion.oid.substr( 0, 8 ) );
    revision.set_kind( ws::RK_CHECKIN );
    revision.set_author( MdwValidUtf8( aVersion.author ) );
    revision.set_timestamp_unix( aVersion.time );
    revision.set_comment( MdwValidUtf8( aVersion.summary ) );
    revision.mutable_state()->set_category( ws::LC_EDITABLE );

    for( const std::string& parent : aVersion.parents )
        revision.add_parent_revision_ids( parent );

    return revision;
}


// The repository path a LocateItems path names from the project at aPrefix, empty outside the repository
// or for a bare climb, nullopt when the path is illegal
std::optional<std::string> locatePath( const std::string& aPrefix, const std::string& aRelative )
{
    std::size_t start = 0;
    std::size_t up = 0;

    while( aRelative.compare( start, 3, "../" ) == 0 )
    {
        start += 3;
        up++;
    }

    std::string rest = aRelative.substr( start );

    if( rest == ".." )
    {
        rest.clear();
        up++;
    }

    if( rest.empty() ? up == 0 : !MdwIsSafeRelativePath( rest ) )
        return std::nullopt;

    // Lexical against the prefix, so a link in the tree cannot widen the climb
    std::string base = aPrefix;

    for( ; up > 0 && !base.empty(); up-- )
    {
        std::size_t slash = base.rfind( '/' );
        base.erase( slash == std::string::npos ? 0 : slash );
    }

    if( up > 0 || rest.empty() )
        return std::string();

    return base.empty() ? rest : base + "/" + rest;
}


/// Where push outcomes are kept, or nothing when this machine has no place for backend data
std::filesystem::path pushOutcomesFile()
{
    const fs::path dir = MDW_BackendDataDir( MDW_GIT_BACKEND::BACKEND_ID );

    return dir.is_absolute() ? dir / "push-outcomes.json" : fs::path();
}
} // namespace


MDW_GIT_LIBGIT2::MDW_GIT_LIBGIT2() :
        m_backend( std::make_unique<LIBGIT_BACKEND>() )
{
    m_backend->Init();
    SetGitBackend( m_backend.get() );
}


MDW_GIT_LIBGIT2::~MDW_GIT_LIBGIT2()
{
    SetGitBackend( nullptr );
    m_backend->Shutdown();
}


MDW_GIT_BACKEND::MDW_GIT_BACKEND( MDW_BACKEND_SERVER& aServer, MDW_EVENT_PUBLISHER* aPublisher ) :
        m_server( aServer ),
        m_publisher( aPublisher ),
        m_pushOutcomes( pushOutcomesFile() ),
        m_libraries( aServer, m_remote, m_repoMutex, MDW_BackendDataDir( BACKEND_ID ) / "library-cache" ),
        m_lifecycle( *this )
{
    m_libraries.SetClaimTest(
            [this]( const std::string& aId )
            {
                return ensureOpen() && m_claims && m_claims->Holds( aId );
            } );

    // Claims travel by refs of their own, whose refusal says nothing about writing the library
    m_remote.SetPushObserver(
            [this]( const std::vector<GIT_REF_UPDATE>& aUpdates, ws::PermissionState aState )
            {
                const std::string locator = m_server.HelloContext().workspace().locator();

                const bool claims = std::all_of( aUpdates.begin(), aUpdates.end(),
                                                 []( const GIT_REF_UPDATE& aUpdate )
                                                 {
                                                     return aUpdate.remoteRef.StartsWith( wxS( "refs/kicad/" ) );
                                                 } );

                // Keyed by the signed-in user, else the git identity
                if( !locator.empty() && !claims )
                {
                    m_pushOutcomes.Set( MDW_GIT_SHARED_LIBRARIES::NormalisedLocator( locator ),
                                        account( m_remote.Username() ).user_id(), m_remote.CredentialGeneration(),
                                        aState );
                }
            } );

    m_poller = std::make_unique<MDW_POLLER>( MdwPollInterval( POLL_INTERVAL ),
                                             [this]()
                                             {
                                                 ws::WorkspaceError outcome = pollClaims();

                                                 // Once a pass, so a refused login is not sent again for the library
                                                 if( outcome.code() == ws::WE_OK )
                                                     outcome = announceLibrary();

                                                 return MdwPollOutcome( outcome );
                                             } );
}


MDW_BACKEND_DESCRIPTOR MDW_GIT_BACKEND::Descriptor()
{
    MDW_BACKEND_DESCRIPTOR descriptor;
    descriptor.info.set_backend_id( BACKEND_ID );
    descriptor.info.set_display_name( "Git" );
    descriptor.info.set_description( "Files in a git working tree, committed in place" );
    descriptor.info.set_vendor( "KiCad" );
    descriptor.info.mutable_version()->set_major( 1 );
    descriptor.info.mutable_version()->set_minor( 0 );
    descriptor.info.set_checkin_model( ws::CM_WORKING_COPY );
    descriptor.capabilities.add_groups( ws::CG_REVISIONS );

    // Journals of different repositories sharing one data directory stay apart
    descriptor.idempotency_scope = []( const mdw::Hello& aHello )
    {
        fs::path     root = MdwFsPath( aHello.workspace().local_root() );
        MDW_GIT_REPO repo;

        if( root.empty() )
            return "git-source:" + MDW_GIT_SHARED_LIBRARIES::NormalisedLocator( aHello.workspace().locator() );

        // By folder, since a copy made after this Hello must keep the journal a restart reopens
        if( !aHello.workspace().locator().empty() )
            return "git:" + MdwUtf8Path( root / "" );

        return "git:" + ( repo.Open( root ) ? MdwUtf8Path( repo.Root() ) : MdwUtf8Path( root / "" ) );
    };

    descriptor.auth = []( const mdw::Hello& aHello ) -> std::optional<ws::AuthChallenge>
    {
        std::string  url = aHello.workspace().locator();
        MDW_GIT_REPO repo;

        if( url.empty() && repo.Open( MdwFsPath( aHello.workspace().local_root() ) ) )
            url = repo.RemoteUrl();

        if( !remoteAsksCredentials( url ) )
            return std::nullopt;

        KIGIT::REMOTE_URL parsed = KIGIT::ParseRemoteURL( wxString::FromUTF8( url ) );
        ws::AuthChallenge challenge = MdwGitAuthChallenge( parsed.username.utf8_string(),
                                                           parsed.connType == KIGIT::GIT_CONN_TYPE::GIT_CONN_SSH );
        challenge.set_realm( KIGIT::CredentialRealm( wxString::FromUTF8( url ) ).utf8_string() );
        return challenge;
    };

    return descriptor;
}


void MDW_GIT_BACKEND::Register()
{
    m_server.SetRequestFilter(
            [this]( const std::string& aType, const mdw::MdwRequest& aRequest ) -> std::optional<ws::WorkspaceError>
            {
                mdw::Hello hello;

                if( aType != mdw::Hello::descriptor()->full_name() || !aRequest.payload().UnpackTo( &hello ) )
                    return std::nullopt;

                fs::path   root = MdwFsPath( hello.workspace().local_root() );
                const bool source = !hello.workspace().locator().empty();

                // A library source's connection is tested before it has a copy, so only its locator is known
                if( root.empty() ? !source : !root.is_absolute() )
                    return MdwFail( ws::WE_INVALID_PARAMETER, "The workspace names no local working tree" );

                MDW_GIT_REPO repo;
                bool         opened = !root.empty() && repo.Open( root );

                // A source with a locator may not be cloned yet; anything else must already be a working tree
                if( hello.workspace().locator().empty() && !opened )
                {
                    return MdwFail( ws::WE_ITEM_NOT_FOUND,
                                    hello.workspace().local_root() + " is not in a git working tree" );
                }

                // The first Hello binds the workspace and a later one cannot rebind it
                if( !m_server.HelloContext().workspace().local_root().empty() )
                    return std::nullopt;

                // Claims travel through the remote, so a repository without one cannot share editing
                MDW_BACKEND_DESCRIPTOR descriptor = m_server.Descriptor();
                ws::Capabilities&      capabilities = descriptor.capabilities;
                auto*                  groups = capabilities.mutable_groups();

                groups->erase( std::remove( groups->begin(), groups->end(), ws::CG_LOCKING ), groups->end() );
                groups->erase( std::remove( groups->begin(), groups->end(), ws::CG_EVENTS ), groups->end() );
                groups->erase( std::remove( groups->begin(), groups->end(), ws::CG_SHARED_LIBRARIES ), groups->end() );
                groups->erase( std::remove( groups->begin(), groups->end(), ws::CG_LIFECYCLE ), groups->end() );
                groups->erase( std::remove( groups->begin(), groups->end(), ws::CG_PERMISSIONS ), groups->end() );
                capabilities.clear_custom_states();

                // A locator names a library repository, whose copy this session keeps and whose members it releases
                if( !hello.workspace().locator().empty() )
                {
                    capabilities.add_groups( ws::CG_SHARED_LIBRARIES );
                    capabilities.add_groups( ws::CG_PERMISSIONS );
                    capabilities.set_adopts_whole_files( true );

                    capabilities.add_groups( ws::CG_LIFECYCLE );

                    for( const ws::CustomState& state : MDW_GIT_LIFECYCLE::States() )
                        *capabilities.add_custom_states() = state;
                }

                // Decided once per session, so a source whose copy is made after this Hello still shares editing
                const bool claims = source || ( opened && repo.HasRemote() );

                if( claims )
                    capabilities.add_groups( ws::CG_LOCKING );

                // Claims and library changes are announced, and the host matches its own claims by their echoes
                if( m_publisher && claims )
                    capabilities.add_groups( ws::CG_EVENTS );

                m_server.SetDescriptor( descriptor );
                return std::nullopt;
            } );

    registerAuthHandlers();
    registerClaimHandlers();
    registerItemHandlers();
    registerWorkingCopyHandlers();
    registerRevisionHandlers();
    registerLifecycleHandlers();
    registerPermissionHandlers();
    m_libraries.Register();
}


std::string MDW_GIT_BACKEND::remoteUrl()
{
    const std::string locator = m_server.HelloContext().workspace().locator();

    if( !locator.empty() )
        return locator;

    MDW_GIT_REPO repo;
    return openBoundTree( repo ) ? repo.RemoteUrl() : std::string();
}


GIT_IDENTITY MDW_GIT_BACKEND::identity( const MDW_GIT_REPO& aRepo ) const
{
    return MdwGitIdentity( aRepo.Handle(), m_server.HelloContext().author(),
                           wxString::FromUTF8( m_remote.Username() ) );
}


MDW_GIT_HOLDER MDW_GIT_BACKEND::holder( const MDW_GIT_REPO& aRepo ) const
{
    return MdwGitHolder( identity( aRepo ), m_remote.Username(), aRepo.RemoteUrl() );
}


bool MDW_GIT_BACKEND::openBoundTree( MDW_GIT_REPO& aRepo ) const
{
    const fs::path root = MdwFsPath( m_server.HelloContext().workspace().local_root() );

    if( root.empty() )
        return false;

    return aRepo.Open( root );
}


ws::WorkspaceError MDW_GIT_BACKEND::checkConnection()
{
    const std::string url = remoteUrl();

    if( url.empty() )
        return MdwOk();

    // libgit2 calls a missing local path an unsupported protocol, which points the user the wrong way
    if( url.find( "://" ) == std::string::npos && !remoteAsksCredentials( url ) )
    {
        std::error_code ec;
        const bool      exists = fs::exists( MdwFsPath( url ), ec );

        if( ec )
            return MdwFail( ws::WE_NETWORK, url + ": " + ec.message() );

        if( !exists )
            return MdwFail( ws::WE_NETWORK, KIGIT::NoRepositoryAt( wxString::FromUTF8( url ) ).utf8_string() );

        return MdwOk();
    }

    // Well under the client's call timeout, so a silent remote reads as unreachable rather than as a refusal
    return m_remote.Check( url, std::chrono::seconds( 5 ) );
}


ws::Account MDW_GIT_BACKEND::account( const std::string& aUser ) const
{
    ws::Account  account;
    MDW_GIT_REPO repo;

    // An unopened repository answers the global identity, as a library source with no tree needs
    openBoundTree( repo );

    // Without the login, so a missing author leaves the name to the user id rather than a stand-in
    const GIT_IDENTITY author = MdwGitIdentity( repo.Handle(), m_server.HelloContext().author(), wxString() );

    account.set_user_id( MdwValidUtf8( aUser.empty() ? author.email : aUser ) );
    account.set_display_name( MdwValidUtf8( author.name ) );
    account.set_email( MdwValidUtf8( author.email ) );
    return account;
}


bool MDW_GIT_BACKEND::ensureOpen()
{
    if( m_open )
        return true;

    fs::path root = MdwFsPath( m_server.HelloContext().workspace().local_root() );

    if( root.empty() )
        return false;

    // Until the copy is made its folder may sit in another repository, whose claims are not this source's
    if( !m_server.HelloContext().workspace().locator().empty()
        && !m_libraries.IsCopy( m_server.HelloContext().workspace().local_root() ) )
    {
        return false;
    }

    if( !m_repo.Open( root ) )
        return false;

    std::error_code rootError;
    std::error_code repoError;
    std::error_code relativeError;
    fs::path        bound = fs::weakly_canonical( root, rootError );
    fs::path        top = fs::weakly_canonical( m_repo.Root(), repoError );
    std::u8string   inRepo = fs::relative( bound, top, relativeError ).generic_u8string();
    std::string     prefix( inRepo.begin(), inRepo.end() );

    if( rootError || repoError || relativeError || prefix.empty() || prefix == ".." || prefix.starts_with( "../" ) )
        return false;

    if( prefix == "." )
        prefix.clear();

    m_prefix = prefix;
    m_open = true;

    if( m_repo.HasRemote() )
    {
        m_claims = std::make_unique<MDW_GIT_CHECKOUTS>( m_repo.Handle(), m_remote, m_repo.RemoteName() );

        // Every fetch announces the claim changes it finds, the poll's and a claim call's alike
        if( m_publisher )
        {
            m_claims->SetChangeObserver(
                    [this]( const std::vector<MDW_GIT_CHECKOUTS::CHANGE>& aChanges )
                    {
                        publishClaimChanges( aChanges );
                    } );
        }
    }

    return true;
}


namespace
{
ws::LockInfo lockInfoOf( const std::optional<MDW_GIT_CLAIM>& aClaim )
{
    ws::LockInfo info;

    if( aClaim )
    {
        info.set_is_locked( true );
        info.set_owner_id( aClaim->OwnerId() );
        info.set_owner_display_name( aClaim->user );
        info.set_lock_timestamp_unix( aClaim->since );
        info.set_locked_by_me( aClaim->mine );
        info.set_advisory( true );
    }

    return info;
}


/// Every backend's lock event keys, so the tracker reads the owner a reply would give
/// With aRelease the event says aLock went, and aRelease's keys say who removed it
ws::EventEnvelope claimEvent( const std::string& aWorkspaceId, const std::string& aItemId, const ws::LockInfo& aLock,
                              const nlohmann::json& aRelease = nlohmann::json() )
{
    ws::EventEnvelope event;
    event.set_type( aLock.is_locked() && aRelease.is_null() ? ws::ET_ITEM_LOCKED : ws::ET_ITEM_UNLOCKED );
    event.set_workspace_id( aWorkspaceId );
    event.set_item_id( aItemId );

    if( !aLock.is_locked() )
        return event;

    if( aRelease.is_null() )
    {
        event.set_client_identity( aLock.owner_display_name().empty() ? aLock.owner_id() : aLock.owner_display_name() );
        event.set_timestamp_unix( aLock.lock_timestamp_unix() );
    }

    nlohmann::json payload = { { "owner_id", aLock.owner_id() },
                               { "owner_display_name", aLock.owner_display_name() },
                               { "lock_timestamp_unix", aLock.lock_timestamp_unix() },
                               { "locked_by_me", aLock.locked_by_me() },
                               { "advisory", aLock.advisory() } };

    if( aRelease.is_object() )
        payload.update( aRelease );

    event.set_payload( payload.dump( -1, ' ', false, nlohmann::json::error_handler_t::replace ) );
    return event;
}
} // namespace


void MDW_GIT_BACKEND::publishClaimChanges( const std::vector<MDW_GIT_CHECKOUTS::CHANGE>& aChanges )
{
    const std::string workspaceId = m_server.HelloContext().workspace().workspace_id();

    // By account, so the same person's claim from another clone is theirs too
    const MDW_GIT_HOLDER me = holder( m_repo );

    for( const MDW_GIT_CHECKOUTS::CHANGE& change : aChanges )
    {
        ws::LockInfo lock = lockInfoOf( change.claim );

        if( change.held )
        {
            m_publisher->Publish( claimEvent( workspaceId, change.claim.item, lock ) );
            continue;
        }

        const MDW_GIT_CLAIM& claim = change.claim;
        lock.set_locked_by_me( claim.mine || me.Is( claim.owner, claim.email ) );

        // With no record its holder gave it up, unless this repository still recorded it as ours
        nlohmann::json release = { { "released_by", claim.mine ? std::string() : claim.user },
                                   { "released_by_me", !claim.mine && me.Is( claim.owner, claim.email ) },
                                   { "reason", "" } };

        if( change.release )
        {
            release = { { "released_by", change.release->user },
                        { "released_by_me", me.Is( change.release->owner, change.release->email ) },
                        { "reason", change.release->reason } };
        }

        m_publisher->Publish( claimEvent( workspaceId, claim.item, lock, release ) );
    }
}


void MDW_GIT_BACKEND::echoClaim( const std::string& aItemId, const std::string& aOperationId )
{
    // Without an id the host cannot match the echo to its request and would take our claim for a teammate's
    if( aOperationId.empty() )
        return;

    std::optional<MDW_GIT_CLAIM> claim = m_claims->Find( aItemId );

    // Someone else's claim is not this request's doing, and an echo would make it ours
    if( claim && !claim->mine )
        return;

    m_publisher->Publish(
            claimEvent( m_server.HelloContext().workspace().workspace_id(), aItemId, lockInfoOf( claim ) ),
            aOperationId );
}


ws::WorkspaceError MDW_GIT_BACKEND::lockDestination( const fs::path&                           aTarget,
                                                     std::unique_ptr<KIGIT::REPO_ACTION_LOCK>& aLock )
{
    git_repository* found = nullptr;
    int             result = GetGitBackend()->FindRepository( MdwUtf8Path( destinationDirectory( aTarget ) ), found );

    if( result == GIT_ENOTFOUND )
        return MdwOk();

    if( result != 0 )
        return MdwFail( ws::WE_INTERNAL, MdwGitError() );

    KIGIT::GitRepositoryPtr holder( found );

    // Only the destination's repository is written, so its lock is the one taken even when it is ours
    aLock = KIGIT::RepoActionLock( found );
    return MdwGitLockActions( aLock.get() );
}


template <typename FN>
auto MDW_GIT_BACKEND::withRepo( FN aFn )
{
    return [this, aFn]( const auto& aRequest, auto& aReply )
    {
        std::lock_guard<std::mutex> lock( m_repoMutex );

        if( !ensureOpen() )
            return MdwFail( ws::WE_ITEM_NOT_FOUND, "The workspace has no git working tree yet" );

        // A request queued behind another may have been cancelled while it waited
        if( MdwCurrentRequestCancelled() )
            return MdwCancelled();

        return aFn( aRequest, aReply );
    };
}


namespace
{
// Requests whose success the host applies as our claim taken or given up
bool changesClaim( const mdw::LockRequest& )
{
    return true;
}


bool changesClaim( const mdw::UnlockRequest& )
{
    return true;
}


bool changesClaim( const mdw::CheckoutRequest& aRequest )
{
    return aRequest.lock();
}


bool changesClaim( const mdw::CancelCheckoutRequest& )
{
    return true;
}
} // namespace


template <typename FN>
auto MDW_GIT_BACKEND::echoes( FN aFn )
{
    return [this, aFn]( const auto& aRequest, auto& aReply )
    {
        ws::WorkspaceError error = aFn( aRequest, aReply );

        if( error.code() == ws::WE_OK && m_claims && m_publisher && changesClaim( aRequest ) )
            echoClaim( aRequest.item_id(), aRequest.idempotency_id() );

        return error;
    };
}


void MDW_GIT_BACKEND::registerAuthHandlers()
{
    m_server.RegisterHandler<mdw::AuthenticateRequest, mdw::AuthenticateReply>(
            [this]( const mdw::AuthenticateRequest& aRequest, mdw::AuthenticateReply& aReply )
            {
                std::map<std::string, std::string> fields( aRequest.credentials().fields().begin(),
                                                           aRequest.credentials().fields().end() );
                std::string                        user = fields["username"];

                m_remote.SetCredentials( fields );

                // Kept even when unverified, so a remote that comes back takes them without another prompt
                if( ws::WorkspaceError checked = answered( checkConnection(), true ); checked.code() != ws::WE_OK )
                    return checked;

                aReply.set_authenticated( true );
                *aReply.mutable_account() = account( user );
                return MdwOk();
            } );

    m_server.RegisterHandler<mdw::CheckConnectionRequest, mdw::CheckConnectionReply>(
            [this]( const mdw::CheckConnectionRequest&, mdw::CheckConnectionReply& aReply )
            {
                if( ws::WorkspaceError checked = answered( checkConnection() ); checked.code() != ws::WE_OK )
                    return checked;

                *aReply.mutable_account() = account( m_remote.Username() );
                return MdwOk();
            } );

    // Only the key the last refused connection met can be trusted
    m_server.RegisterHandler<mdw::TrustHostKeyRequest, mdw::TrustHostKeyReply>(
            [this]( const mdw::TrustHostKeyRequest& aRequest, mdw::TrustHostKeyReply& )
            {
                if( !m_remote.Trust( aRequest.host_key() ) )
                    return MdwFail( ws::WE_INVALID_PARAMETER, "No such host key is waiting to be trusted" );

                return MdwOk();
            } );
}


MDW_GIT_CHECKOUTS::RESULT MDW_GIT_BACKEND::takeClaim( const std::string& aItemId, std::string* aCreated )
{
    MDW_GIT_CHECKOUTS::RESULT result = m_claims->Take( aItemId, holder( m_repo ), aCreated );

    // The claim reached the remote, so an outage the last poll met is over
    if( result == MDW_GIT_CHECKOUTS::RESULT::OK )
        m_polled = MdwOk();

    return result;
}


ws::WorkspaceError MDW_GIT_BACKEND::settleClaims( MDW_GIT_CHECKOUTS::RESULT aResult )
{
    ws::WorkspaceError outcome = aResult == MDW_GIT_CHECKOUTS::RESULT::OK ? MdwOk() : claimFailure( aResult );

    // A busy repository or a cancelled call says nothing of the remote, so the next read asks again
    if( aResult == MDW_GIT_CHECKOUTS::RESULT::OK || aResult == MDW_GIT_CHECKOUTS::RESULT::NETWORK )
        m_polled = outcome;

    return outcome;
}


ws::WorkspaceError MDW_GIT_BACKEND::pollClaims()
{
    MDW_GIT_CHECKOUTS::STAGE stage;
    std::string              root;
    MDW_GIT_CHECKOUTS*       claims = nullptr;

    {
        std::lock_guard<std::mutex> lock( m_repoMutex );

        // Opened by a request, since a copy not yet made would open whatever repository encloses its folder
        if( !m_open || !m_claims )
            return MdwOk();

        claims = m_claims.get();
        stage = claims->Stage( { m_lifecycle.MirrorRefspec() } );
        root = MdwUtf8Path( m_repo.Root() );
    }

    // A handle of its own, so a slow remote holds up no local request
    KIGIT::GitRepositoryPtr repo( GetGitBackend()->OpenRepositoryAt( root ) );
    ws::WorkspaceError      fetched =
            repo ? claims->Fetch( repo.get(), stage ) : MdwFail( ws::WE_INTERNAL, root + " cannot be opened" );

    std::lock_guard<std::mutex> lock( m_repoMutex );
    return settleClaims( claims->Apply( fetched, stage ) );
}


ws::WorkspaceError MDW_GIT_BACKEND::announceLibrary()
{
    std::string        tip;
    ws::WorkspaceError read = m_libraries.UnheldTip( tip );

    // Once per tip, since the copy lacks it until the user asks for the update
    if( m_publisher && !tip.empty() && tip != m_announcedTip )
    {
        ws::EventEnvelope event;
        event.set_type( ws::ET_LIBRARY_CHANGED );
        event.set_workspace_id( m_server.HelloContext().workspace().workspace_id() );

        if( m_publisher->Publish( event ) )
            m_announcedTip = tip;
    }

    return read;
}


ws::WorkspaceError MDW_GIT_BACKEND::answered( const ws::WorkspaceError& aChecked, bool aNewCredentials )
{
    const MDW_POLL outcome = MdwPollOutcome( aChecked );

    if( outcome == MDW_POLL::OK )
    {
        m_answered = true;
        m_poller->Resume();
    }
    else if( outcome == MDW_POLL::REFUSED )
    {
        m_poller->Hold();
    }
    else if( aNewCredentials )
    {
        m_poller->Resume();
    }

    return aChecked;
}


ws::WorkspaceError MDW_GIT_BACKEND::readClaims()
{
    // An unreachable remote is asked once per poll, yet a reconnect reads it again at once
    if( m_answered.exchange( false ) && m_polled && m_polled->code() != ws::WE_OK )
        m_polled.reset();

    return m_polled ? *m_polled : settleClaims( m_claims->Refresh( { m_lifecycle.MirrorRefspec() } ) );
}


bool MDW_GIT_BACKEND::isMember( const std::string& aItemId ) const
{
    return !m_server.HelloContext().workspace().locator().empty() && MDW_GIT_SHARED_LIBRARIES::IsMemberId( aItemId );
}


ws::WorkspaceError MDW_GIT_BACKEND::checkoutMember( const mdw::CheckoutRequest& aRequest, mdw::CheckoutReply& aReply )
{
    const std::string& id = aRequest.item_id();
    std::string        path;

    auto check = [&]()
    {
        if( ws::WorkspaceError error = m_libraries.MemberInfo( id, *aReply.mutable_item(), path );
            error.code() != ws::WE_OK )
        {
            return error;
        }

        // A link would carry the editor's saves out of the copy
        if( path.empty() )
            return MdwFail( ws::WE_INVALID_PARAMETER, id + " is not a regular file in the library copy" );

        std::string        tip;
        ws::WorkspaceError whole = MdwOk();

        // A derived symbol can never be released, so it is not taken out to revise either
        if( !m_libraries.SourceMemberFile( id, tip, whole ) && whole.code() == ws::WE_NOT_SUPPORTED )
            return whole;

        std::error_code ec;

        // A part is edited where it lives, so no other file can stand in for its library
        if( !aRequest.local_path().empty()
            && !fs::equivalent( MdwFsPath( aRequest.local_path() ), MdwFsPath( path ), ec ) )
        {
            return MdwFail( ws::WE_INVALID_PARAMETER, id + " is edited in " + path );
        }

        return MdwOk();
    };

    if( ws::WorkspaceError error = check(); error.code() != ws::WE_OK )
        return error;

    if( aRequest.lock() )
    {
        if( !m_claims )
            return MdwFail( ws::WE_NOT_SUPPORTED, "The repository has no remote to share editing through" );

        std::string created;

        if( MDW_GIT_CHECKOUTS::RESULT result = takeClaim( id, &created ); result != MDW_GIT_CHECKOUTS::RESULT::OK )
            return claimFailure( result );

        // Checked again, since the copy may have changed while the remote was asked
        if( ws::WorkspaceError error = check(); error.code() != ws::WE_OK )
        {
            if( !created.empty() && m_claims->GiveClaim( id, created ) != MDW_GIT_CHECKOUTS::RESULT::OK )
                throw std::runtime_error( "checkout failed and its claim could not be released" );

            return error;
        }
    }

    if( m_claims )
        *aReply.mutable_item()->mutable_lock() = lockInfoOf( m_claims->Find( id ) );

    aReply.set_local_path( path );
    return MdwOk();
}


ws::WorkspaceError MDW_GIT_BACKEND::claimFailure( MDW_GIT_CHECKOUTS::RESULT aResult )
{
    using RESULT = MDW_GIT_CHECKOUTS::RESULT;

    const std::optional<MDW_GIT_CLAIM>& holder = m_claims->Holder();

    switch( aResult )
    {
    case RESULT::HELD_BY_OTHER: return MdwFail( ws::WE_LOCK_CONFLICT, "Being edited by " + holder->user );

    case RESULT::NOT_HOLDER:
        return MdwFail( ws::WE_NOT_LOCK_OWNER,
                        holder ? "now being edited by " + holder->user : std::string( "not being edited by you" ) );

    case RESULT::BUSY: return MdwFail( ws::WE_BUSY, "Another KiCad is using this repository; try again in a moment" );

    case RESULT::REPLACED:
        return MdwFail( ws::WE_BUSY, "now being edited by " + holder->user + "; check who is editing and try again" );

    case RESULT::CANCELLED: return MdwCancelled();

    default: break;
    }

    // A refused login or host key means the remote took nothing
    if( std::optional<ws::WorkspaceError> refused = m_claims->CredentialFailure() )
        return *refused;

    if( aResult == RESULT::UNKNOWN )
        throw std::runtime_error( "the remote may have taken the claim change: " + m_claims->Error() );

    return MdwFail( ws::WE_NETWORK, m_claims->Error().empty() ? "Cannot reach the remote" : m_claims->Error() );
}


void MDW_GIT_BACKEND::registerClaimHandlers()
{
    using RESULT = MDW_GIT_CHECKOUTS::RESULT;

    auto noRemote = []()
    {
        return MdwFail( ws::WE_NOT_SUPPORTED, "The repository has no remote to share editing through" );
    };

    m_server.RegisterHandler<mdw::LockRequest, mdw::LockReply>( withRepo( echoes(
            [this, noRemote]( const mdw::LockRequest& aRequest, mdw::LockReply& aReply )
            {
                if( !m_claims )
                    return noRemote();

                if( !m_repo.IsTracked( aRequest.item_id() ) )
                    return MdwFail( ws::WE_ITEM_NOT_FOUND, "No such tracked file" );

                if( RESULT result = takeClaim( aRequest.item_id() ); result != RESULT::OK )
                    return claimFailure( result );

                *aReply.mutable_lock() = lockInfoOf( m_claims->Find( aRequest.item_id() ) );
                return MdwOk();
            } ) ) );

    m_server.RegisterHandler<mdw::UnlockRequest, mdw::UnlockReply>( withRepo( echoes(
            [this, noRemote]( const mdw::UnlockRequest& aRequest, mdw::UnlockReply& )
            {
                if( !m_claims )
                    return noRemote();

                std::optional<MDW_GIT_CHECKOUTS::RELEASE> forced;

                if( aRequest.force() )
                {
                    const MDW_GIT_HOLDER by = holder( m_repo );

                    forced = MDW_GIT_CHECKOUTS::RELEASE{ by.user, by.email, MdwValidUtf8( aRequest.reason() ),
                                                         by.owner, std::nullopt };

                    if( aRequest.has_expected() )
                    {
                        forced->confirmed = std::make_pair( aRequest.expected().owner_id(),
                                                            aRequest.expected().lock_timestamp_unix() );
                    }
                }

                RESULT result = m_claims->Give( aRequest.item_id(), forced );

                return result == RESULT::OK ? MdwOk() : claimFailure( result );
            } ) ) );

    // Take and Give reconcile against the remote through our local claim ref, so rerunning them is safe
    auto rerun = []( const auto&, const std::string&, auto&, ws::WorkspaceError& )
    {
        return MDW_RECOVERY::MISS;
    };

    auto noToken = []( const auto&, std::string& )
    {
        return MdwOk();
    };

    // The claim a release starts from tells a finished release apart from a claim taken after it
    auto unlockToken = [this]( const mdw::UnlockRequest& aRequest, std::string& aToken )
    {
        std::lock_guard<std::mutex> lock( m_repoMutex );

        if( ensureOpen() && m_claims )
            aToken = m_claims->LocalClaimId( aRequest.item_id() );

        return MdwOk();
    };

    auto rerunUnlock = [this]( const mdw::UnlockRequest& aRequest, const std::string& aToken, mdw::UnlockReply&,
                               ws::WorkspaceError& aError )
    {
        // Rerun, a forced release would remove whoever claimed the item since
        if( aRequest.force() )
        {
            aError = MdwFail( ws::WE_VERSION_CONFLICT,
                              "A forced release was interrupted; check who is editing and release it again" );
            return MDW_RECOVERY::FAIL;
        }

        std::lock_guard<std::mutex> lock( m_repoMutex );

        if( !ensureOpen() || !m_claims )
            return MDW_RECOVERY::MISS;

        // A replayed release reaches the host as the original would have, so it echoes the same way
        auto replay = [this, &aRequest]()
        {
            if( m_publisher )
                echoClaim( aRequest.item_id(), aRequest.idempotency_id() );

            return MDW_RECOVERY::REPLAY;
        };

        std::string current = m_claims->LocalClaimId( aRequest.item_id() );

        // A claim recorded since the attempt began came after the release
        if( current != aToken )
        {
            if( !aToken.empty() )
                return replay();

            aError = MdwFail( ws::WE_VERSION_CONFLICT, "The item was claimed again after this release began" );
            return MDW_RECOVERY::FAIL;
        }

        // Pinned to the claim checked above, since another process can release it and claim again meanwhile
        MDW_GIT_CHECKOUTS::RESULT result = m_claims->GiveClaim( aRequest.item_id(), aToken );

        if( result == MDW_GIT_CHECKOUTS::RESULT::OK )
            return replay();

        try
        {
            aError = claimFailure( result );
        }
        catch( const std::runtime_error& e )
        {
            aError = MdwFail( ws::WE_NETWORK, e.what() );
        }

        return MDW_RECOVERY::FAIL;
    };

    m_server.RegisterRecovery<mdw::LockRequest, mdw::LockReply>( noToken, rerun );
    m_server.RegisterRecovery<mdw::UnlockRequest, mdw::UnlockReply>( unlockToken, rerunUnlock );

    m_server.RegisterHandler<mdw::GetLockInfoRequest, mdw::GetLockInfoReply>( withRepo(
            [this, noRemote]( const mdw::GetLockInfoRequest& aRequest, mdw::GetLockInfoReply& aReply )
            {
                if( !m_claims )
                    return noRemote();

                if( ws::WorkspaceError error = readClaims(); error.code() != ws::WE_OK )
                    return error;

                *aReply.mutable_lock() = lockInfoOf( m_claims->Find( aRequest.item_id() ) );
                return MdwOk();
            } ) );

    m_server.RegisterHandler<mdw::ListLocksRequest, mdw::ListLocksReply>( withRepo(
            [this, noRemote]( const mdw::ListLocksRequest&, mdw::ListLocksReply& aReply )
            {
                if( !m_claims )
                    return noRemote();

                if( ws::WorkspaceError error = readClaims(); error.code() != ws::WE_OK )
                    return error;

                for( const MDW_GIT_CLAIM& claim : m_claims->All() )
                {
                    mdw::ItemLock* entry = aReply.add_locks();
                    entry->set_item_id( claim.item );
                    *entry->mutable_lock() = lockInfoOf( claim );
                }

                return MdwOk();
            } ) );
}


void MDW_GIT_BACKEND::registerWorkingCopyHandlers()
{
    MDW_GIT_REPO& repo = m_repo;

    m_server.RegisterHandler<mdw::LocateItemsRequest, mdw::LocateItemsReply>( withRepo(
            [this, &repo]( const mdw::LocateItemsRequest& aRequest, mdw::LocateItemsReply& aReply )
            {
                std::vector<std::string> itemPaths;
                std::vector<std::string> committed;

                for( const std::string& relative : aRequest.relative_paths() )
                {
                    std::optional<std::string> located = locatePath( m_prefix, relative );

                    if( !located )
                    {
                        return MdwFail( ws::WE_INVALID_PARAMETER,
                                        relative + " is not a relative path under the workspace" );
                    }

                    itemPaths.push_back( std::move( *located ) );
                }

                std::vector<MDW_GIT_REPO::PLACE> places = repo.Locate( itemPaths );

                for( size_t ii = 0; ii < itemPaths.size(); ++ii )
                {
                    if( places[ii] == MDW_GIT_REPO::PLACE::IN_HEAD )
                        committed.push_back( itemPaths[ii] );
                    else if( places[ii] == MDW_GIT_REPO::PLACE::UNTRACKED )
                        itemPaths[ii].clear();
                }

                std::map<std::string, MDW_GIT_VERSION> bases =
                        repo.LastChanges( committed, MdwCurrentRequestCancelled );

                for( const std::string& path : committed )
                {
                    if( !isProjectItem( m_prefix, path ) )
                        continue;

                    std::optional<MDW_GIT_VERSION> scope = repo.ScopeRevision( m_prefix, MdwCurrentRequestCancelled );

                    if( scope )
                        bases[path] = *scope;
                }

                if( MdwCurrentRequestCancelled() )
                    return MdwCancelled();

                for( const std::string& itemPath : itemPaths )
                {
                    mdw::LocatedItem* located = aReply.add_items();

                    if( itemPath.empty() )
                        continue;

                    located->mutable_item()->set_item_id( itemPath );
                    located->mutable_item()->set_path_hint( itemPath );

                    if( auto it = bases.find( itemPath ); it != bases.end() )
                    {
                        located->set_base_revision_id( it->second.oid );
                        located->set_base_revision_label( it->second.oid.substr( 0, 8 ) );
                    }
                }

                return MdwOk();
            } ) );

    m_server.RegisterHandler<mdw::ListLocalChangesRequest, mdw::ListLocalChangesReply>( withRepo(
            [this, &repo]( const mdw::ListLocalChangesRequest& aRequest, mdw::ListLocalChangesReply& aReply )
            {
                const std::string bound = m_server.HelloContext().workspace().local_root();

                if( m_libraries.IsCopy( aRequest.local_root() ) )
                    return m_libraries.LocalChanges( aRequest, aReply );

                if( !MdwSameRoot( aRequest.local_root(), bound ) )
                    return MdwFail( ws::WE_INVALID_PARAMETER, aRequest.local_root() + " is not this workspace's root" );

                std::optional<std::vector<MDW_GIT_CHANGE>> changes = repo.LocalChanges( m_prefix );

                if( !changes )
                    return MdwFail( ws::WE_INTERNAL, "Cannot read the git status: " + MdwGitError() );

                std::vector<std::string> committed;

                for( const MDW_GIT_CHANGE& change : *changes )
                {
                    if( change.kind != MDW_GIT_CHANGE::KIND::ADDED )
                        committed.push_back( change.path );
                }

                std::map<std::string, MDW_GIT_VERSION> bases =
                        repo.LastChanges( committed, MdwCurrentRequestCancelled );

                if( MdwCurrentRequestCancelled() )
                    return MdwCancelled();

                // The token binds the page size and this listing, so a copy changed mid-walk conflicts
                std::string listing = repo.HeadOid();

                for( const MDW_GIT_CHANGE& change : *changes )
                {
                    listing += '\0' + change.path + '\0' + std::to_string( static_cast<int>( change.kind ) ) + '\0'
                               + std::to_string( change.tracked );
                }

                const std::string snapshot = std::to_string( std::hash<std::string>{}( listing ) );
                size_t            offset = 0;
                size_t            end = 0;
                std::string       next;

                if( ws::WorkspaceError error =
                            MdwPageRange( aRequest.page(), changes->size(), snapshot, offset, end, next );
                    error.code() != ws::WE_OK )
                {
                    return error;
                }

                for( size_t i = offset; i < end; ++i )
                {
                    const MDW_GIT_CHANGE& change = ( *changes )[i];
                    mdw::LocalChange*     row = aReply.add_changes();
                    std::string           relative = change.path.substr( m_prefix.empty() ? 0 : m_prefix.size() + 1 );

                    row->mutable_member()->set_item_id( change.path );
                    row->mutable_member()->set_path_hint( change.path );
                    row->set_relative_path( relative );
                    row->set_kind( MdwDocumentKind( change.path ) );
                    row->set_provisional( !change.tracked );

                    row->set_change( change.kind == MDW_GIT_CHANGE::KIND::ADDED     ? mdw::CT_ADDED
                                     : change.kind == MDW_GIT_CHANGE::KIND::REMOVED ? mdw::CT_REMOVED
                                                                                    : mdw::CT_MODIFIED );

                    if( auto it = bases.find( change.path ); it != bases.end() )
                        row->set_base_revision_id( it->second.oid );
                }

                aReply.set_next_page_token( next );
                return MdwOk();
            } ) );
}


void MDW_GIT_BACKEND::registerItemHandlers()
{
    MDW_BACKEND_SERVER& server = m_server;
    MDW_GIT_REPO&       repo = m_repo;

    auto describe = [this]( const std::string& aPath, const MDW_GIT_VERSION* aNewest )
    {
        ws::ItemInfo item;
        item.set_item_id( aPath );
        item.set_display_name( MdwGitBaseName( aPath ) );
        item.set_folder_id( parentDir( aPath ) );
        item.set_path_hint( aPath );

        // The client resolves it against the project folder, which may sit below the repository root
        item.set_relative_path( projectRelative( m_prefix, aPath ) );
        item.set_kind( MdwDocumentKind( aPath ) );
        item.mutable_state()->set_category( ws::LC_EDITABLE );

        if( aNewest )
        {
            item.set_revision_id( aNewest->oid );
            item.set_revision_kind( ws::RK_CHECKIN );
            item.set_modified_unix( aNewest->time );
        }

        return item;
    };

    // The project item's revision is the project's, and it reads released while a design release tags that
    auto itemInfo = [this, &repo, describe]( const std::string& aPath )
    {
        if( !isProjectItem( m_prefix, aPath ) )
        {
            std::map<std::string, MDW_GIT_VERSION> newest = repo.LastChanges( { aPath }, MdwCurrentRequestCancelled );

            return describe( aPath, newest.empty() ? nullptr : &newest.begin()->second );
        }

        // A teammate's release counts once a poll mirrors it, so the first read must not precede that poll
        if( m_claims )
            readClaims();

        std::optional<MDW_GIT_VERSION> scope = repo.ScopeRevision( m_prefix, MdwCurrentRequestCancelled );
        ws::ItemInfo                   item = describe( aPath, scope ? &*scope : nullptr );
        std::map<std::string, int>     baselines = m_lifecycle.Baselines( aPath );

        if( auto it = scope ? baselines.find( scope->oid ) : baselines.end(); it != baselines.end() )
        {
            *item.mutable_state() = MdwGitReleasedState();
            item.set_revision_label( MdwGitBaselineLabel( it->second ) );
            item.set_revision_kind( ws::RK_BASELINE );
        }

        return item;
    };

    server.RegisterHandler<mdw::GetRootFoldersRequest, mdw::GetRootFoldersReply>( withRepo(
            [&repo]( const mdw::GetRootFoldersRequest&, mdw::GetRootFoldersReply& aReply )
            {
                std::optional<std::vector<std::string>> files = repo.TrackedFiles();

                if( !files )
                    return MdwFail( ws::WE_INTERNAL, "Cannot read the git index" );

                ws::Folder* folder = aReply.add_folders();
                folder->set_folder_id( "" );
                folder->set_display_name( "Root" );
                folder->set_has_children( std::any_of( files->begin(), files->end(),
                                                       []( const std::string& aPath )
                                                       {
                                                           return aPath.find( '/' ) != std::string::npos;
                                                       } ) );

                return MdwOk();
            } ) );

    server.RegisterHandler<mdw::GetChildFoldersRequest, mdw::GetChildFoldersReply>( withRepo(
            [&repo]( const mdw::GetChildFoldersRequest& aRequest, mdw::GetChildFoldersReply& aReply )
            {
                std::optional<std::vector<std::string>> files = repo.TrackedFiles();

                if( !files )
                    return MdwFail( ws::WE_INTERNAL, "Cannot read the git index" );

                std::set<std::string> folders;

                for( const std::string& path : *files )
                {
                    if( std::string dir = parentDir( path ); !dir.empty() )
                        folders.insert( dir );
                }

                addChildFolders( folders, aRequest.folder_id(), aReply );
                return MdwOk();
            } ) );

    server.RegisterHandler<mdw::ListItemsRequest, mdw::ListItemsReply>( withRepo(
            [this, &repo, describe, itemInfo]( const mdw::ListItemsRequest& aRequest, mdw::ListItemsReply& aReply )
            {
                std::optional<std::vector<std::string>> files = repo.TrackedFiles();

                if( !files )
                    return MdwFail( ws::WE_INTERNAL, "Cannot read the git index" );

                std::vector<std::string> inFolder;

                for( const std::string& path : *files )
                {
                    if( parentDir( path ) == aRequest.folder_id() )
                        inFolder.push_back( path );
                }

                std::map<std::string, MDW_GIT_VERSION> newest =
                        repo.LastChanges( inFolder, MdwCurrentRequestCancelled );

                if( MdwCurrentRequestCancelled() )
                    return MdwCancelled();

                for( const std::string& path : inFolder )
                {
                    auto it = newest.find( path );

                    *aReply.add_items() = isProjectItem( m_prefix, path )
                                                  ? itemInfo( path )
                                                  : describe( path, it == newest.end() ? nullptr : &it->second );
                }

                return MdwOk();
            } ) );

    server.RegisterHandler<mdw::GetItemInfoRequest, mdw::GetItemInfoReply>( withRepo(
            [this, &repo, itemInfo]( const mdw::GetItemInfoRequest& aRequest, mdw::GetItemInfoReply& aReply )
            {
                if( isMember( aRequest.item_id() ) )
                {
                    std::string path;

                    if( ws::WorkspaceError error =
                                m_libraries.MemberInfo( aRequest.item_id(), *aReply.mutable_item(), path );
                        error.code() != ws::WE_OK )
                    {
                        return error;
                    }

                    // A session that never claimed anything must still see a teammate's claim, or offline the last one
                    if( m_claims )
                    {
                        readClaims();
                        *aReply.mutable_item()->mutable_lock() = lockInfoOf( m_claims->Find( aRequest.item_id() ) );
                    }

                    return MdwOk();
                }

                if( !repo.IsTracked( aRequest.item_id() ) )
                    return MdwFail( ws::WE_ITEM_NOT_FOUND, "No such tracked file" );

                *aReply.mutable_item() = itemInfo( aRequest.item_id() );
                return MdwOk();
            } ) );

    server.RegisterHandler<mdw::CheckoutRequest, mdw::CheckoutReply>( withRepo( echoes(
            [this, &repo, itemInfo]( const mdw::CheckoutRequest& aRequest, mdw::CheckoutReply& aReply )
            {
                if( isMember( aRequest.item_id() ) )
                    return checkoutMember( aRequest, aReply );

                fs::path target = aRequest.local_path().empty() ? fs::path() : MdwFsPath( aRequest.local_path() );

                if( !target.empty() && !target.is_absolute() )
                    return MdwFail( ws::WE_INVALID_PARAMETER, "Local path must be absolute" );

                fs::path inTree;

                auto check = [&repo, &aRequest, &inTree]()
                {
                    if( !repo.IsTracked( aRequest.item_id() ) )
                        return MdwFail( ws::WE_ITEM_NOT_FOUND, "No such tracked file" );

                    inTree = MdwLinkFreePath( repo.Root(), aRequest.item_id(), false );

                    if( inTree.empty() )
                        return MdwFail( ws::WE_INVALID_PARAMETER, "Item is not a regular file in the repository" );

                    return MdwOk();
                };

                // A copy into a working tree checks under its lock, which claims come before, so the lock is
                // let go before the claim and the checks run again after
                {
                    std::unique_ptr<KIGIT::REPO_ACTION_LOCK> actions;

                    if( !target.empty() )
                    {
                        if( ws::WorkspaceError error = lockDestination( target, actions ); error.code() != ws::WE_OK )
                            return error;
                    }

                    if( ws::WorkspaceError error = check(); error.code() != ws::WE_OK )
                        return error;
                }

                std::string created;

                if( aRequest.lock() )
                {
                    if( !m_claims )
                        return MdwFail( ws::WE_NOT_SUPPORTED, "The repository has no remote to share editing through" );

                    if( MDW_GIT_CHECKOUTS::RESULT result = takeClaim( aRequest.item_id(), &created );
                        result != MDW_GIT_CHECKOUTS::RESULT::OK )
                    {
                        return claimFailure( result );
                    }
                }

                // A claim taken above must not outlive a checkout that then fails, but one held before stays
                auto refuse = [this, &aRequest, &created]( const ws::WorkspaceError& aError )
                {
                    if( !created.empty()
                        && m_claims->GiveClaim( aRequest.item_id(), created ) != MDW_GIT_CHECKOUTS::RESULT::OK )
                        throw std::runtime_error( "checkout failed and its claim could not be released" );

                    return aError;
                };

                *aReply.mutable_item() = itemInfo( aRequest.item_id() );

                if( aRequest.lock() )
                {
                    const MDW_GIT_HOLDER by = holder( repo );
                    ws::LockInfo*        lock = aReply.mutable_item()->mutable_lock();
                    lock->set_is_locked( true );
                    lock->set_owner_id( by.owner );
                    lock->set_owner_display_name( by.user );
                    lock->set_locked_by_me( true );
                }

                // The working tree is already the local copy unless the caller asked for it elsewhere
                if( target.empty() )
                {
                    aReply.set_local_path( MdwUtf8Path( inTree ) );
                    return MdwOk();
                }

                ws::WorkspaceError error = MdwOk();

                // The claim is released only after this lock is, since claims come before it
                {
                    std::unique_ptr<KIGIT::REPO_ACTION_LOCK> actions;
                    std::error_code                          ec;

                    error = lockDestination( target, actions );

                    if( error.code() == ws::WE_OK )
                        error = check();

                    if( error.code() == ws::WE_OK && !fs::equivalent( inTree, target, ec ) )
                    {
                        if( MdwCurrentRequestCancelled() )
                        {
                            error = MdwCancelled();
                        }
                        else
                        {
                            fs::create_directories( target.parent_path(), ec );

                            std::optional<std::string> bytes = MDW_ReadFile( inTree );
                            std::error_code            failure = bytes ? MDW_ReplaceFile( target, *bytes )
                                                                       : std::make_error_code( std::errc::io_error );

                            if( failure )
                                error = MdwFail( ws::WE_INTERNAL,
                                                 "Cannot write " + aRequest.local_path() + ": " + failure.message() );
                        }
                    }
                }

                if( error.code() != ws::WE_OK )
                    return refuse( error );

                aReply.set_local_path( aRequest.local_path() );
                return MdwOk();
            } ) ) );

    server.RegisterHandler<mdw::CheckinRequest, mdw::CheckinReply>( withRepo(
            [this, &repo]( const mdw::CheckinRequest& aRequest, mdw::CheckinReply& aReply )
            {
                // Before the checks too, since a branch switch can change what they answer
                std::unique_ptr<KIGIT::REPO_ACTION_LOCK> actions = KIGIT::RepoActionLock( repo.Handle() );

                if( ws::WorkspaceError error = MdwGitLockActions( actions.get() ); error.code() != ws::WE_OK )
                    return error;

                if( !repo.IsTracked( aRequest.item_id() ) )
                    return MdwFail( ws::WE_ITEM_NOT_FOUND, "No such tracked file" );

                fs::path inTree = MdwLinkFreePath( repo.Root(), aRequest.item_id(), false );

                if( inTree.empty() )
                    return MdwFail( ws::WE_INVALID_PARAMETER, "Item is not a regular file in the repository" );

                // libgit2 cannot run a filter driver such as git-lfs, so the bytes would not be what git stores
                if( std::string driver = repo.FilterDriver( aRequest.item_id() ); !driver.empty() )
                    return filterDriverError( aRequest.item_id(), driver );

                WORKING_FILE_GUARD guard;

                // A refusal must leave nothing behind; one whose file cannot be put back goes to recovery
                auto refuse = [&guard]( ws::WorkspaceError aError )
                {
                    if( !guard.Restore() )
                        throw std::runtime_error( "a failed check-in could not restore the working file" );

                    return aError;
                };

                if( !aRequest.local_path().empty() )
                {
                    fs::path        source = MdwFsPath( aRequest.local_path() );
                    std::error_code ec;

                    if( !source.is_absolute() || !fs::is_regular_file( source, ec ) )
                        return MdwFail( ws::WE_INVALID_PARAMETER, "No file at " + aRequest.local_path() );

                    if( !fs::equivalent( source, inTree, ec ) )
                    {
                        if( MdwCurrentRequestCancelled() )
                            return MdwCancelled();

                        std::optional<std::string> installed = MDW_ReadFile( source );

                        if( !installed )
                            return MdwFail( ws::WE_INVALID_PARAMETER, "Cannot read " + aRequest.local_path() );

                        if( !guard.Arm( inTree, MdwFsPath( git_repository_path( repo.Handle() ) ), *installed ) )
                            return MdwFail( ws::WE_INTERNAL, "Cannot set aside " + aRequest.item_id() );

                        // The rename either landed whole or not at all, so a failure leaves nothing to restore
                        if( std::error_code failure = MDW_ReplaceFile( inTree, *installed ) )
                        {
                            guard.Keep();
                            return MdwFail( ws::WE_INTERNAL,
                                            "Cannot write " + aRequest.item_id() + ": " + failure.message() );
                        }
                    }
                }

                if( !repo.HasChanges( aRequest.item_id() ) )
                    return refuse( MdwFail( ws::WE_NO_CHANGES, "Nothing to commit" ) );

                const bool  blank = aRequest.comment().find_first_not_of( " \t\r\n" ) == std::string::npos;
                std::string message = blank ? "Check in " + aRequest.item_id() : aRequest.comment();

                // The trailer lands in the same commit, so a crash cannot separate it from the change
                if( !aRequest.idempotency_id().empty() )
                    message += "\n\n" + std::string( OPERATION_TRAILER ) + ": " + operationValue( aRequest );

                std::string oid;
                std::string error;

                const GIT_IDENTITY author = identity( repo );

                switch( repo.CommitFile( aRequest.item_id(), message, author, MdwCurrentRequestCancelled, oid, error ) )
                {
                case MDW_GIT_REPO::COMMIT_RESULT::OK: break;

                case MDW_GIT_REPO::COMMIT_RESULT::NO_CHANGES:
                    return refuse( MdwFail( ws::WE_NO_CHANGES, "Nothing to commit" ) );

                case MDW_GIT_REPO::COMMIT_RESULT::BUSY:
                    return refuse( MdwFail( ws::WE_BUSY, "Another git operation is running; try again" ) );

                case MDW_GIT_REPO::COMMIT_RESULT::CANCELLED: return refuse( MdwCancelled() );

                case MDW_GIT_REPO::COMMIT_RESULT::OPERATION_IN_PROGRESS:
                    return refuse( MdwFail( ws::WE_VERSION_CONFLICT, error ) );

                case MDW_GIT_REPO::COMMIT_RESULT::FAILED: return refuse( MdwFail( ws::WE_INTERNAL, error ) );
                }

                guard.Keep();

                ws::Revision* revision = aReply.mutable_revision();
                revision->set_revision_id( oid );
                revision->set_label( oid.substr( 0, 8 ) );
                revision->set_kind( ws::RK_CHECKIN );
                revision->set_author( MdwValidUtf8( author.SignedName() ) );
                revision->set_comment( aRequest.comment() );
                revision->mutable_state()->set_category( ws::LC_EDITABLE );
                return MdwOk();
            } ) );

    server.RegisterRecovery<mdw::CheckinRequest, mdw::CheckinReply>(
            withRepo(
                    [&repo]( const mdw::CheckinRequest&, std::string& aToken )
                    {
                        // The branch is recorded too, so a retry after switching branches searches where it committed
                        aToken = GetGitBackend()->HeadTarget( repo.Handle() ) + "\n" + repo.HeadOid();
                        return MdwOk();
                    } ),
            [this, &repo]( const mdw::CheckinRequest& aRequest, const std::string& aToken, mdw::CheckinReply& aReply,
                           ws::WorkspaceError& aError )
            {
                std::lock_guard<std::mutex> lock( m_repoMutex );

                auto unresolved = [&aError]()
                {
                    aError = MdwFail( ws::WE_VERSION_CONFLICT, "An interrupted check-in needs reconciliation" );
                    return MDW_RECOVERY::FAIL;
                };

                if( !ensureOpen() )
                    return unresolved();

                size_t            split = aToken.find( '\n' );
                std::string       branch = aToken.substr( 0, split );
                std::string       start = split == std::string::npos ? std::string() : aToken.substr( split + 1 );
                const std::string wanted = operationValue( aRequest );
                const std::string prefix = wanted.substr( 0, wanted.rfind( '-' ) + 1 );
                std::string       found;
                bool              cancelled = false;
                bool              reused = false;

                if( branch.empty() )
                    return unresolved();

                auto visit = [&]( const KIGIT_COMMIT_RECORD& aCommit )
                {
                    if( MdwCurrentRequestCancelled() )
                    {
                        cancelled = true;
                        return false;
                    }

                    for( const auto& [key, value] : KIGIT::MessageTrailers( aCommit.message ) )
                    {
                        if( key != OPERATION_TRAILER )
                            continue;

                        // Only a parsed trailer counts, so quoted message text never matches
                        if( value == wanted )
                        {
                            if( found.empty() )
                                found = aCommit.oid;
                        }
                        else if( value.compare( 0, prefix.size(), prefix ) == 0 )
                        {
                            reused = true;
                            return false;
                        }
                    }

                    return true;
                };

                // A walk that fails part way still proves whatever commit it found
                GetGitBackend()->WalkCommits( repo.Handle(), branch, start, visit );

                if( cancelled )
                {
                    aError = MdwCancelled();
                    return MDW_RECOVERY::FAIL;
                }

                if( reused )
                {
                    aError = MdwFail( ws::WE_INVALID_PARAMETER,
                                      "idempotency_id was already used for a different request" );
                    return MDW_RECOVERY::FAIL;
                }

                // No commit proves nothing ran, since the attempt may already have replaced the working file
                if( found.empty() )
                    return unresolved();

                aReply.mutable_revision()->set_revision_id( found );
                aReply.mutable_revision()->set_label( found.substr( 0, 8 ) );
                aReply.mutable_revision()->set_kind( ws::RK_CHECKIN );
                aReply.mutable_revision()->set_comment( aRequest.comment() );
                aReply.mutable_revision()->mutable_state()->set_category( ws::LC_EDITABLE );
                return MDW_RECOVERY::REPLAY;
            } );

    server.RegisterHandler<mdw::CancelCheckoutRequest, mdw::CancelCheckoutReply>( withRepo( echoes(
            [this, &repo]( const mdw::CancelCheckoutRequest& aRequest, mdw::CancelCheckoutReply& )
            {
                auto check = [&repo, &aRequest]()
                {
                    if( !repo.IsTracked( aRequest.item_id() ) )
                        return MdwFail( ws::WE_ITEM_NOT_FOUND, "No such tracked file" );

                    // A deleted file is restored, but a linked or odd path never is
                    if( MdwLinkFreePath( repo.Root(), aRequest.item_id(), true ).empty() )
                        return MdwFail( ws::WE_INVALID_PARAMETER, "Item is not a regular file in the repository" );

                    // libgit2 cannot run a filter driver such as git-lfs, so the bytes would not be what git stores
                    if( std::string driver = repo.FilterDriver( aRequest.item_id() ); !driver.empty() )
                        return filterDriverError( aRequest.item_id(), driver );

                    return MdwOk();
                };

                // Released before restoring, so a failed release leaves the edits alone. Only a claim
                // this repository recorded is released, which keeps an offline cancel offline
                bool released = false;

                // A branch switch changes what the checks answer, so they hold the repository, but claims
                // come before it, so it is let go across the release and the checks run again after
                {
                    std::unique_ptr<KIGIT::REPO_ACTION_LOCK> actions = KIGIT::RepoActionLock( repo.Handle() );

                    if( ws::WorkspaceError error = MdwGitLockActions( actions.get() ); error.code() != ws::WE_OK )
                        return error;

                    if( ws::WorkspaceError error = check(); error.code() != ws::WE_OK )
                        return error;

                    released = m_claims && m_claims->Holds( aRequest.item_id() );
                }

                if( released )
                {
                    if( MDW_GIT_CHECKOUTS::RESULT result = m_claims->Give( aRequest.item_id() );
                        result != MDW_GIT_CHECKOUTS::RESULT::OK )
                    {
                        return claimFailure( result );
                    }
                }

                // Past a release the request has an effect, so failing from here on is left to recovery
                auto refuse = [released]( const ws::WorkspaceError& aError )
                {
                    if( released )
                        throw std::runtime_error( "editing was given up but the file was not restored" );

                    return aError;
                };

                std::unique_ptr<KIGIT::REPO_ACTION_LOCK> actions = KIGIT::RepoActionLock( repo.Handle() );

                if( ws::WorkspaceError error = MdwGitLockActions( actions.get() ); error.code() != ws::WE_OK )
                    return refuse( error );

                if( ws::WorkspaceError error = check(); error.code() != ws::WE_OK )
                    return refuse( error );

                // Restoring from the index like git checkout -- path keeps whatever was already staged
                if( GetGitBackend()->CheckoutFromIndex( repo.Handle(), { aRequest.item_id() } ) != 0 )
                    return refuse( MdwFail( ws::WE_INTERNAL, MdwGitError() ) );

                return MdwOk();
            } ) ) );
}


void MDW_GIT_BACKEND::registerPermissionHandlers()
{
    m_server.RegisterHandler<mdw::GetPermissionsRequest, mdw::GetPermissionsReply>(
            [this]( const mdw::GetPermissionsRequest&, mdw::GetPermissionsReply& aReply )
            {
                const std::string locator = m_server.HelloContext().workspace().locator();

                if( locator.empty() )
                    return MdwFail( ws::WE_NOT_SUPPORTED, "Only a library source reports permissions" );

                for( const char* name : { "checkin", "release" } )
                    *aReply.add_permissions() = writePermission( name );

                return MdwOk();
            } );
}


ws::Permission MDW_GIT_BACKEND::writePermission( const std::string& aName )
{
    const std::string locator = m_server.HelloContext().workspace().locator();

    const MDW_GIT_PUSH_OUTCOMES::OUTCOME written =
            m_pushOutcomes.Get( MDW_GIT_SHARED_LIBRARIES::NormalisedLocator( locator ),
                                account( m_remote.Username() ).user_id(), m_remote.CredentialGeneration() );

    ws::Permission permission;
    permission.set_name( aName );
    permission.set_state( written.state );

    if( written.state == ws::PS_DENIED )
        permission.set_reason( "The library's server refused your last push" );
    else if( written.refusedBefore )
        permission.set_reason( "Your last check-in to this library was refused" );

    return permission;
}


void MDW_GIT_BACKEND::registerLifecycleHandlers()
{
    MDW_LIFECYCLE_HOOKS hooks;

    hooks.use = [this]( const MDW_LIFECYCLE_HOOKS::CALL& aCall )
    {
        std::lock_guard<std::mutex> lock( m_repoMutex );

        // A request queued behind another may have been cancelled while it waited
        if( MdwCurrentRequestCancelled() )
            return MdwCancelled();

        return aCall( m_lifecycle );
    };

    MdwServeLifecycle( m_server, hooks );
}


void MDW_GIT_BACKEND::registerRevisionHandlers()
{
    MDW_GIT_REPO& repo = m_repo;

    m_server.RegisterHandler<mdw::GetRevisionHistoryRequest, mdw::GetRevisionHistoryReply>( withRepo(
            [this, &repo]( const mdw::GetRevisionHistoryRequest& aRequest, mdw::GetRevisionHistoryReply& aReply )
            {
                if( isMember( aRequest.item_id() ) )
                {
                    std::vector<ws::Revision> releases;

                    if( ws::WorkspaceError error = m_libraries.MemberHistory( aRequest.item_id(), releases );
                        error.code() != ws::WE_OK )
                    {
                        return error;
                    }

                    for( ws::Revision& revision : releases )
                        *aReply.add_revisions() = std::move( revision );

                    return MdwOk();
                }

                if( !repo.IsTracked( aRequest.item_id() ) )
                    return MdwFail( ws::WE_ITEM_NOT_FOUND, "No such tracked file" );

                const bool                 project = isProjectItem( m_prefix, aRequest.item_id() );
                std::map<std::string, int> baselines;

                if( project )
                    baselines = m_lifecycle.Baselines( aRequest.item_id() );

                for( const MDW_GIT_VERSION& version :
                     repo.History( project ? m_prefix : aRequest.item_id(), MdwCurrentRequestCancelled ) )
                {
                    ws::Revision revision = revisionOf( version );

                    if( auto it = baselines.find( version.oid ); it != baselines.end() )
                    {
                        revision.set_kind( ws::RK_BASELINE );
                        revision.set_label( MdwGitBaselineLabel( it->second ) );
                        *revision.mutable_state() = MdwGitReleasedState();
                    }

                    *aReply.add_revisions() = std::move( revision );
                }

                if( MdwCurrentRequestCancelled() )
                    return MdwCancelled();

                return MdwOk();
            } ) );

    m_server.RegisterHandler<mdw::DownloadRevisionRequest, mdw::DownloadRevisionReply>(
            [this, &repo]( const mdw::DownloadRevisionRequest& aRequest, mdw::DownloadRevisionReply& aReply )
            {
                std::lock_guard<std::mutex> lock( m_repoMutex );
                const bool                  member = MDW_GIT_SHARED_LIBRARIES::IsMemberId( aRequest.item_id() );
                const bool                  opened = ensureOpen();

                // A library source downloads members from its cache before Materialize makes a working tree
                if( !opened && !member )
                    return MdwFail( ws::WE_ITEM_NOT_FOUND, "The workspace has no git working tree yet" );

                if( MdwCurrentRequestCancelled() )
                    return MdwCancelled();

                fs::path target = MdwFsPath( aRequest.local_path() );

                if( !target.is_absolute() )
                    return MdwFail( ws::WE_INVALID_PARAMETER, "Local path must be absolute" );

                std::unique_ptr<KIGIT::REPO_ACTION_LOCK> actions;

                if( ws::WorkspaceError error = lockDestination( target, actions ); error.code() != ws::WE_OK )
                    return error;

                std::optional<std::string> content;
                std::string                revision = aRequest.revision_id();

                if( member )
                {
                    ws::WorkspaceError error;
                    content = opened ? MDW_GIT_SHARED_LIBRARIES::MemberFile( repo.Handle(), aRequest.item_id(),
                                                                             revision, error )
                                     : m_libraries.SourceMemberFile( aRequest.item_id(), revision, error );

                    if( !content )
                        return error;
                }
                else
                {
                    if( revision.empty() )
                        revision = repo.HeadOid();

                    content = repo.ContentAt( revision, aRequest.item_id() );
                }

                if( !content )
                    return MdwFail( ws::WE_REVISION_NOT_FOUND, "No such revision of the item" );

                if( MdwCurrentRequestCancelled() )
                    return MdwCancelled();

                if( std::error_code failure = MDW_ReplaceFile( target, *content ) )
                    return MdwFail( ws::WE_INTERNAL,
                                    "Cannot write " + aRequest.local_path() + ": " + failure.message() );

                aReply.mutable_revision()->set_revision_id( revision );
                aReply.mutable_revision()->set_label( revision.substr( 0, 8 ) );

                return MdwOk();
            } );
}
