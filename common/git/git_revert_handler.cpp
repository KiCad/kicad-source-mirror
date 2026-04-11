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

#include "git_revert_handler.h"
#include "git_backend.h"


GIT_REVERT_HANDLER::GIT_REVERT_HANDLER( git_repository* aRepository )
{
    m_repository = aRepository;
}


GIT_REVERT_HANDLER::~GIT_REVERT_HANDLER()
{
}


bool GIT_REVERT_HANDLER::Revert( const wxString& aFilePath )
{
    m_filesToRevert.push_back( aFilePath );
    return true;
}

void GIT_REVERT_HANDLER::PerformRevert()
{
    GetGitBackend()->PerformRevert( this );
}

