/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
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

#ifndef __MOVE_TOOL_H
#define __MOVE_TOOL_H

#include <math/vector2d.h>
#include <tool/tool_interactive.h>
#include <tool/item_state.h>
#include <view/view_group.h>

class BOARD_ITEM;
class SELECTION_TOOL;

namespace KiGfx
{
class VIEW_GROUP;
}

/**
 * Class MOVE_TOOL
 *
 * Our sample move tool. Allows to move, rotate and flip items selected by
 * pcbnew.InteractiveSelection tool.
 */

class MOVE_TOOL : public TOOL_INTERACTIVE
{
public:
    MOVE_TOOL();
    ~MOVE_TOOL();

    /**
     * Function Reset()
     *
     * Resets the tool and initializes it.
     */
    void Reset();

    /**
     * Function Main()
     *
     * Main loop in which events are handled.
     */
    int Main( TOOL_EVENT& aEvent );

private:
    /// Adds an item to the VIEW_GROUP that holds all moved items and displays them on the overlay
    void viewGroupAdd( BOARD_ITEM* aItem, KiGfx::VIEW_GROUP* aGroup );

    /// Changes visibility settings for items stored in a VIEW_GROUP
    void vgSetVisibility( KiGfx::VIEW_GROUP* aGroup, bool aVisible ) const;

    /// Updates items stored in a VIEW_GROUP with selected update flag
    void vgUpdate( KiGfx::VIEW_GROUP* aGroup, KiGfx::VIEW_ITEM::ViewUpdateFlags aFlags ) const;

    /// Saves the state of items and allows to restore them
    ITEM_STATE m_state;

    /// Selection tool used for obtaining selected items
    SELECTION_TOOL* m_selectionTool;

    /// Set of selected items (obtained from pcbnew.InteractiveSelection tool)
    std::set<BOARD_ITEM*> m_selection;

    /// VIEW_GROUP that helds currently moved items
    KiGfx::VIEW_GROUP m_items;

    /// Register hotkey for activation of the move tool
    TOOL_ACTION m_activate;

    /// Register hotkey for rotation of selected objects
    TOOL_ACTION m_rotate;

    /// Register hotkey for flipping of selected objects
    TOOL_ACTION m_flip;
};

#endif
