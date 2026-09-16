/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "conn_keys.h"
#include <wx/string.h>

class SCH_SHEET_LIST;

namespace SCH_CONNECTIVITY
{
/**
 * The hierarchy position of one sheet instance.
 *
 * @see @ref sch_conn_glossary
 */
struct INSTANCE_SCOPE
{
    INST_ID  instance = INVALID_ID; ///< The session handle of the instance KIID_PATH.
    wxString path;                  ///< The human-readable sheet path, which prefixes scoped names.
    uint16_t depth = 0;             ///< The number of sheets in the path, with the root sheet as one.
    // Only instantiated children contribute hierarchy port edges.
    std::map<KIID, INST_ID> children; ///< The child instance of each SCH_SHEET KIID on this screen.
    bool                    operator==( const INSTANCE_SCOPE& ) const = default;
};

/**
 * One entry of the captured hierarchy.
 */
struct FRAME_INSTANCE
{
    INSTANCE_SCOPE scope;
    SCREEN_ID      screen = 0; ///< The screen that the instance shows. Many instances can share it.
    bool           operator==( const FRAME_INSTANCE& ) const = default;
};

// Canonical instance descriptors; missing screens do not expose child ports.
/**
 * Interns every sheet instance and returns the frame in KIID_PATH order, so each parent precedes its children.
 */
std::vector<FRAME_INSTANCE> CaptureHierarchy( const SCH_SHEET_LIST& aPaths, SESSION_KEYS& aKeys );

} // namespace SCH_CONNECTIVITY
