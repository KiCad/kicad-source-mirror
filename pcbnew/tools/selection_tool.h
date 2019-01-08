/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2017 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 * Copyright (C) 2017 KiCad Developers, see CHANGELOG.TXT for contributors.
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
#include <tools/pcb_tool.h>
#include <tool/context_menu.h>
#include <tool/selection.h>

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
class SELECTION_TOOL : public PCB_TOOL
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
    SELECTION& GetSelection();

    /**
     * Function RequestSelection()
     *
     * Returns the current selection set, filtered according to aFlags
     * and aClientFilter.
     * If the set is empty, performs the legacy-style hover selection.
     * @param aFiltered is an optional vector, that is filled with items removed by the filter
     */
    SELECTION& RequestSelection( CLIENT_SELECTION_FILTER aClientFilter,
            std::vector<BOARD_ITEM*>* aFiltered = NULL  );


    inline TOOL_MENU& GetToolMenu()
    {
        return m_menu;
    }

    ///> Checks if the user has agreed to modify locked items for the given selection.
    SELECTION_LOCK_FLAGS CheckLock();

    ///> Select a single item under cursor event handler.
    int CursorSelection( const TOOL_EVENT& aEvent );

    ///> Clear current selection event handler.
    int ClearSelection( const TOOL_EVENT& aEvent );

    ///> Item selection event handler.
    int SelectItem( const TOOL_EVENT& aEvent );

    ///> Multiple item selection event handler
    int SelectItems( const TOOL_EVENT& aEvent );

    ///> Item unselection event handler.
    int UnselectItem( const TOOL_EVENT& aEvent );

    ///> Multiple item unselection event handler
    int UnselectItems( const TOOL_EVENT& aEvent );

    /**
     * Function SelectionMenu()
     * Allows the selection of a single item from a list of items via a popup menu.  The
     * list is passed as aEvent's parameter.
     */
    int SelectionMenu( const TOOL_EVENT& aEvent );

    ///> Event sent after an item is selected.
    static const TOOL_EVENT SelectedEvent;

    ///> Event sent after an item is unselected.
    static const TOOL_EVENT UnselectedEvent;

    ///> Event sent after selection is cleared.
    static const TOOL_EVENT ClearedEvent;

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
     * @param aSelectAlways forces to select an item even if there is an item already selected.
     * @param aClientFilter allows the client to perform tool- or action-specific filtering.
     * @return true if eventually there is an item selected, false otherwise.
     */
    bool selectCursor( bool aSelectAlways = false,
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
     * highlighted on the canvas when hovered in the menu.
     * @param aTitle (optional) Allows the menu to be titled (ie: "Clarify Selection").
     */
    BOARD_ITEM* doSelectionMenu( GENERAL_COLLECTOR* aItems, const wxString& aTitle );

    ///> Selects a trivial connection (between two junctions) of items in selection
    int selectConnection( const TOOL_EVENT& aEvent );

    ///> Expands the current selection to select a connection between two junctions
    int expandSelectedConnection( const TOOL_EVENT& aEvent );

    ///> Selects items with a continuous copper connection to items in selection
    int selectCopper( const TOOL_EVENT& aEvent );

    /**
     * Selects all copper connections belonging to the same net(s) as the
     * items in the selection
     */
    int selectNet( const TOOL_EVENT& aEvent );

    /**
     * Selects all items connected by copper tracks to the given TRACK
     */
    void selectAllItemsConnectedToTrack( TRACK& aSourceTrack );

    /**
     * Selects all items connected (by copper) to the given item
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
    int selectOnSheetFromEeschema( const TOOL_EVENT& aEvent );

    ///> Selects all modules belonging to same hierarchical sheet
    ///> as the selected footprint.
    int selectSameSheet( const TOOL_EVENT& aEvent );

    ///> Find dialog callback.
    void findCallback( BOARD_ITEM* aItem );

    ///> Find an item.
    int find( const TOOL_EVENT& aEvent );

    ///> Find an item and start moving.
    int findMove( const TOOL_EVENT& aEvent );

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
     * Function toggleSelection()
     * Changes selection status of a given item.
     *
     * @param aItem is the item to have selection status changed.
     * @param aForce causes the toggle to happen without checking selectability
     */
    void toggleSelection( BOARD_ITEM* aItem, bool aForce = false );

    /**
     * Function selectable()
     * Checks conditions for an item to be selected.
     *
     * @return True if the item fulfills conditions to be selected.
     */
    bool selectable( const BOARD_ITEM* aItem, bool checkVisibilityOnly = false ) const;

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
     * Function selectVisually()
     * Highlights the item visually.
     * @param aItem is an item to be be highlighted.
     * @param aHighlightMode should be either SELECTED or BRIGHTENED
     * @param aGroup is the group to add the item to in the BRIGHTENED mode.
     */
    void highlight( BOARD_ITEM* aItem, int aHighlightMode, SELECTION& aGroup );

    /**
     * Function unselectVisually()
     * Unhighlights the item visually.
     * @param aItem is an item to be be highlighted.
     * @param aHighlightMode should be either SELECTED or BRIGHTENED
     * @param aGroup is the group to remove the item from.
     */
    void unhighlight( BOARD_ITEM* aItem, int aHighlightMode, SELECTION& aGroup );

    /**
     * Function selectionContains()
     * Checks if the given point is placed within any of selected items' bounding box.
     *
     * @return True if the given point is contained in any of selected items' bouding box.
     */
    bool selectionContains( const VECTOR2I& aPoint ) const;

    /**
     * Function guessSelectionCandidates()
     * Tries to guess best selection candidates in case multiple items are clicked, by
     * doing some braindead heuristics.
     * @param aCollector is the collector that has a list of items to be queried.
     * @param aWhere is the selection point to consider
     */
    void guessSelectionCandidates( GENERAL_COLLECTOR& aCollector, const VECTOR2I& aWhere ) const;

    /**
     * Event handler to update the selection VIEW_ITEM.
     */
    int updateSelection( const TOOL_EVENT& aEvent );

    const GENERAL_COLLECTORS_GUIDE getCollectorsGuide() const;

    /// Pointer to the parent frame.
    PCB_BASE_FRAME* m_frame;

    /// Current state of selection.
    SELECTION m_selection;

    /// Flag saying if items should be added to the current selection or rather replace it.
    bool m_additive;

    /// Flag saying if items should be removed from the current selection
    bool m_subtractive;

    /// Flag saying if multiple selection mode is active.
    bool m_multiple;

    /// Flag saying that heuristics should be skipped while choosing selection
    bool m_skip_heuristics;

    /// Can other tools modify locked items.
    bool m_locked;

    /// Menu model displayed by the tool.
    TOOL_MENU m_menu;

    /// Private state (opaque pointer/compilation firewall)
    class PRIV;
    std::unique_ptr<PRIV> m_priv;
};

#endif
