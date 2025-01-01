/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
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

#ifndef PL_SELECTION_TOOL_H
#define PL_SELECTION_TOOL_H

#include <tool/selection_tool.h>
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


class PL_SELECTION_TOOL : public SELECTION_TOOL
{
public:
    PL_SELECTION_TOOL();
    ~PL_SELECTION_TOOL() override { }

    /// @copydoc TOOL_BASE::Init()
    bool Init() override;

    /// @copydoc TOOL_INTERACTIVE::Reset()
    void Reset( RESET_REASON aReason ) override;

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

    int ClearSelection( const TOOL_EVENT& aEvent );
    void ClearSelection();

    /**
     * Rebuild the selection from the flags in the view items.  Useful after a hard redraw
     * or an undo or redo operation.
     */
    void RebuildSelection();

protected:
    SELECTION& selection() override { return m_selection; }

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
     * Handle disambiguation actions including displaying the menu.
     */
    int disambiguateCursor( const TOOL_EVENT& aEvent );

    /**
     * Takes necessary action mark an item as selected.
     *
     * @param aItem is an item to be selected.
     */
    void select( EDA_ITEM* aItem ) override;

    /**
     * Take necessary action mark an item as unselected.
     *
     * @param aItem is an item to be unselected.
     */
    void unselect( EDA_ITEM* aItem ) override;

    /**
     * Highlight the item visually.
     *
     * @param aItem is an item to be be highlighted.
     * @param aHighlightMode should be either SELECTED or BRIGHTENED
     * @param aGroup is the group to add the item to in the BRIGHTENED mode.
     */
    void highlight( EDA_ITEM* aItem, int aHighlightMode, SELECTION* aGroup = nullptr ) override;

    /**
     * Unhighlight the item visually.
     *
     * @param aItem is an item to be be highlighted.
     * @param aHighlightMode should be either SELECTED or BRIGHTENED
     * @param aGroup is the group to remove the item from.
     */
    void unhighlight( EDA_ITEM* aItem, int aHighlightMode, SELECTION* aGroup = nullptr ) override;

    /**
     * @return True if the given point is contained in any of selected items' bounding box.
     */
    bool selectionContains( const VECTOR2I& aPoint ) const;

    ///< Set up handlers for various events.
    void setTransitions() override;

private:
    PL_EDITOR_FRAME* m_frame;   // Pointer to the parent frame
    PL_SELECTION m_selection;   // Current state of selection
};

#endif //PL_SELECTION_TOOL_H
