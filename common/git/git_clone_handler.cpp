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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "git_clone_handler.h"

#include <trace_helpers.h>
#include "git_backend.h"
#include "git_init_handler.h"

GIT_CLONE_HANDLER::GIT_CLONE_HANDLER( KIGIT_COMMON* aCommon ) :  KIGIT_REPO_MIXIN( aCommon )
{}


GIT_CLONE_HANDLER::~GIT_CLONE_HANDLER()
{}


bool GIT_CLONE_HANDLER::PerformClone()
{
    const bool ok = GetGitBackend()->Clone( this );

    // Apply KiCad's conventions to the freshly-cloned repo so subsequent
    // command-line merges through this clone route through kicad-cli the
    // same way a locally-initialised repo would.
    if( ok )
        ApplyKicadGitConventions( GetClonePath() );

    return ok;
}


void GIT_CLONE_HANDLER::UpdateProgress( int aCurrent, int aTotal, const wxString& aMessage )
{
    ReportProgress( aCurrent, aTotal, aMessage );
}
