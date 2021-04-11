/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright (C) 2019-2020 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#ifndef EE_SELECTION_H
#define EE_SELECTION_H

class SCH_REFERENCE_LIST;
class SCH_SCREEN;
class SCH_SHEET_PATH;

#include <tool/selection.h>
#include <sch_sheet_path.h> // SCH_MULTI_UNIT_REFERENCE_MAP


class EE_SELECTION : public SELECTION
{
    /**
     * Screen of selected objects.  Used to fetch library symbols for copy.
     */
    SCH_SCREEN* m_screen;

public:
    EE_SELECTION( SCH_SCREEN* aScreen = nullptr );

    EDA_ITEM* GetTopLeftItem( bool onlyModules = false ) const override;

    EDA_RECT GetBoundingBox() const override;

    void SetScreen( SCH_SCREEN* aScreen ) { m_screen = aScreen; }
    SCH_SCREEN* GetScreen() { return m_screen; }

    /**
     * Adds #SCH_REFERENCE object to \a aReferences for each symbol in the selection.
     *
     * @param aReferences List of references to populate.
     * @param aSelectionPath The path to the sheet containing this selection.
     * @param aIncludePowerSymbols set to false to only get normal symbols.
     * @param aForceIncludeOrphanSymbols set to true to include symbols having no symbol found
     *                                   in lib.   The normal option is false, and set to true
     *                                   only to build the full list of symbols.
     */
    void GetSymbols( SCH_REFERENCE_LIST& aReferences, const SCH_SHEET_PATH& aSelectionPath,
                     bool aIncludePowerSymbols = true, bool aForceIncludeOrphanSymbols = false );

    /**
     * Add a #SCH_REFERENCE_LIST object to \a aRefList for each same-reference set of
     * multi-unit parts in the selection.
     *
     * The map key for each element will be the reference designator.
     *
     * @param aRefList Map of reference designators to reference lists
     * @param aSelectionPath The path to the sheet containing this selection.
     * @param aIncludePowerSymbols Set to false to only get normal symbols.
     */
    void GetMultiUnitSymbols( SCH_MULTI_UNIT_REFERENCE_MAP& aRefList,
                              const SCH_SHEET_PATH& aSelectionPath,
                              bool aIncludePowerSymbols = true );

    /**
     * Checks if all items in the selection support line strokes
     *
     * @return True if all items support line strokes
     */
    bool AllItemsHaveLineStroke() const;
};

#endif  //  EE_SELECTION_H
