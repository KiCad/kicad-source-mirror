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
 * @param aKey is the name of the boolean flag
 * @param aValue is the value to write
 */
KICOMMON_API void FormatBool( OUTPUTFORMATTER* aOut, const wxString& aKey, bool aValue );

KICOMMON_API void FormatUuid( OUTPUTFORMATTER* aOut, const KIID& aUuid );

KICOMMON_API void Prettify( std::string& aSource, bool aCompactSave );

} // namespace KICAD_FORMAT

#endif //KICAD_IO_UTILS_H
