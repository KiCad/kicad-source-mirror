/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Chris Pavlina <pavlina.chris@gmail.com>
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

#ifndef GENERATE_ALIAS_INFO_H
#define GENERATE_ALIAS_INFO_H

#include <wx/string.h>

class LIB_ID;
class SYMBOL_LIBRARY_MANAGER_ADAPTER;

/**
 * Return an HTML page describing a #LIB_ID in a #SYMBOL_LIB_TABLE. This is suitable for inclusion
 * in a HTML_WINDOW (wxHtmlWindow).
 */
wxString GenerateAliasInfo( SYMBOL_LIBRARY_MANAGER_ADAPTER* aLibs, LIB_ID const& aLibId, int aUnit );

#endif // GENERATE_ALIAS_INFO_H
