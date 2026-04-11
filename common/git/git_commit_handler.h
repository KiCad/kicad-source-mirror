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

    CommitResult PerformAmend( const std::vector<wxString>& aFiles, const wxString& aMessage,
                               const wxString& aAuthorName, const wxString& aAuthorEmail );

    wxString GetErrorString() const;

private:
    friend class LIBGIT_BACKEND;
    void AddErrorString( const wxString& aErrorString );

    wxString m_errorString;
};

#endif // GIT_COMMIT_HANDLER_H