/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2017 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 * Copyright (C) 2017-2019 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#ifndef __SELECTION_TOOL_H
#define __SELECTION_TOOL_H

#include <memory>

#include <math/vector2d.h>
#include <tools/pcb_tool_base.h>
#include <tool/action_menu.h>
#include <tools/pcbnew_selection.h>
#include <tools/pcb_selection_conditions.h>
#include <tool/tool_menu.h>

class PCB_BASE_FRAME;
class BOARD_ITEM;
class GENERAL_COLLECTOR;

namespace KIGFX
{
    class GAL;
}


typedef void (*CLIENT_SELECTION_FILTER)( const VECTOR2I&, GENERAL_COLLECTOR& );


/**
 * Class SELECTION_TOOL
 *
 * Our sample selection tool: currently supports:
 * - pick single objects (click LMB)
 * - add objects to existing selection (Shift+LMB)
 * - draw selection box (drag LMB)
 * - handles MODULEs properly (i.e. selects either MODULE or its PADs, TEXTs, etc.)
 * - takes into account high-contrast & layer visibility settings
 * - invokes InteractiveEdit tool when user starts to drag selected items
 */
class SELECTION_TOOL : public PCB_TOOL_BASE
{
public:
    SELECTION_TOOL();
    ~SELECTION_TOOL();

    /// @copydoc TOOL_BASE::Init()
    bool Init() override;

    /// @copydoc TOOL_BASE::Reset()
    void Reset( RESET_REASON aReason ) override;

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
    PCBNEW_SELECTION& GetSelection();

    /**
     * Function RequestSelection()
     *
     * Returns the current selection set, filtered according to aFlags
     * and aClientFilter.
     * If the set is empty, performs the legacy-style hover selection.
     * @param aFiltered is an optional vector, that is filled with items removed by the filter
     */
    PCBNEW_SELECTION& RequestSelection( CLIENT_SELECTION_FILTER aClientFilter,
            std::vector<BOARD_ITEM*>* aFiltered = nullptr, bool aConfirmLockedItems = false );

    ///> Checks if the user has agreed to modify locked items for the given selection.
    SELECTION_LOCK_FLAGS CheckLock();

    ///> Select a single item under cursor event handler.
    int CursorSelection( const TOOL_EVENT& aEvent );

    ///> Clear current selection event handler.
    int ClearSelection( const TOOL_EVENT& aEvent );

    ///> Item selection event handler.
    int SelectItem( const TOOL_EVENT& aEvent );
    void AddItemToSel( BOARD_ITEM* aItem, bool aQuietMode = false );

    ///> Multiple item selection event handler
    int SelectItems( const TOOL_EVENT& aEvent );

    ///> Item unselection event handler.
    int UnselectItem( const TOOL_EVENT& aEvent );

    ///> Multiple item unselection event handler
    int UnselectItems( const TOOL_EVENT& aEvent );

    void BrightenItem( BOARD_ITEM* aItem );
    void UnbrightenItem( BOARD_ITEM* aItem );

    /**
     * Function selectable()
     * Checks conditions for an item to be selected.
     *
     * @return True if the item fulfills conditions to be selected.
     */
    bool Selectable( const BOARD_ITEM* aItem, bool checkVisibilityOnly = false ) const;

    /**
     * Function guessSelectionCandidates()
     * Tries to guess best selection candidates in case multiple items are clicked, by doing
     * some brain-dead heuristics.
     * @param aCollector is the collector that has a list of items to be queried.
     * @param aWhere is the selection point to consider
     */
    void GuessSelectionCandidates( GENERAL_COLLECTOR& aCollector, const VECTOR2I& aWhere ) const;

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

    ///> Sets up handlers for various events.
    void setTransitions() override;

    ///> Zooms the screen to center and fit the current selection.
    void zoomFitSelection();

private:
    /**
     * Function selectPoint()
     * Selects an item pointed by the parameter aWhere. If there is more than one item at that
     * place, there is a menu displayed that allows one to choose the item.
     *
     * @param aWhere is the place where the item should be selected.
     * @param aOnDrag indicates whether a drag operation is being performed.
     * @param aSelectionCancelledFlag allows the function to inform its caller that a selection
     * was cancelled (for instance, by clicking outside of the disambiguation menu).
     * @param aClientFilter allows the client to perform tool- or action-specific filtering.
     * @return True if an item was selected, false otherwise.
     */
    bool selectPoint( const VECTOR2I& aWhere, bool aOnDrag = false,
                      bool* aSelectionCancelledFlag = NULL,
                      CLIENT_SELECTION_FILTER aClientFilter = NULL );

    /**
     * Function selectCursor()
     * Selects an item under the cursor unless there is something already selected or aSelectAlways
     * is true.
     * @param aForceSelect forces to select an item even if there is an item already selected.
     * @param aClientFilter allows the client to perform tool- or action-specific filtering.
     * @return true if eventually there is an item selected, false otherwise.
     */
    bool selectCursor( bool aForceSelect = false,
                       CLIENT_SELECTION_FILTER aClientFilter = NULL );

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
     * @param aTitle (optional) Allows the menu to be titled (ie: "Clarify Selection").
     * @return true if an item was picked
     */
    bool doSelectionMenu( GENERAL_COLLECTOR* aItems, const wxString& aTitle );

    ///> Selects a trivial connection (between two junctions) of items in selection
    int selectConnection( const TOOL_EVENT& aEvent );

    ///> Expands the current selection to select a connection between two junctions
    int expandConnection( const TOOL_EVENT& aEvent );

    ///> Selects items with a continuous copper connection to items in selection
    int selectCopper( const TOOL_EVENT& aEvent );

    /**
     * Selects all copper connections belonging to the same net(s) as the
     * items in the selection
     */
    int selectNet( const TOOL_EVENT& aEvent );

    /**
     * Selects all items connected by copper tracks to the given TRACK
     * This selects tracks and vias but stops at pads
     */
    void selectAllItemsConnectedToTrack( TRACK& aSourceTrack );

    /**
     * Selects all items connected (by copper) to the given item
     * This selects tracks and vias _and_ continues through pads without selecting
     */
    void selectAllItemsConnectedToItem( BOARD_CONNECTED_ITEM& aSourceItem );

    /**
     * Selects all items with the given net code
     */
    void selectAllItemsOnNet( int aNetCode );

    /**
     * Selects all items with the given sheet timestamp name
     * (the sheet path)
     */
    void selectAllItemsOnSheet( wxString& aSheetpath );

    ///> Selects all modules belonging to same sheet, from Eeschema,
    ///> using crossprobing
    int selectSheetContents( const TOOL_EVENT& aEvent );

    ///> Selects all modules belonging to same hierarchical sheet
    ///> as the selected footprint.
    int selectSameSheet( const TOOL_EVENT& aEvent );

    ///> Find dialog callback.
    void findCallback( BOARD_ITEM* aItem );

    ///> Find an item.
    int find( const TOOL_EVENT& aEvent );

    ///> Invoke filter dialog and modify current selection
    int filterSelection( const TOOL_EVENT& aEvent );

    /**
     * Function clearSelection()
     * Clears the current selection.
     */
    void clearSelection();

    /**
     * Function pickSmallestComponent()
     * Allows one to find the smallest (in terms of bounding box area) item from the list.
     *
     * @param aCollector containes the list of items.
     */
    BOARD_ITEM* pickSmallestComponent( GENERAL_COLLECTOR* aCollector );

    /**
     * Function select()
     * Takes necessary action mark an item as selected.
     *
     * @param aItem is an item to be selected.
     */
    void select( BOARD_ITEM* aItem );

    /**
     * Function unselect()
     * Takes necessary action mark an item as unselected.
     *
     * @param aItem is an item to be unselected.
     */
    void unselect( BOARD_ITEM* aItem );

    /**
     * Function highlight()
     * Highlights the item visually.
     * @param aItem is an item to be be highlighted.
     * @param aHighlightMode should be either SELECTED or BRIGHTENED
     * @param aGroup is the group to add the item to in the BRIGHTENED mode.
     */
    void highlight( BOARD_ITEM* aItem, int aHighlightMode, PCBNEW_SELECTION* aGroup = nullptr );

    /**
     * Function unhighlight()
     * Unhighlights the item visually.
     * @param aItem is an item to be be highlighted.
     * @param aHighlightMode should be either SELECTED or BRIGHTENED
     * @param aGroup is the group to remove the item from.
     */
    void unhighlight( BOARD_ITEM* aItem, int aHighlightMode, PCBNEW_SELECTION* aGroup = nullptr );

    /**
     * Function selectionContains()
     * Checks if the given point is placed within any of selected items' bounding box.
     *
     * @return True if the given point is contained in any of selected items' bouding box.
     */
    bool selectionContains( const VECTOR2I& aPoint ) const;

    /**
     * Event handler to update the selection VIEW_ITEM.
     */
    int updateSelection( const TOOL_EVENT& aEvent );

    /**
     * Pass the selection to a conditional menu for updating.
     */
    int UpdateMenu( const TOOL_EVENT& aEvent );

    const GENERAL_COLLECTORS_GUIDE getCollectorsGuide() const;

private:
    PCB_BASE_FRAME*  m_frame;     // Pointer to the parent frame
    PCBNEW_SELECTION m_selection; // Current state of selection

    bool m_additive;              // Items should be added to selection (instead of replacing)
    bool m_subtractive;           // Items should be removed from selection
    bool m_exclusive_or;          // Items' selection state should be toggled
    bool m_multiple;              // Multiple selection mode is active
    bool m_skip_heuristics;       // Heuristics are not allowed when choosing item under cursor
    bool m_locked;                // Other tools are not allowed to modify locked items

    /// Private state (opaque pointer/compilation firewall)
    class PRIV;
    std::unique_ptr<PRIV> m_priv;
};

#endif
