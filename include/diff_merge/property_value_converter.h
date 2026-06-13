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

#ifndef KICAD_DIFF_PROPERTY_VALUE_CONVERTER_H
#define KICAD_DIFF_PROPERTY_VALUE_CONVERTER_H

#include <kicommon.h>

#include <diff_merge/kicad_diff_types.h>

#include <wx/any.h>


class PROPERTY_BASE;


namespace KICAD_DIFF
{

/**
 * Convert a wxAny value read from a PROPERTY_BASE getter into a DIFF_VALUE that
 * the engine can store, serialize, and compare.
 *
 * The aProperty argument is consulted when needed to interpret enum-backed
 * values (so the integer payload can be paired with the choice label) and for
 * coordinate-typed properties.
 *
 * Returns a DIFF_VALUE::T::NONE if the wxAny is empty or carries an unsupported
 * type; the differ can use that as a signal to omit the property from its delta
 * rather than producing a bogus comparison.
 */
KICOMMON_API DIFF_VALUE WxAnyToDiffValue( const wxAny& aValue,
                                          PROPERTY_BASE* aProperty = nullptr );


/**
 * Convert a DIFF_VALUE back into a wxAny a PROPERTY_BASE setter can consume.
 *
 * This is the inverse direction of WxAnyToDiffValue, used by the merge
 * appliers when a CUSTOM resolution carries a user-edited value that must be
 * written onto the merged item.
 *
 * Returns false (leaving @p aOut untouched) for DIFF_VALUE::T::NONE, which
 * carries no value; returns true for every other variant after populating
 * @p aOut with the matching wxAny payload.
 */
KICOMMON_API bool DiffValueToWxAny( const DIFF_VALUE& aValue, wxAny& aOut );

} // namespace KICAD_DIFF

#endif // KICAD_DIFF_PROPERTY_VALUE_CONVERTER_H
