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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file mdw_bom_compare.h
 * Compares the design's symbols with a workspace BOM, keyed by reference designator.
 */

#ifndef MDW_BOM_COMPARE_H
#define MDW_BOM_COMPARE_H

#include <lib_id.h>
#include <mdw/client/mdw_bom.h>

#include <functional>
#include <optional>
#include <vector>

class SCH_REFERENCE;
class SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL;


enum class MDW_BOM_STATUS
{
    MATCHES,
    DIFFERENT_PART,
    PART_UNKNOWN,     ///< In both, but the design side's part could not be identified
    QUANTITY_DIFFERS, ///< A count of pieces that disagrees with the design or the line's own designators
    NOT_IN_WORKSPACE,
    NOT_IN_DESIGN
};


/// One designator of the design and what is known of the part behind it
struct MDW_DESIGN_SYMBOL
{
    wxString                    reference;
    wxString                    path;       ///< Sheet path and symbol UUID, as cross-probing finds it
    std::optional<MDW_ITEM_REF> part;       ///< The symbol's own part identity, when it carries one
    std::vector<MDW_ITEM_REF>   candidates; ///< Parts using its library member, when it carries none
    std::optional<MDW_ITEM_REF> member;     ///< Its library member, the key for finding candidates
    LIB_ID                      lib_id;     ///< The library symbol it places in the current variant
};


struct MDW_BOM_ROW
{
    MDW_BOM_STATUS        status = MDW_BOM_STATUS::MATCHES;
    std::vector<wxString> design_refs;
    std::vector<wxString> design_paths; ///< Every unit of every design reference
    std::vector<wxString> workspace_refs;
    int                   line = -1; ///< Index into MDW_BOM_DATA::lines, or -1
    double                workspace_quantity = 0.0;
};


/**
 * Compare aDesign, grouped as the fields table groups it, with aBom. A design group splits
 * into one row per status and BOM line. Lines without designators match symbols by part and
 * compare their count instead. Rows keep the design's group order, then list what only the
 * workspace has, in BOM order. aBomSource is the locator of the source that returned aBom; an
 * item with an empty source on either side belongs to it.
 */
std::vector<MDW_BOM_ROW> MdwCompareBom( const std::vector<std::vector<MDW_DESIGN_SYMBOL>>& aDesign,
                                        const MDW_BOM_DATA& aBom, const wxString& aBomSource );


/// What the comparison shows and counts
struct MDW_BOM_VIEW
{
    std::vector<size_t> shown;           ///< Indexes into the compared rows, in order
    int                 differences = 0; ///< Rows known to differ
    int                 unknown = 0;     ///< Rows whose part could not be identified
    int                 references = 0;  ///< Design references compared
};


/**
 * The rows to list for aRows. With aDifferencesOnly, only known differences are listed; a part
 * that could not be identified is neither a match nor a difference, so it is counted apart.
 */
MDW_BOM_VIEW MdwBomView( const std::vector<MDW_BOM_ROW>& aRows, bool aDifferencesOnly );


/// Fills in the part identity of one placed symbol
using MDW_SYMBOL_IDENTITY = std::function<void( const SCH_REFERENCE&, MDW_DESIGN_SYMBOL& )>;


/**
 * The design side of the comparison, one group per row of aModel as the Edit page groups it
 * for its variant. Every unit of a symbol is listed, and MdwCompareBom counts each reference once.
 */
std::vector<std::vector<MDW_DESIGN_SYMBOL>> MdwDesignGroups( SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL& aModel,
                                                             const MDW_SYMBOL_IDENTITY&            aIdentify );

#endif // MDW_BOM_COMPARE_H
