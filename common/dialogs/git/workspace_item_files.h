/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef WORKSPACE_ITEM_FILES_H
#define WORKSPACE_ITEM_FILES_H

#include <kicommon.h>
#include <mdw/client/mdw_client_types.h>
#include <mdw/client/mdw_item.h>
#include <mdw/client/mdw_navigation.h>
#include <mdw/client/mdw_revision.h>

#include <wx/string.h>

#include <map>
#include <optional>
#include <utility>
#include <vector>


/**
 * The local files behind the Workspace page's Check Out and Check In, and the display text it builds for them. A
 * backend without a working tree of its own writes and reads only the path the request names.
 */

/// Where a check out of aItem writes, its relative path under aProjectDir. Empty when the path is missing, would
/// land outside the directory, or names a project's local settings, so there is nowhere to check it out to.
KICOMMON_API wxString WorkspaceCheckoutPath( const wxString& aProjectDir, const MDW_ITEM_INFO& aItem );

/// The file a check in of aItem uploads, which is the one aRecorded (the project's file to item id map, keyed
/// relative to aProjectDir) holds for it, else its check out path unless aRecorded binds that to another item.
KICOMMON_API wxString WorkspaceCheckinPath( const std::map<wxString, wxString>& aRecorded, const wxString& aProjectDir,
                                            const MDW_ITEM_INFO& aItem );

/// The display path of aFolder below the folder named aParentName, which is empty for a root
KICOMMON_API wxString WorkspaceFolderName( const wxString& aParentName, const MDW_FOLDER& aFolder );

/// How the Versions list names aRevision, so a status line names it the same way.
KICOMMON_API wxString WorkspaceRevisionText( const MDW_REVISION& aRevision );

/// The file aItem is, from its display path or else its id, so no folder or backend id shows
KICOMMON_API wxString MdwItemFileName( const MDW_ITEM_REF& aItem );

/// How a Compare names one side, the file (or member) the item is and its version, as "amp-board.kicad_sch v1"
KICOMMON_API wxString MdwVersionSideLabel( const MDW_ITEM_REF& aItem, const MDW_REVISION& aRevision );

/**
 * The older and newer row Compare opens in the Versions list, or nothing when the selection has nothing to compare.
 * aRows counts the versions, newest first. A list with a working copy has it as row 0 above them.
 * One selected row compares with the row below it, the oldest with the working copy. Two compare with each other.
 */
KICOMMON_API std::optional<std::pair<long, long>> MdwComparePair( long aRows, const std::vector<long>& aSelected,
                                                                  bool aHasWorkingRow );

/// True when both files exist and their bytes differ. A file that cannot be read says nothing, so it never differs.
KICOMMON_API bool MdwFilesDiffer( const wxString& aPath, const wxString& aOther );

/// The status line after checking in aItem as aRevision.
KICOMMON_API wxString WorkspaceCheckinReport( const MDW_ITEM_INFO& aItem, const MDW_REVISION& aRevision );

/// Where Keep Both saves aPath beside itself, as <name>.<ext>.mine, numbered past any file already there.
KICOMMON_API wxString WorkspaceKeepBothPath( const wxString& aPath );

#endif // WORKSPACE_ITEM_FILES_H
