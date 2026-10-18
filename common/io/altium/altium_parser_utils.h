/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020-2021 Thomas Pointhuber <thomas.pointhuber@gmx.at>
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

#ifndef ALTIUM_PARSER_UTILS_H
#define ALTIUM_PARSER_UTILS_H

#include <map>

#include <string_utils.h>
#include <lib_id.h>

#include <iostream>
#include <math/vector2d.h>
#include <set>

LIB_ID AltiumToKiCadLibID( const wxString& aLibName, const wxString& aLibReference );

wxString AltiumPropertyToKiCadString( const wxString& aString );

/**
 * Escape the KiCad variables and expressions in literal Altium text, which Altium shows as typed.
 */
wxString AltiumEscapeTextVars( const wxString& aText );

/**
 * Convert the special strings in Altium schematic text to KiCad variables.
 *
 * Literal text is escaped, since Altium shows a ${ or @{ in it as typed.
 *
 * @param[out] aUnmapped, when not null, collects each variable that is not an override.
 */
wxString AltiumSchSpecialStringsToKiCadVariables( const wxString&                     aString,
                                                  const std::map<wxString, wxString>& aOverrides,
                                                  std::set<wxString>*                 aUnmapped = nullptr );

/**
 * Convert the special strings in Altium PCB text to KiCad variables.
 *
 * @param[out] aUnmapped, when not null, collects each variable that is not an override, which KiCad
 *             cannot resolve by itself.
 */
wxString AltiumPcbSpecialStringsToKiCadStrings( const wxString&                     aString,
                                                const std::map<wxString, wxString>& aOverrides,
                                                std::set<wxString>*                 aUnmapped = nullptr );

wxString AltiumPinNamesToKiCad( wxString& aString );

/**
 * Convert an Altium pin designator string to the equivalent KiCad pin number.
 *
 * Altium represents a pin that electrically ties multiple physical pads by placing all
 * designators in a single comma-separated string (e.g. "1,2,3" or "1, 2, 3").  KiCad
 * represents the same construct with stacked-pin bracket notation (e.g. "[1,2,3]").  The
 * input string is always stripped of surrounding whitespace; if it contains no comma it
 * is returned after that trim.
 */
wxString AltiumPinDesignatorToKiCad( const wxString& aDesignator );

VECTOR2I AltiumGetEllipticalPos( double aMajor, double aMinor, double aAngleRadians );
#endif //ALTIUM_PARSER_UTILS_H
