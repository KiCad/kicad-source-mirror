/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#ifndef QA_EESCHEMA_TIMESTAMP_TEST_UTILS__H
#define QA_EESCHEMA_TIMESTAMP_TEST_UTILS__H

#include <algorithm>
#include <string>

/**
 * @file
 * Test utilities for timestamps
 */

namespace KI_TEST
{

/**
 * Predicate for checking a timestamp character
 * @param  aChr the character
 * @return      true if it's a valid timestamp char (0-9, A-F)
 */
inline bool IsTimeStampChar( char aChr )
{
    return ( aChr >= 'A' && aChr <= 'F' ) || ( aChr >= '0' && aChr <= '9' );
}

/**
 * Check if the string between the iterators looks like a timestamp (i.e. 8 hex digits)
 */
template <typename T>
bool IsTimeStampish( const T& aBegin, const T& aEnd )
{
    // Wrong length
    if( aEnd != aBegin + 8 )
        return false;

    // Check all chars
    return std::all_of( aBegin, aEnd, IsTimeStampChar );
}

/**
 * Predicate to check if a string look like it ends in a timestamp
 * @param  aStr the string to check
 * @return      true if it does
 */
bool EndsInTimestamp( const std::string& aStr );

/**
 * Predicate to check a string is a timestmap path format
 *
 * Eg. levels=2: /1234ABCD/9878DEFC/
 *
 * @param  aStr   candidate string
 * @param  levels expected levels
 * @return        true if format matches
 */
bool IsTimestampStringWithLevels( const std::string& aStr, unsigned aLevels );

} // namespace KI_TEST

#endif // QA_EESCHEMA_TIMESTAMP_TEST_UTILS__H