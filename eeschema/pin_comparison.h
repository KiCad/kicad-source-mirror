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

#include <map>
#include <optional>
#include <math/vector2d.h>
#include <pin_type.h>
#include <wx/string.h>

struct PIN_ALTERNATE
{
    wxString m_Name;
    GRAPHIC_PINSHAPE m_Shape;
    ELECTRICAL_PINTYPE m_Type;
    bool operator==( const PIN_ALTERNATE& ) const = default;
};

/** Owned library-pin inputs; no parent, layout cache or schematic pointers. */
struct PIN_COMPARISON_DATA
{
    int unit = 0;
    int bodyStyle = 0;
    bool isPrivate = false;
    VECTOR2I position;
    wxString number;
    wxString name;
    std::optional<int> length;
    PIN_ORIENTATION orientation = PIN_ORIENTATION::PIN_RIGHT;
    GRAPHIC_PINSHAPE shape = GRAPHIC_PINSHAPE::LINE;
    ELECTRICAL_PINTYPE type = ELECTRICAL_PINTYPE::PT_UNSPECIFIED;
    std::optional<bool> hidden;
    std::optional<int> numberTextSize;
    std::optional<int> nameTextSize;
    std::map<wxString, PIN_ALTERNATE> alternates;

    int Compare( const PIN_COMPARISON_DATA& aOther, int aCompareFlags ) const;
    bool operator==( const PIN_COMPARISON_DATA& ) const = default;
};
