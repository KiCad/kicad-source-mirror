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

#ifndef SYM_ITEM_DIFF_H
#define SYM_ITEM_DIFF_H

#include <diff_merge/kicad_diff_types.h>
#include <diff_merge/lib_element_diff.h>

#include <memory>
#include <set>
#include <vector>
#include <wx/string.h>


class LIB_SYMBOL;
class EDA_ITEM;


namespace KICAD_DIFF
{

/**
 * Compares the elements of two library symbols. Pins match by number and
 * fields by name. Graphics and text first match identical shapes, then pair up
 * the leftovers so an edited shape reads as modified. Only changed elements are
 * returned. Either symbol may be null for a whole symbol add or remove.
 */
std::vector<LIB_ELEMENT> DiffSymbolElements( const LIB_SYMBOL* aBefore, const LIB_SYMBOL* aAfter );

/**
 * Combine two edits of one library symbol when neither can lose anything. One side may have
 * changed anything, the other only fields and symbol properties, and no name may be touched by
 * both. The field-only side's edits must also rebuild that side from aAncestor exactly, so state
 * no diff reports is never dropped, and must leave every edit of the other side in place. Returns
 * the first side with the other side's edits applied, or null when the edits must be resolved by
 * hand.
 */
std::unique_ptr<LIB_SYMBOL> CombineSymbolEdits( const LIB_SYMBOL& aAncestor, const LIB_SYMBOL& aOurs,
                                                const LIB_SYMBOL& aTheirs, const ITEM_CHANGE& aOurChange,
                                                const ITEM_CHANGE& aTheirChange );

} // namespace KICAD_DIFF

#endif // SYM_ITEM_DIFF_H
