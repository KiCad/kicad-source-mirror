/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2017 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GERBVIEW_SELECTION_TOOL_H
#define __GERBVIEW_SELECTION_TOOL_H

#include <memory>
#include <math/vector2d.h>

#include <tool/tool_interactive.h>
#include <tool/context_menu.h>
#include <tool/selection.h>
#include <tool/selection_conditions.h>
#include <tool/tool_menu.h>

#include <gerbview_frame.h>

class SELECTION_AREA;
class GERBER_COLLECTOR;

namespace KIGFX
{
    class GAL;
}


/**
 * Class GERBVIEW_SELECTION_TOOL
 *
 * Selection tool for GerbView, based on the one in PcbNew
 */
class GERBVIEW_SELECTION_TOOL : public TOOL_INTERACTIVE
{
public:
    GERBVIEW_SELECTION_TOOL();
    ~GERBVIEW_SELECTION_TOOL();

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

    inline TOOL_MENU& GetToolMenu()
    {
        return m_menu;
    }

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

    ///> Launches a tool to measure between points
    int MeasureTool( const TOOL_EVENT& aEvent );

    ///> Event sent after an item is selected.
    static const TOOL_EVENT SelectedEvent;

    ///> Event sent after an item is unselected.
    static const TOOL_EVENT UnselectedEvent;

    ///> Event sent after selection is cleared.
    static const TOOL_EVENT ClearedEvent;

    ///> Sets up handlers for various events.
    void setTransitions() override;

    ///> Zooms the screen to center and fit the current selection.
    void zoomFitSelection( void );

private:
    /**
     * Function selectPoint()
     * Selects an item pointed by the parameter aWhere. If there is more than
     * one item at that place, there is a menu displayed that allows one to
     * choose the item.
     *
     * @param aWhere is the place where the item should be selected.
     * @param aAllowDisambiguation decides what to do in case of disambiguation. If true, then
     * a menu is shown, otherise function finishes without selecting anything.
     * @return True if an item was selected, false otherwise.
     */
    bool selectPoint( const VECTOR2I& aWhere, bool aOnDrag = false );

    /**
     * Function selectCursor()
     * Selects an item under the cursor unless there is something already selected or aSelectAlways
     * is true.
     * @param aSelectAlways forces to select an item even if there is an item already selected.
     * @return true if eventually there is an item selected, false otherwise.
     */
    bool selectCursor( bool aSelectAlways = false );

    /**
     * Function selectMultiple()
     * Handles drawing a selection box that allows one to select many items at
     * the same time.
     *
     * @return true if the function was cancelled (i.e. CancelEvent was received).
     */
    bool selectMultiple();

    /**
     * Function clearSelection()
     * Clears the current selection.
     */
    void clearSelection();

    /**
     * Function disambiguationMenu()
     * Handles the menu that allows one to select one of many items in case
     * there is more than one item at the selected point (@see selectCursor()).
     *
     * @param aItems contains list of items that are displayed to the user.
     */
    EDA_ITEM* disambiguationMenu( GERBER_COLLECTOR* aItems );

    /**
     * Function toggleSelection()
     * Changes selection status of a given item.
     *
     * @param aItem is the item to have selection status changed.
     */
    void toggleSelection( EDA_ITEM* aItem );

    /**
     * Function selectable()
     * Checks conditions for an item to be selected.
     *
     * @return True if the item fulfills conditions to be selected.
     */
    bool selectable( const EDA_ITEM* aItem ) const;

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
     * Function selectVisually()
     * Marks item as selected, but does not add it to the ITEMS_PICKED_LIST.
     * @param aItem is an item to be be marked.
     */
    void selectVisually( EDA_ITEM* aItem );

    /**
     * Function unselectVisually()
     * Marks item as selected, but does not add it to the ITEMS_PICKED_LIST.
     * @param aItem is an item to be be marked.
     */
    void unselectVisually( EDA_ITEM* aItem );

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
     */
    void guessSelectionCandidates( GERBER_COLLECTOR& aCollector ) const;

    /// Pointer to the parent frame.
    GERBVIEW_FRAME* m_frame;

    /// Current state of selection.
    SELECTION m_selection;

    /// Flag saying if items should be added to the current selection or rather replace it.
    bool m_additive;

    /// Flag saying if items should be removed from the current selection
    bool m_subtractive;

    /// Flag saying if multiple selection mode is active.
    bool m_multiple;

    /// Determines if the selection is preliminary or final.
    bool m_preliminary;

    /// Menu model displayed by the tool.
    TOOL_MENU m_menu;
};

#endif
