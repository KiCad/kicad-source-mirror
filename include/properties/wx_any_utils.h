/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef WX_ANY_UTILS_H
#define WX_ANY_UTILS_H

#include <kicommon.h>

#include <wx/any.h>


class PROPERTY_BASE;


/**
 * Compare two wxAny values for equality across the KiCad property type set.
 *
 * This is a general-purpose comparator that dispatches over the scalar, string,
 * geometry, color, identifier, and layer types that PROPERTY_BASE getters can
 * carry. Two wxAny values are equal only when they hold the same supported type
 * and that type's operator== reports equality; mismatched types compare unequal.
 * An unsupported type pair is conservatively reported as not equal.
 *
 * Passing the optional property lets enum-backed values be compared via the
 * wxPGChoices integer payload (or label fallback), which is the only way to
 * compare two PROPERTY_ENUM getters generically, since their wxAny stores the
 * enum type rather than int.
 */
KICOMMON_API bool KiWxAnyEquals( const wxAny& aA, const wxAny& aB,
                                 const PROPERTY_BASE* aProperty = nullptr );

#endif // WX_ANY_UTILS_H
