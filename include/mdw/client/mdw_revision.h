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
 * @file mdw_revision.h
 * @brief A point-in-time snapshot of a managed item.
 */

#ifndef MDW_REVISION_H
#define MDW_REVISION_H

// Load-bearing for MSVC, which otherwise emits std::vector<wxString> members wx already exports
#include <mdw/client/mdw_types.h>

#include <wx/arrstr.h>
#include <wx/string.h>
#include <wx/datetime.h>
#include <cstdint>
#include <vector>

/**
 * One revision. parent_revision_ids is empty at a root and carries more than
 * one entry for a merge. Ids are opaque and backend-supplied.
 */
struct MDW_REVISION
{
    wxString              revision_id;
    wxString              label;                ///< "3", "A.1"; the text a user sees
    uint64_t              ordinal = 0;          ///< Sorts one item's revisions of one kind
    MDW_REVISION_KIND     kind = MDW_REVISION_KIND::UNKNOWN;
    std::vector<wxString> parent_revision_ids;
    wxString              author;
    wxDateTime            timestamp;
    wxString              comment;
    wxString              content_digest;       ///< "v1:<sha256>" stored with a release
    wxString              member_digest;        ///< Of the whole member, where it differs from content_digest
    MDW_LIFECYCLE_STATE   state;
};


#endif // MDW_REVISION_H
