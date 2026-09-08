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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <utility>
#include <vector>
#include <wx/string.h>

struct SIM_MODEL_INPUT
{
    wxString prefix;
    /** Resolved shown values in symbol order, adapted to USER fields without further expansion. */
    std::vector<std::pair<wxString, wxString>> fields;

    /** Selected-unit pin numbers in lexical order, including duplicates. */
    std::vector<wxString> inferencePins;

    /** All library pin numbers in natural (StrNumCmp) order, including duplicates. */
    std::vector<wxString> modelPins;

    bool operator==( const SIM_MODEL_INPUT& ) const = default;
};
