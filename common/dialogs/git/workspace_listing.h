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

#ifndef WORKSPACE_LISTING_H
#define WORKSPACE_LISTING_H

#include <kicommon.h>
#include <mdw/client/mdw_client_types.h>
#include <mdw/client/mdw_item.h>
#include <mdw/client/mdw_navigation.h>

#include <wx/string.h>

#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <vector>

class MDW_SOURCE;


/// Everything the Workspace page lists, as one walk of a source's folders read it
struct WORKSPACE_LISTING
{
    MDW_RESULT                   status = MDW_RESULT::Ok();
    std::vector<MDW_ITEM_INFO>   items;       ///< Items, each with its folder, without folder rows among them
    std::vector<MDW_FOLDER>      folders;     ///< Each folder once, its parent id the folder it was read under
    std::map<wxString, wxString> folderNames; ///< Each folder's display path, by folder id
};


/// The reads one walk makes, each filling its rows and answering its status
struct WORKSPACE_LISTING_READS
{
    std::function<MDW_RESULT( std::vector<MDW_FOLDER>& )>                    rootFolders;
    std::function<MDW_RESULT( const wxString&, std::vector<MDW_FOLDER>& )>    childFolders; ///< Of a folder id
    std::function<MDW_RESULT( const wxString&, std::vector<MDW_ITEM_INFO>& )> items;        ///< Of a folder id
};


/**
 * Walk aSource's folders breadth first from its roots, reading each folder's items and child folders. A root with
 * an empty id stands for the workspace itself, so its items and children sit at the top rather than under it. Each
 * folder id is read at most once, whatever ids the backend gives. The first failed read ends the walk with what was
 * read so far.
 *
 * The walk ends with CANCELLED once aCancelled is set, which also stops the request in flight, or once aStopped
 * answers true, which it asks before each request.
 */
KICOMMON_API WORKSPACE_LISTING ReadWorkspaceListing( const std::shared_ptr<MDW_SOURCE>&       aSource,
                                                     std::shared_ptr<const std::atomic<bool>> aCancelled,
                                                     const std::function<bool()>&             aStopped = nullptr );

/// The same walk over aReads, ending with CANCELLED once aStopped answers true before a read
KICOMMON_API WORKSPACE_LISTING ReadWorkspaceListing( const WORKSPACE_LISTING_READS& aReads,
                                                     const std::function<bool()>&   aStopped = nullptr );

#endif // WORKSPACE_LISTING_H
