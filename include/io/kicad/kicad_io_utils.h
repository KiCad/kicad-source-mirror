/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef KICAD_IO_UTILS_H
#define KICAD_IO_UTILS_H

#include <wx/string.h>

class OUTPUTFORMATTER;
class KIID;

namespace KICAD_FORMAT {

/**
 * Writes a boolean to the formatter, in the style (aKey [yes|no])
 *
 * @param aOut is the output formatter to write to
 * @param aNestLevel is passed to the output formatter to control indentation
 * @param aKey is the name of the boolean flag
 * @param aValue is the value to write
 * @param aSuffix is the character to format after the end of the boolean (after the close paren)
 */
KICOMMON_API void FormatBool( OUTPUTFORMATTER* aOut, int aNestLevel, const wxString& aKey,
                              bool aValue, char aSuffix = 0 );

KICOMMON_API void FormatUuid( OUTPUTFORMATTER* aOut, int aNestLevel, const KIID& aUuid,
                              char aSuffix = 0 );

KICOMMON_API void Prettify( std::string& aSource, char aQuoteChar = '"' );

} // namespace KICAD_FORMAT

#endif //KICAD_IO_UTILS_H
