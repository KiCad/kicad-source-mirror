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

#include <lib_id.h>
#include <wx/string.h>


class KIWAY;


/**
 * All the information needed to query the symbol libraries for the symbols using a footprint.
 */
struct FOOTPRINT_USERS_QUERY
{
    /// The footprint to find the users of.  A footprint with no library nickname still
    /// matches references that name it without one.
    LIB_ID m_Footprint;

    /// When true, symbols whose footprint filters select the footprint are returned.
    bool m_MatchFilters = true;

    /// When true, symbols whose footprint field names the footprint are returned.
    bool m_MatchFootprintField = true;

    /// When true, symbols whose pin maps are bound to the footprint are returned.
    bool m_MatchPinMaps = true;

    /// Limits the number of returned symbols; zero means no limit.
    int m_MaxResults = 400;

    /// Serialise to the JSON object that carries a query over the Eeschema kiface.
    wxString ToJsonStr() const;

    /// Parse the JSON object written by ToJsonStr().
    static FOOTPRINT_USERS_QUERY FromJsonStr( const wxString& aJson );
};


/**
 * One symbol that uses a footprint, and the reason it matched.
 */
struct FOOTPRINT_USER_MATCH
{
    /// The "library:symbol" name of the matching symbol.
    LIB_ID m_Symbol;

    /// The number of pins of the matching symbol
    int m_PinCount = 0;

    /// True when the symbol is derived from another symbol rather than being a root symbol.
    bool m_IsDerived = false;

    /// The footprint filter that selected the footprint, empty when the match did not come
    /// from a filter.
    wxString m_MatchedFilter;

    /// True when the symbol's footprint field names the footprint.
    bool m_MatchesFootprintField = false;

    /// The names of the symbol's pin maps that are bound to the footprint, in the order they
    /// were found; empty when none is.
    std::vector<wxString> m_MatchedPinMaps;
};


/// The outcome of a footprint users query, and the JSON encoding used to move it across the
/// Eeschema kiface.
struct FOOTPRINT_USERS_RESULT
{
    /// False if the query could not be performed, in which case Matches is empty.
    bool m_Success = false;

    /// The matching symbols, in the order the libraries were searched.
    std::vector<FOOTPRINT_USER_MATCH> m_Matches;

    /// True if the result set is limited by the maximum number of results.
    bool m_IsLimited = false;

    /// Serialise to the JSON object that carries a result over the Eeschema kiface.
    wxString ToJsonStr() const;

    /// Parse the JSON object written by ToJsonStr().
    static FOOTPRINT_USERS_RESULT FromJsonStr( const wxString& aJson );
};


/**
 * Ask the symbol libraries, for the symbols that use a given footprint (over the
 * Eeschema kiface).
 *
 * @param aKiway is used to reach the eeschema kiface.
 * @param aQuery is the footprint to look for and the kind of matches to return.
 *
 * @return the matching "library:symbol" IDs.  On failure Success is false and
 *         MatchingNames is empty, which happens when the eeschema kiface is unavailable
 *         (for example in a standalone tool without symbol support) or the query
 *         could not be performed.
 */
FOOTPRINT_USERS_RESULT QueryFootprintUsers( KIWAY& aKiway, const FOOTPRINT_USERS_QUERY& aQuery );


/**
 * Start loading the symbol libraries in the background.
 *
 * This call does not block.  It is a no-op if the libraries are
 * already loaded, or a load is already running.
 *
 * @return true if the background load was successfully started,
 *         false otherwise.
 */
bool StartSymbolLibrariesLoad( KIWAY& aKiway );


/**
 * Return how far the background load of the symbol libraries has got.
 *
 * Never blocks, so it can be polled while showing a loading indication.
 *
 * @param aKiway is used to reach the eeschema kiface.
 * @return the fraction of the queued libraries processed so far, or 1.0 when there is
 *         nothing to wait for (for example without the Eeschema kiface).
 */
float GetSymbolLibrariesLoadProgress( KIWAY& aKiway );
