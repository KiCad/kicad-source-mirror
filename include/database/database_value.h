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

#ifndef KICAD_DATABASE_VALUE_H
#define KICAD_DATABASE_VALUE_H

#include <cstdint>
#include <string>
#include <variant>

/// A single database cell, where monostate is SQL NULL
using DB_VALUE = std::variant<std::monostate, std::string, int64_t, double, bool>;

/**
 * Convert a database value to the text KiCad stores for it.
 *
 * Doubles use the shortest text that reads back to the same value, booleans become "1" or "0" and
 * NULL becomes an empty string.  Strings are returned unchanged.
 */
std::string DatabaseValueToString( const DB_VALUE& aValue );

/// A C string would silently convert to bool, so the caller must build a std::string
std::string DatabaseValueToString( const char* ) = delete;

/**
 * Trim redundant trailing zeros and a dangling decimal point from decimal text, for example
 * "4700000.0000" becomes "4700000", and add the missing zero to ".5".  A leading sign is kept except
 * that a zero value with a decimal point becomes "0".  Text without a decimal point or that is not
 * plain decimal is returned unchanged.
 */
std::string NormalizeDecimalText( const std::string& aText );

#endif // KICAD_DATABASE_VALUE_H
