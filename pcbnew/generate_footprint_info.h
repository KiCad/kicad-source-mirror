/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Chris Pavlina <pavlina.chris@gmail.com>
 * Copyright (C) 2017 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef GENERATE_FOOTPRINT_INFO_H
#define GENERATE_FOOTPRINT_INFO_H

#include <wx/string.h>

class LIB_ID;
class FP_LIB_TABLE;

/**
 * Return an HTML page describing a #LIB_ID in a #FP_LIB_TABLE. This is suitable for inclusion
 * in a wxHtmlWindow.
 */
wxString GenerateFootprintInfo( FP_LIB_TABLE* aFpLibTable, LIB_ID const& aLibId );

#endif // GENERATE_FOOTPRINT_INFO_H
