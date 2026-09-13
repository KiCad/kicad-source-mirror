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


#include "mdw_git_repo.h"
#include "mdw_git_json.h"

#include <git/git_commit_handler.h>
#include <git/git_config_handler.h>
#include <git/git_status_handler.h>
#include <git/kicad_git_common.h>
#include <git/kicad_git_memory.h>
#include <git/kigit_lock_retry.h>
#include <git/kigit_remote_url.h>
#include <git/project_git_utils.h>
#include <mdw_runtime/mdw_paths.h>

#include <api/workspace/workspace_types.pb.h>

#include <git2.h>

#include <algorithm>
#include <cstring>

namespace fs = std::filesystem;


namespace
{

// A git trailer like a check-in's KiCad-MDW-Operation, so it reads as metadata below any note a person wrote
const char RECORD_TRAILER[] = "KiCad-MDW-Record: ";

} // namespace


std::string MdwGitRecordMessage( const std::string& aSubject, const std::string& aBody, const nlohmann::json& aJson )
{
    std::string message = aSubject + "\n\n";

    if( !aBody.empty() )
        message += aBody + "\n\n";

    // JSON escapes a newline inside a string, so the record stays one line whatever it holds
    return message + RECORD_TRAILER + aJson.dump( -1, ' ', false, nlohmann::json::error_handler_t::replace ) + "\n";
}


nlohmann::json MdwGitRecordJson( const std::string& aMessage )
{
    // Found directly, since git's trailer parser stops at a "---" line a release note may hold
    size_t start = aMessage.rfind( std::string( "\n" ) + RECORD_TRAILER );

    if( start == std::string::npos )
        return nlohmann::json();

    start += 1 + std::strlen( RECORD_TRAILER );

    nlohmann::json record =
            nlohmann::json::parse( aMessage.substr( start, aMessage.find( '\n', start ) - start ), nullptr, false );

    return record.is_discarded() ? nlohmann::json() : record;
}


std::string MdwGitBaseName( const std::string& aPath )
{
    return aPath.substr( aPath.rfind( '/' ) + 1 );
}


int MdwGitCommittedBytes( git_repository* aRepo )
{
    git_config* config = nullptr;
    git_config* local = nullptr;

    if( int rc = git_repository_config( &config, aRepo ) )
        return rc;

    KIGIT::GitConfigPtr configPtr( config );

    if( int rc = git_config_open_level( &local, config, GIT_CONFIG_LEVEL_LOCAL ) )
        return rc;

    KIGIT::GitConfigPtr localPtr( local );

    if( int rc = git_config_set_bool( local, "core.autocrlf", false ) )
        return rc;

    if( int rc = git_config_set_string( local, "core.eol", "lf" ) )
        return rc;

    return git_repository_is_bare( aRepo )
                   ? GIT_OK
                   : git_config_set_string( local, "core.attributesfile", MDW_GIT_NO_USER_ATTRIBUTES );
}


void MDW_GIT_REPO::REPO_FREE::operator()( git_repository* aRepo ) const
{
    git_repository_free( aRepo );
}


MDW_GIT_REPO::MDW_GIT_REPO() = default;


MDW_GIT_REPO::~MDW_GIT_REPO() = default;


bool MDW_GIT_REPO::Open( const fs::path& aPath )
{
    std::u8string   path = aPath.u8string();
    git_repository* repo = GetGitBackend()->GetRepositoryForFile( reinterpret_cast<const char*>( path.c_str() ),
                                                                   nullptr, nullptr );

    m_common.reset();
    m_repo.reset( repo );

    const char* workdir = repo ? git_repository_workdir( repo ) : nullptr;

    if( !workdir )
    {
        m_repo.reset();
        return false;
    }

    m_common = std::make_unique<KIGIT_COMMON>( repo );
    m_root = MdwFsPath( workdir );
    return true;
}


std::optional<std::vector<std::string>> MDW_GIT_REPO::TrackedFiles()
{
    return GetGitBackend()->IndexedPaths( m_repo.get() );
}


bool MDW_GIT_REPO::IsTracked( const std::string& aPath )
{
    return MdwIsSafeRelativePath( aPath ) && m_repo
           && GetGitBackend()->LocatePaths( m_repo.get(), { aPath } ).front().inIndex;
}


std::map<std::string, MDW_GIT_VERSION> MDW_GIT_REPO::LastChanges( const std::vector<std::string>& aPaths,
                                                                  const MDW_GIT_CANCEL&           aCancelled )
{
    std::vector<std::string> safe;

    std::copy_if( aPaths.begin(), aPaths.end(), std::back_inserter( safe ), MdwIsSafeRelativePath );

    return GetGitBackend()->LastChanges( m_repo.get(), safe, aCancelled );
}


std::optional<MDW_GIT_VERSION> MDW_GIT_REPO::ScopeRevision( const std::string&    aScope,
                                                            const MDW_GIT_CANCEL& aCancelled )
{
    if( !aScope.empty() && !MdwIsSafeRelativePath( aScope ) )
        return std::nullopt;

    std::map<std::string, MDW_GIT_VERSION> newest =
            GetGitBackend()->LastChanges( m_repo.get(), { aScope }, aCancelled );

    if( newest.empty() )
        return std::nullopt;

    return newest.begin()->second;
}


std::vector<MDW_GIT_VERSION> MDW_GIT_REPO::History( const std::string& aPath, const MDW_GIT_CANCEL& aCancelled )
{
    if( !aPath.empty() && !MdwIsSafeRelativePath( aPath ) )
        return {};

    return GetGitBackend()->PathHistory( m_repo.get(), aPath, aCancelled );
}


std::optional<std::string> MDW_GIT_REPO::ContentAt( const std::string& aOid, const std::string& aPath )
{
    if( !MdwIsSafeRelativePath( aPath ) )
        return std::nullopt;

    std::optional<KIGIT_TREE_ENTRY> entry = GetGitBackend()->TreeEntryAt( m_repo.get(), aOid, aPath );

    if( !entry || entry->isTree )
        return std::nullopt;

    return GetGitBackend()->ReadBlob( m_repo.get(), entry->oid );
}


std::vector<MDW_GIT_REPO::PLACE> MDW_GIT_REPO::Locate( const std::vector<std::string>& aPaths )
{
    std::vector<PLACE>            places( aPaths.size(), PLACE::UNTRACKED );
    std::vector<KIGIT_PATH_PLACE> found = GetGitBackend()->LocatePaths( m_repo.get(), aPaths );

    for( size_t ii = 0; ii < aPaths.size() && ii < found.size(); ++ii )
    {
        if( !MdwIsSafeRelativePath( aPaths[ii] ) )
            continue;

        if( found[ii].inHead )
            places[ii] = PLACE::IN_HEAD;
        else if( found[ii].inIndex )
            places[ii] = PLACE::TRACKED;
    }

    return places;
}


std::optional<std::vector<MDW_GIT_CHANGE>> MDW_GIT_REPO::LocalChanges( const std::string& aPrefix )
{
    if( !m_common )
        return std::nullopt;

    GIT_STATUS_HANDLER             handler( m_common.get() );
    bool                           readOk = false;
    std::map<wxString, FileStatus> status =
            handler.GetFileStatus( wxString::FromUTF8( aPrefix ), true, &readOk );

    if( !readOk )
        return std::nullopt;

    const wxString              workdir = m_common->GetProjectDir();
    const std::string           scope = aPrefix.empty() ? std::string() : aPrefix + "/";
    const wxString              projectPrefix = wxString::FromUTF8( scope );
    std::vector<MDW_GIT_CHANGE> changes;

    for( const auto& [absPath, fileStatus] : status )
    {
        const unsigned int flags = fileStatus.gitStatus;
        const wxString     relative = absPath.Mid( workdir.length() );
        const std::string  path = relative.utf8_string();

        if( flags == GIT_STATUS_CURRENT || ( flags & GIT_STATUS_IGNORED ) )
            continue;

        if( !scope.empty() && std::strncmp( path.c_str(), scope.c_str(), scope.size() ) != 0 )
            continue;

        // One filter with the Version Control pane, so editor locks and backups never block a check-in
        if( KIGIT::PROJECT_GIT_UTILS::IsHiddenTransient( relative, projectPrefix, flags, fileStatus.isDirectory ) )
            continue;

        // WT_NEW means absent from the index, so absent from HEAD too unless the index deleted it
        const bool untracked = ( flags & GIT_STATUS_WT_NEW ) && !( flags & GIT_STATUS_INDEX_DELETED );
        const bool inHead = !untracked && !( flags & GIT_STATUS_INDEX_NEW );
        const bool inWorkdir = !( flags & GIT_STATUS_WT_DELETED )
                               && !( ( flags & GIT_STATUS_INDEX_DELETED ) && !( flags & GIT_STATUS_WT_NEW ) );

        MDW_GIT_CHANGE change;
        change.path = path;
        change.tracked = !untracked;

        if( inHead && inWorkdir )
            change.kind = MDW_GIT_CHANGE::KIND::MODIFIED;
        else if( inHead )
            change.kind = MDW_GIT_CHANGE::KIND::REMOVED;
        else if( inWorkdir )
            change.kind = MDW_GIT_CHANGE::KIND::ADDED;
        else
            continue;

        changes.push_back( std::move( change ) );
    }

    std::sort( changes.begin(), changes.end(),
               []( const MDW_GIT_CHANGE& aLeft, const MDW_GIT_CHANGE& aRight )
               {
                   return aLeft.path < aRight.path;
               } );

    return changes;
}


bool MDW_GIT_REPO::HasChanges( const std::string& aPath )
{
    if( !m_common )
        return true;

    GIT_STATUS_HANDLER             handler( m_common.get() );
    bool                           readOk = false;
    std::map<wxString, FileStatus> status = handler.GetFileStatus( wxString::FromUTF8( aPath ), true, &readOk );

    return !readOk || !status.empty();
}


std::string MDW_GIT_REPO::HeadOid() const
{
    return m_common ? m_common->GetHeadCommitId().utf8_string() : std::string();
}


std::string MDW_GIT_REPO::RemoteName() const
{
    return m_common ? m_common->GetRemoteNameOrDefault().utf8_string() : std::string( "origin" );
}


bool MDW_GIT_REPO::HasRemote() const
{
    if( !m_common )
        return false;

    const wxString                                   name = m_common->GetRemoteNameOrDefault();
    const std::vector<std::pair<wxString, wxString>> remotes = m_common->GetRemotes();

    return std::any_of( remotes.begin(), remotes.end(),
                        [&name]( const std::pair<wxString, wxString>& aRemote )
                        {
                            return aRemote.first == name;
                        } );
}


std::string MDW_GIT_REPO::RemoteUrl() const
{
    git_remote* remote = nullptr;

    if( !m_repo || git_remote_lookup( &remote, m_repo.get(), RemoteName().c_str() ) != GIT_OK )
        return std::string();

    KIGIT::GitRemotePtr remotePtr( remote );
    const char*         url = git_remote_url( remote );

    return url ? std::string( url ) : std::string();
}


std::string MDW_GIT_REPO::FilterDriver( const std::string& aPath ) const
{
    std::optional<wxString> driver =
            KIGIT::PROJECT_GIT_UTILS::FilterDriver( m_repo.get(), wxString::FromUTF8( aPath ) );

    return driver ? driver->utf8_string() : std::string( "unreadable" );
}


GIT_IDENTITY MdwGitIdentity( git_repository* aRepo, const kiapi::workspace::Account& aHello,
                             const wxString& aSignedInUser )
{
    GitUserConfig local;
    GitUserConfig global;
    KIGIT::ReadAuthorConfig( aRepo, local, global );

    // Hello carries the preference already resolved, so it takes the preference's place
    COMMON_SETTINGS::GIT hello{};
    hello.useDefaultAuthor = false;
    hello.authorName = wxString::FromUTF8( aHello.display_name() );
    hello.authorEmail = wxString::FromUTF8( aHello.email() );

    wxString user = aSignedInUser;

    // A blank value falls through to the next source, since a signature refuses it
    for( wxString* value : { &local.authorName, &local.authorEmail, &global.authorName, &global.authorEmail,
                             &hello.authorName, &hello.authorEmail, &user } )
    {
        value->Trim( true ).Trim( false );
    }

    const GitUserConfig author = KIGIT::ResolveAuthor( hello, local, global, user );

    return { author.authorName.utf8_string(), author.authorEmail.utf8_string() };
}


std::string MdwGitOwnerId( const std::string& aUsername, const std::string& aRemoteUrl, const std::string& aEmail )
{
    if( aUsername.empty() )
        return aEmail;

    const wxString url = wxString::FromUTF8( aRemoteUrl );
    wxString       realm = KIGIT::CredentialRealm( url );

    // An SSH remote has no realm, so its server stands in
    if( realm.IsEmpty() )
        realm = KIGIT::CredentialServer( url );

    return realm.IsEmpty() ? aUsername : aUsername + "@" + realm.utf8_string();
}


MDW_GIT_REPO::COMMIT_RESULT MDW_GIT_REPO::CommitFile( const std::string& aPath, const std::string& aMessage,
                                                      const GIT_IDENTITY& aAuthor, const MDW_GIT_CANCEL& aCancelled,
                                                      std::string& aOid, std::string& aError )
{
    if( !m_common || !MdwIsSafeRelativePath( aPath ) )
    {
        aError = "no repository";
        return COMMIT_RESULT::FAILED;
    }

    // The commit itself refuses too, but this names the case for the caller
    if( m_common->IsOperationInProgress() )
    {
        aError = "finish the merge, rebase or other git operation in progress before checking in";
        return COMMIT_RESULT::OPERATION_IN_PROGRESS;
    }

    if( !IsTracked( aPath ) )
    {
        aError = aPath + " is not tracked";
        return COMMIT_RESULT::FAILED;
    }

    GIT_COMMIT_HANDLER handler( m_repo.get() );

    handler.SetSkipUnchanged( true );
    handler.SetCancelCheck( aCancelled );
    handler.SetLockRetry( KIGIT::BACKEND_LOCK_BUDGET );

    CommitResult result = handler.PerformCommit( { wxString::FromUTF8( aPath ) }, wxString::FromUTF8( aMessage ),
                                                 wxString::FromUTF8( aAuthor.SignedName() ),
                                                 wxString::FromUTF8( aAuthor.SignedEmail() ) );

    switch( result )
    {
    case CommitResult::Success:
    case CommitResult::SuccessIndexStale:
        // The commit stands either way; a stale index entry only shows the path as staged in reverse
        aOid = handler.CreatedCommit();
        return COMMIT_RESULT::OK;

    case CommitResult::Unchanged: return COMMIT_RESULT::NO_CHANGES;
    case CommitResult::Busy:      return COMMIT_RESULT::BUSY;

    case CommitResult::Cancelled:
        aError = "cancelled";
        return COMMIT_RESULT::CANCELLED;

    case CommitResult::Error: break;
    }

    KIGIT_ERROR_MESSAGE error = handler.PopError();

    aError = ( error.details.IsEmpty() ? error.summary : error.summary + wxS( " " ) + error.details ).utf8_string();

    if( aError.empty() )
        aError = "the commit failed";

    return COMMIT_RESULT::FAILED;
}
