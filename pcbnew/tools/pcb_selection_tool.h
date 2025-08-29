/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2017 CERN
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#ifndef PCB_SELECTION_TOOL_H
#define PCB_SELECTION_TOOL_H

#include <functional>
#include <memory>

#include <tool/actions.h>
#include <math/vector2d.h>
#include <project/board_project_settings.h>
#include <preview_items/selection_area.h>
#include <tool/action_menu.h>
#include <tool/selection_tool.h>
#include <tool/tool_menu.h>
#include <tools/pcb_selection_conditions.h>
#include <tools/pcb_tool_base.h>
#include <tools/pcb_selection.h>

class PCB_BASE_FRAME;
class BOARD_ITEM;
class GENERAL_COLLECTOR;
class PCB_TABLE;

namespace KIGFX
{
    class GAL;
}


/**
 * The selection tool: currently supports:
 * - pick single objects (click LMB)
 * - add objects to existing selection (Shift+LMB)
 * - draw selection box (drag LMB)
 * - handles FOOTPRINTs properly (i.e. selects either FOOTPRINT or its PADs, TEXTs, etc.)
 * - takes into account high-contrast & layer visibility settings
 * - invokes InteractiveEdit tool when user starts to drag selected items
 */
class PCB_SELECTION_TOOL : public SELECTION_TOOL
{
public:
    PCB_SELECTION_TOOL();
    ~PCB_SELECTION_TOOL();

    /// @copydoc TOOL_BASE::Init()
    bool Init() override;

    /// @copydoc TOOL_BASE::Reset()
    void Reset( RESET_REASON aReason ) override;

    void OnIdle( wxIdleEvent& aEvent );

    bool IsFootprintEditor()
    {
        return m_isFootprintEditor;
    }

    /**
     * The main loop.
     */
    int Main( const TOOL_EVENT& aEvent );

    /**
     * @return the set of currently selected items.
     */
    PCB_SELECTION& GetSelection();

    /**
     * Return the current selection, filtered according to aClientFilter.
     *
     * If the set is empty, performs the legacy-style hover selection.
     *
     * @param aClientFilter A callback to allow tool- or action-specific filtering.
     */
    PCB_SELECTION& RequestSelection( CLIENT_SELECTION_FILTER aClientFilter );

    ///< Select a single item under cursor event handler.
    int CursorSelection( const TOOL_EVENT& aEvent );

    int SelectColumns( const TOOL_EVENT& aEvent );
    int SelectRows( const TOOL_EVENT& aEvent );
    int SelectTable( const TOOL_EVENT& aEvent );

    ///< Clear current selection event handler.
    int ClearSelection( const TOOL_EVENT& aEvent );
    void ClearSelection( bool aQuietMode = false );

    ///< Select all items on the board
    int SelectAll( const TOOL_EVENT& aEvent );

    ///< Unselect all items on the board
    int UnselectAll( const TOOL_EVENT& aEvent );

    ///< Change the selection mode
    int SetSelectRect( const TOOL_EVENT& aEvent );
    int SetSelectPoly( const TOOL_EVENT& aEvent );

    /**
     * Handles drawing a selection box that allows multiple items to be selected simultaneously.
     *
     * @return true if the operation was canceled (i.e. a CancelEvent was received).
     */
    int SelectRectArea( const TOOL_EVENT& aEvent );

    /**
     * Handles drawing a lasso selection area that allows multiple items to be selected
     * simultaneously.
     *
     * @return true if the operation was canceled (i.e. a CancelEvent was received).
     */
    int SelectPolyArea( const TOOL_EVENT& aEvent );

    /**
     * Selects multiple PCB items within a specified area.
     */
    void SelectMultiple( KIGFX::PREVIEW::SELECTION_AREA& aArea, bool aSubtractive = false,
                         bool aExclusiveOr = false );

    /**
     * Take necessary actions to mark an item as found.
     *
     * @param aItem The item that was found and needs to be highlighted/focused/etc.
     */
    void FindItem( BOARD_ITEM* aItem );

    /**
     * Take necessary action mark an item as selected.
     *
     * @param aItem The item to be selected.
     */
    void select( EDA_ITEM* aItem ) override;

    /**
     * @return true if an item fulfills conditions to be selected.
     */
    bool Selectable( const BOARD_ITEM* aItem, bool checkVisibilityOnly = false ) const;

    /**
     * Select all items with the given net code.
     *
     * @param aNetCode is the target net to select
     * @param aSelect is true to add the items to the selection, false to remove them (deselect)
     */
    void SelectAllItemsOnNet( int aNetCode, bool aSelect = true );

    /**
     * Try to guess best selection candidates in case multiple items are clicked, by doing
     * some brain-dead heuristics.
     *
     * @param aCollector [in, out] The collector that has a list of items to be narrowed.
     * @param aWhere The selection point to consider.
     */
    void GuessSelectionCandidates( GENERAL_COLLECTOR& aCollector, const VECTOR2I& aWhere ) const;

    /**
     * Rebuild the selection from the EDA_ITEMs' selection flags.
     *
     * Commonly called after rolling back an undo state to make sure there aren't any stale
     * pointers.
     */
    void RebuildSelection();

    PCB_SELECTION_FILTER_OPTIONS& GetFilter()
    {
        return m_filter;
    }

    ///< Set up handlers for various events.
    void setTransitions() override;

    ///< Zoom the screen to center and fit the current selection.
    void zoomFitSelection();

    ///< Zoom the screen to fit the bounding box for cross probing/selection sync.
    void ZoomFitCrossProbeBBox( const BOX2I& bbox );

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
    PCB_GROUP* GetEnteredGroup() { return m_enteredGroup; }

    PCB_LAYER_ID GetActiveLayer() { return m_frame->GetActiveLayer(); }

    /**
     * In the PCB editor strip out any locked items unless the OverrideLocks checkbox is set.
     */
    void FilterCollectorForLockedItems( GENERAL_COLLECTOR& aCollector );

    /**
     * In general we don't want to select both a parent and any of it's children.  This includes
     * both footprints and their items, and groups and their members.
     */
    void FilterCollectorForHierarchy( GENERAL_COLLECTOR& aCollector, bool aMultiselect ) const;

    /**
     * Check the "allow free pads" setting and if disabled, replace any pads in the collector
     * with their parent footprints.
     */
    void FilterCollectorForFreePads( GENERAL_COLLECTOR& aCollector,
                                     bool aForcePromotion = false ) const;

    /**
     * Promote any table cell selections to the whole table.
     */
    void FilterCollectorForTableCells( GENERAL_COLLECTOR& aCollector ) const;

    /**
     * Drop any PCB_MARKERs from the collector.
     */
    void FilterCollectorForMarkers( GENERAL_COLLECTOR& aCollector ) const;

    /**
     * Apply the SELECTION_FITLER_OPTIONS to the collector.
     */
    void FilterCollectedItems( GENERAL_COLLECTOR& aCollector, bool aMultiSelect,
                               PCB_SELECTION_FILTER_OPTIONS* aRejected = nullptr );

    /**
     * Drop footprints that are not directly selected
    */
    void FilterCollectorForFootprints( GENERAL_COLLECTOR& aCollector,
                                       const VECTOR2I& aWhere ) const;

protected:
    KIGFX::PCB_VIEW* view() const
    {
        return static_cast<KIGFX::PCB_VIEW*>( getView() );
    }

    KIGFX::VIEW_CONTROLS* controls() const
    {
        return getViewControls();
    }

    PCB_BASE_FRAME* frame() const
    {
        return getEditFrame<PCB_BASE_FRAME>();
    }

    PCB_BASE_EDIT_FRAME* editFrame() const
    {
        return getEditFrame<PCB_BASE_EDIT_FRAME>();
    }

    BOARD* board() const
    {
        return getModel<BOARD>();
    }

    PCB_DRAW_PANEL_GAL* canvas() const
    {
        return static_cast<PCB_DRAW_PANEL_GAL*>( frame()->GetCanvas() );
    }

    virtual bool ctrlClickHighlights() override;

    SELECTION& selection() override { return m_selection; }

private:
    /**
     * Select an item pointed by the parameter \a aWhere.
     *
     * If there is more than one item at that place, there is a menu displayed that allows
     * one to choose the item.
     *
     * @param aWhere is the place where the item should be selected.
     * @param aOnDrag indicates whether a drag operation is being performed.
     * @param aSelectionCancelledFlag allows the function to inform its caller that a selection
     *                                was canceled (for instance, by clicking outside of the
     *                                disambiguation menu).
     * @param aClientFilter a callback to allow tool- or action-specific filtering.
     * @return whether or not the selection is empty.
     */
    bool selectPoint( const VECTOR2I& aWhere, bool aOnDrag = false,
                      bool* aSelectionCancelledFlag = nullptr,
                      CLIENT_SELECTION_FILTER aClientFilter = nullptr );

    /**
     * Select an item under the cursor unless there is something already selected.
     *
     * @param aForceSelect [optional] Forces an item to be selected even if there is already a
     *                     selection.
     * @param aClientFilter A callback to allow tool- or action-specific filtering.
     * @return whether or not the selection is empty.
     */
    bool selectCursor( bool aForceSelect = false,
                       CLIENT_SELECTION_FILTER aClientFilter = nullptr );

    bool selectTableCells( PCB_TABLE* aTable );

    /**
     * Handle disambiguation actions including displaying the menu.
     */
    int disambiguateCursor( const TOOL_EVENT& aEvent );

    /**
     * Expand the current connected-item selection to the next boundary (junctions, pads, or all)
     */
    int expandConnection( const TOOL_EVENT& aEvent );

    /**
     * Unroute the selected board connected items.
     */
    int unrouteSelected( const TOOL_EVENT& aEvent );

    /**
     * Unroute the selected track connected item.
     */
    int unrouteSegment( const TOOL_EVENT& aEvent );

    /**
     * Select all copper connections belonging to the same net(s) as the items in the selection.
     */
    int selectNet( const TOOL_EVENT& aEvent );

    /**
     * Select nearest unconnected footprints on same net as selected items.
     */
    int selectUnconnected( const TOOL_EVENT& aEvent );

    /**
     * Select and move other nearest footprint unconnected on same net as selected items.
     */
    int grabUnconnected( const TOOL_EVENT& aEvent );

    enum STOP_CONDITION
    {
        /**
         * Stop at any place where more than two traces meet.
         *
         * Because vias are also traces, this makes selection stop at a via if there is a trace
         * on another layer as well, but a via with only one connection will be selected.
         */
        STOP_AT_JUNCTION,
        /** Stop when reaching a segment (next track/arc/via). */
        STOP_AT_SEGMENT,
        /** Stop when reaching a pad. */
        STOP_AT_PAD,
        /** Select the entire net. */
        STOP_NEVER
    };

    /**
     * Select connected tracks and vias.
     *
     * @param aStopCondition Indicates where to stop selecting more items.
     */
    void selectAllConnectedTracks( const std::vector<BOARD_CONNECTED_ITEM*>& aStartItems,
                                   STOP_CONDITION aStopCondition );

    /**
     * Select all non-closed shapes that are graphically connected to the given start items.
     *
     * @param aStartItems is a list of one or more non-closed shapes
     */
    void selectAllConnectedShapes( const std::vector<PCB_SHAPE*>& aStartItems );

    /**
     * @return true if the given item is an open PCB_SHAPE on a non-copper layer
     */
    bool isExpandableGraphicShape( const EDA_ITEM* aItem ) const;

    /*
     * Select tracks and vias connected to specified board items.
     */
    void selectConnections( const std::vector<BOARD_ITEM*>& aItems );

    /**
     * Select all items with the given sheet timestamp/UUID name (the sheet path).
     *
     * The path of the root sheet is "/".
     */
    void selectAllItemsOnSheet( wxString& aSheetPath );

    ///< Select all footprints belonging to same sheet, from Eeschema using cross-probing.
    int selectSheetContents( const TOOL_EVENT& aEvent );

    ///< Select all footprints belonging to same hierarchical sheet as the selected footprint
    ///< (same sheet path).
    int selectSameSheet( const TOOL_EVENT& aEvent );

    ///< Set selection to items passed by parameter and connected nets (optionally).
    ///< Zooms to fit, if enabled
    int  syncSelection( const TOOL_EVENT& aEvent );
    int  syncSelectionWithNets( const TOOL_EVENT& aEvent );
    void doSyncSelection( const std::vector<BOARD_ITEM*>& aItems, bool aWithNets );

    ///< Invoke filter dialog and modify current selection
    int filterSelection( const TOOL_EVENT& aEvent );

    ///< Return true if the given item passes the current SELECTION_FILTER_OPTIONS.
    bool itemPassesFilter( BOARD_ITEM* aItem, bool aMultiSelect,
                           PCB_SELECTION_FILTER_OPTIONS* aRejected = nullptr );

    /**
     * Take necessary action mark an item as unselected.
     *
     * @param aItem is an item to be unselected.
     */
    void unselect( EDA_ITEM* aItem ) override;

    /**
     * Highlight the item visually.
     *
     * @param aItem The item to be highlighted.
     * @param aHighlightMode Either SELECTED or BRIGHTENED
     * @param aGroup [optional A group to add the item to.
     */
    void highlight( EDA_ITEM* aItem, int aHighlightMode, SELECTION* aGroup = nullptr ) override;

    /**
     * Unhighlight the item visually.
     *
     * @param aItem The item to be highlighted.
     * @param aHighlightMode Either SELECTED or BRIGHTENED
     * @param aGroup [optional] A group to remove the item from.
     */
    void unhighlight( EDA_ITEM* aItem, int aHighlightMode, SELECTION* aGroup = nullptr ) override;

    /**
     * @return true if the given point is contained in any of selected items' bounding boxes.
     */
    bool selectionContains( const VECTOR2I& aPoint ) const;

    /**
     * @return the distance from \a aWhere to \a aItem, up to and including \a aMaxDistance.
     */
    int hitTestDistance( const VECTOR2I& aWhere, BOARD_ITEM* aItem, int aMaxDistance ) const;

    /**
     * Event handler to update the selection VIEW_ITEM.
     */
    int updateSelection( const TOOL_EVENT& aEvent );

    void pruneObscuredSelectionCandidates( GENERAL_COLLECTOR& aCollector ) const;

    const GENERAL_COLLECTORS_GUIDE getCollectorsGuide() const;

private:
    void highlightInternal( EDA_ITEM* aItem, int aHighlightMode, bool aUsingOverlay );

    void unhighlightInternal( EDA_ITEM* aItem, int aHighlightMode, bool aUsingOverlay );

private:
    PCB_BASE_FRAME*          m_frame;                // Pointer to the parent frame
    bool                     m_isFootprintEditor;

    PCB_SELECTION            m_selection;            // Current state of selection

    PCB_SELECTION_FILTER_OPTIONS m_filter;

    KICURSOR                 m_nonModifiedCursor;    // Cursor in the absence of shift/ctrl/alt

    PCB_GROUP*               m_enteredGroup;         // If non-null, selections are limited to
                                                     // members of this group
    KIGFX::VIEW_GROUP        m_enteredGroupOverlay;  // Overlay for the entered group's frame.

    SELECTION_MODE           m_selectionMode;        // Current selection mode

    /// Private state (opaque pointer/compilation firewall)
    class PRIV;
    std::unique_ptr<PRIV>    m_priv;
};

#endif /* PCB_SELECTION_TOOL_H */
