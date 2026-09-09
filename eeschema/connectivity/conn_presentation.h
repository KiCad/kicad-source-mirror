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

#include <optional>
#include <vector>
#include <wx/string.h>

class SCH_ITEM;
class SCH_SHEET_PATH;
class MSG_PANEL_ITEM;

namespace SCH_CONNECTIVITY
{
/** Append connection details; return its name for a signal, or no value for a bus/missing row. */
std::optional<wxString> AppendConnectionInfo( const SCH_ITEM& aItem,
                                             std::vector<MSG_PANEL_ITEM>& aList,
                                             const SCH_SHEET_PATH* aPath = nullptr );
}
