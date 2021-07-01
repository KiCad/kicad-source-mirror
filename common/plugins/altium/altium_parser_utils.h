/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020-2021 Thomas Pointhuber <thomas.pointhuber@gmx.at>
 * Copyright (C) 2020-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef ALTIUM_PARSER_UTILS_H
#define ALTIUM_PARSER_UTILS_H

#include <map>

#include <kicad_string.h>
#include <lib_id.h>

#include <iostream>

struct CASE_INSENSITIVE_COMPARATOR
{
    bool operator()( const wxString& s1, const wxString& s2 ) const
    {
        // Altium variables are case insensitive.
        return s1.CmpNoCase( s2 ) < 0;
    }
};

typedef std::map<wxString, wxString, CASE_INSENSITIVE_COMPARATOR> altium_override_map_t;

LIB_ID AltiumToKiCadLibID( wxString aLibName, wxString aLibReference );

wxString AltiumPropertyToKiCadString( const wxString& aString );

wxString AltiumSpecialStringsToKiCadVariables( const wxString&              aString,
                                               const altium_override_map_t& aOverride );

#endif //ALTIUM_PARSER_UTILS_H
