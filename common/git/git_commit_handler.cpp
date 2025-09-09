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

#include "git_commit_handler.h"
#include "git_backend.h"

GIT_COMMIT_HANDLER::GIT_COMMIT_HANDLER( git_repository* aRepo ) :
    KIGIT_COMMON( aRepo )
{}


GIT_COMMIT_HANDLER::~GIT_COMMIT_HANDLER()
{}


CommitResult
GIT_COMMIT_HANDLER::PerformCommit( const std::vector<wxString>& aFiles,
                                   const wxString&               aMessage,
                                   const wxString&               aAuthorName,
                                   const wxString&               aAuthorEmail )
{
    return GetGitBackend()->Commit( this, aFiles, aMessage, aAuthorName, aAuthorEmail );
}


wxString GIT_COMMIT_HANDLER::GetErrorString() const
{
    return m_errorString;
}


void GIT_COMMIT_HANDLER::AddErrorString( const wxString& aErrorString )
{
    m_errorString += aErrorString;
}

