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

#include "git_init_handler.h"
#include <git/kicad_git_common.h>
#include <git/kicad_git_memory.h>
#include <trace_helpers.h>
#include <wx/log.h>

GIT_INIT_HANDLER::GIT_INIT_HANDLER( KIGIT_COMMON* aCommon ) : KIGIT_REPO_MIXIN( aCommon )
{}

GIT_INIT_HANDLER::~GIT_INIT_HANDLER()
{}

bool GIT_INIT_HANDLER::IsRepository( const wxString& aPath )
{
    git_repository* repo = nullptr;
    int error = git_repository_open( &repo, aPath.mb_str() );

    if( error == 0 )
    {
        git_repository_free( repo );
        return true;
    }

    return false;
}

InitResult GIT_INIT_HANDLER::InitializeRepository( const wxString& aPath )
{
    // Check if directory is already a git repository
    if( IsRepository( aPath ) )
    {
        return InitResult::AlreadyExists;
    }

    git_repository* repo = nullptr;

    if( git_repository_init( &repo, aPath.mb_str(), 0 ) != GIT_OK )
    {
        if( repo )
            git_repository_free( repo );

        AddErrorString( wxString::Format( _( "Failed to initialize Git repository: %s" ),
                                          KIGIT_COMMON::GetLastGitError() ) );
        return InitResult::Error;
    }

    // Update the common repository pointer
    GetCommon()->SetRepo( repo );

    wxLogTrace( traceGit, "Successfully initialized Git repository at %s", aPath );
    return InitResult::Success;
}

bool GIT_INIT_HANDLER::SetupRemote( const RemoteConfig& aConfig )
{
    git_repository* repo = GetRepo();

    if( !repo )
    {
        AddErrorString( _( "No repository available to set up remote" ) );
        return false;
    }

    // Update connection settings in common
    GetCommon()->SetUsername( aConfig.username );
    GetCommon()->SetPassword( aConfig.password );
    GetCommon()->SetSSHKey( aConfig.sshKey );

    git_remote* remote = nullptr;
    wxString fullURL;

    // Build the full URL based on connection type
    if( aConfig.connType == KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_SSH )
    {
        fullURL = aConfig.username + "@" + aConfig.url;
    }
    else if( aConfig.connType == KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_HTTPS )
    {
        fullURL = aConfig.url.StartsWith( "https" ) ? "https://" : "http://";

        if( !aConfig.username.empty() )
        {
            fullURL.append( aConfig.username );

            if( !aConfig.password.empty() )
            {
                fullURL.append( wxS( ":" ) );
                fullURL.append( aConfig.password );
            }

            fullURL.append( wxS( "@" ) );
        }

        // Extract the bare URL (without protocol prefix)
        wxString bareURL = aConfig.url;
        if( bareURL.StartsWith( "https://" ) )
            bareURL = bareURL.Mid( 8 );
        else if( bareURL.StartsWith( "http://" ) )
            bareURL = bareURL.Mid( 7 );

        fullURL.append( bareURL );
    }
    else
    {
        fullURL = aConfig.url;
    }

    int error = git_remote_create_with_fetchspec( &remote, repo, "origin",
                                                  fullURL.ToStdString().c_str(),
                                                  "+refs/heads/*:refs/remotes/origin/*" );

    KIGIT::GitRemotePtr remotePtr( remote );

    if( error != GIT_OK )
    {
        AddErrorString( wxString::Format( _( "Failed to create remote: %s" ),
                                          KIGIT_COMMON::GetLastGitError() ) );
        return false;
    }

    wxLogTrace( traceGit, "Successfully set up remote origin" );
    return true;
}

void GIT_INIT_HANDLER::UpdateProgress( int aCurrent, int aTotal, const wxString& aMessage )
{
    ReportProgress( aCurrent, aTotal, aMessage );
}