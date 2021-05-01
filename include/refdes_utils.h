/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2021 KiCad Developers, see CHANGELOG.TXT for contributors.
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

/**
 * @file
 * Collection of utility functions for component reference designators (refdes)
 */

#ifndef REFDES_UTILS__H
#define REFDES_UTILS__H

#include <wx/string.h>

namespace UTIL
{

/**
 * Get the (non-numeric) prefix from a refdes - e.g.
 *      R1    -> R
 *      IC34  -> IC
 *      U?    -> U
 * @param  aRefDes full refdes
 * @return         the prefix, or empty string if nothing found
 */
wxString GetRefDesPrefix( const wxString& aRefDes );

/**
 * Return an unannotated refdes from either a prefix or an existing refdes.
 *      R    -> R?
 *      IC34 -> IC?
 *      U?   -> U?
 * @param aRefDes
 * @return
 */
wxString GetRefDesUnannotated( const wxString& aRefDes );

/**
 * Get the numeric suffix from a refdes - e.g.
 *      R1    -> 1
 *      IC34  -> 34
 *      R?    -> -1
 * @param  aRefDes full refdes
 * @return         the suffix, or -1 if nothing found
 */
int GetRefDesNumber( const wxString& aRefDes );

/**
 * Acts just like the strcmp function but treats numbers within the string text
 * correctly for sorting.  eg. A10 > A2
 * return -1 if first string is less than the second
 * return 0 if the strings are equal
 * return 1 if the first string is greater than the second
 */
int RefDesStringCompare( const wxString& lhs, const wxString& rhs );

} // namespace UTIL

#endif // REFDES_UTILS__H