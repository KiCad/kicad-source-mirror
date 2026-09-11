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


class KIWAY;


/**
 * All the information needed to query the footprint libraries for matching footprints.
 */
struct FOOTPRINT_MATCH_QUERY
{
    /// Wildcard patterns matched against the footprint name, or against
    /// "library:footprint" when a pattern contains ':'.
    std::vector<wxString> m_Patterns;

    /// When greater than zero, only footprints with this many pads are returned.
    std::optional<int> m_PinCount;

    /// Limits the number of returned footprints; zero means no limit.
    int m_MaxResults = 400;

    /// When true, a query with no patterns (and no pin count) matches nothing rather
    /// than everything.
    bool m_ZeroFilters = true;
};


/// The outcome of a footprint match query, and the JSON encoding used to move it across the
/// Pcbnew kiface.
struct FOOTPRINT_MATCH_RESULT
{
    /// False if the query could not be performed, in which case MatchingNames is empty.
    bool m_Success = false;

    /// The matching "library:footprint" names.
    std::vector<wxString> m_MatchingNames;

    /// True if the result set is limited by the maximum number of results.
    bool m_IsLimited = false;

    /// Serialise to the JSON object that carries a result over the Pcbnew kiface.
    wxString ToJsonStr() const;

    /// Parse the JSON object written by ToJsonStr().  Anything else, including a result
    /// whose Success is false, yields a failed result with no names.
    static FOOTPRINT_MATCH_RESULT FromJsonStr( const wxString& aJson );
};


/**
 * Ask the footprint libraries, for the footprints matching a query (over the
 * Pcbnew kiface).
 *
 * @param aKiway is used to reach the pcbnew kiface.
 * @param aQuery is the set of criteria to match.
 *
 * @return the matching "library:footprint" IDs.  On failure Success is false and
 *         MatchingNames is empty, which happens when the pcbnew kiface is unavailable
 *         (for example in a standalone tool without footprint support) or the query
 *         could not be performed.
 */
FOOTPRINT_MATCH_RESULT QueryMatchingFootprints( KIWAY& aKiway, const FOOTPRINT_MATCH_QUERY& aQuery );


/**
 * Start loading the footprint libraries in the background.
 *
 * This call does not block. It is a no-op if the libraries are
 * already loaded, or a load is already running.
 *
 * @return true if the background load was successfully started,
 *         false otherwise.
 */
bool StartFootprintLibrariesLoad( KIWAY& aKiway );


/**
 * Return how far the background load of the footprint libraries has got.
 *
 * Never blocks, so it can be polled while showing a loading indication.
 *
 * @param aKiway is used to reach the pcbnew kiface.
 * @return the fraction of the queued libraries processed so far, or 1.0 when there is
 *         nothing to wait for (for example without the Pcbnew kiface).
 */
float GetFootprintLibrariesLoadProgress( KIWAY& aKiway );
