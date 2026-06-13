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
 */

#ifndef GIT_COMPARE_HANDLER_H
#define GIT_COMPARE_HANDLER_H

#include <kicommon.h>

#include <wx/string.h>

#include <vector>


struct git_repository;


namespace KIGIT
{

enum class FILE_CHANGE_STATUS
{
    UNCHANGED,
    ADDED,
    REMOVED,
    MODIFIED,
    RENAMED,
    COPIED,
    TYPECHANGE
};


struct KICOMMON_API CHANGED_FILE
{
    wxString           path;
    wxString           oldPath;       // populated for RENAMED / COPIED
    FILE_CHANGE_STATUS status = FILE_CHANGE_STATUS::UNCHANGED;
};


/**
 * Compare two git refs (branch / tag / commit OID) within a repository and
 * return the per-file change list.
 *
 * Used by the git PR-review dialog (Phase 10) to populate the "changed files"
 * list a user clicks through to open the file-compare dialog.
 *
 * `aRepo` must not be null. Returns an empty vector on error and logs via
 * wxLogTrace( traceGit ).
 */
KICOMMON_API std::vector<CHANGED_FILE> CompareRefs( git_repository* aRepo,
                                                    const wxString& aBaseRef,
                                                    const wxString& aHeadRef );


KICOMMON_API const char* FileChangeStatusToString( FILE_CHANGE_STATUS aStatus );

} // namespace KIGIT

#endif // GIT_COMPARE_HANDLER_H
