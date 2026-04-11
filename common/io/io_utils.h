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
* along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef IO_UTILS_H
#define IO_UTILS_H

#include <kicommon.h>
#include <cstdint>
#include <vector>
#include <optional>

#include <wx/string.h>

namespace IO_UTILS
{

extern KICOMMON_API const std::vector<uint8_t> COMPOUND_FILE_HEADER;

/**
 * Check if a file starts with a defined string.
 *
 * @param aFilePath path to the file where we want to check the prefix.
 * @param aPrefix prefix string which should match with the initial characters in the file.
 * @param aIgnoreWhitespace true if whitespace characters should be ignored before the prefix.
 */
KICOMMON_API bool fileStartsWithPrefix( const wxString& aFilePath, const wxString& aPrefix,
                                        bool aIgnoreWhitespace );

/**
 * Check if a file starts with a defined binary header.
 *
 * @param aFilePath path to the file where we want to check the prefix.
 * @param aHeader vector of bytes which need to match with the start of the file.
 * @param aOffset offset in the file where the header should be checked.
 */
KICOMMON_API bool fileHasBinaryHeader( const wxString&             aFilePath,
                                       const std::vector<uint8_t>& aHeader,
                                       size_t                      aOffset = 0 );

/**
 * Calculates an MMH3 hash of a given file. This is not a secure hash, use it
 * as a checksum, not as a secure digest!
 *
 * @param aFilePath path to the file where we want to calculate the hash for.
 * @return hash value as a hex string or none in case of an error.
 */
KICOMMON_API std::optional<wxString> fileHashMMH3( const wxString& aFilePath );

}


#endif // IO_UTILS_H
