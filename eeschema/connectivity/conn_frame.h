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
struct INSTANCE_SCOPE
{
    INST_ID  instance = INVALID_ID;
    wxString path;
    uint16_t depth = 0;
    // Only instantiated children contribute hierarchy port edges.
    std::map<KIID, INST_ID> children;
    bool                    operator==( const INSTANCE_SCOPE& ) const = default;
};

struct FRAME_INSTANCE
{
    INSTANCE_SCOPE scope;
    SCREEN_ID      screen = 0;
    bool           operator==( const FRAME_INSTANCE& ) const = default;
};

// Canonical instance descriptors; missing screens do not expose child ports.
std::vector<FRAME_INSTANCE> CaptureHierarchy( const SCH_SHEET_LIST& aPaths, SESSION_KEYS& aKeys );

} // namespace SCH_CONNECTIVITY
