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

#ifndef GIT_COMMIT_HANDLER_H
#define GIT_COMMIT_HANDLER_H

// Define a class to handle git commit operations

#include <git/kicad_git_common.h>
#include <import_export.h>
#include "git_backend.h"

#include <string>
#include <vector>
#include <wx/arrstr.h> // for MSVC to see std::vector<wxString> is exported from wx
#include <wx/string.h>

class LIBGIT_BACKEND;

class APIEXPORT GIT_COMMIT_HANDLER : public KIGIT_COMMON
{
public:
    GIT_COMMIT_HANDLER( git_repository* aRepo );
    virtual ~GIT_COMMIT_HANDLER();

    CommitResult PerformCommit( const std::vector<wxString>& aFiles,
                               const wxString&               aMessage,
                               const wxString&               aAuthorName,
                               const wxString&               aAuthorEmail );

    wxString GetErrorString() const;

private:
    friend class LIBGIT_BACKEND;
    void AddErrorString( const wxString& aErrorString );

    wxString m_errorString;
};

#endif // GIT_COMMIT_HANDLER_H