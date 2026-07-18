/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file mdw_item.h
 * @brief Managed item metadata and the checkout/checkin operation structs.
 *
 * The category and lock fields carry last-known state so a browser listing renders without
 * follow-up calls.
 */

#ifndef MDW_ITEM_H
#define MDW_ITEM_H

#include <mdw/client/mdw_types.h>
#include <mdw/client/mdw_lock.h>
#include <mdw/client/mdw_revision.h>

#include <wx/string.h>
#include <wx/datetime.h>

struct MDW_ITEM_INFO
{
    wxString            item_id;
    wxString            display_name;
    wxString            path_hint;              ///< Display only
    wxString            relative_path;          ///< Where a checkout lands under the project
    wxString            folder_id;
    bool                is_folder = false;
    MDW_DOCUMENT_KIND   kind = MDW_DOCUMENT_KIND::UNKNOWN;
    wxDateTime          modified;
    wxString            revision_id;            ///< The revision described; empty for folders
    wxString            revision_label;
    MDW_REVISION_KIND   revision_kind = MDW_REVISION_KIND::UNKNOWN;
    MDW_LIFECYCLE_STATE state;                  ///< The state of revision_id
    MDW_LOCK_INFO       lock;                   ///< Last known
};


/// idempotency_id lets a retry after a mid-call crash avoid applying the checkout twice
struct MDW_CHECKOUT_REQUEST
{
    wxString item_id;
    wxString local_path;
    bool     lock = false;
    wxString idempotency_id;
};


/// local_path names the working copy, which may differ from the path the request asked for
struct MDW_CHECKOUT_RESULT
{
    MDW_ITEM_INFO item;
    wxString      local_path;
};


struct MDW_CHECKIN_REQUEST
{
    wxString item_id;
    wxString local_path;
    wxString comment;
    wxString idempotency_id;
    wxString base_revision_id; ///< The revision local_path was edited from; empty when unknown
};


/// Request to make the file at local_path a new item, which a checkout places at relative_path
struct MDW_ADD_ITEM_REQUEST
{
    wxString relative_path;
    wxString local_path;
    wxString comment;
    wxString idempotency_id;
};


/// aLocalPath relative to aRoot with '/' separators. Empty for the root itself, for a path outside the root
/// (including "../" escapes), and when either side cannot be canonicalized
KICOMMON_API wxString MdwRootRelative( const wxString& aRoot, const wxString& aLocalPath );

/// MdwRootRelative, but a path outside the root keeps its leading "../" run, as LocateItems takes it.
/// Empty for the root and its ancestors, for a path on another drive or share, and when either side cannot be
/// canonicalized
KICOMMON_API wxString MdwRootRelativeUp( const wxString& aRoot, const wxString& aLocalPath );


/// The local path of a root-relative path, joined by component with no "~" or "." interpretation
KICOMMON_API wxString MdwLocalPath( const wxString& aRoot, const wxString& aRelative );

#endif // MDW_ITEM_H
