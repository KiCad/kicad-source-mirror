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

#include "git_status_handler.h"
#include <trace_helpers.h>
#include "git_backend.h"

GIT_STATUS_HANDLER::GIT_STATUS_HANDLER( KIGIT_COMMON* aCommon ) : KIGIT_REPO_MIXIN( aCommon )
{}

GIT_STATUS_HANDLER::~GIT_STATUS_HANDLER()
{}

bool GIT_STATUS_HANDLER::HasChangedFiles()
{
    return GetGitBackend()->HasChangedFiles( this );
}

std::map<wxString, FileStatus> GIT_STATUS_HANDLER::GetFileStatus( const wxString& aPathspec )
{
    return GetGitBackend()->GetFileStatus( this, aPathspec );
}

wxString GIT_STATUS_HANDLER::GetCurrentBranchName()
{
    return GetGitBackend()->GetCurrentBranchName( this );
}

void GIT_STATUS_HANDLER::UpdateRemoteStatus( const std::set<wxString>& aLocalChanges,
                                             const std::set<wxString>& aRemoteChanges,
                                             std::map<wxString, FileStatus>& aFileStatus )
{
    GetGitBackend()->UpdateRemoteStatus( this, aLocalChanges, aRemoteChanges, aFileStatus );
}

wxString GIT_STATUS_HANDLER::GetWorkingDirectory()
{
    return GetGitBackend()->GetWorkingDirectory( this );
}

KIGIT_COMMON::GIT_STATUS GIT_STATUS_HANDLER::ConvertStatus( unsigned int aGitStatus )
{
    if( aGitStatus & GIT_STATUS_IGNORED )
    {
        return KIGIT_COMMON::GIT_STATUS::GIT_STATUS_IGNORED;
    }
    else if( aGitStatus & ( GIT_STATUS_INDEX_MODIFIED | GIT_STATUS_WT_MODIFIED ) )
    {
        return KIGIT_COMMON::GIT_STATUS::GIT_STATUS_MODIFIED;
    }
    else if( aGitStatus & ( GIT_STATUS_INDEX_NEW | GIT_STATUS_WT_NEW ) )
    {
        return KIGIT_COMMON::GIT_STATUS::GIT_STATUS_ADDED;
    }
    else if( aGitStatus & ( GIT_STATUS_INDEX_DELETED | GIT_STATUS_WT_DELETED ) )
    {
        return KIGIT_COMMON::GIT_STATUS::GIT_STATUS_DELETED;
    }
    else if( aGitStatus & ( GIT_STATUS_CONFLICTED ) )
    {
        return KIGIT_COMMON::GIT_STATUS::GIT_STATUS_CONFLICTED;
    }
    else
    {
        return KIGIT_COMMON::GIT_STATUS::GIT_STATUS_CURRENT;
    }
}

void GIT_STATUS_HANDLER::UpdateProgress( int aCurrent, int aTotal, const wxString& aMessage )
{
    ReportProgress( aCurrent, aTotal, aMessage );
}