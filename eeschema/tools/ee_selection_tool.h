/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef KICAD_SCH_SELECTION_TOOL_H
#define KICAD_SCH_SELECTION_TOOL_H

#include <tool/tool_interactive.h>
#include <tool/action_menu.h>
#include <tool/tool_menu.h>
#include <tools/ee_selection.h>
#include <ee_collectors.h>
#include <sch_component.h>

class SCH_BASE_FRAME;
class SCH_ITEM;

namespace KIGFX
{
    class GAL;
}


class EE_CONDITIONS : public SELECTION_CONDITIONS
{
public:
    static SELECTION_CONDITION Empty;
    static SELECTION_CONDITION Idle;
    static SELECTION_CONDITION IdleSelection;

    static SELECTION_CONDITION SingleSymbol;
    static SELECTION_CONDITION SingleDeMorganSymbol;
    static SELECTION_CONDITION SingleMultiUnitSymbol;
};


class EE_SELECTION_TOOL : public TOOL_INTERACTIVE
{
public:
    EE_SELECTION_TOOL();
    ~EE_SELECTION_TOOL();

    /// @copydoc TOOL_BASE::Init()
    bool Init() override;

    /// @copydoc TOOL_BASE::Reset()
    void Reset( RESET_REASON aReason ) override;

    int UpdateMenu( const TOOL_EVENT& aEvent );

    /**
     * Function Main()
     *
     * The main loop.
     */
    int Main( const TOOL_EVENT& aEvent );

    /**
     * Function GetSelection()
     *
     * Returns the set of currently selected items.
     */
    EE_SELECTION& GetSelection();

    /**
     * Function RequestSelection()
     *
     * Returns either an existing selection (filtered), or the selection at the current
     * cursor if the existing selection is empty.
     */
    EE_SELECTION& RequestSelection( const KICAD_T* aFilterList = EE_COLLECTOR::AllItems );

    /**
     * Function selectPoint()
     * Selects an item pointed by the parameter aWhere. If there is more than one item at that
     * place, there is a menu displayed that allows one to choose the item.
     *
     * @param aWhere is the place where the item should be selected.
     * @param aSelectionCancelledFlag allows the function to inform its caller that a selection
     * was cancelled (for instance, by clicking outside of the disambiguation menu).
     * @param aCheckLocked indicates if locked items should be excluded
     */
    EDA_ITEM* SelectPoint( const VECTOR2I& aWhere,
                           const KICAD_T* aFilterList = EE_COLLECTOR::AllItems,
                           bool* aSelectionCancelledFlag = NULL, bool aCheckLocked = false,
                           bool aAdd = false, bool aSubtract = false, bool aExclusiveOr = false );

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

    ///> Find (but don't select) node under cursor
    EDA_ITEM* GetNode( VECTOR2I aPosition );

    ///> Select node under cursor
    int SelectNode( const TOOL_EVENT& aEvent );

    ///> If node selected then expand to connection, otherwise select connection under cursor
    int SelectConnection( const TOOL_EVENT& aEvent );

    ///> Clear current selection event handler.
    int ClearSelection( const TOOL_EVENT& aEvent );

    void ClearSelection();

    /**
     * Function Selectable()
     * Checks conditions for an item to be selected.
     * @return True if the item fulfills conditions to be selected.
     */
    bool Selectable( const EDA_ITEM* aItem, bool checkVisibilityOnly = false ) const;

    /**
     * Apply heuristics to try and determine a single object when multiple are found under the
     * cursor.
     */
    void GuessSelectionCandidates( EE_COLLECTOR& collector, const VECTOR2I& aPos );

    /**
     * Function SelectionMenu()
     * Shows a popup menu to trim the COLLECTOR passed as aEvent's parameter down to a single
     * item.
     *
     * NOTE: this routine DOES NOT modify the selection.
     */
    int SelectionMenu( const TOOL_EVENT& aEvent );

    /**
     * Rebuilds the selection from the EDA_ITEMs' selection flags.  Commonly called after
     * rolling back an undo state to make sure there aren't any stale pointers.
     */
    void RebuildSelection();

private:
    /**
     * Function selectMultiple()
     * Handles drawing a selection box that allows one to select many items at
     * the same time.
     *
     * @return true if the function was cancelled (i.e. CancelEvent was received).
     */
    bool selectMultiple();

    /**
     * Allows the selection of a single item from a list via pop-up menu.  The items are
     * highlighted on the canvas when hovered in the menu.  The collector is trimmed to
     * the picked item.
     * @return true if an item was picked
     */
    bool doSelectionMenu( EE_COLLECTOR* aItems );

    /**
     * Function select()
     * Takes necessary action mark an item as selected.
     *
     * @param aItem is an item to be selected.
     */
    void select( EDA_ITEM* aItem );

    /**
     * Function unselect()
     * Takes necessary action mark an item as unselected.
     *
     * @param aItem is an item to be unselected.
     */
    void unselect( EDA_ITEM* aItem );

    /**
     * Function highlight()
     * Highlights the item visually.
     * @param aItem is an item to be be highlighted.
     * @param aHighlightMode should be either SELECTED or BRIGHTENED
     * @param aGroup is the group to add the item to in the BRIGHTENED mode.
     */
    void highlight( EDA_ITEM* aItem, int aHighlightMode, EE_SELECTION* aGroup = nullptr );

    /**
     * Function unhighlight()
     * Unhighlights the item visually.
     * @param aItem is an item to be be highlighted.
     * @param aHighlightMode should be either SELECTED or BRIGHTENED
     * @param aGroup is the group to remove the item from.
     */
    void unhighlight( EDA_ITEM* aItem, int aHighlightMode, EE_SELECTION* aGroup = nullptr );

    /**
     * Sets the reference point to the anchor of the top-left item.
     */
    void updateReferencePoint();

    /**
     * Function selectionContains()
     * Checks if the given point is placed within any of selected items' bounding box.
     *
     * @return True if the given point is contained in any of selected items' bouding box.
     */
    bool selectionContains( const VECTOR2I& aPoint ) const;

    ///> Sets up handlers for various events.
    void setTransitions() override;

private:
    SCH_BASE_FRAME* m_frame;     // Pointer to the parent frame
    EE_SELECTION    m_selection; // Current state of selection

    bool m_additive;             // Items should be added to selection (instead of replacing)
    bool m_subtractive;          // Items should be removed from selection
    bool m_exclusive_or;         // Items' selection state should be toggled
    bool m_multiple;             // Multiple selection mode is active
    bool m_skip_heuristics;      // Heuristics are not allowed when choosing item under cursor

    bool m_isLibEdit;            // True when libedit is the parent frame
    bool m_isLibView;            // True when libview is the parent frame
    int  m_unit;                 // Fixed unit filter (for symbol editor)
    int  m_convert;              // Fixed DeMorgan filter (for symbol editor)
};

#endif //KICAD_SCH_SELECTION_TOOL_H
