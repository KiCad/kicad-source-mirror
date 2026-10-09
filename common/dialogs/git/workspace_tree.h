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

#ifndef WORKSPACE_TREE_H
#define WORKSPACE_TREE_H

#include <kicommon.h>
#include <mdw/client/mdw_item.h>
#include <mdw/client/mdw_navigation.h>

#include <wx/string.h>

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>


/// A team library the Workspace page offers, with its members once they have been listed
struct WORKSPACE_TREE_LIBRARY
{
    wxString              key;      ///< "sym:<nickname>" or "fp:<nickname>"
    wxString              name;     ///< The library table nickname
    bool                  listed = false;
    std::vector<wxString> members;  ///< Member names, when listed
};


/// One row of the Workspace page's tree
struct WORKSPACE_TREE_NODE
{
    enum class KIND
    {
        ROOT,
        FOLDER,
        ITEM,
        LIBRARIES,
        LIBRARY,
        MEMBER,
        UNLISTED ///< Stands in for the members of a library not listed yet, so it can be expanded
    };

    KIND                             kind = KIND::ROOT;
    wxString                         key; ///< folder_id, item_id, library key, "<library key>/<member>", or
                                          ///< "<library key>/" for UNLISTED
    wxString                         name;
    size_t                           item = SIZE_MAX; ///< Index into the items, for ITEM
    std::vector<WORKSPACE_TREE_NODE> children;
};


/**
 * Arrange folders, items and team libraries as the Workspace page shows them. Folders sort before items and
 * names sort naturally. A folder whose parent is missing or part of a loop sits at the root, and so does an item
 * whose folder is unknown. The libraries form one group after everything else, and a library not listed yet holds
 * one UNLISTED row.
 *
 * aFilter keeps rows whose name contains it, ignoring case, with the ancestors of each match. A matching folder
 * or library keeps everything below it. An empty filter keeps every row. aItemsMatched says a source's search
 * already matched aItems, so the filter applies only to the libraries, which that search does not reach.
 */
KICOMMON_API WORKSPACE_TREE_NODE BuildWorkspaceTree( const std::vector<MDW_FOLDER>&            aFolders,
                                                     const std::vector<MDW_ITEM_INFO>&         aItems,
                                                     const std::vector<WORKSPACE_TREE_LIBRARY>& aLibraries,
                                                     const wxString& aFilter, bool aItemsMatched = false );

/// A row as the page remembers it across rebuilds. Keys are unique only within a kind, since a file may be named
/// like a member's "<library key>/<member>"
using WORKSPACE_TREE_KEY = std::pair<WORKSPACE_TREE_NODE::KIND, wxString>;

/// The nodes from the top of aRoot down to the aKind node with aKey, for expanding a selection; empty when the tree
/// does not hold it
KICOMMON_API std::vector<const WORKSPACE_TREE_NODE*> WorkspaceTreePath( const WORKSPACE_TREE_NODE& aRoot,
                                                                        const WORKSPACE_TREE_KEY&  aKey );

#endif // WORKSPACE_TREE_H
