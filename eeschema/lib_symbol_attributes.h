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
#include <vector>
#include <pin_map.h>
#include <wx/arrstr.h>
#include <wx/string.h>

/* values for member .m_options */
enum LIBRENTRYOPTIONS
{
    ENTRY_NORMAL,     // Libentry is a standard symbol (real or alias)
    ENTRY_GLOBAL_POWER,      // Libentry is a power symbol
    ENTRY_LOCAL_POWER // Libentry is a local power symbol
};

// Raw symbol attributes consulted by library parity and duplicate-pin suppression.
struct LIB_SYMBOL_ATTRIBUTES
{
    LIBRENTRYOPTIONS                  options = ENTRY_NORMAL;
    int                               unitCount = 1;
    wxArrayString                     footprintFilters;
    PIN_MAP_SET                       pinMaps;
    std::vector<ASSOCIATED_FOOTPRINT> associatedFootprints;
    wxString                          keywords;
    int                               pinNameOffset = 0;
    bool                              showPinNames = false;
    bool                              showPinNumbers = false;
    bool                              excludedFromSim = false;
    bool                              excludedFromBOM = false;
    bool                              excludedFromBoard = false;
    bool                              excludedFromPosFiles = false;
    bool                              dnp = false;
    bool                              unitsLocked = false;
    std::map<int, wxString>           unitDisplayNames;
    std::vector<wxString>             bodyStyleNames;
    bool                              duplicatePinNumbersAreJumpers = false;

    bool Matches( const LIB_SYMBOL_ATTRIBUTES& aOther, int aCompareFlags ) const;
    bool operator==( const LIB_SYMBOL_ATTRIBUTES& ) const = default;
};
