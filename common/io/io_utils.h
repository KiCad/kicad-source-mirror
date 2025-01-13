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

#ifndef IO_UTILS_H
#define IO_UTILS_H

#include <kicommon.h>
#include <cstdint>
#include <vector>

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
 */
KICOMMON_API bool fileStartsWithBinaryHeader( const wxString&             aFilePath,
                                              const std::vector<uint8_t>& aHeader );

}


#endif // IO_UTILS_H
