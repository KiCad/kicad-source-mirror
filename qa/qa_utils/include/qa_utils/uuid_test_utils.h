/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#ifndef QA_UUID_TEST_UTILS__H
#define QA_UUID_TEST_UTILS__H

#include <algorithm>
#include <string>
#include <regex>

/**
 * @file
 * Test utilities for timestamps
 */

namespace KI_TEST
{

/**
 * Check if the string between the iterators looks like a UUID
 */
bool IsUUID( const std::string& aStr );


/**
 * Predicate to check a string is a UUID path format
 *
 * Eg. levels=2: /1d33ca6f-67e8-41ae-a0aa-49d857ab38d5/1d33ca6f-67e8-41ae-a0aa-49d857ab38d5/
 *
 * @param  aStr   candidate string
 * @param  levels expected levels
 * @return        true if format matches
 */
bool IsUUIDPathWithLevels( const std::string& aStr, unsigned aLevels );

} // namespace KI_TEST

#endif // QA_UUID_TEST_UTILS__H