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

#ifndef NAME_VALIDATION_H
#define NAME_VALIDATION_H

#include <wx/string.h>
#include <kicommon.h>

/*
 * Single source of truth for the characters KiCad forbids in library-related names.
 * Filename rules are a strict superset of LIB_ID rules, so the filename list is built
 * from the LIB_ID list rather than hand-copied. Formerly the same characters were
 * duplicated in lib_id.cpp, footprint.cpp and validators.cpp and drifted apart.
 */

/**
 * Characters illegal in a LIB_ID item name. Library nicknames use a different set
 * (isLegalLibraryNameChar) and are not covered here.
 */
KICOMMON_API const wxString& GetLibIdForbiddenChars();

/**
 * Characters illegal in a footprint library filename. This is the LIB_ID set plus the
 * path and format characters that a name may contain but a filename may not.
 */
KICOMMON_API const wxString& GetLibFilenameForbiddenChars();

#endif // NAME_VALIDATION_H
