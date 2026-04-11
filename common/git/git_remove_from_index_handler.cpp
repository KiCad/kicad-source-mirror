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

#include "git_remove_from_index_handler.h"
#include "git_backend.h"

#include <wx/log.h>

GIT_REMOVE_FROM_INDEX_HANDLER::GIT_REMOVE_FROM_INDEX_HANDLER( git_repository* aRepository ) :
    KIGIT_COMMON( aRepository )
{
    m_filesToRemove.clear();
}


GIT_REMOVE_FROM_INDEX_HANDLER::~GIT_REMOVE_FROM_INDEX_HANDLER()
{
}


bool GIT_REMOVE_FROM_INDEX_HANDLER::RemoveFromIndex( const wxString& aFilePath )
{
    return GetGitBackend()->RemoveFromIndex( this, aFilePath );
}


void GIT_REMOVE_FROM_INDEX_HANDLER::PerformRemoveFromIndex()
{
    GetGitBackend()->PerformRemoveFromIndex( this );
}
