/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright (C) 2019-2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <tool/selection_tool.h>
#include <tool/action_menu.h>
#include <tool/tool_menu.h>
#include <tools/ee_selection.h>
#include <ee_collectors.h>
#include <sch_symbol.h>
#include <gal/cursors.h>

class SCH_BASE_FRAME;
class SCH_ITEM;
class EE_GRID_HELPER;

namespace KIGFX
{
    class GAL;
}


class EE_CONDITIONS : public SELECTION_CONDITIONS
{
public:
    static SELECTION_CONDITION SingleSymbol;
    static SELECTION_CONDITION SingleSymbolOrPower;
    static SELECTION_CONDITION SingleDeMorganSymbol;
    static SELECTION_CONDITION SingleMultiUnitSymbol;
    static SELECTION_CONDITION SingleNonExcludedMarker;
    static SELECTION_CONDITION MultipleSymbolsOrPower;
};


class EE_SELECTION_TOOL : public SELECTION_TOOL
{
public:
    EE_SELECTION_TOOL();
    ~EE_SELECTION_TOOL();

    /// @copydoc TOOL_BASE::Init()
    bool Init() override;

    /// @copydoc TOOL_BASE::Reset()
    void Reset( RESET_REASON aReason ) override;

    /**
     * The main loop.
     */
    int Main( const TOOL_EVENT& aEvent );

    void OnIdle( wxIdleEvent& aEvent );

    ///< Zoom the screen to fit the bounding box for cross probing/selection sync.
    void ZoomFitCrossProbeBBox( const BOX2I& aBBox );

    /**
     * @return the set of currently selected items.
     */
    EE_SELECTION& GetSelection();

    /**
     * Return either an existing selection (filtered), or the selection at the current cursor
     * position if the existing selection is empty.
     *
     * @param aScanTypes [optional] List of item types that are acceptable for selection.
     * @return either the current selection or, if empty, the selection at the cursor.
     */
    EE_SELECTION& RequestSelection( const std::vector<KICAD_T>& aScanTypes = { SCH_LOCATE_ANY_T } );

    /**
     * Perform a click-type selection at a point (usually the cursor position).
     *
     * @param aWhere Point from which the selection should be made.
     * @param aScanTypes [optional] List of item types that are acceptable for selection.
     * @param aItem [out, optional] The newly selected item if only one was selected, otherwise
     *              unchanged.
     * @param aSelectionCancelledFlag [out] Allows the function to inform its caller that a
     *                                selection was canceled (for instance, by clicking outside of
     *                                the disambiguation menu).
     * @param aCheckLocked Indicates if locked items should be excluded.
     * @param aAdd Indicates if found item(s) should be added to the selection.
     * @param aSubtract Indicates if found item(s) should be subtracted from the selection.
     * @param aExclusiveOr Indicates if found item(s) should be toggle in the selection.
     * @return true if the selection was modified.
     */
    bool SelectPoint( const VECTOR2I& aWhere,
                      const std::vector<KICAD_T>& aScanTypes = { SCH_LOCATE_ANY_T },
                      EDA_ITEM** aItem = nullptr, bool* aSelectionCancelledFlag = nullptr,
                      bool aCheckLocked = false, bool aAdd = false, bool aSubtract = false,
                      bool aExclusiveOr = false );

    /**
     * Finds a connected item at a point (usually the cursor position).  Iterative process with a
     * decreasing slop factor.
     * @param aPosition Cursor position from which the search should be made.
     * @return a connected item or nullptr.
     */
    EDA_ITEM* GetNode( const VECTOR2I& aPosition );

    /**
     * Selects the connected item at the current cursor position.  Iterative process with a
     * decreasing slop factor.
     */
    int SelectNode( const TOOL_EVENT& aEvent );

    /**
     * If a connected item is selected then expand the selection to the entire connection,
     * otherwise select connection under the current cursor position.
     */
    int SelectConnection( const TOOL_EVENT& aEvent );

    ///< Clear current selection event handler.
    int ClearSelection( const TOOL_EVENT& aEvent );

    ///< Select all visible items in sheet
    int SelectAll( const TOOL_EVENT& aEvent );

    void ClearSelection( bool aQuietMode = false );

    /**
     * Check conditions for an item to be selected.
     *
     * @return True if the item fulfills conditions to be selected.
     */
    bool Selectable( const EDA_ITEM* aItem, const VECTOR2I* aPos = nullptr,
                     bool checkVisibilityOnly = false ) const;

    /**
     * Apply heuristics to try and determine a single object when multiple are found under the
     * cursor.
     */
    void GuessSelectionCandidates( EE_COLLECTOR& collector, const VECTOR2I& aPos );

    /**
     * Rebuild the selection from the EDA_ITEMs' selection flags.
     *
     * Commonly called after rolling back an undo state to make sure there aren't any stale
     * pointers.
     */
    void RebuildSelection();

    /**
     * Collect one or more items at a given point.  This method does not attempt to disambiguate
     * multiple items and is simply "collecting".
     *
     * @param aCollector [in, out] Provides collection conditions and stores collected items.
     * @param aWhere Point from which the collection should be made.
     * @param aScanTypes [optional] A list of item types that are acceptable for collection.
     */
    bool CollectHits( EE_COLLECTOR& aCollector, const VECTOR2I& aWhere,
                      const std::vector<KICAD_T>& aScanTypes = { SCH_LOCATE_ANY_T } );

    ///< Set selection to items passed by parameter.
    ///< Zooms to fit, if enabled.
    void SyncSelection( const std::optional<SCH_SHEET_PATH>& targetSheetPath, SCH_ITEM* focusItem,
                        const std::vector<SCH_ITEM*>& items );

protected:
    SELECTION& selection() override { return m_selection; }

private:
    OPT_TOOL_EVENT autostartEvent( TOOL_EVENT* aEvent, EE_GRID_HELPER& aGrid, SCH_ITEM* aItem );

    /**
     * Apply rules to narrow the collection down to selectable objects, and then heuristics
     * to try and narrow it to a single object.
     *
     * @param aCollector [in, out] Provides collection conditions and stores collected items.
     * @param aWhere point where we should narrow (if relevant)
     * @param aCheckLocked If false, remove locked elements from #collector
     * @param aSelectedOnly If true, remove non-selected items from #collector
     */
    void narrowSelection( EE_COLLECTOR& collector, const VECTOR2I& aWhere, bool aCheckLocked,
                          bool aSelectedOnly = false );

    /**
     * Perform a click-type selection at a point (usually the cursor position).
     *
     * @param aCollector [in, out] Provides collection conditions and stores collected items.
     * @param aWhere Point from which the selection should be made.
     * @param aItem [out, optional] The newly selected item if only one was selected, otherwise
     *              unchanged.
     * @param aSelectionCancelledFlag [out] Allows the function to inform its caller that a
     *                                selection was canceled (for instance, by clicking outside of
     *                                the disambiguation menu).
     * @param aAdd Indicates if found item(s) should be added to the selection.
     * @param aSubtract Indicates if found item(s) should be subtracted from the selection.
     * @param aExclusiveOr Indicates if found item(s) should be toggle in the selection.
     * @return true if the selection was modified.
     */
    bool selectPoint( EE_COLLECTOR& aCollector, const VECTOR2I& aWhere, EDA_ITEM** aItem = nullptr,
                      bool* aSelectionCancelledFlag = nullptr, bool aAdd = false,
                      bool aSubtract = false, bool aExclusiveOr = false );

    /**
     * Handle drawing a selection box that allows one to select many items at the same time.
     *
     * @return true if the function was canceled (i.e. CancelEvent was received).
     */
    bool selectMultiple();

    /**
     * Handle disambiguation actions including displaying the menu.
     */
    int disambiguateCursor( const TOOL_EVENT& aEvent );

    /**
     * Take necessary action to mark an item as selected.
     *
     * @param aItem The item to be selected.
     */
    void select( EDA_ITEM* aItem ) override;

    /**
     * Take necessary action to mark an item as unselected.
     *
     * @param aItem The item to be unselected.
     */
    void unselect( EDA_ITEM* aItem ) override;

    /**
     * Highlight the item visually.
     *
     * @param aItem The item to be highlighted.
     * @param aHighlightMode Either SELECTED or BRIGHTENED
     * @param aGroup [otpional] A group to add the item to.
     */
    void highlight( EDA_ITEM* aItem, int aHighlightMode, SELECTION* aGroup = nullptr ) override;

    /**
     * Unhighlight the item visually.
     *
     * @param aItem is an item to be highlighted.
     * @param aHighlightMode should be either SELECTED or BRIGHTENED
     * @param aGroup [optional] A group to remove the item from.
     */
    void unhighlight( EDA_ITEM* aItem, int aHighlightMode, SELECTION* aGroup = nullptr ) override;

    /**
     * Set the reference point to the anchor of the top-left item.
     */
    void updateReferencePoint();

    /**
     * @return true if the given point is contained in any of selected items' bounding boxes.
     */
    bool selectionContains( const VECTOR2I& aPoint ) const;

    ///< Set up handlers for various events.
    void setTransitions() override;

private:

    SCH_BASE_FRAME* m_frame;             // Pointer to the parent frame
    EE_SELECTION    m_selection;         // Current state of selection

    KICURSOR        m_nonModifiedCursor; // Cursor in the absence of shift/ctrl/alt

    bool            m_isSymbolEditor;    // True when the symbol editor is the parent frame
    bool            m_isSymbolViewer;    // True when the symbol browser is the parent frame
    int             m_unit;              // Fixed unit filter (for symbol editor)
    int             m_convert;           // Fixed DeMorgan filter (for symbol editor)
};

#endif //KICAD_SCH_SELECTION_TOOL_H
