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

#ifndef KICAD_DATABASE_AUTOFILL_H
#define KICAD_DATABASE_AUTOFILL_H

#include <any>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <kicommon.h>
#include <wx/string.h>

/**
 * Guesses the table and column mappings of a new database library from their names.
 *
 * Names are compared word by word, where words break at separators, digits and case changes, so
 * "PartID" and "part_id" both end in the word "id" while "Identical" and "Flacid" only contain it.
 */
namespace DB_AUTOFILL
{

/// One mapping to fill, such as the key column or the Value field
struct ROLE
{
    std::vector<wxString>    terms;      ///< Words to look for, in English and translated
    std::vector<std::string> remembered; ///< Columns the user chose before, most recent first
    std::set<std::string>    excluded;   ///< Columns this role must not take, even when remembered
};

/**
 * Chooses a column for each role, best scores first, so no column serves two roles.
 *
 * @return the chosen column per role, or an empty string where nothing matched
 */
KICOMMON_API std::vector<std::string> AssignColumns( const std::vector<std::string>& aColumns,
                                                     const std::vector<ROLE>& aRoles );

/// @return the table named like the library, or an empty string when none shares a whole word
KICOMMON_API std::string MatchTable( const std::vector<std::string>& aTables, const std::string& aLibraryName );

/**
 * Counts, per column, the rows whose value already appeared in an earlier row, with NULL counted
 * as empty.  A key column needs a count of zero, or parts sharing a key hide each other.
 */
KICOMMON_API std::map<std::string, size_t>
RepeatCounts( const std::vector<std::map<std::string, std::any>>& aRows );

/// Moves aColumn to the front of a role's history, dropping repeats and the oldest entries
KICOMMON_API void RememberChoice( std::vector<std::string>& aHistory, const std::string& aColumn );

} // namespace DB_AUTOFILL

#endif // KICAD_DATABASE_AUTOFILL_H
