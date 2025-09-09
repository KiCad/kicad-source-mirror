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
#include "git_backend.h"
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
    return GetGitBackend()->BranchExists( this, aBranchName );
}

BranchResult GIT_BRANCH_HANDLER::SwitchToBranch( const wxString& aBranchName )
{
    return GetGitBackend()->SwitchToBranch( this, aBranchName );
}

void GIT_BRANCH_HANDLER::UpdateProgress( int aCurrent, int aTotal, const wxString& aMessage )
{
    ReportProgress( aCurrent, aTotal, aMessage );
}