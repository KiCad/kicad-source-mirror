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

#ifndef SCH_SELECTION_TOOL_H
#define SCH_SELECTION_TOOL_H

#include <project/sch_project_settings.h>
#include <tool/selection_tool.h>
#include <tool/action_menu.h>
#include <tool/tool_menu.h>
#include <tools/sch_selection.h>
#include <sch_collectors.h>
#include <sch_symbol.h>
#include <gal/cursors.h>

class SCH_BASE_FRAME;
class SCH_GROUP;
class SCH_ITEM;
class SCH_TABLE;
class EE_GRID_HELPER;
class SCH_TABLECELL;

namespace KIGFX
{
class GAL;

namespace PREVIEW
{
class SELECTION_AREA;
}
}


class SCH_CONDITIONS : public SELECTION_CONDITIONS
{
public:
    static SELECTION_CONDITION SingleSymbol;
    static SELECTION_CONDITION SingleSymbolOrPower;
    static SELECTION_CONDITION SingleMultiBodyStyleSymbol;
    static SELECTION_CONDITION SingleMultiUnitSymbol;
    static SELECTION_CONDITION SingleMultiFunctionPin;
    static SELECTION_CONDITION SingleNonExcludedMarker;
    static SELECTION_CONDITION MultipleSymbolsOrPower;
    static SELECTION_CONDITION AllPins;
    static SELECTION_CONDITION AllPinsOrSheetPins;
};


class SCH_SELECTION_TOOL : public SELECTION_TOOL
{
public:
    SCH_SELECTION_TOOL();
    ~SCH_SELECTION_TOOL();

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
     * Enter the group at the head of the current selection.
     */
    void EnterGroup() override;

    /**
     * Leave the currently-entered group.
     *
     * @param aSelectGroup [optional] Select the group after leaving.
     */
    void ExitGroup( bool aSelectGroup = false ) override;

    /**
     * @return the currently-entered group.
     */
    SCH_GROUP* GetEnteredGroup() { return m_enteredGroup; }

    /**
     * @return the set of currently selected items.
     */
    SCH_SELECTION& GetSelection();

    /**
     * Return either an existing selection (filtered), or the selection at the current cursor
     * position if the existing selection is empty.
     *
     * @param aScanTypes [optional] List of item types that are acceptable for selection.
     * @return either the current selection or, if empty, the selection at the cursor.
     *
     * @param aPromoteCellSelections [optional] If true, cell selections are promoted to their parent
     *
     * @param aPromoteGroups [optional] If true, group selections are promoted the items within the group
     */
    SCH_SELECTION& RequestSelection( const std::vector<KICAD_T>& aScanTypes = { SCH_LOCATE_ANY_T },
                                     bool aPromoteCellSelections = false,
                                     bool aPromoteGroups = false );

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
    bool SelectPoint( const VECTOR2I& aWhere, const std::vector<KICAD_T>& aScanTypes = { SCH_LOCATE_ANY_T },
                      EDA_ITEM** aItem = nullptr, bool* aSelectionCancelledFlag = nullptr, bool aCheckLocked = false,
                      bool aAdd = false, bool aSubtract = false, bool aExclusiveOr = false );

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

    int SelectColumns( const TOOL_EVENT& aEvent );
    int SelectRows( const TOOL_EVENT& aEvent );
    int SelectTable( const TOOL_EVENT& aEvent );

    ///< Clear current selection event handler.
    int ClearSelection( const TOOL_EVENT& aEvent );

    ///< Select all visible items in sheet
    int SelectAll( const TOOL_EVENT& aEvent );

    ///< Unselect all visible items in sheet
    int UnselectAll( const TOOL_EVENT& aEvent );

    ///< Select next net item
    int SelectNext( const TOOL_EVENT& aEvent );

    ///< Select previous net item
    int SelectPrevious( const TOOL_EVENT& aEvent );

    void ClearSelection( bool aQuietMode = false );

    /**
     * Check conditions for an item to be selected.
     *
     * @return True if the item fulfills conditions to be selected.
     */
    bool Selectable( const EDA_ITEM* aItem, const VECTOR2I* aPos = nullptr, bool checkVisibilityOnly = false ) const;

    /**
     * Apply heuristics to try and determine a single object when multiple are found under the
     * cursor.
     */
    void GuessSelectionCandidates( SCH_COLLECTOR& collector, const VECTOR2I& aPos );

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
    bool CollectHits( SCH_COLLECTOR& aCollector, const VECTOR2I& aWhere,
                      const std::vector<KICAD_T>& aScanTypes = { SCH_LOCATE_ANY_T } );

    ///< Set selection to items passed by parameter.
    ///< Zooms to fit, if enabled.
    void SyncSelection( const std::optional<SCH_SHEET_PATH>& targetSheetPath, SCH_ITEM* focusItem,
                        const std::vector<SCH_ITEM*>& items );

    SCH_SELECTION_FILTER_OPTIONS& GetFilter() { return m_filter; }

protected:
    SELECTION& selection() override { return m_selection; }

private:
    OPT_TOOL_EVENT autostartEvent( TOOL_EVENT* aEvent, EE_GRID_HELPER& aGrid, SCH_ITEM* aItem );

    std::set<SCH_ITEM*> expandConnectionWithGraph( const SCH_SELECTION& aItems );
    std::set<SCH_ITEM*> expandConnectionGraphically( const SCH_SELECTION& aItems );

    /**
     * Apply rules to narrow the collection down to selectable objects, and then heuristics
     * to try and narrow it to a single object.
     *
     * @param aCollector [in, out] Provides collection conditions and stores collected items.
     * @param aWhere point where we should narrow (if relevant)
     * @param aCheckLocked If false, remove locked elements from #collector
     * @param aSelectedOnly If true, remove non-selected items from #collector
     */
    void narrowSelection( SCH_COLLECTOR& collector, const VECTOR2I& aWhere, bool aCheckLocked,
                          bool aSelectedOnly = false,
                          SCH_SELECTION_FILTER_OPTIONS* aRejected = nullptr );

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
    bool selectPoint( SCH_COLLECTOR& aCollector, const VECTOR2I& aWhere, EDA_ITEM** aItem = nullptr,
                      bool* aSelectionCancelledFlag = nullptr, bool aAdd = false, bool aSubtract = false,
                      bool aExclusiveOr = false );

    /**
     * Handle drawing a selection box that allows one to select many items at the same time.
     *
     * @return true if the function was canceled (i.e. CancelEvent was received).
     */
    bool selectMultiple();

    bool selectLasso();

    int SetSelectRect( const TOOL_EVENT& aEvent );
    int SetSelectPoly( const TOOL_EVENT& aEvent );

    void SelectMultiple( KIGFX::PREVIEW::SELECTION_AREA& aArea, bool aSubtractive = false,
                         bool aExclusiveOr = false );

    /**
     * Handle a table cell drag selection within a table.
     *
     * @return true if the function was canceled (i.e. CancelEvent was received).
     */
    bool selectTableCells( SCH_TABLE* aTable );

    /**
     * Initialize the selection state of table cells.
     */
    void InitializeSelectionState( SCH_TABLE* aTable );

    /**
     * Select table cells within a rectangular area between two points.
     *
     * @param start The starting point of the rectangular selection area.
     * @param end The ending point of the rectangular selection area.
     * @param aTable The table containing the cells to check and update.
     */
    void SelectCellsBetween( const VECTOR2D& start, const VECTOR2D& end, SCH_TABLE* aTable );

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
     * @param aMode Either SELECTED or BRIGHTENED
     * @param aGroup [otpional] A group to add the item to.
     */
    void highlight( EDA_ITEM* aItem, int aMode, SELECTION* aGroup = nullptr ) override;

    /**
     * Unhighlight the item visually.
     *
     * @param aItem is an item to be highlighted.
     * @param aMode should be either SELECTED or BRIGHTENED
     * @param aGroup [optional] A group to remove the item from.
     */
    void unhighlight( EDA_ITEM* aItem, int aMode, SELECTION* aGroup = nullptr ) override;

    /**
     * Set the reference point to the anchor of the top-left item.
     */
    void updateReferencePoint();

    /**
     * @return true if the given point is contained in any of selected items' bounding boxes.
     */
    bool selectionContains( const VECTOR2I& aPoint ) const;

    /**
     * Event handler to update the selection VIEW_ITEM.
     */
    int updateSelection( const TOOL_EVENT& aEvent );

    /**
     * Return true if the given item passes the stateful selection filter
     */
    bool itemPassesFilter( EDA_ITEM* aItem, SCH_SELECTION_FILTER_OPTIONS* aRejected = nullptr );

    /**
     * In general we don't want to select both a parent and any of it's children.  This includes
     * both symbols and their items, and groups and their members.
     */
    void filterCollectorForHierarchy( SCH_COLLECTOR& aCollector, bool aMultiselect ) const;
    void filterCollectedItems( SCH_COLLECTOR& aCollector, bool aMultiSelect );

    ///< Set up handlers for various events.
    void setTransitions() override;

private:
    SCH_BASE_FRAME* m_frame;     // Pointer to the parent frame
    SCH_SELECTION   m_selection; // Current state of selection

    KICURSOR m_nonModifiedCursor; // Cursor in the absence of shift/ctrl/alt

    bool m_isSymbolEditor; // True when the symbol editor is the parent frame
    bool m_isSymbolViewer; // True when the symbol browser is the parent frame
    int  m_unit;           // Fixed unit filter (for symbol editor)
    int  m_bodyStyle;      // Fixed DeMorgan filter (for symbol editor)

    SCH_GROUP*        m_enteredGroup;         // If non-null, selections are limited to
                                              // members of this group
    KIGFX::VIEW_GROUP m_enteredGroupOverlay;  // Overlay for the entered group's frame.

    SCH_SELECTION_FILTER_OPTIONS m_filter;

    SELECTION_MODE m_selectionMode; // Current selection mode

    SCH_TABLECELL* m_previous_first_cell; // First selected cell for shift+click selection range
};

#endif //SCH_SELECTION_TOOL_H
