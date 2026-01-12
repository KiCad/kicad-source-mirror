/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "kicad_git_common.h"
#include "kicad_git_memory.h"
#include "git_repo_mixin.h"

#include <git/git_progress.h>
#include <git/kicad_git_compat.h>
#include <kiplatform/secrets.h>
#include <trace_helpers.h>

#include <git2.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/textfile.h>
#include <wx/utils.h>
#include <map>
#include <vector>

KIGIT_COMMON::KIGIT_COMMON( git_repository* aRepo ) :
        m_repo( aRepo ), m_connType( GIT_CONN_TYPE::GIT_CONN_LOCAL ), m_testedTypes( 0 ),
        m_nextPublicKey( 0 ), m_secretFetched( false )
{}

KIGIT_COMMON::KIGIT_COMMON( const KIGIT_COMMON& aOther ) :
        // Initialize base class and member variables
        m_repo( aOther.m_repo ),
        m_projectDir( aOther.m_projectDir ),
        m_connType( aOther.m_connType ),
        m_remote( aOther.m_remote ),
        m_hostname( aOther.m_hostname ),
        m_username( aOther.m_username ),
        m_password( aOther.m_password ),
        m_testedTypes( aOther.m_testedTypes ),
        // The mutex is default-initialized, not copied
        m_gitActionMutex(),
        m_publicKeys( aOther.m_publicKeys ),
        m_nextPublicKey( aOther.m_nextPublicKey ),
        m_secretFetched( aOther.m_secretFetched )
{
}

KIGIT_COMMON::~KIGIT_COMMON()
{}


git_repository* KIGIT_COMMON::GetRepo() const
{
    return m_repo;
}


wxString KIGIT_COMMON::GetProjectDir() const
{
    if( !m_projectDir.IsEmpty() )
        return m_projectDir;

    if( m_repo )
    {
        const char* workdir = git_repository_workdir( m_repo );

        if( workdir )
            return wxString( workdir );
    }

    return wxEmptyString;
}


wxString KIGIT_COMMON::GetCurrentBranchName() const
{
    wxCHECK( m_repo, wxEmptyString );
    git_reference* head = nullptr;

    int retval = git_repository_head( &head, m_repo );

    if( retval && retval != GIT_EUNBORNBRANCH && retval != GIT_ENOTFOUND )
        return wxEmptyString;

    KIGIT::GitReferencePtr headPtr( head );
    git_reference*         branch;

    if( git_reference_resolve( &branch, head ) )
    {
        wxLogTrace( traceGit, "Failed to resolve branch" );
        return wxEmptyString;
    }

    KIGIT::GitReferencePtr branchPtr( branch );
    const char*            branchName = "";

    if( git_branch_name( &branchName, branch ) )
    {
        wxLogTrace( traceGit, "Failed to get branch name" );
        return wxEmptyString;
    }

    return wxString( branchName );
}


wxString KIGIT_COMMON::GetPassword()
{
    if( !m_secretFetched )
    {
        if( m_connType != GIT_CONN_TYPE::GIT_CONN_LOCAL && !m_remote.IsEmpty() )
        {
            wxString secret;

            if( KIPLATFORM::SECRETS::GetSecret( m_remote, m_username, secret ) )
                m_password = secret;
        }

        m_secretFetched = true;
    }

    return m_password;
}


std::vector<wxString> KIGIT_COMMON::GetBranchNames() const
{
    if( !m_repo )
        return {};

    std::vector<wxString> branchNames;
    std::map<git_time_t, wxString> branchNamesMap;
    wxString firstName;

    git_branch_iterator* branchIterator = nullptr;

    if( git_branch_iterator_new( &branchIterator, m_repo, GIT_BRANCH_LOCAL ) )
    {
        wxLogTrace( traceGit, "Failed to get branch iterator" );
        return branchNames;
    }

    KIGIT::GitBranchIteratorPtr branchIteratorPtr( branchIterator );
    git_reference*              branchReference = nullptr;
    git_branch_t                branchType;

    while( git_branch_next( &branchReference, &branchType, branchIterator ) != GIT_ITEROVER )
    {
        const char* branchName = "";
        KIGIT::GitReferencePtr branchReferencePtr( branchReference );

        if( git_branch_name( &branchName, branchReference ) )
        {
            wxLogTrace( traceGit, "Failed to get branch name in iter loop" );
            continue;
        }

        const git_oid* commitId = git_reference_target( branchReference );

        git_commit* commit = nullptr;

        if( git_commit_lookup( &commit, m_repo, commitId ) )
        {
            wxLogTrace( traceGit, "Failed to get commit in iter loop" );
            continue;
        }

        KIGIT::GitCommitPtr commitPtr( commit );
        git_time_t          commitTime = git_commit_time( commit );

        if( git_branch_is_head( branchReference ) )
            firstName = branchName;
        else
            branchNamesMap.emplace( commitTime, branchName );
    }

    // Add the current branch to the top of the list
    if( !firstName.IsEmpty() )
        branchNames.push_back( firstName );

    // Add the remaining branches in order from newest to oldest
    for( auto rit = branchNamesMap.rbegin(); rit != branchNamesMap.rend(); ++rit )
        branchNames.push_back( rit->second );

    return branchNames;
}


std::vector<wxString> KIGIT_COMMON::GetProjectDirs()
{
    wxCHECK( m_repo, {} );
    std::vector<wxString> projDirs;

    git_oid oid;
    git_commit* commit;
    git_tree *tree;

    if( git_reference_name_to_id( &oid, m_repo, "HEAD" ) != GIT_OK )
    {
        wxLogTrace( traceGit, "An error occurred: %s", KIGIT_COMMON::GetLastGitError() );
        return projDirs;
    }

    if( git_commit_lookup( &commit, m_repo, &oid ) != GIT_OK )
    {
        wxLogTrace( traceGit, "An error occurred: %s", KIGIT_COMMON::GetLastGitError() );
        return projDirs;
    }

    KIGIT::GitCommitPtr commitPtr( commit );

    if( git_commit_tree( &tree, commit ) != GIT_OK )
    {
        wxLogTrace( traceGit, "An error occurred: %s", KIGIT_COMMON::GetLastGitError() );
        return projDirs;
    }

    KIGIT::GitTreePtr treePtr( tree );

    // Define callback
    git_tree_walk(
            tree, GIT_TREEWALK_PRE,
            []( const char* root, const git_tree_entry* entry, void* payload )
            {
                std::vector<wxString>* prjs = static_cast<std::vector<wxString>*>( payload );
                wxFileName             root_fn( git_tree_entry_name( entry ) );

                root_fn.SetPath( root );

                if( git_tree_entry_type( entry ) == GIT_OBJECT_BLOB
                    && ( ( root_fn.GetExt() == "kicad_pro" ) || ( root_fn.GetExt() == "pro" ) ) )
                {
                    prjs->push_back( root_fn.GetFullPath() );
                }

                return 0; // continue walking
            },
            &projDirs );

    std::sort( projDirs.begin(), projDirs.end(),
               []( const wxString& a, const wxString& b )
               {
                    int a_freq = a.Freq( wxFileName::GetPathSeparator() );
                    int b_freq = b.Freq( wxFileName::GetPathSeparator() );

                    if( a_freq == b_freq )
                        return a < b;
                    else
                        return a_freq < b_freq;

               } );

    return projDirs;
}


std::pair<std::set<wxString>,std::set<wxString>> KIGIT_COMMON::GetDifferentFiles() const
{
    auto get_modified_files = [&]( const git_oid* from_oid, const git_oid* to_oid ) -> std::set<wxString>
    {
        std::set<wxString> modified_set;
        git_revwalk* walker = nullptr;

        if( !m_repo || !from_oid || IsCancelled() )
            return modified_set;

        if( git_revwalk_new( &walker, m_repo ) != GIT_OK )
        {
            wxLogTrace( traceGit, "Failed to create revwalker: %s", KIGIT_COMMON::GetLastGitError() );
            return modified_set;
        }

        KIGIT::GitRevWalkPtr walkerPtr( walker );

        if( git_revwalk_push( walker, from_oid ) != GIT_OK )
        {
            wxLogTrace( traceGit, "Failed to set from commit: %s", KIGIT_COMMON::GetLastGitError() );
            return modified_set;
        }

        if( to_oid && git_revwalk_hide( walker, to_oid ) != GIT_OK )
        {
            wxLogTrace( traceGit, "Failed to set end commit (maybe new repo): %s", KIGIT_COMMON::GetLastGitError() );
        }

        git_oid     oid;
        git_commit* commit;

        // iterate over all local commits not in remote
        while( git_revwalk_next( &oid, walker ) == GIT_OK )
        {
            // Check for cancellation to allow early exit during shutdown
            if( IsCancelled() )
                return modified_set;

            if( git_commit_lookup( &commit, m_repo, &oid ) != GIT_OK )
            {
                wxLogTrace( traceGit, "Failed to lookup commit: %s", KIGIT_COMMON::GetLastGitError() );
                continue;
            }

            KIGIT::GitCommitPtr commitPtr( commit );
            git_tree *tree, *parent_tree = nullptr;

            if( git_commit_tree( &tree, commit ) != GIT_OK )
            {
                wxLogTrace( traceGit, "Failed to get commit tree: %s", KIGIT_COMMON::GetLastGitError() );
                continue;
            }

            KIGIT::GitTreePtr treePtr( tree );

            // get parent commit tree to diff against
            if( !git_commit_parentcount( commit ) )
            {
                git_tree_walk(
                    tree, GIT_TREEWALK_PRE,
                    []( const char* root, const git_tree_entry* entry, void* payload )
                    {
                        std::set<wxString>* modified_set_internal = static_cast<std::set<wxString>*>( payload );
                        wxString filePath = wxString::Format( "%s%s", root, git_tree_entry_name( entry ) );
                        modified_set_internal->insert( std::move( filePath ) );
                        return 0; // continue walking
                    },
                    &modified_set );
                continue;
            }

            git_commit* parent;

            if( git_commit_parent( &parent, commit, 0 ) != GIT_OK )
            {
                wxLogTrace( traceGit, "Failed to get parent commit: %s", KIGIT_COMMON::GetLastGitError() );
                continue;
            }

            KIGIT::GitCommitPtr parentPtr( parent );


            if( git_commit_tree( &parent_tree, parent ) != GIT_OK )
            {
                wxLogTrace( traceGit, "Failed to get parent commit tree: %s", KIGIT_COMMON::GetLastGitError() );
                continue;
            }

            KIGIT::GitTreePtr parentTreePtr( parent_tree );


            git_diff*        diff;
            git_diff_options diff_opts;
            git_diff_init_options( &diff_opts, GIT_DIFF_OPTIONS_VERSION );

            if( !m_repo || !parent_tree || !tree )
                continue;

            if( git_diff_tree_to_tree( &diff, m_repo, parent_tree, tree, &diff_opts ) == GIT_OK )
            {
                size_t num_deltas = git_diff_num_deltas( diff );

                for( size_t i = 0; i < num_deltas; ++i )
                {
                    const git_diff_delta* delta = git_diff_get_delta( diff, i );
                    modified_set.insert( delta->new_file.path );
                }

                git_diff_free( diff );
            }
            else
            {
                wxLogTrace( traceGit, "Failed to diff trees: %s", KIGIT_COMMON::GetLastGitError() );
            }
        }

        wxLogTrace( traceGit, "Finished walking commits with end: %s", KIGIT_COMMON::GetLastGitError() );

        return modified_set;
    };

    std::pair<std::set<wxString>,std::set<wxString>> modified_files;

    if( !m_repo || IsCancelled() )
        return modified_files;

    git_reference* head = nullptr;
    git_reference* remote_head = nullptr;

    if( git_repository_head( &head, m_repo ) != GIT_OK )
    {
        wxLogTrace( traceGit, "Failed to get modified HEAD" );
        return modified_files;
    }

    KIGIT::GitReferencePtr headPtr( head );

    if( git_branch_upstream( &remote_head, head ) != GIT_OK )
    {
        wxLogTrace( traceGit, "Failed to get modified remote HEAD" );
    }

    KIGIT::GitReferencePtr remoteHeadPtr( remote_head );

    if( remote_head != nullptr && head != nullptr )
    {
        const git_oid*         head_oid = git_reference_target( head );
        const git_oid*         remote_oid = git_reference_target( remote_head );

        modified_files.first = get_modified_files( head_oid, remote_oid );
        modified_files.second = get_modified_files( remote_oid, head_oid );
    }

    return modified_files;
}


bool KIGIT_COMMON::HasLocalCommits() const
{
    if( !m_repo )
        return false;

    git_reference* head = nullptr;
    git_reference* remote_head = nullptr;

    if( git_repository_head( &head, m_repo ) != GIT_OK )
    {
        wxLogTrace( traceGit, "Failed to get HEAD: %s", KIGIT_COMMON::GetLastGitError() );
        return false;
    }

    KIGIT::GitReferencePtr headPtr( head );

    if( git_branch_upstream( &remote_head, head ) != GIT_OK )
    {
        // No remote branch, so we have local commits (new repo?)
        wxLogTrace( traceGit, "Failed to get remote HEAD: %s", KIGIT_COMMON::GetLastGitError() );
        return true;
    }

    KIGIT::GitReferencePtr remoteHeadPtr( remote_head );
    const git_oid*         head_oid = git_reference_target( head );
    const git_oid*         remote_oid = git_reference_target( remote_head );
    git_revwalk*           walker = nullptr;

    if( git_revwalk_new( &walker, m_repo ) != GIT_OK )
    {
        wxLogTrace( traceGit, "Failed to create revwalker: %s", KIGIT_COMMON::GetLastGitError() );
        return false;
    }

    KIGIT::GitRevWalkPtr walkerPtr( walker );

    if( !head_oid || git_revwalk_push( walker, head_oid ) != GIT_OK )
    {
        wxLogTrace( traceGit, "Failed to push commits: %s", KIGIT_COMMON::GetLastGitError() );
        return false;
    }

    if( remote_oid && git_revwalk_hide( walker, remote_oid ) != GIT_OK )
    {
        wxLogTrace( traceGit, "Failed to push/hide commits: %s", KIGIT_COMMON::GetLastGitError() );
        return false;
    }

    git_oid oid;

    // If we can't walk to the next commit, then we are at or behind the remote
    if( git_revwalk_next( &oid, walker ) != GIT_OK )
    {
        wxLogTrace( traceGit, "Failed to walk to next commit: %s", KIGIT_COMMON::GetLastGitError() );
        return false;
    }

    return true;
}


bool KIGIT_COMMON::HasPushAndPullRemote() const
{
    wxCHECK( m_repo, false );
    git_remote* remote = nullptr;

    if( git_remote_lookup( &remote, m_repo, "origin" ) != GIT_OK )
    {
        wxLogTrace( traceGit, "Failed to get remote for haspushpull" );
        return false;
    }

    KIGIT::GitRemotePtr remotePtr( remote );

    // Get the URLs associated with the remote
    const char* fetch_url = git_remote_url( remote );
    const char* push_url = git_remote_pushurl( remote );

    // If no push URL is set, libgit2 defaults to using the fetch URL for pushing
    if( !push_url )
    {
        wxLogTrace( traceGit, "No push URL set, using fetch URL" );
        push_url = fetch_url;
    }

    return fetch_url && push_url;
}


wxString KIGIT_COMMON::GetRemotename() const
{
    wxCHECK( m_repo, wxEmptyString );

    wxString retval;
    git_reference* head = nullptr;
    git_reference* upstream = nullptr;

    if( git_repository_head( &head, m_repo ) != GIT_OK )
    {
        wxLogTrace( traceGit, "Failed to get remote name: %s", KIGIT_COMMON::GetLastGitError() );
        return retval;
    }

    KIGIT::GitReferencePtr headPtr( head );

    if( git_branch_upstream( &upstream, head ) != GIT_OK )
    {
        wxLogTrace( traceGit, "Failed to get upstream branch: %s", KIGIT_COMMON::GetLastGitError() );
        git_strarray remotes = { nullptr, 0 };

        if( git_remote_list( &remotes, m_repo ) == GIT_OK )
        {
            if( remotes.count == 1 )
                retval = remotes.strings[0];

            git_strarray_dispose( &remotes );
        }
        else
        {
            wxLogTrace( traceGit, "Failed to list remotes: %s", KIGIT_COMMON::GetLastGitError() );

            // If we can't get the remote name from the upstream branch or the list of remotes,
            // just return the default remote name

            git_remote* remote = nullptr;

            if( git_remote_lookup( &remote, m_repo, "origin" ) == GIT_OK )
            {
                retval = git_remote_name( remote );
                git_remote_free( remote );
            }
            else
            {
                wxLogTrace( traceGit, "Failed to get remote name from default remote: %s",
                            KIGIT_COMMON::GetLastGitError() );
            }
        }

        return retval;
    }

    KIGIT::GitReferencePtr upstreamPtr( upstream );
    git_buf     remote_name = GIT_BUF_INIT_CONST( nullptr, 0 );

    if( git_branch_remote_name( &remote_name, m_repo, git_reference_name( upstream ) ) == GIT_OK )
    {
        retval = remote_name.ptr;
        git_buf_dispose( &remote_name );
    }
    else
    {
        wxLogTrace( traceGit,
                    "Failed to get remote name from upstream branch: %s",
                    KIGIT_COMMON::GetLastGitError() );
    }

    return retval;
}

void KIGIT_COMMON::SetSSHKey( const wxString& aKey )
{
    auto it = std::find( m_publicKeys.begin(), m_publicKeys.end(), aKey );

    if( it != m_publicKeys.end() )
        m_publicKeys.erase( it );

    m_publicKeys.insert( m_publicKeys.begin(), aKey );
}


wxString KIGIT_COMMON::GetGitRootDirectory() const
{
    if( !m_repo )
        return wxEmptyString;

    const char *path = git_repository_path( m_repo );
    wxString    retval = path;
    return retval;
}


void KIGIT_COMMON::updatePublicKeys()
{
    m_publicKeys.clear();

    wxFileName keyFile( wxGetHomeDir(), wxEmptyString );
    keyFile.AppendDir( ".ssh" );
    keyFile.SetFullName( "id_rsa" );

    if( keyFile.FileExists() )
        m_publicKeys.push_back( keyFile.GetFullPath() );

    keyFile.SetFullName( "id_dsa" );

    if( keyFile.FileExists() )
        m_publicKeys.push_back( keyFile.GetFullPath() );

    keyFile.SetFullName( "id_ecdsa" );

    if( keyFile.FileExists() )
        m_publicKeys.push_back( keyFile.GetFullPath() );

    keyFile.SetFullName( "id_ed25519" );

    if( keyFile.FileExists() )
        m_publicKeys.push_back( keyFile.GetFullPath() );

    // Parse SSH config file for hostname information
    wxFileName sshConfig( wxGetHomeDir(), wxEmptyString );
    sshConfig.AppendDir( ".ssh" );
    sshConfig.SetFullName( "config" );

    if( sshConfig.FileExists() )
    {
        wxTextFile configFile( sshConfig.GetFullPath() );
        configFile.Open();

        bool match = false;

        for( wxString line = configFile.GetFirstLine(); !configFile.Eof(); line = configFile.GetNextLine() )
        {
            line.Trim( false ).Trim( true );

            if( line.StartsWith( "Host " ) )
                match = false;

            // The difference here is that we are matching either "Hostname" or "Host" to get the
            // match.  This is because in the absence of a "Hostname" line, the "Host" line is used
            if( line.StartsWith( "Host" ) && line.Contains( m_hostname ) )
                match = true;

            if( match && line.StartsWith( "IdentityFile" ) )
            {
                wxString keyPath = line.AfterFirst( ' ' ).Trim( false ).Trim( true );

                // Expand ~ to home directory if present
                if( keyPath.StartsWith( "~" ) )
                    keyPath.Replace( "~", wxGetHomeDir(), false );

                // Add the public key to the beginning of the list
                if( wxFileName::FileExists( keyPath ) )
                    SetSSHKey( keyPath );
            }
        }

        configFile.Close();
    }
}


void KIGIT_COMMON::UpdateCurrentBranchInfo()
{
    wxCHECK( m_repo, /* void */ );

    // We want to get the current branch's upstream url as well as the stored password
    // if one exists given the url and username.

    wxString remote_name = GetRemotename();
    git_remote* remote = nullptr;

    m_remote.clear();
    m_password.clear();
    m_secretFetched = false;

    if( git_remote_lookup( &remote, m_repo, remote_name.ToStdString().c_str() ) == GIT_OK )
    {
        const char* url = git_remote_url( remote );

        if( url )
            m_remote = url;

        git_remote_free( remote );
    }

    updateConnectionType();
    updatePublicKeys();
}

KIGIT_COMMON::GIT_CONN_TYPE KIGIT_COMMON::GetConnType() const
{
    wxString remote = m_remote;

    if( remote.IsEmpty() )
        remote = GetRemotename();

    if( remote.StartsWith( "https://" ) || remote.StartsWith( "http://" ) )
    {
        return GIT_CONN_TYPE::GIT_CONN_HTTPS;
    }
    else if( remote.StartsWith( "ssh://" ) || remote.StartsWith( "git@" ) || remote.StartsWith( "git+ssh://" )
             || remote.EndsWith( ".git" ) )
    {
        return GIT_CONN_TYPE::GIT_CONN_SSH;
    }

    return GIT_CONN_TYPE::GIT_CONN_LOCAL;
}

void KIGIT_COMMON::updateConnectionType()
{
    if( m_remote.StartsWith( "https://" ) || m_remote.StartsWith( "http://" ) )
        m_connType = GIT_CONN_TYPE::GIT_CONN_HTTPS;
    else if( m_remote.StartsWith( "ssh://" ) || m_remote.StartsWith( "git@" ) || m_remote.StartsWith( "git+ssh://" ) )
        m_connType = GIT_CONN_TYPE::GIT_CONN_SSH;
    else
        m_connType = GIT_CONN_TYPE::GIT_CONN_LOCAL;

    if( m_connType != GIT_CONN_TYPE::GIT_CONN_LOCAL )
    {
        wxString uri = m_remote;
        size_t atPos = uri.find( '@' );

        if( atPos != wxString::npos )
        {
            size_t protoEnd = uri.find( "//" );

            if( protoEnd != wxString::npos )
            {
                wxString credentials = uri.Mid( protoEnd + 2, atPos - protoEnd - 2 );
                size_t colonPos = credentials.find( ':' );

                if( colonPos != wxString::npos )
                {
                    m_username = credentials.Left( colonPos );
                    m_password = credentials.Mid( colonPos + 1, credentials.Length() - colonPos - 1 );
                }
                else
                {
                    m_username = credentials;
                }
            }
            else
            {
                m_username = uri.Left( atPos );
            }
        }

        if( m_remote.StartsWith( "git@" ) )
        {
            // SSH format: git@hostname:path
            size_t colonPos = m_remote.find( ':' );
            if( colonPos != wxString::npos )
                m_hostname = m_remote.Mid( 4, colonPos - 4 );
        }
        else
        {
            // other URL format: proto://[user@]hostname/path
            size_t hostStart = m_remote.find( "://" ) + 2;
            size_t hostEnd = m_remote.find( '/', hostStart );
            wxString host;

            if( hostEnd != wxString::npos )
                host = m_remote.Mid( hostStart, hostEnd - hostStart );
            else
                host = m_remote.Mid( hostStart );

            atPos = host.find( '@' );

            if( atPos != wxString::npos )
                m_hostname = host.Mid( atPos + 1 );
            else
                m_hostname = host;
        }
    }

    m_secretFetched = !m_password.IsEmpty();
}


int KIGIT_COMMON::HandleSSHKeyAuthentication( git_cred** aOut, const wxString& aUsername )
{
    if( !( m_testedTypes & KIGIT_CREDENTIAL_SSH_AGENT ) )
        return HandleSSHAgentAuthentication( aOut, aUsername );

    // SSH key authentication with password
    wxString sshKey = GetNextPublicKey();

    if( sshKey.IsEmpty() )
    {
        wxLogTrace( traceGit, "Finished testing all possible ssh keys" );
        m_testedTypes |= GIT_CREDENTIAL_SSH_KEY;
        return GIT_PASSTHROUGH;
    }

    wxString sshPubKey = sshKey + ".pub";
    wxString password = GetPassword();

    wxLogTrace( traceGit, "Testing %s\n", sshKey );

    if( git_credential_ssh_key_new( aOut, aUsername.mbc_str(), sshPubKey.mbc_str(), sshKey.mbc_str(),
                                password.mbc_str() ) != GIT_OK )
    {
        wxLogTrace( traceGit, "Failed to create SSH key credential for %s: %s",
                    aUsername, KIGIT_COMMON::GetLastGitError() );
        return GIT_PASSTHROUGH;
    }

    return GIT_OK;
}


int KIGIT_COMMON::HandlePlaintextAuthentication( git_cred** aOut, const wxString& aUsername )
{
    wxString password = GetPassword();

    git_credential_userpass_plaintext_new( aOut, aUsername.mbc_str(), password.mbc_str() );
    m_testedTypes |= GIT_CREDENTIAL_USERPASS_PLAINTEXT;

    return GIT_OK;
}

int KIGIT_COMMON::HandleSSHAgentAuthentication( git_cred** aOut, const wxString& aUsername )
{
    if( git_credential_ssh_key_from_agent( aOut, aUsername.mbc_str() ) != GIT_OK )
    {
        wxLogTrace( traceGit, "Failed to create SSH agent credential for %s: %s",
                    aUsername, KIGIT_COMMON::GetLastGitError() );
        return GIT_PASSTHROUGH;
    }

    m_testedTypes |= KIGIT_CREDENTIAL_SSH_AGENT;
    return GIT_OK;
}


extern "C" int fetchhead_foreach_cb( const char*, const char*,
                                     const git_oid* aOID, unsigned int aIsMerge, void* aPayload )
{
    if( aIsMerge )
        git_oid_cpy( (git_oid*) aPayload, aOID );

    return 0;
}


extern "C" void clone_progress_cb( const char* aStr, size_t aLen, size_t aTotal, void* aPayload )
{
    KIGIT_REPO_MIXIN* parent = reinterpret_cast<KIGIT_REPO_MIXIN*>( aPayload );

    wxString progressMessage( aStr );
    parent->UpdateProgress( aLen, aTotal, progressMessage );
}


extern "C" int progress_cb( const char* str, int len, void* aPayload )
{
    KIGIT_REPO_MIXIN* parent = reinterpret_cast<KIGIT_REPO_MIXIN*>( aPayload );

    if( parent->GetCommon()->IsCancelled() )
    {
        wxLogTrace( traceGit, "Progress CB cancelled" );
        return GIT_EUSER;
    }

    wxString progressMessage( str, len );
    parent->UpdateProgress( 0, 0, progressMessage );

    return 0;
}


extern "C" int transfer_progress_cb( const git_transfer_progress* aStats, void* aPayload )
{
    KIGIT_REPO_MIXIN* parent = reinterpret_cast<KIGIT_REPO_MIXIN*>( aPayload );

    wxString progressMessage = wxString::Format( _( "Received %u of %u objects" ),
                                                 aStats->received_objects,
                                                 aStats->total_objects );
    if( parent->GetCommon()->IsCancelled() )
    {
        wxLogTrace( traceGit, "Transfer progress cancelled" );
        return GIT_EUSER;
    }

    parent->UpdateProgress( aStats->received_objects, aStats->total_objects, progressMessage );

    return 0;
}


extern "C" int update_cb( const char* aRefname, const git_oid* aFirst, const git_oid* aSecond,
                          void* aPayload )
{
    constexpr int cstring_len = 8;
    char          a_str[cstring_len + 1];
    char          b_str[cstring_len + 1];

    KIGIT_REPO_MIXIN* parent = reinterpret_cast<KIGIT_REPO_MIXIN*>( aPayload );
    wxString          status;

    git_oid_tostr( b_str, cstring_len, aSecond );

#if ( LIBGIT2_VER_MAJOR >= 1 ) || ( LIBGIT2_VER_MINOR >= 99 )
    if( !git_oid_is_zero( aFirst ) )
#else
    if( !git_oid_iszero( aFirst ) )
#endif
    {
        git_oid_tostr( a_str, cstring_len, aFirst );
        status = wxString::Format( _( "* [updated] %s..%s %s" ), a_str, b_str, aRefname );
    }
    else
    {
        status = wxString::Format( _( "* [new] %s %s" ), b_str, aRefname );
    }

    parent->UpdateProgress( 0, 0, status );

    return 0;
}


extern "C" int push_transfer_progress_cb( unsigned int aCurrent, unsigned int aTotal, size_t aBytes,
                                          void* aPayload )
{
    long long         progress = 100;
    KIGIT_REPO_MIXIN* parent = reinterpret_cast<KIGIT_REPO_MIXIN*>( aPayload );

    if( aTotal != 0 )
    {
        progress = ( aCurrent * 100ll ) / aTotal;
    }

    wxString progressMessage = wxString::Format( _( "Writing objects: %lld%% (%u/%u), %zu bytes" ),
                                                 progress, aCurrent, aTotal, aBytes );
    parent->UpdateProgress( aCurrent, aTotal, progressMessage );

    return 0;
}


extern "C" int push_update_reference_cb( const char* aRefname, const char* aStatus, void* aPayload )
{
    KIGIT_REPO_MIXIN* parent = reinterpret_cast<KIGIT_REPO_MIXIN*>( aPayload );
    wxString          status( aStatus );

    if( !status.IsEmpty() )
    {
        wxString statusMessage = wxString::Format( _( "* [rejected] %s (%s)" ), aRefname, aStatus );
        parent->UpdateProgress( 0, 0, statusMessage );
    }
    else
    {
        wxString statusMessage = wxString::Format( _( "[updated] %s" ), aRefname );
        parent->UpdateProgress( 0, 0, statusMessage );
    }

    return 0;
}


extern "C" int credentials_cb( git_cred** aOut, const char* aUrl, const char* aUsername,
                               unsigned int aAllowedTypes, void* aPayload )
{
    KIGIT_REPO_MIXIN* parent = reinterpret_cast<KIGIT_REPO_MIXIN*>( aPayload );
    KIGIT_COMMON* common = parent->GetCommon();

    wxLogTrace( traceGit, "Credentials callback for %s, testing %d", aUrl, aAllowedTypes );

    if( parent->GetConnType() == KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_LOCAL )
    {
        wxLogTrace( traceGit, "Local repository, no credentials needed" );
        return GIT_PASSTHROUGH;
    }

    if( aAllowedTypes & GIT_CREDENTIAL_USERNAME
        && !( parent->TestedTypes() & GIT_CREDENTIAL_USERNAME ) )
    {
        wxString username = parent->GetUsername().Trim().Trim( false );
        wxLogTrace( traceGit, "Username credential for %s at %s with allowed type %d",
                    username, aUrl, aAllowedTypes );
        if( git_credential_username_new( aOut, username.ToStdString().c_str() ) != GIT_OK )
        {
            wxLogTrace( traceGit, "Failed to create username credential for %s: %s",
                        username, KIGIT_COMMON::GetLastGitError() );
        }
        else
        {
            wxLogTrace( traceGit, "Created username credential for %s", username );
        }

        parent->TestedTypes() |= GIT_CREDENTIAL_USERNAME;
    }
    else if( parent->GetConnType() == KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_HTTPS
                && ( aAllowedTypes & GIT_CREDENTIAL_USERPASS_PLAINTEXT )
                && !( parent->TestedTypes() & GIT_CREDENTIAL_USERPASS_PLAINTEXT ) )
    {
        // Plaintext authentication
        wxLogTrace( traceGit, "Plaintext authentication for %s at %s with allowed type %d",
                    parent->GetUsername(), aUrl, aAllowedTypes );
        return common->HandlePlaintextAuthentication( aOut, parent->GetUsername() );
    }
    else if( parent->GetConnType() == KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_SSH
                && ( aAllowedTypes & GIT_CREDENTIAL_SSH_KEY )
                && !( parent->TestedTypes() & GIT_CREDENTIAL_SSH_KEY ) )
    {
        // SSH key authentication
        return common->HandleSSHKeyAuthentication( aOut, parent->GetUsername() );
    }
    else
    {
        // If we didn't find anything to try, then we don't have a callback set that the
        // server likes
        if( !parent->TestedTypes() )
            return GIT_PASSTHROUGH;

        git_error_clear();
        git_error_set_str( GIT_ERROR_NET, _( "Unable to authenticate" ).mbc_str() );
        // Otherwise, we did try something but we failed, so return an authentication error
        return GIT_EAUTH;
    }

    return GIT_OK;
};
