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

#include "git_push_handler.h"
#include <git/kicad_git_common.h>

#include <iostream>

GIT_PUSH_HANDLER::GIT_PUSH_HANDLER( KIGIT_COMMON* aRepo ) :  KIGIT_COMMON( *aRepo )
{}


GIT_PUSH_HANDLER::~GIT_PUSH_HANDLER()
{}


PushResult GIT_PUSH_HANDLER::PerformPush()
{
    PushResult result = PushResult::Success;

    // Fetch updates from remote repository
    git_remote* remote = nullptr;

    if( git_remote_lookup( &remote, m_repo, "origin" ) != 0 )
    {
        AddErrorString( _( "Could not lookup remote" ) );
        return PushResult::Error;
    }

    git_remote_callbacks remoteCallbacks;
    git_remote_init_callbacks( &remoteCallbacks, GIT_REMOTE_CALLBACKS_VERSION );
    remoteCallbacks.sideband_progress = progress_cb;
    remoteCallbacks.transfer_progress = transfer_progress_cb;
    remoteCallbacks.update_tips = update_cb;
    remoteCallbacks.push_transfer_progress = push_transfer_progress_cb;
    remoteCallbacks.credentials = credentials_cb;
    remoteCallbacks.payload = this;

    m_testedTypes = 0;
    ResetNextKey();

    if( git_remote_connect( remote, GIT_DIRECTION_PUSH, &remoteCallbacks, nullptr, nullptr ) )
    {
        AddErrorString( wxString::Format( _( "Could not connect to remote: %s" ),
                                          git_error_last()->message ) );
        git_remote_free( remote );
        return PushResult::Error;
    }

    git_push_options pushOptions;
    git_push_init_options( &pushOptions, GIT_PUSH_OPTIONS_VERSION );
    pushOptions.callbacks = remoteCallbacks;

    // Get the current HEAD reference
    git_reference* head = nullptr;

    if( git_repository_head( &head, m_repo ) != 0 )
    {
        AddErrorString( _( "Could not get repository head" ) );
        git_remote_free( remote );
        return PushResult::Error;
    }

    // Create refspec for current branch
    const char* refs[1];
    refs[0] = git_reference_name( head );
    const git_strarray refspecs = { (char**) refs, 1 };

    git_reference_free(head);
    if( git_remote_push( remote, &refspecs, &pushOptions ) )
    {
        AddErrorString( wxString::Format( _( "Could not push to remote: %s" ),
                                          git_error_last()->message ) );
        git_remote_disconnect( remote );
        git_remote_free( remote );
        return PushResult::Error;
    }

    git_remote_disconnect( remote );
    git_remote_free( remote );

    return result;
}


void GIT_PUSH_HANDLER::UpdateProgress( int aCurrent, int aTotal, const wxString& aMessage )
{
    ReportProgress( aCurrent, aTotal, aMessage );
}
