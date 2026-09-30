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


#ifndef WORKSPACE_DETAIL_H
#define WORKSPACE_DETAIL_H

#include <kicommon.h>
#include <mdw/client/mdw_capabilities.h>
#include <mdw/client/mdw_item.h>
#include <mdw/client/mdw_lock.h>

#include <wx/datetime.h>
#include <wx/string.h>

#include <optional>
#include <vector>


/// The State column for an item; empty without LIFECYCLE and for an editable state the backend does not name
KICOMMON_API wxString WorkspaceStateCell( const MDW_CAPABILITIES& aCaps, const MDW_LIFECYCLE_STATE& aState,
                                          const wxString& aRevisionLabel );


/// One label and value pair of the Workspace page's details block
struct WORKSPACE_DETAIL_ROW
{
    wxString label;
    wxString value;
};


/// The details rows for an item, in display order. aModified is unset while the local changes are unknown.
KICOMMON_API std::vector<WORKSPACE_DETAIL_ROW> WorkspaceDetailRows( const MDW_CAPABILITIES& aCaps,
                                                                    const MDW_ITEM_INFO& aItem,
                                                                    const MDW_LOCK_INFO& aLock,
                                                                    std::optional<bool> aModified,
                                                                    const wxString& aFolderPath,
                                                                    const wxDateTime& aNow );

#endif // WORKSPACE_DETAIL_H
