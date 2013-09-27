/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
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

#ifndef __SELECTION_TOOL_H
#define __SELECTION_TOOL_H

#include <set>

#include <math/vector2d.h>
#include <tool/tool_interactive.h>
#include <tool/context_menu.h>

class SELECTION_AREA;
class BOARD_ITEM;
class GENERAL_COLLECTOR;

/**
 * Class SELECTION_TOOL
 *
 * Our sample selection tool: currently supports:
 * - pick single objects (click LMB)
 * - add objects to existing selection (Shift+LMB)
 * - draw selection box (drag LMB)
 * - handles MODULEs properly (ie. selects either MODULE or its PADs, TEXTs, etc.)
 * - takes into account high-contrast & layer visibility settings
 * - invokes InteractiveMove tool when user starts to drag selected items
 */

class SELECTION_TOOL : public TOOL_INTERACTIVE
{
public:
    SELECTION_TOOL();
    ~SELECTION_TOOL();

    /// @copydoc TOOL_INTERACTIVE::Reset()
    void Reset();

    /**
     * Function Main()
     *
     * The main loop.
     */
    int Main( TOOL_EVENT& aEvent );

    /**
     * Function GetSelection()
     *
     * Returns the set of currently selected items.
     */
    const std::set<BOARD_ITEM*>& GetSelection() const
    {
        return m_selectedItems;
    }

    /**
     * Function AddAction()
     *
     * Adds a menu entry to run a TOOL_ACTION on selected items.
     * @param aAction is a menu entry to be added.
     */
    void AddMenuItem( const TOOL_ACTION& aAction );

private:
    /**
     * Function selectSingle()
     * Selects an item pointed by the parameter aWhere. If there is more than one item at that
     * place, there is a menu displayed that allows to choose the item.
     *
     * @param aWhere is the place where the item should be selected.
     */
    void selectSingle( const VECTOR2I& aWhere );

    /**
     * Function selectMultiple()
     * Handles drawing a selection box that allows to select many items at the same time.
     *
     * @return true if the function was cancelled (ie. CancelEvent was received).
     */
    bool selectMultiple();

    /**
     * Function disambiguationMenu()
     * Handles the menu that allows to select one of many items in case there is more than one
     * item at the selected point (@see selectSingle()).
     *
     * @param aItems contains list of items that are displayed to the user.
     */
    BOARD_ITEM* disambiguationMenu( GENERAL_COLLECTOR* aItems );

    /**
     * Function pickSmallestComponent()
     * Allows to find the smallest (in terms of bounding box area) item from the list.
     *
     * @param aCollector containes the list of items.
     */
    BOARD_ITEM* pickSmallestComponent( GENERAL_COLLECTOR* aCollector );

    /**
     * Function toggleSelection()
     * Changes selection status of a given item.
     *
     * @param aItem is the item to have selection status changed.
     */
    void toggleSelection( BOARD_ITEM* aItem );

    /**
     * Function clearSelection()
     * Clears selections of currently selected items.
     */
    void clearSelection();

    /**
     * Function selectable()
     * Checks conditions for an item to be selected.
     *
     * @return True if the item fulfills conditions to be selected.
     */
    bool selectable( const BOARD_ITEM* aItem ) const;

    /**
     * Function selectItem()
     * Takes necessary action mark an item as selected.
     *
     * @param aItem is an item to be selected.
     */
    void selectItem( BOARD_ITEM* aItem )
    {
        aItem->SetSelected();
        m_selectedItems.insert( aItem );

        // Now the context menu should be enabled
        SetContextMenu( &m_menu, CMENU_BUTTON );
    }

    /**
     * Function deselectItem()
     * Takes necessary action mark an item as deselected.
     *
     * @param aItem is an item to be deselected.
     */
    void deselectItem( BOARD_ITEM* aItem )
    {
        aItem->ClearSelected();
        m_selectedItems.erase( aItem );

        if( m_selectedItems.empty() )
            // If there is nothing selected, disable the context menu
            SetContextMenu( &m_menu, CMENU_OFF );
    }

    /**
     * Function containsSelected()
     * Checks if the given point is placed within any of selected items' bounding box.
     *
     * @return True if the given point is contained in any of selected items' bouding box.
     */
    bool containsSelected( const VECTOR2I& aPoint ) const;

    /// Container storing currently selected items
    std::set<BOARD_ITEM*> m_selectedItems;

    /// Visual representation of selection box
    SELECTION_AREA* m_selArea;

    /// Flag saying if items should be added to the current selection or rather replace it
    bool m_additive;

    /// Flag saying if multiple selection mode is active
    bool m_multiple;

    /// Right click popup menu
    CONTEXT_MENU m_menu;
};

#endif
