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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <optional>

#include <wx/stream.h>
#include <wx/string.h>

#include <kicommon.h>

class OUTPUTFORMATTER;
class KIID;

namespace KICAD_FORMAT {

/**
 * Writes a boolean to the formatter, in the style (aKey [yes|no])
 *
 * @param aOut is the output formatter to write to
 * @param aKey is the name of the boolean flag
 * @param aValue is the value to write
 */
KICOMMON_API void FormatBool( OUTPUTFORMATTER* aOut, const wxString& aKey, bool aValue );

/**
 * Writes an optional boolean to the formatter.
 * If a value is present, calls FormatBool.
 * If no value is present, Writes (aKey none).
 * 
 * @param aOut is the output formatter to write to
 * @param aKey is the name of the boolean flag
 * @param aValue is the value to write
 */
KICOMMON_API void FormatOptBool( OUTPUTFORMATTER* aOut, const wxString& aKey,
                                 std::optional<bool> aValue );

KICOMMON_API void FormatUuid( OUTPUTFORMATTER* aOut, const KIID& aUuid );

/**
 * Write binary data to the formatter as base 64 encoded string.
 */
KICOMMON_API void FormatStreamData( OUTPUTFORMATTER& aOut, const wxStreamBuffer& aStream );

/**
 * Controls the pretty-printing mode used by Prettify
 */
enum class KICOMMON_API FORMAT_MODE
{
    NORMAL,                     ///< Follows standard pretty-printing rules
    COMPACT_TEXT_PROPERTIES,    ///< Collapses certain text properties to single-line
    LIBRARY_TABLE               ///< Puts library table rows on a single line
};

/**
 * Pretty-prints s-expression text according to KiCad format rules
 *
 * Formatting rules:
 * - All extra (non-indentation) whitespace is trimmed
 * - Indentation is one tab
 * - Starting a new list (open paren) starts a new line with one deeper indentation
 * - Lists with no inner lists go on a single line
 * - End of multi-line lists (close paren) goes on a single line at same indentation as its start
 *
 * For example:
 * (first
 *  (second
 *   (third list)
 *   (another list)
 *  )
 *  (fifth)
 *  (sixth thing with lots of tokens
 *   (and a sub list)
 *  )
 * )
 */
KICOMMON_API void Prettify( std::string& aSource, FORMAT_MODE aMode = FORMAT_MODE::NORMAL );

} // namespace KICAD_FORMAT
