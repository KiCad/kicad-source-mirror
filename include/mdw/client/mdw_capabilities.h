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
 * @file mdw_capabilities.h
 * @brief What a backend implements, consulted before exposing UI or calling an optional sub-interface.
 */

#ifndef MDW_CAPABILITIES_H
#define MDW_CAPABILITIES_H

#include <mdw/client/mdw_types.h>

#include <api/workspace/workspace_types.pb.h>

#include <wx/string.h>
#include <set>
#include <vector>

/**
 * Optional capability groups. Mandatory checkout is not a group. Numbering
 * matches kiapi.workspace.CapabilityGroup.
 */
enum class MDW_CAPABILITY_GROUP
{
    UNKNOWN          = 0,
    LOCKING          = 1,
    REVISIONS        = 2,
    LIFECYCLE        = 4,
    EVENTS           = 5,
    SEARCH           = 6,
    SHARED_LIBRARIES = 9,
    BOM              = 10,
    ECO              = 11,
    REVIEW           = 12,
    PARTS            = 13,
    PERMISSIONS      = 14
};


/// The capability advertisement returned by the handshake
struct MDW_CAPABILITIES
{
    std::set<MDW_CAPABILITY_GROUP> groups;
    std::vector<kiapi::workspace::CustomState> custom_states; ///< Display names layered on lifecycle categories
    bool                           adopts_whole_files = false; ///< A library check-in honours whole_files

    bool Has( MDW_CAPABILITY_GROUP aGroup ) const
    {
        return groups.find( aGroup ) != groups.end();
    }
};


#endif // MDW_CAPABILITIES_H
