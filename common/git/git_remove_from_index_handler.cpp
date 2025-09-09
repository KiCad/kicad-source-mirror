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
