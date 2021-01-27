/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright (C) 2019-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef PL_SELECTION_TOOL_H
#define PL_SELECTION_TOOL_H

#include <tool/tool_interactive.h>
#include <tool/tool_menu.h>
#include "tools/pl_selection.h"

class PL_EDITOR_FRAME;
class SCH_ITEM;
class COLLECTOR;

namespace KIGFX
{
    class GAL;
}


class PL_SELECTION_TOOL : public TOOL_INTERACTIVE
{
public:
    PL_SELECTION_TOOL();
    ~PL_SELECTION_TOOL() override { }

    /// @copydoc TOOL_BASE::Init()
    bool Init() override;

    /// @copydoc TOOL_INTERACTIVE::Reset()
    void Reset( RESET_REASON aReason ) override;

    int UpdateMenu( const TOOL_EVENT& aEvent );

    /**
     * The main loop.
     */
    int Main( const TOOL_EVENT& aEvent );

    /**
     * Return the set of currently selected items.
     */
    PL_SELECTION& GetSelection();

    /**
     * Return either an existing selection (filtered), or the selection at the current
     * cursor if the existing selection is empty.
     */
    PL_SELECTION& RequestSelection();

    /**
     * Select an item pointed by the parameter aWhere. If there is more than one item at that
     * place, there is a menu displayed that allows one to choose the item.
     *
     * @param aWhere is the place where the item should be selected.
     * @param aSelectionCancelledFlag allows the function to inform its caller that a selection
     *                                was canceled (for instance, by clicking outside of the
     *                                disambiguation menu).
     */
    void SelectPoint( const VECTOR2I& aWhere, bool* aSelectionCancelledFlag = nullptr );

    int AddItemToSel( const TOOL_EVENT& aEvent );
    void AddItemToSel( EDA_ITEM* aItem, bool aQuietMode = false );
    int AddItemsToSel( const TOOL_EVENT& aEvent );
    void AddItemsToSel( EDA_ITEMS* aList, bool aQuietMode = false );

    int RemoveItemFromSel( const TOOL_EVENT& aEvent );
    void RemoveItemFromSel( EDA_ITEM* aItem, bool aQuietMode = false );
    int RemoveItemsFromSel( const TOOL_EVENT& aEvent );
    void RemoveItemsFromSel( EDA_ITEMS* aList, bool aQuietMode = false );

    void BrightenItem( EDA_ITEM* aItem );
    void UnbrightenItem( EDA_ITEM* aItem );

    int ClearSelection( const TOOL_EVENT& aEvent );
    void ClearSelection();

    /**
     * Rebuild the selection from the flags in the view items.  Useful after a hard redraw
     * or an undo or redo operation.
     */
    void RebuildSelection();

    /**
     * Shows a popup menu to trim the COLLECTOR passed as aEvent's parameter down to a single
     * item.
     *
     * @note This routine **does not** modify the selection.
     */
    int SelectionMenu( const TOOL_EVENT& aEvent );

private:
    /**
     * Handle drawing a selection box that allows one to select many items at the same time.
     *
     * @return true if the function was canceled (i.e. CancelEvent was received).
     */
    bool selectMultiple();

    /**
     * Apply heuristics to try and determine a single object when multiple are found under the
     * cursor.
     */
    void guessSelectionCandidates( COLLECTOR& collector, const VECTOR2I& aWhere );

    /**
     * Allow the selection of a single item from a list via pop-up menu.  The items are
     * highlighted on the canvas when hovered in the menu.  The collector is trimmed to
     * the picked item.
     *
     * @return true if an item was picked.
     */
    bool doSelectionMenu( COLLECTOR* aItems );

    /**
     * Takes necessary action mark an item as selected.
     *
     * @param aItem is an item to be selected.
     */
    void select( EDA_ITEM* aItem );

    /**
     * Take necessary action mark an item as unselected.
     *
     * @param aItem is an item to be unselected.
     */
    void unselect( EDA_ITEM* aItem );

    /**
     * Highlight the item visually.
     *
     * @param aItem is an item to be be highlighted.
     * @param aHighlightMode should be either SELECTED or BRIGHTENED
     * @param aGroup is the group to add the item to in the BRIGHTENED mode.
     */
    void highlight( EDA_ITEM* aItem, int aHighlightMode, PL_SELECTION* aGroup = nullptr );

    /**
     * Unhighlight the item visually.
     *
     * @param aItem is an item to be be highlighted.
     * @param aHighlightMode should be either SELECTED or BRIGHTENED
     * @param aGroup is the group to remove the item from.
     */
    void unhighlight( EDA_ITEM* aItem, int aHighlightMode, PL_SELECTION* aGroup = nullptr );

    /**
     * @return True if the given point is contained in any of selected items' bounding box.
     */
    bool selectionContains( const VECTOR2I& aPoint ) const;

    ///< Set up handlers for various events.
    void setTransitions() override;

private:
    PL_EDITOR_FRAME* m_frame;   // Pointer to the parent frame
    PL_SELECTION m_selection;   // Current state of selection

    bool m_additive;            // Items should be added to selection (instead of replacing)
    bool m_subtractive;         // Items should be removed from selection
    bool m_exclusive_or;        // Items' selection state should be toggled
    bool m_multiple;            // Multiple selection mode is active
    bool m_skip_heuristics;     // Heuristics are not allowed when choosing item under cursor
};

#endif //PL_SELECTION_TOOL_H
