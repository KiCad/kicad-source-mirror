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

#pragma once

#include <optional>

#include <wx/string.h>

#include <kicommon.h>

/**
 * Generic string incrementer.
 */
KICOMMON_API bool IncrementString( wxString& aStr, int aDelta );


/**
 * Heuristically increment a string's n'th part from the right.
 *
 * For example: incrementing the 0th part of A1 -> A2
 *                               1st part of A1 -> B1
 *
 * This is a bit subjective as to what represents suitable
 * "incrementable" parts, but it tries to be smart about it.
 */
class KICOMMON_API STRING_INCREMENTER
{
public:
    /**
     * If a alphabetic part is found, skip the letters I, O, S, Q, X, Z.
     * (if one is already there, increment it anyway).
     */
    void SetSkipIOSQXZ( bool aSkip ) { m_SkipIOSQXZ = aSkip; }

    /**
     * Set the maximum index for alphabetic parts.
     *
     * This means that if the index is greater than this, it will be treated
     * as un-incrementable. This is to avoid incrementing things like "TX" or
     * "CAN", which would be indexes of hundreds (unlikely to be a BGA row prefix,
     * for example).
     *
     * Setting < 0 disables the check (no limit)
     */
    void SetAlphabeticMaxIndex( int aMaxIndex ) { m_AlphabeticMaxIndex = aMaxIndex; }

    /**
     * Increment the n-th part from the right of the given string.
     */
    std::optional<wxString> Increment( const wxString& aStr, int aDelta, size_t aRightIndex ) const;

private:
    enum class STRING_PART_TYPE
    {
        ALPHABETIC,
        INTEGER,
        SKIP,
    };

    bool incrementPart( wxString& aPart, STRING_PART_TYPE aType, int aDelta ) const;

    bool m_SkipIOSQXZ = true;
    int  m_AlphabeticMaxIndex = 50;
};

/**
 * Attempt to convert a string to an integer, assuming it is an alphabetic
 * string like "A", "B", ... "Z", "AA", "AB", ... "ZZ", "AAA", ... in some
 * alphabet.
 *
 * @return The value of the string, or -1 if a character is
 *         not in the alphabet.
 */
KICOMMON_API int IndexFromAlphabetic( const wxString& aStr, const wxString& aAlphabet );

/**
 * Get an alphabetic string like A, B, ... Z, AA, AB, ... ZZ, AAA, ...
 *
 * @param aIndex    The index to convert.
 * @param aAlphabet The alphabet to use.
 * @param aZeroBasedNonUnitCols If true, cols other than the right most use the 0'th entry
 *                              (e.g. Z -> AA, not BA, but 9 -> 10, not 00).
 */
KICOMMON_API wxString AlphabeticFromIndex( size_t aN, const wxString& aAlphabet,
                                           bool aZeroBasedNonUnitCols );