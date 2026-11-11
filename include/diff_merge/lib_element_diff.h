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
 */

#ifndef LIB_ELEMENT_DIFF_H
#define LIB_ELEMENT_DIFF_H

#include <diff_merge/kicad_diff_types.h>

#include <functional>
#include <map>
#include <vector>
#include <wx/string.h>


class EDA_ITEM;


namespace KICAD_DIFF
{

/// One changed element of a library item, such as a pin, pad, field or graphic
struct LIB_ELEMENT
{
    const EDA_ITEM*             item; // after-side item, or before-side for REMOVED
    wxString                    typeName;
    wxString                    key;  // pin number or field name, empty for unkeyed elements
    CHANGE_KIND                 kind; // ADDED, REMOVED or MODIFIED
    std::vector<PROPERTY_DELTA> deltas;
};

/// Elements that are matched by a key, such as pins by number or fields by name.
void DiffKeyedElements( std::vector<LIB_ELEMENT>& aOut, const wxString& aTypeName,
                        const std::map<wxString, const EDA_ITEM*>& aBefore,
                        const std::map<wxString, const EDA_ITEM*>& aAfter );

/**
 * Elements with no key. Identical elements are matched first, then each leftover is paired with
 * the nearest leftover of its type so an edited element reads as modified. Elements are identical
 * when no property differs and, if given, aSameGeometry holds for geometry that is no property.
 */
void DiffUnkeyedElements( std::vector<LIB_ELEMENT>& aOut, const std::vector<const EDA_ITEM*>& aBefore,
                          const std::vector<const EDA_ITEM*>&                            aAfter,
                          const std::function<wxString( const EDA_ITEM* )>&              aTypeName,
                          const std::function<bool( const EDA_ITEM*, const EDA_ITEM* )>& aSameGeometry = {} );

/// Add each element to aChange as a child change, which is how a library item reports its elements.
void AppendElementChanges( ITEM_CHANGE& aChange, const std::vector<LIB_ELEMENT>& aElements );

} // namespace KICAD_DIFF

#endif // LIB_ELEMENT_DIFF_H
