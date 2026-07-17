/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
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

/**
 * @file kigit_symbol_split.h
 * @brief The top-level symbols of a .kicad_sym file, found in its text without parsing them.
 *
 * The backend and the client share it, so both find where each symbol starts and ends the same way.
 */

#ifndef KIGIT_SYMBOL_SPLIT_H
#define KIGIT_SYMBOL_SPLIT_H

#include <cstddef>
#include <map>
#include <string>

#include <import_export.h>

namespace KIGIT
{

/// A .kicad_sym's top-level symbols by name, with where each sits in the text and the text around them
struct SYMBOL_FILE
{
    struct SPAN
    {
        std::size_t start = 0;
        std::size_t length = 0;
        bool        derived = false; ///< It extends another symbol
    };

    std::string                 header; ///< The text before the first symbol
    std::string                 frame;  ///< The text outside every symbol
    std::map<std::string, SPAN> spans;
    std::size_t                 close = std::string::npos; ///< Where the library's own list closes
    bool                        complete = true; ///< False when the text is not a list of uniquely named symbols
};

/// The top-level (symbol "NAME" ...) lists of aText. A line starting with '#' is a comment, as DSNLEXER reads it.
SYMBOL_FILE SplitSymbols( const std::string& aText );

/// The file format version the header of .kicad_sym text aText names, or empty when it names none
std::string FormatVersion( const std::string& aText );

/// True when format version aVersion, as FormatVersion() gives it, is newer than aThan
bool NewerFormat( const std::string& aVersion, const std::string& aThan );

/// The git blob id of aText, which a client and a backend reading the same bytes agree on
APIEXPORT std::string BlobDigest( const std::string& aText );

} // namespace KIGIT

#endif // KIGIT_SYMBOL_SPLIT_H
