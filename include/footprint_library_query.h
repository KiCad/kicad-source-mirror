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

#include <vector>

#include <wx/string.h>


class KIWAY;


/// The criteria for a #QueryMatchingFootprints() call.
struct FOOTPRINT_MATCH_QUERY
{
    /// Wildcard patterns matched against the footprint name, or against
    /// "library:footprint" when a pattern contains ':'.
    std::vector<wxString> patterns;

    /// When greater than zero, only footprints with this many pads are returned.
    int pinCount = 0;

    /// Limits the number of returned footprints; zero means no limit.
    int maxResults = 400;

    /// When true, a query with no patterns (and no pin count) matches nothing rather
    /// than everything.
    bool zeroFilters = true;
};


/**
 * Ask the footprint libraries, through the pcbnew kiface, for the footprints matching a query.
 *
 * Matching is case-insensitive and uses the same wildcard rules as the footprint chooser:
 * '?' and '*' are wildcards, and a pattern containing ':' is matched against the
 * "library:footprint" LIB_ID rather than against the footprint name alone.
 *
 * @param aKiway is used to reach the pcbnew kiface.
 * @param aQuery is the set of criteria to match.
 * @param aMatches receives the matching "library:footprint" IDs, replacing any previous
 *                 contents.  It is left empty when the function returns false.
 * @return false if the pcbnew kiface is unavailable (for example in a standalone tool
 *         without footprint support) or the query could not be performed.
 */
bool QueryMatchingFootprints( KIWAY& aKiway, const FOOTPRINT_MATCH_QUERY& aQuery, std::vector<wxString>& aMatches );
