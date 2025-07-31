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

#include "git_branch_handler.h"
#include <git/kicad_git_common.h>
#include <git/kicad_git_memory.h>
#include <trace_helpers.h>
#include <wx/log.h>

GIT_BRANCH_HANDLER::GIT_BRANCH_HANDLER( KIGIT_COMMON* aCommon ) : KIGIT_REPO_MIXIN( aCommon )
{}

GIT_BRANCH_HANDLER::~GIT_BRANCH_HANDLER()
{}

bool GIT_BRANCH_HANDLER::BranchExists( const wxString& aBranchName )
{
    git_repository* repo = GetRepo();

    if( !repo )
        return false;

    git_reference* branchRef = nullptr;
    bool exists = LookupBranchReference( aBranchName, &branchRef );

    if( branchRef )
        git_reference_free( branchRef );

    return exists;
}

bool GIT_BRANCH_HANDLER::LookupBranchReference( const wxString& aBranchName, git_reference** aReference )
{
    git_repository* repo = GetRepo();

    if( !repo )
        return false;

    // Try direct lookup first
    if( git_reference_lookup( aReference, repo, aBranchName.mb_str() ) == GIT_OK )
        return true;

    // Try dwim (Do What I Mean) lookup for short branch names
    if( git_reference_dwim( aReference, repo, aBranchName.mb_str() ) == GIT_OK )
        return true;

    return false;
}

BranchResult GIT_BRANCH_HANDLER::SwitchToBranch( const wxString& aBranchName )
{
    git_repository* repo = GetRepo();

    if( !repo )
    {
        AddErrorString( _( "No repository available" ) );
        return BranchResult::Error;
    }

    // Look up the branch reference
    git_reference* branchRef = nullptr;

    if( !LookupBranchReference( aBranchName, &branchRef ) )
    {
        AddErrorString( wxString::Format( _( "Failed to lookup branch '%s': %s" ),
                                          aBranchName, KIGIT_COMMON::GetLastGitError() ) );
        return BranchResult::BranchNotFound;
    }

    KIGIT::GitReferencePtr branchRefPtr( branchRef );
    const char* branchRefName = git_reference_name( branchRef );
    git_object* branchObj = nullptr;

    if( git_revparse_single( &branchObj, repo, aBranchName.mb_str() ) != GIT_OK )
    {
        AddErrorString( wxString::Format( _( "Failed to find branch head for '%s': %s" ),
                                          aBranchName, KIGIT_COMMON::GetLastGitError() ) );
        return BranchResult::Error;
    }

    KIGIT::GitObjectPtr branchObjPtr( branchObj );

    // Switch to the branch
    if( git_checkout_tree( repo, branchObj, nullptr ) != GIT_OK )
    {
        AddErrorString( wxString::Format( _( "Failed to switch to branch '%s': %s" ),
                                          aBranchName, KIGIT_COMMON::GetLastGitError() ) );
        return BranchResult::CheckoutFailed;
    }

    // Update the HEAD reference
    if( git_repository_set_head( repo, branchRefName ) != GIT_OK )
    {
        AddErrorString( wxString::Format( _( "Failed to update HEAD reference for branch '%s': %s" ),
                                          aBranchName, KIGIT_COMMON::GetLastGitError() ) );
        return BranchResult::Error;
    }

    wxLogTrace( traceGit, "Successfully switched to branch '%s'", aBranchName );
    return BranchResult::Success;
}

void GIT_BRANCH_HANDLER::UpdateProgress( int aCurrent, int aTotal, const wxString& aMessage )
{
    ReportProgress( aCurrent, aTotal, aMessage );
}